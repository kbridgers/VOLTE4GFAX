#!/bin/sh

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi

command=$1

drvhlpr_dut="drvhlpr_dut"
TMP=/tmp

start_dut_drvctrl()
{
	print2log DBG "wave_wlan_dut_drvctrl.sh: Start"
	(. $ETC_PATH/mtlk_insmod_wls_driver.sh start dut)
	echo "f_saver = $ETC_PATH/wave_wlan_dut_file_saver.sh" > $TMP/$drvhlpr_dut.config
	ln -s $BINDIR/drvhlpr $TMP/$drvhlpr_dut
	$TMP/$drvhlpr_dut --dut -p $TMP/$drvhlpr_dut.config &
	touch /tmp/dut_mode_on
	print2log DBG "wave_wlan_dut_drvctrl.sh: Start done"
}

first_stop_dut_drvctrl()
{
	print2log DBG "wave_wlan_dut_drvctrl.sh: First Stop"
	$ETC_PATH/rc.bringup_wlan stop
	$ETC_PATH/mtlk_insmod_wls_driver.sh stop
	print2log DBG "wave_wlan_dut_drvctrl.sh: First Stop done"
}

stop_dut_drvctrl()
{
	print2log DBG "wave_wlan_dut_drvctrl.sh: Stop"
	if [ -e /tmp/dut_mode_on ]; then rm /tmp/dut_mode_on; fi
	$ETC_PATH/mtlk_insmod_wls_driver.sh stop
	print2log DBG "wave_wlan_dut_drvctrl.sh: Stop done"
}

case $command in
	start )
		start_dut_drvctrl
    ;;
	stop )
		if [ -e /tmp/dut_mode_on ] ; then
			stop_dut_drvctrl
		else
			first_stop_dut_drvctrl
		fi
	;;
	* )
		print2log WARNING "wave_wlan_dut_drvctrl.sh: Unknown command=$command"
    ;;
esac
