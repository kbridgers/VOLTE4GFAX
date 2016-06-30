#!/bin/sh

if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
	 . /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

eval ppp_iface='$'wanppp_${1}_ifppp
peer_ip=`ifconfig $WAN 2> /dev/null`
if [ $? -eq 0 ]; then
	peer_ip=${peer_ip%%Mask*}
	peer_ip=${peer_ip##*:}
fi
/bin/ping -c 4 $peer_ip
peer_ip=`ifconfig $WAN 2> /dev/null`
if [ $? -eq 0 ]; then
	peer_ip=${peer_ip%%Mask*}
	peer_ip=${peer_ip##*:}
fi
/usr/sbin/status_oper SET "Wan${1}_If_Info" STATUS CONNECTED PEER_IP $peer_ip

echo "0"
