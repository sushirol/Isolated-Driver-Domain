# XDD Makefile for AIX with HPSS installed
SHELL =	/bin/sh
CFLAGS =	-O2 -DAIX -q32 -qcpluscmt -g -DHPSS -I/opt/hpss/include -I/opt/hpss/include/dmapi -I/opt/hpss/include/dmapi/dmg
PROJECT =	xdd
OBJECTS =	xdd.o access_pattern.o barrier.o global_time.o initialization.o parse.o pclk.o read_after_write.o results.o ticker.o time_stamp.o verify.o
HEADERS = 	xdd.h pclk.h ticker.h misc.h 
TSOBJECTS =	timeserver.o pclk.o ticker.o
GTOBJECTS = gettime.o global_time.o pclk.o ticker.o


all:	xdd timeserver gettime

xdd:	$(OBJECTS) 
	xlc  -o xdd $(CFLAGS) $(OBJECTS) -lpthread  -lxnet  -v -bnoquiet -lbsd 
	mv -f xdd bin/xdd.aixhpss

timeserver: $(TSOBJECTS)
	xlc -o timeserver $(CFLAGS) $(TSOBJECTS) -lxnet -v
	mv -f timeserver bin/timeserver.aixhpss

gettime: $(GTOBJECTS)
	xlc -o gettime $(CFLAGS) $(GTOBJECTS) -lxnet -v
	mv -f gettime bin/gettime.aixhpss

access_pattern.o:	access_pattern.c
	xlc  $(CFLAGS) -c access_pattern.c

barrier.o:	barrier.c
	xlc  $(CFLAGS) -c barrier.c

gettime.o: gettime.c
	xlc $(CFLAGS) -c gettime.c

global_time.o:	global_time.c
	xlc  $(CFLAGS) -c global_time.c

initialization.o:	initialization.c
	xlc  $(CFLAGS) -c initialization.c

parse.o:	parse.c
	xlc  $(CFLAGS) -c parse.c

pclk.o:	pclk.c 
	xlc  $(CFLAGS) -c pclk.c

read_after_write.o:	read_after_write.c
	xlc  $(CFLAGS) -c read_after_write.c

results.o:	results.c
	xlc  $(CFLAGS) -c results.c

ticker.o:	ticker.c
	xlc  $(CFLAGS) -c ticker.c

time_stamp.o:	time_stamp.c
	xlc  $(CFLAGS) -c time_stamp.c

timeserver.o: timeserver.c
	xlc $(CFLAGS) -c timeserver.c

verify.o: verify.c
	xlc $(CFLAGS) -c verify.c

xdd.o:  xdd.c 
	xlc  $(CFLAGS) -c xdd.c

dist:	clean
	tar cf ../dist.tar .
clean:
	-rm -f xdd timeserver gettime a.out $(OBJECTS) $(TSOBJECTS) $(GTOBJECTS)

