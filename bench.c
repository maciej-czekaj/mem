#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sched.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "bench.h"

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

#define MAXTHREADS 16

struct bench_shared {
	uint32_t barrier;
	struct thrarg thrargs[MAXTHREADS];
};

struct bench_shared *shared;

static void *alloc_shared(size_t size)
{
	void *ptr = mmap(NULL, size,
		PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if (ptr == NULL) {
		perror("mmap");
		exit(1);
	}
	return ptr;
}

__attribute__((unused))
static void free_shared(void *ptr, size_t size)
{
	munmap(ptr, size);
}

static void barrier_wait(uint32_t *barrier)
{
	uint32_t val = __atomic_sub_fetch(barrier, 1, __ATOMIC_RELAXED);
	while (val != 0)
		val = __atomic_load_n(barrier, __ATOMIC_RELAXED);
}

static void *thread(void *arg)
{
	uint64_t t1, t2;
	size_t tid = (size_t)arg;
	struct thrarg *thrarg = &shared->thrargs[tid];

	thrarg->init(thrarg);

	barrier_wait(&shared->barrier);

	t1 = getclock();

	thrarg->benchmark(thrarg);

	t2=getclock();

	thrarg->res = (t2 - t1)/(double)(thrarg->iters);
	thrarg->sum = (t2 - t1);
	return NULL;
}

void benchmark_once_thread(struct thrarg *thrarg, unsigned iters)
{
	const int new_thread = 0;
	size_t i;
	unsigned nthreads = thrarg->threads;
	struct thrarg *thrargs;
	pthread_t threads[nthreads];
	pthread_attr_t attr;
	cpu_set_t c;

	thrarg->iters = iters;

	if (!shared)
		shared = alloc_shared(sizeof(struct bench_shared));

	thrargs = shared->thrargs;
	shared->barrier = nthreads;

	pthread_attr_init(&attr);

	for (i=0; i < nthreads; i++) {
		thrargs[i] = *thrarg;
		thrargs[i].id = i;
	}

	i = (new_thread) ? 0 : 1;
	for (; i < nthreads; i++) {
		CPU_ZERO(&c);
		CPU_SET(i, &c);
		pthread_attr_setaffinity_np(&attr, sizeof(c), &c);
		pthread_create(&threads[i], &attr, thread, (void *)i);
	}

	if (!new_thread)
		thread((void *)(size_t)0);

	i = (new_thread) ? 0 : 1;
	for (; i < nthreads; i++)
		pthread_join(threads[i], NULL);

	thrarg->res = thrargs[0].res;
	thrarg->sum = thrargs[0].sum;
}

void benchmark_once_fork(struct thrarg *thrarg, unsigned iters)
{
	size_t i;
	unsigned nthreads = thrarg->threads;
	struct thrarg *thrargs;
	cpu_set_t c;
	pid_t pids[nthreads];

	if (!shared)
		shared = alloc_shared(sizeof(struct bench_shared));

	thrargs = shared->thrargs;
	shared->barrier = nthreads;

	thrarg->iters = iters;

	for (i=0; i < nthreads; i++) {
		thrargs[i] = *thrarg;
		thrargs[i].id = i;
	}

	for (i=0; i < nthreads; i++) {
		CPU_ZERO(&c);
		CPU_SET(i, &c);
		pids[i] = fork();
		if (!pids[i]) {
			pthread_setaffinity_np(pthread_self(), sizeof(c), &c);
			thread((void *)i);
			exit(0);
		}
	}

	for (i=0; i < nthreads; i++)
		waitpid(pids[i], NULL, 0);

	thrarg->res = thrargs[0].res;
	thrarg->sum = thrargs[0].sum;
}


void (*benchmark_once)(struct thrarg *, unsigned) = benchmark_once_fork;

static inline double sqr(double x)
{
	return x*x;
}

static void benchmark_auto(struct thrarg *thrarg)
{
	const double min_time_ns = 10000; //10us
	const unsigned max_iters = 10000000;
	const unsigned initial_samples = 10;
	const double t_initial = 2.228;
	const double error = 0.05;
	size_t i;
	unsigned iters;

	for (iters = 1; iters < max_iters; iters *= 2) {
		benchmark_once(thrarg, iters);
		if (thrarg->sum > min_time_ns)
			break;
	}

	if (iters > max_iters) {
		thrarg->iters = 0;
		return;
	}

	double isamples[initial_samples];
	fprintf(stderr, "isamples=%u iters = %u ",initial_samples, iters);

	double sum = 0;
	for (i = 0; i< initial_samples; i++) {
		benchmark_once(thrarg, iters);
		isamples[i] = thrarg->res;
		sum += isamples[i];
	}
	double avg = sum/initial_samples;
	double var = 0.0;
	for (i = 0; i< initial_samples; i++)
		var += sqr(avg - isamples[i]);
	double std_dev = sqrt(var/(initial_samples-1));
	fprintf(stderr, "var=%f avg=%f std_dev = %f ",var, avg, std_dev);

	double n_sqrt = (2*t_initial*std_dev)/(error*avg);
	unsigned n_final = (unsigned)ceil(sqr(n_sqrt))+4;
	fprintf(stderr, "n_final=%u ", n_final);

	double *samples = (double *)calloc(n_final, sizeof(double));

	int print_samples = thrarg->print_samples;
	sum = 0.0;
	for (i=0; i< n_final; i++) {
		benchmark_once(thrarg, iters);
		samples[i] = thrarg->res;
		sum += samples[i];
	}
	avg = sum/n_final;

	var = 0.0;
	for (i = 0; i< n_final; i++)
		var += sqr(avg - samples[i]);
	std_dev = sqrt(var/(n_final-1));

	thrarg->res = avg;
	thrarg->samples = n_final;
	thrarg->iters = iters;
	thrarg->sum = sum;
	thrarg->sdev = std_dev;

	fprintf(stderr, "sdev = %f\n",std_dev);
	if (print_samples)
		for (i = 0; i< n_final; i++)
			fprintf(stderr, "%f\n", samples[i]);
	free(samples);
}

void do_benchmark(struct thrarg *thrarg)
{
	if (thrarg->iters)
		benchmark_once(thrarg, thrarg->iters);
	else
		benchmark_auto(thrarg);
}

