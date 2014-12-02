#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>

#define NPAD 31

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


uint64_t memtest(size_t size, unsigned iters)
{
	unsigned i;
	struct timespec ts1, ts2;
	struct list *p,*l;
	
	l = meminit(size);

	if (clock_gettime(CLOCK_REALTIME, &ts1) != 0) {
		perror("clock_gettime");
		exit(1);
	}
	
	p = &l[0];
	for (i = 0; i < iters; i++) {
		p = p->next;
		asm volatile ("" :: "r" (p));
	}
	
	if (clock_gettime(CLOCK_REALTIME, &ts2) != 0) {
		perror("clock_gettime");
		exit(1);
	}

	free(l);

	return diff(ts1, ts2);
}


int main(int argc, char **argv, char **arge) 
{
	uint64_t time;
	size_t size;
  	unsigned iters;

	(void)arge;
	if (argc != 3)
		return 1;
	
	if (sscanf(argv[1], "%lu", &size) != 1)
		return 1;
	if (sscanf(argv[2], "%u", &iters) != 1)
		return 1;

	time = memtest(size, iters);

	printf("size=%lu iters=%u %.2f\n", size, iters, (time*1.0)/iters);

	return 0;
}
