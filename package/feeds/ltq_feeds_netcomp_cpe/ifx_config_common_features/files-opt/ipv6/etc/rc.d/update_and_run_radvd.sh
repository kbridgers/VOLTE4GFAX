#!/bin/sh
DATABASE="/tmp/database"
BACKUP_DATABASE="/tmp/database.lastknownworking"
BUGGY_DATABASE="/tmp/database.buggy"
RADVDCONF="/var/radvd.conf"
RADVDPID="/var/run/radvd.pid"
LOGFILE="/var/log/messages"
TMPFILE_RDNSS="/tmp/.rdnss"
TMPFILE_ROUTEINFO="/tmp/.routeinfo"
OWNPIDFILE="/var/run/radvd_sc.pid"
RADVD_PD="/var/radvd_pd"
ETCSOLV="/etc/resolv.conf"
DELEGATED_PREFIX_FIREWALL="/etc/rc.d/firewall6 start filter delegated_prefix update"


zero_routerlife="yes"
max_rainterval=600
bridge_inf="br0"
lan_mode="SLDHCP"
ipv6_mtu=0
sixrd_in_use="false"
radvd_ret=0
reconfig_dhcpv6="yes"
IPV6_RETVAL=""

export PATH=$PATH:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

if [ -f $OWNPIDFILE -a -d /proc/$(cat $OWNPIDFILE 2> /dev/null) ]; then
	sleep 5 #for sync
fi

echo $$ > ${OWNPIDFILE}

touch $DATABASE

if [ "${INTF}" == "" ];then
	INTF="br0"
fi
if [ "${SRC_INTF}" == "" ];then
	SRC_INTF="none"
fi

store_args()
{
	ARG_INTF=${INTF} 
	ARG_PREFIX=${PREFIX}
	ARG_PLIFETIME=${PLIFETIME}
	ARG_VLIFETIME=${VLIFETIME} 
	ARG_EVENT_SRC=${EVENT_SRC} 
	ARG_EVENT_TYPE=${EVENT_TYPE} 
	ARG_SRC_INTF=${SRC_INTF}
	ARG_LMODE=${LMODE}
	ARG_DNS1=${DNS1}
	ARG_DNS2=${DNS2}
	ARG_IPV6_MTU=${IPV6_MTU}
	ARG_RLZERO=${RLZERO}
	ARG_DOMAIN_NAME6=$(echo ${DOMAIN_NAME6}|sed 's/ //g')
	ARG_DELEGATED_PREFIX=${DELEGATED_PREFIX}
	GREPSTR_PREFIX=`echo ${PREFIX} | cut -d"/" -f1`
}

store_args

reset_env()
{
	INTF=""
	PREFIX=""
	PLIFETIME=""
	VLIFETIME=""
	EVENT_SRC=""
	EVENT_TYPE=""
	SRC_INTF=""
	LMODE=""
	DNS1=""
	DNS2=""
	IPV6_MTU=""
	RLZERO=""
	DOMAIN_NAME6=""
	DELEGATED_PREFIX=""
}
gettime()
{
	cat /proc/uptime | cut -d'.' -f1
}

backup_database()
{
	cp -f $DATABASE $BACKUP_DATABASE
}
restore_database()
{
	echo "WARNING: ipv6: trying to restore database" >> $LOGFILE
	cp -f $DATABASE $BUGGY_DATABASE
	cp -f $BACKUP_DATABASE $DATABASE
}

add_newprefix()
{
	if [  "${ARG_PREFIX}" == "" ]; then
		echo "ipv6:$0 Wrong prefix" >> $LOGFILE
		return 
	fi
	echo ${ARG_DOMAIN_NAME6} | grep '[;="]' 2>&1 > /dev/null
	if [ $? -eq 0 ]; then
		echo "ipv6:Wrong domain name ${ARG_DOMAIN_NAME6}" >> $LOGFILE
		return
	fi
	ARG_PREFIX="$(echo ${ARG_PREFIX} | cut -d "/" -f 1 )/64"
	
	cat <<eof >> $DATABASE
INTF=${ARG_INTF}; \
PREFIX=${ARG_PREFIX}; \
PLIFETIME=${ARG_PLIFETIME}; \
VLIFETIME=${ARG_VLIFETIME}; \
EVENT_SRC=${ARG_EVENT_SRC}; \
EVENT_TYPE=${ARG_EVENT_TYPE}; \
SRC_INTF=${ARG_SRC_INTF}; \
TIME=$(gettime); \
LMODE=${ARG_LMODE}; \
DNS1=${ARG_DNS1}; \
DNS2=${ARG_DNS2}; \
IPV6_MTU=${ARG_IPV6_MTU}; \
RLZERO=${ARG_RLZERO}; \
DOMAIN_NAME6="${ARG_DOMAIN_NAME6}"; \
DELEGATED_PREFIX=${ARG_DELEGATED_PREFIX};
eof
}
update_database()
{
	if [  "$ARG_INTF" == "" -o "$ARG_EVENT_SRC" == "" ]; then
		return 
	fi

	if [ "z$ARG_EVENT_SRC" == "zWAN" -a "$ARG_SRC_INTF" == "" ]; then
		return
	fi 

	if [ "z$ARG_EVENT_SRC" == "zWAN" -a "z$ARG_SRC_INTF" == "znone" ]; then
		return
	fi 

	if [ "z$ARG_EVENT_TYPE" != "zUP" -a "z$ARG_EVENT_TYPE" != "zDOWN" ]; then
		return
	fi 

	if [ "$ARG_DNS1" == "::" ]; then
		ARG_DNS1=""
	fi

	if [ "$ARG_DNS2" == "::" ]; then
		ARG_DNS2=""
	fi
	backup_database
	case $ARG_EVENT_SRC in
	LAN)
		sed -i "/${ARG_INTF}.*EVENT_SRC=${ARG_EVENT_SRC}/ s/=UP/=DOWN/" $DATABASE #make everything DOWN 1st
		grep "${ARG_INTF}.*${GREPSTR_PREFIX}.*EVENT_SRC=${ARG_EVENT_SRC}" $DATABASE 2>&1 > /dev/null #already exists?
		if [ $? -eq 0 -a "z$GREPSTR_PREFIX" != "z" ]; then
			sed -i "/${ARG_INTF}.*${GREPSTR_PREFIX}.*EVENT_SRC=${ARG_EVENT_SRC}/d" $DATABASE #delete old entrie
		fi
		add_newprefix
		;;
	WAN)
		sed -i "/${ARG_INTF}.*EVENT_SRC=${ARG_EVENT_SRC}.*SRC_INTF=${ARG_SRC_INTF};/ s/=UP/=DOWN/" $DATABASE #make everything DOWN 1st
		grep "${ARG_INTF}.*${GREPSTR_PREFIX}.*EVENT_SRC=${ARG_EVENT_SRC}.*SRC_INTF=${ARG_SRC_INTF};" $DATABASE 2>&1 > /dev/null #already exists?
		if [ $? -eq 0 -a "z$GREPSTR_PREFIX" != "z" ]; then
			sed -i "/${ARG_INTF}.*${GREPSTR_PREFIX}.*EVENT_SRC=${ARG_EVENT_SRC}.*SRC_INTF=${ARG_SRC_INTF};/d" $DATABASE #delete old entrie
		fi
		add_newprefix
		;;
	sixrd)
		sed -i "/${ARG_INTF}.*EVENT_SRC=${ARG_EVENT_SRC}.*SRC_INTF=${ARG_SRC_INTF};/ s/=UP/=DOWN/" $DATABASE #make everything DOWN 1st
		grep "${ARG_INTF}.*${GREPSTR_PREFIX}.*EVENT_SRC=${ARG_EVENT_SRC}.*SRC_INTF=${ARG_SRC_INTF};" $DATABASE 2>&1 > /dev/null #already exists?
		if [ $? -eq 0 -a "z$GREPSTR_PREFIX" != "z" ]; then
			sed -i "/${ARG_INTF}.*${GREPSTR_PREFIX}.*EVENT_SRC=${ARG_EVENT_SRC}.*SRC_INTF=${ARG_SRC_INTF};/d" $DATABASE #delete old entrie
		fi
		add_newprefix

		;;
	esac
}

init_radvd_conf()
{
	> $RADVDCONF
	> $TMPFILE_RDNSS
	> $RADVD_PD
	> $TMPFILE_ROUTEINFO
}

init_conf_inf()
{
	cat <<eof >> $RADVDCONF
interface ${bridge_inf} {
	AdvLinkMTU 0;
	AdvSendAdvert on;
	AdvDefaultLifetime 0;
	AdvOtherConfigFlag on;
	AdvManagedFlag on;
	MaxRtrAdvInterval 100;
eof
	cat <<eof >> $TMPFILE_RDNSS
	RDNSS  { };
	DNSSL  { };
eof
}
init_etcresolv()
{
	touch ${ETCSOLV}
}

update_etcresolv()
{
	
	case $1 in
	UP)
		if [ "z$DNS1" != "z" ]; then
			grep -w $DNS1 ${ETCSOLV} 2>&1 > /dev/null
			if [ $? -ne 0 ]; then
				echo "nameserver $DNS1" >> ${ETCSOLV}
			fi
		fi
		if [ "z$DNS2" != "z" ]; then
			grep  -w $DNS2 ${ETCSOLV} 2>&1 > /dev/null
			if [ $? -ne 0 ]; then
				echo "nameserver $DNS2" >> ${ETCSOLV}
			fi
		fi
		if [ "z$DOMAIN_NAME6" != "z" ]; then
			
			grep -w $DOMAIN_NAME6 ${ETCSOLV} 2>&1 > /dev/null
			if [ $? -ne 0 ]; then
				echo "domain $DOMAIN_NAME6" >> ${ETCSOLV}
			fi
		fi
		;;
	*)
		;;
	esac

}

deprecated_prefix()
{
	echo "" > /dev/null
}

active_prefix()
{
	echo "" > /dev/null
}

add_prefix()
{
	autonomous="on"
	
	if [ ! -z $PLIFETIME -a ! -z $VLIFETIME  -a ! -z $PREFIX -a `expr $PLIFETIME + 1 2> /dev/null` -a `expr $VLIFETIME + 1 2> /dev/null` -a  $VLIFETIME -ge $PLIFETIME  ]; then
		echo "Adding prefix $PREFIX with plife=$PLIFETIME & vlife=$VLIFETIME" >> $LOGFILE
	else
		echo "prefix $PREFIX with plife=$PLIFETIME & vlife=$VLIFETIME is not valid" >> $LOGFILE
		return
	fi
	if [ "z$EVENT_TYPE" == "zDOWN" ]; then
		PLIFETIME=0
		VLIFETIME=0
		deprecated_prefix
	else 
		active_prefix
		if [ ! -z $DELEGATED_PREFIX ]; then
			cat <<eof >> $TMPFILE_ROUTEINFO
	route ${DELEGATED_PREFIX} {
		AdvRouteLifetime ${VLIFETIME};	
	};
eof
		fi
	fi

	if [ $VLIFETIME -ne 0 -a "z$lan_mode" == "zSLAAC" ] ; then
		sed -i "s/RDNSS/RDNSS $DNS1 $DNS2/" $TMPFILE_RDNSS
	fi
	if [ $VLIFETIME -ne 0 -a "z$lan_mode" == "zSLAAC" -a "z$DOMAIN_NAME6" != "z" ] ; then
		sed -i "s/DNSSL/DNSSL ${DOMAIN_NAME6}/" $TMPFILE_RDNSS
	fi
	if [ "z$LMODE" == "zSFDHCP" ]; then
		autonomous="off"
	fi
	
	cat <<eof >> $RADVDCONF
	prefix ${PREFIX} {
                AdvValidLifetime ${VLIFETIME};
                AdvPreferredLifetime ${PLIFETIME};
                AdvOnLink on;
                AdvAutonomous $autonomous;
        };
eof
}


finish_conf_inf()
{
	cat $TMPFILE_ROUTEINFO >> $RADVDCONF
	if [ "z$lan_mode" == "zSLAAC" ] ; then
		cat $TMPFILE_RDNSS >> $RADVDCONF
		sed -i "/RDNSS *{ };/d" $RADVDCONF
		sed -i "/DNSSL *{ };/d" $RADVDCONF
	fi
	echo "};" >> $RADVDCONF
}

intf_mtu=$(ifconfig ${bridge_inf} | grep -o "MTU:[0-9]*" | sed 's/MTU://')
record_mtu()
{
	if [ $IPV6_MTU -ge 1280 -a ${IPV6_MTU} -le ${intf_mtu} 2>/dev/null ]; then 
		if [  $ipv6_mtu  -eq 0 -o ${IPV6_MTU} -lt ${ipv6_mtu} 2>/dev/null ]; then
			ipv6_mtu=${IPV6_MTU}
		fi
	fi
}
update_lan_mode()
{
	l=`grep "EVENT_SRC=LAN.*EVENT_TYPE=UP" $DATABASE`
	if [ "$l" != "" ]; then 
		eval $l
		if [ $? -ne 0 -o "z$LMODE" != "zSFDHCP" -a "z$LMODE" != "zSLAAC" -a "z$LMODE" != "SLDHCP" ]; then
			lan_mode="SLDHCP"
		else
			lan_mode=$LMODE
		fi
		reset_env
	else
		 lan_mode="SLDHCP"
	fi

}
record_rlife()
{
	if [ "z$RLZERO" == "z0" ];then
		zero_routerlife="no"
	fi
}
get_max_RAinterval()
{
	if [ $PLIFETIME -gt 4 2>/dev/null ]; then
		if [ $PLIFETIME -lt $max_rainterval 2>/dev/null ]; then
			max_rainterval=$PLIFETIME
		fi
	fi
}

run_radvd()
{
	kill -9 $(cat $RADVDPID 2> /dev/null) 2> /dev/null
	sleep 1
	radvd -C $RADVDCONF -p $RADVDPID 2>/dev/null
	radvd_ret=$?
}

start_radvd()
{
	init_radvd_conf
	init_conf_inf
	update_lan_mode
	init_etcresolv

	while read line 
	do
		eval $line
		if [ $? -ne 0 ]; then
			echo "invalid database entry $line in $database" >> $LOGFILE
			reset_env	
			continue;
		fi
		if [ "z$EVENT_TYPE" == "zUP" ]; then
			case $EVENT_SRC in
			LAN)
				;;
			WAN)
				record_rlife
				get_max_RAinterval
				update_etcresolv $EVENT_TYPE
				echo "${INTF} ${PREFIX}" >> ${RADVD_PD}
				;;
			sixrd)
				sixrd_in_use="true"
				echo "${INTF} ${PREFIX}" >> ${RADVD_PD}
				;;
			*)
				;;
			esac
			record_mtu	
		fi
		add_prefix
		reset_env	
	done < $DATABASE
	finish_conf_inf
	if [ "z$zero_routerlife" == "zno" -o "z$sixrd_in_use" == "ztrue" ] ; then
		sed -i '/AdvDefaultLifetime 0;/d' $RADVDCONF
	fi
	if [ $ipv6_mtu -ge 1280 2>/dev/null ] ; then
		sed -i "s/AdvLinkMTU 0;/AdvLinkMTU ${ipv6_mtu};/" $RADVDCONF
	else
		sed -i '/AdvLinkMTU 0;/d' $RADVDCONF
	fi

	if [ $max_rainterval -ge 4 2>/dev/null ]; then
		sed -i "s/MaxRtrAdvInterval 100;/MaxRtrAdvInterval ${max_rainterval};/" $RADVDCONF
	fi

	case $lan_mode in
	SLDHCP)
		sed -i '/AdvManagedFlag on;/d' $RADVDCONF
		;;

	SLAAC)
		sed -i '/AdvManagedFlag on;/d' $RADVDCONF
		sed -i '/AdvOtherConfigFlag on;/d' $RADVDCONF
		;;
	SFDHCP)
		sed -i '/AdvOtherConfigFlag on;/d' $RADVDCONF
		;;
	*)
		sed -i '/AdvManagedFlag on;/d' $RADVDCONF
		;;
	esac
		
	run_radvd

}

retry_and_restore_radvd()
{
	if [ -f $RADVDPID -a -d /proc/$(cat $RADVDPID 2> /dev/null) ]; then
		echo "radvd with pid $(cat $RADVDPID 2> /dev/null) is running" >> $LOGFILE
		return;
	fi
	run_radvd
	if [ $radvd_ret -ne 0 ]; then
		restore_database
		start_radvd
	fi
}
	
update_rlife()
{
	intf=$1
	ra_rcvd=$2
	if [ "z$intf" != "z" ]; then
		if [ "$ra_rcvd" == "0" ]; then
			echo "ipv6: update_rlife updating RLZERO=1 for $intf" >> $LOGFILE
			sed  -i "/${ARG_INTF}.*EVENT_SRC=WAN.*EVENT_TYPE=UP.*SRC_INTF=$intf;/ s/RLZERO=[01]*;/RLZERO=1;/" $DATABASE
		else
			echo "ipv6: update_rlife updating RLZERO=0 for $intf" >> $LOGFILE
			sed  -i "/${ARG_INTF}.*EVENT_SRC=WAN.*EVENT_TYPE=UP.*SRC_INTF=$intf;/ s/RLZERO=[01]*;/RLZERO=0;/" $DATABASE
		fi
	else
		echo "ipv6: update_rlife invalid interface name" >> $LOGFILE
	fi
}

case $1 in
	update)
		if [ `expr $ARG_PLIFETIME + 1 2> /dev/null` -a `expr $ARG_VLIFETIME + 1 2> /dev/null` -a  $ARG_VLIFETIME -ge $ARG_PLIFETIME  ]; then
			echo "Updating prefix $PREFIX with plife=$PLIFETIME & vlife=$VLIFETIME" >> $LOGFILE
			update_database
		elif [ "z$ARG_EVENT_SRC" == "zWAN" -a  "z$ARG_SRC_INTF" != "z" -a "z$ARG_EVENT_TYPE" == "zDOWN" ];then
			update_database
		else
			echo "prefix $ARG_PREFIX with plife=$ARG_PLIFETIME & vlife=$ARG_VLIFETIME is not valid" >> $LOGFILE
		fi
		;;
	stop)
		kill -9 $(cat $RADVDPID 2> /dev/null) 2> /dev/null
		;;
	rlife)
		reconfig_dhcpv6="no"
		update_rlife $2 $3
		if [ "$3" = "1" ]; then
			/etc/rc.d/ds-lite.sh update_slaac $2 &
                        /etc/rc.d/rc.bringup_ipv6_staticRoutes
		fi
		;;
	reset)
		mv $DATABASE $DATABASE.$(gettime)
		touch $DATABASE
		;;
	lightwt)
		reconfig_dhcpv6="no"
		;;
esac

if [ "z$1" != "zstop" ];then
	if [ "z$reconfig_dhcpv6" == "zyes" ]; then
        	. /etc/rc.d/create_and_run_dhcp6c_cfg serverstart br0 0
	fi
	${DELEGATED_PREFIX_FIREWALL} 
	start_radvd
	if [ $radvd_ret -ne 0 ]; then
		retry_and_restore_radvd
	fi
fi
reset_env
rm -f ${TMPFILE_RDNSS}
rm -f ${TMPFILE_ROUTEINFO}
rm -f ${OWNPIDFILE}
