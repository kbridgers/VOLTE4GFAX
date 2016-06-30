#!/bin/sh
return_status=true

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then
	. /tmp/mtlk_init_platform.sh
fi

print2log DBG "mtlk_init_hostapd.sh: args: $*"

command=$1
apIndex=$2

timestamp "mtlk_init_hostapd.sh:$command:$apIndex:begin"

# Get corresponding wlan network interface from mapping file
wlan=`find_wave_if_from_index $apIndex`

if_state=$3
if [ "$if_state" = "" ]
then
	if_state=$IF_UP
fi

start_mtlk_init_hostapd()
{
	print2log DBG "mtlk_init_hostapd.sh: Start"
	timestamp "mtlk_init_hostapd.sh:start_func:$apIndex:begin"
	
	hostapd_count=`ps | grep "\<hostapd_$wlan\> " | grep -vc grep`			
	if [ $hostapd_count != 0 ]; then return; fi

	if [ ! -e /tmp/hostapd ]; then ln -s $BINDIR/hostapd /tmp/hostapd; fi
	if [ ! -e /tmp/hostapd.eap_user ]; then ln -s $BINDIR/hostapd.eap_user /tmp/hostapd.eap_user; fi
	if [ ! -e /tmp/hostapd_cli ]; then ln -s $BINDIR/hostapd_cli /tmp/hostapd_cli; fi
	if [ ! -e /tmp/hostapd_$wlan.conf ]; then ln -s $CONFIGS_PATH/hostapd_$wlan.conf /tmp/hostapd_$wlan.conf; fi
	if [ ! -e /tmp/hostapd_$wlan ]; then ln -s $BINDIR/hostapd /tmp/hostapd_$wlan; fi
	/tmp/hostapd_$wlan -d $CONFIGS_PATH/hostapd_$wlan.conf -e /tmp/hostapd_ent_$wlan -B > /dev/null 2>/dev/null

	# Send out INIT event in order to initialize WLAN station LEDs
	( . $HOSTAPD_EVENTS_SCRIPT $wlan WLAN-MODE-INIT ap )
	
	# Currently, hostapd_cli is used only for WPS external registrar events.
	# Optimization: Read WPS state from the config file instead of from host_api to reduce overhead
	WPS_ON=`get_wps_on $apIndex $wlan`
	if [ "$WPS_ON" != "$NO" ]
	then
		hostapd_cli -i$wlan -a$HOSTAPD_EVENTS_SCRIPT -B
	fi
	print2log DBG "mtlk_init_hostapd.sh: Done Start"
	timestamp "mtlk_init_hostapd.sh:start_func:$apIndex:done"
}

stop_mtlk_init_hostapd()
{
	print2log DBG "mtlk_init_hostapd.sh: Stop"
	timestamp "mtlk_init_hostapd.sh:stop_func:$apIndex:begin"
	brutal_kill=$NO
	hostapd_count=`ps | grep "\<hostapd_$wlan\> " | grep -vc grep`
	hostapd_cli_count=`ps | grep "\<hostapd_cli -i$wlan\> " | grep -vc grep`
	if [ $hostapd_cli_count != 0 ]
	then
		hostapd_cli_ps=`ps | grep "\<hostapd_cli -i$wlan\> " | grep -v grep | awk '{print $1}'`
		for ps in $hostapd_cli_ps
		do
			print2log DBG "mtlk_init_hostapd.sh: executing kill on hostapd_cli process $ps"
			kill $ps
		done
	fi
	if [ $hostapd_count != 0 ]
	then
		hostapd_ps=`ps | grep "\<hostapd_$wlan\> " | grep -v grep | awk '{print $1}'`
		for ps in $hostapd_ps
		do
			print2log DBG "mtlk_init_hostapd.sh: executing regular kill on hostapd_$wlan process $ps"
			kill $ps
		done
		# Repeating the loop in case hostapd was not down
		hostapd_ps=`ps | grep "\<hostapd_$wlan\> " | grep -v grep | awk '{print $1}'`
		for ps in $hostapd_ps
		do
			print2log DBG "mtlk_init_hostapd.sh: executing kill -9 on hostapd_$wlan process $ps"
			brutal_kill=$YES
			kill -9 $ps
		done
		if [ "$brutal_kill" = "$YES" ]
		then
			rm -f /var/run/hostapd/$wlan
		fi
	fi
	if [ -e $WPS_PIN_TEMP_FILE ]
	then
		rm $WPS_PIN_TEMP_FILE
	fi
	
	if [ -e $WPS_MAC_TEMP_FILE ]
	then
		rm $WPS_MAC_TEMP_FILE
	fi
	print2log DBG "mtlk_init_hostapd.sh: Done stop"
	timestamp "mtlk_init_hostapd.sh:stop_func:$apIndex:done"
}

reconfigure_mtlk_init_hostapd()
{
	# WARNING: Reconfiguring with -HUP does not work in every case - e.g. WEP cannot be configured with this method (due to hostapd limitation)
	PID=`ps | awk -v wlan=$wlan '{if ($5 == "/tmp/hostapd_"wlan)  print $1}'`
	kill -HUP $PID
	print2log DBG "mtlk_init_hostapd.sh: Done Reconfigure "
}

create_config_mtlk_init_hostapd()
{
	print2log DBG "mtlk_init_hostapd.sh: Create config"
	timestamp "mtlk_init_hostapd.sh:create_config_func:$apIndex:begin"
	hostapd_params=/tmp/hostapd_params.sh
	
	# Read all parameters into a temp file to hold all current values.
	host_api get_all $$ $apIndex 'wlan_main|wlan_phy|wlan_sec|wlan_wep|wlan_psk|wlan_wps|lan_bd_cfg|wlan_1x'> $hostapd_params
	
	# Source the hostapd_params file
	. $hostapd_params
	
	# Write parameters to hostapd configuration file
	echo "logger_syslog_level=3" > /tmp/temp_hostapd.conf
	echo "ctrl_interface=/var/run/hostapd" >> /tmp/temp_hostapd.conf
	echo "ctrl_interface_group=0" >> /tmp/temp_hostapd.conf
	echo "interface=$wlan" >> /tmp/temp_hostapd.conf
	echo "driver=mtlk" >> /tmp/temp_hostapd.conf
	echo "ssid=$(printf "%b" "$ESSID")" >> /tmp/temp_hostapd.conf
	echo "macaddr_acl=0" >> /tmp/temp_hostapd.conf
	
	# in case of dual band, in bridge mode set br0 in both cards. 
	echo "bridge=br0"   >> /tmp/temp_hostapd.conf
		
	# When eap_server=1, we use internal eap server, when eap_server=0, we use Radius server.
	eap_server=1
	
	case $NonProcSecurityMode in
		$open)
			echo "# ------AP is in Plain Text mode------" >> /tmp/temp_hostapd.conf
			;;
		$wep)
			echo "wep_default_key=$WepTxKeyIdx" >> /tmp/temp_hostapd.conf
			
			for i in 0 1 2 3
				do
					eval key='$'WepKeys_DefaultKey${i}
					if [ `echo $key | grep 0x` ]
					then
						echo "wep_key$i=`echo $key | sed 's/^0x//'`" >> /tmp/temp_hostapd.conf
					else
						echo "wep_key$i=\"$(printf "%b" "$key")\"" >> /tmp/temp_hostapd.conf
					fi

				done
			echo "# ------AP is in WEP mode------" >> /tmp/temp_hostapd.conf
			;;
		$WPA_Personal)
			# NonProc_WPA_Peronal_Mode = 4 is Restricted Mixed Mode when not connecting WPA-CCMP and WPA2-TKIP.
			if [ "$NonProc_WPA_Personal_Mode" = "4" ]
			then
				echo "wpa=3" >> /tmp/temp_hostapd.conf
			else
				echo "wpa=$NonProc_WPA_Personal_Mode" >> /tmp/temp_hostapd.conf
			fi

			echo "wpa_key_mgmt=WPA-PSK" >> /tmp/temp_hostapd.conf
			
			psk_length=`printf "%b" "$NonProc_WPA_Personal_PSK" | wc -L`
			if [ $psk_length = 64 ]
			then
				echo "wpa_psk=$NonProc_WPA_Personal_PSK" >> /tmp/temp_hostapd.conf
			else
				echo "wpa_passphrase=$(printf "%b" "$NonProc_WPA_Personal_PSK")" >> /tmp/temp_hostapd.conf
			fi

			echo "wpa_group_rekey=$wpa_group_rekey" >> /tmp/temp_hostapd.conf
			echo "wpa_gmk_rekey=$wpa_group_rekey"  >> /tmp/temp_hostapd.conf

			if [ "$NonProc_WPA_Personal_Mode" != "4" ]
			then
				case $NonProc_WPA_Personal_Encapsulation in
					0) 
						echo "wpa_pairwise=TKIP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 PSK, TKIP------" >> /tmp/temp_hostapd.conf
						;;
					1)
						echo "wpa_pairwise=CCMP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 PSK, CCMP------" >> /tmp/temp_hostapd.conf
						;;
					2)
						echo "wpa_pairwise=TKIP CCMP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 PSK, TKIP CCMP------" >> /tmp/temp_hostapd.conf
						;;
				esac
			else
				echo "wpa_pairwise=TKIP" >> /tmp/temp_hostapd.conf
				echo "rsn_pairwise=CCMP" >> /tmp/temp_hostapd.conf
				echo "# ------AP is in WPA TKIP / WPA2 CCMP------" >> /tmp/temp_hostapd.conf
			fi
		;;

		$WPA_Enterprise)
			# When using Radius server, internal eap_server is off.
			eap_server=0
			echo "ieee8021x=1" >> /tmp/temp_hostapd.conf
			echo "auth_server_addr=$NonProc_WPA_Enterprise_Radius_IP" >> /tmp/temp_hostapd.conf
			echo "auth_server_port=$NonProc_WPA_Enterprise_Radius_Port" >> /tmp/temp_hostapd.conf
			echo "auth_server_shared_secret=$(printf "%b" "$NonProc_WPA_Enterprise_Radius_Key")" >> /tmp/temp_hostapd.conf
			echo "acct_server_addr=$NonProc_WPA_Enterprise_Radius_IP" >> /tmp/temp_hostapd.conf
			echo "acct_server_port=$((NonProc_WPA_Enterprise_Radius_Port+1))" >> /tmp/temp_hostapd.conf
			echo "acct_server_shared_secret=$(printf "%b" "$NonProc_WPA_Enterprise_Radius_Key")" >> /tmp/temp_hostapd.conf
			echo "wpa_key_mgmt=WPA-EAP" >> /tmp/temp_hostapd.conf
			echo "eap_reauth_period=$NonProc_WPA_Enterprise_Radius_ReKey_Interval" >> /tmp/temp_hostapd.conf

			# NonProc_WPA_Enterprise_Mode = 4 is Restricted Mixed Mode when not connecting WPA-CCMP and WPA2-TKIP
			if [ "$NonProc_WPA_Enterprise_Mode" = "4" ]
			then
				echo "wpa=3" >> /tmp/temp_hostapd.conf
			else
				echo "wpa=$NonProc_WPA_Enterprise_Mode" >> /tmp/temp_hostapd.conf
			fi

			if [ "$NonProc_WPA_Enterprise_Mode" != "4" ]
			then
				case $NonProc_WPA_Enterprise_Encapsulation in
					0) 
						echo "wpa_pairwise=TKIP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 Enterprise, TKIP------" >> /tmp/temp_hostapd.conf
						;;
					1)
						echo "wpa_pairwise=CCMP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 Enterprise, CCMP------" >> /tmp/temp_hostapd.conf
						;;
					2)
						echo "wpa_pairwise=TKIP CCMP" >> /tmp/temp_hostapd.conf
						echo "# ------AP is in WPA/WPA2 Enterprise, TKIP CCMP------" >> /tmp/temp_hostapd.conf
						;;
				esac
			else
				echo "wpa_pairwise=TKIP" >> /tmp/temp_hostapd.conf
				echo "rsn_pairwise=CCMP" >> /tmp/temp_hostapd.conf
				echo "# ------AP is in WPA TKIP / WPA2 CCMP  Enterprise ------" >> /tmp/temp_hostapd.conf
			fi
			;;
	esac
	
	# sourcing the file that contains the device info.
	. /etc/sys.conf
	echo "manufacturer=$device_info_manu" >> /tmp/temp_hostapd.conf
	echo "model_name=$device_info_modname" >> /tmp/temp_hostapd.conf
	echo "device_name=$device_info_modname" >> /tmp/temp_hostapd.conf
	echo "model_number=$device_info_modnum" >> /tmp/temp_hostapd.conf
	echo "serial_number=$device_info_sernum" >> /tmp/temp_hostapd.conf

	config_methods="label virtual_display push_button virtual_push_button physical_push_button keypad"
	echo "config_methods=$config_methods" >> /tmp/temp_hostapd.conf

	echo "device_type=6-0050F204-1" >> /tmp/temp_hostapd.conf
	echo "os_version=01020300" >> /tmp/temp_hostapd.conf
	echo "ignore_broadcast_ssid=0" >> /tmp/temp_hostapd.conf

	echo "auth_algs=$Authentication" >> /tmp/temp_hostapd.conf
	echo "eapol_key_index_workaround=0" >> /tmp/temp_hostapd.conf
	
	echo "eap_server=$eap_server" >> /tmp/temp_hostapd.conf

	if [ "$NonProc_WPS_ActivateWPS" = "$YES" ]
	then
		# Chek if AP supports proxy mode (act as a proxy for enrollee to be configured via external registrar).
		# If no value returnd, set AP to enable.
		if [ -z "$WPS_external_registrar" ]
		then
			WPS_external_registrar=$YES
		fi
		if [ "$WPS_external_registrar" = "$YES" ]
		then
			echo "friendly_name=$device_info_friendlyname" >> /tmp/temp_hostapd.conf
			echo "manufacturer_url=$device_info_tr64url" >> /tmp/temp_hostapd.conf
			echo "upnp_iface=br0" >> /tmp/temp_hostapd.conf
			echo "pbc_in_m1=1" >> /tmp/temp_hostapd.conf
		fi
		# Check if AP supports enrollee mode (can be configured via external registrar). If no value returnd, set AP to enable.
		if [ -z "$enrolleeEna" ]
		then
			enrolleeEna=$YES
		fi
		if [ "$enrolleeEna" = "$YES" ]
		then
			echo "ap_setup_locked=0" >> /tmp/temp_hostapd.conf
			if [ -z "$WPS_PIN" ]
			then
				WPS_PIN=12345670
				# writing PIN code to rc.conf
				host_api set $$ $apIndex WPS_PIN $WPS_PIN
				host_api commit $$
				print2log INFO1 "mtlk_init_hostapd.sh: updated WPS-PIN to $WPS_PIN"
			fi
			echo "ap_pin=$WPS_PIN" >> /tmp/temp_hostapd.conf
		else
			echo "ap_setup_locked=1" >> /tmp/temp_hostapd.conf
		fi
		# Check if AP is in configured or un-configured mode. If no value returned, set AP to be configured.
		if [ -z "$cfgState" ]
		then
			cfgState=$Configured_AP
		fi
		if [ "$cfgState" = "$Unconfigured_AP" ]
		then
			echo "wps_state=1" >> /tmp/temp_hostapd.conf
		else
			echo "wps_state=2" >> /tmp/temp_hostapd.conf
		fi
		echo "wps_cred_processing=2" >> /tmp/temp_hostapd.conf
		# Use defined UUID, otherwise use a random one
		# Use: cat /proc/sys/kernel/random/uuid if uuidgen doesn't work
		uuid=`host_api get $$ sys UUID_WPS`
		if [ -z "$uuid" ]
		then
			uuid=`uuidgen`
			host_api set $$ sys UUID_WPS $uuid
		fi
		echo "uuid=$uuid" >> /tmp/temp_hostapd.conf
		echo "wps_pin_requests=$HOSTAPD_PIN_REQ" >> /tmp/temp_hostapd.conf
		if [ "$FrequencyBand" = "$BAND_5G" ]
		then
			echo "wps_rf_bands=a" >> /tmp/temp_hostapd.conf
		else
			echo "wps_rf_bands=g" >> /tmp/temp_hostapd.conf
		fi		
	else
		echo "wps_state=0" >> /tmp/temp_hostapd.conf
	fi

	mv /tmp/temp_hostapd.conf $CONFIGS_PATH/hostapd_$wlan.conf

	#write to flash
	#host_api commit $$
	print2log DBG "mtlk_init_hostapd.sh: Done create config"
	timestamp "mtlk_init_hostapd.sh:create_config_func:$apIndex:done"
}

should_run_mtlk_init_hostapd()
{
	if [ -e $wave_init_failure ]
	then
		return_status=false
	fi
}

case $command in
	start)
		start_mtlk_init_hostapd 
	;;
	reconfigure)
		stop_mtlk_init_hostapd
		create_config_mtlk_init_hostapd
		# Optimization: Allow skipping the start, if this will be done later by a different script. (Default: start)
		if [ "$if_state" = "$IF_UP" ]
		then
			start_mtlk_init_hostapd
		fi

		# TODO: reconfiguring existing hostapd (HUP) is faster than stop+start,
		# but it doesn't work consistently. e.g. WEP isn't set by hostapd reconfigure
		# Need to revisit (fix HUP), and then replace stop+start with:
		#reconfigure_mtlk_init_hostapd
	;;
	reload)
		# Reload existing hostapd configuration without recreating config file
		# This is used for reconfiguring a downed interface, when security settings weren't changed
		#reconfigure_mtlk_init_hostapd
		stop_mtlk_init_hostapd
		if [ "$if_state" = "$IF_UP" ]
		then
			timestamp "mtlk_init_hostapd.sh:ifconfig_up:$apIndex:begin"
			#TODO: NEEDED ifconfig up?:
			ifconfig $wlan up
			timestamp "mtlk_init_hostapd.sh:ifconfig_up:$apIndex:done"
			start_mtlk_init_hostapd
		fi
	;;
	stop)
		stop_mtlk_init_hostapd
	;;
	create_config)
		create_config_mtlk_init_hostapd
	;;
	should_run)
		should_run_mtlk_init_hostapd
	;;
esac

timestamp "mtlk_init_hostapd.sh:$command:$apIndex:done"
$return_status
