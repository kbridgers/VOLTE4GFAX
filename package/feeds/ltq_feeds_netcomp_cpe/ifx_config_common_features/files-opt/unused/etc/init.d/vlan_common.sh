#!/bin/sh
#509141: add vlan common script
if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		 . /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi


FUNCNAME=`basename $1`

case "$FUNCNAME" in
"vbridge_get_ifname")
	index="$2"
	if [ $index -eq 0 ]; then
		echo "br0"
	elif [ $index -gt 0 -a $index -lt 16 ]; then
		eval link_type='$'wan_${index}_linkType
		eval addr_type='$'wanip_${index}_addrType

		if [ "$link_type" = "1" -a "$addr_type" = "0" ]; then # EoATM Bridge Mode
			eval nasif='$'wan_${index}_iface
		else
			nasif=""
		fi
		

		echo $nasif
	elif [ $index -ge 16 -a $index -le 20 ]; then
		echo "swport$(( $index - 16 ))"
	else
		echo ""
	fi
	;;
"vbridge_get_cfg")
	index="$2"
	total=$vb_pbvgs_groups
	found=0
	vid=0
	i=1
	while [ "$i" -le "$vb_pbvgs_groups" -a "$found" -eq 0 ]
	do
		eval group='$'vb_pbvgs_groups_$i
		if [ `echo $group | cut -f$index -d"_"` -eq 1 ]; then
			vid=$i
			found=1
		fi
		i=$(( $i + 1 ))
	done
	if [ "$found" -eq 1 ]; then
		#604181:Nirav - By defualt the ingress mode is hybrid
		echo "$vid 2 0"
	else
		echo ""
	fi
	;;
*)
	echo ""
	;;
esac
