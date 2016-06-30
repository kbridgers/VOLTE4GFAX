#!/bin/sh
if [ ! "$ENVLOADED" ]; then
  if [ -r /etc/rc.conf ]; then
    . /etc/rc.conf 2> /dev/null
    ENVLOADED="1"
  fi
fi

l2if_stop_mode()
{
  mode=$1
  if [ "$mode" != "6" ]; then
    i=0
    while [ $i -lt $vlan_ch_cfg_Count ]; do
      eval pcpeSec='$'vlanch_${i}_pcpeSec
      if [ "$mode" = "0" -a "$pcpeSec" = "adsl_vcchannel" ]; then
        . /etc/rc.d/rc.bringup_l2if stop $i 
      elif [ "$mode" = "3" -o "$mode" = "4" -a "$pcpeSec" = "ptm_channel" ]; then
        . /etc/rc.d/rc.bringup_l2if stop $i 
      elif [ "$mode" = "1" -o "$mode" = "2" -a "$pcpeSec" = "eth_channel" ]; then
        . /etc/rc.d/rc.bringup_l2if stop $i 
      # TODO - need to add LTE vlan support here when multi PDN Support is added
      fi
      i=`expr $i + 1`
    done
    if [ "$mode" = "7" ]; then
      ifconfig lte0 down
    fi
  fi
}

get_vlan_ch_instance()
{
    if [ "$1" = "IP" ]; then
        eval wan_l2if='$'wanip_${2}_l2ifName
    else
        eval wan_l2if='$'wanppp_${2}_l2ifName
    fi  

    vlan_idx_use=-1
    vlan_idx=0
    while [ $vlan_idx -lt $vlan_ch_cfg_Count ]
    do  
        eval ch_l2if='$'vlanch_${vlan_idx}_l2ifName
        if [ "$ch_l2if" = "$wan_l2if" ]; then
            vlan_idx_use=$vlan_idx
            return
        fi
        vlan_idx=`expr $vlan_idx + 1`
    done
}

start_def_wan() {
  # echo "Start the Default WAN connection on secondary wan - $dw_sec_wanphy_phymode"

  # get the wan mode configured from the wanphy mode and tc
  . /etc/init.d/get_wan_mode $1 $2

  # for multiwan get the wan index and start wanppp or wanip accordingly
  eval wanConnName='$'default_wan_${wanMode}_conn_connName
  wan_idx=${wanConnName##WANIP}
  if [ ! -z $wan_idx ] && [ -z "${wan_idx//[0-9]/}" ]; then
    # type is IP  
    if [ "$1" = "6" ] && [ -f "/tmp/cwan_status.txt" ]; then
      ifconfig lte0 down
      ifconfig lte0 up
    elif [ "$1" != "5" ]; then
      get_vlan_ch_instance "IP" $wan_idx
      /etc/rc.d/rc.bringup_l2if start $vlan_idx_use
    fi
    /etc/rc.d/rc.bringup_wanip_start ${wan_idx}
  else
    wan_idx=${wanConnName##WANPPP}
    if [ "$1" != "5" ]; then
      get_vlan_ch_instance "PPP" $wan_idx
      /etc/rc.d/rc.bringup_l2if start $vlan_idx_use
    fi
  
    /etc/rc.d/rc.bringup_wanppp_start ${wan_idx}
  fi
}

case $1 in
  "boot"|"start")
    echo "dual wan start called with args $1"
    if [ "$1" = "start" ]; then
      # start the wan connections which are already made up during bootup
      /etc/init.d/ltq_wan_changeover_start.sh
    fi
    #if bootup and failover type = HSB, start the secondary wan also
    if [ "$dw_standby_type" = "2" ]; then
      # Start the Default WAN connection on secondary wan
      start_def_wan $dw_sec_wanphy_setphymode $dw_sec_wanphy_settc
    fi

    # Update the initial probe status 
    /usr/sbin/status_oper SET "dw_probe_status_info" "dw_pri_l2_status" 0 "dw_pri_l3_status" 0 \
                  "dw_sec_l2_status" 0 "dw_sec_l3_status" 0

    # Start the dual wan daemon
    /usr/sbin/dw_daemon &
  ;;
  "stop")
    # echo "dual wan stop called with args $1 "
    if [ "$2" != "sig" ]; then
      killall -TERM dw_daemon  #Use signal name. signal numbers may differ
    fi
    # 
    # Since dw_daemon is stopped gracefully, so executing following commands is
    # not required.
    # killall -9 dw_daemon_probe_wan.sh
    # killall -9 dw_daemon_use_wan.sh
  ;;
  "start_def_wan")
    # $2 - phymode, $3 - tc mode
    start_def_wan $2 $3
  ;;
  "stop_wan")
    . /etc/init.d/get_wan_mode $dw_pri_wanphy_setphymode $dw_pri_wanphy_settc
    pri_mode=$wanMode
    . /etc/rc.d/rc.bringup_wan stop_mode $pri_mode
    l2if_stop_mode $pri_mode
    . /etc/init.d/get_wan_mode $dw_sec_wanphy_setphymode $dw_sec_wanphy_settc
    sec_mode=$wanMode
    . /etc/rc.d/rc.bringup_wan stop_mode $sec_mode
    l2if_stop_mode $sec_mode
    #
    # BUG: WANs are not started when active WAN is changed.
    # Reset link_up_once. Recent changes in ifx_util_event expects this var to
    # be reset.
    #
    /usr/sbin/status_oper SET "wan_link_status" "link_up_once" 0
    sleep 3
  ;;
*)
  ;;
esac
