#!/bin/sh /etc/rc.common

START=85

if [ ! "$ENVLOADED" ]; then
        if [ -r /etc/rc.conf ]; then
                 . /etc/rc.conf 2> /dev/null
                ENVLOADED="1"
        fi
fi

start() {

if [ $port_wan_binding_status_enable = 1 ]; then
	. /etc/rc.d/ltq_pwb_config.sh add_all
fi

}
