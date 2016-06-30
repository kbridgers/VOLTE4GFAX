#!/bin/sh

# Defines
if [ ! "MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

if [ "$1" = "w1" ] 
then
  echo delete tmp_wpa_state_mirr
  rm /tmp/tmp_wpa_state_mirr
else
	/tmp/wpa_cli status | awk -F '=' '/^wpa_state/ {print $2}' > /tmp/tmp_wpa_state_mirr
fi

