#!/bin/sh
# 1 - start to create tunnel and stop to delete tunnel 
# 2 - WAN index
# 3 - WAN IP
# 4 - ISP 6RD PREFIX
# 5 - ISP 6RD PREFIX LEN
# 6 - remote ipv4 address
# 7 - ipv4masklength
# 8 - MTU


TUNIF=tun${2}
TUNTTL=255
ISPBRIP=${6} 
WAN4IP=${3}
ISP6to4PREFIX=${4}
ISP6to4PREFIXLEN=${5}
IPv4MASKLEN=${7}
MTU=${8}



sixrd_radvd_up()
{
 PREFIX=$br0_addr/64
 PLIFETIME=604800
 VLIFETIME=2592000
 EVENT_SRC=sixrd
 SRC_INTF=$TUNIF
 EVENT_TYPE=UP
 INTF=br0
 if [ ! -z "$MTU" ]; then
	if [ $MTU -ge 1280 ]; then
		IPV6_MTU=$MTU
	fi

 fi
 . /etc/rc.d/update_and_run_radvd.sh update
}



sixrd_radvd_down()
{
 EVENT_SRC=sixrd
 SRC_INTF=$TUNIF
 EVENT_TYPE=DOWN
 . /etc/rc.d/update_and_run_radvd.sh update
}



start()
{
 echo "Starting"
 insmod /lib/modules/*/tunnel4.ko 2> /dev/null
 insmod /lib/modules/*/sit.ko 2> /dev/null
 sleep 1
 ipv6_addr=$(/usr/sbin/ipv6helper -p $ISP6to4PREFIX -i $WAN4IP -m $IPv4MASKLEN -l $ISP6to4PREFIXLEN -t sixrd)
 ret=$?
 if [ "$ret" = "0" ]; then
        tunnel_addr=$(echo $ipv6_addr | awk '{print $1}')
        br0_addr=$(echo $ipv6_addr | awk '{print $2}')
 ip tunnel add $TUNIF mode sit ttl $TUNTTL remote $ISPBRIP  local $WAN4IP
if [ ! -z "$MTU" ]; then
	if [ $MTU -ge 1280 ]; then
 		ifconfig $TUNIF mtu $MTU
	fi
fi
 ifconfig $TUNIF up
 ip -6 addr add $tunnel_addr/$ISP6to4PREFIXLEN dev $TUNIF
 ip -6 addr add $br0_addr/64 dev br0
 echo "$br0_addr/64" > /tmp/6addrbr0.conf
 route -A inet6 add default dev $TUNIF
 sixrd_radvd_up
 /bin/echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
 ppacmd addwan -i $TUNIF
 fi
 return;
}



stop()
{

 echo "Stoping"
ppacmd delwan -i $TUNIF
ifconfig $TUNIF down
ip tunnel del $TUNIF
ip -6 addr del $(cat /tmp/6addrbr0.conf) dev br0
sixrd_radvd_down
return ;
}

wan_restart()
{
echo "Bringup 6rd wan re-started for wan index ${2} ${3} !!"
if [ ${3} == "ip" ]; then
	. /etc/rc.d/rc.bringup_wanip_stop ${2}
	sleep 1
	. /etc/rc.d/rc.bringup_wanip_start ${2}
else
	. /etc/rc.d/rc.bringup_wanppp_services_stop ${2}
	sleep 1
	. /etc/rc.d/rc.bringup_wanppp_services_start ${2}
fi

}


usage()
{
echo -e "usage: \n Create a tunnel : \"$0 start wan_index wan_ip ispprefix ispprefix_length remote_ip IPv4MASKLEN MTU(optional)\" \n Delete the tunnel : \"$0 stop wan_index\" \n NOTE: MTU=1280 is recommended while connecting to Internet (6RD Comcast etc..) as per RFC 2460 : Section 5 - Packet Size Issues. \n Otherwise to get default MTU, leave this field blank."
exit
}


case "$1" in
     start)
		start
		;;
     stop)
		stop
		;;
     wan_restart)
		wan_restart $@
		;;
     *)
		usage
		;;
esac

