#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bench.h"

#define N (1*1000*1000)
#define MAXTHREADS 16


struct pingpong {
	long ping;
	long *cache[0] __attribute__((aligned(128)));
	long pong;
} *pingpong;



void ping(struct thrarg *arg)
{
	(void)arg;
	long ping_val, pong_val;
	long *ping = &pingpong->ping;
	long *pong = &pingpong->pong;
	long n = arg->params.iters;

	ping_val = __atomic_load_n(ping, __ATOMIC_RELAXED);
	assert(ping_val == 0);
	do {
		ping_val += 1;
		__atomic_store_n(ping, ping_val, __ATOMIC_RELEASE);
		do {
			pong_val = __atomic_load_n(pong, __ATOMIC_ACQUIRE);
		} while (pong_val != ping_val);
	} while (pong_val != n);
}

void pong(struct thrarg *arg)
{
	(void)arg;
	long ping_val, pong_val;
	long *ping = &pingpong->ping;
	long *pong = &pingpong->pong;
	long n = arg->params.iters;

	pong_val = __atomic_load_n(pong, __ATOMIC_RELAXED);
	assert(pong_val == 0);
	do {
		do {
			ping_val = __atomic_load_n(ping, __ATOMIC_ACQUIRE);
		} while (ping_val == pong_val);
		pong_val += 1;
		__atomic_store_n(pong, pong_val, __ATOMIC_RELEASE);
	} while (ping_val != n);
}


void benchmark_ping(struct thrarg *arg)
{
	if (arg->params.id)
		pong(arg);
	else
		ping(arg);
}

void init(struct thrarg *arg)
{
	(void)arg;
	memset(pingpong, 0, sizeof(struct pingpong));
}

int main(int argc, char **argv)
{
	unsigned nthreads = 2;
	(void)argc; (void)argv;

	if (posix_memalign((void **)&pingpong, 128, sizeof(struct pingpong))) {
		perror("posix_memalign");
		return 1;
	}

	memset(pingpong, 0, sizeof(struct pingpong));

	struct thrarg thrarg = { .params = {
		.threads = nthreads,
		.benchmark = benchmark_ping,
		.init = init,
		.print_samples = true,
		.min_time =  10*1000*1000,
	}};

	int err = benchmark_auto(&thrarg);
	if (err) {
		fprintf(stderr, "Bench error %s\n", strerror(err));
		return 1;
	}
	printf("%.4f\n", thrarg.result.avg);

	return 0;
}
