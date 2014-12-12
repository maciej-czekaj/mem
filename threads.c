#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sched.h>

#define CLOCK_TYPE CLOCK_MONOTONIC_RAW

uint64_t getclock()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_TYPE, &ts) != 0) {
		perror("clock_gettime");
		exit(1);
	}
	return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

#define N (500*1000*1000)

static pthread_barrier_t barrier;

void benchmark(long *p)
{
	unsigned i;

	for (i=0; i < N; i++) {
		*(volatile long *)p += 1;
		//asm volatile ("" :: "m" (*p));
	}
}

static void *thread(void *arg)
{
	long *p = arg;
	uint64_t t1, t2;

//	printf("%p\n", p);

	pthread_barrier_wait(&barrier);	

	t1 = getclock();

	benchmark(p);
	
	t2=getclock();

	return (void *)(t2 - t1);
}

#define MAXTHREADS 16

int main(int argc, char **argv) 
{
	char *mem;
	unsigned i;
	unsigned nthreads = 2;
	unsigned pad;
	pthread_t threads[MAXTHREADS];
	pthread_attr_t attr;
	cpu_set_t c;
	uint64_t time;

	if (argc < 2 || sscanf(argv[1], "%u", &pad) < 1)
		return 1;

	if (posix_memalign((void **)&mem, 64, nthreads*pad)) {
		perror("posix_memalign");
		return 1;
	}

	pthread_barrier_init(&barrier, NULL, nthreads);
	pthread_attr_init(&attr);

	for (i = 1; i < nthreads; i++) {
		CPU_ZERO(&c);
		CPU_SET(i, &c);
		pthread_attr_setaffinity_np(&attr, sizeof(c), &c);
		mem[i] = 0;
		pthread_create(&threads[i], &attr, thread, &mem[i*pad]);
	}
	
	CPU_ZERO(&c);
	CPU_SET(0, &c);
	pthread_setaffinity_np(pthread_self(), sizeof(c), &c);
	mem[0] = 0;

	time = (uint64_t) thread(&mem[0]);

	printf("%.2f\n", (time*1.0)/N);

	assert(*(long *)&mem[0] == N);

	for (i = 1; i < nthreads; i++) {
		pthread_join(threads[i], (void **)&time);
		assert(*(long *)&mem[i*pad] == N);
		printf("%.2f\n", (time*1.0)/N);
	}
		
	return 0;
}
