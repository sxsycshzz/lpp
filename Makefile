### Environment constants

ARCH ?=
CROSS_COMPILE ?=
export

### general build targets

all:
	$(MAKE) all -e -C zigbee
	$(MAKE) all -e -C lora


pre_install:
	$(MAKE) pre_install -e -C zigbee
	$(MAKE) pre_install -e -C lora 


clean:
	$(MAKE)	clean -e -C zigbee
	$(MAKE)	clean -e -C lora


### EOF
