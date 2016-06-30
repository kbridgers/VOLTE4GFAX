#!/bin/sh
# A pushbutton daemon

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

action_type=$1
apIndex=$2

# Get corresponding wlan network interface from mapping file
wlan=`find_wave_if_from_index $apIndex`

# For now allow WPS session for wlan0 only
# TODO: If we reached this script, validation was already done, so the following is probably redundant
WPS_PBC_GPIO=`host_api get $$ hw_wlan0 WPS_PB`
WPS_ON=`get_wps_on $apIndex $wlan`
if [ "$#" -eq "0" ] || [ "$WPS_PBC_GPIO" = "null" ] || [ ! -e "$WPS_PBC_GPIO" ] || [ "$WPS_ON" = "$NO" ]
then
	echo "WARNING: WPS PBC is not operational.  Reason: WPS_PBC_GPIO=$WPS_PBC_GPIO, WPS_ON=$WPS_ON" > /dev/console
	return
fi

# Perform init sequence for PBC 
# TODO: pbc_wps should be the same as WPS_PBC_GPIO. We should have only one location for definitions.
( . $ETC_PATH/wave_wlan_gpio_ctrl.sh pbc_init $WPS_PBC_GPIO $pbc_wps_trigger )


while [ true ]
do
	# Wait for the button to be pushed ("cat" is stuck until then)
	( . $ETC_PATH/wave_wlan_gpio_ctrl.sh pbc_wait $WPS_PBC_GPIO )
	
	# Execute the WPS command (conf_via_pbc or get_conf_via_pbc)
	$ETC_PATH/mtlk_wps_cmd.sh $action_type $apIndex
	sleep 1
done

