#!/bin/sh /etc/rc.common

STOP=01
if [ ! "$CONFIGLOADED" ]; then
	if [ -r /etc/rc.d/config.sh ]; then
		sync
		. /etc/rc.d/config.sh 2>/dev/null
		CONFIGLOADED="1"
	fi
fi

stop ()
{

	touch /tmp/upgrade_chk.txt
	sync;
	if [ "$CONFIG_IFX_MODEL_NAME" = "ARX382_GW_EL_ADSL" ] ; then
	sleep 3;
	fi

	while : ; do
		grep -q "(upgrade)" /proc/*/stat && {
			sleep 3;
			echo -en "\n ####################################\n"
			echo -en "\n Hold until upgrade process completes\n"
			echo -en "\n ####################################\n"
		} || break
	done
	if [ "$CONFIG_IFX_MODEL_NAME" = "ARX382_GW_EL_ADSL" -a -f "/tmp/devm_reboot_chk.txt" ] ; then
		sleep 20;
	fi
	sync;
}

