#!/bin/sh

# In case it compiled as module
if [ -e /lib/modules/adsl/drv_dsl_cpe_api.ko ]; then
	/sbin/insmod /lib/modules/adsl/drv_dsl_cpe_api.ko
fi

if [ ! -e /dev/dsl_cpe_api ]; then
	/usr/sbin/mknod_util drv_dsl_cpe_os_linux /dev/dsl_cpe_api
	if [ ! -e /dev/dsl_cpe_api ]; then
		/usr/sbin/mknod_util drv_dsl_cpe_api /dev/dsl_cpe_api
	fi
	if [ ! -e /dev/dsl_cpe_api ]; then
		echo " ==== Can't create /dev/dsl_cpe_api for ADSL usage!!! ===="
		exit 1
	fi
fi

/usr/sbin/restore_dsl_settings.sh
# You don't need to specific default multiple-mode for Annex-A or Annex-B firmware, such as
#     -i05_00_04_00_0C_01_00_00 for Annex-A
# DSL API from now on will automatically detect information within FW and use correct multiple mode value.
if [ -e /etc/dsl_api/cmv_scripts ]; then
	# Capability for ADSL IOP patch
	/usr/sbin/dsl_cpe_control -n"/etc/rc.d/init.d/xdslrc.sh" -i -f/firmware/modemhwe.bin -a /etc/dsl_api/cmv_scripts &
else
	/usr/sbin/dsl_cpe_control -n"/etc/rc.d/init.d/xdslrc.sh" -i -f/firmware/modemhwe.bin &
fi
