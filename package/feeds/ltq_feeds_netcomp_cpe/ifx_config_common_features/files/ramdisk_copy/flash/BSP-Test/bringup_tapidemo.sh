#!/bin/sh

TAPI_WANIF=$1

if [ A"$TAPI_WANIF" = "A" ]; then
	echo "Usage: bringup_tapidemo.sh WAN-IF-NAME"
	exit 1
fi

if [ -r /etc/rc.d/model_config.sh ]; then
	. /etc/rc.d/model_config.sh
fi 

if [ -r /etc/rc.conf ]; then
	. /etc/rc.conf
fi

TAPI_DEBUG_LEVEL=$tapiDebugLevel
if [ A"$TAPI_DEBUG_LEVEL" == "A" ]; then
	TAPI_DEBUG_LEVEL=4
fi

TAPI_WANIF_IP=`ifconfig $TAPI_WANIF | grep 'inet addr:' | cut -f2 -d: | cut -f1 -d' '`

TAPI_EXTRA_FLAGS_FXO=
if [ A"$tapiDaaMode" == "A1" ]; then
	TAPI_EXTRA_FLAGS_FXO="-x"
fi

if [ A"$tapiKpi2Udp" == "A1" ]; then
	/usr/sbin/tapidemo -d $TAPI_DEBUG_LEVEL $TAPI_EXTRA_FLAGS_FXO -q -s -i $TAPI_WANIF_IP -l /root/voip_fw/ &
else
	/usr/sbin/tapidemo -d $TAPI_DEBUG_LEVEL $TAPI_EXTRA_FLAGS_FXO -s -i $TAPI_WANIF_IP -l /root/voip_fw/ &
fi

