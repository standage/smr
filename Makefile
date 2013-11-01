CC=gcc
CXX=g++
DC=dmd
CFLAGS=-Wall -O3

smr:		smr.c khash.h
		$(CC) $(CFLAGS) -o smr smr.c

smr-cpp:	smr.cpp
		$(CXX) -std=c++11 -Wall -o smr-cpp smr.cpp

smr-d:		smr.d
		$(DC) -ofsmr-d smr.d

all:		smr smr-cpp smr-d
		

clean:		
		rm -f smr smr-cpp smr-d smr-d.o
