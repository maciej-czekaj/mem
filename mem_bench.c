#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

#include "bench.h"

#define max(a,b) ((a) > (b) ? (a) : (b))

#define N (1*1000)

struct list {
	struct list *next;
	/* Romiar wypełnienia jest konfigurowany*/
	long pad[0];
} *list;


struct bench {
	int write;
	size_t size;
} bench;

void permutation(unsigned *l, size_t n)
{
	int i,k;
	unsigned tmp;

	for (k = n-1; k > 1; k--) {
		i = rand() % k;
		tmp = l[i];
		l[i] = l[k];
		l[k] = tmp;
	}
}

static struct list * meminit(size_t size, size_t line, int shuffle)
{
	unsigned i;
	unsigned n = size/line;
	struct list *p;
	char *l;
	unsigned *words;

	l = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0); 

	if (l == NULL) {
		perror("mmap");
		exit(1);
	}

	if (madvise(l, size, MADV_NOHUGEPAGE)) {
		perror("madvice");
		exit(1);
	}

	words = malloc((n+1)*sizeof(*words));

	for (i=1; i<n; i++)
		words[i] = n-i-1;
	words[0] = n-1;

	if (shuffle)
		permutation(words, n-1);

	p = (struct list *)l;
	for (i = 0; i < n; i++) {
		p->next = (struct list *)&l[words[i]*line];
		p = p->next;
	}

	free(words);

	return (struct list *)l;
}

void benchmark(struct thrarg *arg)
{
	struct list *l = list;
	unsigned iters = arg->iters;
	int w = bench.write;
	size_t id = arg->id;

	while (iters--) {
		if (w)
			l->pad[id] += 1;
		l = l->next;
	}

	USE(l);
}

void init(struct thrarg *arg)
{
	(void)arg;
	size_t x = 0;
	size_t i;
	size_t *mem = (size_t *)list;

	for (i = 0; i < bench.size/sizeof(size_t); i++)
		x += mem[i];
	USE(x);
}

void memtest_init(size_t size, size_t line, int shuffle)
{
	list = meminit(size, line, shuffle);
}


int main(int argc, char **argv)
{
	unsigned long size, line;
	char unit = 'b';
	int shuffle = 1;
	unsigned nthreads = 1;

	if (argc < 3)
		return 1;

	if (sscanf(argv[1], "%lu%c", &size, &unit) < 1)
		return 1;

	switch (unit) {
		case 'k':
			size <<= 10;
			break;
		case 'm':
			size <<= 20;
			break;
	}

	if (sscanf(argv[2], "%lu", &line) < 1)
		return 1;

	if (size < line || line < 8)
		return 1;

	if (argc > 3 && strcmp(argv[3],"-l") == 0) {
		shuffle = 0;
	}
	else if (argc > 3 && sscanf(argv[3],"%u", &nthreads) == 1) {
		shuffle = 0;
	}

	if (argc > 4 && strcmp(argv[4],"-w") == 0) {
		bench.write = 1;
	}

	bench.size = size;
	memtest_init(size, line, shuffle);

	//unsigned n = size/line;
	//unsigned iterations = max(N, 2*n);

	struct thrarg thrarg = {
		.threads = nthreads,
		.iters = 0,
		.id = 0,
		.benchmark = benchmark,
		.init = init,
		.print_samples = 1,
	};

//	size_t i;
//	for (i=0;i<20;i++) {
		do_benchmark(&thrarg);
		printf("%lu %.4f\n", size, thrarg.res);
//	}

	return 0;
}
