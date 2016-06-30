#!/bin/sh

# Used in scenario while connecting AR9/ADSL to pure VDSL2 CO (no multi-mode support)

SWITCH_THRESHOLD=$1

sleep $SWITCH_THRESHOLD
if [ A`dsl_cpe_pipe.sh lsg | grep 801` == "A" ]; then
	# Not able to reach showtime in threshold; switch back to VINAX.
	target=/etc/rc.conf
	if [ -s $target ]; then
		temp=$( ls -l "$target" )
		target=${temp#* -> }
	fi
	cd `dirname $target`
	target=`basename $target`
	/bin/sed -i -e "s,^ppaA5WanMode=.*,ppaA5WanMode=\'2\',g" $target
	/usr/sbin/savecfg.sh
	sync; sleep 3
	reboot
fi

