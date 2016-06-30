#!/bin/sh


#if [ "$CONFIG_IFX_CONFIG_CPU" = "XRX3XX" ]; then
	
	if [ "$CONFIG_PACKAGE_KMOD_LTQCPE_AR10_F2_SUPPORT" = "1" ]; then
		WAN_PORT="1"
		LAN_PORT_1="2"	
		LAN_PORT_2="3"	
		LAN_PORT_3="4"	
		LAN_PORT_4="5"
	
#		LAN_PORTs="2 3 4 5"
#		LAN_PORT_Qs="8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23"
#		WAN_PORT_Qs="31 30 29 28 7 6 5 4"
		
#		LP1_Qs="8 9 10 11"
#		LP2_Qs="12 13 14 15"
#		LP3_Qs="16 17 18 19"
#		LP4_Qs="20 21 22 23"
	
		LP1_Q1="8"	
		LP1_Q2="9"	
		LP1_Q3="10"	
		LP1_Q4="11"	
		
		LP2_Q1="12"	
		LP2_Q2="13"	
		LP2_Q3="14"	
		LP2_Q4="15"
			
		LP3_Q1="16"	
		LP3_Q2="17"	
		LP3_Q3="18"	
		LP3_Q4="19"

		LP4_Q1="20"	
		LP4_Q2="21"	
		LP4_Q3="22"	
		LP4_Q4="23"
	
		WP1_Q1="31"	
		WP1_Q2="30"	
		WP1_Q3="29"	
		WP1_Q4="28"
		WP1_Q5="7"
		WP1_Q6="6"
		WP1_Q7="5"
		WP1_Q8="4"
	else
		WAN_PORT="5"	
		LAN_PORT_1="0"	
		LAN_PORT_2="1"	
		LAN_PORT_3="2"	
		LAN_PORT_4="4"	
		
#		LAN_PORTs="2 3 4 5"
#		LAN_PORT_Qs="0 1 2 3 4 5 6 7 8 9 10 11 16 17 18 19"
#		WAN_PORT_Qs="31 30 29 28 23 22 21 20"


#		LP1_Qs="0 1 2 3"
#		LP2_Qs="4 5 6 7"
#		LP3_Qs="8 9 10 11"
#		LP4_Qs="16 17 18 19"

		LP1_Q1="0"	
		LP1_Q2="1"	
		LP1_Q3="2"	
		LP1_Q4="3"
	
		LP2_Q1="4"	
		LP2_Q2="5"	
		LP2_Q3="6"	
		LP2_Q4="7"

		LP3_Q1="8"	
		LP3_Q2="9"	
		LP3_Q3="10"	
		LP3_Q4="11"
	
		LP4_Q1="16"	
		LP4_Q2="17"	
		LP4_Q3="18"	
		LP4_Q4="19"

		WP1_Q1="31"	
		WP1_Q2="30"	
		WP1_Q3="29"	
		WP1_Q4="28"
		WP1_Q5="23"
		WP1_Q6="22"
		WP1_Q7="21"
		WP1_Q8="20"
	fi

#fi
