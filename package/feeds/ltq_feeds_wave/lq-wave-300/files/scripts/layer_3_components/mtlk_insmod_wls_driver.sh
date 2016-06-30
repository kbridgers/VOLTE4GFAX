#!/bin/sh
return_status=true

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

command=$1
driver_mode=$2
if [ -z "$driver_mode" ]; then driver_mode="ap"; fi

insmod_device()
{
	cd /tmp	
	insmod $@
	# verify that insmod was successful
	if [ `lsmod | grep "mtlk " -c` -eq 0  ]
	then
		print2log ALERT "mtlk_insmod_wls_driver: driver insmod is failed"
		echo "mtlk_insmod_wls_driver: driver insmod is failed" >> $wave_init_failure
		exit
        fi

	cd - > /dev/null
}

rmmod_device()
{
	device=$1
	rmmod $device	
	res=$?
	# verify that rmmod was successful
	count=0
	while [ $res != 0 ]
	do
		if [ $count -lt 3 ]
		then
			rmmod $device
			res=$?
			sleep 2
		else
			print2log ALERT "mtlk_insmod_wls_driver: rmmod of $device is failed"
			print2log INFO "mtlk_insmod_wls_driver: Kill drvhlpr process before rmmod of $device"
			break
		fi
		count=`expr $count + 1`
	done		
}

check_wave400()
{
	if [ `ls /sys/bus/platform/devices/ | grep mtlk -c` -eq 0 ]
	then
		return_status=false
	fi
}

start_insmod_wls_driver()
{	
	print2log DBG "mtlk_insmod_wls_driver: Start"
	# Get the index of wlan0
	apIndex=`find_index_from_wave_if wlan0`
	wlan_count=`host_api get $$ sys wlan_count`	
	NETWORK_TYPE=`host_api get $$ $apIndex network_type`
	wlanm_cmd=""
	sys_freq_cmd=""
	check_wave400
	if [ $return_status == true ]
	then
		#wlanm is retrieved from u-boot and represents the MAC FW start address in DDR.
		wlanm=`uboot_env --get --name wlanm | cut -d M -f 0`
		wlanm_cmd="bb_cpu_ddr_mb_number=$wlanm"
		sys_freq=`cat /proc/driver/ifx_cgu/clk_setting | awk '/DDR clock/ {print $4}'`
		sys_freq_cmd="cpu_freq=$sys_freq"
	fi

	if [ "$NETWORK_TYPE" = "$AP" ]
	then
		if [ $wlan_count = 1 ]
		then
			insmod_device mtlk.ko $driver_mode=1 $wlanm_cmd $sys_freq_cmd
		else
			insmod_device mtlk.ko $driver_mode=1,1 $wlanm_cmd $sys_freq_cmd
		fi
	else
		insmod_device mtlk.ko $wlanm_cmd $sys_freq_cmd
	fi
	
	print2log DBG "mtlk_insmod_wls_driver: Done Start"
}

stop_insmod_wls_driver()
{
	print2log DBG "mtlk_insmod_wls_driver: Stop"
		
	rmmod_device mtlk
	
	print2log DBG "mtlk_insmod_wls_driver: Done Stop"
}

create_config_insmod_wls_driver()
{	
	return	
}

should_run_insmod_wls_driver()
{
	if [ -e $wave_init_failure ]
	then
		return_status=false
	fi
}

case $command in
	start)
		start_insmod_wls_driver
	;;
	stop)
		stop_insmod_wls_driver
	;;
	create_config)
		create_config_insmod_wls_driver
	;;
	should_run)
		should_run_insmod_wls_driver
	;;
esac

$return_status
