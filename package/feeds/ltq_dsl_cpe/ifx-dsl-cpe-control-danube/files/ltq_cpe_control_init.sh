#!/bin/sh /etc/rc.common
# Copyright (C) 2013 OpenWrt.org
START=22
xTSE=""

BIN_DIR=/opt/lantiq/bin
#FW_DIR=/lib/firmware/`uname -r`
FW_DIR=/firmware
INIT_DIR=/etc/init.d
xDSL_CtrlAppName="ltq_cpe_control_init.sh"

if [ ! "$CONFIGLOADED" ]; then
   if [ -r /etc/rc.d/config.sh ]; then
      . /etc/rc.d/config.sh 2>/dev/null
      CONFIGLOADED="1"
   fi
fi

# Parameters that are externally defined and which are of relevance within
# this script handling
# Starting with "xDSL_Cfg_xxx" or "xDSL_Dbg_xx" are defined within dsl.cfg
start() {

   DBG_TEST_IF=""
   DTI_IF_STR=""
   TCPM_IF_STR=""
   AUTOBOOT_ADSL=""
   NOTIFICATION_SCRIPT=""
   DEBUG_CFG=""
   TCPM_IF=""
   DTI_IF=""
   XTM_MULTIMODE=""
   FW_FILENAME=""
   FW_FOUND=0
   START_CTRL=0

   # This default initializations will be overwritten with external values defined
   # within rc.conf. In case of inconsistencies within rc.conf it takes care that
   # this script can be executed without problems using default settings
   xTM_Mgmt_Mode=""
   wan_mode="ADSL"
   BS_ENA=1
   SRA_ENA=1
   RETX_ENA=0
   CNTL_MODE_ENA=0
   CNTL_MODE=0

   echo "0" > /tmp/adsl_status
   if [ -r /etc/rc.conf ]; then
      . /etc/rc.conf 2> /dev/null
   fi

   if [ -r ${BIN_DIR}/dsl.cfg ]; then
      . ${BIN_DIR}/dsl.cfg 2> /dev/null
   fi

   # This script checks if one of primary or secondary wan modes is DSL and returns the values.
   # Current running DSL Phy and TC  - get_phy_tc_info
   # xTSE bits for the current running mode in X_X_X_X_X_X_X_X format - calc_xtse
   if [ -e /etc/rc.d/ltq_dsl_functions.sh ]; then
      . /etc/rc.d/ltq_dsl_functions.sh
   fi
   # get_phy_tc_info fucntion returns wan_mode = VDSL or ADSL.
   # nTC_Mode - 1 - ATM, 2- PTM, 4 - Auto
   get_phy_tc_info
   if [ "$wan_mode" = "ADSL" ]; then

      echo `cat /proc/modules` | grep -q "drv_dsl_cpe_api" && {
         START_CTRL=1
      }

      if [ -e ${BIN_DIR}/adsl.scr ]; then
         AUTOBOOT_ADSL="-a ${BIN_DIR}/adsl.scr"
      fi

      if [ -e ${INIT_DIR}/xdslrc.sh ]; then
         NOTIFICATION_SCRIPT="-n ${INIT_DIR}/xdslrc.sh"
      fi

      if [ -e ${FW_DIR}/dsl_firmware_a.bin ]; then
         FW_FILENAME="dsl_firmware_a.bin"
         FW_FOUND=1
      elif [ -e ${FW_DIR}/dsl_firmware_b.bin ]; then
         FW_FILENAME="dsl_firmware_b.bin"
         FW_FOUND=1
      # Backward compatibility only
      elif [ -e ${FW_DIR}/modemhwe.bin ]; then
         FW_FILENAME="modemhwe.bin"
         FW_FOUND=1
      fi

      # Check if debug capabilities are enabled within dsl_cpe_control
      # and configure according settings if required
      echo `${BIN_DIR}/dsl_cpe_control -h` | grep -q "(-D)" && {
         if [ "$xDSL_Dbg_DebugLevel" != "" ]; then
            DEBUG_LEVEL_COMMON="-D${xDSL_Dbg_DebugLevel}"
            if [ "$xDSL_Dbg_DebugLevelsApp" != "" ]; then
               DEBUG_LEVELS_APP="-G${xDSL_Dbg_DebugLevelsApp}"
            fi
            if [ "$xDSL_Dbg_DebugLevelsDrv" != "" ]; then
               DEBUG_LEVELS_DRV="-g${xDSL_Dbg_DebugLevelsDrv}"
            fi
            DEBUG_CFG="${DEBUG_LEVEL_COMMON} ${DEBUG_LEVELS_DRV} ${DEBUG_LEVELS_APP}"
            echo "${xDSL_CtrlAppName}: TestCfg: DEBUG_CFG=${DEBUG_CFG}"
         else
            if [ -e ${BIN_DIR}/debug_level.cfg ]; then
               # read in the global definition of the debug level
               . ${BIN_DIR}/debug_level.cfg 2> /dev/null

               if [ "$ENABLE_DEBUG_OUTPUT" != "" ]; then
                  DEBUG_CFG="-D${ENABLE_DEBUG_OUTPUT}"
               fi
            fi
         fi
      }

      # Usage of debug and test interfaces (if available).
      # Configuration from dsl.cfg
      case "$xDSL_Dbg_DebugAndTestInterfaces" in
         "0")
            # Do not use interfaces, empty string is anyhow default (just in case)
            DTI_IF_STR=""
            TCPM_IF_STR=""
            ;;
         "1")
            # Use LAN interfaces for debug and test communication
            DBG_TEST_IF=`ifconfig br0 | grep -E -o \
               'addr:([0-9]{1,3}?\.){3}([0-9]{1,3}?{1})' | cut -d':' -f2`
            if [ "$DBG_TEST_IF" != "" ]; then
               DTI_IF_STR="-d${DBG_TEST_IF}"
               TCPM_IF_STR="-t${DBG_TEST_IF}"
            else
               echo "${xDSL_CtrlAppName}: ERROR processing LAN IP-Address " \
                  "(no test and debug functionality available)!"
            fi
            ;;
         "2")
            # Use all interfaces for debug and test communication
            DTI_IF_STR="-d0.0.0.0"
            TCPM_IF_STR="-t0.0.0.0"
            ;;
      esac

      echo `${BIN_DIR}/dsl_cpe_control -h` | grep -q "(-d)" && {
         DTI_IF="${DTI_IF_STR}"
      }

      echo `${BIN_DIR}/dsl_cpe_control -h` | grep -q "(-t)" && {
         TCPM_IF="${TCPM_IF_STR}"
      }

      # Special test and debug functionality to use multimode realted
      # configuration for initial SystemInterface configuration
      if [ "$xDSL_Cfg_SystemInterface" != "" ]; then
         # Use multimode realted configuration from dsl.cfg
         XTM_MULTIMODE="-T${xDSL_Cfg_SystemInterface}"
         echo "${xDSL_CtrlAppName}: TestCfg: XTM_MULTIMODE=${XTM_MULTIMODE}"
      else
         # Use multimode realted configuration from UGW system level
         # configuration (rc.conf)
         if [ "$nTC_Mode" != "" ]; then
            XTM_MULTIMODE="-T$nTC_Mode:0x1:0x1"
         else
            XTM_MULTIMODE=""
         fi
      fi

      ##########################################################################
      # start dsl cpe control application with appropriate options

      if [ ${FW_FOUND} = 0 -o ${START_CTRL} = 0 ]; then
         echo "${xDSL_CtrlAppName}: API *not* started due to the following reason"
         if [ ${FW_FOUND} = 0 ]; then
            echo "${xDSL_CtrlAppName}: -> No firmware binary available within '${FW_DIR}'"
         fi
         if [ ${START_CTRL} = 0 ]; then
            echo "${xDSL_CtrlAppName}: -> API driver (drv_dsl_cpe_api) not installed within system"
         fi
      else
         # call the function to calculate the xTSE bits
         calc_xtse

         # Special test and debug functionality uses xTSE configuration from dsl.cfg
         if [ "$xDSL_Cfg_G997XTU" != "" ]; then
            xTSE="${xDSL_Cfg_G997XTU}"
            echo "${xDSL_CtrlAppName}: TestCfg: xTSE=${xTSE}"
         fi

         # Special test and debug functionality uses Bitswap configuration from dsl.cfg
         if [ "$xDSL_Cfg_BitswapEnable" != "" ]; then
            BS_ENA="${xDSL_Cfg_BitswapEnable}"
            echo "${xDSL_CtrlAppName}: TestCfg: BS_ENA=${BS_ENA}"
         fi

         # Special test and debug functionality uses ReTx configuration from dsl.cfg
         if [ "$xDSL_Cfg_ReTxEnable" != "" ]; then
            RETX_ENA="${xDSL_Cfg_ReTxEnable}"
            echo "${xDSL_CtrlAppName}: TestCfg: RETX_ENA=${RETX_ENA}"
         fi

         # Special test and debug functionality to activate DSL related kernel prints
         if [ "$xDSL_Dbg_EnablePrint" == "1" ]; then
            echo 7 > /proc/sys/kernel/printk
         fi

         # start DSL CPE Control Application in the background
         ${BIN_DIR}/dsl_cpe_control ${DEBUG_CFG} -i${xTSE} -f ${FW_DIR}/${FW_FILENAME} \
            ${XTM_MULTIMODE} ${AUTOBOOT_ADSL} ${NOTIFICATION_SCRIPT} \
            ${TCPM_IF} ${DTI_IF} &

         PS=`ps`
         # Timeout to wait for dsl_cpe_control startup [in seconds]
         iLp=10
         echo $PS | grep -q dsl_cpe_control && {
            # workaround for nfs: allow write to pipes for non-root
            while [ ! -e /tmp/pipe/dsl_cpe1_ack -a $iLp -gt 0 ] ; do
               iLp=`expr $iLp - 1`
               sleep 1;
            done

            if [ ${iLp} -le 0 ]; then
               echo "${xDSL_CtrlAppName}: Problem with pipe handling, exit" \
                  "dsl_cpe_control startup!!!"
               false
            fi

            chmod a+w /tmp/pipe/dsl_*
         }
         echo $PS | grep -q dsl_cpe_control || {
            echo "${xDSL_CtrlAppName}: Start of dsl_cpe_control failed!!!"
            false
         }

         # Special test and debug functionality to activate event console prints
         if [ "$xDSL_Dbg_EnablePrint" == "1" ]; then
            tail -f /tmp/pipe/dsl_cpe0_event &
         fi

         sleep 1
         /usr/sbin/status_oper SET BW_INFO max_us_bw "512"

         # If BitSwap is disabled, or Retransmission is enabled
         # then set the DSL Parameters accordingly
         if [ "$BS_ENA" != "1" -o "$RETX_ENA" = "1" ]; then
            dir="0 1"
            for j in $dir ; do
               LFCG_VALS=`${BIN_DIR}/dsl_cpe_pipe.sh lfcg $j`
               if [ "$?" = "0" ]; then
                  for i in $LFCG_VALS; do eval $i 2>/dev/null; done
                  ${BIN_DIR}/dsl_cpe_pipe.sh lfcs $nDirection \
                     $bTrellisEnable $BS_ENA $RETX_ENA $bVirtualNoiseSupport \
                     $b20BitSupport >/dev/null
               else
                  if [ "$j" = "0" ]; then
                     ${BIN_DIR}/dsl_cpe_pipe.sh lfcs $j 1 $BS_ENA $RETX_ENA \
                        0 -1 >/dev/null
                  else
                     ${BIN_DIR}/dsl_cpe_pipe.sh lfcs $j 1 $BS_ENA $RETX_ENA \
                        0 0 >/dev/null
                  fi
               fi
            done
            sleep 1
         fi
         # If SRA is disabled, RA_MODE to be set AT_INIT(2) State.
         # By Default DSL CPE configurations have SRA enabled and RA_MODE = Dynamic (3)
         if [ "$SRA_ENA" = "0" ]; then
           ${BIN_DIR}/dsl_cpe_pipe.sh g997racs 0 2 >/dev/null
           ${BIN_DIR}/dsl_cpe_pipe.sh g997racs 1 2 >/dev/null
         fi
         sleep 1
         if [ "$CNTL_MODE_ENA" = "1" ]; then
            if [ "$CNTL_MODE" = "0" ]; then
               ${BIN_DIR}/dsl_cpe_pipe.sh tmcs 1 >/dev/null
            elif [ "$CNTL_MODE" = "1" ]; then
               ${BIN_DIR}/dsl_cpe_pipe.sh tmcs 2 >/dev/null
            fi
         fi

         if [ "$xDSL_Cfg_RebootCritSet" == "1" ]; then
            ${BIN_DIR}/dsl_cpe_pipe.sh rccs $xDSL_Cfg_RebootCritVal >/dev/null
         fi

         ${BIN_DIR}/dsl_cpe_pipe.sh acs 2
      fi
   fi
}

stop() {
   ${BIN_DIR}/dsl_cpe_pipe.sh acos 1 1 1 0 0 0
   ${BIN_DIR}/dsl_cpe_pipe.sh acs 2
   sleep 3
   ${BIN_DIR}/dsl_cpe_pipe.sh acs 0
   ${BIN_DIR}/dsl_cpe_pipe.sh quit
}
