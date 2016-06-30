#!/bin/sh

cd /ramdisk/etc/dsl_api

if [ -e /ramdisk/etc/dsl_api/cmv_scripts ]; then
  cp cmv_scripts cmv_scripts.1
  gzip cmv_scripts
  /usr/sbin/upgrade /ramdisk/etc/dsl_api/cmv_scripts.gz fwdiag 0 1
  mv cmv_scripts.1 cmv_scripts
else
  echo cmv_scripts does not exist.
fi

cd -
