#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"

#define assert(x) if(__builtin_expect((x), 0)) __builtin_trap()
#define assert_eq(x,y) \
	do { \
		if((x) != (y)) { \
			printf("ERROR %s %ld != %ld\n", #x, (long)(x), (long)(y)); \
			__builtin_trap(); \
		} \
	} while (0)


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
	struct message messages[RING_LEN];
}__attribute__((aligned(64))) Ring;



static bool mbox_lock(struct mbox *mbx)
{
#if 0
	unsigned lock = 0;
	while(!__atomic_compare_exchange_n(
				&mbx->lock, &lock, 1, false,
				__ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		while ((lock = __atomic_load_n(&mbx->lock, __ATOMIC_ACQUIRE)) != 0)
			;
#else
	while (__atomic_test_and_set(&mbx->lock, __ATOMIC_ACQUIRE))
		while ((__atomic_load_n(&mbx->lock, __ATOMIC_ACQUIRE)) != 0)
			;
#endif
	return true;
}

static void mbox_unlock(struct mbox *mbx)
{
	assert_eq(mbx->lock, 1);
#if 0
//	__atomic_store_n(&mbx->lock, 0, __ATOMIC_RELEASE);
#else
	__atomic_clear(&mbx->lock,__ATOMIC_RELEASE);
#endif
}


bool ring_send(struct ring *r, size_t n, struct message msg[n])
{
	bool success = false;
	unsigned received, sent, len, space;

	sent = r->sent;
	len = r->len;
	received  = __atomic_load_n(&r->received, __ATOMIC_ACQUIRE);
	space = sent - received + n;
	if (space > len)
		return false;

	}
	
	for (size_t i = 0; i < n; i++)
		r->messages[(sent+i) & len] = msg[i];

	__atomic_store_n(&r->sent, space, __ATOMIC_RELEASE);

	return true;
}


bool mbox_receive(struct mbox *mbx, struct message *msg)
{
	bool success = false;

	if (mbox_lock(mbx)) {
		assert_eq(mbx->lock,1);
		if (mbx->sent > mbx->received) {
			*msg = mbx->message;
			mbx->received++;
			success = true;
		}
		mbox_unlock(mbx);
	}
	return success;
}

void send(size_t n)
{
	struct message msg = {.count = 0};

	for (size_t i = 0; i < n; i++) {
		msg.count = i;
		while (!mbox_send(&A, &msg))
			;
		while (!mbox_receive(&B, &msg))
			;
		assert_eq(msg.count,i);
		printf("%zu\n", i);
	}
}

void recv(size_t n)
{
	struct message msg;

	for (size_t i = 0; i < n; i++) {
		while (!mbox_receive(&A, &msg))
			;
		assert_eq(msg.count,i);
		printf("%zu\n", i);
		while (!mbox_send(&B, &msg))
			;
	}
}

void benchmark_ping(struct thrarg *arg)
{
	if (arg->params.id)
		send(arg->params.iters);
	else
		recv(arg->params.iters);
}

void init(struct thrarg *arg)
{
	(void)arg;
	memset(mbox, 0, sizeof(struct mbox));
	memset(&A, 0, sizeof(struct mbox));
	memset(&B, 0, sizeof(struct mbox));
}

int main(int argc, char **argv)
{
	unsigned nthreads = 2;
	(void)argc; (void)argv;

	if (posix_memalign((void **)&mbox, 64, sizeof(struct mbox))) {
		perror("posix_memalign");
		return 1;
	}

	memset(mbox, 0, sizeof(struct mbox));

	struct thrarg thrarg = { .params = {
		.threads = nthreads,
		.benchmark = benchmark_ping,
		.init = init,
		.min_time =  10*1000,
		.max_samples =  10,
		.iters = 10,
	}};

//	int err = benchmark_auto(&thrarg);
	int err = benchmark_once(&thrarg);
	if (err < 0) {
		fprintf(stderr, "Bench error %s\n", strerror(err));
		return 1;
	}
	printf("%.2f %.2f %.2f\n", thrarg.result.avg, thrarg.result.err, thrarg.result.u);

	return 0;
}
