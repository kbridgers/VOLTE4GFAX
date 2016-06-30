#!/bin/sh /etc/rc.common

START=999

	if [ ! "$CONFIGLOADED" ]; then
	if [ -r /etc/rc.d/config.sh ]; then
		. /etc/rc.d/config.sh 2>/dev/null
#		. /etc/rc.d/model_config.sh 2>/dev/null
		CONFIGLOADED="1"
	fi
	fi

#        if [ -r /etc/rc.d/model_config.sh ]; then
#                . /etc/rc.d/model_config.sh 2>/dev/null
#        fi

OUTPUT="/tmp/cmd_xxx"
flush_output()
{
	echo "" > "$OUTPUT"
}
remove_output()
{
	rm -f "$OUTPUT"
}
SED="/bin/sed"

wan_type=`echo "$SIP_IF" | sed -n "s/[0-9].*//1p"`
wan_idx=`echo "$SIP_IF" | sed -n "s/WAN.*P//;1p"`

start(){
	if [ "$CONFIG_IFX_MODEL_NAME" = "ARX182_GW_EL_FXS_DECT" ]; then
		if [ -n "$CONFIG_FEATURE_IFX_VOIP" -a "$CONFIG_FEATURE_IFX_VOIP" = "1" ]; then
			flush_output
			#echo $wan_main_index > "$OUTPUT"
			echo $SIP_IF > ""
			# restrict access to board to avoid SPI flash access 
			killall -9 httpd
			killall -9 inetd
			i=0
			if [ "$wan_type" = "WANIP" ]; then
				while [ $i -lt $wan_ip_Count ]; do
					if [ "$i" = "$wan_idx" ]; then
						/etc/rc.d/rc.bringup_voip_start $i "ip"
	                                        break
					fi
					i=`expr $i + 1`
				done
			elif [ "$wan_type" = "WANPPP" ]; then
				while [ $i -lt $wan_ppp_Count ]; do
					if [ "$i" = "$wan_idx" ]; then
						/etc/rc.d/rc.bringup_voip_start $i "ppp"
	                                        break
					fi
					i=`expr $i + 1`
				done
			fi
			/usr/sbin/status_oper SET "dect_fw" "status" "init"
			# delay to wait download complete
			sleep 30
			inetd /etc/inetd.conf
			remove_output
		fi
	fi
}

# Commented out below lines due a hang observed in a reboot call triggered by Devm on CD router reboot test case.
# Since devm trigger a reboot on the same time when this script is being executed, the drop_caches and reboot call
# happens in parallel. So system hangs and this require a hard reboot.
#sync
#echo 3 > /proc/sys/vm/drop_caches

