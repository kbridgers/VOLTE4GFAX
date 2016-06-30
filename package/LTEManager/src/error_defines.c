/*
 * error_defines.c
 *
 *  Created on: 30-Mar-2015
 *      Author: vvdnlt230
 */


#include "error_defines.h"

#if 1
#include <stdio.h>
#include "config_file.h"
#define DBG printf
#else
		DBG
#endif

#define ERROR_NO_FORMAT 	"FLTE_ERROR\nNo-%d\t,Name-%s\n"




/*@todo: Complete Error Database Need to updated before the release*/
const FLTE_STRUCT_ERROR_type_t error_defines[]=
{
		//Pipe Creation Errors
		{.FLTE_APP_ERROR_NO=FLTE_APP_ERROR_FAILED_TO_CREATE_PING_PIPE,.pFLTE_APP_ERROR_NAME="PING PIPE Creation Failed",.FLTE_APP_ERROR_ACTION=failedpipe},
		{.FLTE_APP_ERROR_NO=FLTE_APP_ERROR_FAILED_TO_CREATE_LTE_PIPE,.pFLTE_APP_ERROR_NAME="LTE PIPE Creation Failed",.FLTE_APP_ERROR_ACTION=failedpipe},
		{.FLTE_APP_ERROR_NO=FLTE_APP_ERROR_FAILED_TO_CREATE_AUDIO_PIPE,.pFLTE_APP_ERROR_NAME="Audio PIPE Creation Failed",.FLTE_APP_ERROR_ACTION=failedpipe},
		{.FLTE_APP_ERROR_NO=FLTE_APP_ERROR_FAILED_TO_CREATE_LTE_PROCESS,.pFLTE_APP_ERROR_NAME="LTE Process Spawn Failed",.FLTE_APP_ERROR_ACTION=processforkfailed},
		{.FLTE_APP_ERROR_NO=FLTE_APP_ERROR_FAILED_TO_CREATE_FAX_PROCESS,.pFLTE_APP_ERROR_NAME="FAX Process Spawn Failed",.FLTE_APP_ERROR_ACTION=processforkfailed},
		{.FLTE_APP_ERROR_NO=FLTE_APP_ERROR_FAILED_TO_CREATE_LEDU_PROCESS,.pFLTE_APP_ERROR_NAME="LED updater Process Spawn Failed",.FLTE_APP_ERROR_ACTION=processforkfailed},
		{.FLTE_APP_ERROR_NO=FLTE_APP_ERROR_FAILED_TO_OPEN_PING_PIPE,.pFLTE_APP_ERROR_NAME="Cannot able to open the PING pipe",.FLTE_APP_ERROR_ACTION=failedopen},
		{.FLTE_APP_ERROR_NO=FLTE_APP_ERROR_DATA_CHKSUM_FAILED,.pFLTE_APP_ERROR_NAME="PING communication Checksum error from the process\n",.FLTE_APP_ERROR_ACTION=processpingfailed},
		{.FLTE_APP_ERROR_NO=FLTE_APP_ERROR_MODEM_DEVICE_BUSY_OR_NOT_FOUND,.pFLTE_APP_ERROR_NAME="Cannot open Modem Device. Resource may be busy or not found\n",.FLTE_APP_ERROR_ACTION=modemfailure},





};

/*
 * Error Action For the Failed PIPE error
 * We can add related actions here.
 * Added Only logs Now
 * */
void failedpipe(FLTE_STRUCT_ERROR_info_t errorinfo)
{
#if EN_DEBUG_MSGS
	DBG(ERROR_NO_FORMAT,errorinfo.ErrorType,error_defines[errorinfo.ErrorType].pFLTE_APP_ERROR_NAME);
	//DBG("FLTE- ERROR Name - %s\n",error_defines[errorno].pFLTE_SYSTEM_ERROR_NAME);
#endif
}

/*Seperating the error functions only for adding error related error actions Later like blinking LED like*/
void processforkfailed(FLTE_STRUCT_ERROR_info_t errorinfo)
{
#if EN_DEBUG_MSGS
	DBG(ERROR_NO_FORMAT,errorinfo.ErrorType,error_defines[errorinfo.ErrorType].pFLTE_APP_ERROR_NAME);
#endif
}
/*Seperating the error functions only for adding error related error actions Later like blinking LED like*/
void failedopen(FLTE_STRUCT_ERROR_info_t errorinfo)
{
#if EN_DEBUG_MSGS
	//DBG("FLTE_ERROR - %d\n",erro);
	DBG(ERROR_NO_FORMAT,errorinfo.ErrorType,error_defines[errorinfo.ErrorType].pFLTE_APP_ERROR_NAME);
#endif
}


void processpingfailed(FLTE_STRUCT_ERROR_info_t errorinfo)
{
#if EN_DEBUG_MSGS
	DBG(ERROR_NO_FORMAT,errorinfo.ErrorType,error_defines[errorinfo.ErrorType].pFLTE_APP_ERROR_NAME);
	DBG("Failiure in the PING packet from the %d process\n",errorinfo.cErrorDetails);			//Using the additional information
#endif
}


/*Function for handling the modem failure*/
void modemfailure(FLTE_STRUCT_ERROR_info_t errorinfo)
{
#if EN_DEBUG_MSGS
	DBG(ERROR_NO_FORMAT,errorinfo.ErrorType,error_defines[errorinfo.ErrorType].pFLTE_APP_ERROR_NAME);
#endif
}
