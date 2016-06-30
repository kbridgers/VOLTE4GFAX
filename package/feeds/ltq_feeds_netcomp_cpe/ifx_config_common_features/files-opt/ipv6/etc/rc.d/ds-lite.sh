#!/bin/sh
#echo "ds-lite.sh:$$ $*" >> /tmp/debug.log

DSLITE_BASEINTF="/tmp/.dslitebase"
DSLITE_LOCK="/tmp/dslite.lock"
dslite_lock()
{
	if [ -f ${DSLITE_LOCK} ]; then
		echo "failed"
	else
		echo $$ > ${DSLITE_LOCK}	
		echo "success"
	fi

}
dslite_lock_exit()
{
	if [ "$(dslite_lock)" == "failed" ]; then
		if [ -d /proc/$(cat ${DSLITE_LOCK} 2>/dev/null) ]; then
			echo "$0 Already used by PID $(cat ${DSLITE_LOCK})"
			exit 1
		fi
	fi
}
dslite_unlock()
{

	rm -f ${DSLITE_LOCK}	
}



SED="/bin/sed"

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


usage()
{
cat << EOF
usage: $0 options

This script starts or stops DS-lite tunnel on a specified wan interface over a machine.

OPTIONS:
   -h      Show this message
   -o      operation, can be start or stop. This option is mandatory.
   -i      wan index on which the tunnel will be created or deleted. This option is mandatory.
   -r      remote(AFTR) IPv6 address. This otion is necessary to create tunnel.
   -l      local IPv6 address. This otion may be specified to create tunnel.
   -a      ds-lite tunnel interface address(IPv4). This otion is necessary to create tunnel.
   -p      netmask of the ds-lite tunnel interface address(IPv4). This otion is necessary to create tunnel.
   -m      MTU(OPTIONAL). NOTE: MTU=1280 is recommended while connecting to Internet.
           Otherwise to get default MTU, leave this field blank.     

Example:
To create a tunnel:
usage: $0 -o start -i 1 -r 2001:660::1 -a 192.0.0.2 -p 255.255.255.248

To stop a tunnel:
usage: $0 -o stop -i 1

EOF
exit 1;
}


dslite_operation=
dslite_wan_index=
dslite_remote_ip=
dslite_local_ip=
dslite_tunintf_addr=
dslite_netmask=
dslite_MTU=
dslite_TUNTTL=255
ret100=1
ret200=1
ret300=1
ret400=1
dslite_wantype=""
dslite_windex=
dslite_TUNIF="dsltun0"

get_type()
{
        echo $1 | grep -i PPP 2>&1 > /dev/null
	if [ $? -eq 0 ]; then
		dslite_wantype="ppp"
		dslite_windex=$(echo $1 | sed 's/WANPPP//g')
	else
		dslite_wantype="ip"
		dslite_windex=$(echo $1 | sed 's/WANIP//g')
	fi
}

get_index()
{
        ifname=$1
	get_type $1
		
        for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
        do
                . /etc/rc.d/get_wan_if  $i ${dslite_wantype}
                if [ "${WAN_IFNAME}z" = "${ifname}z" ]; then
                        echo $i
                        break
                fi
        done
	dslite_windex=$i
}

get_remoteaddr()
{
RESOLVED_ADDR_FILE="/tmp/dyn_dslite_info_resolved"
DSLITE_DOMAINNAME="/tmp/dyn_dslite_info"
	if [ $2 == "ppp" 2> /dev/null ]; then
		IPv6TAG="WanPPP${1}_IF_IPv6_Info"
	else
		IPv6TAG="WanIP${1}_IF_IPv6_Info"
	fi
#	aftr=$(/usr/sbin/status_oper GET ${IPv6TAG} AFTR_addr)
	aftr=$(cat ${DSLITE_DOMAINNAME})
	resolved_aftr=$(cat ${RESOLVED_ADDR_FILE} | cut -f1 -d' ')
	resolved_aftr_addr=$(cat ${RESOLVED_ADDR_FILE} | cut -f2 -d' ')

	if [ "$resolved_aftr" == "$aftr" -a ! -z ${resolved_aftr_addr} ]; then
		echo ${resolved_aftr_addr}	
	elif [ "$aftr" != ""  ]; then
		resolved_addr=$(nslookup $aftr | awk '/Address 1:/ {print $3}' | grep ':')
		if [ ! -z $resolved_addr 2>/dev/null ]; then
			echo "${aftr} ${resolved_addr}" > ${RESOLVED_ADDR_FILE}
		fi
		echo "${resolved_addr}"
	fi
}

# args $1 interface name, $2 wan index, $3 wan type(ip/ppp)
#  mode slaac(1)/statefull(0)
get_global_addr()
{
       
	eval wan_mode='$'wan${3}_${2}_dhcpv6State
	case $wan_mode in
	1) #slaac
		ip -6 addr show dev $1 | awk '/global dynamic/ {print $2}' | tail -1 | cut -f1 -d'/' 2>/dev/null
		;;
	0) #statefull
		ip -6 addr show dev $1 | awk '/global/ {print $2}' | tail -1| cut -f1 -d'/' 2>/dev/null
		;;
	esac

}


start_tunnel()
{
	echo "Starting DS-lite tunnel"
	insmod /lib/modules/*/tunnel6.ko 2>/dev/null
	insmod /lib/modules/*/ip6_tunnel.ko 2>/dev/null
	sleep 1
	#asdf 2>&1 > /dev/null

        eval dslite_portrange='$'wan_portrange
        eval dslite_mode='$'wan_dslite_mode
        if [ "$dslite_mode" = "2" -o "$dslite_mode" = "3" ]; then
           lp=$(echo $dslite_portrange'-' | cut -d- -f1)
           up=$(echo $dslite_portrange'-' | cut -d- -f2)
           echo $lp | grep -E '^-?[0-9]*\.?[0-9]*$' 2>/dev/null
           if [ $? -ne 0 ]; then
              exit 25;
           fi

           echo $up | grep -E '^-?[0-9]*\.?[0-9]*$' 2>/dev/null
           if [ $? -ne 0 ]; then
              exit 25;
           fi
           if [ -z $up ]; then
              exit 25;
           fi
           if [ $lp -lt "0" -o $up -gt "65535" -o $up -lt $lp ]; then
              exit 25;
           fi
        fi

	dslite_addr=$(/usr/sbin/ipv6helper -t caddr6 -r $dslite_remote_ip)
	dslite_ret1=$?
	if [ "$dslite_ret1" = "0" ]; then	
		if [ ! -z "$dslite_local_ip" ]; then
			addr6=$(/usr/sbin/ipv6helper -t caddr6 -r $dslite_local_ip)
			dslite_ret2=$?
			if [ "$dslite_ret2" = "0" ]; then
				ip -6 tunnel add $dslite_TUNIF mode ipip6 ttl $dslite_TUNTTL remote $dslite_remote_ip  local $dslite_local_ip
				echo "$dslite_local_ip" > /tmp/dslite_tun${dslite_wan_index}
			else
				echo "Invalid Ds-lite local ipv6 address. Please validate inpute and try again"
				exit 1;
			fi
		else
			. /etc/rc.d/get_wan_if $dslite_windex $dslite_wantype
			dslite_WAN=$WAN_IFNAME

			if [ ! -z "$dslite_WAN" ]; then
				#sleep 5
				dslite_local_ipv6=$(/usr/sbin/ipv6helper -t dslite -w $dslite_WAN -r $dslite_remote_ip)
				dslite_ret3=$?
				if [ "$dslite_ret3" != "0" ]; then # Sometimes /usr/sbin/ipv6helper returns err if default gw is absent
					dslite_local_ipv6=$(get_global_addr $dslite_WAN $dslite_windex $dslite_wantype)
					[ ! -z "$dslite_local_ipv6" ] && dslite_ret3=0
				fi
				if [ "$dslite_ret3" = "0" ]; then
					dslite_cglobal=$(ip -6 addr show dev $dslite_WAN | grep $dslite_local_ipv6 | awk '{print $4}')
					if [ "$dslite_cglobal" = "global" ]; then
						dslite_check_ifname=$(ip -6 addr show dev $dslite_WAN | grep $dslite_local_ipv6)
						if [ "$dslite_check_ifname" = "" ]; then
							echo "Remote address matches with the prefix of another WAN Index having address $dslite_local_ipv6"
							exit 2;
						else

							if [ "$dslite_WAN" != "$(cat ${DSLITE_BASEINTF} 2>/dev/null)" ]; then
								#delete old tunnel if exists
								stop_old
								sleep 1
							fi
							ip -6 tunnel add $dslite_TUNIF mode ipip6 ttl $dslite_TUNTTL remote $dslite_remote_ip  local $dslite_local_ipv6
							ret100=$?
							echo "$dslite_local_ipv6" > /tmp/dslite_tun${dslite_wan_index}
						fi
					else
						dslite_local_addr6=$(get_global_addr $dslite_WAN $dslite_windex $dslite_wantype)
						if [ ! -z "$dslite_local_addr6" ] ; then

							if [ "$dslite_WAN" != "$(cat ${DSLITE_BASEINTF} 2>/dev/null)" ]; then
								#delete old tunnel if exists
								stop_old
								sleep 1
							fi
							ip -6 tunnel add $dslite_TUNIF mode ipip6 ttl $dslite_TUNTTL remote $dslite_remote_ip \
							  local $dslite_local_addr6
							ret100=$?
							echo "$dslite_local_addr6" > /tmp/dslite_tun${dslite_wan_index}
						else
							echo "IPv6 address is not configured on that interface or Worng remote address(Link local address) or remote address matches with the prefix of another WAN connection. Please validate user input and try again"
							exit 3;
						fi
					fi
				else
					echo "Please validate user input and try again"
					exit 1;
				fi
			else
				echo "Invalid WAN index. Please validate user input and try again"
				exit 4;
			fi
		fi
		if [ ! -z "$dslite_MTU" ]; then
			ifconfig $dslite_TUNIF mtu $dslite_MTU
		fi
		ifconfig $dslite_TUNIF $dslite_tunintf_addr netmask $dslite_netmask up
		ret200=$?
		ppacmd addwan -i $dslite_TUNIF
		ret400=$?
		route add default dev $dslite_TUNIF
		ret300=$?
		if [ "$ret100" = "0" ]; then
			echo " $dslite_WAN" > $DSLITE_BASEINTF # store base intf info of current tunnel

			if [ "$ret200" = "0" ]; then
				echo "connected $dslite_tunintf_addr $dslite_netmask $dslite_remote_ip" > /tmp/dslite_tun${dslite_wan_index}_status
			else
				dsl_netmask=$(ifconfig $dslite_TUNIF | grep 'Mask' | awk '{print $4}' | sed 's/Mask://')
				if [ "$?" = "0" ]; then
					echo "connected $dslite_tunintf_addr $dsl_netmask $dslite_remote_ip" > /tmp/dslite_tun${dslite_wan_index}_status
				else
					echo "connected $dslite_tunintf_addr Undefined $dslite_remote_ip" > /tmp/dslite_tun${dslite_wan_index}_status
				fi
			fi
		else
			echo "Not_Connected Undefined Undefined Undefined" > /tmp/dslite_tun${dslite_wan_index}_status
			exit 6; 
		fi
	else
		echo "Invalid Ds-lite remote ipv6 address. Please validate inpute and try again"
		exit 5;
	fi
        eval dslite_mode='$'wan_dslite_mode
        if [ "$dslite_mode" = "2" -o "$dslite_mode" = "3" ]; then
        . /etc/rc.d/stateless_dslite.sh
        fi
	return;
}


stop_tunnel()
{
	echo "Stoping DS-lite tunnel"
	ppacmd delwan -i $dslite_TUNIF
	ifconfig $dslite_TUNIF down
	ip tunnel del $dslite_TUNIF
	echo "" > /tmp/dslite_tun${dslite_wan_index}
	echo "" > /tmp/dslite_tun${dslite_wan_index}_status
	return ;
}
stop_old()
{
	TMP_FILE="/tmp/.tmp.$$"
	echo "Stoping old DS-lite tunnel"
	echo "" > ${TMP_FILE}
	ppacmd delwan -i $dslite_TUNIF
	ifconfig $dslite_TUNIF down
	ip tunnel del $dslite_TUNIF
	find /tmp/dslite_tun* |xargs cp ${TMP_FILE}
	rm -f ${TMP_FILE}
	. /etc/rc.d/stateless_dslite.sh stop
	
}
case $1 in
dhcp6c_restart)
		. /etc/rc.d/get_wan_if  $3 ${4}
                if [ "${WAN_IFNAME}z" != "z" ]; then
			. /etc/rc.d/create_and_run_dhcp6c_cfg stop ${WAN_IFNAME} $3 $4
			stop_old
			sleep 1
			. /etc/rc.d/create_and_run_dhcp6c_cfg start ${WAN_IFNAME} $3 $4
                fi
                eval dslite_mode='$'wan_dslite_mode
                if [ "$dslite_mode" = "2" -o "$dslite_mode" = "3" ]; then
                . /etc/rc.d/stateless_dslite.sh
                fi
		exit 0
		;;
esac


if [ "$1" = "update_slaac" -o "$1" = "update_stateful" ]; then
	if [ "$1" = "update_slaac" ]; then
		dslite_WAN_internal=$2
		WAN_IDX=$(get_index $2)
		dslite_windex=$WAN_IDX
		echo $2 | grep -i ppp  2>&1 > /dev/null
		if [ $? -eq 0 ]; then
                        dslite_wantype="ppp"
                else
                        dslite_wantype="ip"
                fi

		eval wan_ipv6_dhcpv6state='$'wan${dslite_wantype}_${WAN_IDX}_dhcpv6State
		if [ "$wan_ipv6_dhcpv6state" != "1" -a "$3" != "dynamic" ]; then
			exit 0;
		fi
	else
		WAN_IDX=$(get_index $3)
		dslite_windex=$WAN_IDX
		echo $3 | grep -i ppp  2>&1 > /dev/null
		if [ $? -eq 0 ]; then 
			dslite_wantype="ppp"
		else
			dslite_wantype="ip"
		fi
		eval wan_ipv6_dhcpv6state='$'wan${dslite_wantype}_${WAN_IDX}_dhcpv6State
		dslite_WAN_internal=$3
		current_ipv6_addr=$4
		if [ "$wan_ipv6_dhcpv6state" != "0" ]; then
			exit 0;
		fi
	fi
        eval wan_dslite='$'wan${dslite_wantype}_${WAN_IDX}_tunnel
        eval pwan_ipv6='$'wan${dslite_wantype}_${WAN_IDX}_ipv6
        eval wan_ipv6_dhcpv6state='$'wan${dslite_wantype}_${WAN_IDX}_dhcpv6State
	new_wanname=""

	if [ $dslite_wantype == "ppp" 2> /dev/null ]; then
		new_wanname="WANPPP${WAN_IDX}"
	else
		new_wanname="WANIP${WAN_IDX}"
	fi

	if [ "$wan_dslite_mode" == "1" ]; then #dynamic configuration
		dslite_lock_exit	
		wan_dsliteremoteipv6=$(get_remoteaddr $WAN_IDX $dslite_wantype)
		dslite_unlock
	fi

        if [ "$wan_dslite" = "2" -o "$wan_dslite" = "3" ]; then
        	if [ "$ipv6_status" = "1" -a $wan_dslitewanidx = $new_wanname  -a "$pwan_ipv6" = "2" ]; then

                	is_dslite=$(ifconfig $dslite_TUNIF)
                      	if [ -z $is_dslite ]; then
							 
                            	if [ "$wan_dslite_mtu" = "0" ]; then
                                       	/etc/rc.d/ds-lite.sh -o start -i ${new_wanname} -r $wan_dsliteremoteipv6 -a $wan_dslitetunip -p $wan_dslitemask
                                else
                                      	/etc/rc.d/ds-lite.sh -o start -i ${new_wanname} -r $wan_dsliteremoteipv6 -a $wan_dslitetunip -p $wan_dslitemask -m $wan_dslite_mtu
                                fi
                        else
	                        prev_dslite_base_intf_addr=$(cat /tmp/dslite_tun${WAN_IDX})
				if [ "$1" = "update_slaac" ]; then
                                        sleep 1
                                        preferable_dslite_base_intf_addr=$(/usr/sbin/ipv6helper -t dslite -w $dslite_WAN_internal -r $wan_dsliteremoteipv6)
                                        ret_internal=$?

                                        if [ "$ret_internal" = "0" ]; then
                                                if [ "$prev_dslite_base_intf_addr" != "$preferable_dslite_base_intf_addr" -o "$prev_dslite_base_intf_addr" = ""  ]; then
                                                        if [ -z "$preferable_dslite_base_intf_addr" ]; then
                                                                        /etc/rc.d/ds-lite.sh -o start -i ${new_wanname} -r $wan_dsliteremoteipv6 -a $wan_dslitetunip -p $wan_dslitemask
                                                                if [ "$wan_dslite_mtu" = "0" ]; then
                                                                        /etc/rc.d/ds-lite.sh -o start -i $WAN_IDX -r $wan_dsliteremoteipv6 -a $wan_dslitetunip -p $wan_dslitemask
                                                                else
                                                                        /etc/rc.d/ds-lite.sh -o start -i ${new_wanname} -r $wan_dsliteremoteipv6 -a $wan_dslitetunip -p $wan_dslitemask -m $wan_dslite_mtu
                                                                fi
                                                        fi
                                                fi
                                        fi
        	                elif [ "$1" = "update_stateful" ]; then
					if [ "$prev_dslite_base_intf_addr" != "$current_ipv6_addr" -o "$prev_dslite_base_intf_addr" = "" ]; then
						sleep 1
						preferable_dslite_base_intf_addr=$(/usr/sbin/ipv6helper -t dslite -w $dslite_WAN_internal -r $wan_dsliteremoteipv6)
						ret_internal=$?
						if [ "$ret_internal" = "0" ]; then
							if [ "$prev_dslite_base_intf_addr" != "$preferable_dslite_base_intf_addr" -o "$prev_dslite_base_intf_addr" = ""  ]; then
								if [ -z "$preferable_dslite_base_intf_addr" ]; then
                	        					/etc/rc.d/ds-lite.sh -o stop -i $WAN_IDX
                        	        				if [ "$wan_dslite_mtu" = "0" ]; then
                                						/etc/rc.d/ds-lite.sh -o start -i $WAN_IDX -r $wan_dsliteremoteipv6 -a $wan_dslitetunip -p $wan_dslitemask
                                					else
                                						/etc/rc.d/ds-lite.sh -o start -i $WAN_IDX -r $wan_dsliteremoteipv6 -a $wan_dslitetunip -p $wan_dslitemask -m $wan_dslite_mtu
                                					fi
								fi
							fi
						fi
                        		fi
				fi
                	fi
        	fi
	fi
	exit 0;
fi

arg=$(getopt ho:i:r:l:a:p:m: "$@")
dslite_ret5=$?
if [ "$dslite_ret5" = "0" ]; then
	set -- $arg
	if [ $? -ne 0 ]; then
		usage
		exit 1;
	fi
#while getopts "ho:i:r:l:a:p:m:v" OPTION
	for o
	do
		case $o in
			-h)		
				usage
				shift
				exit 
				;;
			-o)
				shift
				dslite_operation=$1
				shift
				;;
			-i)
				shift
				dslite_wan_index=$1
				get_type $dslite_wan_index
				shift
				;;
			-r)
				shift
				dslite_remote_ip=$1
				shift
				;;
			-l)
				shift
				dslite_local_ip=$1
				shift
				;;
			-a)
				shift
				dslite_tunintf_addr=$1
				shift
				;;
			-p)
				shift
				dslite_netmask=$1
				shift
				;;
			-m)
				shift
				dslite_MTU=$1
				shift
				;;			
			--)
				shift
				break;
				;;
		esac
	done
else
	echo "Please validate input."
	usage
	exit 1;
fi


#if [ -z $dslite_operation  -o -z $dslite_wan_index ]; then
if [[ -z $dslite_operation ]] || [[ -z $dslite_wan_index ]]
then
	usage
	exit 1;
else
	if [ "$dslite_operation" = "start" ]; then
		if [[ -z $dslite_remote_ip ]] || [[ -z $dslite_tunintf_addr ]] || [[ -z $dslite_netmask ]]
		then
			usage
			exit 1;
		else
			start_tunnel
		fi
	elif [ "$dslite_operation" = "stop" ]; then
		stop_tunnel
	else
		usage
		exit 1;
	fi
fi

