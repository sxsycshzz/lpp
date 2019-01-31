### Environment constants

ARCH ?=
CROSS_COMPILE ?=
export

APP ?= libloragw/test_loragw_reg libloragw/test_loragw_spi libloragw/test_loragw_mqtt

PRE_ROOT ?= ../deployment/
PRE_INSTALL_DIR ?=$(PRE_ROOT)usr/local/bin/

### general build targets

all:
	$(MAKE) all -e -C libloragw


pre_install:all
	cp $(APP) $(PRE_INSTALL_DIR) 
	echo "Copy $(APP) to $(PRE_INSTALL_DIR)"


clean:
	$(MAKE)	clean -e -C libloragw


### EOF
