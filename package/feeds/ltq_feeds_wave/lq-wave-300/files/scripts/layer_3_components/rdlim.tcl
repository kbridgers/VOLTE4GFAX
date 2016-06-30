#!/bin/tclsh

set platform_log_level $env(log_level)

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

proc Configure80211D {{VendorID ""} {DeviceID ""} {HW_TYPE ""} {HW_REVISION ""} {wlan ""}} {
	
	set HWkey "${VendorID}_${DeviceID}_${HW_TYPE}_${HW_REVISION}"
	print2log "INFO" "HWKey is $HWkey"
	
	# load the ini file
	if {[catch {set handler [IniOpen /tmp/rdlim.ini]}]} {
		puts "ERROR: Can't create handler file"
		exit 1
	}
	
	# search HW type
	print2log "INFO" "Searching key $HWkey"
	
	set HwSection [IniGetKey $handler HWTypes $HWkey]
	
	if {$HwSection == ""} {
		exec echo "ERROR: HwSection isn't found" > /dev/console
		exit 1
	}
	
	set out [open "/ramdisk/flash/hw_$wlan.ini" w]
	
		
	puts $out "HW_name=$HwSection"
	puts $out "HW_TYPE=$HW_TYPE"
	if {[catch {set sec [array names ::inihandler "$HwSection#*"]}]} {
		puts "ERROR Can't write hw_$wlan.ini file"
	}
	foreach key $sec {
		set HWitem [lindex [split $key #] 1]
		set item [IniGetKey $handler $HwSection $HWitem]
		puts $out "${HWitem}=${item}"
	}
	close $out	
}

#puts "###################################################################"
print2log "INFO" "Start of RDLIM init section"
#puts "###################################################################"

if {[info exists ::argv]} {
	set wlan [lindex $argv 0]
} else {
	print2log "WARNING" "wlan not defined"
	exit
}


source /tmp/ini.tcl

set VendorID 0
set DeviceID 0
set HW_TYPE ""
set HW_REVISION ""
set HW_TYPE ""
set HW_REVISION ""
set HW_ID ""

set eeprom_info [exec iwpriv $wlan gEEPROM]
regexp {HW type       : (0x[A-Z0-9]+)} $eeprom_info Dummy HW_TYPE
regexp {HW revision   : (0x[A-Z0-9]+)} $eeprom_info Dummy HW_REVISION
regexp {HW ID         : (([^,]*),([^,]*))} $eeprom_info Dummy HW_ID VendorID DeviceID

#make sure that number of digits is correct
set VendorID [format {0x%04x} $VendorID]
set DeviceID [format {0x%04x} $DeviceID]
set HW_TYPE [format {0x%02x} $HW_TYPE]
set HW_REVISION [format {0x%02x} $HW_REVISION]


print2log "DBG" "# HWID = $HW_ID HWtype = $HW_TYPE HWrevision = $HW_REVISION"
print2log "DBG" "# VendorID=$VendorID DeviceID=$DeviceID "

Configure80211D $VendorID $DeviceID $HW_TYPE $HW_REVISION $wlan



#puts "###################################################################"
print2log "INFO"  "END of RDLIM init section"
#puts "###################################################################"
