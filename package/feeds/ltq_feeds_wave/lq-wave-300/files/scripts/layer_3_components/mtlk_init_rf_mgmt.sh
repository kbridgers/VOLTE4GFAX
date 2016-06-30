#!/bin/sh
return_status=true

# This script must run before creating drvhlpr.conf because drvhlpr uses the RFMgmtEnable parameter.
	
#Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

command=$1
apIndex=$2

# Get corresponding wlan network interface from mapping file
wlan=`find_wave_if_from_index $apIndex`
	
start_mtlk_init_rf_mgmt()
{
	print2log DBG "mtlk_init_rf_mgmt.sh: Start"

	RFMgmtEnable=0

	print2log DBG "mtlk_init_rf_mgmt.sh: wlan=$wlan"

	# Beamforming can be:
	# 0 == Off (in case of 2 antennas, this is the only option)
	# 1 == Antenna selection mode (in case of 6 antennas where Ant_selection=1)
	# 2 == Explicit beamforming (not used)
	# 3 == Implicit beamforming (in case of 3 antennas, this is the default value)
	Ant_selection=`host_api get $$ hw_$wlan Ant_selection`
	Support_3TX=`host_api get $$ hw_$wlan Support_3TX`
	if [ "$Ant_selection" = "1" ]
	then
		RFMgmtEnable=1
	else
		if [ "$Support_3TX" = "0" ]
		then
			RFMgmtEnable=0
		fi
	fi
	host_api set $$ $apIndex RFMgmtEnable $RFMgmtEnable
	print2log DBG "mtlk_init_rf_mgmt.sh: Done Start"
}

stop_mtlk_init_rf_mgmt()
{
    return
}

create_config_mtlk_init_rf_mgmt()
{
    return
}

should_run_mtlk_init_rf_mgmt()
{
	# RFMgmtEnable can be changed by the user only in the case of Support_3TX=1 and Ant_selection=0
	# On other cases, the value will be set by this script.
	
	print2log DBG "mtlk_init_rf_mgmt.sh: should_run"
	
	RFMgmtEnable=`host_api get $$ $apIndex RFMgmtEnable`
	Ant_selection=`host_api get $$ hw_$wlan Ant_selection`
	Support_3TX=`host_api get $$ hw_$wlan Support_3TX`
	if [ "$Support_3TX" = "1" ] && [ "$Ant_selection" = "0" ]
	then
		return_status=false
	fi
	if [ -e $wave_init_failure ]
	then
		return_status=false
	fi
}

case $command in
    start)
    	start_mtlk_init_rf_mgmt
    ;;
    stop)
    	stop_mtlk_init_rf_mgmt
    ;;
    create_config)
    	create_config_mtlk_init_rf_mgmt
    ;;
    should_run)
    	should_run_mtlk_init_rf_mgmt
    ;;
esac

$return_status