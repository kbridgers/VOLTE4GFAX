#!/bin/sh
if [ ! "$CONFIGLOADED" ]; then
  if [ -r /etc/rc.d/config.sh ]; then
    . /etc/rc.d/config.sh 2>/dev/null
    CONFIGLOADED="1"
  fi
fi

if [ ! "$ENVLOADED" ]; then
  if [ -r /etc/rc.conf ]; then
    . /etc/rc.conf 2> /dev/null
    ENVLOADED="1"
  fi
fi


platform=`/usr/sbin/status_oper GET ppe_config_status ppe_platform`

# probe address type: IP or URL
probeDstType="$1"

# $22== WAN Type - 1: Primary, 2: Secondary, 3:Both
wan_mode="$2"

# Set ping Count
#pingCount="3"
PING="/bin/ping"
SED="/bin/sed"
RM="/bin/rm"
SystemDefaultGw=""
#Following list the static parameters to be passed to pin.                                                                   
#Ping will wait for 5 secs for first response and at max it will wait for 10 secs                                            
PingParam="-W 5 -w 10 -c 3"
# Probe address
probe_dest=$dw_ipaddr

log()
{
  echo "<DW:Script>: $1"
}

# log "dw_daemon_probe_wan script called "

# Update the status information in the /tmp/system_status file.
# This will be read by the daemon and corresponding wan handle will happen
update_status()
{
  if [ "$1" = "1" ]; then
    /usr/sbin/status_oper -u SET "dw_probe_status_info" \
                                 "dw_pri_l2_status" $2  \
                                 "dw_pri_l3_status" $3
  else # secondary wan 
    /usr/sbin/status_oper -u SET "dw_probe_status_info" \
                                 "dw_sec_l2_status" $2  \
                                 "dw_sec_l3_status" $3
  fi
}

probe_phy_status() 
{
  # Note: Set phy_status to zero before calling this routine
  if [ "$phyMode" = "0" -o "$phyMode" = "3" ]; then  # ADSL or VDSL
    [ "`cat /tmp/adsl_status`" != "0" ] && phy_status="1"  
  elif [ "$phyMode" = "2" ]; then  # Mii1

    if [ "$platform" = "VR9" -o "$platform" = "AR10" ]; then
      link_status=`switch_cli dev=$CONFIG_LTQ_SWITCH_DEVICE_ID IFX_ETHSW_PORT_LINK_CFG_GET nPortId=5 2>&- | grep eLink |awk -F " " '{print $2}'`
    fi  
    
    [ "$link_status" = "0" ] &&  phy_status="1"
  elif [ "$phyMode" = "5" -o "$phyMode" = "6" ]; then # Cell WAN or LTE WAN
    # Check if the usb dongle is connected.
    [ -e "/tmp/cwan_status.txt" ] &&  phy_status="1"
  else
    log "Phy status probing for $phyMode is not handled"
  fi
}

# 
# Used to get wan_idx, wan_type and default wan interface (wanDefIf)
#
get_def_wan_idx_type()
{
#  tmpFile="/tmp/dwProbe.txt"
  . /etc/init.d/get_wan_mode $1 $2
  eval wanConnName='$'default_wan_${wanMode}_conn_connName
  eval wanDefIf='$'default_wan_${wanMode}_conn_iface

  # Get wan index and wan type
  wan_idx=${wanConnName##WANIP}
  if [ ! -z $wan_idx ] && [ -z "${wan_idx//[0-9]/}" ]; then
    wan_type="IP"  
  else
    wan_idx=${wanConnName##WANPPP}
    wan_type="PPP"
  fi
#  echo ${wanConnName} > "$tmpFile"
#  wan_idx=`"$SED" -n 's,WANIP,,;1p' "$tmpFile"`
#  if [ "$wan_idx" = "$wanConnName" ]; then
#    wan_idx=`"$SED" -n 's,WANPPP,,;1p' "$tmpFile"`
#    wan_type="PPP"
#  else
#    wan_type="IP"  
#  fi
#  $RM -f $tmpFile
}


get_wan_gw_status()
{
  local WAN_CONN_TAG
  local stat

  get_def_wan_idx_type $1 $2
  WAN_CONN_TAG="Wan${wan_type}${wan_idx}_IF_Info"
  stat="`/usr/sbin/status_oper GET $WAN_CONN_TAG STATUS`"

  if [ "$stat" = "CONNECTED" ]; then

    WAN_GW_TAG="Wan${wan_type}${wan_idx}_GATEWAY"
    wanGw="`/usr/sbin/status_oper GET $WAN_GW_TAG ROUTER1`"

    [ -z $wanDefIf ] && {
      log "Error: Default WAN interface is not set for $wanConnName"
      return 0
    } 

    if [ -n "$wanGw" ]; then

      if [ -z $SystemDefaultGw ]; then
        ip route add default via $wanGw dev $wanDefIf > /dev/null
        [ $? != "0" ] && { 
           log "Error: Could not add default route"
           return 1
        }
      fi

      if [ "$phyMode" = "5" -o "$phyMode" = "6" ]; then
        return 0
      else    
        # log "Pinging default gateway: $wanGw"
        $PING $PingParam -I $wanDefIf $wanGw > /dev/null
        return $?
      fi
    fi
  fi

  return 1
}

#
# This routine return 0 if given URL is reachable on default interface
# else returns 1
#
resolveAndCheckReachability()
{
  local url=$1
  local file="/tmp/DwAddr.txt"
  local tmpFile="/tmp/DwAddr_t.txt"
  local fDfRouteAdded=0
  local fActiveWanStatus=0  
  local iLp=0 
  local iCount=0
  local status=1
  local dnsServer
   
  if [ $dw_pri_wanphy_phymode -eq $wanphy_phymode ] ; then
    fActiveWanStatus="`/usr/sbin/status_oper GET dw_probe_status_info dw_pri_l3_status`"
  else
    fActiveWanStatus="`/usr/sbin/status_oper GET dw_probe_status_info dw_sec_l3_status`"
  fi

  if [ $fActiveWanStatus -eq 0 ] ; then 
    #
    # Default gateway is not reachable, so change it.
    # When primary is down, to get dns reply either dns should be started on
    # secondary WAN or dns should be killed & pass dns server to nslookup.
    # In this version preffered first option. 
    # Note: nslookup failing with server(secondary WAN's dns) option when dnrd
    # is running. Primary may be temporarily down for very short duration, so
    # dnrd needs to be restarted.
    #   
    WAN_GW_TAG="Wan${wan_type}${wan_idx}_DNS_SERVER"
    dnsServer="`/usr/sbin/status_oper GET $WAN_GW_TAG DNS1`"
    if [ -z dnsServer ] ; then
      log "No DNS servers defined for $wanDefIf interface"
      return 1
    fi
    route del default gw $SystemDefaultGw >/dev/null
    route add default gw $wanGw dev $wanDefIf >/dev/null
    /etc/init.d/dns_relay_start $wan_type $wan_idx
    fDfRouteAdded=1
  fi

  # Do nslookup
  nslookup $url > $tmpFile
  if [ "$?" = "0" ]; then
    
    cat $tmpFile | sed '1,4'd > $file
    list=`cat $file | wc -l`
    
    # In some cases if default WAN gateway is not that of current WAN interface
    # ping is not succeeding. So add current WAN gateway as default route.
    # 
    [ $fDfRouteAdded -eq 0 -a  "$wanGw" != "$SystemDefaultGw" ] && {
       # log "$wanGw is not default gateway"
       route add default gw $wanGw dev $wanDefIf > /dev/null
       fDfRouteAdded=1
    }

    # Ping till success on each IPv4 addresses returned by nslookup. Restrict to
    # max of 4 address
    while [ $iLp -le $list -a $iCount -lt 4 ]; do
      probe_ip=`cat $file | awk '/Address '$iLp':/ {print $3}' | grep -E "[0-9].*\."`
      [ ! -z $probe_ip ] && {
        
        # log "resolved IP = $probe_ip"
        # log "$PING $PingParam -I $wanDefIf $probe_ip"
        $PING $PingParam -I $wanDefIf $probe_ip > /dev/null    
        if [ "$?" = "0" ]; then
          status=0
          break
        fi
        unset probe_ip
        iCount=`expr $iCount + 1`
      } 
      
     iLp=`expr $iLp + 1`
      
    done
  fi

  [ $fDfRouteAdded -eq 1 ] &&  {
    # log "Deleting temp default route"
    route del default gw $wanGw dev $wanDefIf >/dev/null
  }

  [ $fActiveWanStatus -eq 0 ] &&  {
    # log "Restore default route and restart DNS"
    route add default gw $SystemDefaultGw >/dev/null
    /etc/init.d/dns_relay restart &
  }
  return $status
}

probe_wan_status()  
{
  # $1 - wan to be probed (1: primary, 2:secondary)
  # $2 - probe_string (IP/URL) 
  # $3 - destination url/ip to be model
  
  # Note: Set wan_status to zero before calling this routine
  
  get_wan_gw_status $phyMode $tcMode
 
  if [ "$?" = "0" ] ; then 

    if [ "$2" = "IP" -o "$wanphy_phymode" = "$phyMode" ]; then
      
      # log "$PING $PingParam -I $wanDefIf $3"
      
      $PING $PingParam -I $wanDefIf $3 > /dev/null    
      [ "$?" = "0" ] && wan_status="1"
    else
      # When dnrd is running on other WAN, ping on ipv4 addresses returned by 
      # nslookup till success
      #
      
      resolveAndCheckReachability $3
      [ "$?" = "0" ] && wan_status="1"
    fi
  fi
  [ "$wan_status" = "0" ] && log "$3 is not REACHABLE (on $1)"
}

probe() 
{
  phy_status="0"
  wan_status=0
  if [ "$1" = "1" ]; then
    phyMode=$dw_pri_wanphy_phymode
    tcMode=$dw_pri_wanphy_tc
  elif [ "$1" = "2" ]; then
    phyMode=$dw_sec_wanphy_phymode
    tcMode=$dw_sec_wanphy_tc
  fi
  
  # log "Probe: phyMode=$phyMode & tcMode=$tcMode" 

  probe_phy_status
  # if physical status is UP, then check the wan status
  if [ "$phy_status" = "1" ]; then
    probe_wan_status $1 $probeDstType $probe_dest
  fi

  update_status $1 $phy_status $wan_status
}

# Get systems default route
fileRoute="/tmp/DwRoute.txt"
ip route > $fileRoute
SystemDefaultGw=`cat $fileRoute | awk '/default via/ {print $3}'`
$RM -f $fileRoute

if [ "$wan_mode" = "1" -o "$wan_mode" = "2" ]; then    
  # mode = primary or secondary
  # Probe and update the status for the given wan
  probe $wan_mode

elif [ "$wan_mode" = "3" ]; then       # mode = both
  # probe and update the primary wan status
  probe 1
  
  # Probe  and update secondary wan status  
  probe 2
fi

# Unused Code - Kept for later use
# Check link status on AR9 Mii0
#  elif [ "$phyMode" = "1" ]; then  # Mii0
#    if [ "$platform" = "AR9" ]; then
#      if [ "$CONFIG_PACKAGE_KMOD_LTQCPE_GW188_SUPPORT" = "1" ]; then
#        link_status=`switch_cli dev=$CONFIG_LTQ_SWITCH_DEVICE_ID IFX_ETHSW_PORT_LINK_CFG_GET nPortId=3 2>&- |grep eLink |awk -F " " '{print $2}'`
#      else
#        link_status=`switch_cli dev=$CONFIG_LTQ_SWITCH_DEVICE_ID IFX_ETHSW_PORT_LINK_CFG_GET nPortId=4 2>&- |grep eLink |awk -F " " '{print $2}'`
#      fi
#    fi
#    if [ "$link_status" = "0" ]; then # 0 - UP , 1 - DOWN in switch_utility
#      #echo "Phy Link for WAN MODE $wan_mode is UP"
#      phy_status="1"
#    else
#      echo "Phy Link for WAN MODE $wan_mode is DOWN"
#      phy_status="0"
#    fi
# Check link status on AR9 Mii1
#    elif [ "$platform" = "AR9" ]; then
#      link_status=`switch_cli dev=0 IFX_ETHSW_PORT_LINK_CFG_GET nPortId=1 2>&- |grep eLink |awk -F " " '{print $2}'`
