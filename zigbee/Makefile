ARCH ?=
CROSS_COMPILE ?=

CC := $(CROSS_COMPILE)gcc

APP = test_zigbee
all : $(APP)

PRE_ROOT ?= ../deployment/
PRE_INSTALL_DIR ?=$(PRE_ROOT)usr/local/bin/

$(APP): test_zigbee.c sbuf.c
	$(CC) $(CFLAGS) $^ -o $@ 

pre_install: $(APP)
	cp $^ $(PRE_INSTALL_DIR) 
	echo "Copy $(APP) to $(PRE_INSTALL_DIR)"

clean:
	-rm -f *.o $(APP) 

