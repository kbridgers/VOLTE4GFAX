#!/bin/sh

ARGV1=$1

if [ A$ARGV1 != "A" ]; then
	if [ $ARGV1 == "nas0" ]; then
		T_US_RATE=`cat /tmp/dsl_us_rate`
		T_US_RATE=`expr $T_US_RATE \* 8 / 10000`kbps
		T_DS_RATE=`cat /tmp/dsl_ds_rate`
		T_DS_RATE=`expr $T_DS_RATE \* 8 / 10000`kbps
	elif [ $ARGV1 == "eth1" ]; then
		HTB_EXTRA_OPT="r2q 100"
		T_US_RATE=1000Mbps
		T_DS_RATE=1000Mbps
	else
		echo "Please specific interface for QoS demo in NAT mode."
		echo "Only support nas0 (AR9/ADSL) or eth1 (GR9/ETH)"
		exit 1
	fi
else
	echo "Please specific interface for QoS demo in NAT mode."
	echo "Only support nas0 (AR9/ADSL) or eth1 (GR9/ETH)"
	exit 1
fi


echo "Start QOS on $ARGV1 -- for ADSL 2+ only"

# flush previous configuration
tc qdisc del dev $ARGV1 root
iptables -t mangle -F

## configure queueing discipline
tc qdisc add dev $ARGV1 root handle 1:0 htb default 12 ${HTB_EXTRA_OPT}

## create classes
tc class add dev $ARGV1 parent 1: classid 1:1 htb rate ${T_US_RATE}

tc class add dev $ARGV1 parent 1:1 classid 1:10 htb rate ${T_US_RATE} ceil ${T_US_RATE} burst 512k cburst 512k prio 1
tc class add dev $ARGV1 parent 1:1 classid 1:11 htb rate 64kbit ceil ${T_US_RATE} burst 64k cburst 64k prio 2
tc class add dev $ARGV1 parent 1:1 classid 1:12 htb rate 5kbit ceil ${T_US_RATE} prio 3
tc qdisc add dev $ARGV1 parent 1:12 pfifo limit 10

## voice data traffic with highest priority
tc filter add dev $ARGV1 parent 1:0 prio 0 protocol ip handle 10 fw flowid 1:10
#iptables -A OUTPUT -t mangle -m tos --tos 16 -j MARK --set-mark 10
#iptables -A OUTPUT -t mangle -m tos --tos 16 -j RETURN
iptables -A OUTPUT -t mangle -m dscp --dscp 46 -j MARK --set-mark 10
iptables -A OUTPUT -t mangle -m dscp --dscp 46 -j RETURN
iptables -A OUTPUT -t mangle -m dscp --dscp 26 -j MARK --set-mark 10
iptables -A OUTPUT -t mangle -m dscp --dscp 26 -j RETURN

## up to 64 octett packets for TCP control message with higher priority
tc filter add dev $ARGV1 parent 1:0 prio 1 protocol ip handle 11 fw flowid 1:11
iptables -A OUTPUT -t mangle -o $ARGV1 -p tcp -m length --length :64 -j MARK --set-mark 11
iptables -A OUTPUT -t mangle -o $ARGV1 -p tcp -m length --length :64 -j RETURN

tc -d qdisc show dev $ARGV1
tc -d class show dev $ARGV1 
tc -d filter show dev $ARGV1


