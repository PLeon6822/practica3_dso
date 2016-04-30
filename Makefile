#
# Makefile del sistema de ficheros
#

INCLUDEDIR=./include
CC=gcc
CFLAGS=-g -Wall -I$(INCLUDEDIR)
AR=ar
MAKE=make

OBJS_DEV= ufs.o filesystem.o
LIB=libfs.a


all: create_disk test

test: $(LIB)
	$(CC) $(CFLAGS) -o test test.c libfs.a

filesystem.o: $(INCLUDEDIR)/filesystem.h
ufs.o: $(INCLUDEDIR)/ufs.h

$(LIB): $(OBJS_DEV)
	$(AR) rcv $@ $^

create_disk: create_disk.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(LIB) $(OBJS_DEV) test create_disk create_disk.o
