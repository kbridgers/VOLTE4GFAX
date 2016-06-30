#!/bin/sh

. /flash/rc.conf

# based on WAN mode, execute relevant command to retrieve RX byte count
# compare new reading with previous reading
# if new reading is more, then take difference of new and previous readings and send to file
# if new reading is less, then it's case of wrap round of value (data type max value reached)
#			then do calculation to find wrap-arounded value add to new reading and send to file
# if both readings are same, then send previous value to file and if this happens for > 2 times then send value 0 to file

if [ "$wanphy_phymode" = "1" -o "$wanphy_phymode" = "2" ]; then
	prevVal=0
	sameVal=0
	while [ 1 ]
	do
		diff=0
		newVal=`switch_cli IFX_ETHSW_RMON_GET nPortId=5 | grep nRxGoodBytes  | awk '{print $2;}'`
		if [ $newVal -gt $prevVal ]; then
			cnt_val=`expr $newVal - $prevVal`
		elif [ $newVal -lt $prevVal ]; then
			if [ $prevVal -gt 3000000000 ] && [ $newVal -lt 1000000000 ]; then
				diff=`expr 4294967295 - $prevVal`
			fi
			cnt_val=`expr $diff + $newVal`
		else
			sameVal=`expr $sameVal + 1`
		fi
		# if same value continues for > 2 readings then set plot value as 0
		if [ $sameVal -gt 2 ]; then
			cnt_val=0
			sameVal=0
		fi
		echo $cnt_val > /tmp/pkt_cnt
		prevVal=$newVal
		sleep 1
	done
elif [ "$wanphy_phymode" = "0" ]; then
	prevVal=0
	sameVal=0
	while [ 1 ]
	do
		diff=0
		newVal=`cat /proc/eth/mib | grep wrx_total_byte | sed 's/[^0-9.]*\\([0-9.]*\\).*/\\1/'`
		if [ $newVal -gt $prevVal ]; then
			cnt_val=`expr $newVal - $prevVal`
		elif [ $newVal -lt $prevVal ]; then
			#if [ $prevVal -gt 3000000000 ] && [ $newVal -lt 1000000000 ]; then
			#	diff=`expr 4294967295 - $prevVal`
			#fi
			cnt_val=`expr $diff + $newVal`
		else
			sameVal=`expr $sameVal + 1`
		fi
		# if same value continues for > 2 readings then set plot value as 0
		if [ $sameVal -gt 2 ]; then
			cnt_val=0
			sameVal=0
		fi
		echo $cnt_val > /tmp/pkt_cnt
		prevVal=$newVal
		sleep 1
	done
else
	prevVal=0
	sameVal=0
	while [ 1 ]
	do
		diff=0
		newVal=`cat /proc/net/dev | grep lte0 | sed -n "s/lte0://;1p" | awk '{printf $1;}'`
		if [ $newVal -gt $prevVal ]; then
			cnt_val=`expr $newVal - $prevVal`
		elif [ $newVal -lt $prevVal ]; then
			if [ $prevVal -gt 3000000000 ] && [ $newVal -lt 1000000000 ]; then
				diff=`expr 4294967295 - $prevVal`
			fi
			cnt_val=`expr $diff + $newVal`
		else
			sameVal=`expr $sameVal + 1`
		fi
		# if same value continues for > 2 readings then set plot value as 0
		if [ $sameVal -gt 2 ]; then
			cnt_val=0
			sameVal=0
		fi
		echo $cnt_val > /tmp/pkt_cnt
		prevVal=$newVal
		sleep 1
	done
fi
