/*
 * config_file.h
 *
 *  Created on: 10-Jun-2015
 *      Author: vvdnlt230
 */

#ifndef CONFIG_FILE_H_
#define CONFIG_FILE_H_

#define LTEMANAGER_INITIAL_DTMF_TIMEOUT_S 5	//seconds
#define LTEMANAGER_INITIAL_DTMF_TIMEOUT_MS 0	//seconds

#define LTEMANAGER_CALL_TIMEOUT_S 		3		//seconds
#define LTE_MANAGER_NOTIFICATION_INDEX 	2


#define FLTE_REG_STATUS_OK 1
#define FLTE_EVENT_TIMEOUT 0

#define FLTE_SIGNAL_STRENGTH_TIMEOUT 10
#define FLTE_VOICE_CALL_REJECT_TIMEOUT 60


#define FLTE_SIGNAL_STRENGTH_FAIR 95
#define FLTE_SIGNAL_STRENGTH_MEDIUM 115
#define MAX_FAIL_WAIT_TIME 	20				//Time wen telit not responding

#define ENABLE_AUTOFAX 1					//Telit Will automatically answers the call after 2 rings


#define EN_DEBUG_MSGS 1
#define ENABLE_PING	0
#define LTE_MANAGER_PROCESSID 1

#endif /* CONFIG_FILE_H_ */
