CFLAGS=-Wall -pipe -O2 -funroll-loops
OUTPUT=netnuke

all:
	gcc -o ${OUTPUT} -lutil ${CFLAGS} netnuke.c
