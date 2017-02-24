HOME=/root
ARCH:=arm
CROSS_COMPILE:=$(HOME)/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-
KPATH:=$(HOME)/raspberry/linux
CFLAGS_mpu9250.o:=-std=gnu99 -Wall -Wno-declaration-after-statement

obj-m:=mpu9250.o

all:
	make V=$(VERBOSE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KPATH) M=$(PWD) modules

tags:
	gtags -vv

clean:
	make -C $(KPATH) M=$(PWD) clean
