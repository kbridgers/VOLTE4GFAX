#!/bin/sh
return_status=true

# Defines
if [ ! "MTLK_INIT_PLATFORM" ]; then
	. /tmp/mtlk_init_platform.sh
fi

command=$1
apIndex=$2

# Get corresponding wlan network interface from mapping file
wlan=`find_wave_if_from_index $apIndex`

start_mtlk_init_mbss()
{
	print2log DBG "mtlk_init_mbss.sh: Start"
	
	# vap needs to be added to phy AP
	phyAP=`echo $wlan | cut -f1 -d "."`
	# vap index to add is needed by driver
	vapIdx=`echo $wlan | cut -f2 -d "."`

	print2log DBG "mtlk_init_mbss.sh: phyAP = $phyAP"
	print2log DBG "mtlk_init_mbss.sh: vapIdx $vapIdx"
	iwpriv $phyAP sAddVap $vapIdx
	res=$?
	# verify that VAP was added successful
	count=0
	while [ $res != 0 ]
	do
		if [ $count -lt 10 ]
		then
			sleep 1
			iwpriv $phyAP sAddVap $vapIdx
			res=$?
		else
			print2log ALERT "mtlk_init_mbss.sh: add of vapIdx $vapIdx to $phyAP failed"
			echo "mtlk_init_mbss.sh: add of vapIdx $vapIdx to $phyAP failed" >> $wave_init_failure
			exit 1
		fi
		let count=$count+1
	done
	
	print2log DBG "mtlk_init_mbss.sh: BSS $wlan created. Done Start"
}

stop_mtlk_init_mbss()
{
	print2log DBG "mtlk_init_mbss.sh: Start stop_mtlk_init_mbss"
	# vap is removed from phy AP
	phyAP=`echo $wlan | cut -f1 -d "."`
	# vap needs to be removed from phy AP -> index for wave300 driver
	vapIdx=`echo $wlan | cut -f2 -d "."`
	print2log DBG "mtlk_init_mbss.sh: vapIdx $vapIdx"
	iwpriv $phyAP sDelVap $vapIdx
	res=$?
	# verify that VAP was removed successful
	count=0
	while [ $res != 0 ]
	do
		if [ $count -lt 10 ]
		then
			sleep 1
			iwpriv $phyAP sDelVap $vapIdx
			res=$?
		else
			print2log ALERT "Removing VAP $wlan failed by driver after 10 tries"
			exit
		fi
		let count=$count+1
	done
	
	print2log DBG "mtlk_init_mbss.sh: BSS $wlan removed"
}

create_config_mtlk_init_mbss()
{
	return
}

should_run_mtlk_init_mbss()
{
#	mbss=`host_api get $$ $apIndex MBSSEnabled`
#	print2log DBG "mtlk_init_mbss.sh: should_run: mbss = $mbss"
#	if [ $mbss ]
#	then
#		true
#	else
#		print2log DBG "mtlk_init_mbss.sh: should_run: MBSS not enabled"
#		false
#	fi		
#	return

	if [ -e $wave_init_failure ]
	then
		return_status=false
	fi
}


case $command in
	start)
		start_mtlk_init_mbss
	;;
	stop)
		stop_mtlk_init_mbss
	;;
	create_config)
		create_config_mtlk_init_mbss
	;;
	should_run)
		should_run_mtlk_init_mbss
	;;
esac

$return_status