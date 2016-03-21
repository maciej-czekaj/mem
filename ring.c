#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sched.h>

#include "bench.h"

#define assert(x) if(__builtin_expect((x), 0)) __builtin_trap()
#define assert_eq(x,y) \
	do { \
		if((x) != (y)) { \
			printf("ERROR %s %ld != %ld\n", #x, (long)(x), (long)(y)); \
			__builtin_trap(); \
		} \
	} while (0)

#define RING_LEN 32
//#define WITH_LOCK
//#define EXCLUSIVE true

struct message
{
	size_t count;
};

struct ring {
	unsigned lock;
	void *cache1[0] __attribute__((aligned(64)));
	unsigned received;
	void *cache2[0] __attribute__((aligned(64)));
	unsigned sent;
	void *cache3[0] __attribute__((aligned(64)));
	unsigned len;
	struct message messages[0];
}__attribute__((aligned(64))) *R;


//#define CAS
#ifdef WITH_LOCK
static void lock(unsigned *lock)
{
#ifdef CAS
	unsigned lock_val = 0;
	while(!__atomic_compare_exchange_n(
				lock, &lock_val, 1, false,
				__ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		while ((lock_val = __atomic_load_n(lock, __ATOMIC_ACQUIRE)) != 0)
			;
#else
	do {
		while ((__atomic_load_n(lock, __ATOMIC_RELAXED)) != 0)
			;
			//sched_yield();
	} while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE));
#endif
}

static void unlock(unsigned *lock)
{
	assert_eq(*lock, 1);
#ifdef CAS
	__atomic_store_n(lock, 0, __ATOMIC_RELEASE);
#else
	__atomic_clear(lock,__ATOMIC_RELEASE);
#endif
}

static bool ring_send(struct ring *r, size_t n, struct message msg[n], bool excl)
{
	unsigned received, sent, len, space, mask;

	if (excl)
		lock(&r->lock);

	sent = r->sent;
	len = r->len;
	mask = len - 1;
	if (excl)
		received = r->received;
	else
		received  = __atomic_load_n(&r->received, __ATOMIC_ACQUIRE);
	space = sent - received + n;
	if (space > len) {
		if (excl)
			unlock(&r->lock);
		return false;
	}
	for (size_t i = 0; i < n; i++)
		r->messages[(sent+i) & mask] = msg[i];

	if (excl) {
		r->sent += n;
		unlock(&r->lock);
	}
	else {
		__atomic_store_n(&r->sent, sent + n, __ATOMIC_RELEASE);
	}
	return true;
}


bool ring_receive(struct ring *r, size_t n, struct message msg[n], bool excl)
{
	unsigned received, sent, len, space, mask;

	if (excl)
		lock(&r->lock);
	received = r->received;
	len = r->len;
	mask = len - 1;
	if (excl)
		sent = r->sent;
	else
		sent = __atomic_load_n(&r->sent, __ATOMIC_ACQUIRE);
	space = sent - received;
	if (space < n) {
		if (excl)
			unlock(&r->lock);
		return false;
	}

	for (size_t i = 0; i < n; i++)
		msg[i] = r->messages[(received+i) & mask];

	if (excl) {
		r->received += n;
		unlock(&r->lock);
	} else {
		__atomic_store_n(&r->received, received + n, __ATOMIC_RELEASE);
	}
	return true;
}

#else
bool ring_send(struct ring *r, size_t n, struct message msg[n])
{
	unsigned received, sent, len, space, mask;

	sent = r->sent;
	len = r->len;
	mask = len - 1;
	received  = __atomic_load_n(&r->received, __ATOMIC_ACQUIRE);
	space = sent - received + n;
	if (space > len)
		return false;

	for (size_t i = 0; i < n; i++)
		r->messages[(sent+i) & mask] = msg[i];

	__atomic_store_n(&r->sent, sent + n, __ATOMIC_RELEASE);

	return true;
}


bool ring_receive(struct ring *r, size_t n, struct message msg[n])
{
	unsigned received, sent, len, space, mask;

	received = r->received;
	len = r->len;
	mask = len - 1;
	sent  = __atomic_load_n(&r->sent, __ATOMIC_ACQUIRE);
	space = sent - received;
	if (space < n)
		return false;

	for (size_t i = 0; i < n; i++)
		msg[i] = r->messages[(received+i) & mask];

	__atomic_store_n(&r->received, received + n, __ATOMIC_RELEASE);

	return true;
}
#endif

void send(size_t n)
{
	struct message msg = {.count = 0};

	for (size_t i = 0; i < n; i++) {
		msg.count = i;
		while (!ring_send(R, 1, &msg))
			;
		//printf("%zu\n", i);
	}
}

void recv(size_t n)
{
	struct message msg;

	for (size_t i = 0; i < n; i++) {
		while (!ring_receive(R, 1, &msg))
			;
		assert_eq(msg.count,i);
		//printf("%zu\n", i);
	}
}

void benchmark_ping(struct thrarg *arg)
{
	if (arg->params.id)
		send(arg->params.iters);
	else
		recv(arg->params.iters);
}

void ring_reset(struct ring *r)
{
	r->sent = 0;
	r->received = 0;
	r->lock = 0;
}

struct ring *ring_new(size_t ring_len)
{
	struct ring *r = NULL;
	if (posix_memalign((void **)&r, 64, sizeof(struct ring) + ring_len * sizeof(struct message))) {
		perror("posix_memalign");
		exit(1);
	}
	ring_reset(r);
	r->len = ring_len;
	return r;
}

void init(struct thrarg *arg)
{
	(void)arg;
	ring_reset(R);
}

void usage()
{
	fprintf(stderr, "Usage:\n\tring [ <size> ]\n");
}

int main(int argc, char **argv)
{
	unsigned nthreads = 2;

	unsigned len = RING_LEN;

	if (argc == 2 && sscanf(argv[1],"%u", &len) != 1) {
		usage();
		return 1;
	}

	if (argc > 2) {
		usage();
		return 1;
	}

	R = ring_new(len);

	struct thrarg thrarg = { .params = {
		.threads = nthreads,
		.benchmark = benchmark_ping,
		.init = init,
		.min_time =  100*1000,
		.max_samples =  30,
		.max_error = 10,
		.iters = 10,
	}};

	int err = benchmark_auto(&thrarg);
//	int err = benchmark_once(&thrarg);
	if (err < 0) {
		fprintf(stderr, "Bench error %s\n", strerror(err));
		return 1;
	}
	printf("%.2f %.2f %.2f\n", thrarg.result.avg, thrarg.result.err, thrarg.result.u);

	return 0;
}
