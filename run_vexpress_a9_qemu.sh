#!/bin/bash

sudo qemu-system-arm \
	-M vexpress-a9 \
	-m 512M \
	-kernel u-boot \
	-net nic,vlan=0 -net tap,vlan=0,ifname=tap0 \
	-nographic \
	-sd ./a9rootfs.ext3 \

