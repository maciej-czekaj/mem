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

#define N (500*1000*1000)
#define MAXTHREADS 16
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

struct list {
	struct list *next;
	long val;
};

struct thrarg {
	double res;
	void *arg;
};

static pthread_barrier_t barrier;
static char *mem;
static long (*benchmark)(struct list*);

long benchmark_v(struct list *l)
{
	unsigned i;

	for (i=0; i < N; i++) {
		l->val += 1;
		l = l->next;
	}
	return N;
}

long benchmark_a(struct list *l)
{
	unsigned i;
	long r;

	for (i=0; i < N; i++) {
		r = __atomic_load_n(&l->val, __ATOMIC_RELAXED);
		__atomic_store_n(&l->val, r + 10, __ATOMIC_RELAXED);
	}
	return N;
}

long benchmark_s(struct list *l)
{
	unsigned i;
	unsigned n = N/10;

	for (i=0; i < n; i++) {
		__atomic_fetch_add(&l->val, 10, __ATOMIC_RELAXED);
	}
	return n;
}

long benchmark_r(struct list *l)
{
	unsigned i;
	long r;

	for (i=0; i < N; i++) {
		if (l == (struct list *)mem) {
			r = __atomic_load_n(&l->val, __ATOMIC_RELAXED);
			__atomic_store_n(&l->val, r + 10, __ATOMIC_RELAXED);
		} else {
			r = __atomic_load_n(&l->val, __ATOMIC_RELAXED);
		}
	}
	(void)r;
	return N;
}

static void *thread(void *arg)
{
	uint64_t t1, t2;
	long n;
	struct thrarg *thrarg = (struct thrarg *)arg;

	pthread_barrier_wait(&barrier);	

	t1 = getclock();

	n = benchmark(thrarg->arg);

	t2=getclock();

	thrarg->res = (t2 - t1)/(n*1.0);

	return NULL;
}


int main(int argc, char **argv) 
{
	unsigned i;
	unsigned nthreads = 2;
	unsigned pad;
	pthread_t threads[MAXTHREADS];
	struct thrarg thrargs[MAXTHREADS];
	pthread_attr_t attr;
	cpu_set_t c;

	if (argc < 3)
		return 1;

	if ( sscanf(argv[1], "%u", &pad) < 1)
		return 1;

	if (pad < sizeof(struct list))
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
		break;
	case 'a':
		benchmark = benchmark_a;
		break;
	default:
		benchmark = NULL;
	}

	if (posix_memalign((void **)&mem, 128, nthreads*pad)) {
		perror("posix_memalign");
		return 1;
	}

	pthread_barrier_init(&barrier, NULL, nthreads);
	pthread_attr_init(&attr);

	memset(mem, 0, nthreads*pad);
	for (i=0; i < nthreads; i++) {
		struct list *l = (struct list *)&mem[i*pad];
		l->next = l;
	}

	for (i = 0; i < nthreads; i++) {
		CPU_ZERO(&c);
		CPU_SET(i, &c);
		pthread_attr_setaffinity_np(&attr, sizeof(c), &c);
		thrargs[i].arg = &mem[i*pad];
		pthread_create(&threads[i], &attr, thread, &thrargs[i]);
	}

	for (i = 0; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
		printf(" %.2f", thrargs[i].res);
	}
	printf("\n");

	return 0;
}
