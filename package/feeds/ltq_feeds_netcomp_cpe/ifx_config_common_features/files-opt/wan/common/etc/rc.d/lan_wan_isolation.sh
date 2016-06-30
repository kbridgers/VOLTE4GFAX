#!/bin/sh


if [ $# -ne 1 ]
then 
 echo "$# parameters are entered : (Usage : $0 arg1)"
 exit 127

fi

echo $# $0 $1 $2

if [ "$1" = "add" ]
then
     
      
      ebtables -A FORWARD -p 0x8863 -j ACCEPT
      ebtables -A FORWARD -p 0x8864 -j ACCEPT
      ebtables -P FORWARD DROP

fi

if [ "$1" = "del" ]
then

      ebtables -D FORWARD -p 0x8863 -j ACCEPT
      ebtables -D FORWARD -p 0x8864 -j ACCEPT
      ebtables -P FORWARD ACCEPT

fi
#exit 0

