#!/bin/sh /etc/rc.common
#
# Install codec driver and make codec device node

START=37

bin_dir=/lib/modules/`uname -r`/
drv_obj_file_name=tlv320aic34.ko


start() {
	[ -e ${bin_dir}/${drv_obj_file_name} ] && {
		insmod ${bin_dir}/${drv_obj_file_name};
		#create device node
		/usr/sbin/mknod_util codec /dev/codec
		mknod /dev/codec p
	}
}
