# !/bin/sh
#usage: rc.voip_routing_rule.sh add/del remote_ip_addr interface_name
#This script adds rule: "route add/del -host remote_ip_addr gw voip_gw_ip dev interface_name"
#$1 = add/del	
#$2	=	remote_ip_addr 
#$3	= interface name

ACTION=$1
RemoteIp=$2
if [ ! "$ENVLOADED" ]; then
    if [ -r /etc/rc.conf ]; then
        . /etc/rc.conf 2> /dev/null
        ENVLOADED="1"
    fi
fi
voip_gw_ip=`/usr/sbin/status_oper GET "WAN${SIP_IF}_GATEWAY" ROUTER1`

#Get Interface Name
. /etc/rc.d/get_wan_if $SIP_IF

#Add/Del from routing table
/sbin/route $ACTION -host $RemoteIp gw $voip_gw_ip dev $WAN_IFNAME

