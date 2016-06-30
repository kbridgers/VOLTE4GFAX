#!/bin/sh

if [ ! "$ENVLOADED" ]; then
        if [ -r /etc/rc.conf ]; then
                 . /etc/rc.conf 2> /dev/null
                ENVLOADED="1"
        fi
fi

if [ ! "$CONFIGLOADED" ]; then
        if [ -r /etc/rc.d/config.sh ]; then
                . /etc/rc.d/config.sh 2>/dev/null
                CONFIGLOADED="1"
        fi
fi

if [ "$CONFIG_FEATURE_IFX_WIRELESS" = "1" ]; then
	
	#read the interfaces in a loop
	exec < /tmp/ppe_wlan
	while read line
	do
		export iface=${line#\[*\]}
		#ppa register
		if [ "$CONFIG_FEATURE_IFX_WIRELESS_WAVE300" = "1" ]; then
			iwpriv $iface sIpxPpaEnabled 1
		elif [ "$CONFIG_FEATURE_IFX_WIRELESS_ATHEROS" = "1" ]; then
			iwpriv $iface ppa 1
		fi
			ppacmd addlan -i $iface
	done
fi
