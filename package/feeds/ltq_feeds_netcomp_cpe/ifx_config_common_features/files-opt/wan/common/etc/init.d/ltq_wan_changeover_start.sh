#!/bin/sh
if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		. /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi
if [ ! "$CONFIGLOADED" ]; then
	if [ -r /etc/rc.d/config.sh ]; then
		. /etc/rc.d/config.sh 2>/dev/null
		CONFIGLOADED="1"
	fi
fi
. /etc/rc.d/common_func.sh

echo "WAN Changeover Start"

eval any_wan=`/usr/sbin/status_oper GET AnyWan status`
if [ "A$any_wan" == "A1" ]; then #check for anywan status 
        anywan_flow="1"
fi

# Function to start the DSL process or update the DSL params
# If the current mode is DSL, the function checks if the DSL driver is loaded 
# and the DSL process is running. If yes, then it updates the DSL Config Parameters
# for xTSE bit setting and SIC Bits. If not, the Driver is loaded and the process is started   
start_or_update_dsl() {
	if [ "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" = "1" ]; then
		bonding=`/usr/sbin/status_oper GET xDSL_Bonding Bonding_mode`
		if [ "$bonding" != "0" ]; then 
			line_num="-1"
		fi
	fi
	# check if the DSL process is running.
	grep -qw dsl_cpe_control /proc/*/status && {
		echo "Update DSL xTSE and SIC Params"
		BIN_DIR=/opt/lantiq/bin
		FW_TYPE=`/usr/sbin/status_oper GET dsl_fw_type type`
		# Get the ADSL Bits based on the loaded firmware.
		ADSL_XTSE_Bits=`[ "$FW_TYPE" = "a" ] && echo "0x5 0x0 0x4 0x0 0xc 0x1 0x0" || echo "0x10 0x0 0x10 0x0 0x0 0x4 0x0"` 

		# Set the xTSE bits		
		if [ "$CONFIG_PACKAGE_IFX_DSL_CPE_API_VRX" = "1" -o "$CONFIG_PACKAGE_IFX_DSL_CPE_API_VRX_BONDING" = "1" ]; then
			if [ "$wan_mode" = "AUTO" ]; then
				${BIN_DIR}/dsl_cpe_pipe.sh g997xtusecs $line_num $ADSL_XTSE_Bits 0x7
			elif [ "$wan_mode" = "VDSL" ]; then
				${BIN_DIR}/dsl_cpe_pipe.sh g997xtusecs $line_num 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x7
			elif [ "$wan_mode" = "ADSL" ]; then
				${BIN_DIR}/dsl_cpe_pipe.sh g997xtusecs $line_num $ADSL_XTSE_Bits 0x0
			fi
		fi

		# Set the SystemInterfaceConfiguration for the ADSL and VDSL Mode as per user configuration
		# nDSL_Mode - 0 - ADSL, 1 - VDSL
		# nTC_Mode - 1 - ATM, 2 - PTM, 4 - Auto
		for i in $nDSL_Mode; do
			${BIN_DIR}/dsl_cpe_pipe.sh sics $line_num $i $nTC_Mode 1 1 3
		done

#		${BIN_DIR}/dsl_cpe_pipe.sh acos $line_num 1 1 1 1 1 1 
		${BIN_DIR}/dsl_cpe_pipe.sh acs $line_num 2
		sleep 1
	} || {
		echo "Start DSL"
		`grep -q "drv_dsl_cpe_api" /proc/modules` || {
			/etc/init.d/ltq_load_dsl_cpe_api.sh start
		}
		grep -qw dsl_cpe_control /proc/*/status || {
			/etc/init.d/ltq_cpe_control_init.sh start
		}
	}
}
#echo "Restarting the WAN Services"
if [ -z "`grep ifxmips_ppa /proc/modules`" ]; then
	#  echo "Loading the new ppa driver modules"                                     
	/etc/init.d/load_ppa_modules.sh start    
fi 

wan_mode="NOT_DSL"
if [ -e /etc/rc.d/ltq_dsl_functions.sh ]; then
	. /etc/rc.d/ltq_dsl_functions.sh 
fi	

# This funtion gets the nDSL_Mode and nTC_Mode filled with configured PHY and TC information.
get_phy_tc_info
if [ "$wan_mode" != "NOT_DSL" -a "$1" != "dsl" ]; then
	start_or_update_dsl &
fi

# start PPA commands if PPA is enabled
if [ "A$CONFIG_FEATURE_PPA_SUPPORT" = "A1" ]; then
	/sbin/ppacmd init 2> /dev/null

	# echo "add lan interface to ppa"
	. /etc/rc.d/get_lan_if
	/etc/rc.d/ppa_config.sh addlan $LAN_IFNAME
	if [ "$lan_port_sep_enable" = "0" ]; then
		if [ "$wanphy_phymode" = "1" ]; then
			/sbin/ppacmd addvlanrange -s 3 -e 0xfff 2> /dev/null	
			/etc/rc.d/ppa_config.sh addlan eth0.501
		else
			/etc/rc.d/ppa_config.sh addlan eth0
		fi
	fi

	#if PPA is enabled, enable hardware based QoS to be used later
	if [ -z  "$anywan_flow" ]; then #check for anywan 
		. /etc/init.d/ipqos_qprio_cfg.sh
	fi
fi

. /etc/init.d/ltq_switch_config.sh do_switch_config

if [ "$CONFIG_FEATURE_LTQ_PORT_WAN_BINDING" = "1" -a "$port_wan_binding_status_enable" = "1" ]; then
	if [ -n "$dw_failover_state" -a "$dw_failover_state" = "1" -a "$dw_standby_type" = "2" ]; then
		# In case of dual wan in hybrid mode two WAN phy modes will be active at a time. 
		# calling ltq_pwb_config.sh add_all will be done only once in this case ans it is from MAPI.
		echo
	else
		. /etc/rc.d/ltq_pwb_config.sh add_all
	fi
fi

do_wan_config gen_start

#echo "Starting IPQoS"
if [ -z  "$anywan_flow" ]; then #check for anywan
	/etc/init.d/init_ipqos.sh start
fi

#echo "Register WLAN to direct path"
/etc/init.d/ltq_wlan_register_ppa.sh

if [ "$CONFIG_FEATURE_WAN_AUTO_DETECT" = "1" ]; then
	# Handle WAN connections manually only if auto WAN detect is disabled
	wanStart="1"
	get_wan_start_flag
	wanStart=$__wanStart
	if [ "$wanStart" = "0" ]; then
		#echo "changing default wan connection"
		/usr/sbin/ifx_event_util "DEFAULT_WAN" "MOD"
	fi
else
	/usr/sbin/ifx_event_util "DEFAULT_WAN" "MOD"
fi
