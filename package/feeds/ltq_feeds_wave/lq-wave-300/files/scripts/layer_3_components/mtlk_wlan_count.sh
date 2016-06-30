#!/bin/sh

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

command=$1

start_mtlk_wlan_count()
{
	print2log DBG "start mtlk_wlan_count"
	
	# wlan_count is actual number of wlan cards in system
	# Wireless can be either PCI or AHB
	pci_count=`cat /proc/bus/pci/devices | grep 1a30 -c`
	ahb_count=`ls /sys/bus/platform/devices | grep mtlk -c`
	wlan_count=`expr $pci_count + $ahb_count`
	host_api set $$ sys wlan_count $wlan_count
	print2log DBG "wlan_count=$wlan_count"
	if [ "$wlan_count" = "0" ]
	then
		print2log ALERT "mtlk_wlan_count: No wlan cards were found!!!"
		echo "mtlk_wlan_count: No wlan cards were found!!!" >> $wave_init_failure
	fi
	
	print2log DBG "finish mtlk_wlan_count"
}

stop_mtlk_wlan_count()
{
	return
}

create_config_mtlk_wlan_count()
{
	return
}

should_run_mtlk_wlan_count()
{
	true
}

case $command in
	start)
		start_mtlk_wlan_count
	;;
	stop)
		stop_mtlk_wlan_count
	;;
	create_config)
		create_config_mtlk_wlan_count
	;;
	should_run)
		should_run_mtlk_wlan_count
	;;
esac