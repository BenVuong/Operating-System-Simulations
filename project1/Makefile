CC=gcc
CFLAGS=-std=c99 -lpthread -lrt

all: worker oss

worker: worker.c
        $(CC) $(CFLAGS) -o worker worker.c

oss: oss.c
        $(CC) $(CFLAGS) -o oss oss.c

clean:
        rm -f worker oss
