CFLAGS=-std=c99 -Wall -pipe -O2 
LFLAGS=-lutil -ltermcap
OUTPUT=netnuke

all:
	cc -o ${OUTPUT} ${LFLAGS} ${CFLAGS} netnuke.c 

