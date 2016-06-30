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

print2log DBG "mtlk_drvhlpr.sh: args: $*"

command=$1
apIndex=$2

# Get corresponding wlan network interface from mapping file
wlan=`find_wave_if_from_index $apIndex`

start_mtlk_drvhlpr()
{
	print2log DBG "mtlk_drvhlpr.sh: Start"
	if [ ! -e /tmp/drvhlpr_$wlan ]; then ln -s $BINDIR/drvhlpr /tmp/drvhlpr_$wlan; fi
	$ETC_PATH/drvhlpr.sh $wlan &
	print2log DBG "mtlk_drvhlpr.sh: Done"
}

stop_mtlk_drvhlpr()
{
	print2log DBG "mtlk_drvhlpr.sh: Stop"
	A=`ps | grep "\<drvhlpr_$wlan\> " | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill $a 2>/dev/null; done
	# Repeating the loop in case drvhlpr was not down
	A=`ps | grep "\<drvhlpr_$wlan\> " | grep -v grep | awk '{print $1}'`
	for a in $A ; do kill -9 $a ; done
	print2log DBG "mtlk_drvhlpr.sh: Done stop mtlk_drvhlpr"
}

create_config_mtlk_drvhlpr()
{
	print2log DBG "mtlk_drvhlpr.sh: create_config"
	drvhlprConfFile=$CONFIGS_PATH/drvhlpr_$wlan.conf
	if [ "$wlan" = "wlan0" ]
	then
		wps_script_path=$ETC_PATH/mtlk_wps_event.sh
	else
		wps_script_path=""
	fi
	wls_link_script_path=$ETC_PATH/mtlk_linkstat_event.sh
	data_link_script_path=$ETC_PATH/mtlk_data_link_status.sh
	leds_conf_file=$ETC_PATH/leds.conf
	drvhlpr_params=/tmp/drvhlpr_params.sh
	
	# Read all parameters into a temp file to hold all current values and source that file.
	host_api get_all $$ $apIndex 'gen_bd_cfg|wlan_wave300|wlan_sec|wlan_phy'> $drvhlpr_params
	. $drvhlpr_params 2>/dev/null
	
	#The driver NonProcSecurityMode value is decreased by one from the web value
	NonProcSecurityMode=`expr $NonProcSecurityMode - 1`
		
	cp $leds_conf_file $drvhlprConfFile
	
	echo "network_type = $network_type" >> $drvhlprConfFile
	# some parameters are only for physical AP
	if [ "$network_type" = "$AP" ]
	then
		echo "wps_script_path = $wps_script_path" >> $drvhlprConfFile
		echo "RFMgmtEnable = $RFMgmtEnable" >> $drvhlprConfFile
		echo "RFMgmtRefreshTime = $RFMgmtRefreshTime" >> $drvhlprConfFile
		echo "RFMgmtKeepAliveTimeout = $RFMgmtKeepAliveTimeout" >> $drvhlprConfFile
		echo "RFMgmtAveragingAlpha = $RFMgmtAveragingAlpha" >> $drvhlprConfFile
		echo "RFMgmtMetMarginThreshold = $RFMgmtMetMarginThreshold" >> $drvhlprConfFile
	fi

	echo "Debug_SoftwareWatchdogEnable = $Debug_SoftwareWatchdogEnable" >> $drvhlprConfFile
	echo "NonProcSecurityMode = $NonProcSecurityMode" >> $drvhlprConfFile
	echo "interface = $wlan" >> $drvhlprConfFile
	echo "arp_iface0 = eth0" >> $drvhlprConfFile
	arp_eth1=`grep eth1 /proc/net/dev`
	if [ "$arp_eth1" ]; then echo "arp_iface1 = eth1" >> $drvhlprConfFile; fi

	echo "led_resolution = 1" >> $drvhlprConfFile
	echo "wls_link_script_path = $wls_link_script_path" >> $drvhlprConfFile
	echo "wls_link_status_script_path = $data_link_script_path" >> $drvhlprConfFile

	print2log DBG "mtlk_drvhlpr.sh: Done create_config"
}

should_run_mtlk_drvhlpr()
{
	if [ -e $wave_init_failure ]
	then
		return_status=false
	fi
}

case $command in
	start)
		start_mtlk_drvhlpr 
	;;
	stop)
		stop_mtlk_drvhlpr
	;;
	create_config)
		create_config_mtlk_drvhlpr
	;;
	should_run)
		should_run_mtlk_drvhlpr
	;;
esac

$return_status