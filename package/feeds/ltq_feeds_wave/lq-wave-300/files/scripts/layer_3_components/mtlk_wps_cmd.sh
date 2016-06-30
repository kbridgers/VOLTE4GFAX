#!/bin/sh

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

print2log DBG "mtlk_wps_cmd.sh: args: $*"

action=$1
apIndex=$2

# Get corresponding wlan network interface from mapping file
wlan=`find_wave_if_from_index $apIndex`

#for now, when working with WPS single-band, the tmp_wps_folder will be /tmp.
# When inserting CDB WPS, we will use /tmp/$wlan
tmp_wps_folder=/tmp

#File names
HOSTAPD_CONF="$tmp_wps_folder/hostapd_$wlan.conf"
SUPPLICANT_CONF="$tmp_wps_folder/wpa_supplicant0.conf"
HOSTAPD_CLI="$tmp_wps_folder/hostapd_cli"
WPA_CLI="$tmp_wps_folder/wpa_cli"
WPS_CMD_SCRIPT="mtlk_wps_cmd.sh"
WPS_ACTION_STATFILE="$tmp_wps_folder/wps_action"

#Since events are not yet supported in UGW, the following files left as a comment as a preparation for supporting drvhlpr events.
#WPS_START_TIME_STATFILE="$tmp_wps_folder/wps_startup_time"
#This variable should be used by the display layer to figure out what the current WPS status is 
#WPS_CURRENT_STATUS_STATFILE="$tmp_wps_folder/wps_current_status"
#WPS_LAST_CODE_STATFILE="$tmp_wps_folder/wps_last_code"


#Constant variables
STA=0
AP=2
NOT_PBC=0
IS_PBC=1
STA_enrollee=0
AP_enrollee=2
AP_proxy=1

CMD_QUIT=0
CMD_AP_GET_CONF=1
CMD_REG_CONF_AP=2
CMD_STA_GET_CONF=3
CMD_REG_CONF_STA=4
CMD_AP_ABORT=5

ap_select=0
securityOpen=1
securityWEP=2
securityWPAPersonal=3

cmd_length=2
wps_msg_length=256

WPS_Active_Blink=6
WPS_Idle_Blink=0

network_type=`host_api get $$ $apIndex network_type`

#Setting a parameter to know if we are using hostapd_wlanX or supplicant
if [ "$network_type" = "$AP" ]
then
	Hostapd_Supplicant="hostapd_$wlan"
elif [ "$network_type" = "$STA" ]
then
	Hostapd_Supplicant="wpa_supplicant"
fi

# see if this is not the first instance, if another instance is alive, abort the run
# Since we saw some cases where ps detected too many processes, we set that the ps would be printed to the screen in case on the error.
ps_result=`ps`
isWpsCmd=`echo $ps_result | grep "\<$WPS_CMD_SCRIPT\>" | grep -vc grep`
#isWpsCmd=`ps | grep "\<$WPS_CMD_SCRIPT\>" | grep -vc grep`
if [ $isWpsCmd ] && [ $isWpsCmd -gt 2 ]
then
	print2log ERROR "mtlk_wps_cmd: Aborted. $WPS_CMD_SCRIPT is already running"
	#echo "ps result: $ps_result" > /dev/console 
	exit
fi
	
showUsage()
{
	echo -e "Wave300 WPS control script"
	echo -e "This script is used to control the WPS Session state"
	echo -e "Syntax :"
	echo -e "mtlk_wps_cmd.sh action "
	echo -e "\nactions : "
	echo -e "conf_via_pin <PIN> : Configure a STA via PIN. "
	echo -e "\t PIN : PIN Number"
	echo -e "conf_via_pbc : Configure a STA using PBC"
	echo -e "get_conf_via_pin <MANUAL>: Get configured by an AP using PIN"
	echo -e "\t MANUAL : 1 for manual AP selection, 0 for automatic AP selection"
	echo -e "get_conf_via_pbc : Get configured by an AP using PBC"
	echo -e "ap_selected_from_list <MAC> : Select an AP to connect to."
	echo -e "\t MAC  : MAC Address of AP to connect to"
	echo -e "show_settings : Display current WPS settings"
	echo -e "save_settings : Save current WPS session settings to configuration files."
}

checkAPconfigured()
{
	print2log DBG "mtlk_wps_cmd: Start checkAPconfigured"
	WPS_ApStatus=`host_api get $$ $apIndex cfgState`
	
	#If it is AP and it is non-configured.
	if [ "$network_type" = "$AP" ] && [ "$WPS_ApStatus" = "$Unconfigured_AP" ]
	then
		print2log DBG "mtlk_wps_cmd: checkAPconfigured. AP is non-configured, setting to configured"
		host_api set $$ $apIndex cfgState $Configured_AP
		host_api commit $$
		(. $ETC_PATH/mtlk_init_hostapd.sh stop $apIndex)
		(. $ETC_PATH/mtlk_init_hostapd.sh should_run $apIndex) && (. $ETC_PATH/mtlk_init_hostapd.sh create_config $apIndex) && (. $ETC_PATH/mtlk_init_hostapd.sh start $apIndex)
	else
		print2log DBG "mtlk_wps_cmd: checkAPconfigured. AP is in configured mode"
	fi
	print2log DBG "mtlk_wps_cmd: End checkAPconfigured"
}

get_param() {
    string=`grep "^\<$1\>" $2 | sed 's/^[^=]*=[ ]*//'`
    if [ "$1" = "ssid" ] || [ "$1" = "wpa_passphrase" ]
    then
        ascii2hex "$string"
    else
        echo $string
    fi
}

stopWPSPBC()
{
	#PBC functions are not supported at the moment and will be changed when it will be supported by UGW.
	print2log DBG "mtlk_wps_cmd: Start stopPBC"
	killall wps_pbc.sh

	wpsPBCgpio=`host_api get $$ hw_$wlan WPS_PB`
	if [ $wpsPBCgpio ] && [ "$wpsPBCgpio" != "null" ]
	then
		A=`ps | grep "\<cat $wpsPBCgpio\>" | grep -v grep | awk '{print $1}'`
		for a in $A ; do kill -9 $a ; done
	fi
	print2log DBG "mtlk_wps_cmd: End stopPBC"
}

WPSActionStart()
{
	print2log DBG "mtlk_wps_cmd: Start WPSActionStart"
	# erase Wildcard_ESSID if not empty
	wildcard_essid=`iwpriv $wlan gActiveScanSSID | sed 's#.*gActiveScanSSID[^:]*:[ ]*##'`
	if [ "$wildcard_essid" ]
	then
		print2log DBG "mtlk_wps_cmd: WPSActionStart erasing ActiveScanSSID"
		iwpriv $wlan sActiveScanSSID ""
	fi
	
	isHostapdSupplicantAlive=`ps | grep "\<$Hostapd_Supplicant\> " | grep -vc grep`
	#If hostapd/supplicant is not up, start it.
	if [ $isHostapdSupplicantAlive ] && [ $isHostapdSupplicantAlive = 0 ]
	then
		if [ "$network_type" = "$STA" ]
		then
			print2log DBG "mtlk_wps_cmd: WPSActionStart starting supplicant"
			(. $ETC_PATH/mtlk_init_wpa_supplicant.sh should_run $apIndex) && (. $ETC_PATH/mtlk_init_wpa_supplicant.sh create_config $apIndex) && (. $ETC_PATH/mtlk_init_wpa_supplicant.sh start $apIndex)
		else
			print2log DBG "mtlk_wps_cmd: WPSActionStart starting hostapd"
			(. $ETC_PATH/mtlk_init_hostapd.sh should_run $apIndex) && (. $ETC_PATH/mtlk_init_hostapd.sh create_config $apIndex) && (. $ETC_PATH/mtlk_init_hostapd.sh start $apIndex)
		fi		
		killall -USR2 drvhlpr
	fi

	# Reload the WPS_PBC.sh if it's not there.
	isWpsPbcAlive=`ps | grep wps_pbc | grep -vc grep`
	if [ $isWpsPbcAlive ] && [ $isWpsPbcAlive = 0 ]
	then
		print2log DBG "mtlk_wps_cmd: WPSActionStart reloading WPS_PBC"
		(. $ETC_PATH/mtlk_wps_pbc.sh should_run $apIndex) && (. $ETC_PATH/mtlk_wps_pbc.sh start $apIndex)
	fi
	print2log DBG "mtlk_wps_cmd: End WPSActionStart"
}

WPSActionStop ()
{
	print2log DBG "mtlk_wps_cmd: Start WPSActionStop"
	
	stopWPSPBC
	
	rm $WPS_ACTION_STATFILE

	killall -USR1 drvhlpr
	
	# reload hostapd/supplicant
	if [ "$network_type" = "$STA" ]
	then
		print2log DBG "mtlk_wps_cmd: WPSActionStop reloading supplicant"
		(. $ETC_PATH/mtlk_init_wpa_supplicant.sh should_run $apIndex) && (. $ETC_PATH/mtlk_init_wpa_supplicant.sh create_config $apIndex) && (. $ETC_PATH/mtlk_init_wpa_supplicant.sh reload $apIndex)
	else
		print2log DBG "mtlk_wps_cmd: WPSActionStop reloading hostapd"
		(. $ETC_PATH/mtlk_init_hostapd.sh should_run $apIndex) && (. $ETC_PATH/mtlk_init_hostapd.sh reconfigure $apIndex)
	fi
	
	print2log DBG "mtlk_wps_cmd: End WPSActionStop"
}

WPSActionAPAbort()
{
	print2log DBG "mtlk_wps_cmd: Start WPSActionAPAbort"
	WPSActionStop
	print2log DBG "mtlk_wps_cmd: End WPSActionAPAbort"
}

# Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
# getWPSSessionStat()
# {
#	 print2log DBG "mtlk_wps_cmd: Start getWPSSessionStat"
#	
#	 lastCode=`get_param WPS_LastErrorCode $WPS_LAST_CODE_STATFILE`
#	
#	 isHostapdSupplicantAlive=`ps | grep "\<$Hostapd_Supplicant\>" | grep -vc grep`
#	 if [ $lastCode ] && [ $isHostapdSupplicantAlive -gt 0 ]
#	 then
#		 print2log DBG "mtlk_wps_cmd: getWPSSessionStat last code is $lastCode"
#		 echo $lastCode
#	else
#		 print2log DBG "mtlk_wps_cmd: getWPSSessionStat no last code or $Hostapd_Supplicant is not alive"
#		 echo 0
#	fi
#	
#	print2log DBG "mtlk_wps_cmd: End getWPSSessionStat"
# }

#Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
#CheckWPSManualSessionTimeout ()
#{
#	print2log DBG "mtlk_wps_cmd: Start CheckWPSManualSessionTimeout"
#	
#	wpsSessionStat=`getWPSSessionStat`
#	if [ $wpsSessionStat = 5 ] || [ $wpsSessionStat = 0 ]
#	then
#		print2log DBG "mtlk_wps_cmd: CheckWPSManualSessionTimeout session is $wpsSessionStat or $Hostapd_Supplicant is not alive"
#		WPSActionStop
#		WPSActionStart
#	fi
#	
#	print2log DBG "mtlk_wps_cmd: End CheckWPSManualSessionTimeout"
#}

#Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
#CheckWPSSessionInProgress()
#{
#	print2log DBG "mtlk_wps_cmd: Start CheckWPSSessionInProgress"
#	
#	wpsSessionStat=`getWPSSessionStat`
#	if [ $wpsSessionStat = 6 ]
#	then
#		# TODO: Only exit when in Registering state? What about other WPS in progress states?
#		# How should PBC behave when pressed twice?
#		exit
#	fi
#	
#	print2log DBG "mtlk_wps_cmd: End CheckWPSSessionInProgress"
#}

# This function was used by the UI in VB. Will be used in the future by UGW GUI
#get_current_uptime()
#{
#	print2log DBG "mtlk_wps_cmd: Start get_current_uptime"
#	
#	echo `awk -F "." '{print $1}' /proc/uptime`
#	
#	print2log DBG "mtlk_wps_cmd: End get_current_uptime"
#}

#This function was used by the UI in VB. Will be used in the future by UGW GUI
#resetStartTime()
#{
#	print2log DBG "mtlk_wps_cmd: Start resetStartTime"
#	
#	rm $WPS_START_TIME_STATFILE
#	CURRENT_TIME=`get_current_uptime`
#	echo "WPS_StartTime = $CURRENT_TIME" > $WPS_START_TIME_STATFILE
#	
#	print2log DBG "mtlk_wps_cmd: End resetStartTime"
#}

set_gpio()
{
	print2log DBG "mtlk_wps_cmd: Start set_gpio"
	
	led_gpio=`host_api get $$ hw_$wlan $1`
	value=$2
	
	#In the case where there is no LED, do nothing
	if [ "$led_gpio" != "null" ]
	then
		echo $value > $led_gpio
		print2log DBG "mtlk_wps_cmd: set_gpio $led_gpio with value $value"
	fi
	
	print2log DBG "mtlk_wps_cmd: End set_gpio"
}

startWPSLedBlink()
{
	# send WPS-SESSION-START event to the WLAN events script
	( . $HOSTAPD_EVENTS_SCRIPT $wlan WPS-SESSION-START )
}

stopWPSLedBlink()
{
	set_gpio WPS_activity_LED $WPS_Idle_Blink
	set_gpio WPS_error_LED $WPS_Idle_Blink
}

# Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
#resetLastWPSCode()
#{
#	print2log DBG "mtlk_wps_cmd: resetLastWPSCode. Deleting $WPS_LAST_CODE_STATFILE"
#	rm $WPS_LAST_CODE_STATFILE
#}

# Since events are not yet supported in UGW, this function is left as a comment as a preparation for supporting drvhlpr events.
#resetCurrentWPSStatus()
#{
#	print2log DBG "mtlk_wps_cmd: resetCurrentWPSStatus. Deleting $WPS_CURRENT_STATUS_STATFILE"
#	rm $WPS_CURRENT_STATUS_STATFILE
#}
	
update_wps_action()
{
	print2log DBG "mtlk_wps_cmd: update_wps_action. WPS action is: $action"
	echo "WPS_action = $action" > $WPS_ACTION_STATFILE
}	

WPSGetConfInit()
{
	print2log DBG "mtlk_wps_cmd: Start WPSGetConfInit"
	
	startWPSLedBlink				
	#CheckWPSManualSessionTimeout
	#CheckWPSSessionInProgress			
	#resetStartTime
	#resetLastWPSCode
	WPSActionStart
	startWPSLedBlink	
	
	print2log DBG "mtlk_wps_cmd: End WPSGetConfInit"
}

#########################################################
# The following functions are the actions available:
# conf_via_pin
#conf_via_pbc
#get_conf_via_pin
#get_conf_via_pbc
#show_settings
#ap_selected_from_list
#save_settings
#########################################################
conf_via_pin()
{
	print2log DBG "mtlk_wps_cmd: Start conf_via_pin"
	
	enrollee_PIN=$1 
	enrollee_type=$2
	enrollee_mac=$3
	# uuid parameter is optional, if not received, see handling below
	enrollee_uuid=$4
	# Using hard-coded timeout of 30 minutes
	wps_pin_timeout=1800
	
	if [ "$network_type" = "$STA" ] 
	then
		# STA registrar is currently not supported.
		echo "STA registrar is currently not supported !!!" > /dev/console
		print2log DBG "mtlk_wps_cmd: End conf_via_pin"
		return
	fi
	
	startWPSLedBlink
	checkAPconfigured
	#resetStartTime
	#resetLastWPSCode
	update_wps_action
	
	# Check if web selection was to accept all incoming PIN connectios with correct PIN. In such case, no MAC and timeout are needed.
	if [ -z "$enrollee_mac" -o "$enrollee_mac" == "FF:FF:FF:FF:FF:FF" ]
	then
		enrollee_uuid=any
		enrollee_mac=""
		wps_pin_timeout=""
	fi
	
	# Check if no uuid was received, need to read from pin_req file or generate new random
	if [ -z "$enrollee_uuid" ]
	then
		# Get the MAC from the file and compare with requesting MAC
		# Get the MACs and uuids from the file
		pin_req_list=`cat $HOSTAPD_PIN_REQ | awk '{print $3"\n"$2}'`
		
		# Go over the list and compare MACs (MACs appear every odd line, corresponding uuid in following even line)
		# Going over the entire list, since STA can change its uuid and newest are added at the end of the file.
		i=0
		need_uuid=$NO
		for line in $pin_req_list
		do
			let "res=i&1"
			if [ $res = 0 ]
			then
				if [ "$enrollee_mac" == "$line" ]
				then
					need_uuid=$YES
				fi
			else
				if [ "$need_uuid" -eq "$YES" ]
				then
					enrollee_uuid=$line
				fi
			fi
			i=`expr $i + 1`
		done
		
		# If MAC wasn't found in list, generate random uuid
		if [ "$need_uuid" -eq "$NO" ]
		then
			enrollee_uuid=`uuidgen`
		fi
	fi
	
	print2log DBG "mtlk_wps_cmd: $HOSTAPD_CLI -i$wlan wps_pin $enrollee_uuid $enrollee_PIN $wps_pin_timeout $enrollee_mac"
	$HOSTAPD_CLI -i$wlan wps_pin $enrollee_uuid $enrollee_PIN $wps_pin_timeout $enrollee_mac

	print2log DBG "mtlk_wps_cmd: End conf_via_pin"		
}

conf_via_pbc()
{
	print2log DBG "mtlk_wps_cmd: Start conf_via_pbc"
	if [ "$network_type" = "$STA" ]
	then
		echo "mtlk_wps_cmd: conf_via_pbc is not supported by STA !!!" > /dev/console
		print2log DBG "mtlk_wps_cmd: End get_conf_via_pbc"
		return
	fi
	
	startWPSLedBlink
	checkAPconfigured

	#resetStartTime
	#resetLastWPSCode
	update_wps_action
	$HOSTAPD_CLI -i $wlan wps_pbc
	print2log DBG "mtlk_wps_cmd: End conf_via_pbc"
}

get_conf_via_pin()
{
	print2log DBG "mtlk_wps_cmd: Start get_conf_via_pin"
	if [ "$network_type" = "$AP" ]
	then
		echo "mtlk_wps_cmd: get_conf_via_pin is not supported by AP !!!" > /dev/console
		print2log DBG "mtlk_wps_cmd: End get_conf_via_pin"
		return
	fi
	manual_AP=""
#	manual_AP=$1
	ap_select="any"
	enrollee_PIN=$1 
	
	WPSGetConfInit	
		
	update_wps_action
	
	if [ $manual_AP ] && [ $manual_AP != "" ]
	then
		ap_select=$manual_AP	
	fi
	
	$WPA_CLI disconnect
	#not using random PIN, also, interface is always wlan0
	$WPA_CLI wps_pin $ap_select $enrollee_PIN
	#call background script to update WEB and configure DB
	$ETC_PATH/wlan_sta_wps_update_profile &
	print2log DBG "mtlk_wps_cmd: End get_conf_via_pin"
}

get_conf_via_pbc()
{
	print2log DBG "mtlk_wps_cmd: Start get_conf_via_pbc"
	if [ "$network_type" = "$AP" ]
	then
		echo "mtlk_wps_cmd: get_conf_via_pbc is not supported by AP !!!" > /dev/console
		print2log DBG "mtlk_wps_cmd: End get_conf_via_pbc"
		return
	fi
	
	WPSGetConfInit	
	update_wps_action
		
	$WPA_CLI disconnect
	#interface is always wlan0
	$WPA_CLI wps_pbc
	#call background script to update WEB and configure DB
	$ETC_PATH/wlan_sta_wps_update_profile &
	print2log DBG "mtlk_wps_cmd: End get_conf_via_pbc"
}

show_settings()
{	
	echo "SSID: `host_api get $$ $apIndex ESSID`"
	echo "Security Mode: `host_api get $$ $apIndex NonProcSecurityMode`"
	echo "PSK: `host_api get $$ $apIndex NonProc_WPA_Personal_PSK`"
}

# This option is valid only for STA. In order for it to work with supplican of hostapd06, we need to modify this function.
# The wpa_cli needs the index of the MAC of the selected AP and not the MAC itself.
#ap_selected_from_list()
#{
#	print2log DBG "mtlk_wps_cmd: Start ap_selected_from_list"
#	if [ "$network_type" = "$AP" ]
#	then
#		echo "mtlk_wps_cmd: ap_selected_from_list is not supported by AP !!!" > /dev/console
#		print2log DBG "mtlk_wps_cmd: End ap_selected_from_list"
#		return
#	fi
#
#	mac=$1
#	#echo WPS_LastErrorCode = 1 > $WPS_LAST_CODE_STATFILE		
#	#resetStartTime
#		
#	$WPA_CLI select_network $mac
#	print2log DBG "mtlk_wps_cmd: End ap_selected_from_list"	
#}

stop()
{
	print2log DBG "mtlk_wps_cmd: Start 'stop'"
		
	isWPSenabled=`host_api get $$ $apIndex NonProc_WPS_ActivateWPS`
	if [ ! $isWPSenabled ] || [ $isWPSenabled = 0 ]
	then
		echo "WPS is already disabled !!!" > /dev/console 
		print2log DBG "mtlk_wps_cmd: End 'stop'"
		return
	fi
	
	host_api set $$ $apIndex NonProc_WPS_ActivateWPS 0
	host_api commit $$
	WPSActionStop

	print2log DBG "mtlk_wps_cmd: End 'stop'"
}

get_param_psk()
{
      echo `grep "$1" $2 | sed 's/^[^=]*=[ ]*//'`
}

save_settings()
{
	#save_settings saves the settings after WPS from the conf file to the rc.conf using host_api
	#In order for save_settings to work, we need drvhlpr to support wps events
	print2log DBG "mtlk_wps_cmd: Start save_settings"
	
	# get current settings.
	security=`host_api get $$ $apIndex NonProcSecurityMode`
	psk=`host_api get $$ $apIndex NonProc_WPA_Personal_PSK`
	
	# if this is an AP, update accordingly
	# Updating AP will occur only on external registrar case, on other cases, settings will not change.
	if [ "$network_type" = "$AP" ]
	then
		# Check hostapd configuration file exists.
		if [ ! -e $HOSTAPD_CONF ]
		then
			print2log ERROR "mtlk_wps_cmd: Aborted. $HOSTAPD_CONF file doesn't exist"
			exit
		fi

		# Get ssid from hostapd.conf and set it to rc.conf
		g_ssid=`get_param ssid $HOSTAPD_CONF`
		print2log DBG "mtlk_wps_cmd: g_ssid=$g_ssid"
		host_api set $$ $apIndex ESSID $g_ssid

		# Get security information from hostapd.conf and set it to rc.conf

		# Set security to OPEN and change if found other security mode
		g_security_mode=$open

		# Check if WEP mode by searching wep_default_key in conf file.
		g_wep_default_key=`get_param wep_default_key $HOSTAPD_CONF`
		if [ "$g_wep_default_key" ]
		then
			g_security_mode=$wep
			
		fi

		# Check if in WPA-personal or WPA-enterparise security mode by searching wpa_key_mgmt in conf file.
		g_wpa_mode=`get_param wpa_key_mgmt $HOSTAPD_CONF`
		if [ "$g_wpa_mode" ]
		then
			if [ "$g_wpa_mode" = "WPA-PSK" ]
			then
				g_security_mode=$WPA_Personal
			else
				g_security_mode=$WPA_Enterprise
			fi
		fi

		# Set the appropriate parameters for the current security mode
		case $g_security_mode in			
			$wep)
				# Set WEP default key
				g_wep_default_key=`get_param wep_default_key $HOSTAPD_CONF`
				print2log DBG "mtlk_wps_cmd: g_wep_default_key=$g_wep_default_key"
				host_api set $$ $apIndex WepTxKeyIdx $g_wep_default_key

				# Get the 4 WEP keys values from conf file
				g_wep_key0=`get_param wep_key0 $HOSTAPD_CONF | sed 's/\"\([^\"]*\)\"/\1/'`
				g_wep_key1=`get_param wep_key1 $HOSTAPD_CONF | sed 's/\"\([^\"]*\)\"/\1/'`
				g_wep_key2=`get_param wep_key2 $HOSTAPD_CONF | sed 's/\"\([^\"]*\)\"/\1/'`
				g_wep_key3=`get_param wep_key3 $HOSTAPD_CONF | sed 's/\"\([^\"]*\)\"/\1/'`

				# Determine and set the length of the key
				wep_key0_length=`echo $g_wep_key0 | wc -L`
				if [ $wep_key0_length = 5 ] || [ $wep_key0_length = 10 ]
				then
					WepKeyLength=64
				fi
				if [ $wep_key0_length = 13 ] || [ $wep_key0_length = 26 ]
				then
					WepKeyLength=128
				fi
				host_api set $$ $apIndex NonProc_WepKeyLength $WepKeyLength

				# host_api distinguishes between HEX and ASCII by checking if 0x prefix exists.
				# If the key length is 5 or 13 chars, the key is ASCII and no prefix is needed, otherwise add 0x prefix.
				if [ $wep_key0_length = 5 ] || [ $wep_key0_length = 13 ]
				then
					key_prefix=""
				else
					key_prefix="0x"
				fi

				host_api set $$ $apIndex WepKeys_DefaultKey0 $key_prefix$g_wep_key0
				host_api set $$ $apIndex WepKeys_DefaultKey1 $key_prefix$g_wep_key1
				host_api set $$ $apIndex WepKeys_DefaultKey2 $key_prefix$g_wep_key2
				host_api set $$ $apIndex WepKeys_DefaultKey3 $key_prefix$g_wep_key3
				
				# WEP in WPS is always OPEN (shared is not allowed with WPS). Setting authentication to OPEN(1).
				host_api set $$ $apIndex Authentication 1
				;;
				
			$WPA_Personal)
				# Set Beacon type (WPA/WPA2/mixed/mixed restricted)
				g_beacon_type=`get_param wpa $HOSTAPD_CONF`
				print2log DBG "mtlk_wps_cmd: g_beacon_type=$g_beacon_type"
				# wpa=3 in hostapd conf file is mixed mode. in UGW, value 4 is mixed mode-restricted.
				if [ $g_beacon_type -eq 3 ]
				then
					g_beacon_type=4
				fi
				host_api set $$ $apIndex NonProc_WPA_Personal_Mode $g_beacon_type

				# Set Ecryption type (TKIP/CCMP/TKIP-CCMP)
				g_encryption_type=`get_param wpa_pairwise $HOSTAPD_CONF`
				print2log DBG "mtlk_wps_cmd: g_encryption_type=$g_encryption_type"

				case $g_encryption_type in
					"TKIP")
						enc=0
						;;
					"CCMP")
						enc=1
						;;
					"TKIP CCMP")
						enc=2
						;;
					"CCMP TKIP")
						enc=2
						;;

				esac
				host_api set $$ $apIndex NonProc_WPA_Personal_Encapsulation $enc

				# Set psk/passphrase
				g_psk=`get_param wpa_passphrase $HOSTAPD_CONF`
				print2log DBG "mtlk_wps_cmd: g_psk=$g_psk (passphrase)"
				if [ ! $g_psk ]
				then
					g_psk=`get_param wpa_psk $HOSTAPD_CONF`
					print2log DBG "mtlk_wps_cmd: g_psk=$g_psk (psk)"
				fi
				host_api set $$ $apIndex NonProc_WPA_Personal_PSK $g_psk
				;;
		esac
		
		# Set the security mode
		host_api set $$ $apIndex NonProcSecurityMode $g_security_mode
		
		# Change AP to be in configured mode
		host_api set $$ $apIndex cfgState $Configured_AP
	fi
 	
	# if this is a STA, update accordingly
	if [ "$network_type" = "$STA" ]
	then
		if [ ! -e $SUPPLICANT_CONF ]
		then
			print2log ERROR "mtlk_wps_cmd: Aborted. $SUPPLICANT_CONF file doesn't exist"
			exit
		fi
		
		#***********************************************************************/
		#Fix for hostapd06:
		#Supplicant keeps all former connections, therefore more than one network may exist.
		#The command (get_param paramX $SUPPLICANT_CONF |  sed 's/\"\([^\"]*\)\"/\1/') returns all occurances of paramX.
		#We keep only the last network (last connection always last) in new file named networks.

		noOfNet=`awk '/^network=/ {print $0}' /tmp/wpa_supplicant0.conf | grep network -c`
		print2log DBG "noOfNet=$noOfNet"

		i=0
		printLines=0

		echo -e "network={" > /tmp/networks
		while read line
		do
			print2log DBG "$line"
			recname=`echo $line | cut -f1`
			if [ "$recname" = "}" ]
			then
				printLines=0
			fi
			if [ $printLines = 1 ]
			then
				print2log DBG "i=$i"
				if [ $i -eq $noOfNet ]
				then
					print2log DBG "i eq noOfNet :-)"
					echo -e "\t$line" >> /tmp/networks
				fi
			fi
			if [ "$recname" = "network={" ]
			then
				printLines=1
				i=`expr $i + 1`
			fi
		done < /tmp/wpa_supplicant0.conf
		echo -e "}" >> /tmp/networks

		g_ssid=`get_param ssid /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
		echo -e "ssid=$g_ssid"
		host_api set $$ $apIndex ESSID $g_ssid
		
		g_psk=`get_param_psk \#psk /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
		echo -e "psk=$g_psk"
		#if psk remark assci name not exist try psk eith no remark:
		if [ ! $g_psk ]
		then
			g_psk=`get_param psk /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
			echo -e "psk=$g_psk"
		fi
		#***********************************************************************/
			
		#Set Open security as default
		g_security=$securityOpen
		
		print2log DBG "psk found = $g_psk"

		if [ $g_psk ]
		then
			g_security=$securityWPAPersonal
			host_api set $$ $apIndex NonProc_WPA_Personal_PSK $g_psk
			
			g_personal=`get_param proto /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
			wpa_personal_mode=1
			if [ "$g_personal" = "RSN" ]
			then
				wpa_personal_mode=2
			fi
			host_api set $$ $apIndex NonProc_WPA_Personal_Mode $wpa_personal_mode

			g_pairwise=`get_param pairwise /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
			wpa_pairwise=0
			if [ "$g_pairwise" = "CCMP" ]
			then
				wpa_pairwise=1
			fi
			if [ "$g_pairwise" = "CCMP TKIP" ]
			then
				wpa_pairwise=2
			fi
			host_api set $$ $apIndex NonProc_WPA_Personal_Encapsulation $wpa_pairwise
		fi
			
		# Since wep key is saved as "WEP_KEY", we need to save the wep_key as WEP_KEY	
		g_wep_key0=`get_param wep_key0 /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
		
		if [ $g_wep_key0 ]
		then
			# Handling WEP:
			g_wep_key1=`get_param wep_key1 /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
			g_wep_key2=`get_param wep_key2 /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
			g_wep_key3=`get_param wep_key3 /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
			#g_wep_index=`get_param wep_tx_keyidx /tmp/networks |  sed 's/\"\([^\"]*\)\"/\1/'`
			
			g_security=$securityWEP
			#In order to determine if it's WEP-64 or WEP-128, we need to check length of the key.
			wep_key0_length=`echo $g_wep_key0 | wc -L`
			if [ $wep_key0_length = 5 ] || [ $wep_key0_length = 10 ]
			then
				WepKeyLength=64
			fi
			if [ $wep_key0_length = 13 ] || [ $wep_key0_length = 26 ]
			then
				WepKeyLength=128
			fi
			host_api set $$ $apIndex NonProc_WepKeyLength $WepKeyLength
			# TODO: check how ascii and how hex are saved
			host_api set $$ $apIndex WepKeys_DefaultKey0 $g_wep_key0
			host_api set $$ $apIndex WepKeys_DefaultKey1 $g_wep_key1
			host_api set $$ $apIndex WepKeys_DefaultKey2 $g_wep_key2
			host_api set $$ $apIndex WepKeys_DefaultKey3 $g_wep_key3
			#wpa_supplicant always use index no. 1 in wps
			$g_wep_index=1
			host_api set $$ $apIndex WepTxKeyIdx $g_wep_index
					
			# The NonProc_Authentication also setting WEP-OPEN that value is 1, 2 is shared, 3 is auto. Only 'open' is supported on WPS.
			host_api set $$ $apIndex NonProc_Authentication 1					
		fi
		host_api set $$ $apIndex NonProcSecurityMode $g_security


		# set NeverConnected in $wlan.conf to 0 so that in reboot, supplicant will be activated 
		NeverConnected=`host_api get $$ $apIndex NeverConnected`

	    if [ $NeverConnected = 1 ]
	    then
	    	host_api set $$ $apIndex NeverConnected 0
	    fi
	fi
	
	host_api commit $$
	
	# remove old supplicant params file
	rm -f $SUPPLICANT_PARAMS_PATH
	
	print2log DBG "mtlk_wps_cmd: End save_settings"
}

case $action in
	conf_via_pin)
		conf_via_pin $3 $4 $5 $6
	;;
	conf_via_pbc)
		conf_via_pbc
	;;
	get_conf_via_pin)
		get_conf_via_pin $3
	;;
	get_conf_via_pbc)
		get_conf_via_pbc
	;;
	show_settings)
		show_settings
	;;
	ap_selected_from_list)
		ap_selected_from_list $3
	;;
	save_settings)
		save_settings
	;;
	*)
		showUsage
	;;
esac
