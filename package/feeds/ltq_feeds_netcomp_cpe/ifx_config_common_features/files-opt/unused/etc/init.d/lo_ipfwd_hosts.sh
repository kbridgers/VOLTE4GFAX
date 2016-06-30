#!/bin/sh /etc/rc.common
#START=34

start() {
	# Setup lo Interface Addresses
	/sbin/ifconfig lo 127.0.0.1 netmask 255.0.0.0
	
	echo 1 > /proc/sys/net/ipv4/ip_forward
	
	# Setup Hostname
	echo "127.0.0.1 localhost.localdomain localhost" > /etc/hosts
	if [ -f /bin/hostname ]; then
		/bin/hostname $hostname
	fi
	
	i=0
	while [ $i -lt $lan_main_Count ]
	do
		eval ip='$lan_main_'$i'_ipAddr'
		shorthost=${hostname%%.*}
		echo "$ip ${hostname} $shorthost" >> /etc/hosts
		i=$(( $i + 1 ))
	done

	if [ "$ipv6_status" = "1" ]; then
		eval hn=`uname -n`'6'
		i=0
        	while [ $i -lt $lan_main_Count ]
        	do
                	eval ip='$lan_main_ipv6_'$i'_ip6Addr'
                	echo "$ip $hn.$lan_dhcpv6_dName $hn" >> /etc/hosts
                	i=$(( $i + 1 ))
        	done
	fi
}
