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
	#save the interfaces
	if [ "$CONFIG_FEATURE_IFX_WIRELESS_WAVE300" = "1" ]; then
		ppacmd getlan | grep wlan | cut -b 16-20 > /tmp/ppe_wlan
	elif [ "$CONFIG_FEATURE_IFX_WIRELESS_ATHEROS" = "1" ]; then
		ppacmd getlan | grep ath | cut -b 16-20 > /tmp/ppe_wlan
	fi
	
	#in a loop
	exec < /tmp/ppe_wlan
	while read line
	do
		export iface=${line#\[*\]}
		#echo $iface
		#ppa unregister
		ppacmd dellan -i $iface
		if [ "$CONFIG_FEATURE_IFX_WIRELESS_WAVE300" = "1" ]; then
			iwpriv $iface sIpxPpaEnabled 0
		elif [ "$CONFIG_FEATURE_IFX_WIRELESS_ATHEROS" = "1" ]; then
			iwpriv $iface ppa 0
		fi
	done
fi
