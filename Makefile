CC=gcc
CFLAGS=-O3 -g -Wall -Wextra -Werror
#LDFLAGS=-lrt

all: mem

mem: mem.o

clean:
	rm -f mem mem.o
