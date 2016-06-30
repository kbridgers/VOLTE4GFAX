#!/bin/sh
if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		 . /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

. /etc/rc.d/common_func.sh

echo "WAN Changeover Stop "

# This script optionally takes an argument which will tell what is next mode
# that needs to be configured. 
if [ "$#" != "0" ]; then 
	next_mode=$1
else
	next_mode="-1"
fi

eval any_wan=`/usr/sbin/status_oper GET AnyWan status`
if [ "A$any_wan" == "A1" ]; then #check for anywan status
        anywan_flow="1"
fi

# Funtion to pause DSL. But not stop the process itself - NOT USED now.
pause_dsl() {
	if [ "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" = "1" ]; then
		bonding=`/usr/sbin/status_oper GET xDSL_Bonding Bonding_mode`
		line_num="-1"
	fi
	BIN_DIR=/opt/lantiq/bin

	# Take care to set DSL line to a defined state (no link)
	${BIN_DIR}/dsl_cpe_pipe.sh acos $line_num 1 1 1 0 0 0
	${BIN_DIR}/dsl_cpe_pipe.sh acs $line_num 2
	sleep 3

	# Stop autoboot handling (including freeing up resources)
	${BIN_DIR}/dsl_cpe_pipe.sh acs $line_num 0
}

# Function to Stop the DSL process and unload the DSL driver.
stop_dsl() {
	echo "Stop DSL"
	# check if already running and stop
	grep -qw dsl_cpe_control /proc/*/status && {
		/etc/init.d/ltq_cpe_control_init.sh stop
	} || {
#		echo "DSL process not running"
		echo -n
	}
	# this sleep is required without which the DSL process crashes
	# when teh driver is getting unloaded
	sleep 3
	`grep -q "drv_dsl_cpe_api" /proc/modules` && {
		/etc/init.d/ltq_load_dsl_cpe_api.sh stop
	} || {
#		echo "DSL Driver not loaded"
		echo -n
	}
}

wanStopped="0"
non_dsl_mode="0"
is_non_dsl_mode

if [ "$non_dsl_mode" = "0" ]; then
	# Handle DSL mode HERE
	wanStopped="1"
	do_wan_config changeover_stop
fi

# if the next_mode is not DSL, then stop the DSL process from running further
if [ "$next_mode" = "0" -o "$next_mode" = "3" -o "$next_mode" = "dsl" -o "$next_mode" = "qos" ]; then
#	echo "next_mode - $next_mode"	
	echo -n
else
	stop_dsl &
fi


#echo "Stopping WAN Services"
if [ -n "$dw_failover_state" -a "$dw_failover_state" = "1" ]; then
	#stop all wan connections on primary and secondary
	. /etc/init.d/dw_daemon.sh stop_wan
	# in case of failover mode only one WAN phy mode is active at a time so the port binding rules on the existing wan modes needs to be removed
	if [ "$dw_standby_type" = "1" ]; then
		if [ "$CONFIG_FEATURE_LTQ_PORT_WAN_BINDING" = "1" -a "$port_wan_binding_status_enable" = "1" ]; then
			. /etc/rc.d/ltq_pwb_config.sh del_all
		fi
	fi
else
	# DSL WAN down is handled above - HERE
	if [ "$non_dsl_mode" = "1" -a "$wanStopped" = "0" ]; then
		do_wan_config changeover_stop
	fi

	if [ "$CONFIG_FEATURE_LTQ_PORT_WAN_BINDING" = "1" -a "$port_wan_binding_status_enable" = "1" ]; then
		. /etc/rc.d/ltq_pwb_config.sh del_all
	fi

fi

# Stop IPQoS if already enabled
if [ -z  "$anywan_flow" ]; then #check for anywan
	if [ $qm_enable -eq 1 ]; then
		#echo " DISABLING IPQOS "
		/etc/rc.d/ipqos_disable
	fi
	killall qos_rate_update
fi

#echo "Deregistering WLAN from direct path"
/etc/init.d/ltq_wlan_unregister_ppa.sh

. /etc/init.d/ltq_switch_config.sh undo_switch_config

# Get the previous wan mode and ppa status
# Stop ppacmd if PPA is enabled in current state
if [ "A$CONFIG_FEATURE_PPA_SUPPORT" = "A1" ]; then
	/sbin/ppacmd exit
fi

#echo "Unloading the ppa driver modules"
. /etc/init.d/unload_ppa_modules.sh

# Retrieved the cached memory
echo 1 > /proc/sys/vm/drop_caches; usleep 250000

