# $Id$

all: core
upload: core core.u

core:
	$(MAKE) core.firmware && \
	$(MAKE) core.firmware && \
	$(MAKE) core.firmware

CONTIKI=../..
TARGET=msb430

blinker.cm: blinker.co blinker2.co

ifdef ENABLE_BACNET
APPS += bacnet
APPDIRS += apps/bacnet
endif

include $(CONTIKI)/Makefile.include
