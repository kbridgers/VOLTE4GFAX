#!/bin/sh

# Defines
if [ ! "MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

wlan=$1

# The wave_wlan_stop script can be executed only once, so if drvhlpr tries to stop the system while stop is already executing we need to prevent it.
# Using a temp file to verify the system is not already in the process of stop.
restart_in_progress=/tmp/wlan_restart_in_progress

/tmp/drvhlpr_$wlan -p $CONFIGS_PATH/drvhlpr_$wlan.conf </dev/console 1>/dev/console 2>&1
term_stat=$?
if [ $term_stat = 1 ]
then
	if [ -e $restart_in_progress ]
	then
		echo "drvhlpr.sh: Received stop from $wlan, already in stop process - ignoring"
	else
		echo "drvhlpr.sh: MAC assert executed stop for $wlan" >> $restart_in_progress
		echo "drvhlpr.sh: wave MAC HANG"
		if [ -e /tmp/mtlk_info_dump.sh ]; then sh -c /tmp/mtlk_info_dump.sh; fi
		# Don't reboot the system in UGW - just restart the wls interfaces
		echo "drvhlpr.sh: Restarting wireless interface"
		$ETC_PATH/wave_wlan_restart
		rm $restart_in_progress
	fi
elif [ $term_stat = 2 ]
then
	echo "drvhlpr.sh: drvhlpr return rmmod"
else
	echo "drvhlpr.sh: drvhlpr terminated with status $term_stat"
fi
