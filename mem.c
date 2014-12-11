#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

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


#if defined(DEBUG) || defined(TEST)
void dump_list(struct list *l, size_t n)
{
	unsigned i, offset;

	for (i = 0; i < n; i++) {
		offset = (unsigned)(l[i].next - l);
		printf("%u:%u ", i, offset);
	}
	puts("");
}

void trace_list(struct list *l, size_t n)
{
	unsigned i;
	int offset;
	struct list *p = l;

	printf("n = %lu\n", n);
	for (i = 0; i < n; i++) {
		offset = (int)(p -l);
		printf("%i ", offset);
		p = p->next;
	}
	puts("");
}
#endif

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

static struct list * meminit(size_t size, int shuffle) 
{
	unsigned i;
	unsigned n = size/sizeof(struct list);
	struct list *l, *p;
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
	
#ifdef DEBUG
	for (i=0;i<n;i++)
		printf("%u ", words[i]);
	puts("");
#endif
	p = l;
	for (i = 0; i < n; i++) {
		p->next = &l[words[i]];
#ifdef DEBUG
		printf("%p %p %u\n", p, p->next, words[i]);
#endif
		p = p->next;
	}

	free(words);

#ifdef TEST
	trace_list(l, n);
#endif
	return l;
}

#define CLOCK_TYPE CLOCK_MONOTONIC_RAW
//CLOCK_THREAD_CPUTIME_ID
//CLOCK_MONOTONIC
//CLOCK_REALTIME 
//CLOCK_THREAD_CPUTIME_ID


float memtest(size_t size, int shuffle, unsigned *refs)
{
	unsigned i, iters;
	struct timespec ts1, ts2;
	struct list *p,*l;
	unsigned n = size/sizeof(struct list);

	iters = 10*1000*1000/n;
	if (iters == 0)
		iters = 5;
	*refs = iters*n;

	l = meminit(size, shuffle);

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

	return (diff(ts1, ts2)*1.0)/(n*iters);
}


int main(int argc, char **argv, char **arge) 
{
	float time;
	size_t size;
  	unsigned refs;
	char unit = 'b';
	struct timespec res;
	int shuffle = 1;

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

	if (argc > 2 && strcmp(argv[2],"-l") == 0)
		shuffle = 0;

	if (clock_getres(CLOCK_TYPE, &res)) {
		perror("clock_gettime");
		return(1);
	}

	time = memtest(size, shuffle, &refs);

	//printf("stride = %lu res = %lu size = %lu refs = %u time = %.2f\n", sizeof(struct list), res.tv_nsec, size, refs, time);
	printf("%lu %.2f\n", size, time);

	return 0;
}
