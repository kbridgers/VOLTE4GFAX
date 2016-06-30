#!/bin/sh /etc/rc.common
# Copyright (C) 2013 OpenWrt.org
START=17
DebugLevel=3

BIN_DIR=/opt/lantiq/bin

start() {
	[ -z "`cat /proc/modules | grep ifxos`" ] && {
		echo "Ooops - IFXOS isn't loaded, MEI Driver will do it. Check your basefiles..."
		insmod /lib/modules/*/drv_ifxos.ko
	}

	if [ -r ${BIN_DIR}/dsl.cfg ]; then
		. ${BIN_DIR}/dsl.cfg 2> /dev/null
	fi

	if [ "$xDSL_Dbg_DebugLevel" != "" ]; then
		DebugLevel="${xDSL_Dbg_DebugLevel}"
	else
		if [ -e ${BIN_DIR}/debug_level.cfg ]; then
			# read in the global definition of the debug level
			. ${BIN_DIR}/debug_level.cfg 2> /dev/null

			if [ "$ENABLE_DEBUG_OUTPUT" != "" ]; then
				DebugLevel="${ENABLE_DEBUG_OUTPUT}"
			fi
		fi
	fi

	# loading DSL MEI Driver -
	cd ${BIN_DIR}
	${BIN_DIR}/inst_drv_mei_cpe.sh $DebugLevel
}
