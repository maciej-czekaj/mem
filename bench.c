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



static pthread_barrier_t barrier;

static void *thread(void *arg)
{
	uint64_t t1, t2;
	struct thrarg *thrarg = (struct thrarg *)arg;

	pthread_barrier_wait(&barrier);

	t1 = getclock();

	thrarg->benchmark(thrarg);

	t2=getclock();

	thrarg->res = (t2 - t1)/(double)(thrarg->iters);
	thrarg->sum = (t2 - t1);
	return NULL;
}

static void benchmark_once(struct thrarg *thrarg, unsigned iters)
{
	const int new_thread = 1;
	size_t i;
	unsigned nthreads = thrarg->threads;
	pthread_t threads[nthreads];
	struct thrarg thrargs[nthreads];
	pthread_attr_t attr;
	cpu_set_t c;

	thrarg->iters = iters;

	pthread_barrier_init(&barrier, NULL, nthreads);
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
		pthread_create(&threads[i], &attr, thread, &thrargs[i]);
	}

	if (!new_thread)
		thread(&thrargs[0]);

	i = (new_thread) ? 0 : 1;
	for (; i < nthreads; i++)
		pthread_join(threads[i], NULL);

	thrarg->res = thrargs[0].res;
	thrarg->sum = thrargs[0].sum;
}

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
			printf("%f\n", samples[i]);
	free(samples);
}

void do_benchmark(struct thrarg *thrarg)
{
	if (thrarg->iters)
		benchmark_once(thrarg, thrarg->iters);
	else
		benchmark_auto(thrarg);
}

