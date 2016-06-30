#!/bin/sh
brctl delif br0 eth0
brctl delif br0 eth1
ifconfig br0 down
brctl delbr br0

ifconfig eth1 192.168.200.2 netmask 255.255.255.0
ifconfig eth0 192.168.1.1 netmask 255.255.255.0
route add -net 224.0.0.0 netmask 240.0.0.0 dev eth1

echo 2 > /proc/sys/net/ipv4/conf/all/force_igmp_version

echo "Bringup igmpproxy in 2 seconds... press Ctrl+C to terminate!"
sleep 2
/usr/bin/igmpproxy -d -c /etc/igmpproxy.conf&


