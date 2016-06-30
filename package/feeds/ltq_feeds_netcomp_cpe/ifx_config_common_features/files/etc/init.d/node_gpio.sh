#!/bin/sh /etc/rc.common
#START=14
start() {
	/usr/sbin/mknod_util ifx_gpio /dev/ifx_gpio
}
