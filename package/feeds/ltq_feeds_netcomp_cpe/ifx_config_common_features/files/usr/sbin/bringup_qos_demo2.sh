#!/bin/sh

ARGV1=$1

if [ A$ARGV1 == "A" ]; then
	echo "Please specific interface for QoS demo in NAT mode."
	echo "Only support nas0 (AR9/ADSL) or eth1 (GR9/ETH)"
	exit 1
fi


echo "Start QOS on $ARGV1 -- for ADSL 2+ only"

# flush previous configuration
tc qdisc del dev $ARGV1 root
iptables -t mangle -F

## configure queueing discipline
tc qdisc add dev $ARGV1 root handle 1: prio default 3

## create classes
tc filter add dev $ARGV1 parent 1:0 prio 1 protocol ip handle 10 fw flowid 1:1
tc filter add dev $ARGV1 parent 1:0 prio 2 protocol ip handle 11 fw flowid 1:2
tc filter add dev $ARGV1 parent 1:0 prio 3 protocol ip handle 12 fw flowid 1:3

## voice data/call traffic with highest priority
iptables -A OUTPUT -t mangle -m dscp --dscp 46 -j MARK --set-mark 10
iptables -A OUTPUT -t mangle -m dscp --dscp 46 -j RETURN
iptables -A OUTPUT -t mangle -m dscp --dscp 26 -j MARK --set-mark 10
iptables -A OUTPUT -t mangle -m dscp --dscp 26 -j RETURN
## up to 64 octett packets for TCP control message with higher priority
iptables -A OUTPUT -t mangle -p tcp -m length --length :64 -j MARK --set-mark 11
iptables -A OUTPUT -t mangle -p tcp -m length --length :64 -j RETURN
iptables -A OUTPUT -t mangle -j MARK --set-mark 12

## others traffic
iptables -A PREROUTING -t mangle -j MARK --set-mark 12

tc -d qdisc show dev $ARGV1
tc -d class show dev $ARGV1 
tc -d filter show dev $ARGV1


