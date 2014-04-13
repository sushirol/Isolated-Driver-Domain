# XDD Makefile for IRIX
CFLAGS =	-O2 -DIRIX -g
SHELL =		/bin/sh
PROJECT =	xdd
OBJECTS =	xdd.o access_pattern.o barrier.o global_time.o initialization.o parse.o pclk.o read_after_write.o results.o ticker.o time_stamp.o verify.o
HEADERS = 	xdd.h pclk.h ticker.h misc.h 
TSOBJECTS =	timeserver.o pclk.o ticker.o
GTOBJECTS = gettime.o global_time.o pclk.o ticker.o

all:	xdd timeserver gettime

xdd:	$(OBJECTS) 
	cc  -o xdd $(CFLAGS) $(OBJECTS) -lpthread -v
	mv -f xdd bin/xdd.irix

timeserver: $(TSOBJECTS)
	cc -o timeserver $(CFLAGS) $(TSOBJECTS) -v
	mv -f timeserver bin/timeserver.irix

gettime: $(GTOBJECTS)
	cc -o gettime $(CFLAGS) $(GTOBJECTS) -v
	mv -f gettime bin/gettime.irix

access_pattern.o:	access_pattern.c
	cc  $(CFLAGS) -c access_pattern.c

barrier.o:	barrier.c
	cc  $(CFLAGS) -c barrier.c

gettime.o: gettime.c
	cc $(CFLAGS) -c gettime.c

global_time.o:	global_time.c
	cc  $(CFLAGS) -c global_time.c

initialization.o:	initialization.c
	cc  $(CFLAGS) -c initialization.c

parse.o:	parse.c
	cc  $(CFLAGS) -c parse.c

pclk.o:	pclk.c 
	cc  $(CFLAGS) -c pclk.c

read_after_write.o:	read_after_write.c
	cc  $(CFLAGS) -c read_after_write.c

results.o:	results.c
	cc  $(CFLAGS) -c results.c

ticker.o:	ticker.c
	cc  $(CFLAGS) -c ticker.c

time_stamp.o:	time_stamp.c
	cc  $(CFLAGS) -c time_stamp.c

timeserver.o: timeserver.c
	cc $(CFLAGS) -c timeserver.c

verify.o: verify.c
	cc $(CFLAGS) -c verify.c

xdd.o:  xdd.c 
	cc  $(CFLAGS) -c xdd.c

dist:	clean
	tar cf ../dist.tar .
clean:
	-rm -f xdd timeserver gettime a.out $(OBJECTS) $(TSOBJECTS) $(GTOBJECTS)
