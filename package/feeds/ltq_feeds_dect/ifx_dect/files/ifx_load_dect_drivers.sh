#!/bin/sh /etc/rc.common
#
# Install DECT drivers

START=38

# check for linux 2.6.x
uname -r | grep -q 2.6.
if [ $? -eq 0 ]; then
    MODEXT=.ko
fi

bin_dir=/usr/drivers/
drv_obj_file_name1=drv_timer.ko
drv_obj_file_name2=drv_dect.ko
drv_obj_file_name3=paging.ko

start() {
	/usr/sbin/read_img dectconfig /ramdisk/flash/dect_rc.conf.gz
	/bin/gunzip -f /ramdisk/flash/dect_rc.conf.gz 2>/dev/null
	if [ $? -ne 0 ]; then
		/bin/rm -f /ramdisk/flash/dect_rc.conf
		/usr/sbin/upgrade /etc/dect_rc.conf.gz dectconfig 0 1 2>/dev/null
		/usr/sbin/read_img dectconfig /ramdisk/flash/dect_rc.conf.gz
		/bin/gunzip -f /ramdisk/flash/dect_rc.conf.gz
	fi
	chmod 777 /flash/dect_rc.conf
	[ -e ${bin_dir}/${drv_obj_file_name1} ] && {
		insmod ${bin_dir}/${drv_obj_file_name1};
		# for netdev budget 10
		echo 10 > /proc/sys/net/core/netdev_budget
		#create device nodes for...
		#major_no=`grep $drv_dev_base_name /proc/devices |cut -d' ' -f1`
		/bin/mknod /dev/timer_drv c 212 0
	}
	[ -e ${bin_dir}/${drv_obj_file_name2} ] && {
		insmod ${bin_dir}/${drv_obj_file_name2};
		#create device nodes for...
		#major_no=`grep $drv_dev_base_name /proc/devices |cut -d' ' -f1`
		/bin/mknod /dev/dect_drv c 213 0
	}
	[ -e ${bin_dir}/${drv_obj_file_name3} ] && {
		insmod ${bin_dir}/${drv_obj_file_name3};
		#create device nodes for...
		#major_no=`grep $drv_dev_base_name /proc/devices |cut -d' ' -f1`
		/bin/mknod /dev/pb c 150 0
	}

	# Start VoIP service on bridge interface so that internal VoIP calls can be made without need of WAN connection.
	# Whan WAN connection comes up, VoIP service will be re-started on that interface to serve both internal and external calls.
	/etc/rc.d/rc.bringup_voip_start default
}
