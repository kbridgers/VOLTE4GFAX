#!/bin/sh

cd /ramdisk/etc/dsl_api

/usr/sbin/read_img fwdiag /ramdisk/etc/dsl_api/cmv_scripts.gz
if [ $? -eq 0 ]; then
  rm -f /ramdisk/etc/dsl_api/cmv_scripts
  gunzip cmv_scripts.gz
else
  /usr/sbin/save_dsl_settings.sh
fi

cd -
