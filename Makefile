#Makefile for dcp

DEBDEPS:=gcc libxxhash-dev libssl-dev

CC:=gcc -Wall
BINS:=dcp dcp-cbr
HDRS:=$(shell ls *.h)
OBJS:=dcp.o file.o hash.o compare.o
LIBS:=-lxxhash -lm -lpthread -lcrypto
STRIP:=strip -s
TESTS:=test-a.tst test-b.tst

ifeq ($(D),1)
CC:=$(CC) -DDEBUG
endif

ifeq ($(S),1)
CC:=$(CC) -static
endif

.PHONY: debdeps clean distclean strip all testfiles

default: $(BINS)

all: strip testfiles

debdeps:
	sudo apt install $(DEBDEPS)

%.o: %.c $(HDRS)
	$(CC) -c $< -o $@

test-a.tst:
	dd if=/dev/random of=$@ bs=1M count=1024 status=progress

test-b.tst: test-a.tst dcp-cbr
	cp $< $@
	./dcp-cbr $@ 10

testfiles: $(TESTS)

dcp-cbr: dcp-cbr.c file.o
	$(CC) $^ -o $@

dcp: $(OBJS)
	$(CC) $^ $(LIBS) -o $@

strip: $(BINS)
	$(STRIP) $^

clean:
	rm -f $(BINS) $(OBJS)

distclean: clean
	rm -f $(TESTS)
