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


#define N (1*1000*1000)
#define MAXTHREADS 16


#ifdef EXCLUSIVE
#define mbox_writer_begin mbox_trylock
#define mbox_writer_end mbox_unlock

#define mbox_reader_begin mbox_trylock
#define mbox_reader_end mbox_unlock
#else
#define mbox_writer_begin mbox_writer_acquire
#define mbox_writer_end mbox_writer_release

#define mbox_reader_begin mbox_reader_acquire
#define mbox_reader_end mbox_reader_release
#endif

struct message
{
	size_t count;
	char message[32];
};

struct mbox {
	unsigned lock;
	void *cache1[0] __attribute__((aligned(64)));
	size_t received;
	void *cache2[0] __attribute__((aligned(64)));
	size_t sent;
	void *cache3[0] __attribute__((aligned(64)));
	struct message message;
}__attribute__((aligned(64))) *mbox, A, B;


#if 0
static bool mbox_writer_acquire(struct mbox *mbx)
{
	return __atomic_load_n(&mbx->sent, __ATOMIC_ACQUIRE) == 0;
}

static void mbox_writer_release(struct mbox *mbx)
{
	__atomic_fetch_add(&mbx->sent, 1, __ATOMIC_RELEASE);
}

static bool mbox_reader_acquire(struct mbox *mbx)
{
	return __atomic_load_n(&mbx->sent, __ATOMIC_ACQUIRE) == 1;
}

static void mbox_reader_release(struct mbox *mbx)
{
	__atomic_fetch_sub(&mbx->sent, 1, __ATOMIC_RELEASE);
}
#endif

#if 0

#ifdef EXCLUSIVE
static bool mbox_trylock(struct mbox *mbx)
{
	size_t sent = __atomic_load_n(&mbx->sent, __ATOMIC_RELAXED);
	if (sent != 0)
		return false;
	bool success =  __atomic_compare_exchange_n(&mbx->sent, &sent, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
	return success;
}

static void mbox_unlock(struct mbox *mbx)
{
	assert_eq(mbx->sent, 1);
	__atomic_store_n(&mbx->sent, 0, __ATOMIC_RELEASE);
}
#else
static bool mbox_writer_acquire(struct mbox *mbx)
{
	size_t received;

	received  = __atomic_load_n(&mbx->received, __ATOMIC_ACQUIRE);

	return mbx->sent == received;
}

static void mbox_writer_release(struct mbox *mbx)
{
	__atomic_store_n(&mbx->sent, mbx->sent + 1, __ATOMIC_RELEASE);
}

static bool mbox_reader_acquire(struct mbox *mbx)
{
	size_t sent;
	sent = __atomic_load_n(&mbx->sent, __ATOMIC_ACQUIRE);

	return sent > mbx->received;
}

static void mbox_reader_release(struct mbox *mbx)
{
	__atomic_store_n(&mbx->received, mbx->received + 1, __ATOMIC_RELEASE);
}
#endif
#endif

#if 0
bool mbox_send(struct mbox *mbx, struct message *msg)
{
	if (mbox_writer_begin(mbx)) {
		assert_eq(mbx->sent,1);
		mbx->message = *msg;
		mbox_writer_end(mbx);
		return true;
	}
	return false;
}

bool mbox_receive(struct mbox *mbx, struct message *msg)
{
	if (mbox_reader_begin(mbx)) {
		*msg =  mbx->message;
		mbox_reader_end(mbx);
		return true;
	}
	return false;
}
#endif

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


bool mbox_send(struct mbox *mbx, struct message *msg)
{
	bool success = false;

	if (mbox_lock(mbx)) {
		assert_eq(mbx->lock,1);
		if (mbx->sent == mbx->received) {
			mbx->message = *msg;
			mbx->sent++;
			success = true;
		}
		mbox_unlock(mbx);
	}
	return success;
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
