CC=gcc
CFLAGS=-Wall -O3

smr:	smr.c khash.h
	$(CC) $(CFLAGS) -o smr smr.c

clean:	
	rm -f smr
