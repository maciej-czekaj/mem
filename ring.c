#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sched.h>

#include "bench.h"

#define assert(x) if(!(x)) __builtin_trap()
#define assert_eq(x,y) \
	do { \
		if((x) != (y)) { \
			printf("ERROR %s %ld != %ld\n", #x, (long)(x), (long)(y)); \
			__builtin_trap(); \
		} \
	} while (0)

#define RING_LEN 32
//#define WITH_LOCK
//#define EXCLUSIVE true

#define CACHE_LINE 64

struct message
{
  size_t count;
};

struct ring
{
  unsigned len;
  unsigned received __attribute__ ((aligned(CACHE_LINE)));
  unsigned sent __attribute__ ((aligned(CACHE_LINE)));
  struct message messages[0];
} __attribute__ ((aligned(CACHE_LINE)));

bool ring_send(struct ring *r, size_t n,
	       struct message msg[n])
{
  unsigned received, sent, len, free_entries, mask;

  // Do sent pisze bieżący wątek, więc nie trzeba bariery.
  sent = r->sent;
  len = r->len;			// len jest stałe
  mask = len - 1;
  // Do received pisze wątek konsumenta więc
  // potrzebna jest bariera.
  received =
    __atomic_load_n(&r->received, __ATOMIC_ACQUIRE);
  /* Oblicz wolne miejsce */
  free_entries = len + received - sent;
  if (free_entries < n)
    return false;		// za mało miejsca

  // Kopiuj wiadomości
  for (size_t i = 0; i < n; i++)
    r->messages[(sent + i) & mask] = msg[i];

  // Publikuj nową wartosć sent do innych rdzeni (bariera release)
  // Gwarantuje, że wiadomości zostały uprzednio skoiowane
  __atomic_store_n(&r->sent, sent + n, __ATOMIC_RELEASE);

  return true;
}


bool ring_receive(struct ring * r, size_t n,
		  struct message msg[n])
{
  unsigned received, sent, len, valid_entries, mask;

  // do received pisze ten wątek, nie trzeba bariery
  received = r->received;
  len = r->len;			// tylko do odczytu
  mask = len - 1;
  // sent jest publikowaqny przez wątek producenta
  // trzeba użyć bariery acquire
  sent = __atomic_load_n(&r->sent, __ATOMIC_ACQUIRE);
  // Oblicz, ile wiadomości czeka
  valid_entries = sent - received;
  if (valid_entries < n)	// za mało
    return false;

  // Kopiuj wiadomości
  for (size_t i = 0; i < n; i++)
    msg[i] = r->messages[(received + i) & mask];

  // Publikuj nową wartośc received do innych rdzeni (bariera release)
  // Gwarantuje, że wiadomości zostały uprzednio skoiowane
  __atomic_store_n(&r->received, received + n,
		   __ATOMIC_RELEASE);

  return true;
}

void send(size_t n, struct ring *r)
{
  struct message msg = {.count = 0 };

  for (size_t i = 0; i < n; i++)
    {
      msg.count = i;
      while (!ring_send(r, 1, &msg))
	;
    }
}

void recv(size_t n, struct ring *r)
{
  struct message msg;

  for (size_t i = 0; i < n; i++)
    {
      while (!ring_receive(r, 1, &msg))
	;
      assert(msg.count == i);
    }
}

struct ring *R;

void benchmark_ping(struct thrarg *arg)
{
  if (arg->params.id)
    send(arg->params.iters, R);
  else
    recv(arg->params.iters, R);
}

void ring_reset(struct ring *r)
{
  r->sent = 0;
  r->received = 0;
}

struct ring *ring_new(size_t ring_len)
{
  struct ring *r = NULL;
  if (posix_memalign
      ((void **) &r, 64,
       sizeof(struct ring) +
       ring_len * sizeof(struct message)))
    {
      perror("posix_memalign");
      exit(1);
    }
  ring_reset(r);
  r->len = ring_len;
  return r;
}

void init(struct thrarg *arg)
{
  (void) arg;
  ring_reset(R);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

void usage()
{
  fprintf(stderr, "Usage:\n\tring [ <size> ]\n");
}

int main(int argc, char **argv)
{
  unsigned nthreads = 2;

  unsigned len = RING_LEN;

  if (argc == 2 && sscanf(argv[1], "%u", &len) != 1)
    {
      usage();
      return 1;
    }

  if (argc > 2)
    {
      usage();
      return 1;
    }

  R = ring_new(len);

  struct thrarg thrarg = {.params = {
				     .threads = nthreads,
				     .benchmark =
				     benchmark_ping,
				     .init = init,
				     .min_time = 100 * 1000,
				     .max_samples = 100,
				     .max_error = 10,
				     .iters = 10,
				     }
  };

  int err = benchmark_auto(&thrarg);
//      int err = benchmark_once(&thrarg);
  if (err < 0)
    {
      fprintf(stderr, "Bench error %s\n", strerror(err));
      return 1;
    }
  printf("%.2f %.2f %.2f\n", thrarg.result.avg,
	 thrarg.result.err, thrarg.result.u);

  return 0;
}
