#!/bin/sh /etc/rc.common
#########START=20

#This is being come from DSL API team, need to align with them
start() {
   if [ A`echo $MODEL_NAME | grep D5` = "A" ]; then
	if [ A"$IFX_ADSL_FIRMWARE_IN_ROOTFS" = "A" ]; then
		echo "Mount ADSL Firmware Now"
		/bin/mount /firmware
	fi
	if [ A`echo $MODEL_NAME | grep A5` != "A" ]; then
		if [ $ppaA5WanMode -eq 0 ]; then
			# ATM WAN mode
			/usr/sbin/bringup_adsl.sh
			if [ A`echo $MODEL_NAME | grep VINAX` != "A" ]; then
				if [ $vinaxSwitchEnable -eq 1 ]; then
					echo "$MODEL_NAME enabling one time checking"
					/usr/sbin/check_dsl_status.sh $vinaxSwitchThreshold &
				fi
			fi
		elif [ $ppaA5WanMode -eq 1 ]; then 
			echo "$MODEL_NAME in ETH0 WAN/LAN mixed mode. No ADSL support!!"
		elif [ $ppaA5WanMode -eq 2 ]; then
			if [ A`echo $MODEL_NAME | grep VINAX` != "A" ]; then
				if [ $vinaxVdslMode -eq 1 -o $vinaxAdslMode -eq 1 ]; then
					echo "$MODEL_NAME in ETH1 WAN mode with Vinax/xDSL!!"
					/usr/sbin/bringup_vdsl.sh $vinaxVdslMode $vinaxAdslMode $vinaxDebugLevel
				else
					echo "$MODEL_NAME in ETH1 WAN mode without Vinax/xDSL!!"
				fi
			else
				echo "$MODEL_NAME in ETH1 WAN mode. No ADSL support!!"
			fi
		else
			# default using ATM WAN
			/usr/sbin/bringup_adsl.sh
		fi
	else
		# A1 mode, always bringup DSL link ATM/EFM
		/usr/sbin/bringup_adsl.sh
	fi
   else
	if [ $ppaD5WanMode -eq 2 ]; then
		if [ A`echo $MODEL_NAME | grep VINAX` != "A" ]; then
			if [ $vinaxVdslMode -eq 1 -o $vinaxAdslMode -eq 1 ]; then
				echo "$MODEL_NAME in ETH1 WAN mode with Vinax/xDSL!!"
				/usr/sbin/bringup_vdsl.sh $vinaxVdslMode $vinaxAdslMode $vinaxDebugLevel
			else
				echo "$MODEL_NAME in ETH1 WAN mode without Vinax/xDSL!!"
			fi
		fi
	fi
   fi
}
