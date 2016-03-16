#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bench.h"

struct list {
	struct list *next;
	long val;
};
#define MAX_THREADS 2

static char *mem;
static struct list *lists[MAX_THREADS];

static void update_shared(struct list *l, size_t n)
{
	size_t i;

	for (i=0; i < n; i++) {
		l->val += 1;
		l = l->next;
	}
}

void benchmark_v(struct thrarg *thr)
{
	struct list *l = lists[thr->params.id];
	size_t n  = thr->params.iters;

	update_shared(l, n);
}


void benchmark_a(struct thrarg *thr)
{
	size_t i;
	long r = 0;
	struct list *l = lists[thr->params.id];
	size_t n  = thr->params.iters;

	for (i=0; i < n; i++) {
		l->val += 1;
		l = l->next;
	}
	USE(r);
}

void benchmark_s(struct thrarg *thr)
{
	size_t i;
	struct list *l = lists[thr->params.id];
	size_t n  = thr->params.iters;

	for (i=0; i < n; i++) {
		__atomic_fetch_add(&l->val, 10, __ATOMIC_RELAXED);
		l = l->next;
	}
}

void benchmark_w(struct thrarg *thr)
{
	size_t i;
	long r = 0;
	unsigned id = thr->params.id;
	struct list *l = lists[id];
	size_t n  = thr->params.iters;

	for (i=0; i < n; i++) {
		if (id > 0) {
			l->val += 1;
		} else {
			r = l->val;
		}
		l = l->next;
	}
	USE(r);
}

void benchmark_r(struct thrarg *thr)
{
	size_t i;
	long r = 0;
	unsigned id = thr->params.id;
	struct list *l = lists[id];
	size_t n  = thr->params.iters;

	for (i=0; i < n; i++) {
		r = l->val;
		l = l->next;
	}
	USE(r);
}

void init(struct thrarg *arg)
{
	(void)*arg;
}

int main(int argc, char **argv)
{
	unsigned i;
	unsigned nthreads = MAX_THREADS;
	unsigned pad;
	benchmark_t benchmark;

	if (argc < 3)
		return 1;

	if ( sscanf(argv[1], "%u", &pad) < 1)
		return 1;

	if (pad < sizeof(struct list) && pad != 0)
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
	case 'w':
		benchmark = benchmark_w;
		break;
	case 'a':
		benchmark = benchmark_a;
		break;
	default:
		return 1;
	}

	if (posix_memalign((void **)&mem, 128, nthreads*pad)) {
		perror("posix_memalign");
		return 1;
	}

	memset(mem, 0, nthreads*pad);
	for (i=0; i < nthreads; i++) {
		struct list *l = (struct list *)&mem[i*pad];
		l->next = l;
		lists[i] = l;
	}

	const char *bp = getenv("BENCH_PRINT");
	bool print_samples = (bp && strcmp(bp,"y") == 0) ? true : false;

	struct thrarg thrarg = { .params = {
		.threads = nthreads,
		.benchmark = benchmark,
		.init = init,
		.print_samples = print_samples,
		.max_samples = 100,
		.min_time =  10*1000*1000,
		.max_error = 10,
	}};

	int err = benchmark_auto(&thrarg);
	if (err < 0) {
		fprintf(stderr, "Bench error %s\n", strerror(err));
		return 1;
	}
	printf("%u %.2f %.2f %.2f\n", pad, thrarg.result.avg, thrarg.result.err, thrarg.result.u);

	return 0;
}
