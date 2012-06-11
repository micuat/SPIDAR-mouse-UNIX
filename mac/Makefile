###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-07-03
#
# Modified by Naoto Hieda
# 2012-06-11
###########################################

all: spidar-mouse-server

CC=gcc
CXX=g++
COBJS=hid.o
CPPOBJS=../src/main.o
OBJS=$(COBJS) $(CPPOBJS)
CFLAGS+=-I../hidapi -Wall -g -c 
LIBS=-framework IOKit -framework CoreFoundation


spidar-mouse-server: $(OBJS)
	g++ -Wall -g $^ $(LIBS) -o spidar-mouse-server

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -f *.o spidar-mouse-server $(CPPOBJS)

.PHONY: clean
