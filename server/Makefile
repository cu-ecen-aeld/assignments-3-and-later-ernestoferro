CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -lpthread -lrt

all: aesdsocket
default: aesdsocket

.PHONY: all default clean

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f aesdsocket *.o
