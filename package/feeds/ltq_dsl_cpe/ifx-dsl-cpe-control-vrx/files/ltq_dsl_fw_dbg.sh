#!/bin/sh /etc/rc.common
# Copyright (C) 2012 OpenWrt.org
START=99

BIN_DIR=/opt/lantiq/bin
FW_DIR=/opt/lantiq/firmware

start() {
	# This default initializations will be overwritten with external values defined
	# within file "/opt/lantiq/bin/dsl.cfg"
	xDSL_Dbg_UpdateFw=0
	# Temporary name for firmware download file name
	xDSL_Dbg_TmpFwName="xcpe_hw_dnl.bin"
	
	if [ -r ${BIN_DIR}/dsl.cfg ]; then
		. ${BIN_DIR}/dsl.cfg 2> /dev/null
	fi

	if [ ${xDSL_Dbg_UpdateFw} != 0 ]; then

		[ -d ${FW_DIR} ] || mkdir ${FW_DIR}
		cd ${FW_DIR}

		echo ""
		echo "FwDbgEnv: Start download of user defined DSL Firmware ..."
		echo "   file name: ${xDSL_Dbg_RemoteFwName}"
		echo "   server IP: ${xDSL_Dbg_TftpServerIp}"
		echo ""
		tftp -gr ${xDSL_Dbg_RemoteFwName} -l ${xDSL_Dbg_TmpFwName} ${xDSL_Dbg_TftpServerIp}

		if [ -s ${FW_DIR}/${xDSL_Dbg_TmpFwName} ]; then
			mv -f ${FW_DIR}/${xDSL_Dbg_TmpFwName} ${FW_DIR}/${xDSL_Dbg_RemoteFwName}
			if [ -s ${FW_DIR}/${xDSL_Dbg_RemoteFwName} ]; then
				echo "   success downloading DSL Firmware."
				echo "   fw version: `${BIN_DIR}/what.sh ${FW_DIR}/${xDSL_Dbg_RemoteFwName}`"
				echo ""
			else
				echo "   failed downloading DSL Firmware!!!"
			fi
		else
			echo "   failed downloading DSL Firmware!!!"
			echo "   Check if TFTP server is running and DSL FW binary exists."
			echo ""
		fi
	fi

	if [ -s ${FW_DIR}/${xDSL_Dbg_RemoteFwName} ]; then
		echo "FwDbgEnv: Reconfigure DSL CPE API for usage of user defined DSL Firmware ..."
		echo "   Used DSL Firmware: ${FW_DIR}/${xDSL_Dbg_RemoteFwName}"
		echo ""
		${BIN_DIR}/dsl_cpe_pipe.sh "alf ${FW_DIR}/${xDSL_Dbg_RemoteFwName}" > /dev/null
		echo "FwDbgEnv: Restart autoboot handler of DSL CPE API..."
		echo ""
		echo ""
		${BIN_DIR}/dsl_cpe_pipe.sh "acs 2" > /dev/null
	fi
}
