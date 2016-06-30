#!/bin/sh
return_status=true

#Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then
	. /tmp/mtlk_init_platform.sh
fi
command=$1

start_mtlk_init_country()
{
	print2log DBG "mtlk_init_country.sh: Start"

	wlan_count=`host_api get $$ sys wlan_count`

	NETWORK_TYPE=`host_api get $$ wlan0 network_type`
	COUNTRY_CODE=`iwpriv wlan0 gEEPROM | sed -n '/EEPROM country:/{s/EEPROM country:.*\([A-Z?][A-Z?]\)/\1/p}'`
	
	i=0
	while [ "$i" -lt "$wlan_count" ]
	do
		wlan=wlan$i

		if [ "$NETWORK_TYPE" = "$STA" ]
		then
			ACTIVE_11D=`host_api get $$ $wlan dot11dActive`
			if [ "$ACTIVE_11D" = "0" ]
			then
				return
			fi
		fi
		if [ -n "$COUNTRY_CODE" ] && [ "$COUNTRY_CODE" != "??" ]
		then
			host_api set $$ $wlan EEPROMCountryValid $YES
			host_api set $$ $wlan Country $COUNTRY_CODE
		else
			host_api set $$ $wlan EEPROMCountryValid $NO
		fi
		let i=$i+1
	done
	# No need to commit, since values are evaluated after each reboot.
	print2log DBG "mtlk_init_country.sh: Done"
}

stop_mtlk_init_country()
{
	return
}

create_config_mtlk_init_country()
{
	return
}

should_run_mtlk_init_country()
{
	if [ -e $wave_init_failure ]
	then
		return_status=false
	fi
}

case $command in
	start)
		start_mtlk_init_country
	;;
	stop)
		stop_mtlk_init_country
	;;
	create_config)
		create_config_mtlk_init_country
	;;
	should_run)
		should_run_mtlk_init_country
	;;
esac

$return_status