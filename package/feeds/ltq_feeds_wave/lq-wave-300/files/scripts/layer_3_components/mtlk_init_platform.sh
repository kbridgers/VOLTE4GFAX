#!/bin/sh

# This script sets platform specific settings for the UGW platforms.

if [ ! "$CONFIGLOADED" ]; then
	if [ -r /etc/rc.d/config.sh ]; then
		. /etc/rc.d/config.sh 2>/dev/null
		CONFIGLOADED="1"
	fi
fi

export RWFS_PATH=/mnt/jffs2
export ROOT_PATH=/root/mtlk
export ETC_PATH=/etc/rc.d
export IMAGES_PATH=/root/mtlk/images
export DRIVER_PATH=/lib/modules/$(uname -r)/net
export WEB_PATH=/root/mtlk/web
export CONFIGS_PATH=/ramdisk/flash
export BCL_PATH=/root/mtlk/bcl
export BINDIR=/bin
export SUPPLICANT_PARAMS_PATH=/tmp/supplicant_params.sh
export WAVE_MAP_CPEID=/tmp/wave_map_index
export WAVE_MAP_SECTION=wave_map
export WAVE_NEXT_SECTION=wave_next
export STATUS_OPER=/usr/sbin/status_oper

export log_level=1
#log_path="/dev/console"
# Uncomment to turn on timestamp based profiling:
#export WLAN_PROFILING=1

export AP=2
export VAP=3
export STA=0

#STATIC=1
export DHCPC=0

#VIDEO_BRIDGE=0
#LAN_PARTY=1

export BAND_5G=0
export BAND_24G=1
export BOTH_BAND=2


export ENABLE=1
export DISABLE=0
export YES=1
export NO=0

export MacCloning=3
export L2NAT=2
export FourAddresses=1
export ThreeAddresses=0

export open=1
export wep=2
export WPA_Personal=3
export WPA_Enterprise=4

export Enrollee=0
export Wirless_registrar=1
export Configured_AP=2
export Unconfigured_AP=1
export Ap_Proxy_registrar=1

export SSID_CHANGED=1
export SSID_NOT_CHANGED=0

export IF_UP=1
export IF_DOWN=0

export HOSTAPD_EVENTS_SCRIPT=$ETC_PATH/wave_wlan_hostapd_events
export HOSTAPD_PIN_REQ=/var/run/hostapd.pin-req
export WPS_PIN_TEMP_FILE=/tmp/wps_current_pin
export WPS_MAC_TEMP_FILE=/tmp/wps_current_mac
export LED_VARS_FILE=/tmp/wlan_leds_scan_results

export wave_init_failure=/tmp/wave_init_failure

export WAVE_VENDOR_NAME=LANTIQ

# Start the led manager on the dongle
#LEDMAN=0

# Dongle has no pushbutton for WPS
#WPS_PB=1

# Don't use lower memory usage - use default net queues
#LOWER_MEM_USAGE=1


# Don't mangle multicast packets (this is done on STAR to reduce CPU utilization)
#MULTICAST_MANGLING=0

# GPIO pins used for output LEDs
#GPIO_LEDS=0,3
# GPIO pins used for pushbuttons
#GPIO_PUSHBUTTONS=1,13

# TODO: This info is also available from HW.ini so maybe remove here, and use host_api get to read
# 1 = trigger on PBC release
export pbc_wps_trigger=1

print2log()
{
    case $1 in
	INFO)
		#if [ $log_level -ge 2 ]; then logger -t INFO "$2"; fi
		if [ $log_level -ge 2 ]; then echo "$2" > /dev/console ; fi
	;;
	DBG)
		#if [ $log_level -ge 3 ]; then logger -t DBG "$2"; fi
		if [ $log_level -ge 3 ]; then echo "$2" > /dev/console ; fi
	;;
	*)
		#logger -t $1 "$2"
		echo $1 $2 > /dev/console
	;;
    esac
}

get_wps_on() {
# Get WPS on/off state: 0 off, 1 on.
# This is an optimization that reads wps_state (off/unconfigure/configured) from the config file to reduce host_api overhead.
# host_api is only called if the config file doesn't exist yet.
# To save ap index translation overhead, both the index and wlan interface name are passed as args. Ugly but works :-(
# TODO: This optimization is for AP, what about STA? Currently it will fall back to using host_api like before.

	apIndex=$1
	wlan=$2
	wps_on=$NO
	wps_state=`awk -F '=' '/wps_state/ {print $2}' $CONFIGS_PATH/hostapd_$wlan.conf`
	if [ -z $wps_state ]
	then
		wps_on=`host_api get $$ $apIndex NonProc_WPS_ActivateWPS`
	else
		if [ $wps_state != 0 ]
		then
			wps_on=$YES
		fi
	fi
	echo $wps_on
}

timestamp() {
# timestamp function for profiling.
# Results added to:  /tmp/wlanprofiling.log

	if [ -z "$WLAN_PROFILING" ]
	then
		return
	fi
	SECS=`date +%s`
	if [ -n $1 ]
	then
		PREFIX="[$1]"
	fi
	echo ${PREFIX}${SECS} >> /tmp/wlanprofiling.log
}

ascii2hex() {
# Converts ascii to hex

    ascii_X=$1
    ascii_LEN=`echo "${#ascii_X}"`
    i=0
    while [ $i -lt $ascii_LEN ]
    do
        ascii_char=${ascii_X:$i:1}
        printf '\\\\x%02x' "'$ascii_char" | sed 's/00/20/'
        let i=i+1
    done 
}

# Get interface name from index
find_wave_if_from_index()
{
	apIndex=$1
	cpeId=`$STATUS_OPER -f $CONFIGS_PATH/rc.conf GET "wlan_main" "wlmn_${apIndex}_cpeId"`
	wlan=`$STATUS_OPER -f $WAVE_MAP_CPEID GET "$WAVE_MAP_SECTION" "wlmap_$cpeId"`
	echo $wlan
}

# Get index from interface name
find_index_from_wave_if()
{
	wlan=$1
	cpeId=`cat $WAVE_MAP_CPEID | grep wlmap_.*$wlan\" | cut -d "=" -f 1 | cut -d "_" -f2`
	# Safe read from rc.conf
	# Create script to run in order to read from rc.conf
	echo "#!/bin/sh" > /tmp/find_index_for_if
	echo "apIndex=\`cat $CONFIGS_PATH/rc.conf | grep wlmn_.*_cpeId=\\\"${cpeId}\\\" | cut -d "_" -f2\`" >> /tmp/find_index_for_if
	echo 'echo $apIndex' >> /tmp/find_index_for_if
	chmod +x /tmp/find_index_for_if
	# Lock rc.conf and execute script
	apIndex=`/usr/sbin/syscfg_lock $CONFIGS_PATH/rc.conf "/tmp/find_index_for_if"`
	echo $apIndex
}

# Check if the wlan interface is ready (eeprom/calibration file exist and ifconfig is working)
# If the interface is up, return 0, else return 1.
check_if_is_ready()
{
	wlan=$1
	parent_if=`echo $wlan | cut -d "." -f 1`
	# Check if the hw_wlan.ini file exists for this interface, file is created if eeprom/calibration file exist.
	if [ ! -e $CONFIGS_PATH/hw_$parent_if.ini ]
	then
		print2log DBG "mtlk_init_platform.sh: hw_$parent_if.ini is missing"
		echo 1
		return
	fi
	# Check if the wlan interface already exists.
	ifconfig_status=`ifconfig $wlan`
	if [ $? -ne 0 ]
	then
		print2log DBG "mtlk_init_platform.sh: $wlan interface not ready."
		echo 1
		return
	fi
	echo 0
}

MTLK_INIT_PLATFORM="1"
