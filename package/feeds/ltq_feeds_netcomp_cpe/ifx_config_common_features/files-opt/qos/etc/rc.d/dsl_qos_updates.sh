# Include model information
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

get_qIfType()
{
	# Define interface types
	# NOTE: These values should match with the values in the enum 
	QOS_INTF_WAN_ATM=9
	QOS_INTF_WAN_PTM=10

	case "$wanphy_phymode" in
		0)
			case "$wanphy_tc" in
				0)
					#wan mode is ATM
					qIfTypeActive=$QOS_INTF_WAN_ATM;
					;;
				1)
					#wan mode is PTM
					qIfTypeActive=$QOS_INTF_WAN_PTM;
					;;
			esac
			;;
		3)
			#wan mode is PTM
			qIfTypeActive=$QOS_INTF_WAN_PTM;
			;;
	esac
}

case "$1" in
	INTERFACE_STATUS_UP)
		if [ "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" != "1" ]; then
			DSL_US_DATARATE="$2"
			DSL_DS_DATARATE="$3"
			UPSTREAM_RATE=$(( $DSL_US_DATARATE / 1000 ))
			DOWNSTREAM_RATE=$(( $DSL_DS_DATARATE/ 1000 ))
		else
			line_0_us_rate=$((`/usr/sbin/status_oper GET "xDSL_Bonding" "US_0"` / 1000 ))
			line_0_ds_rate=$(( `/usr/sbin/status_oper GET "xDSL_Bonding" "DS_0"` / 1000 ))
			line_1_us_rate=$(( `/usr/sbin/status_oper GET "xDSL_Bonding" "US_1"` / 1000 ))
			line_1_ds_rate=$(( `/usr/sbin/status_oper GET "xDSL_Bonding" "DS_1"` / 1000 ))
			UPSTREAM_RATE=`expr $line_0_us_rate + $line_1_us_rate`
			DOWNSTREAM_RATE=`expr $line_0_ds_rate + $line_1_us_rate`

			link_rate_update=1
			/usr/sbin/status_oper -u SET "qos_bk" "link_rate_status" $link_rate_update
		fi
		get_qIfType
		if [ $qIfTypeActive -eq $QOS_INTF_WAN_ATM -o  $qIfTypeActive -eq $QOS_INTF_WAN_PTM ]; then
			ds_rate=`/usr/sbin/status_oper GET "qos_bk" "down_link_rate"`
			. /etc/rc.d/ipqos_rate_update $UPSTREAM_RATE $ds_rate 0 &
		fi
	;;
	INTERFACE_STATUS_DOWN)
		if [ "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" = "1" ]; then
			line_0_us_rate=$((`/usr/sbin/status_oper GET "xDSL_Bonding" "US_0"` / 1000 ))
			line_0_ds_rate=$(( `/usr/sbin/status_oper GET "xDSL_Bonding" "DS_0"` / 1000 ))
			line_1_us_rate=$(( `/usr/sbin/status_oper GET "xDSL_Bonding" "US_1"` / 1000 ))
			line_1_ds_rate=$(( `/usr/sbin/status_oper GET "xDSL_Bonding" "DS_1"` / 1000 ))
			UPSTREAM_RATE=`expr $line_0_us_rate + $line_1_us_rate`
			DOWNSTREAM_RATE=`expr $line_0_ds_rate + $line_1_us_rate`

			link_rate_update=`/usr/sbin/status_oper GET "qos_bk" "link_rate_status"`
			if [ $link_rate_update -eq 1 ]; then
				ds_rate=`/usr/sbin/status_oper GET "qos_bk" "down_link_rate"`
				. /etc/rc.d/ipqos_rate_update $UPSTREAM_RATE $ds_rate 0 &
				link_rate_update=0
				/usr/sbin/status_oper -u SET "qos_bk" "link_rate_status" $link_rate_update
			fi
		fi
	;;
	DSL_DATARATE_STATUS)
		DSL_US_DATARATE="$2"
		DSL_DS_DATARATE="$3"
		echo $DSL_DATARATE_US_BC0 > /tmp/dsl_us_rate
		echo $DSL_DATARATE_DS_BC0 > /tmp/dsl_ds_rate
	;;
	DSL_DATARATE_STATUS_US)
		DSL_US_DATARATE="$2"
		DSL_DS_DATARATE="$3"
		# convert the upstream data rate in kbps to cells/sec and store in running config file
		# this will be used for bandwidth allocation during wan connection creation
		# 8 * 53 = 424

		DSL_DATARATE_US_CPS=$(( ${DSL_US_DATARATE} / 424 ))
		/usr/sbin/status_oper SET BW_INFO max_us_bw "${DSL_DATARATE_US_CPS}"
		# Adjust ATM and IP QoS Rate shaping parameters based on line rate
		UPSTREAM_RATE=$(( $DSL_US_DATARATE / 1000 ))
		DOWNSTREAM_RATE=$(( $DSL_DS_DATARATE / 1000 ))
		get_qIfType
		if [ $qIfTypeActive -eq $QOS_INTF_WAN_ATM -o  $qIfTypeActive -eq $QOS_INTF_WAN_PTM ]; then
			ds_rate=`/usr/sbin/status_oper GET "qos_bk" "down_link_rate"`
#			. /etc/rc.d/ipqos_rate_update $UPSTREAM_RATE $DOWNSTREAM_RATE
			. /etc/rc.d/ipqos_rate_update $UPSTREAM_RATE $ds_rate 0 &
		fi
	;;
esac
