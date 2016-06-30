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

# if qos not enabled, exit
if [ $qm_enable -eq 0 ]; then
	#exit 0
	return 0
fi
# if tr69 not enabled, exit
if [ -z $CONFIG_PACKAGE_IFX_DEVM ]; then
	#exit 0
	return 0
fi
if [ $CONFIG_PACKAGE_IFX_DEVM -eq 0 ]; then
	#exit 0
	return 0
fi


# if start
if [ $1 = "start" ]; then
	if [ $2 -eq 0 ]; then

		if [ $3 != "" ] && [ $5 != "" ]; then
			# Upstrean traffic
			/usr/sbin/iptables -t mangle -I IPQOS_OUTPUT_SIP_MGMT 1 -p $5 -s $3 --sport $4 -j MARK --set-mark 0xffffff2

			# Down stream traffic
			/usr/sbin/iptables -t mangle -I IPQOS_PREROUTE_SIP_MGMT 1 -p $5 -d $3 --dport $4 -j MARK --set-mark 0xffffff4
		fi
	fi
	if [ $2 -eq 1 ]; then
		
		if [ $3 != "" ] && [ $5 != "" ]; then
		
			# Upstream traffic
			/usr/sbin/iptables -t mangle -I IPQOS_OUTPUT_SIP_MGMT 1 -p $5 -d $3 --dport $4 -j MARK --set-mark 0xffffff2

			# down stream traffic
			/usr/sbin/iptables -t mangle -I IPQOS_PREROUTE_SIP_MGMT 1 -p $5 -s $3 --sport $4 -j MARK --set-mark 0xffffff4
		fi
	fi
fi

# if stop
if [ $1 = "stop" ]; then
	if [ $2 -eq 0 ]; then
		if [ $3 != "" ] && [ $5 != "" ]; then
			# Upstream traffic
			/usr/sbin/iptables -t mangle -D IPQOS_OUTPUT_SIP_MGMT -p $5 -s $3 --sport $4 -j MARK --set-mark 0xffffff2

			# Downstream traffic
			/usr/sbin/iptables -t mangle -D IPQOS_PREROUTE_SIP_MGMT -p $5 -d $3 --dport $4 -j MARK --set-mark 0xffffff4
		fi
	fi
	if [ $2 -eq 1 ]; then
		if [ $3 != "" ] && [ $5 != "" ]; then
			# Upstream traffic
			/usr/sbin/iptables -t mangle -D IPQOS_OUTPUT_SIP_MGMT  -p $5 -d $3 --dport $4 -j MARK --set-mark 0xffffff2
			# down stream traffic
			/usr/sbin/iptables -t mangle -D IPQOS_PREROUTE_SIP_MGMT -p $5 -s $3 --sport $4 -j MARK --set-mark 0xffffff4
		fi
	fi
fi

