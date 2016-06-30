#!/bin/sh

wan_mode=$1
wan_iface=$2

if [ "$CONFIG_FEATURE_LED" = "1" ]; then
	if [ "$wan_mode" = "1" -o "$wan_mode" = "2" ]; then
        	echo netdev > /sys/class/leds/internet_led/trigger
	        echo "$wan_iface" > /sys/class/leds/internet_led/device_name
        	echo "link tx rx" > /sys/class/leds/internet_led/mode
	        echo 1 > /sys/class/leds/internet_led/brightness
	        [ -f /sys/class/leds/internet_led/delay_on ] && echo 125 > /sys/class/leds/internet_led/delay_on
	        [ -f /sys/class/leds/internet_led/delay_off ] && echo 125 > /sys/class/leds/internet_led/delay_off
	        [ -f /sys/class/leds/internet_led/timeout ] && echo 500 > /sys/class/leds/internet_led/timeout
	else
		if [  -n "`/bin/cat /tmp/adsl_status | grep "7"`" ]; then
		        echo dsl_data > /sys/class/leds/internet_led/trigger
		        echo 1 > /sys/class/leds/internet_led/brightness
		        [ -f /sys/class/leds/internet_led/delay_on ] && echo 125 > /sys/class/leds/internet_led/delay_on
	        	[ -f /sys/class/leds/internet_led/delay_off ] && echo 125 > /sys/class/leds/internet_led/delay_off
	                [ -f /sys/class/leds/internet_led/timeout ] && echo 500 > /sys/class/leds/internet_led/timeout
		fi
	fi
fi
