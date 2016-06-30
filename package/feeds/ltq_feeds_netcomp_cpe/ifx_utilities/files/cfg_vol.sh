#!/bin/sh

cfg_mtd=$(grep config_vol /proc/mtd|cut -d: -f1)

put ()
{
	dd if="$1" of=/dev/mtdblock${cfg_mtd:3} bs=64k >&- 2>&-; sync
}

get ()
{
	dd if=/dev/mtd${cfg_mtd:3} of="$1" bs=64k >&- 2>&-; sync
}

$1 $2
