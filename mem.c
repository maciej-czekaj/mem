#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

#define max(a,b) ((a) > (b) ? (a) : (b))

struct Good {
  int flags;
  int counter;
  long array[7];
};

struct Bad {
  int flags;
  long a[7];
  int counter;
};

struct Ugly {
  long *not_used;
  long array[7];
  int flags;
  int counter;
} ;

struct Pretty {
  int flags;
  int counter;
  long array[7];
  long not_used;
};

struct Good a;
struct Bad b;
struct Pretty p;
struct Ugly u;
struct list {
	struct list *next;
	/* Romiar wypełnienia jest konfigurowany*/
	long pad[0];
} *list;

#define CLOCK_TYPE CLOCK_MONOTONIC_RAW
//#define N (100*1000*1000)
#define N (1*1000)
#define MAXTHREADS 16

static pthread_barrier_t barrier;
static unsigned iterations;
static float times[MAXTHREADS];
static int write;

uint64_t getclock_2()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_TYPE, &ts) != 0) {
		perror("clock_gettime");
		exit(1);
	}
	return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

uint64_t getclock_1()
{
	union {
		uint64_t tsc_64;
		struct {
			uint32_t lo_32;
			uint32_t hi_32;
		};
	} tsc;

	asm volatile("rdtsc" :
		     "=a" (tsc.lo_32),
		     "=d" (tsc.hi_32));
	return tsc.tsc_64;
}

#define getclock getclock_2

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

void benchmark(long id)
{
	struct list *l = list;
	unsigned iters = iterations;
	int w = write;

	while (iters--) {
		if (w)
			l->pad[id] += 1;
		l = l->next;
	}

	asm volatile ("" :: "r" (l));
}

void *thread(void *arg)
{
	uint64_t t1, t2;
	long id = (long )arg;

	pthread_barrier_wait(&barrier);
	t1 = getclock();

	benchmark(id);

	t2 = getclock();

	times[id] = ((t2-t1)*1.0)/iterations;

	return NULL;
}

void memtest_init(size_t size, size_t line, int shuffle)
{
	unsigned n = size/line;
	iterations = max(N, 2*n);

	list = meminit(size, line, shuffle);
}

float memtest(unsigned nthreads)
{
	unsigned i;
	pthread_t threads[nthreads];
	pthread_attr_t attr;
	cpu_set_t c;

	pthread_barrier_init(&barrier, NULL, nthreads);
	pthread_attr_init(&attr);

	for (i = 1; i < nthreads; i++) {
		CPU_ZERO(&c);
		CPU_SET(i, &c);
		pthread_attr_setaffinity_np(&attr, sizeof(c), &c);
		pthread_create(&threads[i], &attr, thread, (void *)(long)i);
	}

	CPU_ZERO(&c);
	CPU_SET(0, &c);
	pthread_setaffinity_np(pthread_self(), sizeof(c), &c);

	thread((long)0);

	//printf("%.2f\n", times[0]);
	for (i = 1; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
		//printf("%.2f\n", times[i]);
	}

	return times[0];
}


int main(int argc, char **argv) 
{
	float time;
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
		write = 1;
	}

	memtest_init(size, line, shuffle);

	unsigned i;
	for (i=0;i<20;i++) {
		time = memtest(nthreads);
		printf("%lu %.4f\n", size, time);
	}

	return 0;
}
