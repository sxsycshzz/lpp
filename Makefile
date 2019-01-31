### Environment constants

ARCH ?=
CROSS_COMPILE ?=
export

### general build targets

all:
	$(MAKE) all -e -C zigbee


pre_install:
	$(MAKE) pre_install -e -C zigbee


clean:
	$(MAKE)	clean -e -C zigbee


### EOF
