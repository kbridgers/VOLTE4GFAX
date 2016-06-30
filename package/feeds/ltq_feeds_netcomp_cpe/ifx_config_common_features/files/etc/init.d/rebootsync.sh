#!/bin/sh /etc/rc.common

STOP=02
if [ ! "$CONFIGLOADED" ]; then
	if [ -r /etc/rc.d/config.sh ]; then
		sync
		. /etc/rc.d/config.sh 2>/dev/null
		CONFIGLOADED="1"
	fi
fi

stop ()
{
		if [ "$CONFIG_PACKAGE_KMOD_LTQCPE_GW188_SUPPORT" = "1" ]; then
			/sbin/reboot -f
		fi
		sync
        iRe=3; while [ $iRe -gt 0 ]; do
		iRe=`expr $iRe - 1`;
		echo -en "\rReboot in progress... $iRe";
		sleep 1;
	done; echo
}

