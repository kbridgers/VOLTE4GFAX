#!/bin/sh /etc/rc.common
START=06

# Include model information
if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
	. /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

# Get the platform information.
platform=${CONFIG_IFX_MODEL_NAME%%_*}
target=`echo $platform | cut -c -4`
#echo "$target"


check_bonding_status() {
	bonding=`/usr/sbin/status_oper GET xDSL_Bonding Bonding_mode`
	if [ "$sub_platform" = "vrx318" ]; then
		bonding_mode="NA"
	else
		if [ "$bonding" = "" ]; then 
			if [ "$CONFIG_PACKAGE_KMOD_LTQCPE_PPE_E5_OFFCHIP_BONDING" = "1" ]; then
				# OFF Chip Bonding
				bonding_mode="2"
			else
				# ON Chip Bonding
				bonding_mode="1"
			fi
		else
			bonding_mode="$bonding"
		fi
	fi
	/usr/sbin/status_oper SET "xDSL_Bonding" "Bonding_mode" $bonding_mode "Bonding_status" "INACTIVE" "active_line" "-1" "Status_0" "DOWN" "Status_1" "DOWN" "US_0" "0" "DS_0" "0" "US_1" "0" "DS_1" "0"
}

if [ "$wanphy_phymode" = "5" -o "$wanphy_phymode" = "6" ]; then 
	#if 3G, use D5 as ethernet driver 
	echo "Wireless WAN Mode selected"
	wanphy_phymode="2"
	# if Dual WAN is enabled, use the driver for the alternate mode
	if [ "$CONFIG_FEATURE_DUAL_WAN_SUPPORT" = "1" -a "$dw_failover_state" = "1" ]; then
		if [ "$dw_pri_wanphy_phymode" = "5" -o  "$dw_pri_wanphy_phymode" = "6" ]; then
			wanphy_phymode=$dw_sec_wanphy_phymode
			wanphy_tc=$dw_sec_wanphy_tc
		elif [ "$dw_sec_wanphy_phymode" = "5" -o  "$dw_sec_wanphy_phymode" = "6" ]; then
			wanphy_phymode="$dw_pri_wanphy_phymode"
			wanphy_tc="$dw_pri_wanphy_tc"
		fi
	fi
fi


toUpper(){ echo "$1"|sed 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/';}
toLower(){ echo "$1"|sed 'y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/';}

if [ $qm_enable -eq 1 ]; then
   wan_qos=8
else
   wan_qos=0
fi

if [ "$CONFIG_FEATURE_WWAN_LTE_SUPPORT" = "1" ]; then
	wan_itf=8
else
	wan_itf=0
fi
set_platform_config() {
	module=""
	ppe_module=""
	ppe_platform=""
	builtin_package=""
	loadable_package=""
	phy_mode=$2

	case $target in
		ARX1|GRX1|GW188)
			platform="ar9"
			module="a5"
			if [ "$1" = "PTM" ]; then
				module="e5"
			elif [ "$1" = "ETHERNET" ]; then
				module="d5"
			fi
		;;
		VRX2|GRX2)
			platform="vr9"
			module="a5"
			if [ "$1" = "ETHERNET" -o "$1" = "LTE" ]; then
				module="d5"
			elif [ "$1" = "PTM" ]; then
				phy_mode="0"
				module="e5"
			fi
		;;
		ARX3|GRX3|VRX3)
			platform="ar10"
			[ "$CONFIG_TARGET_LTQCPE_PLATFORM_AR10_VRX318" = "1" ] && {
				sub_platform="vrx318"
				if [ "$CONFIG_PACKAGE_KMOD_LTQCPE_PPA_VRX318_MODULE" = "1" ]; then
					loadable_package_val=1
				elif [ "$CONFIG_PACKAGE_KMOD_LTQCPE_PPA_VRX318_BUILTIN" = "1" ]; then
					builtin_package_val=1
				fi
				if [ "$1" = "ATM" ]; then
					module="a1"
				elif [ "$1" = "PTM" ]; then
					phy_mode="0"
					module="e1"
				fi
			} || {
				module="a5"
				if [ "$1" = "ETHERNET" -o "$1" = "LTE" ]; then
					module="d5"
				 elif [ "$1" = "PTM" ]; then
					phy_mode="0"
					module="e5"
				fi
			}
		;;
	esac

# NOTE - The below macros needs to be present in config.sh. 
# So, defining them for use here. Please dont remove them
#
# CONFIG_PACKAGE_KMOD_LTQCPE_PPA_A5_BUILTIN
# CONFIG_PACKAGE_KMOD_LTQCPE_PPA_E5_BUILTIN
# CONFIG_PACKAGE_KMOD_LTQCPE_PPA_D5_BUILTIN
# CONFIG_PACKAGE_KMOD_LTQCPE_PPA_A5_MOD
# CONFIG_PACKAGE_KMOD_LTQCPE_PPA_E5_MOD
# CONFIG_PACKAGE_KMOD_LTQCPE_PPA_D5_MOD
#
	ppe_module=`toUpper $module`
	ppe_platform=`toUpper $platform`
	if [ "$ppe_module" != "" -a "$sub_platform" = "" ]; then
		builtin_package=CONFIG_PACKAGE_KMOD_LTQCPE_PPA_${ppe_module}_BUILTIN
		loadable_package=CONFIG_PACKAGE_KMOD_LTQCPE_PPA_${ppe_module}_MOD
		eval builtin_package_val='$'$builtin_package
		eval loadable_package_val='$'$loadable_package
	fi
}

update_ppe_status() {
	if [ "$sub_platform" != "" ]; then
	/usr/sbin/status_oper SET "ppe_config_status" "ppe_platform" $ppe_platform "ppe_firmware" $ppe_module "ppe_subtarget" $sub_platform
	else
	/usr/sbin/status_oper SET "ppe_config_status" "ppe_platform" $ppe_platform "ppe_firmware" $ppe_module
	fi
}

load_modules() {
	set_platform_config $1 $2
	
	if [ "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" = "1" ]; then
		check_bonding_status
	fi
	if [ "$builtin_package_val" = "1" ]; then
		echo "package builtin - $builtin_package"
		update_ppe_status
	elif [ "$loadable_package_val" = "1" ]; then
		if [ "$sub_platform" = "vrx318" ]; then
			ethernet_driver=ltqmips_ppe_drv.ko
			if [ -r /lib/modules/*/$ethernet_driver ]; then
#				echo "insmod /lib/modules/*/$ethernet_driver ethwan=$phy_mode wanitf=$wan_itf"
				insmod /lib/modules/*/$ethernet_driver ethwan=$phy_mode wanitf=$wan_itf wanqos_en=$wan_qos
			fi
			if [ "$ppe_module" != "" ]; then
				datapath_driver=ltqmips_${sub_platform}_${module}.ko
				insmod /lib/modules/*/$datapath_driver
			else
				ppe_module="D5"
			fi
		else
			datapath_driver=ifxmips_ppa_datapath_${platform}_${module}.ko
			hal_driver=ifxmips_ppa_hal_${platform}_${module}.ko
			if [ -r /lib/modules/*/$datapath_driver ]; then
				if [ "$ppe_module" = "D5" -a "$target" = "GRX2" -a "$CONFIG_FEATURE_WWAN_LTE_SUPPORT" = "1" ]; then
					insmod /lib/modules/*/$datapath_driver ethwan=$phy_mode wanqos_en=0 wanitf=$wan_itf
				elif [ "$ppe_module" = "D5" -a "$target" = "GRX2" ]; then
					insmod /lib/modules/*/$datapath_driver ethwan=$phy_mode wanqos_en=0
				elif [ "$ppe_module" = "E5" -a "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" = "1" ]; then
					insmod /lib/modules/*/$datapath_driver ethwan=$phy_mode wanqos_en=$wan_qos dsl_bonding=$bonding_mode
				else
					insmod /lib/modules/*/$datapath_driver ethwan=$phy_mode wanqos_en=$wan_qos wanitf=$wan_itf
				fi
				insmod /lib/modules/*/$hal_driver
			fi
		fi
		insmod /lib/modules/*/ifx_ppa_api.ko
		insmod /lib/modules/*/ifx_ppa_api_proc.ko 

		update_ppe_status
		# Disable flow control if PPA is enabled.
		if [ "$CONFIG_IFX_CONFIG_CPU" = "XRX288" ]; then
			switch_cli IFX_ETHSW_PORT_CFG_SET nPortId=6 eFlowCtrl=4
		fi
	else
		if [ "$CONFIG_PACKAGE_KMOD_LTQCPE_ETH_DRV_MOD" = "1" ]; then
			grep ifxmips_eth_drv /proc/modules
			if [ "$?" != "0" ]; then
				insmod /lib/modules/*/ifxmips_eth_drv.ko
			fi
		elif [ "$CONFIG_PACKAGE_KMOD_LTQCPE_ETH_DRV_BUILTIN" = "1" ]; then
			echo -n
		else
			echo "Ethernet Driver not available. Exit Show"
		fi
		case $1 in 
			ATM)
				if [ "$CONFIG_PACKAGE_KMOD_LTQCPE_ATM_MOD" = "1" ]; then
					# This is Non-Accelerated Mode, so insert stand-alone
					# Ethernet/Switch driver along with A1 ATM Driver
					insmod /lib/modules/*/ifxmips_atm.ko
					ppe_module="A1"
					update_ppe_status
				fi
			;;	
			PTM)
				if [ "$CONFIG_PACKAGE_KMOD_LTQCPE_PTM_MOD" = "1" ]; then	
					# This is Non-Accelerated Mode, so insert stand-alone
					# Ethernet/Switch driver along with E1 PTM Driver
					insmod /lib/modules/*/ifxmips_ptm.ko
					ppe_module="E1"
					update_ppe_status
				fi
			;;
			ETHERNET)
				switch_cli IFX_FLOW_REGISTER_SET nRegAddr=0xccd nData=0x20
			;;
		esac
	fi
}

disable_vrx_switch_ports() {
	SWITCH_PORT="0 1 2 3 4 5 7 8 9"
	for argument in $SWITCH_PORT ;do 
		switch_cli dev=$CONFIG_LTQ_SWITCH_DEVICE_ID IFX_ETHSW_PORT_CFG_SET nPortId=$argument eEnable=0 bUnicastUnknownDrop=0 bMulticastUnknownDrop=0 bReservedPacketDrop=0 bBroadcastDrop=0 bAging=0 bLearningMAC_PortLock=1 nLearningLimit=255 ePortMonitor=0 eFlowCtrl=0
	done 

	SWITCH_PORT="6" 
	for argument in $SWITCH_PORT ;do 
		switch_cli dev=$CONFIG_LTQ_SWITCH_DEVICE_ID IFX_ETHSW_PORT_CFG_SET nPortId=$argument eEnable=0 bUnicastUnknownDrop=0 bMulticastUnknownDrop=0 bReservedPacketDrop=0 bBroadcastDrop=0 bAging=0 bLearningMAC_PortLock=0 nLearningLimit=255 ePortMonitor=0 eFlowCtrl=0
	done 
}

enable_vrx_switch_ports() {
	SWITCH_PORT="6" 
	for argument in $SWITCH_PORT ;do 
		switch_cli dev=$CONFIG_LTQ_SWITCH_DEVICE_ID IFX_ETHSW_PORT_CFG_SET nPortId=$argument eEnable=1 bUnicastUnknownDrop=0 bMulticastUnknownDrop=0 bReservedPacketDrop=0 bBroadcastDrop=0 bAging=0 bLearningMAC_PortLock=0 nLearningLimit=255 ePortMonitor=0 eFlowCtrl=0
	done 

	SWITCH_PORT="0 1 2 3 4 5 7 8 9"
	for argument in $SWITCH_PORT ;do 
		switch_cli dev=$CONFIG_LTQ_SWITCH_DEVICE_ID IFX_ETHSW_PORT_CFG_SET nPortId=$argument eEnable=1 bUnicastUnknownDrop=0 bMulticastUnknownDrop=0 bReservedPacketDrop=0 bBroadcastDrop=0 bAging=0 bLearningMAC_PortLock=0 nLearningLimit=255 ePortMonitor=0 eFlowCtrl=0
	done 
}

start() {
	
	if [ "$CONFIG_IFX_CONFIG_CPU" = "XRX288" ]; then
		disable_vrx_switch_ports;
	fi

	if [ "$wanphy_phymode" = "1" -o "$wanphy_phymode" = "2" ]; then #Ethernet
		load_modules "ETHERNET" $wanphy_phymode
	elif [ "$wanphy_phymode" = "6" -a "$CONFIG_FEATURE_DUAL_WAN_SUPPORT" != "1" ]; then
		load_modules "LTE" 2
	else
		if [ "$wanphy_tc" = "0" ]; then 
			load_modules ATM 0 
		elif [ "$wanphy_tc" = "1" ]; then
			load_modules PTM 0 
		else
			echo "UNSUPPORTED TC MODE : $wanphy_tc"
			if [ ! -f /etc/rc.conf.gz ]; then
				#Running in evaluation mode. Loading ethernet driver..
				cat /proc/modules | grep ifxmips_eth_drv
				[ $? -ne 0 ] && insmod /lib/modules/*/ifxmips_eth_drv.ko && [ `mount|grep -q nfs;echo $?` -eq  0 ] || ifconfig eth0 up
			fi
		fi
	fi
	if [ "$platform" = "vr9" ]; then
		enable_vrx_switch_ports;
	fi
}

