# 1046gateway

## 1. Introduce

It is the gateway project based on ls1046a from CertusNet Inc.

## 2. Directory

This project is made up of many modules, one for each directory.

The directory **app_pkg** is for app installation packages which will be used on the ls1046a board.

The directory **deployment** is for the install of the software on the ls1046a board. Then we can unzip the **deployment.tar.gz** with the command on the board:

	tar --no-same-owner -zxvf deployment.tar.gz -C /

However, the tar should be created with the commands:
	
	cd deployment
	tar zcvf deployment.tar.gz *

## 3. Make
Run following command to compile the whole project. 

**Note:** The **CC** should point to the actual position on your host.
	
	make     // for X86 or arm64 on host
	make ARCH=arm64 CC="/home/wang/openil/openil/output/host/usr/bin/aarch64-linux-gnu-gcc"   // for arm64,  cross compile on X86 PC

Run **make pre_install** command to install the final files to the **deployment** directory.

Run **make install** command to install the final files on the ls1046a board if make directly on the ls1046a board. 


