#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>

#define NPAD 15

struct list {
	struct list *next;
	long pad[NPAD];
};

static uint64_t ts2scalar(struct timespec ts)
{
	return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

uint64_t diff(struct timespec start, struct timespec end)
{
	return ts2scalar(end) - ts2scalar(start);
}

void permutation(struct list *l, size_t n)
{
	int i,k;
	struct list tmp;

	for (k = n-1; k > 1; --k) {
		i = rand() % k;
		tmp = l[i];
		l[i] = l[k];
		l[k] = tmp;
	}
}

void dump_list(struct list *l, size_t n)
{
	unsigned i, offset;

	for (i = 0; i < n; i++) {
		offset = (unsigned)(l[i].next - l);
		printf("%u:%u ", i, offset);
	}
	puts("");
}

static struct list * meminit(size_t size) 
{
	unsigned i;
	unsigned n = size/sizeof(struct list);

	struct list *l = malloc(size);

	if (l == NULL) {
		perror("malloc");
		exit(1);
	}
	for (i = 0; i < n-1; i++)
		l[i+1].next = &l[i];
	l[0].next = &l[n-1];
	return l;
}

#define CLOCK_TYPE CLOCK_MONOTONIC_RAW
//CLOCK_THREAD_CPUTIME_ID
//CLOCK_MONOTONIC
//CLOCK_REALTIME 
//CLOCK_THREAD_CPUTIME_ID

float memtest(size_t size, unsigned iters)
{
	unsigned i;
	struct timespec ts1, ts2;
	struct list *p,*l;
	unsigned n = size/sizeof(struct list);
	
	iters = 1000;
	if ((iters * n) > 10000000)
		iters = 10000000/n;
	

	l = meminit(size);
	permutation(l, n);

	if (clock_gettime(CLOCK_TYPE, &ts1) != 0) {
		perror("clock_gettime");
		exit(1);
	}
	
	p = &l[0];
	for (i = 0; i < iters; i++) {
		unsigned j;
		for (j = 0; j < n; j++) {
			p = p->next;
			asm volatile ("" :: "r" (p));
		}
	}
	
	if (clock_gettime(CLOCK_TYPE, &ts2) != 0) {
		perror("clock_gettime");
		exit(1);
	}

	free(l);

	return (diff(ts1, ts2)*1.0)/(n*iters);
}


int main(int argc, char **argv, char **arge) 
{
	float time;
	size_t size;
  	unsigned iters;
	char unit = 'b';
	struct timespec res;

	(void)arge;
	if (argc < 2)
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

	if (argc > 2 && sscanf(argv[2], "%u", &iters) != 1)
		return 1;

	if (clock_getres(CLOCK_TYPE, &res)) {
		perror("clock_gettime");
		return(1);
	}

	time = memtest(size, iters);

	printf("stride=%lu res=%lu size=%lu %.2f\n", sizeof(struct list), res.tv_nsec, size, time);

	return 0;
}
