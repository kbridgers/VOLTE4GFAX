#!/bin/sh /etc/rc.common
#START=66
start() {
	        /etc/rc.d/init.d/port_trigger_init 2>/dev/null
}
