
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define CLOCK_TYPE CLOCK_MONOTONIC_RAW

uint64_t getclock()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_TYPE, &ts) != 0) {
		perror("clock_gettime");
		exit(1);
	}
	return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

#define N ((8<<20)/sizeof(long)) //8MB
#define K ((1<<30) / N)

long a[N];// __attribute__((aligned(64)));

void quiz(unsigned step)
{
	unsigned k,i;
	for (k = 0; k < K ; k++)
		for (i = 0; i < N; i += step)
			a[i] *= 3;
}

int main(int argc, char **argv)
{
	unsigned step;
	memset(a,1,sizeof(a));
	if (argc == 2) {
		uint64_t time = getclock();
		step = atoi(argv[1]);
		quiz(step);
		time = getclock() - time;
		printf("%u %.1f\n", step, time/10e6);
		return 0;
	} 
	for (step=1; step < 16; step++) {
		uint64_t time = getclock();
		quiz(step);
		time = getclock() - time;
		printf("%u %.1f\n", step, time/10e6);
	}
	return 0;
}
