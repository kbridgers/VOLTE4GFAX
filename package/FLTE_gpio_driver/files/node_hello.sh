#!/bin/sh /etc/rc.common
#
# Install codec driver and make codec device node

START=71
bin_dir=/lib/modules/`uname -r`/
drv_obj_file_name=FLTE_gpio_driver.ko

start() {
	[ -e ${bin_dir}/${drv_obj_file_name} ] && {
		insmod ${bin_dir}/${drv_obj_file_name};
		#create device node
		/usr/sbin/mknod_util gpio_flte /dev/gpio_flte
		mknod /dev/gpio_flte p
	}
}
