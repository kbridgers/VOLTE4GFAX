#!/bin/sh

# This script has been enhanced to handle the XDSL Events for Multimode FSM
# and subsequent DSL Link bringup handling.

# Add New Events from DSL FSM to handle PTM Bonding
# Refer Sec 4.5 Script Notification Handling in DSL API Rel 4.10.4 UMPR

# Include model information
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

dsl_pipe () {
	#echo "dsl_pipe $*"
	result=`/opt/lantiq/bin/dsl_cpe_pipe.sh $*`
	#echo "result $result"
	status=${result%% *}
	if [ "$status" != "nReturn=0" ]; then
		echo "dsl_pipe $* failed: $result"
	fi
}

# Function to set the Bonding, DSL Status, QoS Rates in Bonding Models
# $1 - Line Number
# $2 - Status of the LINE - UP/DOWN
update_bonding_link_status()
{
	bonding=`/usr/sbin/status_oper GET "xDSL_Bonding" "Bonding_status"`
	case $bonding in
		ACTIVE)
			# Get the status and rates of the other line.
			# ex: if line 0 information is received from the DSL, then read the status of 
			# the line 1 from system status. This used to call adsl_up and down events
			if [ "$1" = "0" ]; then
				status=`/usr/sbin/status_oper GET "xDSL_Bonding" "Status_1"`
			elif [ "$1" = "1" ]; then
				status=`/usr/sbin/status_oper GET "xDSL_Bonding" "Status_0"`
			fi
			case $2 in
				UP)
					echo "xDSL Enter SHOWTIME!! for line $1. Status of other line - $status"
					/usr/sbin/status_oper -u SET "xDSL_Bonding" "Status_$1" $DSL_INTERFACE_STATUS "US_$1" $3 "DS_$1" $4
					# if this is the first line to come up, then the wan connections have not started till now.
					# So, call the adsl_up script
					if [ "$status" == "DOWN" ]; then
						. /etc/rc.d/adsl_up 
					fi
					/etc/rc.d/dsl_qos_updates.sh "INTERFACE_STATUS_UP" &
				;;
				DOWN)
					echo "xDSL Leave SHOWTIME!! for line $1. Status of other line - $status"
					# if both lines are down, then stop wan connections. Else, update the qos rates.
					if [ "$status" = "UP" ]; then
						/etc/rc.d/dsl_qos_updates.sh "INTERFACE_STATUS_DOWN" &
					else
						. /etc/rc.d/adsl_down
						bonding="INACTIVE"
					fi
					/usr/sbin/status_oper -u SET "xDSL_Bonding" "Bonding_status" $bonding "Status_$1" $DSL_INTERFACE_STATUS "US_$1" 0 "DS_$1" 0
				;;
			esac
		;;
		# If Bonding Status is INACTIVE, then it is as good as single link. So, call adsl_up and sown events as usual
		# this needs to be fixed.
		INACTIVE)
			line=`/usr/sbin/status_oper GET xDSL_Bonding active_line`
			case $2 in
				UP)
					if [ "$line" = "-1" -o "$line" = "$1" ]; then
						echo "xDSL Enter Showtime for line $1"
						. /etc/rc.d/adsl_up 
						line="$1"
						/usr/sbin/status_oper -u SET "xDSL_Bonding" "active_line" "$line" "Status_$1" $DSL_INTERFACE_STATUS "US_$1" $3 "DS_$1" $4
						/etc/rc.d/dsl_qos_updates.sh "INTERFACE_STATUS_UP" &
					fi
				;;
				DOWN)
					if [ "$line" = "$1" ]; then
					echo "xDSL Leave Showtime for line $line"
					. /etc/rc.d/adsl_down
					/usr/sbin/status_oper -u SET "xDSL_Bonding" "active_line" "-1" "Status_$1" $DSL_INTERFACE_STATUS "US_$1" 0 "DS_$1" 0
					fi
				;;
			esac
		;;
	esac
}

# Function is used to update the Bonding Status in the system status and
# to intimate the same to the PPA by setting the proc entries
# $1 - Bonding Status - ACTIVE/INACTIVE
# $2 - LINE NUMBER
update_bonding_status() {
	sub_platform=`/usr/sbin/status_oper GET ppe_config_status ppe_subtarget`
	# By default, the board will be loaded with E5_Bonding driver
	# Now check the DSL Status as whether Bonding is Active or Inactive. 
	# Accordingly set the mode in proc status, such that the PPA is notified
	echo "Negotiated Bonding Status is - $1"
	line=`/usr/sbin/status_oper GET xDSL_Bonding active_line`
	case $1 in 
		INACTIVE)
			echo DSL_LINE_NUMBER $2 DSL_BONDING_STATUS inactive > /proc/dsl_tc/status
			if [ "$sub_platform" = "vrx318" ]; then
				echo bdmdswitch L2 > /proc/eth/vrx318/tfwdbg
			else
				echo switch_mode l2 > /proc/eth/bonding
			fi
		;;
		ACTIVE)
			echo DSL_LINE_NUMBER $2 DSL_BONDING_STATUS active > /proc/dsl_tc/status
			line="-1"
			if [ "$sub_platform" = "vrx318" ]; then
				echo bdmdswitch L1 > /proc/eth/vrx318/tfwdbg
			else
				# Set the Bonding status so that the PPA can update accordingly
				echo switch_mode l1 > /proc/eth/bonding
			fi
		;;
	esac
	/usr/sbin/status_oper -u SET "xDSL_Bonding" "Bonding_status" $1 "active_line" $line
}

# Function to handle the xDSL / xTC status negotiated by DSL.
# Based on the current configured mode and current negotiated mode, action will be taken to 
# either load the new drivers as per new TC or ignore the status.
# $1 - Negotiated DSL Phy Status
# $2 - Negotiated TC Status
phy_tc_handling()
{
	NEGOTIATED_PHY=$1
	NEGOTIATED_TC=$2
	echo "Negotiated DSL Mode = $NEGOTIATED_PHY"
	echo "Negotiated TC Mode = $NEGOTIATED_TC"

	[ "$NEGOTIATED_PHY" = "VDSL" ] && PHY_MODE="3" || PHY_MODE="0" 
	[ "$NEGOTIATED_TC" = "ATM" ] && TC_MODE="0" || TC_MODE="1" 

	# Phy Mode - Check if the negotiated mode and configured mode are same
	[ "$PHY_MODE" = "$wanphy_phymode" ] && {
		# negotiated and configured phy mode match - Starts
		# TC Mode - Check if the negotiated and the configured tc mode are same
		[ "$TC_MODE" = "$wanphy_tc" ] && {
			# EVERYTHING MATCHES - Do nothing as we have already inserted & started the DSL SM
			echo -n 
		} || { 
			# if the negotiated and configured TC modes are different,
			# then check if Auto TC is enabled
			[ "$wanphy_settc" = "2" ] && {
				#if Auto TC is enabled, then enable switchover of the PPA modules to support the new TC
				echo "User Configured Auto TC, Current Configured phy/tc = $wanphy_phymode/$wanphy_tc , setphy/settc = $wanphy_setphymode/$wanphy_settc"
				# Update the tc configuration in flash and initiate a wan mode changeover
				. /etc/init.d/ltq_wan_changeover_stop.sh dsl
				/usr/sbin/status_oper -u -f /etc/rc.conf SET "wan_phy_cfg" "wanphy_tc" $TC_MODE
				usleep 250000
				/etc/init.d/ltq_wan_changeover_start.sh dsl
			} || { 
				# if Auto TC is not configured by user, then switchover for different TC cannot be supported
				echo "User Configured wanphy/tc = $wanphy_phymode/$wanphy_tc setphy/settc = $wanphy_setphymode/$wanphy_settc"
			} 
		} 
		# negotiated and configured phy mode match - Ends
	} || {
		# negotiated and configured phy modes DO NOT match.
		# check if the Auto Phy mode configuration is enabled.
		[ "$wanphy_setphymode" = "4" ] && {
			# if Auto Phymode configuraiton is enabled, then check if the switchover need to be (or) can be initiated.
			echo "User Configured Auto PhyMode, Current Configured phy/tc = $wanphy_phymode/$wanphy_tc setphy/settc = $wanphy_setphymode/$wanphy_settc"
			# The negotiated and the configured TC are matching.
			[ "$TC_MODE" = "$wanphy_tc" ] && {
				# Already the right drivers are loaded. But wan phy configuration needs to be updated
				# update the phymode and restart wans
				echo "Restart WANs for new WAN Mode"
#				echo "Initiating WAN Stop for mode - $wanphy_phymode tc - $wanphy_tc"dd
				/etc/rc.d/rc.bringup_wan stop 
#				echo " Setting the phy mode to $DSL_XTU_STATUS"
				/usr/sbin/status_oper -u -f /etc/rc.conf SET "wan_phy_cfg" "wanphy_phymode" $PHY_MODE
#				echo " Initiating WAN Start for mode - $PHY_MODE tc - $wanphy_tc"
				/etc/rc.d/rc.bringup_wan start &
			} || { 
				# The inserted TC mode doesn't match with the current trained Mode
				# Update the configuration in flash and initiate a wan mode change 		
				. /etc/init.d/ltq_wan_changeover_stop.sh dsl
#				echo " Setting the phy mode to $DSL_XTU_STATUS, tc mode to $DSL_TC_LAYER_STATUS"
				# phy mode = DSL_XTU_STATUS; tc mode = DSL_TC_LAYER_STATUS
				/usr/sbin/status_oper -u -f /etc/rc.conf SET "wan_phy_cfg" "wanphy_phymode" $PHY_MODE "wanphy_tc" $TC_MODE
				usleep 250000
				/etc/init.d/ltq_wan_changeover_start.sh dsl
			}
		} || {
			# Neither the DSL, TC Modes match, Nor the configuration to switchover(Auto) is enabled.
			# so do not update any information in rc.conf
			echo "Current Configured wanphy/tc = $wanphy_phymode/$wanphy_tc setphy/settc = $wanphy_setphymode/$wanphy_settc"
			echo "User Configured Phy Mode and Negotiated Phy mode does not match"
		}
	}
}

# DSL Event handling script - Triggered from DSL CPE control Application
case "$DSL_NOTIFICATION_TYPE" in
	DSL_STATUS)
		# Handles the DSL Link Bringup sequence
		echo "DSL_STATUS Notification"
		case $DSL_XTU_STATUS in
			VDSL)
				echo "Negotiated DSL Status = $DSL_XTU_STATUS "
			
				if [ "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" = "1" ]; then
					update_bonding_status $DSL_BONDING_STATUS $DSL_LINE_NUMBER 
				fi
				phy_tc_handling $DSL_XTU_STATUS $DSL_TC_LAYER_STATUS

			;;
			ADSL)
				phy_tc_handling $DSL_XTU_STATUS $DSL_TC_LAYER_STATUS
				# TODO - This is a workaround to update PPA proc entry to disable Bonding mode when in ADSL PTM Mode.
				# Ideally DSL should indicate to PPA that the Bonding Mode is disabled in PTM.
				# So, remove this once the fix is available.
				if [ "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" = "1" -a "$DSL_TC_LAYER_STATUS" = "EFM" ]; then
					echo bdmdswitch L2 > /proc/eth/vrx318/tfwdbg
				fi				
			;;
		esac
#		echo "Negotiated DSL Mode = $DSL_XTU_STATUS"
#		echo "Negotiated TC Mode = $DSL_TC_LAYER_STATUS"
	;;
	DSL_INTERFACE_STATUS)
		case "$DSL_INTERFACE_STATUS" in  
			"UP")
				# DSL link up trigger
				if [ "$CONFIG_FEATURE_LED" = "1" ]; then
					if [ "$DSL_LINE_NUMBER" != "" -a "$DSL_LINE_NUMBER" = "1" ]; then
						echo none > /sys/class/leds/broadband_led1/trigger
						echo 1 > /sys/class/leds/broadband_led1/brightness
					else
						echo none > /sys/class/leds/broadband_led/trigger
						echo 1 > /sys/class/leds/broadband_led/brightness
					fi
				fi	

				if [ "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" != "1" ]; then
					echo "xDSL Enter SHOWTIME!!"
					. /etc/rc.d/adsl_up
					# Enable Far-End Parameter Request
#					/usr/sbin/dsl_cpe_pipe.sh ifcs 0 0 0 0 0 0
#					/etc/rc.d/dsl_qos_updates.sh "INTERFACE_STATUS_UP" $DSL_DATARATE_US_BC0 $DSL_DATARATE_DS_BC0 &
				else
					#set status in proc to notify the PPA
					echo DSL_LINE_NUMBER $DSL_LINE_NUMBER DSL_INTERFACE_STATUS up > /proc/dsl_tc/status
					update_bonding_link_status $DSL_LINE_NUMBER $DSL_INTERFACE_STATUS $DSL_DATARATE_US_BC0 $DSL_DATARATE_DS_BC0
				fi
				/etc/rc.d/dsl_qos_updates.sh "INTERFACE_STATUS_UP" $DSL_LINE_NUMBER $DSL_DATARATE_US_BC0 $DSL_DATARATE_DS_BC0 &
			;;
			"DOWN")
				# DSL link down trigger
				if [ "$CONFIG_FEATURE_LED" = "1" ]; then
					if [ "$DSL_LINE_NUMBER" != "" -a "$DSL_LINE_NUMBER" = "1" ]; then
						echo none > /sys/class/leds/broadband_led1/trigger
						echo 0 > /sys/class/leds/broadband_led1/brightness
					else
						echo none > /sys/class/leds/broadband_led/trigger
						echo 0 > /sys/class/leds/broadband_led/brightness
					fi
				fi	
				if [ "$CONFIG_FEATURE_DSL_BONDING_SUPPORT" != "1" ]; then
					echo "xDSL Leave SHOWTIME!!"
					. /etc/rc.d/adsl_down 
				else
					#set status in proc to notify the PPA
					echo DSL_LINE_NUMBER $DSL_LINE_NUMBER DSL_INTERFACE_STATUS down > /proc/dsl_tc/status
					update_bonding_link_status $DSL_LINE_NUMBER $DSL_INTERFACE_STATUS
				fi
			;;
			"READY")
				# DSL Handshake 2 HZ
				if [ "$CONFIG_FEATURE_LED" = "1" ]; then
					if [ "$DSL_LINE_NUMBER" != "" -a "$DSL_LINE_NUMBER" = "1" ]; then
						echo timer > /sys/class/leds/broadband_led1/trigger
						echo 1 > /sys/class/leds/broadband_led1/brightness
						echo 250 > /sys/class/leds/broadband_led1/delay_on
						echo 250 > /sys/class/leds/broadband_led1/delay_off
					else
						echo timer > /sys/class/leds/broadband_led/trigger
						echo 1 > /sys/class/leds/broadband_led/brightness
						echo 250 > /sys/class/leds/broadband_led/delay_on
						echo 250 > /sys/class/leds/broadband_led/delay_off
					fi
				fi
				# setting the Current TC setting for detecting VDSL ATM on VRX platform.
				# wanphy_tc = 0 --> Current Configuration is ATM TC and A5/A1 is loaded
				# wanphy_tc = 1 --> Current Configuration is PTM and E5/E1 is loaded
				if [ "$CONFIG_FEATURE_DUAL_WAN_SUPPORT" = "1" -a "$dw_failover_state" = "1" ]; then
					[ "$dw_pri_wanphy_phymode" = "0" -o "$dw_pri_wanphy_phymode" = "3" ] && {
						wanphy_tc="$dw_pri_wanphy_tc"
					} || {
						[ "$dw_sec_wanphy_phymode" = "0" -o "$dw_sec_wanphy_phymode" = "3" ] && {
							wanphy_tc="$dw_sec_wanphy_tc"
						}
					}
				fi
				case "$wanphy_tc" in
				"0")
					# CMD_TC_FW_InfoSet (MsgID: 0x1762), ATM TC-Layer is currently used
					dsl_pipe dms $DSL_LINE_NUMBER 1762 0 1 2
				;;
				"1")
					# CMD_TC_FW_InfoSet (MsgID: 0x1762), PTM/EFM TC-Layer is currently used
					dsl_pipe dms $DSL_LINE_NUMBER 1762 0 1 1
				;;
				esac
			;;
	
			"TRAINING")
				# DSL Training 4 HZ
				if [ "$CONFIG_FEATURE_LED" = "1" ]; then
					if [ "$DSL_LINE_NUMBER" != "" -a "$DSL_LINE_NUMBER" = "1" ]; then
						echo timer > /sys/class/leds/broadband_led1/trigger
						echo 1 > /sys/class/leds/broadband_led1/brightness
						echo 125 > /sys/class/leds/broadband_led1/delay_on
						echo 125 > /sys/class/leds/broadband_led1/delay_off
					else
						echo timer > /sys/class/leds/broadband_led/trigger
						echo 1 > /sys/class/leds/broadband_led/brightness
						echo 125 > /sys/class/leds/broadband_led/delay_on
						echo 125 > /sys/class/leds/broadband_led/delay_off
					fi
				fi	
				#echo "xDSL Training !!"
			;;
		esac
	;;

	DSL_DATARATE_STATUS)
		echo "DSL US Data Rate = "`expr $DSL_DATARATE_US_BC0 / 1000`" kbps"
#		echo $DSL_DATARATE_US_BC0 > /tmp/dsl_us_rate
		echo "DSL DS Data Rate = "`expr $DSL_DATARATE_DS_BC0 / 1000`" kbps"
#		echo $DSL_DATARATE_DS_BC0 > /tmp/dsl_ds_rate
		/etc/rc.d/dsl_qos_updates.sh "DSL_DATARATE_STATUS" $DSL_DATARATE_US_BC0 $DSL_DATARATE_DS_BC0 &
	;;

	DSL_DATARATE_STATUS_US)
		echo "DSL US Data Rate = "$(( $DSL_DATARATE_US_BC0 / 1000 ))" kbps"
		# convert the upstream data rate in kbps to cells/sec and store in running config file
		# this will be used for bandwidth allocation during wan connection creation
		# 8 * 53 = 424
		/etc/rc.d/dsl_qos_updates.sh "DSL_DATARATE_STATUS_US" $DSL_DATARATE_US_BC0 $DSL_DATARATE_DS_BC0 &
		[ "$wanphy_tc" = "1" ] && ( [ -z $CONFIG_FEATURE_LQ_OPTIMIZATION ] || ifconfig ptm0 txqueuelen 400 )
	;;
esac

