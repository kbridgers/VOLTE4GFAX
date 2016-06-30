#!/bin/sh /etc/rc.common
#
# Install VoIP common drivers

START=37

# check for linux 2.6.x
uname -r | grep -q 2.6.
if [ $? -eq 0 ]; then
    MODEXT=.ko
fi

bin_dir=/lib/modules/`uname -r`/
drv_obj_file_name1=voip_timer_driver.ko


start() {
	[ -e ${bin_dir}/${drv_obj_file_name1} ] && {
		insmod ${bin_dir}/${drv_obj_file_name1};
		#create device nodes for...
		#major_no=`grep $drv_dev_base_name /proc/devices |cut -d' ' -f1`
		/bin/mknod /dev/voip_timer_driver c 229 0
	}
	# Start VoIP service on bridge interface so that internal VoIP calls can be made without need of WAN connection.
	# Whan WAN connection comes up, VoIP service will be re-started on that interface to serve both internal and external calls.
	if [ "A$CONFIG_IFX_DECT_SUPPORT" != "A1" ]; then
		# if DECT is not enabled then start VoIP service here, else it will happen in S38 script
		/etc/rc.d/rc.bringup_voip_start default
	fi
}
