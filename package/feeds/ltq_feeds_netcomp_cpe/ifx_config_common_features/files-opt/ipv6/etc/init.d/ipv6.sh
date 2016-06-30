#!/bin/sh /etc/rc.common
#START=49
start() {

	if [ "$CONFIG_PACKAGE_KMOD_IPV6" = "1"  -a  "$ipv6_status" = "1" ]; then
#		kernel_version=`uname -r`
		/sbin/insmod /lib/modules/*/ipv6.ko
		echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
	elif [ "$CONFIG_FEATURE_IPv6" = "1" ]; then
		echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
	fi
}
