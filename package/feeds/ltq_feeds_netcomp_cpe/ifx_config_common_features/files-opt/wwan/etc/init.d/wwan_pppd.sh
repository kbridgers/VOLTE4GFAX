#!/bin/sh

if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		. /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

## PPP Details for WWAN ##########################
PPP_iface_num=60
PPP_serial_dev="/dev/ttyUSB0"
PPP_serial_baud=115200

## STOP Call #####################################
stop ()
{
	ppp_PID=`cat /var/run/ppp-wwan.pid 2>&-|grep -v ppp 2>&-`
	ppp_Plug=`cat /var/run/ppp-wwan-plug.pid 2>&-|grep -v ppp 2>&-`
	if [ -n "$ppp_PID" ]; then
		kill -TERM "$ppp_PID" >&- 2>&-
	fi
	if [ -n "$ppp_Plug" ]; then
		kill -TERM "$ppp_Plug" >&- 2>&-
		rm -f /var/run/ppp-wwan-plug.pid
	fi
}

## START Call ####################################
start ()
{
	if [ -f /tmp/cwan_status.txt ]; then
		cwi=0;
		c_flg=0;
		while [ $cwi -lt $cell_wan_Count ]; do
			eval [ '$'cwan_$cwi'_ena' = "1" ] && {
				c_flg=1;
				break
			}
			cwi=$((cwi+1))
		done

		if [ "$c_flg" = "1" ]; then
			eval c_ENA='$'cwan_$cwi'_ena'
			eval c_NAM='$'cwan_$cwi'_profName'
			eval c_APN='$'cwan_$cwi'_apn'
			eval c_USR='$'cwan_$cwi'_user'
			eval c_PAS='$'cwan_$cwi'_passwd'
			eval c_AUT='$'cwan_$cwi'_authType'
			eval c_PIN='$'cwan_$cwi'_usePIN'
			eval c_COD='$'cwan_$cwi'_PIN'
			eval c_DIA='$'cwan_$cwi'_dialNum'
			eval c_IDL='$'cwan_$cwi'_idleDisc'
			eval c_IDN='$'cwan_$cwi'_idleDiscTO'
			
			if [ -n "$c_IDL" -a "$c_IDL" = 1 ]; then
				local demand_dial=1
			fi
			if [ -n "$c_USR" ]; then
				local set_auth=1
			fi

			stop;
			/usr/sbin/pppd \
				unit $PPP_iface_num linkname wwan ipparam wwan \
				lcp-echo-interval 30 lcp-echo-failure 4 maxfail 0 holdoff 4 \
				noaccomp nopcomp novj nobsdcomp noauth noipdefault \
				local ipcp-accept-local ipcp-accept-remote usepeerdns \
				$([ -n "$demand_dial" ] && echo demand idle $c_IDN || echo persist) \
				$([ -n "$set_auth" ] && echo user "$c_USR" password "$c_PAS") \
				lock crtscts "$PPP_serial_baud" "$PPP_serial_dev" \
				connect "USE_APN="$c_APN" USE_DIALNUM="$c_DIA" USE_TIMEOUT=0 /usr/sbin/chat -e -t5 -v -E -f /etc/chatscripts/3g.chat"
			
			cp -f /var/run/ppp-wwan.pid /var/run/ppp-wwan-plug.pid >&- 2>&-
			if [ -n "$demand_dial" ]; then
				(sleep 2; ping -c 4 `status_oper GET WanPPP0_PPP_Info REMOTE_IP` >&- 2>&-) &
			fi
		fi
	fi
}

[ -n "$1" ] && $1 || echo "Usage: $0 <start/stop>"

