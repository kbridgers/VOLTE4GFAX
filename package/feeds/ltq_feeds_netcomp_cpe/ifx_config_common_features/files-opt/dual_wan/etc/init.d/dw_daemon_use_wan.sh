#!/bin/sh
if [ ! "$ENVLOADED" ]; then
  if [ -r /etc/rc.conf ]; then
     . /etc/rc.conf 2> /dev/null
    ENVLOADED="1"
  fi
fi

#reset_lte_modem ()
#{
#  ACTION="RESET" /etc/hotplug.d/usb/ltq-wwan-usb-lte.hotplug
#}

#echo "Daemon Use WAN - mode $1 (1:Pri, 2:Sec, 3:Both)"

# $2, $3 - Gives current wan phy_mode and tc
probe_mode=$1

# $1 gives the wan mode to be used
if [ "$probe_mode" = "1" ]; then #update primary wan information
	# wan information to be updated in rc.conf
	phy_mode="$dw_pri_wanphy_phymode"
	tc_mode="$dw_pri_wanphy_tc"
	ethVlan_mode="$dw_pri_wanphy_mii1ethVlanMode"
	ptmVlan_mode="$dw_pri_wanphy_ptmVlanMode"
	encap_req="$dw_pri_wanphy_tr69_encaprequested"
elif [ "$probe_mode" = "2" ]; then #update sec wan information
	# wan information to be updated in rc.conf
	phy_mode="$dw_sec_wanphy_phymode"
	tc_mode="$dw_sec_wanphy_tc"
	ethVlan_mode="$dw_sec_wanphy_mii1ethVlanMode"
	ptmVlan_mode="$dw_sec_wanphy_ptmVlanMode"
	encap_req="$dw_sec_wanphy_tr69_encaprequested"
fi

#if [ "$dw_pri_wanphy_phymode" = "5" -o "$dw_sec_wanphy_phymode" = "5" -o "$dw_pri_wanphy_phymode" = "6" -o "$dw_sec_wanphy_phymode" = "6" ]; then
if [ "$dw_pri_wanphy_phymode" = "5" -o "$dw_sec_wanphy_phymode" = "5" ]; then
	echo "combination is wired_wireless"
	wwan=1
fi
if [ "$CONFIG_FEATURE_PTM_WAN_SUPPORT" = "1" ]; then
	PTM_PARAM="wanphy_ptmVlanMode $ptmVlan_mode"
fi
if [ "$CONFIG_FEATURE_ETH_WAN_SUPPORT" = "1" ]; then
	ETH_PARAM="wanphy_mii1ethVlanMode $ethVlan_mode"
fi

#echo "Supported Standby Type - $dw_standby_type (1:CSB, 2:HSB)"
if [ "$dw_standby_type" = "1" ]; then 	# FO_TYPE = CSB
	# Check if wan mode is getting updated from what is existing current wan
	# stop the existing wan
	if [ "$wwan" = "1" ]; then
		# stop the wan connections only
		. /etc/init.d/dw_daemon.sh stop_wan
		if [ $qm_enable -eq 1 ]; then
			#echo " DISABLING IPQOS "
			/etc/rc.d/ipqos_disable
		fi
		killall qos_rate_update
	else
		/etc/init.d/ltq_wan_changeover_stop.sh
	fi
	. /etc/init.d/get_wan_mode $wanphy_phymode $wanphy_tc
	eval defWanIf='$'default_wan_${wanMode}_conn_iface
	/sbin/ifconfig ${defWanIf} down

	# Update the rc.conf with the new wan mode to be used
	/usr/sbin/status_oper -u -f /etc/rc.conf SET "wan_phy_cfg" "wanphy_phymode" $phy_mode \
                              "wanphy_tc" $tc_mode "wanphy_setphymode" $phy_mode \
                              "wanphy_settc" $tc_mode $ETH_PARAM $PTM_PARAM "wanphy_tr69_encaprequested" $encap_req

	if [ "$wwan" = "1" ]; then
		# start the wan connections only
		/etc/rc.d/rc.bringup_l2if start
		#echo "Starting wan services"
		/etc/rc.d/rc.bringup_wan start
		/etc/init.d/init_ipqos.sh start
		/usr/sbin/ifx_event_util "DEFAULT_WAN" "MOD"
	else
		/etc/init.d/ltq_wan_changeover_start.sh
    # The reset of LTE dongle is required for following reason.
    # When moved from primary to secondary, LTE dongle is not responding. So need to 
    # reset the dongle(reason - dongle is up and got IP from service provider, since
    # udhcpc is not started on LTE interface, so dongle does not respond to any 
    # command other than reset. To start dhcpc,needs fix in evetn_util)
    # NOTE: THIS IS TEMPORARY FIX
    #
    # if [ $phy_mode -eq 6 ] && [ -f "/sys/class/net/lte0/operstate" ] ; then
    #  reset_lte_modem
    # fi
	fi

	# start WANp if WANx($1) is secondary
	# for DSL modes, it will be taken care in the adsl_up script
	if [ "$probe_mode" = "2" ] && [ "$dw_pri_wanphy_setphymode" != "0" -a "$dw_pri_wanphy_setphymode" != "3" ]; then
		#echo "starting default wan conxn on Wan Primary"

		# Start the Default wan of WANp.
		. /etc/init.d/dw_daemon.sh start_def_wan $dw_pri_wanphy_setphymode $dw_pri_wanphy_settc
	fi
elif [ "$dw_standby_type" = "2" ]; then		#FO_TYPE = HSB
	# if 3G WAN is selected, dont call stop, as it is the only wan
	# else stop all wan conections other than default wan conxn
	if [ "$wanphy_phymode" = "5" ]; then
		#echo "Only default interface on 3G wan is active not stoppping"
		defWanIf="ppp60"
	else
		# echo " Stop the Prev WAN except Default"
		# Stop the WANprev except default
		# stop all the prev_wanphy_phymode connections except default 
# Not required now, as this is taken care in the adsl_up script
#		if [ "$wanphy_phymode" = "0" -o "$wanphy_phymode" = "3" ]; then
#			if [ -n "`/bin/cat /tmp/adsl_status | grep "0"`" ]; then
#				/etc/init.d/dw_daemon.sh start_def_wan $wanphy_phymode $wanphy_tc
#			fi
#		fi
		/etc/rc.d/rc.bringup_wan stop_except_default
		. /etc/init.d/get_wan_mode $wanphy_phymode $wanphy_tc
		eval defWanIf='$'default_wan_${wanMode}_conn_iface
	fi

	#update the rc.conf with the new wan mode information
	/usr/sbin/status_oper -u -f /etc/rc.conf SET "wan_phy_cfg" "wanphy_phymode" $phy_mode \
                              "wanphy_tc" $tc_mode "wanphy_setphymode" $phy_mode \
                              "wanphy_settc" $tc_mode $ETH_PARAM $PTM_PARAM "wanphy_tr69_encaprequested" $encap_req
	# Start WANx 
	# Default WAN is already running. 
	# So, start all the wan connections in wan_mode (wanphy_phymode) except default
	/usr/sbin/ifx_event_util "DEFAULT_WAN" "MOD"
	/sbin/ifconfig ${defWanIf} down
#	if [ "$phy_mode" = "5" ]; then	# 3G WAN
		#echo "Only default interface on 3G wan is active so not restarting"
		# add default gw for 3G WAN. TODO - This needs to be removed based on updates in DNRD
		# route add default dev ppp60
		/etc/init.d/dns_relay restart
#	else
		# echo "Starting all wans except default on wan phy mode - $wanphy_phymode"
		/etc/rc.d/rc.bringup_wan start_except_default
#	fi
	/sbin/ifconfig ${defWanIf} up
fi

