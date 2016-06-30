#!/bin/sh

if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		 . /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

get_wan_start_flag()
{
	__wanStart="1"

	if [ "A$CONFIG_FEATURE_WAN_AUTO_DETECT" = "A1" ]; then
		__wanMode=$wanMode

		. /etc/init.d/get_wan_mode $wanphy_phymode $wanphy_tc # return value in wanMode
		__g_wanMode=$wanMode

		if [ "$__g_wanMode" = "0" ]; then
			# ADSL-ATM mode
			if [ "$auto_detect_L3" = "1" -o "$auto_detect_L2" = "1" -o "$auto_detect_Vlan_ATM" = "1" ]; then
				__wanStart="0"
			fi
		elif [ "$__g_wanMode" = "1" ]; then
			# MII-0 mode
			if [ "$auto_detect_mii0_L3" = "1" -o "$auto_detect_Vlan_ETH0" = "1" ]; then
				__wanStart="0"
			fi
		elif [ "$__g_wanMode" = "2" ]; then
			# MII-1 mode
			if [ "$auto_detect_mii1_L3" = "1" -o "$auto_detect_Vlan_ETH1" = "1" ]; then
				__wanStart="0"
			fi
		elif [ "$__g_wanMode" = "3" ]; then
			# ADSL-PTM mode
			if [ "$auto_detect_adsl_ptm_L3" = "1" -o "$auto_detect_Vlan_ADSL_PTM" = "1" ]; then
				__wanStart="0"
			fi
		elif [ "$__g_wanMode" = "4" ]; then
			# VDSL-PTM mode
			if [ "$auto_detect_vdsl_ptm_L3" = "1" -o "$auto_detect_Vlan_VDSL_PTM" = "1" ]; then
				__wanStart="0"
			fi
		else
			# LTE, 3G
			__wanStart="1"
		fi

		wanMode=$__wanMode
	fi
}

is_non_dsl_mode()
{
	__non_dsl_mode="0"

	if [ "$wanphy_phymode" != "0" -a "$wanphy_phymode" != "3" ]; then
		__non_dsl_mode="1"
	fi
	non_dsl_mode=$__non_dsl_mode
}

do_wan_config()
{
	. /etc/init.d/get_wan_mode $wanphy_phymode $wanphy_tc # return value in wanMode
if [ 1 -eq 1 ]; then
	if [ "$1" = "gen_start" ]; then
		if [ "$wanMode" != "0" -a "$wanMode" != "3" -a "$wanMode" != "4" -a "$wanMode" != "5" ]; then
			/usr/sbin/ifx_event_util "WAN" "START" &
		fi
	elif [ "$1" = "gen_stop" ]; then
		if [ "$wanMode" != "0" -a "$wanMode" != "3" -a "$wanMode" != "4" -a "$wanMode" != "5" ]; then
			/usr/sbin/ifx_event_util "WAN" "STOP" &
		fi
	elif [ "$1" = "changeover_stop" ]; then
		/usr/sbin/ifx_event_util "WAN" "STOP_ALL"
	fi
fi

if [ 1 -eq 0 ]; then
	. /etc/init.d/get_wan_mode $wanphy_phymode $wanphy_tc # return value in wanMode
	if [ "$1" = "gen_start" ]; then
		#if [ $wanphy_phymode = 0 -o $wanphy_phymode = 3 ]; then
		#	/usr/sbin/ifx_event_util "ADSL_LINK" "UP"
		if [ $wanphy_phymode = 1 -o $wanphy_phymode = 2 ]; then
			/usr/sbin/ifx_event_util "ETH_LINK" "L3_UP"
		elif [ $wanMode = 6 ]; then
			/usr/sbin/ifx_event_util "WWAN_3G_LINK" "L3_UP"
		fi
	elif [ "$1" = "gen_stop" ]; then
		#if [ $wanphy_phymode = 0 -o $wanphy_phymode = 3 ]; then
		#	/usr/sbin/ifx_event_util "ADSL_LINK" "L3_DOWN"
		if [ $wanMode = 1 -o $wanMode = 2 ]; then
			/usr/sbin/ifx_event_util "ETH_LINK" "L3_DOWN"
		elif [ $wanMode = 6 ]; then
			/usr/sbin/ifx_event_util "WWAN_3G_LINK" "L3_DOWN"
		fi
	elif [ "$1" = "changeover_stop" ]; then
		#if [ $wanphy_phymode = 0 -o $wanphy_phymode = 3 ]; then
		#	/usr/sbin/ifx_event_util "ADSL_LINK" "DOWN"
		if [ $wanMode = 1 -o $wanMode = 2 ]; then
			/usr/sbin/ifx_event_util "ETH_LINK" "FULL_DOWN"
		elif [ $wanMode = 6 ]; then
			/usr/sbin/ifx_event_util "WWAN_3G_LINK" "FULL_DOWN"
		fi
	fi

	if [ "$1" = "gen_start" ]; then
		# Handle WAN connections manually only if one of below
		# - auto WAN detect is disabled
		# - auto WAN detect is enabled and has been run already
		# Otherwise don't meddle with WAN stop or start
		if [ "$CONFIG_FEATURE_WAN_AUTO_DETECT" = "1" ]; then
			. /etc/init.d/get_wan_mode $wanphy_phymode $wanphy_tc # return value in wanMode
			wanStart="1"
			get_wan_start_flag
			wanStart=$__wanStart

			if [ "$wanStart" = "1" ]; then
				#echo "Starting wan services"
				is_non_dsl_mode
				if [ "$non_dsl_mode" = "1" ]; then
					. /etc/rc.d/rc.bringup_l2if start
					# non-DSL modes only. DSL WAN gets started from adsl_up script
					/etc/rc.d/rc.bringup_wan start &
				fi
			elif [ "$wanMode" = "1" -o "$wanMode" = "2" ]; then
				/usr/sbin/ifx_event_util "ETH_LINK" "UP"
			fi
		else
			#echo "Starting wan services"
			is_non_dsl_mode
			if [ "$non_dsl_mode" = "1" ]; then
				. /etc/rc.d/rc.bringup_l2if start
				# non-DSL modes only. DSL WAN gets started from adsl_up script
				/etc/rc.d/rc.bringup_wan start &
			fi
		fi
	elif [ "$1" = "gen_stop" ]; then
		# if WAN auto-detect is running (by checking for existence of PID file)
		# then stop WAN auto-detect is sufficient
		# otherwise stop DHCP and Static IP connections manually
		if [ -f /var/run/wan_auto_detect.pid ]; then
			echo "Stopping WAN auto-detect"
			/etc/rc.d/killproc wan_autodetect
		else
			# !!!!!!!!!!!!!!!! dependency on dual WAN
			. /etc/rc.d/rc.bringup_wan stop
			. /etc/rc.d/rc.bringup_l2if stop
			#echo "sleep until all wan services are stopped"
			sleep 1
		fi
	fi
fi
}
