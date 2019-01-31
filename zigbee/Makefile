ARCH ?=
CROSS_COMPILE ?=

CC := $(CROSS_COMPILE)gcc

all: test_zigbee

PRE_ROOT ?= ../deployment/
PRE_INSTALL_DIR ?=$(PRE_ROOT)usr/local/bin/

test_zigbee: test_zigbee.c sbuf.c
	$(CC) $(CFLAGS) $^ -o $@ 

pre_install:test_zigbee
	cp $^ $(PRE_INSTALL_DIR) 
	echo "Copy $(APP) to $(PRE_INSTALL_DIR)"

clean:
	-rm -f *.o test_zigbee 

