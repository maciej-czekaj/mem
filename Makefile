CFLAGS=-g -Wall -Wextra -Werror
ifeq ($(DEBUG),1)
 CFLAGS+=-O0 -DDEBUG
else
 CFLAGS+=-O3
endif

ifeq ($(TEST),1)
 CFLAGS+=-DTEST
endif

LDLIBS=-lrt -lpthread

all: mem threads quiz

mem: mem.o

threads: threads.o

quiz: quiz.o

clean:
	rm -f mem mem.o threads threads.o
