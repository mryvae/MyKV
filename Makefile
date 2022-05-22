# Redis Makefile
# Copyright (C) 2009 Salvatore Sanfilippo <antirez at gmail dot com>
# This file is released under the BSD license, see the COPYING file
DEBUG?= -g
CFLAGS?= -std=c99 -pedantic -O2 -Wall -W -DSDS_ABORT_ON_OOM
CCOPT= $(CFLAGS)

OBJ = adlist.o ae.o anet.o dict.o mykv.o sds.o zmalloc.o  lzf_c.o lzf_d.o lru.o carray.o pmemlog.o pmem.o bitmap.o pdict.o libpmem.o
CLIOBJ = anet.o sds.o adlist.o mykv-cli.o zmalloc.o

PRGNAME = mykv-server
CLIPRGNAME = mykv-cli

all: mykv-server mykv-cli

# Deps (use make dep to generate this)
adlist.o: adlist.c adlist.h
ae.o: ae.c ae.h
anet.o: anet.c anet.h
dict.o: dict.c dict.h
mykv-cli.o: mykv-cli.c anet.h sds.h adlist.h
mykv.o: mykv.c ae.h sds.h anet.h dict.h adlist.h zmalloc.h lru.h carray.h pmemlog.h pdict.h bitmap.h pmem.h libpmem.h
sds.o: sds.c sds.h
zmalloc.o: zmalloc.c zmalloc.h
lru.o: lru.c lru.h
carray.o: carray.c carray.h
pmemlog.o: pmemlog.c pmemlog.h libpmem.h
pmem.o: pmem.c pmem.h libpmem.h
bitmap.o: bitmap.c bitmap.h
pdict.o: pdict.h pdict.c pmem.h sds.h
libpmem.o: libpmem.c libpmem.h

mykv-server: $(OBJ)
	$(CC) -o $(PRGNAME) $(CCOPT) $(DEBUG) $(OBJ)
	@echo ""
	@echo "Hint: To run the test-redis.tcl script is a good idea."
	@echo "Launch the redis server with ./mykv-server, then in another"
	@echo "terminal window enter this directory and run 'make test'."
	@echo ""

mykv-cli: $(CLIOBJ)
	$(CC) -o $(CLIPRGNAME) $(CCOPT) $(DEBUG) $(CLIOBJ)

.c.o:
	$(CC) -c $(CCOPT) $(DEBUG) $(COMPILE_TIME) $<

clean:
	rm -rf $(PRGNAME) $(WORKLOADNAME) $(CLIPRGNAME) $(REDISWORKLOADNAME) *.o

dep:
	$(CC) -MM *.c

test:
	tclsh test-redis.tcl
log:
	git log '--pretty=format:%ad %s' --date=short > Changelog

