#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sched.h>
#include <string.h>

#define CLOCK_TYPE CLOCK_MONOTONIC_RAW

uint64_t getclock()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_TYPE, &ts) != 0) {
		perror("clock_gettime");
		exit(1);
	}
	return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

#define N (50*1000*1000)
#define MAXTHREADS 16
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))


struct pingpong {
	long ping;
	long *cache[0] __attribute__((aligned(128)));
	long pong;
} *pingpong;



static pthread_barrier_t barrier;


void benchmark_ping(void)
{
	long ping_val, pong_val;
	long *ping = &pingpong->ping;
	long *pong = &pingpong->pong;

	pthread_barrier_wait(&barrier);
	ping_val = __atomic_load_n(ping, __ATOMIC_RELAXED);
	assert(ping_val == 0);
	do {
		ping_val += 1;
		__atomic_store_n(ping, ping_val, __ATOMIC_RELAXED);
		do {
			pong_val = __atomic_load_n(pong, __ATOMIC_RELAXED);
		} while (pong_val == ping_val);
	} while (pong_val != N);
}

void *benchmark_pong(void *arg)
{
	(void)arg;
	long ping_val, pong_val;
	long *ping = &pingpong->ping;
	long *pong = &pingpong->pong;

	pthread_barrier_wait(&barrier);
	pong_val = __atomic_load_n(pong, __ATOMIC_RELAXED);
	assert(pong_val == 0);
	do {
		do {
			ping_val = __atomic_load_n(ping, __ATOMIC_RELAXED);
		} while (ping_val - pong_val == 1);
		pong_val += 1;
		__atomic_store_n(pong, pong_val, __ATOMIC_RELAXED);
	} while (pong_val != N);

	return NULL;
}

void benchmark_ping_sc(void)
{
	long ping_val, pong_val;
	long *ping = &pingpong->ping;
	long *pong = &pingpong->pong;

	pthread_barrier_wait(&barrier);
	ping_val = __atomic_load_n(ping, __ATOMIC_RELAXED);
	assert(ping_val == 0);
	do {
		ping_val = __atomic_add_fetch(ping, 1, __ATOMIC_SEQ_CST);
		do {
			pong_val = __atomic_load_n(pong, __ATOMIC_RELAXED);
		} while (pong_val == ping_val);
	} while (pong_val != N);
}

void *benchmark_pong_sc(void *arg)
{
	(void)arg;
	long ping_val, pong_val;
	long *ping = &pingpong->ping;
	long *pong = &pingpong->pong;

	pthread_barrier_wait(&barrier);
	pong_val = __atomic_load_n(pong, __ATOMIC_RELAXED);
	assert(pong_val == 0);
	do {
		do {
			ping_val = __atomic_load_n(ping, __ATOMIC_RELAXED);
		} while (ping_val - pong_val == 1);
		pong_val = __atomic_add_fetch(pong, 1, __ATOMIC_SEQ_CST);
	} while (pong_val != N);

	return NULL;
}


int main(int argc, char **argv) 
{
	unsigned nthreads = 2;
	pthread_t threads[MAXTHREADS];
	pthread_attr_t attr;
	cpu_set_t c;
	float time;
	uint64_t t1, t2;
	unsigned i;
	(void)argc; (void)argv;

	if (posix_memalign((void **)&pingpong, 128, sizeof(struct pingpong))) {
		perror("posix_memalign");
		return 1;
	}

	pthread_barrier_init(&barrier, NULL, nthreads);
	pthread_attr_init(&attr);

	memset(pingpong, 0, sizeof(struct pingpong));

	for (i = 1; i < nthreads; i++) {
		CPU_ZERO(&c);
		CPU_SET(i, &c);
		pthread_attr_setaffinity_np(&attr, sizeof(c), &c);
		pthread_create(&threads[i], &attr, benchmark_pong_sc, NULL);
	}

	CPU_ZERO(&c);
	CPU_SET(0, &c);
	pthread_setaffinity_np(pthread_self(), sizeof(c), &c);

	t1 = getclock();
	benchmark_ping_sc();
	t2 = getclock();

	time = (t2 - t1)/N*1.0f;
	printf("%.2f\n", time);

	for (i = 1; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}
