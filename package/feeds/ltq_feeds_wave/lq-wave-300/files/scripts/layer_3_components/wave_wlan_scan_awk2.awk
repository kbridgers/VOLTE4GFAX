	######################################################################################
	# Start AWK2: process the security

	BEGIN {i = 0; wep = 1; wpa = 2; wpa2 = 3; radius = 4; loop = -1; wps = 0; line_mirr = 0; one = 1; print "" > "/tmp/scan_res1"
	;search_beacon_t_string="DEBUG wep_.+=;DEBUG WPA_WPA2_.+=;DEBUG WPA2_.+=;DEBUG Radius_.+=;CCMP_TKIP_.+=" \
	;split(search_beacon_t_string,search_beacon_t,";") \
	;search_auth="DEBUG auth_.+="; search_enc="DEBUG CCMP_TKIP_.+="; search_rssi="DEBUG signal_strength_.+=-";\
	;getvar_string=";;;" \
	;split(getvar_string,getvar,";") \
	;caption_string="wep:wpa:wpa2:radius"\
	;split(caption_string,caption,":") \
	} \

	# Algoritm:
	# run over file, get parameters per AP.
	#	if param exist - get it (remove "), else create with disabled (like wps2=0)
	#	calculate security
	#	calculate signal in percentage
	#	calculate auth as a number

	#work on mirror (while modify), $0 is for reference only

	{line_mirr=$0};\

	{if (match($0, "^BSSID_"))
	{
		#print "NEW AP" > "/dev/console";\
		loop=loop+1; \
	}}\
	{if (match($0, "^DEBUG") == 0)
		print $0 >> "/tmp/scan_res1";\
	}\

	{for (var=1; var<5; var++)
	{\
		#security:
		if (match($0, search_beacon_t[var]))
		{\
			#delete all, only value left
			getvar[var]=sub(search_beacon_t[var], "", line_mirr); \
			sub(getvar[var], line_mirr, getvar[var]); \
		}\
	}}\
	#Authentication:
	# "Authentication Suites (2) : 802.1X  PSK" is a new value
	# ? see also "Authentication Suites (2) : Proprietary 802.1x"
	# ? "Authentication Suites (2) : Proprietary 802.1x"
	{if (match($0, search_auth))
	{\
		# delete all, only value left
		sub(search_auth, "", line_mirr); \
		#do not change order: if "Authentication Suites (2) : 802.1X PSK" - we select PSK a a workaround
		if (match(line_mirr, "802.1x")) auth_Suites=2;\
		if (match(line_mirr, "PSK")) auth_Suites=3;\
		if ((match(line_mirr, "PSK")) && (match(line_mirr, "802.1x"))) auth_Suites=4;\
		# print "auth_Suites ="auth_Suites > "/dev/console";\
	}}\

	#Encription:
	{if (match($0, search_enc))
	{\
		sub(search_enc, "", line_mirr); \
		#do not change order: need "CCMP TKIP" at the end
		if (match(line_mirr, "TKIP")) enc_Suites=2;\
		if (match(line_mirr, "CCMP")) enc_Suites=3;\
		if (match(line_mirr, "CCMP TKIP")) enc_Suites=4;\
		# print "enc_Suites ="enc_Suites > "/dev/console";\
	}}\

	#WPS:
	{if (match($0, "WPS_.=\"1\"")) wps=1};\

	#RSSI:
	{if (match($0, search_rssi)) 
	{\
		sub(search_rssi, "", line_mirr); \
		rssi_percentage=line_mirr;\
		#print "rssi_percentage="rssi_percentage > "/dev/console";\
	}};\

	#if "DEBUG END Cell_" - we are at the end of AP Cell.
	{if (match($0, "^DEBUG END Cell_"))
	{\
		#Security related table:

		#AP Security | Beacon Type | Auth Type                           | Encr Type
		#============|=============|=====================================|===========================|
		#Open        |BASIC(0)     | OPEN(0)                             | NONE(0)
		#------------|---------------------------------------------------|---------------------------|
		#WEP         | BASIC(0)    | if WPS=1 #WPS -> no SHARED          | WEP(1)
		#            |             |   then OPEN(0)                      |
		#            |             | else # SHARE or OPEN? TODO (*)      |
		#------------|-------------|-------------------------------------|---------
		#WPA         | WPA(1)      | if (suites=PSK)                     |if (pairwise cipher = TKIP
		#            |             |   then PSK(3)                       |then TKIP(2)
		#            |             | elif (suites=802.1x)                |elif (cipher = CCMP) 
		#            |             |   then RADIUS(2)                    |then AES(3)
		#            |             | else                                |
		#            |             |   PSK(3) (**)                       |
		#--------------------------|-------------------------------------|----------------------------|
		#WPA2        | WPA2(2)     | See above                           | See above
		#--------------------------|-------------------------------------|----------------------------|
		#WPA/WAP2    | WPA/WPA2(3) | See above                           | See above
		#
		# *  In MTLK WEB user gets to choose	between shared or open. We need	some special code to indicate this
		# ** If both PSK and RADIUS are	supported by AP, for now choose PSK ???

		# print "in calc_security...." > "/dev/console"
		security_t=0; \
		# print "security array: "getvar[wep]" "getvar[wpa]" "getvar[wpa2]" "getvar[radius] > "/dev/console";\
		if (match(getvar[wep], "on")) security_t=security_t+1; getvar[wep]=""; \
		if (match(getvar[wpa], "WPA")) security_t=security_t+1; getvar[wpa]=""; \
		if (match(getvar[wpa2], "WPA2")) security_t=security_t+2; getvar[wpa2]=""; \
		if (security_t == 0) beaconType="beaconType_"loop"=\"0\""; \
		if (security_t == 1) beaconType="beaconType_"loop"=\"0\""; \
		if (security_t == 2) beaconType="beaconType_"loop"=\"1\""; \
		if (security_t == 3) beaconType="beaconType_"loop"=\"2\""; \
		if (security_t == 4) beaconType="beaconType_"loop"=\"3\""; \
		#if (match(getvar[radius], "802.1X")) {security_t=5; beaconType="beaconType_"loop"=\"5\""}; getvar[radius]=""; \
		if (match(getvar[radius], "802.1X")) {security_t=5;}; getvar[radius]=""; \
		# print "beaconType="beaconType  > "/dev/console";\
		
		#calc auth and enc using security:
		if (security_t == 0)
		{\
			encr="encr_"loop"=\"0\""; \
			# when encr=0 also auth=0:
			auth="auth_"loop"=\"0\""; \
		} else if (security_t == 1)
		{\
			encr="encr_"loop"=\"1\""; \
			if (wps == 1)
			{\
				auth="auth_"loop"=\"0\""; \
			}\
			else
			{\
				auth="auth_"loop"=\"-1\""; \
				#print "TODO: fix the Authentication problem !!!" > "/dev/console";\
			}\
		} else
		{\
			encr="encr_"loop"=""\""enc_Suites"\""
			auth="auth_"loop"=""\""auth_Suites"\""
		}\
		# print "auth="auth > "/dev/console";\
		# print "encr="encr > "/dev/console";\

		#rssi:
		if (rssi_percentage > 83)
			rssi="RSSI_"loop"=\"10\""; \
		else if ((rssi_percentage <= 83) && (rssi_percentage > 77))
			rssi="RSSI_"loop"=\"30\""; \
		else if ((rssi_percentage <= 77) && (rssi_percentage > 71))
			rssi="RSSI_"loop"=\"50\""; \
		else if ((rssi_percentage <= 71) && (rssi_percentage > 65))
			rssi="RSSI_"loop"=\"80\""; \
		else
			rssi="RSSI_"loop"=\"90\""; \
		#print "rssi="rssi > "/dev/console";\

		#This list of params are written at the end of every AP cell (indicated by "DEBUG END Cell_" string in /tmp/scan_res )
		{print beaconType >> "/tmp/scan_res1"}; \
		{print auth >> "/tmp/scan_res1"}; \
		{print encr >> "/tmp/scan_res1"}; \
		{print rssi >> "/tmp/scan_res1"}; \

	}}\

	END {print "" > "/dev/console"; \
	}

	# End AWK2
	######################################################################################
