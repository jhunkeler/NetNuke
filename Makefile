CFLAGS=-std=c99 -Wall -pipe -O2 -fomit-frame-pointer 
DEFINES= 
LFLAGS=-lutil -ltermcap
PACKAGE=netnuke

all:
	cc -o $(PACKAGE) $(DEFINES) $(CFLAGS) $(LFLAGS) netnuke.c  
	strip netnuke

clean:
	rm netnuke
