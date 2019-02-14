### Environment constants

ARCH ?=
CROSS_COMPILE ?=
export

CP=cp -a

### general build targets

all:
	$(MAKE) all -e -C zigbee
	$(MAKE) all -e -C lora
	$(MAKE) all -e -C systemd_net


pre_install:
	$(MAKE) pre_install -e -C zigbee
	$(MAKE) pre_install -e -C lora 
	$(MAKE) pre_install -e -C systemd_net

install: pre_install
	$(CP) deployment/* /

clean:
	$(MAKE)	clean -e -C zigbee
	$(MAKE)	clean -e -C lora
	$(MAKE)	clean -e -C systemd_net


### EOF
