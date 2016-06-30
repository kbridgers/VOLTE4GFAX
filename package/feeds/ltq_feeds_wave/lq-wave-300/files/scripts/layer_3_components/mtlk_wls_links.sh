#!/bin/sh
# This is the script set wls links.
return_status=true

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

print2log DBG "mtlk_wls_links.sh: args: $*"

command=$1

start_wls_links()
{
	print2log DBG "mtlk_wls_links.sh: start"
	ln -s $DRIVER_PATH/mtlk.ko /tmp/mtlk.ko

	cd $IMAGES_PATH
	for bin in `ls`
	do
		ln -s $IMAGES_PATH/$bin /tmp/$bin
	done
	
	cd - > /dev/null
	print2log DBG "mtlk_wls_links.sh: Done Start"
}

stop_wls_links()
{
	print2log DBG "mtlk_wls_links.sh: stop"

	cd /tmp
	rm /tmp/ap_upper*
	rm /tmp/sta_upper*
	rm /tmp/ProgModel_*
	rm /tmp/mtlk.ko
	if [ -e /tmp/contr_lm.bin ]; then rm /tmp/contr_lm.bin; fi

	cd - > /dev/null
	
	print2log DBG "mtlk_wls_links.sh: Done stop"
}

create_config_wls_links()
{
	return
}

should_run_wls_links()
{
	if [ -e $wave_init_failure ]
	then
		return_status=false
	fi
}

case $command in
	start)
		start_wls_links
	;;
	stop)
		stop_wls_links
	;;
	create_config)
		create_config_wls_links
	;;
	should_run)
		should_run_wls_links
	;;
esac

$return_status