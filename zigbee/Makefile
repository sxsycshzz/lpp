ARCH ?=
CROSS_COMPILE ?=

CC := $(CROSS_COMPILE)gcc

all: test_zigbee

PRE_ROOT ?= ../deployment/

test_zigbee: test_zigbee.c sbuf.c
	$(CC) $(CFLAGS) $^ -o $@ 

pre_install:test_zigbee
	cp $^ $(PRE_ROOT)usr/local/bin/

clean:
	-rm -f *.o test_zigbee 

