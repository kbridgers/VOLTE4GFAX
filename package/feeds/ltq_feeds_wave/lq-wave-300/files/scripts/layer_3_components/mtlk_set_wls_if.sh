#!/bin/sh
return_status=true

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then			
	. /tmp/mtlk_init_platform.sh
fi
print2log DBG "mtlk_set_wls_if.sh: args: $*"

command=$1
apIndex=$2

# Get corresponding wlan network interface from mapping file
wlan=`find_wave_if_from_index $apIndex`

# Check if the wireless is connected to internal switch, as opposed to external.
# This is relevant for ARX, which uses external switch, and therefore supports a limited feature set in switch_cli.
# TODO: We lack a better method of detecting this, look at platform type.
if [ "$CONFIG_IFX_MODEL_NAME" = "ARX168_RT_HE_BASE" ]
then 
	has_internal_switch=false
else
	has_internal_switch=true
fi

# Run switch_cli command if supported by the platform
safe_switch_cli () {
	if [ "$has_internal_switch" = "true" ]
	then 
		switch_cli $@
	fi
}

start_set_wls_if()
{
	print2log DBG "mtlk_set_wls_if.sh: Start"

	phy_wlan=`echo $wlan | cut -d "." -f 1`
	
	# For Actiontec boards, there is a total of 16 MACs reserved per device.
	# wlan0 mac is based on board mac in uboot + offset 8 (dec base)
	# wlan1 mac is based on board mac in uboot + offset 13 (dec base) - unused, because this is not dual band device.
	# wlan0_inc=8
	# wlan1_inc=13
	
	# TODO: For all other platforms (uncomment outside of Actiontec branch):
	# First interface mac is based on board mac in uboot + offset 16 (dec base)
	# Second interface mac is based on board mac in uboot + offset 24 (dec base)
	# Check the apIndex of the physical interface, 0=First interface, 1=second interface.
	phy_index=`find_index_from_wave_if $phy_wlan`
	index0_inc=16
	index1_inc=24
	eval wlan_inc=\${index${phy_index}_inc}
	
	# Remove ifconfig 0.0.0.0. It is the standard bridge configuration procedure, but this step is unneeded, 
	# and causes an additional redundant activation of the wireless MAC
	# ifconfig $wlan 0.0.0.0 down
	
	NETWORK_TYPE=`host_api get $$ $apIndex network_type`
	
	# Get the MAC of the platform
	board_mac=`/usr/sbin/status_oper GET dev_info mac_addr`
	if [ -z "$board_mac" ]; then
		board_mac=`/usr/sbin/upgrade mac_get 0`
	fi
	if [ -z "$board_mac" ]; then
		print2log ALERT "mtlk_set_wls_if: No MAC is defined for the platform in u-boot"
		echo "mtlk_set_wls_if: No MAC is defined for the platform in u-boot" >> $wave_init_failure
		exit 1
	fi

	print2log DBG "mtlk_set_wls_if.sh: Board MAC = $board_mac"
	
	# Divide the board MAC address to the first 3 bytes and the last 3 byte (which we are going to increment).
	board_mac1=0x`echo $board_mac | cut -c 1-2`
	board_mac23=`echo $board_mac | cut -c 4-8`
	board_mac46=0x`echo $board_mac | sed s/://g | cut -c 7-12`

	# Increment the last byte by the the proper incrementation according to the physical interface (wlan0 or wlan1)
	let board_mac46=$board_mac46+$wlan_inc

	
	# If it is AP, verify MAC ends with 0 or 8 only.
	if [ "$NETWORK_TYPE" = "$AP" ]
	then
		let "suffix=$board_mac46&7"
		if [ $suffix != 0 ]
		then
			print2log ALERT "#####################################################################################"
			print2log ALERT "######### mtlk_set_wls_if: MAC of $phy_wlan is wrong. Must end with 0 or 8 ##########"
			print2log ALERT "######### Number of supported VAPs may be limited due to this error #################"
			print2log ALERT "#####################################################################################"
			# Uncomment the following two lines if you want to disable wlan bringup with unaligned MACs
			# $ETC_PATH/mtlk_insmod_wls_driver.sh stop
			# exit 1
		fi
	fi
	

	# For VAP, use MAC of physical AP incremented by the index of the interface name+1 (wlan0.0 increment wlan0 by 0+1, wlan1.2 increment wlan1 by 2+1).
	if [ "$NETWORK_TYPE" = "$VAP" ]
	then
		# Increment the last byte by the index of the interface name+1.
		index_to_increment=`echo $wlan | cut -d "." -f 2`
		let index_to_increment=index_to_increment+1
		let board_mac46=$board_mac46+$index_to_increment
	fi
	
	# Generate the new MAC.
	let vap_mac4=$board_mac46/65536
	let board_mac46=$board_mac46-$vap_mac4*65536
	let vap_mac5=$board_mac46/256
	let board_mac46=$board_mac46-$vap_mac5*256
	vap_mac6=$board_mac46
	# If the 4th byte is greater than FF (255) set it to 00.
	if [ $vap_mac4 -ge 256 ]
	then
		vap_mac4=0
	fi
	vap_mac=`printf '%02X:%s:%02X:%02X:%02X' $board_mac1 $board_mac23 $vap_mac4 $vap_mac5 $vap_mac6`
	print2log DBG "mtlk_set_wls_if.sh: New VAP MAC = $vap_mac"

	# Set new MAC
	/sbin/ifconfig $wlan hw ether $vap_mac

	brctl addif br0 $wlan

	if [ "$NETWORK_TYPE" = "$STA" ]
	then
		ifconfig wlan0 up
	fi

	# support PPA. PPA is supported on 2 physical interfaces
	if [ "$NETWORK_TYPE" = "$AP" ]
	then
		PPAenabled=`host_api get $$ $apIndex PPAenabled`

		# Check if PPA is hooked, if not, don't add interface to PPA
		ppa_not_init=`cat /proc/ppa/api/hook | grep "not init" -c`
		if [ "$ppa_not_init" = "0" ]
		then
			if [ "$PPAenabled" = "1" ]
			then
				# When working with wlan1, check if PPA for wlan0 is enabled and running.
				if [ "$wlan" = "wlan1" ]
				then
					wlan0_ppa=`ppacmd getlan | grep wlan0 -c `
					if [ "$wlan0_ppa" = "0" ]
					then
						# Get the index of wlan0
						wlan0Index=`find_index_from_wave_if wlan0`
						PPAenabled0=`host_api get $$ $wlan0Index PPAenabled`
						if [ "$PPAenabled0" = "$YES" ]
						then
							print2log DBG "mtlk_set_wls_if.sh: iwpriv wlan0 sIpxPpaEnabled 1"
							iwpriv wlan0 sIpxPpaEnabled 1
							ppacmd addlan -i wlan0
							wlan0_nPortId=`ppacmd getportid -i wlan0 | sed 's/The.* is //'`
							let wlan0_nPortId=$wlan0_nPortId+4
							safe_switch_cli IFX_ETHSW_PORT_CFG_SET nPortId=$wlan0_nPortId bLearningMAC_PortLock=1
						fi
					fi
				fi
				print2log DBG "mtlk_set_wls_if.sh: iwpriv $wlan sIpxPpaEnabled 1"
				iwpriv $wlan sIpxPpaEnabled 1
				ppacmd addlan -i $wlan
				nPortId=`ppacmd getportid -i $wlan | sed 's/The.* is //'`
				let nPortId=$nPortId+4
				safe_switch_cli IFX_ETHSW_PORT_CFG_SET nPortId=$nPortId bLearningMAC_PortLock=1
			fi
		fi	
	fi
	print2log DBG "mtlk_set_wls_if.sh: Finish"
}

stop_set_wls_if()
{
	NETWORK_TYPE=`host_api get $$ $apIndex network_type`
	if [ "$NETWORK_TYPE" = "$AP" ]
	then
		ppacmd getportid -i $wlan > /dev/null
		if [ $? -eq 0 ]
		then
			nPortId=`ppacmd getportid -i $wlan | sed 's/The.* is //'`
			let nPortId=$nPortId+4
			safe_switch_cli IFX_ETHSW_PORT_CFG_SET nPortId=$nPortId bLearningMAC_PortLock=0
			ppacmd dellan -i $wlan
		fi
	fi
	brctl delif br0 $wlan
	ifconfig $wlan down

	return
}

create_config_set_wls_if()
{
	return
}

should_run_set_wls_if()
{
	if [ -e $wave_init_failure ]
	then
		return_status=false
	fi
}


case $command in
	start)
		start_set_wls_if
	;;
	stop)
		stop_set_wls_if
	;;
	create_config)
		create_config_set_wls_if
	;;
	should_run)
		should_run_set_wls_if
	;;
esac

$return_status
