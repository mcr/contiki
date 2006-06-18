# $Id$

all: core
upload: core core.u

core:
	$(MAKE) core.firmware && \
	$(MAKE) core.firmware && \
	$(MAKE) core.firmware

CONTIKI=../..
TARGET=esb


ifdef ENABLE_BACNET
APPS += bacnet
APPDIRS += apps/bacnet
endif

include $(CONTIKI)/Makefile.include
