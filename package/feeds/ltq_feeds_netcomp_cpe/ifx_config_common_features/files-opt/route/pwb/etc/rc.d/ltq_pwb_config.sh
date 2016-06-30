#!/bin/sh
# Filename : ltq_pwb_config.sh
# Author: Kamal Eradath
# This Scripts handles the Addition, Deletion, and Modification of Portbinding configuration
# in the system.
# Paramters:	
#	$1: add / del / add_all / del_all / ip_add_route / ip_del_route / ppp_add_route / ppp_del_route / update_mcast_conf
#	$2: index 
# Revision History:
#	15/03/2013: Initial Version
#	27/05/2013: Added multicast handling functionality
#

# sourcing the rc.conf 
if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		. /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

# sourcing config.sh
if [ ! "$CONFIGLOADED" ]; then
        if [ -r /etc/rc.d/config.sh ]; then
                . /etc/rc.d/config.sh 2>/dev/null
                CONFIGLOADED="1"
        fi
fi

######################################################################################################################################### 	
# checking the number of input arguements
######################################################################################################################################### 	
if [ "$#" = "2" ]; then 
	SEL_OPT=$1
	SEL_IDX=$2
elif [ "$#" = "1" ]; then
	SEL_OPT=$1
	SEL_IDX="-1"
else
	echo "!!!!!!!!!!Wrong number of arguements passed!!!!!!!!!!!!!!!"
	echo "usage ltq_pwb_config.sh [add|delete|add_all|del_all|ip_add_route|ip_del_route|ppp_add_route|ppp_del_route|update_mcast_conf] {index}"
	exit 0	
fi

######################################################################################################################################### 	
# reading the global port binding status
######################################################################################################################################### 	
EN_PWB=$port_wan_binding_status_enable
PWB_MASK=0xE0000000


######################################################################################################################################### 	
# internal function to read all the required parameters based on the inputs.
######################################################################################################################################### 	
read_all_params() {

	local indx=0	
	local wan_idx=0
	local wan_flg=0
	
	#input parameter is the index of PWB rule
	indx=$1

	eval PWB_WANCON='$'pwb_${indx}_wanConName
	eval PWB_LANIFS='$'pwb_${indx}_lanIfNames
	eval PWB_NUMLAN='$'pwb_${indx}_numLANIfs
	eval PWB_NFMARK='$'pwb_${indx}_bitMask
	PWB_TABLENO=$(echo $PWB_NFMARK | awk '{ print rshift ($1, 29) }')
	PWB_TABLENAME=${PWB_WANCON}"_table"


	# to find whether the WAN Connection id IP based
	while [ $wan_idx -lt $wan_ip_Count ]
	do
		eval wan_name='$'wanip_${wan_idx}_connName
		if [ "A$PWB_WANCON" = "A$wan_name" ]; then
			wan_flg=1
			. /etc/rc.d/get_wan_if $wan_idx "ip"
			eval WAN_IF=${WAN_IFNAME}
			eval WAN_ADTYPE='$'wanip_${wan_idx}_addrType
			if [ $WAN_ADTYPE -eq 0 ]; then #bridged ip
				WAN_BRIDGE=1
			else
				eval WAN_STATUS=`/usr/sbin/status_oper GET WanIP${wan_idx}_IF_Info STATUS`
				eval WAN_GW=`/usr/sbin/status_oper GET WanIP${wan_idx}_GATEWAY ROUTER1`
			fi
			eval WAN_MODE='$'wanip_${wan_idx}_wanMode
			break
		fi
		wan_idx=$(($wan_idx + 1))
	done

	# if not IP then checking for PPP
	if [ $wan_flg -eq 0 ]; then
		wan_idx=0
		while [ $wan_idx -lt $wan_ppp_Count ]
		do
			eval wan_name='$'wanppp_${wan_idx}_connName
			if [ "A$PWB_WANCON" = "A$wan_name" ]; then
				wan_flg=1
				. /etc/rc.d/get_wan_if $wan_idx "ppp"
				eval WAN_IF=${WAN_IFNAME}
				eval WAN_MODE='$'wanppp_${wan_idx}_wanMode
				eval WAN_STATUS=`/usr/sbin/status_oper GET WanPPP${wan_idx}_IF_Info STATUS`
				eval WAN_GW=`/usr/sbin/status_oper GET WanPPP${wan_idx}_GATEWAY ROUTER1`
				break
			fi
			wan_idx=$(($wan_idx + 1))
		done
	fi

}

######################################################################################################################################### 	
# Internal function to configure the rules in the system 
######################################################################################################################################### 	
set_pwb_rule() {

	local ifname_sub=0
	local fw_mask=0xE0000000

	if [ "$PWB_NUMLAN" = "1" ]; then
		ifname_sub=$PWB_LANIFS
	else
		ifname_sub=`echo $PWB_LANIFS | sed -n 's,\,, ,gp'`
	fi
	
	#if the wan is in bridge mode
	if [ -n "$WAN_BRIDGE" ]; then
	
		# for each entry in LAN interfaces add the following rules
		for j in $ifname_sub
		do
			/usr/sbin/ebtables -t filter -A INPUT -d Broadcast -i $j -j ACCEPT
			/usr/sbin/ebtables -t filter -A FORWARD -i $j -o ! $WAN_IF -j DROP
			/usr/sbin/ebtables -t filter -A FORWARD -i $WAN_IF -o $j -j ACCEPT
		done
		
		# blocking DHCP client requests from WAN
		/usr/sbin/ebtables -t filter -A INPUT -p IPv4 -i $WAN_IF --ip-proto 17 --ip-sport 68 -j DROP
		/usr/sbin/ebtables -t filter -A FORWARD -p IPv4 -i $WAN_IF --ip-proto 17 --ip-dport 68 -j DROP
		/usr/sbin/ebtables -t filter -A FORWARD -i $WAN_IF -j DROP

	else
	#WAN is in routed mode

		# for each entry in LAN interfaces add the following rule
		for j in $ifname_sub
		do
			#mark the packets based on ingress interface
			/usr/sbin/iptables -t mangle -A IFX_MANGLE_POLICY_ROUTING -m physdev --physdev-in $j -j MARK --set-mark $PWB_NFMARK/$PWB_MASK
		done
		#add ip rule such that all the packets with one mark will go to the specified tableno
		
		echo ${PWB_TABLENO} ${PWB_TABLENAME} >> /ramdisk/etc/iproute2/rt_tables
		
		/usr/sbin/ip rule add fwmark $PWB_NFMARK/$fw_mask table $PWB_TABLENAME
		/usr/sbin/ip route flush table $PWB_TABLENAME
		if [ "$WAN_STATUS" = "CONNECTED" ]; then
			/usr/sbin/ip route add table $PWB_TABLENAME default via $WAN_GW
		else
		#add default rule as destination unreachable
			/usr/sbin/ip route add table $PWB_TABLENAME unreachable default 
		fi
		/usr/sbin/ip route flush cache

	fi
}

######################################################################################################################################### 	
#internal function to remove a PWB rule configured from system
######################################################################################################################################### 	
remove_pwb_rule() {
	local ifname_sub=0
	local fw_mask=0xE0000000

	if [ "$PWB_NUMLAN" = "1" ]; then
		ifname_sub=$PWB_LANIFS
	else
		ifname_sub=`echo $PWB_LANIFS | sed -n 's,\,, ,gp'`
	fi
	
	#if the wan is in bridge mode
	if [ -n "$WAN_BRIDGE" ]; then
	
		# for each entry in LAN interfaces add the following rules
		for j in $ifname_sub
		do
			/usr/sbin/ebtables -t filter -D INPUT -d Broadcast -i $j -j ACCEPT
			/usr/sbin/ebtables -t filter -D FORWARD -i $j -o ! $WAN_IF -j DROP
			/usr/sbin/ebtables -t filter -D FORWARD -i $WAN_IF -o $j -j ACCEPT
		done
		
		# blocking DHCP client requests from WAN
		/usr/sbin/ebtables -t filter -D INPUT -p IPv4 -i $WAN_IF --ip-proto 17 --ip-sport 68 -j DROP
		/usr/sbin/ebtables -t filter -D FORWARD -p IPv4 -i $WAN_IF --ip-proto 17 --ip-dport 68 -j DROP
		/usr/sbin/ebtables -t filter -D FORWARD -i $WAN_IF -j DROP

	else
	#WAN is in routed mode

		# for each entry in LAN interfaces add the following rule
		for j in $ifname_sub
		do
			#mark the packets based on ingress interface
			/usr/sbin/iptables -t mangle -D IFX_MANGLE_POLICY_ROUTING -m physdev --physdev-in $j -j MARK --set-mark $PWB_NFMARK/$PWB_MASK
		done
		/usr/sbin/ip route flush table $PWB_TABLENO
		/usr/sbin/ip rule del fwmark $PWB_NFMARK/$fw_mask table $PWB_TABLENO
		cat /ramdisk/etc/iproute2/rt_tables | /etc/rc.d/del_line "$PWB_TABLENO" /ramdisk/etc/iproute2/rt_tables
		/usr/sbin/ip route flush cache

	fi
}


#########################################################################################################################################
# function : sighup_mcastd
# Arguement : none
# This function sends a sighup to mcastd process so to indicate a change in the configuration
#########################################################################################################################################
sighup_mcastd() {
	local mcastd_pid=$(grep mcastd /proc/*/stat|cut -d: -f2|awk '{ print $1 }')
	kill -HUP $mcastd_pid
}

#########################################################################################################################################
# function : update_mcast_ports
# Arguement : none
# This function updates the multicast section with the lan port names which needs to be excluded from the multicast proxy function
######################################################################################################################################### 	
update_mcast_ports() {
	# Read the multicast section to see proxy is enabled
	local mcast_proxy=$mcast_igmp_proxy_status
	if [ "$EN_PWB" = "1" -a "$mcast_proxy" = "1" ]; then
		local i=0
		local pwb_status="0"
		#read proxy wans
		local proxywans=$mcast_upstream_wan
		local block_ports=""
		local wan_match="0"

		. /etc/init.d/get_wan_mode $wanphy_phymode $wanphy_tc
	 	local active_wan="$wanMode"

		#for each entry in port binding
		while [ $i -lt $port_wan_binding_Count ] 
		do
			if [ "$#" = "1" ]; then
			# arguement passed is the index of pwb to be skipped
				if [ "$i" = "$1" ]; then
					i=`expr $i + 1`
					continue
				fi
			fi
			wan_match="0"
			eval pwb_status='$'pwb_${i}_pwbEnable	
			if [ "$pwb_status" = "1" ];then
				read_all_params $i

				if [ "$active_wan" = "$WAN_MODE" ]; then
					if [ -n "$WAN_BRIDGE" ]; then
					#if it is bridge wan add lan interfaces to list
						wan_match="0"	
					else
					#else check pwb wan and any of the proxy wan matches
						for wan_if in $( echo $proxywans | sed -n 1'p' | tr ',' '\n'); do	
							if [ "$wan_if" = "$PWB_WANCON" ]; then
								wan_match="1"
								break
							fi	
						done
					fi

					#if not add the lan interfaces to the list
					if [ $wan_match = "0" ]; then
						if [ "A${block_ports}A" = "AA" ]; then
							block_ports=$PWB_LANIFS
						else
							block_ports=$block_ports","$PWB_LANIFS
						fi	
					fi
				fi
			fi			
			i=`expr $i + 1`
		done
		if [ "A${block_ports}A" != "AA" ]; then
			/usr/sbin/status_oper -u -f /flash/rc.conf SET mcast_proxy_snooping mcast_interface_filter "$block_ports"
			/usr/sbin/status_oper -u -f /flash/rc.conf SET mcast_proxy_snooping mcast_interface_filter_status "1"
		else
			/usr/sbin/status_oper -u -f /flash/rc.conf SET mcast_proxy_snooping mcast_interface_filter_status "0"
			/usr/sbin/status_oper -u -f /flash/rc.conf SET mcast_proxy_snooping mcast_interface_filter ""
		fi
	else
		/usr/sbin/status_oper -u -f /flash/rc.conf SET mcast_proxy_snooping mcast_interface_filter_status "0"
                /usr/sbin/status_oper -u -f /flash/rc.conf SET mcast_proxy_snooping mcast_interface_filter ""
	fi 
        . /etc/rc.d/backup
}

	
######################################################################################################################################### 	
# function : add_pwb_rule
# Arguement: index of the port binding rule to be added
# *It will add the portbinding rule to the system IFF 
#	1. The WANMODE of the rule is same as the active WANPHY MODE or
# 	2. DUAL_WAN is enabled in HYBRID WAN MODE and WANMODE of the rule is equal to that of primary or secondary WAN.
# *Once this is done all the traffic from the specified LAN interfaces will be directed to the corresonding routing table
# default rule of the routing table will be DROP
######################################################################################################################################### 	
add_pwb_rule() {
	
	if [ "$1" = "-1" ]; then
		echo "!!!!!!!!!!Wrong arguments!!!!!!!!!!!!!!!"
		echo "usage ltq_pwb_config.sh [add|delete|add_all|del_all|ip_add_route|ip_del_route|ppp_add_route|ppp_del_route|update_mcast_conf] {index}"
		return
	else
		eval pwben='$'pwb_${1}_pwbEnable
		if [ "$EN_PWB" = "1" -a "$pwben" = "1" ]; then
			read_all_params $1
			# checking dual wan in hybrid mode
			if [ "$CONFIG_FEATURE_DUAL_WAN_SUPPORT" = "1" -a  "$dw_failover_state" = "1" -a "$dw_standby_type" = "2" ]; then
				. /etc/init.d/get_wan_mode $dw_pri_wanphy_phymode $dw_pri_wanphy_tc
				if [ "$WAN_MODE" = "$wanMode" ]; then
					#echo "add_pwb_rule"$1 $WAN_IF $WAN_MODE >> /tmp/pwb_debug 
					set_pwb_rule 
				else 
					. /etc/init.d/get_wan_mode $dw_sec_wanphy_phymode $dw_sec_wanphy_tc
					if [ "$WAN_MODE" = "$wanMode" ]; then
						#echo "add_pwb_rule"$1 $WAN_IF $WAN_MODE >> /tmp/pwb_debug 
						set_pwb_rule
					fi
				fi
			else
				. /etc/init.d/get_wan_mode $wanphy_phymode $wanphy_tc
				if [ "$WAN_MODE" = "$wanMode" ]; then
					#echo "add_pwb_rule"$1 $WAN_IF $WAN_MODE >> /tmp/pwb_debug 
					set_pwb_rule
				fi
			fi
		fi
	fi
}

######################################################################################################################################### 	
# Function: del_pwb_rule
# Arguement : index of the pwb rule
# This will remove the port binding rule configured in the system.
######################################################################################################################################### 	
del_pwb_rule()
{
	if [ "$1" = "-1" ]; then
                echo "!!!!!!!!!!Wrong arguments!!!!!!!!!!!!!!!"
                echo "usage ltq_pwb_config.sh [add|delete|add_all|del_all|ip_add_route|ip_del_route|ppp_add_route|ppp_del_route|update_mcast_conf] {index}"
                return
        else
		eval pwben='$'pwb_${1}_pwbEnable
		if [ "$pwben" = "1" ]; then
			read_all_params $1
			# checking dual wan in hybrid mode
			if [ "$CONFIG_FEATURE_DUAL_WAN_SUPPORT" = "1" -a  "$dw_failover_state" = "1" -a "$dw_standby_type" = "2" ]; then
				. /etc/init.d/get_wan_mode $dw_pri_wanphy_phymode $dw_pri_wanphy_tc
				if [ "$WAN_MODE" = "$wanMode" ]; then
					#echo "del_pwb_rule"$1 $WAN_IF $WAN_MODE >> /tmp/pwb_debug 
					remove_pwb_rule
				else 
					. /etc/init.d/get_wan_mode $dw_sec_wanphy_phymode $dw_sec_wanphy_tc
					if [ "$WAN_MODE" = "$wanMode" ]; then
						#echo "del_pwb_rule"$1 $WAN_IF $WAN_MODE >> /tmp/pwb_debug 
						remove_pwb_rule
					fi
				fi
			else
				. /etc/init.d/get_wan_mode $wanphy_phymode $wanphy_tc
				if [ "$WAN_MODE" = "$wanMode" ]; then
					#echo "del_pwb_rule"$1 $WAN_IF $WAN_MODE >> /tmp/pwb_debug 
					remove_pwb_rule
				fi
			fi
		fi
	fi	
}

######################################################################################################################################### 	
# Function: add_all_pwb_rules
# This will add all the portbinding rules.
# 1. During system bootup 
# 2. During WAN mode Switch 
# 3. when the portbinding feature is enabled from top level.
######################################################################################################################################### 	
add_all_pwb_rules() {
	
	local i=0
	#if file does not exist
	if [ "$EN_PWB" = "1" ]; then
	    [ -f /tmp/pwb_status ] || {
		/usr/sbin/iptables -t mangle -A IFX_MANGLE_POLICY_ROUTING -j MARK --set-mark 0x0/$PWB_MASK
		while [ $i -lt $port_wan_binding_Count ] 
		do
			add_pwb_rule $i
			i=`expr $i + 1`
		done
		echo "enabled" > /tmp/pwb_status
	    } && true
	fi
}

######################################################################################################################################### 	
# Function: del_all_pwb_rules
# This will delete all the port binding rules from the system.
# 1. During WAN mode Switch 
# 2. When Port binding feature is disabled from the top level
######################################################################################################################################### 	
del_all_pwb_rules() {
	
	local i=0
	#if file exists
	[ -f /tmp/pwb_status ] && {
		/usr/sbin/iptables -t mangle -D IFX_MANGLE_POLICY_ROUTING -j MARK --set-mark 0x0/$PWB_MASK
		i=$port_wan_binding_Count
		while [ $i -ge 0 ] 
		do
			del_pwb_rule $i
			i=`expr $i - 1`
		done
		rm -f /tmp/pwb_status
	} || true

}

######################################################################################################################################### 	
# Function : add_pwb_route
# This function will add default gw for the routing table.
# Will be called when the WAN interface comes up
######################################################################################################################################### 	
set_pwb_route() {

	local wan_idx=$3
	local i=0
	local wancon="none"
	local wangw="none"
	local pwb_wancon="none"
	local pwb_enable=0
	local pwb_tablenname="none"

	if [ "$EN_PWB" = "1" ]; then
		# read the connection name and interface name from the wan index passed
		if [ "$1" = "PPP" ]; then
			eval wancon='$'wanppp_${wan_idx}_connName
			eval wangw=`/usr/sbin/status_oper GET WanPPP${wan_idx}_GATEWAY ROUTER1`
		elif [ "$1" = "IP" ]; then
			eval wancon='$'wanip_${wan_idx}_connName
			eval wangw=`/usr/sbin/status_oper GET WanIP${wan_idx}_GATEWAY ROUTER1`
		fi

		# for each pwb entry check whether the connectioname matches and entry is enabled
		while [ $i -lt $port_wan_binding_Count ]
        	do
			eval pwb_wancon='$'pwb_${i}_wanConName
			eval pwb_enable='$'pwb_${i}_pwbEnable
			if [ "$wancon" = "$pwb_wancon" -a "$pwb_enable" = "1" ]; then
				pwb_tablename=${pwb_wancon}"_table"
				/usr/sbin/ip route flush table $pwb_tablename
				if [ "$2" = "ADD" ]; then
					/usr/sbin/ip route add table $pwb_tablename default via $wangw
				elif [ "$2" = "DEL" ]; then
					/usr/sbin/ip route add table $pwb_tablename unreachable default 
				fi		
				/usr/sbin/ip route flush cache
				break
			fi
			i=`expr $i + 1`
		done
	fi
			
}

######################################################################################################################################### 	
# Call functions based on the inputs 
######################################################################################################################################### 	
case "$SEL_OPT" in
	add)
		add_pwb_rule $SEL_IDX
		if [ "$CONFIG_FEATURE_LTQ_MCAST_FILTER_PORT" = "1" ]; then 
			update_mcast_ports
			sighup_mcastd
		fi
	;;
	delete)
		if [ "$EN_PWB" = "1" ]; then
			del_pwb_rule $SEL_IDX
			if [ "$CONFIG_FEATURE_LTQ_MCAST_FILTER_PORT" = "1" ]; then 
				update_mcast_ports $SEL_IDX
				sighup_mcastd
			fi
		fi
	;;
	add_all)
		add_all_pwb_rules
		if [ "$CONFIG_FEATURE_LTQ_MCAST_FILTER_PORT" = "1" ]; then 
			update_mcast_ports
			sighup_mcastd
		fi
	;;
	del_all)
		del_all_pwb_rules
		if [ "$CONFIG_FEATURE_LTQ_MCAST_FILTER_PORT" = "1" ]; then 
			update_mcast_ports
			sighup_mcastd
		fi
	;;
	ip_add_route)
		set_pwb_route "IP" "ADD" $SEL_IDX
	;;
	ip_del_route)
		set_pwb_route "IP" "DEL" $SEL_IDX
	;;
	ppp_add_route)
		set_pwb_route "PPP" "ADD" $SEL_IDX
	;;
	ppp_del_route)
		set_pwb_route "PPP" "DEL" $SEL_IDX
	;;
	update_mcast_conf)
		update_mcast_ports
	;;
	*)
		echo "!!!!!!!!!!Wrong arguments!!!!!!!!!!!!!!!"
		echo "usage ltq_pwb_config.sh [add|delete|add_all|del_all|ip_add_route|ip_del_route|ppp_add_route|ppp_del_route|update_mcast_conf] {index}"
esac


