FROM ubuntu:16.04

RUN apt-get update \
    && apt-get install -y build-essential libncurses-dev git git-core \
         lib32z1 lib32ncurses5 lib32stdc++6 bc\
    && mkdir /root/raspberry

WORKDIR /root/raspberry

RUN git clone --depth=1 -b rpi-4.4.y https://github.com/raspberrypi/linux && \
    git clone https://github.com/raspberrypi/tools

WORKDIR /root/raspberry/linux

RUN make KERNEL=kernel7 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2709_defconfig && \
    make KERNEL=kernel7 ARCH=arm CROSS_COMPILE=/root/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf- zImage modules dtbs -j 4

WORKDIR /root
