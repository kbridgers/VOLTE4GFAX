#!/bin/sh /etc/rc.common
#START=29
start() {
	if [ "$CONFIG_PACKAGE_KMOD_IFX_NFEXT" = "1" ]; then
		/bin/mknod /dev/vlan c 69 0
	fi
}
