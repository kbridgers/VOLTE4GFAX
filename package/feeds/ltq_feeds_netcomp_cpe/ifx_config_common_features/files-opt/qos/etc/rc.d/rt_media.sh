#!/bin/sh
#echo "rt_media.sh script invoked with args $1 $2 $3 $4"
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
# if VOIP is not enabled, exit
if [ -z $CONFIG_PACKAGE_IFX_VOIP ]; then
	return 0
fi
if [ $CONFIG_PACKAGE_IFX_VOIP -eq 0 ]; then
	return 0
fi


# if start
if [ $1 = "start" ]; then
	# add rule for upstream direction
	/usr/sbin/iptables -t mangle -I IPQOS_OUTPUT_RTP 1 -p $4 -s $2 --sport $3 -j MARK --set-mark 0xffffff1
	#echo "/usr/sbin/iptables -t mangle -I IPQOS_OUTPUT_RTP 1 -p $4 -s $2 --sport $3 -j MARK --set-mark 0xffffff1"

	# In case of GRX288, add a PCE rule to classify voice traffic to appropriate queues
	nORDER=1
	#echo "adding switch rule in RT Media.sh"
	cmd="switch_cli IFX_FLOW_PCE_RULE_WRITE pattern.nIndex=$nORDER pattern.bEnable=1 pattern.bAppDataMSB_Enable=1 pattern.nAppDataMSB=$3 pattern.eSrcIP_Select=1 pattern.nSrcIP=$2 pattern.nSrcIP_Mask=0xffffff00 pattern.bProtocolEnable=1 pattern.nProtocol=$4 pattern.nProtocolMask=0x0 action.eTrafficClassAction=2 action.nTrafficClassAlternate=0"
	#echo $cmd
	$cmd > /tmp/ipqos_log
	
	# add rule for down stream direction
	/usr/sbin/iptables -t mangle -I IPQOS_PREROUTE_RTP 1 -p $4 -d $2 --dport $3 -j MARK --set-mark 0xffffff3
	#echo "/usr/sbin/iptables -t mangle -I IPQOS_PREROUTE_RTP 1 -p $4 -d $2 --dport $3 -j MARK --set-mark 0xffffff3"
	
fi

# if stop
if [ $1 = "stop" ]; then
	# delete rule for upstream direction
	/usr/sbin/iptables -t mangle -D IPQOS_OUTPUT_RTP -p $4 -s $2 --sport $3 -j MARK --set-mark 0xffffff1
	#echo "/usr/sbin/iptables -t mangle -D IPQOS_OUTPUT_RTP -p $4 -s $2 --sport $3 -j MARK --set-mark 0xffffff1"
	
	switch_cli IFX_FLOW_PCE_RULE_DELETE nIndex=1

	#switch_cli IFX_FLOW_PCE_RULE_DELETE nIndex=2
	# delete rule for down stream direction
	/usr/sbin/iptables -t mangle -D IPQOS_PREROUTE_RTP -p $4 -d $2 --dport $3 -j MARK --set-mark 0xffffff3
	#echo "/usr/sbin/iptables -t mangle -D IPQOS_PREROUTE_RTP -p $4 -d $2 --dport $3 -j MARK --set-mark 0xffffff3"
fi



