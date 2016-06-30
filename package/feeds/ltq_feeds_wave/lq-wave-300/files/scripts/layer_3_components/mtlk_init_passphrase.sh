#!/bin/sh
return_status=true

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

print2log DBG "mtlk_init_passphrase.sh: args: $*"

command=$1
apIndex=$2

# Get corresponding wlan network interface from mapping file
wlan=`find_wave_if_from_index $apIndex`

start_mtlk_init_passphrase()
{
	print2log DBG "mtlk_init_passphrase.sh: Start"
	# Get current Passphrase
	wpa_psk=`host_api get $$ $apIndex NonProc_WPA_Personal_PSK`
	# If it's not empty, abort
	if [ $wpa_psk ]; then return; fi

	i=0	
	wpa_psk="123456780"
	while [ $i != 30 ]
	do
		#TODO: BUG if wps closed by default wpa_psk return is empty
		get_wpa_psk=`grep "wpa_psk" /tmp/$wlan/hostapd.conf | sed 's#^[^=]*=[ ]*##' 2>/dev/null`
		if [ $get_wpa_psk ]
		then
			wpa_psk=$get_wpa_psk
			break
		fi
		sleep 1
		i=`expr $i + 1`
	done
			
	# writing passphrase into rc.conf
	host_api set $$ $apIndex NonProc_WPA_Personal_PSK $wpa_psk

	#host_api commit $$
	#config_save.sh
	
	print2log DBG "mtlk_init_passphrase.sh: Done Start"
}

stop_mtlk_init_passphrase()
{
	return
}

create_config_mtlk_init_passphrase()
{
	return
}

should_run_mtlk_init_passphrase()
{
	SECURITY_MODE=`host_api get $$ $apIndex NonProcSecurityMode`
	NETWORK_TYPE=`host_api get $$ $apIndex network_type`
	if [ "$NETWORK_TYPE" = "$AP" -o "$NETWORK_TYPE" = "$VAP" ] && [ "$SECURITY_MODE" = "$WPA_Personal" ]
	then
		if [ -e $wave_init_failure ]
		then
			return_status=false
		fi
	else
		print2log DBG "mtlk_init_passphrase.sh: SECURITY_MODE != WPA_PERSONAL"
		return_status=false
	fi
}

case $command in
	start)
		start_mtlk_init_passphrase
	;;
	stop)
		stop_mtlk_init_passphrase
	;;
	create_config)
		create_config_mtlk_init_passphrase
	;;
	should_run)
		should_run_mtlk_init_passphrase
	;;
esac

$return_status