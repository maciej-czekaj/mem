CFLAGS=-g -Wall -Wextra -Werror -std=gnu11
ifeq ($(DEBUG),1)
 CFLAGS+=-O0 -DDEBUG
else
 CFLAGS+=-O3
endif

ifeq ($(TEST),1)
 CFLAGS+=-DTEST
endif

LDLIBS=-lrt -lpthread -lm

all: mem threads quiz ping mem_bench thr_bench thr_ping

mem: mem.o

thr_ping: thr_ping.o bench.o

thr_bench: thr_bench.o bench.o

threads: threads.o

quiz: quiz.o

ping: ping.o

bench.o: bench.c

mem_bench: bench.o mem_bench.o

clean:
	rm -f mem mem.o threads threads.o ping ping.o
