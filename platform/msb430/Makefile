# $Id$

all: test-abc
upload: test-abc test-abc.u

test-abc:
	$(MAKE) test-abc.firmware && \
	$(MAKE) test-abc.firmware && \
	$(MAKE) test-abc.firmware

CONTIKI=../..
TARGET=msb430

include $(CONTIKI)/Makefile.include
