#!/bin/sh /etc/rc.common
#START=61
start() {
	if [ ! -s /etc/resolv.conf ]; then
		echo "nameserver 168.95.1.1" > /etc/resolv.conf
	fi
}
