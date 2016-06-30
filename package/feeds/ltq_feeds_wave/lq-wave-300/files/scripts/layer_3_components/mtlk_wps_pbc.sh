#!/bin/sh
return_status=true

if [ ! "$CONFIGLOADED" ]; then
	if [ -r /etc/rc.d/config.sh ]; then
		. /etc/rc.d/config.sh 2>/dev/null
		CONFIGLOADED="1"
	fi
fi

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

print2log DBG "mtlk_wps_pbc.sh: args: $*"

command=$1
apIndex=$2

# Get corresponding wlan network interface from mapping file
wlan=`find_wave_if_from_index $apIndex`

start_mtlk_wps_pbc()
{
	print2log DBG "start mtlk_wps_pbc"
	NETWORK_TYPE=`host_api get $$ $apIndex network_type`
	if [ ! -e /tmp/wps_pbc.sh ]; then ln -s $ETC_PATH/wps_pbc.sh /tmp/wps_pbc.sh; fi
	action_type='get_conf_via_pbc'
	if [ "$NETWORK_TYPE" = "$AP" ]; then action_type='conf_via_pbc'; fi		
	$ETC_PATH/wps_pbc.sh $action_type $apIndex &
	print2log DBG "Finish start mtlk_wps_pbc"
}

stop_mtlk_wps_pbc()
{
	killall wps_pbc.sh
}

create_config_mtlk_wps_pbc()
{
	return
}

should_run_mtlk_wps_pbc()
{
	if [ -e $wave_init_failure ]
	then
		return_status=false
		return
	fi
	
	# Currently, WPS PBC is only relevant for VB300 and ARX168
	if [ "$CONFIG_IFX_MODEL_NAME" != "GRX168_RT_HE_VIDEO_BRIDGE" -a "$CONFIG_IFX_MODEL_NAME" != "ARX168_RT_HE_BASE" ]
	then
		return_status=false
		return
	fi
	
	# For now allow WPS session for wlan0 only
	if [ "$wlan" != "wlan0" ]
	then
		return_status=false
	else 
		WPS_PBC_GPIO=`host_api get $$ hw_wlan0 WPS_PB`
		WPS_ON=`get_wps_on $apIndex $wlan`
		if [ "$WPS_ON" != "$YES" ] || [ "$WPS_PBC_GPIO" == "null" ]
		then
			return_status=false
		fi
	fi
}

case $command in
	start)
		start_mtlk_wps_pbc 
	;;
	stop)
		stop_mtlk_wps_pbc
	;;
	create_config)
		create_config_mtlk_wps_pbc
	;;
	should_run)
		should_run_mtlk_wps_pbc
	;;
esac

$return_status