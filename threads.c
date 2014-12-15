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
#define MAXTHREADS 16

static pthread_barrier_t barrier;
static char *mem;
static long (*benchmark)(long *);

long benchmark_v(long *p)
{
	unsigned i;

	for (i=0; i < N; i++) {
		*(volatile long *)p += 1;
	}
	return N;
}

long benchmark_s(long *p)
{
	unsigned i;
	unsigned n = N/10;

	for (i=0; i < n; i++) {
		__sync_add_and_fetch(p, 10);
	}
	return n;
}

long benchmark_r(long *p)
{
	unsigned i;
	long r;

	for (i=0; i < N; i++) {
		if (p == (long *)mem)
			*(volatile long *)p += 1;
		else
			r = *(volatile long *)p;
	}
	(void)r;
	return N;
}

static void *thread(void *arg)
{
	long *p = arg;
	uint64_t t1, t2;
	long n;
	float *res;

	pthread_barrier_wait(&barrier);	

	t1 = getclock();

	n = benchmark(p);
	
	t2=getclock();

	res = malloc(sizeof(float));
	*res = (t2 - t1)/(n*1.0);

	return (void *)res;
}


int main(int argc, char **argv) 
{
	unsigned i;
	unsigned nthreads = 2;
	unsigned pad;
	pthread_t threads[MAXTHREADS];
	pthread_attr_t attr;
	cpu_set_t c;
	float *time;

	if (argc < 3)
		return 1;

	if ( sscanf(argv[1], "%u", &pad) < 1)
		return 1;

	switch(argv[2][0]) {
	
	case 'v':
		benchmark = benchmark_v;
		break;
	case 's':
		benchmark = benchmark_s;
		break;
	case 'r':
		benchmark = benchmark_r;
		break;	default:
		benchmark = NULL;
	}

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

	time = (float *) thread(&mem[0]);

	printf("%.2f\n", *time);

	for (i = 1; i < nthreads; i++) {
		pthread_join(threads[i], (void **)&time);
		printf("%.2f\n", *time);
	}
		
	return 0;
}
