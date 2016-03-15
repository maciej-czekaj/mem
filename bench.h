#ifndef _BENCH_H_
#define _BENCH_H_

#include <stdbool.h>
#include <errno.h>

struct thrarg;

typedef void (*benchmark_t)(struct thrarg *arg);

struct thrarg {
	struct thpar {
		unsigned threads;
		unsigned id;
		benchmark_t benchmark;
		benchmark_t init;
		bool print_samples;
		unsigned iters;
		unsigned max_samples;
		unsigned long min_time;
		unsigned char max_error;
	} params;
	struct thres {
		unsigned iters;
		unsigned samples;
		double avg;
		double sum;
		double sdev;
		double u;
		double err;
	} result;
};

int benchmark_auto(struct thrarg *thrarg);
int benchmark_once(struct thrarg *thrarg);

#define USE(x) asm volatile ("" :: "r" (x));

#endif // _BENCH_H_
