#!/bin/sh /etc/rc.common
#
# Install led driver and make led device node

START=71

bin_dir=/lib/modules/`uname -r`/
drv_obj_file_name=FLTE_led_driver.ko


start() {
	[ -e ${bin_dir}/${drv_obj_file_name} ] && {
		insmod ${bin_dir}/${drv_obj_file_name};
		#create device node
		/usr/sbin/mknod_util led /dev/led
		mknod /dev/fifo_led p
	}
}
