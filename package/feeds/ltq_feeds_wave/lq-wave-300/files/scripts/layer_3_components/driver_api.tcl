#!/bin/tclsh

source /tmp/ini.tcl

set driverApiHandler {}
set platform_log_level $env(log_level)
set wep ""

proc print2log {dbg_level dbg_string} {
	global platform_log_level

	if {$dbg_level == "INFO"} {
		if {$platform_log_level >= 2} {
			exec echo $dbg_string > /dev/console
		}
	} elseif {$dbg_level == "DBG"} {
		if {$platform_log_level >= 3} {
			exec echo $dbg_string > /dev/console
		}
	} else {
		exec echo $dbg_level $dbg_string > /dev/console
	}
}

proc find_wave_if_from_index {apIndex} {
	set cpeId [exec /usr/sbin/status_oper -f /ramdisk/flash/rc.conf GET "wlan_main" "wlmn_${apIndex}_cpeId"]
	set wlan [exec /usr/sbin/status_oper -f /tmp/wave_map_index GET "wave_map" "wlmap_${cpeId}"]
	return $wlan
}

######################################################
# TCL functions to be used by the driver api.ini
######################################################

proc test {condition ret_if_true ret_if_false} {
	if {[expr $condition]} { 
		return $ret_if_true
	} else {
		return $ret_if_false
	}
}

proc WEP_Set {interface param value} {
	if {$value == ""} {
		return
	}	
	
	if {$param == "WepEncryption"} {
		return "iwconfig $interface key [test $value==0 off on]"
	}  elseif {$param == "WepTxKeyIdx"} {
		return "iwconfig $interface key \[[expr $value+1]\] && iwconfig $interface key off"
	}
	
	set idx [string index $param "18"]
	incr idx
	if {[string first ' $value] == 0} {
		return "iwconfig $interface key \[$idx\] s:\"`printf \"%b\" $value`\""
	} else {
		return "iwconfig $interface key \[$idx\] [string range $value 2 end]"
	}
}

proc ESSID_set {interface value} {
	if {$value == ""} {
		return
	}
	return "iwconfig $interface essid \"`printf \"%b\" $value`\""
}

proc PeerAPMacs_Set {interface value} {
	set ret ""
	set PeerAPmacsList ""
	if {![catch {set val [eval "exec iwpriv $interface gPeerAPs"]}]} {
		set PeerAPmacsList [split $val]
	}
	set PeerAPhost_apiList [split "$value"]
	foreach mac $PeerAPhost_apiList {
		if { [ lsearch $PeerAPmacsList $mac ] == -1 } {
			append ret "iwpriv $interface sAddPeerAP $mac;"
		}
	}
	return "$ret"
}

proc set_AP_Authentication {interface value} {
	set encryption ""
	if {$::wep == 0} {
		set encryption "off"
	} 
	if {$value == 1} {
		return "iwconfig $interface key open $encryption"
	} elseif {$value == 2} {
		return "iwconfig $interface key restricted $encryption"
	} elseif {$value == 3} {
		return "iwconfig $interface key open restricted $encryption"
	}
}

proc VapSTALimits_Set {interface value} {
	set value [eval "exec echo $value"]
	return "iwpriv $interface sVapSTALimits $value"
}
proc ProcGet {interface param} {
	exec cat /proc/net/mtlk/$interface/$param
}

proc ProcSet {interface param value} {
	return "echo $value > /proc/net/mtlk/$interface/$param"
}	

proc ProcMtlkGet {param} {
	exec cat /proc/net/mtlk/$param
}

proc ProcMtlkSet {param value} {
	return "echo $value > /proc/net/mtlk/$param"
}

proc ProcLogSet {param value} {
	return "echo 0 cdebug=$value > /proc/net/mtlk_log/$param" 
}	

proc ProcLogGet {param} {
	exec cat /proc/net/mtlk_log/$param	
}

proc IWPRIV_Set {interface param value} {
	return "iwpriv $interface s$param $value"
}

proc IWPRIV_Get {interface param} {
	return "iwpriv $interface g$param"
}

proc iwconfig {interface param value} {
	if {$value == ""} {
		set value auto
	}
	return "iwconfig $interface $param $value"
}
proc iwpriv {interface param value} {
	return "iwpriv $interface $param $value"
}
proc iwgetid {interface param} {
	return "iwgetid $interface $param"
}

# execute the command, run regexp on the response and return first match
proc parse {cmd reg} {
	if {$cmd != ""} {
		if {![catch {set ret [eval "exec $cmd"]}]} {
			if {[regexp $reg $ret all match] > 0} {
				return $match
			}			
		}
	}
	return ""
}
###################################################
#  Driver API functions
#
#
###################################################

proc DriverGet {interface param} {
  set param [string trim $param]
  set retVal [getParamExecuteCmd GET $param]
  if {$retVal != ""} {
		set retVal [eval $retVal]
		
  }  
  return $retVal

}

proc DriverSet {interface param value} {
	set param [string trim $param]
	set value [string trim $value]
	if {$value == ""} {
		return
	}
	set retVal [getParamExecuteCmd SET $param]
	if {$retVal != ""} {				
		if {[catch {set ret [eval $retVal]} err]} {		
			return "# failed to eval $retVal, err: $err"
		}				
		return $ret
	}  
	return $retVal
}


proc setDriverApiHandler {db_file} {
	global driverApiHandler
	# load the ini file
	set driverApiHandler [IniOpen $db_file]	
}


proc getParamExecuteCmd {section param} {
	global driverApiHandler	
	# get the parameter key from the command section
	set retVal [IniGetKey $driverApiHandler $section $param]
	if {$retVal != ""} {
			set retVal1 [IniGetKey $driverApiHandler $retVal $section]
			if {$retVal1 != ""} {
				return $retVal1
			}	
	}
	return $retVal;	
}


proc DriverSetAll {indx param_file_name} {
	# Need to define wep is global parameter since set_AP_Authentication checks this parameter value
	global wep
	
	set interface [find_wave_if_from_index $indx]
	print2log "DBG" "interface = $interface"

	set conf_param [open "/tmp/set_driver_params_${indx}.sh" w]
	#for run conf_param.conf as sh script
	puts $conf_param {#!/bin/sh}
		
	set wep 0
	set network_type ""
	set Country ""
	set EEPROMCountryValid 0
	set Key0 ""
	set Key1 ""
	set Key2 ""
	set Key3 ""
	set KeyIndex ""
	set Authentication ""
	set dot11dActive ""
	set channel ""
	set BridgeMode ""
	set MacCloningAddr ""
	set PeerAPMacs ""
	set WDSEnabled ""
	set AocsRestrictCh ""
	set contents [exec cat $param_file_name]
			
	#Update parameters according to DB. 
	set contents [split $contents "\n"]
	foreach line $contents {		
		if {[regexp {(.*)=(.*)} $line all param value] == 1} {			
			if {$param == "WepEncryption"} {
				set wep $value
				if {$value != 0} {
					#disable the WEP as soon as possible, enable it after the network mode is set
					continue
				}
			}
			
			if {$param == "network_type"} {
				set network_type $value
				continue
			}
			
			if {$param == "Country"} {
				set Country $value
				continue
			}
			
			if {$param == "EEPROMCountryValid"} {
				set EEPROMCountryValid $value
				continue
			}
			
			if {$param == "WepKeys_DefaultKey0" } {
				set Key0 $value
				continue
			}

			if {$param == "WepKeys_DefaultKey1" } {
				set Key1 $value
				continue
			}

			if {$param == "WepKeys_DefaultKey2" } {
				set Key2 $value
				continue
			}

			if {$param == "WepKeys_DefaultKey3" } {
				set Key3 $value
				continue
			}

			if {$param == "WepTxKeyIdx" } {
				set KeyIndex $value
				continue
			}

			if {$param == "NonProc_Authentication" } {
				set Authentication $value
				continue
			}

			if {$param == "dot11dActive"} {
				set dot11dActive $value
				continue
			}

			if {($param == "AclMode") && ($value != 0)} {
				puts $conf_param "iwpriv $interface sDelACL 00:00:00:00:00:00"
			}

			if {$param == "Channel"} {
				set channel $value
				continue
			}

			if {$param == "BridgeMode"} {
				set BridgeMode $value
				continue
			}

			if {$param == "MacCloningAddr"} {
				set MacCloningAddr $value
				continue
			}
			
			if {$param == "PeerAPkeyIdx"} {
				set  PeerAPkeyIdx $value
				continue
			}
			
			if {$param == "PeerAPMacs"} {
				set PeerAPMacs $value
				continue
			}
			
			if {$param == "WDSEnabled"} {
				set WDSEnabled $value
			}
			
			if {$param == "AocsRestrictCh"} {
                        	set AocsRestrictCh $value
				continue
			}

			set ret [DriverSet $interface $param $value]
			if {$ret != ""} {
				puts $conf_param $ret
				#eval "exec $ret"
			}
		}
	}
	
	# country setting only required for physical AP
	if {([string compare $interface "wlan0"] == 0) || ([string compare $interface "wlan1"] == 0)} {
		if {($EEPROMCountryValid != "") && ($network_type != "")} {
			print2log "DBG" "interface = $interface, Country=$Country"
			if {($EEPROMCountryValid == 0) && ($network_type == 2) } {
				set ret [DriverSet $interface "Country" $Country]
				if {$ret != ""} {
					puts $conf_param $ret
					#eval "exec $ret"
				}
			}
		
			if {($EEPROMCountryValid == 0) && ($network_type == 0) && ($dot11dActive == 0)} {
				if {$Country == ""} {
					set Country "US"
				}
				set ret [DriverSet $interface "Country" $Country]
				if {$ret != ""} {
					puts $conf_param $ret
					#eval "exec $ret"
				}
			}
		}
	}
	
	# Disable restricted channels only if manual channel is set.
	if {$AocsRestrictCh != ""} {
		if {$channel != 0} {
			set AocsRestrictCh 0
		}
		set ret [DriverSet $interface AocsRestrictCh $AocsRestrictCh]
		if {$ret != ""} {
			puts $conf_param $ret
		}
	}

	# Channel can only be set after network_mode and Country were set
	if {$channel != ""} {
		set ret [DriverSet $interface "Channel" $channel]
		if {$ret != ""} {
		        puts $conf_param $ret
		}
	}
	
	# this is an ugly workaround, to make sure that we set WepEncryption after we done with the keys
	# write the keys:
	# 	befor enable wep
	#	only if wep is enabled (if not HT - the Web doesn't allow to set wep if HT mode selected)
	#	i f {($network_mode != "12") && ($network_mode != "14") && ($network_mode != "20") && ($network_mode != "22") && ($network_mode != "23")}
	if {$wep != 0 || $WDSEnabled == 1} {
		set errorKey 0
		for {set i 0} {$i<4} {incr i} {
			set ret [DriverSet $interface "WepKeys_DefaultKey$i" [set Key$i]]
			if {$ret != ""} {
				set errorKey 1
				puts $conf_param $ret
			}
		}
		if {$errorKey == 0 && $wep != 0} {
			set ret [DriverSet $interface "WepTxKeyIdx" $KeyIndex]
			if {$ret != ""} {
				puts $conf_param $ret
			}
			set ret [DriverSet $interface "WepEncryption" $wep]
			if {$ret != ""} {
				puts $conf_param $ret
			}
		}
	}
	
	if {$Authentication != ""} {
		set ret [DriverSet $interface "Authentication" $Authentication]
		if {$ret != ""} {
			puts $conf_param $ret
		}
	}
	if {$BridgeMode != ""} {
		# If MAC Cloning is configured and MAC address is set, handle it to the driver
		if {$BridgeMode == "3"} {
			if {$MacCloningAddr != ""} {
				set ret [DriverSet $interface "MAC" $MacCloningAddr]
				if {$ret != ""} {
					puts $conf_param $ret
				}
			}
		}	
		# If L2NAT is configured, handle Local MAC to the driver
		if {$BridgeMode == "2"} {
			set L2NAT_LocMAC [exec ifconfig br0 | awk {NR<2 {print $5}}]
			set ret [DriverSet $interface "L2NAT_LocMAC" $L2NAT_LocMAC]
			if {$ret != ""} {
				puts $conf_param $ret
			}
		}	
	}
	
	catch {set value [exec host_api get $$ sys BridgeMode]}	
	if {$value != ""} {
		set ret [DriverSet $interface BridgeMode $value]
		if {$ret != ""} {
			puts $conf_param $ret
			#eval "exec $ret"
		}
	}
	
	#puts $conf_param "echo DriverSET Param is DONE"
	
	#set wds parameters
	if {$WDSEnabled == 1} {
		set ret [DriverSet $interface "PeerAPkeyIdx" $PeerAPkeyIdx]
		if {$ret != ""} {
			puts $conf_param $ret
		}
		set ret [DriverSet $interface "PeerAPMacs" $PeerAPMacs]
		if {$ret != ""} {
			puts $conf_param $ret
		}
	}
	close $conf_param
	
	catch {exec chmod +x /tmp/set_driver_params_${indx}.sh }
}


# Create a script to get all params from the driver.
# Used for creating a list of the default values of the driver 
# (Call after insmod and before setting any params, after restore defaults)
proc DriverGetAll {interface conf_file} {
	
	# Name of default file to create(TODO: Remove hardcoded path)
	set def_file [open "/mnt/jffs2/drv.def" w]
	
	if {[file readable $conf_file] ==  1} {
		set fp [open $conf_file r]
		set contents [read $fp]
		close $fp
	} else {
		puts "echo Can not read file $conf_file"
		return ''
	}
	
	#for run conf_param.conf as sh script
	
	#Update parameters according to DB. 
	set contents [split $contents "\n"]
	set sed_cmd {sed -r s/\[^:\]*://}
	foreach line $contents {
		if {[regexp {(.*) = (.*)} $line all param value] == 1} {
			set ret [DriverGet $interface $param]
			if {$ret != ""} {
				# Ugly hack: ret contains either a sh cmd (e.g. iwpriv) or the value from parse 
				# exec the cmd and sed to remove iwpriv header, or use the value directly
				if {![catch {set val [eval "exec $ret | $sed_cmd"]}]} {
					puts $def_file "$param = [string trim $val]"
				} else {
					puts $def_file "$param = $ret"
				}
			} 
		}
	}

	# Get BridgeMode: a driver parameter but we hold it in the sys.conf
	set ret [DriverGet $interface BridgeMode]
	if {![catch {set val [eval "exec $ret | $sed_cmd"]}]} {
		puts $def_file "BridgeMode = [string trim $val]"
	}
	
	close $def_file
}


#usage for command line interface
if {[info exists ::argv]} {
	set driverApiHandler [IniOpen /tmp/driver_api.ini]
	if {[lindex $argv 0] == "DriverSetAll"} {
		DriverSetAll [lindex $argv 1] [lindex $argv 2]
	}
	if {[lindex $argv 0] == "DriverGetAll"} {		
		DriverGetAll [lindex $argv 1] [lindex $argv 2]
	}
	if {[lindex $argv 0] == "DriverParamSet"} {
		set ret [DriverSet [lindex $argv 1] [lindex $argv 2] [lindex $argv 3]]
		set exe_list [split $ret ";"]		
		foreach cmd $exe_list {
			if {$cmd != ""} {
				eval "exec $cmd"
			}
		}
	}			
} else {
	set driverApiHandler {}
}
