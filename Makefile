CFLAGS=-Wall -pipe -O2 
OUTPUT=netnuke

all:
	cc -std=c99 -o ${OUTPUT} -lutil ${CFLAGS} netnuke.c 

