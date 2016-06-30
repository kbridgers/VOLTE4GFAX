#!/bin/sh /etc/rc.common
#START=31

#Need to enable admmod module insertion once in
start() {
	if [ "$CONFIG_FEATURE_IFX_ADM6996_UTILITY" = "1" ]; then
		/bin/mknod /dev/adm6996 c 69 0
		/sbin/insmod admmod
	fi
}
