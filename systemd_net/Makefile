ARCH ?=
CROSS_COMPILE ?=

CC := $(CROSS_COMPILE)gcc

APP1 = systemd_net_switch
APP2 = init_networking
all: $(APP1) $(APP2) 

PRE_ROOT ?= ../deployment/
PRE_INSTALL_DIR ?=$(PRE_ROOT)usr/local/bin/

$(APP1): systemd_net_switch.c
	$(CC) $(CFLAGS) $^ -o $@ 

pre_install: $(APP1) $(APP2)
	cp $^ $(PRE_INSTALL_DIR) 
	echo "Copy $^ to $(PRE_INSTALL_DIR)"

clean:
	-rm -f *.o $(APP1) 

