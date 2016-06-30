#!/bin/sh
test "x$1" = xstart && rmmod mtlk;
test "x$1" = xstart && insmod /ramdisk/tmp/mtlk.ko dut=1;
test "x$1" = xstop && rmmod mtlk;
exit 0;