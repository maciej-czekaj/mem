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
//	char message[32];
};

struct mbox {
	size_t sent;
	struct message message;
	void *cache[0] __attribute__((aligned(64)));
	size_t received;
} *mbx;

static bool mbox_reader_begin(struct mbox *mbx)
{
	size_t sent;
	sent = __atomic_load_n(&mbx->sent, __ATOMIC_ACQUIRE);

	return sent > mbx->received;
}

static void mbox_reader_end(struct mbox *mbx)
{
	__atomic_store_n(&mbx->received, mbx->received + 1, __ATOMIC_RELEASE);
}

bool mbox_receive(struct mbox *mbx, struct message *msg)
{
	if (mbox_reader_begin(mbx)) {
		*msg =  mbx->message;
		mbox_reader_end(mbx);
		return true;
	}
	return false;
}

static bool mbox_writer_begin(struct mbox *mbx)
{
	size_t received;

	received  = __atomic_load_n(&mbx->received, __ATOMIC_ACQUIRE);

	return mbx->sent == received;
}

static void mbox_writer_end(struct mbox *mbx)
{
	__atomic_store_n(&mbx->sent, mbx->sent + 1, __ATOMIC_RELEASE);
}

bool mbox_send(struct mbox *mbx, struct message *msg)
{
	if (mbox_writer_begin(mbx)) {
		mbx->message = *msg;
		mbox_writer_end(mbx);
		return true;
	}
	return false;
}

void send(size_t n)
{
	struct message msg = {.count = 0};

	for (size_t i = 0; i < n; i++) {
		msg.count = i;
		while (!mbox_send(mbx, &msg));
	}
}

void recv(size_t n)
{
	struct message msg;

	for (size_t i = 0; i < n; i++) {
		while (!mbox_receive(mbx, &msg));
		assert(msg.count == i);
	}
}

void benchmark_ping(struct thrarg *arg)
{
	if (arg->params.id)
		send(arg->params.iters);
	else
		recv(arg->params.iters);
}

void init(struct thrarg *arg)
{
	(void)arg;
	memset(mbx, 0, sizeof(struct mbox));
}

int main(int argc, char **argv)
{
	unsigned nthreads = 2;
	(void)argc; (void)argv;

	if (posix_memalign((void **)&mbx, 64, sizeof(struct mbox))) {
		perror("posix_memalign");
		return 1;
	}

	memset(mbx, 0, sizeof(struct mbox));

	struct thrarg thrarg = { .params = {
		.threads = nthreads,
		.benchmark = benchmark_ping,
		.init = init,
		.min_time =  1*1000*1000,
	}};

	int err = benchmark_auto(&thrarg);
	if (err < 0) {
		fprintf(stderr, "Bench error %s\n", strerror(err));
		return 1;
	}
	printf("%.2f %.2f %.2f\n", thrarg.result.avg, thrarg.result.err, thrarg.result.u);

	return 0;
}
