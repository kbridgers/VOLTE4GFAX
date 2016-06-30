# This script may be called from any other script to obtain the following information
# FW Type  
# Current running DSL Phy and TC  
# xTSE bits for the current running mode in X_X_X_X_X_X_X_X format

if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
    	. /etc/rc.conf 2> /dev/null
	fi
fi

BIN_DIR=/opt/lantiq/bin

if [ -r ${BIN_DIR}/dsl.cfg ]; then
   . ${BIN_DIR}/dsl.cfg 2> /dev/null
fi


# These variables have the corresponding bit position in xTSE*

# Funtion to generate the BitMask for ADSL Annex A mode.
# Using this bitmask, it can be identified whehter Annex A bits are set or not
# Annex A
generate_annexA_bitmask() {
   g992_1a=$(( 1<< 2 ))
   g992_2=$((  1<< 0 ))
   g992_3a=$(( 1<< 2 ))
   g992_3i=$(( 1<< 4 ))
   g992_3l=12 # It has two bit position enabled,
   # Hardcoded $( (  1<< 2 ) |  (1<< 3) )

   g992_3m=$(( 1<< 6 ))
   g992_5a=$(( 1<< 0 ))
   g992_5i=$(( 1<< 6 ))
   g992_5m=$(( 1<< 2 ))
   t1_413=$(( 1<< 0 ))
}
# Funtion to generate the BitMask for ADSL Annex B mode.
# Using this bitmask, it can be identified whehter Annex B bits are set or not
# Annex B
generate_annexB_bitmask() {
   g992_1b=$(( 1<< 4 ))
   g992_3b=$(( 1<< 4 ))
   g992_3j=$(( 1<< 6 ))
   g992_5b=$(( 1<< 2 ))
   g992_5j=$(( 1<< 0 ))
}

# Funtion to check if any of the AnnexA bits are set in the given configuration.
validate_annexA_bits() {
    annex_a_bit_set_status="false"
    generate_annexA_bitmask

    if [ $(( ${xTSE_Octet_1} & g992_1a )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_2} & g992_2 )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_3} & g992_3a )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_4} & g992_3i )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_5} & g992_3l )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_5} & g992_3m )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_6} & g992_5a )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_6} & g992_5i )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_7} & g992_5m )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_1} & t1_413 )) -gt 0 ]; then

        annex_a_bit_set_status="true"
        # echo Some Firmware A Bit is Set
    else
        annex_a_bit_set_status="false"
        # echo No Firmware A Bit is Set
    fi

}

# Funtion to check if any of the AnnexB bits are set in the given configuration.
validate_annexB_bits() {

    annex_b_bit_set_status="false"
    generate_annexB_bitmask

    if [ $(( ${xTSE_Octet_1} & g992_1b )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_3} & g992_3b )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_4} & g992_3j )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_6} & g992_5b )) -gt 0 ] ||
        [ $(( ${xTSE_Octet_7} & g992_5j )) -gt 0 ]; then

        annex_b_bit_set_status="true"
        # echo Some Firmware B Bit is Set
    else
        annex_b_bit_set_status="false"
        # echo No Firmware B Bit is Set
    fi

}

get_what_string() {
    for i in $1; do
        FW_WHAT_STRING=`strings $i | grep "@(#)"`
        # echo $version
    done
}

# Get the Type of FW loaded and update teh system Status
# The last digit of the xDSL firmware version specifies the Annex type
# 1 - ADSL Annex A
# 2 - ADSL Annex B
# 6 - VDSL
# 7 - Vectoring
get_fw_type() {
	if [ ! "$CONFIGLOADED" ]; then
	    if [ -r /etc/rc.d/config.sh ]; then
    	    . /etc/rc.d/config.sh 2> /dev/null
	    fi
	fi

    FW_TYPE=""
    VDSL_FW_TYPE="NA"
    FW_DIR=/firmware
    FW_FILENAME=""
    FW_WHAT_STRING=""

    if [ -e ${FW_DIR}/dsl_firmware_a.bin ]; then
        FW_FILENAME="dsl_firmware_a.bin"
    elif [ -e ${FW_DIR}/dsl_firmware_b.bin ]; then
        FW_FILENAME="dsl_firmware_b.bin"
        # Backward compatibility only
    elif [ -e ${FW_DIR}/modemhwe.bin ]; then
        FW_FILENAME="modemhwe.bin"
    elif [ -e ${FW_DIR}/xcpe_hw.bin ]; then
        FW_FILENAME="xcpe_hw.bin"
    fi

    get_what_string ${FW_DIR}/${FW_FILENAME}

    # Determine the type of the ADSL Firmware which is mounted
    if [ "$CONFIG_PACKAGE_IFX_DSL_CPE_API_VRX" = "1" -o "$CONFIG_PACKAGE_IFX_DSL_CPE_API_VRX_BONDING" = "1" ]; then
        # For VRX platform there is always a combined VDSL/ADSL firmware binary used.
        # Therefore two what strings (version numbers) are included within binary.
        # The format of the extracted what strings has to have to following format
        # /firmware/xcpe_hw.bin : @(#)V_5.3.1.A.1.6 @(#)5.3.3.0.1.1
        #                               +---------+     +---------+
        #                              VDSL FW Vers.    ADSL FW Vers.
        FW_TYPE=`echo $FW_WHAT_STRING | cut -d'.' -f11 | cut -d'_' -f1 | cut -d' ' -f1`
        VDSL_FW_TYPE=`echo $FW_WHAT_STRING | cut -d'.' -f6 | cut -d'_' -f1 | cut -d' ' -f1`
    elif [ "$CONFIG_PACKAGE_IFX_DSL_CPE_API_DANUBE" = "1" ]; then
        # For ADSL only platforms (Danube, Amazon-SE, ARX100) there is only one
        # what string (version number) included.
        # The format of the extracted what strings has to have to following format
        # /firmware/dsl_firmware_a.bin : @(#)4.4.7.B.0.1
        #                                    +---------+
        #                                   ADSL FW Vers.
        FW_TYPE=`echo $FW_WHAT_STRING | cut -d'.' -f6 | cut -d'_' -f1 | cut -d' ' -f1`
    fi
#    echo "FW_TYPE = $FW_TYPE" 

    # if Annex A, set type as "a", else if Annex B, set type as "b"
    if [ $FW_TYPE = "1" ]; then
        /usr/sbin/status_oper SET "dsl_fw_type" "type" "a" "v_fw_type" $VDSL_FW_TYPE  # writing firmware type in /tmp/system_status file
    elif [ $FW_TYPE = "2" ]; then
        /usr/sbin/status_oper SET "dsl_fw_type" "type" "b" "v_fw_type" $VDSL_FW_TYPE	# writing firmware type in /tmp/system_status file
    fi
        
}

# Get the xTSE bits from rc.conf and check if the value for xTSE matches the 
# xTSE bit setting for the loaded firmware. 
# if Not, then update the default xTSE bit setting
validate_fw_bits() {
    # Get the complete bit string (inclusive of Annex A and B)
    # ADSL_MODE is read from rc.conf
    xTSE_Value=$ADSL_MODE
    
    for i in 1 2 3 4 5 6 7 8
    do
        xtse_bit_hex=`echo $xTSE_Value | cut -d "_" -f$i`
        xtse_bit_int=$(printf %d\\n 0x${xtse_bit_hex})

        if [ $FW_TYPE = "1" -a "$xDSL_Cfg_ActSeq" == "1" ]; then
            xtse_bit_int=$((xtse_bit_int|t1_413))
        fi
        eval xTSE_Octet_${i}=$xtse_bit_int
    done

    # get the DSL Phy and TC mode configured
    get_phy_tc_info

    if [ "$wan_mode" != "AUTO" -a "$wan_mode" = "VDSL" ]; then
        ADSL_MODE="00_00_00_00_00_00_00_07"
        xTSE=$ADSL_MODE
        /usr/sbin/status_oper -u -f /etc/rc.conf SET "adsl_phy" "ADSL_MODE" $ADSL_MODE "xDSL_MODE" "$wan_mode"
        return
    fi
    # Parse Annex A related bits if the mounted firmware type is Annex A.
    # If the Management mode is VDSL or if the parsed bits match Annex A, do nothing.
    # Else, reset the ADSL_MODE and xDSL_MODE parameter to default Annex A supported values
    # and store the values in rc.conf.
    if [ $FW_TYPE = "1" ]; then

        validate_annexA_bits

        # Restore Annex A bits in the config
        if [ $annex_a_bit_set_status = "false" ]; then
            if [ "$wan_mode" = "AUTO" ]; then
                xDSL_MODE="Multimode-xDSL"
                ADSL_MODE="05_00_04_00_0C_01_00_07"
            else
                xDSL_MODE="Multimode-ADSL"
                ADSL_MODE="05_00_04_00_0C_01_00_00"
            fi
            /usr/sbin/status_oper -u -f /etc/rc.conf SET "adsl_phy" "ADSL_MODE" $ADSL_MODE "xDSL_MODE" $xDSL_MODE
        fi
    # If mounted firmware is Annex B, parse Annex B related bits.
    # If the Management mode is VDSL or if the parsed bits match Annex B, do nothing.
    # Else, reset the ADSL_MODE and xDSL_MODE parameter to default Annex B supported values
    # and store the values in rc.conf.
    elif [ $FW_TYPE = "2" ]; then

        validate_annexB_bits

        # Restore Annex B bits in the config
        if [ $annex_b_bit_set_status = "false" ]; then
            if [ "$wan_mode" = "AUTO" ]; then
                xDSL_MODE="Multimode-xDSL"
                ADSL_MODE="10_00_10_00_00_04_00_07"
            else
                ADSL_MODE="10_00_10_00_00_04_00_00"
                xDSL_MODE="Multimode-ADSL"
            fi
            /usr/sbin/status_oper -u -f /etc/rc.conf SET "adsl_phy" "ADSL_MODE" $ADSL_MODE "xDSL_MODE" $xDSL_MODE
        fi
    fi
    xTSE=$ADSL_MODE
}

calc_xtse() {
    get_fw_type
    validate_fw_bits
}

# Funtion to get teh phy and tc mode configured 
# this is helpful in getting the dsl mode even if the not-active wan of dual wan is DSL.

# nDSL_Mode - 0 - ADSL, 1 - VDSL
# nTC_Mode - 1 - ATM, 2 - PTM, 4 - Auto
get_phy_tc_info() {
   wan_mode="NOT_DSL"
   nDSL_Mode=""
   nTC_Mode=""
   if [ "$CONFIG_FEATURE_DUAL_WAN_SUPPORT" = "1" -a "$dw_failover_state" = "1" ]; then
      if [ "$dw_pri_wanphy_phymode" = "0" -o "$dw_pri_wanphy_phymode" = "3" ]; then
         wanphy_setphymode="$dw_pri_wanphy_phymode"
         wanphy_settc="$dw_pri_wanphy_tc"
      elif [ "$dw_sec_wanphy_phymode" = "0" -o "$dw_sec_wanphy_phymode" = "3" ]; then
         wanphy_setphymode="$dw_sec_wanphy_phymode"
         wanphy_settc="$dw_sec_wanphy_tc"
      fi
   fi
   if [ "$wanphy_setphymode" = "4" ]; then
      wan_mode="AUTO"
      nDSL_Mode="0 1"
      if [ "$wanphy_phymode" = "3" ]; then
         next_mode="VDSL"
      elif [ "$wanphy_phymode" = "0" ]; then
         next_mode="ADSL"
      fi
   elif [ "$wanphy_setphymode" = "3" ]; then
      wan_mode="VDSL"
      nDSL_Mode="1"
   elif [ "$wanphy_setphymode" = "0" ]; then
     wan_mode="ADSL"
     nDSL_Mode="0"
   fi
   if [ "$wanphy_settc" = "0" ]; then
      nTC_Mode="1"
   elif [ "$wanphy_settc" = "1" ]; then
      nTC_Mode="2"
   elif [ "$wanphy_settc" = "2" ]; then
      nTC_Mode="4"
   fi
}
