#!/bin/sh

ARGV1=$1

if [ A"$ARGV1" == "A" ]; then
	echo "Please specific WAN interface name: nas0 or eth1"
	exit 1
else
	if [ "$ARGV1" == "nas0" ]; then
		IPSEC_IF_NAME="nas0"
	elif [ "$ARGV1" == "eth1" ]; then
		IPSEC_IF_NAME="eth1"
	else
		echo "Please specific WAN interface name: nas0 or eth1"
		exit 1
	fi
	brctl delif br0 $IPSEC_IF_NAME
fi

## Interface Configuration
echo 1 > /proc/sys/net/ipv4/ip_forward
ifconfig br0 10.0.1.1 netmask 255.255.255.0
ifconfig $IPSEC_IF_NAME 20.0.0.1 netmask 255.255.255.0
route add default gw 20.0.0.2

