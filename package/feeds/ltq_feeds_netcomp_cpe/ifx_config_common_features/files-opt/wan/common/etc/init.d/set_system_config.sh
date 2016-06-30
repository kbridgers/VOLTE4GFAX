#!/bin/sh /etc/rc.common

START=07

platform=${CONFIG_IFX_MODEL_NAME%%_*}

# changed min_free_kbytes to have more free memory a given point of execution
set_mem_opt()
{
	if [ "$platform" = "DANUBE" ]; then
	# handle heavy traffic scenario with resulted in page allocation error of order 0
		echo 1024 > /proc/sys/vm/min_free_kbytes
		echo 2048 > /proc/sys/net/ipv4/route/max_size
	elif [ "$platform" = "AMAZON" ]; then
		echo 1200 > /proc/sys/vm/min_free_kbytes
		echo 2048 > /proc/sys/net/ipv4/route/max_size
		echo 512 > /proc/sys/net/netfilter/nf_conntrack_max
	else
		if [ $CONFIG_UBOOT_CONFIG_IFX_MEMORY_SIZE = "32" ]; then
			echo 4096 > /proc/sys/vm/min_free_kbytes
		else
			echo 1024 > /proc/sys/vm/min_free_kbytes
		fi
		echo 4096 > /proc/sys/net/ipv4/route/max_size
	fi

	# memory tunning for all platform lowmem_reserve_ratio is a MUST
	echo 250 >  /proc/sys/vm/lowmem_reserve_ratio
	echo 2 > /proc/sys/vm/dirty_background_ratio
	echo 250 > /proc/sys/vm/dirty_writeback_centisecs
	echo 10 > /proc/sys/vm/dirty_ratio
	echo 16384 > /proc/sys/vm/max_map_count
	echo 2 > /proc/sys/vm/page-cluster
	echo 70 > /proc/sys/vm/swappiness
}


start(){
	set_mem_opt

	# Optimize Routing & Conntrack cache - needs individual tuning based on model
	if [ "$CONFIG_FEATURE_IFX_LOW_FOOTPRINT" = "1" ]; then
		echo 4096 > /proc/sys/net/ipv4/route/max_size
		echo 512 > /proc/sys/net/netfilter/nf_conntrack_max
		echo 50 > /proc/sys/net/netfilter/nf_conntrack_expect_max
	else
		echo 1984 > /proc/sys/net/netfilter/nf_conntrack_max
		echo 70 > /proc/sys/net/netfilter/nf_conntrack_expect_max
	fi
	if [ "$CONFIG_IFX_CONFIG_CPU" = "AMAZON_SE" ];then
		echo 512 > /proc/sys/net/netfilter/nf_conntrack_max
		echo 40 > /proc/sys/net/netfilter/nf_conntrack_expect_max
	fi

	# WAN specific configuration to be made available before doing WAN startup
	# Can't be put in DSL startup script since it doesn't exist on all models
	# Can't be put in generic WAN startup script, since DSL starts before WAN and might trigger WAN start before WAN startup script
	# names are misnomer !
	/usr/sbin/status_oper SET "wan_link_status" "link_up_once" "0"
}
