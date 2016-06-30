#!/bin/sh /etc/rc.common
START=56

start() {
	# start IPQoS
	if [ "$CONFIG_FEATURE_IFX_IPQOS" = "1" ]; then
		. /etc/rc.d/ipqos_preinit
		if [ $qm_enable -eq 1 ]; then
			/etc/rc.d/ipqos_init &
		fi
		if [ $qm_AccelMngr -eq 1 ]; then
			/etc/rc.d/class_acceleration_manager &
		fi
		
	fi

 }
