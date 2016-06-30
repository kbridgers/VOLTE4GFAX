#!/bin/sh

echo "Unloading PPA Modules"
toLower(){ echo "$1"|sed 'y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/';}

firmware=`/usr/sbin/status_oper GET ppe_config_status ppe_firmware`
ppe_fw=`toLower $firmware`

platform=`/usr/sbin/status_oper GET ppe_config_status ppe_platform`
ppe_platform=`toLower $platform`

grep ifxmips_ppa_datapath_${ppe_platform}_${ppe_fw} /proc/modules
if [ "$?" = "0" ]; then
  rmmod ifx_ppa_api_proc
  rmmod ifx_ppa_api
  rmmod ifxmips_ppa_hal_${ppe_platform}_${ppe_fw}
  sleep 1
  if [ "$1" != "tr69" ]; then
    rmmod ifxmips_ppa_datapath_${ppe_platform}_${ppe_fw}
  fi
fi

