#!/bin/sh /etc/rc.common
#START=05
start() {
	# Optimize Routing & Conntrack cache - needs individual tuning based on model
	if [ "$CONFIG_FEATURE_IFX_LOW_FOOTPRINT" = "1" ]; then
		echo 4096 > /proc/sys/net/ipv4/route/max_size
		echo 512 > /proc/sys/net/netfilter/nf_conntrack_max
	fi
}
