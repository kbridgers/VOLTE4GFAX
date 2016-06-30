#!/bin/sh /etc/rc.common

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

. /etc/rc.d/common_func.sh

#setting anywan status if the macro defined
if [ "A$CONFIG_FEATURE_ANY_WAN_SUPPORT" == "A1" ]; then 
	/usr/sbin/status_oper SET "AnyWan" "status" "0"
fi

START=25

start(){

	/sbin/insmod /lib/modules/*/nf_conntrack_proto_esp.ko

	eval g_wan_phy_mode='$'wanphy_phymode
	eval g_wan_tc_mode='$'wanphy_tc

	### Handle ALGs 
	# Merge from S51algs_bringup.sh
	if [ -z "$CONFIG_FEATURE_IFX_TR69_DEVICE" -o "$CONFIG_FEATURE_IFX_TR69_DEVICE" = "0" ]; then
		if [  "$ipnat_enable" = "1" -a "$CONFIG_FEATURE_ALGS" = "1" ]; then
			/etc/rc.d/init.d/algs start
		fi
	fi

	if [ -z "$CONFIG_FEATURE_IFX_TR69_DEVICE" -o "$CONFIG_FEATURE_IFX_TR69_DEVICE" = "0" ]; then
		/usr/sbin/status_oper SET "wan_con_index" "windex" ""
		
		#804281:<IFTW-fchang>.added
		if [ "$CONFIG_FEATURE_IFX_A4" = "1" ]; then
			insmod amazon_se_ppa_ppe_a4_hal.o
			insmod ifx_ppa_api.o
		fi


		if [ "$CONFIG_FEATURE_DUAL_WAN_SUPPORT" = "1" -a "$dw_failover_state" = "1" ]; then
			/usr/sbin/ifx_event_util "DEFAULT_WAN" "MOD"	
		fi

		do_wan_config gen_start

		/usr/sbin/status_oper SET "http_wan_vcc_select"	"WAN_VCC" ""
		/usr/sbin/status_oper SET "wan_main" "ipoption_wan" "UNKNOWN" "WAN_VCC" ""
	fi

### Setup the default DNS Servers during bootup ###
# Merge from resolv.sh 	
	if [ -f /etc/resolv.conf -a "`cat /etc/resolv.conf 2>/dev/null`" = "" ]; then 
		echo "nameserver 168.95.1.1" > /etc/resolv.conf
	fi

###  Handle Port Triggering Functionality
# Merge from port_trigger.sh
	/etc/rc.d/init.d/port_trigger_init 2>/dev/null

	if [ "$CONFIG_FEATURE_DUAL_WAN_SUPPORT" = "1" -a "$dw_failover_state" = "1" ]; then
		# TODO - Set the primary wan default interface information in the wan mode setting parts
		
		if [ "$wanphy_phymode" != "$dw_pri_wanphy_phymode" ]; then
			/usr/sbin/status_oper -u -f /etc/rc.conf SET "wan_phy_cfg" "wanphy_phymode" $dw_pri_wanphy_phymode \
                              "wanphy_tc" $dw_pri_wanphy_tc "wanphy_mii1ethVlanMode" $dw_pri_wanphy_mii1ethVlanMode \
                              "wanphy_ptmVlanMode" $dw_pri_wanphy_ptmVlanMode "wanphy_tr69_encaprequested" $dw_pri_wanphy_tr69_encaprequested
		fi
			
		echo "starting dual WAN ... "
		. /etc/init.d/dw_daemon.sh boot
	fi

# support scenario : have working dnrd for IPv4 WAN and then ipv6 is enabled , dnsmasq should run with ipv4 resolv.conf . This dnsmasq will be override by ipv6_dns_update

        if [ "$CONFIG_PACKAGE_KMOD_IPV6" = "1"  -a  "$ipv6_status" = "1" ]; then
                . /etc/rc.d/bringup_dnsmasq reconf
        fi
}
