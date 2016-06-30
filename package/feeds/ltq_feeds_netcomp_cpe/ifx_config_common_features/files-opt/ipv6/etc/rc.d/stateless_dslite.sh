#!/bin/sh
OUTPUT="/tmp/cmd_output${1}"
flush_output()
{
        echo "" > "$OUTPUT"
}
remove_output()
{
        rm -f "$OUTPUT"
}
SED="/bin/sed"

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
STATELESS_DSLITE_RULEFILE="/tmp/stateless_dslite_undo_rules"
add_ipt()
{
	echo "iptables $(echo $* | sed s/-I/-D/ | sed s/-A/-D/)" >> ${STATELESS_DSLITE_RULEFILE}
	iptables $* 2>/dev/null
}
del_ipt()
{
	if [ -f ${STATELESS_DSLITE_RULEFILE} ]; then
		. ${STATELESS_DSLITE_RULEFILE} 2> /dev/null
		> ${STATELESS_DSLITE_RULEFILE}
	fi
}

case $1 in

stop)
	del_ipt
	;;
*)

	dslite_TUNIF="dsltun0"
	eval dslite_portrange='$'wan_portrange
	del_ipt
	add_ipt  -t nat -I POSTROUTING -o $dslite_TUNIF -j MASQUERADE
	add_ipt  -t nat -I POSTROUTING -o $dslite_TUNIF -p udp -j MASQUERADE --to-ports $dslite_portrange
	add_ipt  -t nat -I POSTROUTING -o $dslite_TUNIF -p tcp -j MASQUERADE --to-ports $dslite_portrange
	;;
esac


