#!/bin/sh /etc/rc.common
#
# Install led driver and make led device node

START=72


start() {
		LEDUpdater &	
	}

