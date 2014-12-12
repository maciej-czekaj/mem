#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

#define max(a,b) ((a) > (b) ? (a) : (b))

struct list {
	struct list *next;
	long pad[0];
};

#define CLOCK_TYPE CLOCK_MONOTONIC_RAW

uint64_t getclock()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_TYPE, &ts) != 0) {
		perror("clock_gettime");
		exit(1);
	}
	return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

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

void benchmark(struct list *l, unsigned iters)
{
	while (iters--) {
			l = l->next;
	}

	asm volatile ("" :: "r" (l));
}

float memtest(size_t size, size_t line, int shuffle)
{
	unsigned iters;
	uint64_t t1,t2;
	struct list *l;
	unsigned n = size/sizeof(struct list);

	iters = max(10*1000*1000, n);

	l = meminit(size, line, shuffle);

	t1 = getclock();

	benchmark(l, iters);

	t2 = getclock();

	return ((t2-t1)*1.0)/(iters);
}


int main(int argc, char **argv) 
{
	float time;
	size_t size, line;
	char unit = 'b';
	int shuffle = 1;

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

	if (argc > 3 && strcmp(argv[3],"-l") == 0)
		shuffle = 0;

	time = memtest(size, line, shuffle);

	printf("%lu %.2f\n", size, time);

	return 0;
}
