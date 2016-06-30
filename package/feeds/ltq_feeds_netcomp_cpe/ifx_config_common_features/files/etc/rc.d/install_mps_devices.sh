#!/bin/sh

module="ifx_mps"
device1="cmd"
device2="voice"
case "$1" in
  start)
    echo installing mps devices...
    
    if [ -e /dev/ifx_mps/${device1} ]; then
      rm -rf /dev/ifx_mps/
    fi

    for i in `grep $module /proc/devices` ; do
       if [ $i != $module ]; then
          major=$i
       fi
    done
    
#    echo " Major: $major   device1: ${device1}"
    mkdir /dev/ifx_mps
    mknod /dev/ifx_mps/${device1} c $major 1
    mknod /dev/ifx_mps/${device2}0 c $major 2
    mknod /dev/ifx_mps/${device2}1 c $major 3
    mknod /dev/ifx_mps/${device2}2 c $major 4
    mknod /dev/ifx_mps/${device2}3 c $major 5
    
    ;;
   *)
   echo "Usage: $0 {start} "
    #exit 1
    ;;
esac

