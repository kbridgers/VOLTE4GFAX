#!/bin/sh
return_status=true

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then
	. /tmp/mtlk_init_platform.sh
fi

command=$1

start_mtlk_init_wpa_supplicant()
{
	print2log DBG "mtlk_init_wpa_supplicant.sh: Start"
	# Activate the Supplicant if in STA mode.

	# Source Config file
	. $SUPPLICANT_PARAMS_PATH
	
	WPA_SUPPLICANT_COUNT=`ps | grep "\<wpa_supplicant\>" | grep -vc grep`
    #TODO:fix	if [ "$NeverConnected" = "$YES" ] || [ $WPA_SUPPLICANT_COUNT != 0 ] ; then return; fi
	
	#if the bridge mode is mac_clone, for debug add "-ddt"
	if [ "$BridgeMode" = "$MacCloning" ]
	then
		/tmp/wpa_supplicant -Dwext -bbr0 -iwlan0 -c /tmp/wpa_supplicant0.conf -p maclone=$MacCloningAddr -B > /dev/null 2>/dev/null
	else
		/tmp/wpa_supplicant -Dwext -bbr0 -iwlan0 -c /tmp/wpa_supplicant0.conf -B > /dev/null 2>/dev/null
	fi

	# wpa_cli is used for WPS events
	if [ "$NonProc_WPS_ActivateWPS" = "$YES" ]
	then
		/bin/wpa_cli -iwlan0 -a$HOSTAPD_EVENTS_SCRIPT -B
	fi

	# Send out INIT event in order to initialize WLAN station LEDs
	( . $HOSTAPD_EVENTS_SCRIPT wlan0 WLAN-MODE-INIT sta )

	print2log DBG "mtlk_init_wpa_supplicant.sh: Done Start"
}

reload_mtlk_init_wpa_supplicant()
{
	print2log DBG "mtlk_init_wpa_supplicant.sh: reload"
	WPA_SUPPLICANT_COUNT=`ps | grep "\<wpa_supplicant\>" | grep -vc grep`
	if [ $WPA_SUPPLICANT_COUNT != 0 ]
	then
		# TODO: need timeout !?
		/tmp/wpa_cli disconnect
		killall -HUP wpa_supplicant
		/tmp/wpa_cli reconnect
	else
		start_mtlk_init_wpa_supplicant
	fi
	print2log DBG "mtlk_init_wpa_supplicant.sh: Done reload"
}

stop_mtlk_init_wpa_supplicant()
{
	print2log DBG "mtlk_init_wpa_supplicant.sh: Stop"
	WPA_SUPPLICANT_COUNT=`ps | grep "\<wpa_supplicant\>" | grep -vc grep`
	if [ $WPA_SUPPLICANT_COUNT != 0 ]
	then
		killall wpa_supplicant
		A=`ps | grep "\<wpa_supplicant\>" | grep -v grep | awk '{print $1}'`
		for a in $A ; do kill -9 $a ; done
	fi
	print2log DBG "mtlk_init_wpa_supplicant.sh: Done Stop"
}

create_config_mtlk_init_wpa_supplicant()
{
	# Read all parameters into a temp file to hold all current values.
	host_api get_all $$ wlan0 'wlan_profile|lan_bd_cfg|wlan_sta_cfg|wlan_wep' > $SUPPLICANT_PARAMS_PATH
	# Source supplicant params file
	. $SUPPLICANT_PARAMS_PATH
	
	print2log DBG "mtlk_init_wpa_supplicant.sh: Create config"
	if [ ! -e /tmp/wpa_cli ]; then ln -s $BINDIR/wpa_cli /tmp/wpa_cli; fi
	if [ ! -e /tmp/wpa_passphrase ]; then ln -s $BINDIR/wpa_passphrase /tmp/wpa_passphrase; fi
	if [ ! -e /tmp/wpa_supplicant ]; then ln -s $BINDIR/wpa_supplicant /tmp/wpa_supplicant; fi
	if [ ! -e /tmp/wpa_supplicant0.conf ]; then ln -s $CONFIGS_PATH/wpa_supplicant0.conf   /tmp/wpa_supplicant0.conf; fi

	PSK=$NonProc_WPA_Personal_PSK
	if [ -z $PSK ]; then PSK="WPADummyKey"; fi
	
	echo "ctrl_interface=/var/run/wpa_supplicant" > /tmp/temp_supplicant.conf
	echo "ctrl_interface_group=0" >> /tmp/temp_supplicant.conf

	if [ -z $ESSID ]; then	ESSID="WPADummyEssid"; fi

	PSK_Lenght=`echo $PSK | wc -L`
	if [ "$PSK_Lenght" = 64 ]
	then
		/tmp/wpa_passphrase $ESSID "WPADummyEssid" >> /tmp/temp_supplicant.conf
		#replace by real value to temporary, then nv temporary to current:
		cat /tmp/temp_supplicant.conf | sed "s/psk=.*/psk=$PSK/" > /tmp/tmp_supplicant.conf
		mv /tmp/tmp_supplicant.conf /tmp/temp_supplicant.conf
	else
		/tmp/wpa_passphrase $ESSID $PSK >> /tmp/temp_supplicant.conf
	fi

	cat /tmp/temp_supplicant.conf | sed 's/^}//' > /tmp/tmp_supplicant.conf
	mv /tmp/tmp_supplicant.conf /tmp/temp_supplicant.conf
	
	case $NonProcSecurityMode in
		$open)
			echo -e "\t key_mgmt=NONE" >> /tmp/temp_supplicant.conf
			echo -e "\t auth_alg=OPEN" >> /tmp/temp_supplicant.conf
			;;
		$wep)
			echo -e "\t key_mgmt=NONE" >> /tmp/temp_supplicant.conf	
			#setting wep keyes

			for i in 0 1 2 3
			do
				eval key='$'WepKeys_DefaultKey${i}
				if [ $wepKeyType = 0 ]
				then
					echo "wep_key$i=`echo $key | sed 's/^0x//'`" >> /tmp/temp_supplicant.conf
				else
					echo "wep_key$i=\"$key\"" >> /tmp/temp_supplicant.conf
				fi
			done	

		    #wpa_supplicant always use index no. 1 in wps, no need to update
			#echo -e "\t wep_tx_keyidx=`host_api get $$ wlan0 WepTxKeyIdx`" >> /tmp/temp_supplicant.conf
			echo -e "\t group=WEP40 WEP104" >> /tmp/temp_supplicant.conf
			#setting  Authentication			
			case $NonProc_Authentication in
				1) echo -e "\t auth_alg=OPEN" >> /tmp/temp_supplicant.conf ;;
				2) echo -e "\t auth_alg=SHARED" >> /tmp/temp_supplicant.conf ;;
				3) echo -e "\t auth_alg=OPEN SHARED" >> /tmp/temp_supplicant.conf ;;
				*) echo "supported modes are OPEN=1 SHARED=2 OPEN SHARED=3"
			esac
			;;
		$WPA_Personal)
			
			ignore_config=1
			
			if [ $ignore_config = 0 ]
			then
				# TODO- what about key_mgmt !!!???
				#Note- using the defaul is working.
				# I got a problem when setting the proto field.

				case $NonProc_WPA_Personal_Mode in
					1) echo -e "\t proto=WPA" >> /tmp/temp_supplicant.conf ;;
					2) echo -e "\t proto=RSN" >> /tmp/temp_supplicant.conf ;;
				esac
				case $NonProc_WPA_Personal_Encapsulation in
					0) 
						echo -e "\t pairwise=TKIP" >> /tmp/temp_supplicant.conf
						echo -e "\t group=TKIP" >> /tmp/temp_supplicant.conf					
						;;
					1)
						echo -e "\t pairwise=CCMP" >> /tmp/temp_supplicant.conf
						echo -e "\t group=CCMP" >> /tmp/temp_supplicant.conf					
						;;
					2)
						if [ $NonProc_WPA_Personal_Mode != 3 ]
						then
							echo -e "\t pairwise=CCMP TKIP" >> /tmp/temp_supplicant.conf
							echo -e "\t group=CCMP TKIP" >> /tmp/temp_supplicant.conf
						fi
						;;
				esac
			fi
			;;
		$WPA_Enterprise)
			echo -e "\t key_mgmt=WPA-EAP" >> /tmp/temp_supplicant.conf
			echo -e "\t eap=TTLS PEAP TLS" >> /tmp/temp_supplicant.conf
			echo -e "\t phase2=\"auth=MSCHAPV2 auth=GTC\"" >> /tmp/temp_supplicant.conf
			if [ $NonProc_WPA_Enterprise_Certificate = 1 ]
			then
				certificate_file=$ROOT_PATH/saved_configs/certificate.pem
				if [ ! -e $certificate_file ]; then event_log "WARNING: $certificate_file has not found"; fi
				echo -e "\t client_cert=\"$certificate_file\"" >> /tmp/temp_supplicant.conf
			fi
			echo -e "\t identity=\"$NonProc_WPA_Enterprise_Radius_Username\"" >> /tmp/temp_supplicant.conf
			echo -e "\t password=\"$NonProc_WPA_Enterprise_Radius_Password\"" >> /tmp/temp_supplicant.conf
			echo -e "\t auth_alg=OPEN" >> /tmp/temp_supplicant.conf
			;;			
	esac
	
	echo -e "\t scan_ssid=1" >> /tmp/temp_supplicant.conf
	echo "}" >> /tmp/temp_supplicant.conf
	
	
	#hostapd06:	
	#TODO- support also wps_state=1
	if [ "$NonProc_WPS_ActivateWPS" = "$YES" ]
	then
		# Vendor specific information
		# temporary, in the future would be handled by host_api
		. /etc/sys.conf
		echo -e "\t manufacturer=\"$device_info_manu\"" >> /tmp/temp_supplicant.conf
		echo -e "\t model_name=\"$device_info_modname\"" >> /tmp/temp_supplicant.conf
		echo -e "\t model_number=$device_info_modnum" >> /tmp/temp_supplicant.conf
		echo -e "\t device_name=\"$device_info_friendlyname\"" >> /tmp/temp_supplicant.conf
		echo -e "\t serial_number=$device_info_sernum" >> /tmp/temp_supplicant.conf

		# Use defined UUID, otherwise use a default
		#TBD, use: cat /proc/sys/kernel/random/uuid ?????	
		uuid=$UUID_WPS
		if [ -z "$uuid" ]
		then
			uuid=`uuidgen`
			host_api set $$ sys UUID_WPS $uuid
		fi
		echo -e "\t uuid=$uuid" >> /tmp/temp_supplicant.conf
		echo -e "\t update_config=1" >> /tmp/temp_supplicant.conf
		device_type=1-0050F204-1
		echo -e "\t device_type=$device_type" >> /tmp/temp_supplicant.conf
		os_version=01020300
		echo -e "\t os_version=$os_version" >> /tmp/temp_supplicant.conf
	fi

	mv /tmp/temp_supplicant.conf $CONFIGS_PATH/wpa_supplicant0.conf
		
	if [ $NeverConnected = 1 ]
	then
		host_api set $$ 0 NeverConnected 0
	fi

	#write to flash
	host_api commit $$
	print2log DBG "mtlk_init_wpa_supplicant.sh: Done create config"
}

should_run_mtlk_init_wpa_supplicant()
{
	if [ -e $wave_init_failure ]
	then
		return_status=false
	fi
}

case $command in
	start)
		start_mtlk_init_wpa_supplicant 
	;;
	reload)
		reload_mtlk_init_wpa_supplicant
	;;
	stop)
		stop_mtlk_init_wpa_supplicant
	;;
	create_config)
		create_config_mtlk_init_wpa_supplicant
	;;
	should_run)
		should_run_mtlk_init_wpa_supplicant
	;;
esac

$return_status
