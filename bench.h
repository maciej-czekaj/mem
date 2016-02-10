#ifndef _BENCH_H_
#define _BENCH_H_

struct thrarg;

typedef void (*benchmark_t)(struct thrarg *arg);

struct thrarg {
	unsigned threads;
	unsigned iters;
	unsigned samples;
	unsigned id;
	benchmark_t benchmark;
	int print_samples;
	double res;
	double sum;
	double sdev;
};

void do_benchmark(struct thrarg *thrarg);

#endif // _BENCH_H_
