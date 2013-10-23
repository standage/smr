CC=gcc
DC=dmd
CFLAGS=-Wall -O3

smr:	smr.c khash.h
	$(CC) $(CFLAGS) -o smr smr.c

smr-d:	smr.d
	$(DC) -ofsmr-d smr.d

clean:	
	rm -f smr smr-d smr-d.o
