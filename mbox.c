#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bench.h"

#define N (1*1000*1000)
#define MAXTHREADS 16


struct message
{
	size_t count;
	char message[32];
}

struct mbox {
	size_t received;
	struct message message;
	void *cache[0] __attribute__((aligned(64)));
	size_t sent;
} *mbx;



bool mbox_receive(struct mbox *mbx, struct message *msg)
{
	size_t received, sent;

	received  = __atomic_load_n(&mbx->received, __ATOMIC_RELAXED);
	sent = _atomic_load_n(&mbx->sent, __ATOMIC_ACQUIRE);

	if (sent > received) {
		*msg =  mbx->message;
		__atomic_store_n(&mbx->received, received + 1, __ATOMIC_RELEASE);
		return true;
	}
	return false;
}


bool mbox_send(struct mbox *mbx, struct message *msg)
{
	size_t received, sent;

	sent = _atomic_load_n(&mbx->sent, __ATOMIC_RELAXED);
	received  = __atomic_load_n(&mbx->received, __ATOMIC_ACQUIRE);

	if (sent == received) {
		mbx->message = *msg;
		__atomic_store_n(&mbx->sent, sent + 1, __ATOMIC_RELEASE);
		return true;
	}
	return false;
}

void send(size_t n)
{
	struct message msg = {.count = 0};

	msg.message.count++;
	mbox_send
}

void benchmark_ping(struct thrarg *arg)
{
	if (arg->params.id) {
		while (mbox_recv(mbx, "Hello"))
	else
		ping(arg);
}

void init(struct thrarg *arg)
{
	(void)arg;
	memset(pingpong, 0, sizeof(struct pingpong));
}

int main(int argc, char **argv)
{
	unsigned nthreads = 2;
	(void)argc; (void)argv;

	if (posix_memalign((void **)&pingpong, 128, sizeof(struct pingpong))) {
		perror("posix_memalign");
		return 1;
	}

	memset(pingpong, 0, sizeof(struct pingpong));

	struct thrarg thrarg = { .params = {
		.threads = nthreads,
		.benchmark = benchmark_ping,
		.init = init,
		.min_time =  1*1000*1000,
	}};

	int err = benchmark_auto(&thrarg);
	if (err) {
		fprintf(stderr, "Bench error %s\n", strerror(err));
		return 1;
	}
	printf("%.2f %.2f %.2f\n", thrarg.result.avg, thrarg.result.err, thrarg.result.u);

	return 0;
}
