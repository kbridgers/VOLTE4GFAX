#!/bin/sh
#echo "sip.sh script invoked with args $1 $2 $3 $4"
if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		. /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

if [ ! "$CONFIGLOADED" ]; then
	if [ -r /etc/rc.d/config.sh ]; then
		. /etc/rc.d/config.sh 2> /dev/null
		CONFIGLOADED="1"
	fi
fi

# if qos not enabled, exit
if [ $qm_enable -eq 0 ]; then
	return 0
fi


# if VOIP not enabled, exit
if [ -z $CONFIG_PACKAGE_IFX_VOIP ]; then
	return 0
fi
if [ $CONFIG_PACKAGE_IFX_VOIP -eq 0 ]; then
	return 0
fi
		


# if start
if [ $1 = "start" ]; then
	# add rule for upstream direction
	/usr/sbin/iptables -t mangle -I IPQOS_OUTPUT_SIP_MGMT 1 -p $4 -s $2 --sport $3 -j MARK --set-mark 0xffffff2
	#echo "/usr/sbin/iptables -t mangle -I IPQOS_OUTPUT_SIP_MGMT 1 -p $4 -s $2 --sport $3 -j MARK --set-mark 0xffffff2"
	# Add rule for downstream direction
	/usr/sbin/iptables -t mangle -I IPQOS_PREROUTE_SIP_MGMT 1 -p $4 -d $2 --dport $3 -j MARK --set-mark 0xffffff4
	#echo "/usr/sbin/iptables -t mangle -I IPQOS_PREROUTE_SIP_MGMT 1 -p $4 -d $2 --dport $3 -j MARK --set-mark 0xffffff4"

	# In case of GRX288, add a PCE rule to classify traffic from SIP MGMT port to appropriate queues
	nORDER_US=3
	#echo "adding a PCE rule in sip.sh"
	cmd="switch_cli IFX_FLOW_PCE_RULE_WRITE pattern.nIndex=$nORDER_US pattern.bEnable=1 pattern.bAppDataMSB_Enable=1 pattern.nAppDataMSB=$3 pattern.eSrcIP_Select=1 pattern.nSrcIP=$2 pattern.nSrcIP_Mask=0xffffff00 pattern.bProtocolEnable=1 pattern.nProtocol=$4 action.eTrafficClassAction=2 action.nTrafficClassAlternate=1"
#	echo $cmd
	$cmd > /tmp/ipqos_log
	

fi

# if stop
if [ $1 = "stop" ]; then
	# Delete rule for upstream direction
	/usr/sbin/iptables -t mangle -D IPQOS_OUTPUT_SIP_MGMT -p $4 -s $2 --sport $3 -j MARK --set-mark 0xffffff2
	#echo "/usr/sbin/iptables -t mangle -D IPQOS_OUTPUT_SIP_MGMT -p $4 -s $2 --sport $3 -j MARK --set-mark 0xffffff2"
	# Delete rule for downstream direction
	/usr/sbin/iptables -t mangle -D IPQOS_PREROUTE_SIP_MGMT -p $4 -d $2 --dport $3 -j MARK --set-mark 0xffffff4
	#echo "/usr/sbin/iptables -t mangle -D IPQOS_PREROUTE_SIP_MGMT -p $4 -d $2 --dport $3 -j MARK --set-mark 0xffffff4"

	# In case of GRX288, add a PCE rule to classify traffic from SIP MGMT port to appropriate queues
	nORDER_US=3
	switch_cli IFX_FLOW_PCE_RULE_DELETE nIndex=$nORDER_US

fi


