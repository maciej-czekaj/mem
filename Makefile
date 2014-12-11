CC=gcc
CFLAGS=-g -Wall -Wextra -Werror
ifeq ($(DEBUG),1)
 CFLAGS+=-O0
else
 CFLAGS+=-O3
endif
#LDFLAGS=-lrt

all: mem

mem: mem.o

clean:
	rm -f mem mem.o
