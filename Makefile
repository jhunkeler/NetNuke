CFLAGS=-std=c99 -Wall -pipe -O2 
DEFINES= 
LFLAGS=-lutil -ltermcap
PACKAGE=netnuke

all:
	gcc $(SVNDEF) -o $(PACKAGE) $(DEFINES) $(CFLAGS) $(LFLAGS) netnuke.c  
	strip netnuke

clean:
	rm netnuke
