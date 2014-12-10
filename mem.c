#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>

#define NPAD 15

struct list {
	struct list *next;
	long pad[NPAD];
};

uint64_t diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}

	return temp.tv_sec * 1000000000 + temp.tv_nsec;
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
	unsigned i;

	for (i = 0; i < n; i++) {
		printf("%u:%u", i, ((unsigned)(l[i].next - l))/sizeof(struct list));
	}
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

#define CLOCK_TYPE CLOCK_THREAD_CPUTIME_ID

float memtest(size_t size, unsigned iters)
{
	unsigned i;
	struct timespec ts1, ts2;
	struct list *p,*l;
	unsigned n = size/sizeof(struct list);
	
	iters = 100000000 / n;

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

	(void)arge;
	if (argc < 2)
		return 1;
	
	if (sscanf(argv[1], "%lu%c", &size, &unit) < 1)
		return 1;

	switch (unit) {
		case 'K':
			size *= 1024;
			break;
		case 'M':
			size *= 1024*1024;
			break;
	}

	if (argc > 2 && sscanf(argv[2], "%u", &iters) != 1)
		return 1;

	time = memtest(size, iters);

	printf("stride=%lu size=%lu %.2f\n", sizeof(struct list), size, time);

	return 0;
}
