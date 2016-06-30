#!/bin/sh /etc/rc.common
START=02

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

if [ "$CONFIG_FEATURE_PTM_WAN_SUPPORT" = "1" ]; then
	PTM_PARAM="wanphy_ptmVlanMode $dw_pri_wanphy_ptmVlanMode"
fi
if [ "$CONFIG_FEATURE_ETH_WAN_SUPPORT" = "1" ]; then
	ETH_PARAM="wanphy_mii1ethVlanMode $dw_pri_wanphy_mii1ethVlanMode"
fi

start() {
	# while bootup, if the global wan section is updated by secondary wan, 
	# revert the global section to start the wan with primary configuration
	if [ -n "$dw_failover_state" -a "$dw_failover_state" = "1" ]; then
		if [ "$dw_pri_wanphy_phymode" != "$wanphy_phymode" ] && [ "$dw_sec_wanphy_phymode" = "$wanphy_phymode" ]; then
			/usr/sbin/status_oper -u -f /etc/rc.conf SET "wan_phy_cfg" "wanphy_phymode" $dw_pri_wanphy_phymode \
                              "wanphy_tc" $dw_pri_wanphy_tc "wanphy_setphymode" $dw_pri_wanphy_setphymode \
                              "wanphy_settc" $dw_pri_wanphy_settc $ETH_PARAM $PTM_PARAM "wanphy_tr69_encaprequested" $dw_pri_wanphy_tr69_encaprequested
		fi
	fi
}
