/*
 * error_defines.h
 *
 *  Created on: 30-Mar-2015
 *      Author: vvdnlt230
 */

#ifndef ERROR_DEFINES_H_
#define ERROR_DEFINES_H_




typedef enum
{
	FLTE_APP_ERROR_FAILED_TO_CREATE_PING_PIPE,
	FLTE_APP_ERROR_FAILED_TO_CREATE_LTE_PIPE,
	FLTE_APP_ERROR_FAILED_TO_CREATE_AUDIO_PIPE,
	FLTE_APP_ERROR_FAILED_TO_CREATE_LTE_PROCESS,
	FLTE_APP_ERROR_FAILED_TO_CREATE_FAX_PROCESS,
	FLTE_APP_ERROR_FAILED_TO_CREATE_LEDU_PROCESS,
	FLTE_APP_ERROR_FAILED_TO_OPEN_PING_PIPE,
	FLTE_APP_ERROR_DATA_CHKSUM_FAILED,
	FLTE_APP_ERROR_MODEM_DEVICE_BUSY_OR_NOT_FOUND,
}FLTE_ENUM_ERROR_type_t;

typedef struct
{
	FLTE_ENUM_ERROR_type_t ErrorType;
	char cErrorDetails;							//Number specific to the Error
}FLTE_STRUCT_ERROR_info_t;



typedef void (*pfFLTE_ERROR_ACTION)(FLTE_STRUCT_ERROR_info_t );
typedef struct
{
	FLTE_ENUM_ERROR_type_t FLTE_APP_ERROR_NO;
	const char *pFLTE_APP_ERROR_NAME;
	pfFLTE_ERROR_ACTION FLTE_APP_ERROR_ACTION;

}FLTE_STRUCT_ERROR_type_t;

void failedpipe(FLTE_STRUCT_ERROR_info_t errorno);
void processforkfailed(FLTE_STRUCT_ERROR_info_t errorno);
void failedopen(FLTE_STRUCT_ERROR_info_t errorno);
void processpingfailed(FLTE_STRUCT_ERROR_info_t errorno);
void modemfailure(FLTE_STRUCT_ERROR_info_t errorinfo);


#endif /* ERROR_DEFINES_H_ */
