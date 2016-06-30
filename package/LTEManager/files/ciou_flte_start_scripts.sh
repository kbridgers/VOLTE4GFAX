#!/bin/sh /etc/rc.common
#
# Install led driver and make led device node

START=75



start() {
		ifconfig br0 down && ifconfig eth0 192.168.1.1
		echo "waiting for USB device to enumerate"
		echo "ATE0\r\n" > /dev/ttyUSB3
		LTEManager &
		echo "Flte applications started successfully"
	}
