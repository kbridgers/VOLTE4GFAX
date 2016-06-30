#!/bin/sh

case "$1" in
	start)
		#start
		shift
		/etc/init.d/ltq_wan_changeover_start.sh
		;;
	stop)
		#stop
		shift
		/etc/init.d/ltq_wan_changeover_stop.sh
		;;
	*)
		echo $"Usage $0 {start|stop}"
esac
