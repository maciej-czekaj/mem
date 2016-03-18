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



struct mbox {
	unsigned lock;
	unsigned count;
	unsigned a[2];
}__attribute__((aligned(64))) mbox;




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


void mbox_inc(struct mbox *mbx, size_t id)
{
	mbox_lock(mbx);
	assert_eq(mbx->lock,1);
	mbx->count++;
	mbx->a[id]++;
	mbox_unlock(mbx);
}


void inc(size_t n, size_t id)
{
	for (size_t i = 0; i < n; i++) {
		mbox_inc(&mbox, id);
	}
	printf("a = %u, b = %u\n", mbox.a[0], mbox.a[1]);
}


void benchmark_ping(struct thrarg *arg)
{
	inc(arg->params.iters,arg->params.id);
}

void init(struct thrarg *arg)
{
	(void)arg;
	memset(&mbox, 0, sizeof(struct mbox));
}

int main(int argc, char **argv)
{
	unsigned nthreads = 2;
	(void)argc; (void)argv;

	struct thrarg thrarg = { .params = {
		.threads = nthreads,
		.benchmark = benchmark_ping,
		.init = init,
		.min_time =  100*1000,
//		.max_samples =  10,
//		.iters = 10,
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
