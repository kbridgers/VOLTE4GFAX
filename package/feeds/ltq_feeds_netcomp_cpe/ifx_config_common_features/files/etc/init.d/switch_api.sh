#!/bin/sh /etc/rc.common
#START=35
start() {
	/sbin/modprobe drv_switch_api
}
