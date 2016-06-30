#!/bin/sh
VOIP_SIGNAL_PORT_UDP=""
VOIP_SIGNAL_PORT_TCP=""
VOIP_DATA_SPORT=""
VOIP_DATA_EPORT=""
FAX_SPORT=""
FAX_EPORT=""

OLD_VOIP_SIGNAL_PORT_UDP=""
OLD_VOIP_SIGNAL_PORT_TCP=""
OLD_VOIP_DATA_SPORT=""
OLD_VOIP_DATA_EPORT=""
OLD_FAX_SPORT=""
OLD_FAX_EPORT=""

VOIP_SIP_PRIORITY=""
VOIP_RTP_PRIORITY=""

OLD_VOIP_SIP_PRIORITY=""
OLD_VOIP_RTP_PRIORITY=""


#OLD_VOIP_SIGNAL_PORT_UDP 5060 VOIP_SIGNAL_PORT_UDP 5061 OLD_VOIP_SIGNAL_PORT_TCP 5060 VOIP_SIGNAL_PORT_TCP 5162 OLD_VOIP_DATA_SPORT 3000 OLD_VOIP_DATA_EPORT 5000 VOIP_DATA_SPORT 3001  VOIP_DATA_EPORT 5001 OLD_FAX_SPORT 5500 OLD_FAX_EPORT 5200 FAX_SPORT 6500  FAX_EPORT 7500
i=0
echo $#
totargs=$#
while test $i -lt $totargs
do
	i=`expr $i + 2`
	str=$1
	str1=$2
	shift
	shift
	case $str in
	OLD_VOIP_SIGNAL_PORT_UDP) 
		VOIP_SIGNAL_PORT_UDP=$str1
		echo "$str = $str1"
		iptables -D OUTPUT -p udp --sport $VOIP_SIGNAL_PORT_UDP -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p udp --dport $VOIP_SIGNAL_PORT_UDP -j ACCEPT 2> /dev/null
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p udp --dport $VOIP_SIGNAL_PORT_UDP -j ACCEPT 2> /dev/null;;
							   
	OLD_VOIP_SIGNAL_PORT_TCP) 
		VOIP_SIGNAL_PORT_TCP=$str1
		echo "$str = $str1"
		iptables -D OUTPUT -p tcp --sport $VOIP_SIGNAL_PORT_TCP -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p tcp --dport $VOIP_SIGNAL_PORT_TCP -j ACCEPT 2> /dev/null 
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p tcp --dport $VOIP_SIGNAL_PORT_TCP -j ACCEPT 2> /dev/null ;;
	VOIP_SIGNAL_PORT_UDP)
		VOIP_SIGNAL_PORT_UDP=$str1
		echo "$str = $str1"
		iptables -D OUTPUT -p udp --sport $VOIP_SIGNAL_PORT_UDP -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p udp --dport $VOIP_SIGNAL_PORT_UDP -j ACCEPT 2> /dev/null
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p udp --dport $VOIP_SIGNAL_PORT_UDP -j ACCEPT 2> /dev/null
		iptables -A OUTPUT -p udp --sport $VOIP_SIGNAL_PORT_UDP -j ACCEPT
		iptables -A IFX_FW_ACCEPT_SERVICES -p udp --dport $VOIP_SIGNAL_PORT_UDP -j ACCEPT
		iptables -t nat -A IFX_NAPT_PREROUTING_WAN -p udp --dport $VOIP_SIGNAL_PORT_UDP -j ACCEPT ;;

	VOIP_SIGNAL_PORT_TCP)
		VOIP_SIGNAL_PORT_TCP=$str1
		echo "$str = $str1"
		iptables -D OUTPUT -p tcp --sport $VOIP_SIGNAL_PORT_TCP -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p tcp --dport $VOIP_SIGNAL_PORT_TCP -j ACCEPT 2> /dev/null 
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p tcp --dport $VOIP_SIGNAL_PORT_TCP -j ACCEPT 2> /dev/null 
		iptables -A OUTPUT -p tcp --sport $VOIP_SIGNAL_PORT_TCP -j ACCEPT 
		iptables -A IFX_FW_ACCEPT_SERVICES -p tcp --dport $VOIP_SIGNAL_PORT_TCP -j ACCEPT 
		iptables -t nat -A IFX_NAPT_PREROUTING_WAN -p tcp --dport $VOIP_SIGNAL_PORT_TCP -j ACCEPT ;;

	VOIP_DATA_SPORT) 
		i=`expr $i + 2`
		str2=$1
		str3=$2
		shift
		shift
		VOIP_DATA_SPORT=$str1
		VOIP_DATA_EPORT=$str3
		echo "$str = $str1  $str2 = $str3 "
		iptables -D OUTPUT -p udp --sport $VOIP_DATA_SPORT:$VOIP_DATA_EPORT -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p udp --dport $VOIP_DATA_SPORT:$VOIP_DATA_EPORT -j ACCEPT 2> /dev/null
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p udp --dport $VOIP_DATA_SPORT:$VOIP_DATA_EPORT -j ACCEPT 2> /dev/null
		iptables -A OUTPUT -p udp --sport $VOIP_DATA_SPORT:$VOIP_DATA_EPORT -j ACCEPT 
		iptables -A IFX_FW_ACCEPT_SERVICES -p udp --dport $VOIP_DATA_SPORT:$VOIP_DATA_EPORT -j ACCEPT
		iptables -t nat -A IFX_NAPT_PREROUTING_WAN -p udp --dport $VOIP_DATA_SPORT:$VOIP_DATA_EPORT -j ACCEPT ;;

	OLD_VOIP_DATA_SPORT)
		i=`expr $i + 2`
		str2=$1
		str3=$2
		shift
		shift
		VOIP_DATA_SPORT=$str1
		VOIP_DATA_EPORT=$str3
		echo "$str = $str1  $str2 = $str3 "
		iptables -D OUTPUT -p udp --sport $VOIP_DATA_SPORT:$VOIP_DATA_EPORT -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p udp --dport $VOIP_DATA_SPORT:$VOIP_DATA_EPORT -j ACCEPT 2> /dev/null
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p udp --dport $VOIP_DATA_SPORT:$VOIP_DATA_EPORT -j ACCEPT 2> /dev/null ;;

	FAX_SPORT) 

		i=`expr $i + 2`
		str2=$1
		str3=$2
		shift
		shift
		FAX_SPORT=$str1
		FAX_EPORT=$str3
		echo "$str = $str1  $str2 = $str3 "
		iptables -D OUTPUT -p udp --sport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -D OUTPUT -p tcp --sport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p udp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p tcp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null 
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p udp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p tcp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -A OUTPUT -p udp --sport $FAX_SPORT:$FAX_EPORT -j ACCEPT
		iptables -A OUTPUT -p tcp --sport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -A IFX_FW_ACCEPT_SERVICES -p udp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 
		iptables -A IFX_FW_ACCEPT_SERVICES -p tcp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT  
		iptables -t nat -A IFX_NAPT_PREROUTING_WAN -p udp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 
		iptables -t nat -A IFX_NAPT_PREROUTING_WAN -p tcp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT ;;

	OLD_FAX_SPORT) 
		i=`expr $i + 2`
		str2=$1
		str3=$2
		shift
		shift
		FAX_SPORT=$str1
		FAX_EPORT=$str3
		echo "$str = $str1  $str2 = $str3 "
		iptables -D OUTPUT -p udp --sport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -D OUTPUT -p tcp --sport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p udp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -D IFX_FW_ACCEPT_SERVICES -p tcp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null 
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p udp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null
		iptables -t nat -D IFX_NAPT_PREROUTING_WAN -p tcp --dport $FAX_SPORT:$FAX_EPORT -j ACCEPT 2> /dev/null ;;

	VOIP_SIP_PRIORITY*) echo $str
			export $str;;
	OLD_VOIP_SIP_PRIORITY*) echo $str
			export $str;;
	VOIP_RTP_PRIORITY*) echo $str
			export $str;;
	OLD_VOIP_RTP_PRIORITY*) echo $str
			export $str;;
	esac

done
#sh

VOIP_PRIORITY="0"

# XXX: Hack to fit range into start val/mask pairs - approx
#VOIP_DATA_RANGE1="0x1388"
#VOIP_DATA_MASK1="0xFF80"
#VOIP_DATA_RANGE2="0x1400"
#VOIP_DATA_MASK2="0xFE00"
#VOIP_DATA_RANGE3="0x1600"
#VOIP_DATA_MASK3="0xFF00"
#VOIP_DATA_RANGE4="0x1700"
#VOIP_DATA_MASK4="0xFF80"
