#!/bin/sh /etc/rc.common
#START=36
start() {
	if [ -f /usr/sbin/dnrd ]; then
		rm -rf /ramdisk/etc/dnrd
		mkdir /ramdisk/etc/dnrd
		/usr/sbin/dnrd
	fi
}
