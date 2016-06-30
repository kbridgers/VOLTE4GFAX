#!/bin/sh

# Defines
if [ ! "$MTLK_INIT_PLATFORM" ]; then
        . /tmp/mtlk_init_platform.sh

fi

FIRMWARE_DIR=`grep FIRMWARE_DIR= /etc/hotplug/firmware.agent | sed 's/FIRMWARE_DIR=//'`

src_file=$1
dst_file=$FIRMWARE_DIR/$2

if [ ! -e $src_file ]
then
	echo "error: file '$src_file' do not exists" >&2
	exit -1
fi

cp "$src_file" "$dst_file"
ret=$?

if [ $ret = 0 ]
then
	echo "file '$dst_file' saved."
else
	echo "error: failed to save file '$dst_file'" >&2
fi

# Write the new calibration file to the FLASH. The script only writes files name cal_*.bin to the FLASH.
(. $ETC_PATH/wave_wlan_dut write)

exit $ret
