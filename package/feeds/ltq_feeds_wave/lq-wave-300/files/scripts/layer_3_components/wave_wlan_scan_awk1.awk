	######################################################################################
	# Start AWK1

	# About the arrays:
	# search			- holds the rule searching parameter. Fix value 'Cell no. 2' is overun dynamicaly.
	# caption			- the caption to print in WEB list
	# print_fs			- select the FS to use (awk use FS as a File Seperator)
	# print_rule		- select the field number to print
	# print_remove		- rule to clean the string in the selected field
	# search_in			- search final value in order to change output (for example: "not WPS" to "0")
	# replace_str_in	- substring to change from string found by search_in_string, 
	# replace_str_in_op	- as param can have two values, this holds the other possibility (like "WPS" Vs. "no WPS")
	# replace_str_out	- the replecement, when using search_in_string
	# replace_str_out_op- the replecement, when using search_in_string. The other possibility
	# print_remove		- clean selection by remove substring from the line
	# duplicate_field	- hold string that compared to final value to find duplicated lines in AP cell (first win)
	# duplicate_val		- '=0', no duplicated found yet, '=1' already print to file so ignore match


	BEGIN {j = 0; k = 0; ap_count = 0; loop = 0; cell_index = 1; varin="Cell 01"; new_ap_cell = 2; first_time=1; print "" > "/tmp/scan_res" \
	#when change order in array, change also this list order:
	;				SSID_i=1; BSSID_i=2; std_i=3;  WPS_i=4;   chanWidth_i=5; band_i=6; 	encr_i=7;       auth_i=8;                signal_strength_i=9;  wep_i=10;        WPA_WPA2_i=11;       WPA2_i=12;          Radius_i=13;                   CCMP_TKIP_i=14;          AP_count_i=15\
	;search_string="^ESSID:.*;Cell 01;^Extra:.*HT;^Extra.*WPS;.* (MHz);^Extra.+(band);Encryption key:.*;Authentication Suites.*;Signal level=-.* dBm;Encryption key:.*;^IE: WPA Version 1$;^IE:.*WPA2 Version.*;Authentication Suites.+(802.1X);Pairwise Ciphers (.).*" \
	;split(search_string,search,";") \
	;caption_string="SSID_:BSSID_:std_:WPS_:chanWidth_:band_:encr_:auth_:signal_strength_:wep_:WPA_WPA2_:WPA2_:Radius_:CCMP_TKIP_:AP_count"\
	;split(caption_string,caption,":") \
	#;print_fs_sring=":; ;:;:;:;:;key:; ;=;key:;: ;/; ;:" \
	;print_fs_sring=":; ;:;:;:;:;key:;:;=;key:;: ;/; ;:" \
	;split(print_fs_sring,print_fs,";") \
	#;print_rule_string="2:5:2:2:2:2:2:5:3:2:2:2:5:2" \
	;print_rule_string="2:5:2:2:2:2:2:2:3:2:2:2:5:2" \
	;split(print_rule_string,print_rule,":")\
	;search_in_string=";;;Extra.*not WPS;Extra:20 MHz;;;;;;;;;: CCMP;"\
	;split(search_in_string,search_in,";") \
	;replace_str_in_string=";;;not WPS;20 MHz;;;;;;;;;: CCMP;"\
	;split(replace_str_in_string,replace_str_in,";") \
	;replace_str_in_op_string=";;;WPS;40 MHz;;;;;;;;;: TKIP;"\
	;split(replace_str_in_op_string,replace_str_in_op,";") \
	;replace_str_out_string=";;;0;0;;;;;;;;;:CCMP;"\
	;split(replace_str_out_string,replace_str_out,";") \
	;replace_str_out_op_string=";;;1;2;;;;;;;;;:TKIP;"\
	;split(replace_str_out_op_string,replace_str_out_op,";") \
	;print_remove_string="TODO:TODO:TODO:TODO:TODO: band:TODO:TODO: dBm  Noise level:TODO: Version 1: Version 1:TODO:TODO"\
	;split(print_remove_string,print_remove,":") \
	;duplicate_field_string="0;0;0;0;0;0;0;auth;0;0;0;0;0;CCMP_TKIP"\
	;split(duplicate_field_string,duplicate_field,";") \
	;duplicate_val_string="0;0;0;0;0;0;0;0;0;0;0;0;0;0;0"\
	;split(duplicate_val_string,duplicate_val,";")} \

	#search for new AP in list,
	#write bssid into list and increment 'cell_index' to point to next cell in array:

	#clean spaces ans tabs:
	{sub("^[ \t]*", "", $0);} \
	{if (index ($0, search[new_ap_cell]))
	{ \
		#print "dbg_message: AP="cell_index > "/dev/console";\
		ap_count=ap_count+1; \
		if (first_time == 0)
		{
			loop=loop+1; \
			cell_befor=cell_index-1; \
			print "DEBUG END Cell_"cell_befor >> "/tmp/scan_res"; \
		};\
		first_time=0; \
		FS=print_fs[2]; sub (print_remove[new_ap_cell], "",  $0); print caption[new_ap_cell]""loop"=" "\""$print_rule[new_ap_cell]"\"" >> "/tmp/scan_res"; \
		#set to serach next AP (TODO: add '^Cell..' cause error in param parsing !?)
		if (loop > 7) k="";\
		cell_index=cell_index+1; search[new_ap_cell]="Cell "k""cell_index; \
		#be ready for new AP cell
		for (var=1; var<15; var++) duplicate_val[var]=0; \
		{j=j+1}; \
		{k=int(j/10)}; \
	}} \

	#loop over search array in order to find param in the line (if exist in line)
	#if found - add to list according to:
	#	print_fs, print_rule and print_remove arrays (see above).
	#{for (var in print_fs) - TODO: why it does not working for var>=10???????????

	{for (var=0; var<14; var++)
	{\
		# the split function fill in array with string from cell 1, not zero.
		varplus=var+1; \
		# print "varplus="varplus" ""line="$0" ""searching="search[varplus] > "/dev/console"; \
		if (match($0, search[varplus]))
		{\
			#All the params requires manipulation are cell > 5 in array
			if (var > 5)
				debug="DEBUG "; \
			else
				debug=""; \
			#Request to encapsulate all by quotes
			#if (match($0, "^[ \t\r\n\v\f]*ESSID"))
			if (match($0, "^[ \t]*ESSID"))
				quotes="";\
			else if (match($0, "Signal level=-"))
				quotes="";\
			else
				quotes="\""; \

			# print "varplus="varplus" ""line="$0" ""searching="search[varplus] > "/dev/console"; \
			# print "varplus="varplus" ""replace_str_in="replace_str_in[varplus]" ""replace_str_out="replace_str_out[varplus]" ""line="$0 > "/dev/console"; \
			#Use print_fs[] array Set the FS for this param, use print_remove[] array tp remove noise from line, leave only value requested:
			FS=print_fs[varplus]; sub (print_remove[varplus], "",  $0);\
			
			# print "varplus="varplus" ""search_in="search_in[varplus]" ""replace_str_in="replace_str_in[varplus]" ""replace_str_in_op="replace_str_in_op[varplus]" ""line="$0 > "/dev/console";
			#If change value requested, change it:
			if (match($0, search_in[varplus]))
				sub (replace_str_in[varplus], replace_str_out[varplus], $0);\
			else
				sub (replace_str_in_op[varplus], replace_str_out_op[varplus], $0);\
			
			#avoid duplicated rows in scan results
			if (duplicate_field[varplus] != 0) 
			{\
				if (duplicate_val[varplus] == 0)
				{\
					print debug caption[varplus]""loop"=" quotes $print_rule[varplus] quotes >> "/tmp/scan_res";\
					duplicate_val[varplus]=1;\
				}\
			}
			else
			{\
				print debug caption[varplus]""loop"=" quotes $print_rule[varplus] quotes >> "/tmp/scan_res";\
			};\
		}\
	}\
	}\
	END {
	cell_befor=cell_index-1;\
	print "DEBUG END Cell_"cell_befor >> "/tmp/scan_res";\
	print "AP_count=\""ap_count"\"" >> "/tmp/scan_res"}

	# End AWK1
	######################################################################################
