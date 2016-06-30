/**************************************************************************************************
 * Name			:	ATCommands.c
 * Created On	: 	31-Mar-2015
 *      @author	: 	sanju
 *      @desc	:	Complete AT Command Functions. Need to Re-edit/Remove Some functions
 *************************************************************************************************/

/********Standard Includes******/
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
/*****Custom Includes**********/
#include "ATCommands.h"
#include "ioctlcommand_defines.h"
#include "config_file.h"
#include "config_device.h"

//Hello

/*************************************************************************************************/
/*****All AT Command Database. Add your Code her if you are planning to Add a new AT Support******/
/*************************************************************************************************/
const FlteAtCmdDetails_t FLTE_TELIT_ATCommands[]=
{
		//0
		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_TEST_CMD,.iFlteAtCmdTimeout=1,.pFlteAtCmdName="AT\r",
				.pFlteAtCmdGetCurValue=NULL,.pFlteAtCmdGetAllValue=NULL,
				.pFLTE_AT_CMD_DESC="Test Command for Checking the Modem",.iFlteAtNoResponses=1,
				.FlteAtPosResponses={"OK","ERROR"},.pfflte_get_all_pos_value=NULL,
				.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
				.pfflte_at_process_cmd=ProcessTestComand,.pfflte_get_ack=ReadnCheckAck},
		//1
		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_REG_STATUS,.iFlteAtCmdTimeout=1,
				.pFlteAtCmdName="AT+CGREG\r",.pFlteAtCmdGetCurValue="AT+CGREG?\r",
				.pFlteAtCmdGetAllValue="AT+CGREG=?\r",.iFlteAtNoResponses=2,
				.pFLTE_AT_CMD_DESC="Command for checking the registration status\n",
				.FlteAtPosResponses={"OK","ERROR"},.pfflte_get_all_pos_value=NULL,
				.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
				.pfflte_at_process_cmd=ProcessTestComand,.pfflte_get_response=process_cgreg_cmd,
				.pfflte_set_parameter=set_cgreg_parameter},
		//2
		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_DISCONNECT,.iFlteAtCmdTimeout=1,.pFlteAtCmdName="AT+CHUP\r",
				.pFlteAtCmdGetCurValue=NULL,.pFlteAtCmdGetAllValue=NULL,.iFlteAtNoResponses=1,
				.pFLTE_AT_CMD_DESC="Command for Disconnecting the call\n",
				.FlteAtPosResponses={"OK","ERROR"},.pfflte_get_all_pos_value=NULL,
				.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
				.pfflte_at_process_cmd=ProcessCommand,.pfflte_get_ack=ReadnCheckAck},
		//3
		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_OUT_CALL,.iFlteAtCmdTimeout=1,.pFlteAtCmdName="ATD",
				.pFlteAtCmdGetCurValue=NULL,.pFlteAtCmdGetAllValue=NULL,.iFlteAtNoResponses=2,
				.pFLTE_AT_CMD_DESC="Command for Dialling a number\n",
				.FlteAtPosResponses={"OK","NO_CARRIER","ERROR"},.pfflte_get_all_pos_value=NULL,
				.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
				.pfflte_at_process_cmd=ProcessCommand,.pfflte_get_ack=ReadnCheckAck},
		//4
		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_CHECK_SIGNAL_STRENGTH,.iFlteAtCmdTimeout=1,
				.pFlteAtCmdName="AT#RFSTS\r",.pFlteAtCmdGetCurValue=NULL,.pFlteAtCmdGetAllValue=NULL,
				.iFlteAtNoResponses=2,.pFLTE_AT_CMD_DESC="Checking the Signal strength\n",
				.FlteAtPosResponses={"OK","#RFSTS:","ERROR"},.pfflte_get_all_pos_value=NULL,
				.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
				.pfflte_at_process_cmd=GettheSignalstrength,.pfflte_get_ack=CheckSignalStrengthACK},
		//5
		//AT#CNUM
		//+CNUM: "Line 1","+14085686269",145 and OK
		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_CHECK_FAX_NUMBER,.iFlteAtCmdTimeout=1,
				.pFlteAtCmdName="AT+CNUM\r",.pFlteAtCmdGetCurValue=NULL,.pFlteAtCmdGetAllValue=NULL,
				.iFlteAtNoResponses=2,.pFLTE_AT_CMD_DESC="Getting Number FAX Machine\n",
				.FlteAtPosResponses={"OK","+CNUM:","ERROR"},.pfflte_get_all_pos_value=NULL,
				.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
				.pfflte_at_process_cmd=GettheFAXNum,.pfflte_get_ack=NULL},

		//6
		//AT#CGSN=1..
		//#CGSN: 353238060023958 and OK
		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_CHECK_IMEI_NUMBER,.iFlteAtCmdTimeout=1,
				.pFlteAtCmdName="AT#CGSN=1\r",.pFlteAtCmdGetCurValue=NULL,.pFlteAtCmdGetAllValue=NULL,
				.iFlteAtNoResponses=2,.pFLTE_AT_CMD_DESC="Getting the IMEI Number\n",
				.FlteAtPosResponses={"OK","#CGSN:","ERROR"},.pfflte_get_all_pos_value=NULL,
				.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
				.pfflte_at_process_cmd=GettheIMEINum,.pfflte_get_ack=NULL},
		//7
		//AT#CIMI..
		//#CIMI: 311480143684496 and OK
		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_CHECK_IMSI_NUMBER,.iFlteAtCmdTimeout=1,
				.pFlteAtCmdName="AT#CIMI\r",.pFlteAtCmdGetCurValue=NULL,.pFlteAtCmdGetAllValue=NULL,
				.iFlteAtNoResponses=2,.pFLTE_AT_CMD_DESC="Getting the IMEI Number\n",
				.FlteAtPosResponses={"OK","#CIMI:","ERROR"},.pfflte_get_all_pos_value=NULL,
				.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
				.pfflte_at_process_cmd=GettheIMSINum,.pfflte_get_ack=NULL},

		//eFLTE_AT_CMD_TYPE_SET_AUTO_ANS
		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_SET_AUTO_ANS,.iFlteAtCmdTimeout=1,
				.pFlteAtCmdName="ATS0=1\r",.pFlteAtCmdGetCurValue=NULL,.pFlteAtCmdGetAllValue=NULL,
				.iFlteAtNoResponses=2,.pFLTE_AT_CMD_DESC="set auto answer after one ring\n",
				.FlteAtPosResponses={"OK","ERROR"},.pfflte_get_all_pos_value=NULL,
				.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
				.pfflte_at_process_cmd=ProcessCommand,.pfflte_get_ack=NULL},

		{.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_ANS_CALL,.iFlteAtCmdTimeout=1,.pFlteAtCmdName="ATA\r",
						.pFlteAtCmdGetCurValue=NULL,.pFlteAtCmdGetAllValue=NULL,
						.pFLTE_AT_CMD_DESC="Manual Answering Call",.iFlteAtNoResponses=1,
						.FlteAtPosResponses={"OK","ERROR"},.pfflte_get_all_pos_value=NULL,
						.pfflte_get_current_value=NULL,.pfflte_process_update_response=NULL,
						.pfflte_at_process_cmd=ProcessTestComand,.pfflte_get_ack=ReadnCheckAck},

};
/****************************************End AT Command Database**********************************/

/*Global Variable. Only For Accessing the device related info. No Write.
 *Change the Variable prefix to g*/
extern FlteDeviceInfo_t FlteDeviceInfo;
/*Global variable to the file*/
LTEDeviceInfo_t DeviceInfo;

/**************************************************************************************************
 * Function Name		:	Strcpy Function
 * @desc				:	Copy One string to other string
 * @param				:	pointer to source & Destination
 * @note				:	Please Make sure that All pointers having valid string
 *************************************************************************************************/
void strcpy_func(char *dst,char *src)
{
	while (*dst++=*src++);
}


/**************************************************************************************************
 * Function Name		:	ProcessStringData Function
 * @desc				:	Process the different Type of response strings from the Modem
 * @param				:	pointer to string,Response type(or Command Type)
 * @todo				:	Need To optimize the function, You are not allowed to use this much
 * 							memory for a small function. Reduce the memory usage. Workaround did
 * 							for the ciou_flte demo.
 *************************************************************************************************/
int ProcessStringData(unsigned char *str,RespType_t ResponseType)
{
	unsigned char imeistr[MAX_AT_RESP_LENGHTH],imsistr[MAX_AT_RESP_LENGHTH],FaxNum[MAX_AT_RESP_LENGHTH];
	unsigned char imeiindex=RESET,imsiindex;
	int i,commacnt=RESET,FaxNoFlag=RESET;
	int fd;
	switch(ResponseType)
	{
		case RESP_IMEI:
			memset(imeistr,RESET,MAX_AT_RESP_LENGHTH);
			/*@todo: Change it back to standard way with AT cmd Database*/
			if(str[RESP_INDEX_0]==AT_HEADER1&&str[RESP_INDEX_1]==AT_HEADER2&&str[RESP_INDEX_2]=='#'&&str[RESP_INDEX_3]=='C'											\
					&&str[RESP_INDEX_4]=='G'&&str[RESP_INDEX_5]=='S'&&str[RESP_INDEX_6]=='N'&&str[RESP_INDEX_7]==':'
							&&str[RESP_INDEX_8]==' ')
			{
				for(imeiindex=RESET;str[OFFSET_IMEI+imeiindex]!=AT_HEADER1;imeiindex++)
					imeistr[imeiindex]=str[OFFSET_IMEI+imeiindex];
#if EN_DEBUG_MSGS
				printf("FLTE:Your Imei Number is %s\n",imeistr);
#endif
				/*@todo:Need to Add the File writing Functionality the database*************/
				fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
				if(fd<0)
				{
#if EN_DEBUG_MSGS
					printf("No Device Found.\n");
#endif
					return -1;	/*No point in proceeding*/
				}
				strcpy_func(DeviceInfo.FLTEimeiNo,imeistr);
#if EN_DEBUG_MSGS
				printf("FLTE: IMEI Num:- %s\n",DeviceInfo.FLTEimeiNo);
#endif
				ioctl(fd,WRITE_DEVICE_IMEI_NUMBER,&DeviceInfo);
				close(fd);
				return 0;
			}
			else/*If the Resp is not having the above prefix then Something went wrong*/
			{
#if EN_DEBUG_MSGS
				printf("IMEI Number Not Found\n");
#endif
				return -1;
			}
			break;

		case RESP_IMSI:
			memset(imsistr,RESET,MAX_AT_RESP_LENGHTH);
			/*@todo: Need to do with a database function. Options there add the
			 * feature in data base*/
			if(str[RESP_INDEX_0]==AT_HEADER1&&str[RESP_INDEX_1]==AT_HEADER2&&str[RESP_INDEX_2]=='#'&&str[RESP_INDEX_3]=='C'											\
					&&str[RESP_INDEX_4]=='I'&&str[RESP_INDEX_5]=='M'&&str[RESP_INDEX_6]=='I'&&str[RESP_INDEX_7]==':'
							&&str[RESP_INDEX_8]==' ')
			{
				for(imsiindex=RESET;str[OFFSET_IMEI+imsiindex]!=AT_HEADER1;imsiindex++)
					imsistr[imsiindex]=str[OFFSET_IMEI+imsiindex];
#if EN_DEBUG_MSGS
				printf("Your Imsi Number is %s\n",imsistr);
#endif
				fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
				if(fd<0)
				{
#if EN_DEBUG_MSGS
					printf("No Device Found.\n");
#endif
					return -1;
				}
				strcpy_func(DeviceInfo.FLTEimsiNo,imsistr);
#if EN_DEBUG_MSGS
				printf("FLTE:IMSI Num:- %s\n",DeviceInfo.FLTEimeiNo);
#endif
				ioctl(fd,WRITE_DEVICE_IMSI_NUMBER,&DeviceInfo);
				close(fd);
				return 0;
			}
			else
			{
#if EN_DEBUG_MSGS
				printf("FLTE:IMEI Number Not Found\n");
#endif
				return -1;
			}

			break;
		case RESP_FAXNO:
			memset(FaxNum,0,MAX_AT_RESP_LENGHTH);
			/*@todo: Add the database functionality. Code looking bad*/
			if(str[RESP_INDEX_0]==AT_HEADER1&&str[RESP_INDEX_1]==AT_HEADER2&&str[RESP_INDEX_2]=='+'&&str[RESP_INDEX_3]=='C'											\
					&&str[RESP_INDEX_4]=='N'&&str[RESP_INDEX_5]=='U'&&str[RESP_INDEX_6]=='M'&&str[RESP_INDEX_7]==':'
							&&str[RESP_INDEX_8]==' ')
			{
				while(str[i]!=END_OF_STRING)
				{
					if(str[i]==',')
					{
						commacnt++;
						if(commacnt==1)
						{
							FaxNoFlag=SET;
						}
						if(FaxNoFlag==SET)
						{
							FaxNoFlag=RESET;
							for(imsiindex=RESET;str[i+1+imsiindex]!=',';imsiindex++)
								FaxNum[imsiindex]=str[1+i+imsiindex];
#if EN_DEBUG_MSGS
							printf("FLTE:Your FAX Number is %s\n",FaxNum);
#endif
							/*@todo:Need to Add the File writing Functionality the database*************/
							fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
							if(fd<0)
							{
#if EN_DEBUG_MSGS
								printf("FLTE:No Device Found.\n");
#endif
								return -1;
							}
							strcpy_func(DeviceInfo.FLTEDeviceNo,FaxNum);
#if EN_DEBUG_MSGS
							printf("FLTE:FAX Num:- %s\n",DeviceInfo.FLTEDeviceNo);
#endif
							ioctl(fd,WRITE_DEVICE_NUMBER,&DeviceInfo);
							close(fd);
						}
					}
					i++;
				}
				return 0;
			}
			else
			{
#if EN_DEBUG_MSGS
				printf("IMEI Number Not Found\n");
#endif
				return -1;
			}
			break;
	}
}
/**************************************************************************************************
 * Function Name		:	ChecktheIMEIACK Function
 * @desc				:	Function specific to the IMEI ACK process
 * @param				:	pointer to FlteAtCmdArg. Arguments need for the AT commads
 * @todo				:	Need To generalize the function. one function for each AT command is not
 * 							 good.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int ChecktheIMEIACK(FlteAtCmdArg_t *FlteAtCmdArg)
{
	int i=RESET,n,status=RESET;
	char str[50];
	memset(str,RESET,50);							/*Clearing the string for new*/
	do
	{
		/*Read one character at a time. Not Sue what is
		 * the resp length from modem */
		n = read(FlteAtCmdArg->iModemFd, &str[i++], 1);
	} while (n > 0);
	/*Passing it to process function*/
	status=ProcessStringData(str,RESP_IMEI);
	return status;
}

/**************************************************************************************************
 * Function Name		:	ChecktheFAXNumACK Function
 * @desc				:	Function specific to the FAX num ACK process
 * @param				:	pointer to FlteAtCmdArg. Arguments need for the AT commads
 * @todo				:	Need To generalize the function. one function for each AT command is not
 * 							 good.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int ChecktheFAXNumACK(FlteAtCmdArg_t *FlteAtCmdArg)
{
	int i=RESET,n,status=RESET;
	char str[50];
	memset(str,RESET,50);							/*Clearing the string for new*/
	do
	{
		/*Read one character at a time. Not Sue what is
		 * the resp length from modem */
		n = read(FlteAtCmdArg->iModemFd, &str[i++], 1);
	} while (n > 0);
	status=ProcessStringData(str,RESP_FAXNO);
	return status;
}
/**************************************************************************************************
 * Function Name		:	ChecktheIMSIACK Function
 * @desc				:	Function specific to the IMSI ACK process
 * @param				:	pointer to FlteAtCmdArg. Arguments need for the AT commads
 * @todo				:	Need To generalize the function. one function for each AT command is not
 * 							 good.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int ChecktheIMSIACK(FlteAtCmdArg_t *FlteAtCmdArg)
{
	int i=RESET,n,status=RESET;
	char str[50];
	memset(str,RESET,50);							/*Clearing the string for new*/
	do
	{
		/*Read one character at a time. Not Sue what is
		 * the resp length from modem */
		n = read(FlteAtCmdArg->iModemFd, &str[i++], 1);
	} while (n > 0);
	status=ProcessStringData(str,RESP_IMSI);
	return status;
}
/*****************************************IMEI,IMSI,FAX NUM Section*******************************/

/**************************************************************************************************
 * Function Name		:	GettheIMEINum Function
 * @desc				:	Function specific to the IMEI
 * @param				:	pointer to FlteAtCmdArg. Arguments need for the AT commansds
 * @todo				:	Need To generalize the function. one function for each AT command is not
 * 							 good.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int GettheIMEINum(FlteAtCmdArg_t *FlteAtCmdArg)
{
#if EN_DEBUG_MSGS
	//printf("Executing Signal search\n",FLTE_TELIT_ATCommands[FlteAtCmdArg->eFlteAtCmdType].pFlteAtCmdName);
#endif
	write(FlteAtCmdArg->iModemFd,FLTE_TELIT_ATCommands[FlteAtCmdArg->eFlteAtCmdType].pFlteAtCmdName,
			strlen(FLTE_TELIT_ATCommands[FlteAtCmdArg->eFlteAtCmdType].pFlteAtCmdName)+1);
	/****************************Need to process the response string*******************************/
	return ChecktheIMEIACK(FlteAtCmdArg);
}
/**************************************************************************************************
 * Function Name		:	GettheIMSINum Function
 * @desc				:	Function specific to the IMSI process
 * @param				:	pointer to FlteAtCmdArg. Arguments need for the AT commansds
 * @todo				:	Need To generalize the function. one function for each AT command is not
 * 							 good.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int GettheIMSINum(FlteAtCmdArg_t *FlteAtCmdArg)
{
#if EN_DEBUG_MSGS
	//printf("Executing Signal search\n",FLTE_TELIT_ATCommands[FlteAtCmdArg->eFlteAtCmdType].pFlteAtCmdName);
#endif
	write(FlteAtCmdArg->iModemFd,FLTE_TELIT_ATCommands[FlteAtCmdArg->eFlteAtCmdType].pFlteAtCmdName,
			strlen(FLTE_TELIT_ATCommands[FlteAtCmdArg->eFlteAtCmdType].pFlteAtCmdName)+1);
	/****************************Need to process the response string*******************************/
	return ChecktheIMSIACK(FlteAtCmdArg);
}
/**************************************************************************************************
 * Function Name		:	GettheFAXNum Function
 * @desc				:	Function specific to the FAX process
 * @param				:	pointer to FlteAtCmdArg. Arguments need for the AT commands
 * @todo				:	Need To generalize the function. one function for each AT command is not
 * 							 good.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int GettheFAXNum(FlteAtCmdArg_t *FlteAtCmdArg)
{
#if EN_DEBUG_MSGS
	//printf("Executing Signal search\n",FLTE_TELIT_ATCommands[FlteAtCmdArg->eFlteAtCmdType].pFlteAtCmdName);
#endif
	write(FlteAtCmdArg->iModemFd,FLTE_TELIT_ATCommands[FlteAtCmdArg->eFlteAtCmdType].pFlteAtCmdName,
			strlen(FLTE_TELIT_ATCommands[FlteAtCmdArg->eFlteAtCmdType].pFlteAtCmdName)+1);
	/****************************Need to process the response string*******************************/
	return ChecktheFAXNumACK(FlteAtCmdArg);
}

/**************************************************************************************************
 * Function Name		:	getImeiImsiFaxNo Function
 * @desc				:	Function for getting all the details
 * @param				:	get all the imei,imsi,Fax number
 * @todo				:	Need To generalize the function. one function for each AT command is not
 * 							 good.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int getImeiImsiFaxNo ()
{
	int status;
	FlteAtCmdArg_t FLTEArgsInfo;
	/*This structure need to updated before using the processing the data*/
	FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_CHECK_FAX_NUMBER;
	FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
	FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
	/*Mapped to the function responsible for handling it*/
	status=FLTE_TELIT_ATCommands[FLTEArgsInfo.eFlteAtCmdType].pfflte_at_process_cmd(&FLTEArgsInfo);
	if(status!=0)
	{
#if EN_DEBUG_MSGS
		printf("Failed updating the FAX Number\n");
#endif
	}
	/*This structure need to updated before using the processing the data*/
	FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_CHECK_IMEI_NUMBER;
	FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
	FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
	/*Mapped to the function responsible for handling it*/
	status=FLTE_TELIT_ATCommands[FLTEArgsInfo.eFlteAtCmdType].pfflte_at_process_cmd(&FLTEArgsInfo);
	if(status!=0)
	{
#if EN_DEBUG_MSGS
		printf("Failed updating the IMEI Number\n");
#endif
	}
	/*This structure need to updated before using the processing the data*/
	FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_CHECK_IMSI_NUMBER;
	FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
	FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
	/*Mapped to the function responsible for handling it*/
	status=FLTE_TELIT_ATCommands[FLTEArgsInfo.eFlteAtCmdType].pfflte_at_process_cmd(&FLTEArgsInfo);
	if(status!=0)
	{
#if EN_DEBUG_MSGS
		printf("Failed updating the IMEI Number\n");
#endif
	}
	return status;
}





int setAutoAnswer()
{
	int status;
	FlteAtCmdArg_t FLTEArgsInfo;
	/*This structure need to updated before using the processing the data*/
	FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_SET_AUTO_ANS;
	FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
	FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
	FLTEArgsInfo.iArgflag=eARG_FLAG_NO_USER_ARGS;
	FLTEArgsInfo.iPollFlag=eWAIT_FLAG_NO_WAIT_FOR_ACK;
	/*Mapped to the function responsible for handling it*/
	status=FLTE_TELIT_ATCommands[FLTEArgsInfo.eFlteAtCmdType].pfflte_at_process_cmd(&FLTEArgsInfo);
	if(status!=0)
	{
#if EN_DEBUG_MSGS
		printf("Failed updating the FAX Number\n");
#endif
		}

}



/**************************************************************************************************
 * Function Name		:	set_cgreg_parameter Function
 * @desc				:	Function for setting the cgreg details
 * @param				:	set the cgreg param,FlteAtCmdArg_t
 * @todo				:	Need To generalize the function. one function for each AT command is not
 * 							 good.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int set_cgreg_parameter(FlteAtCmdArg_t *FlteAtCmdArg)
{
	//Setting parameters Manually
	write(FlteAtCmdArg->iModemFd,"AT+CGREG=1\r",strlen("AT+CGREG=1\r"));
}

/**************************************************************************************************
 * @name	: 	process_cgreg_cmd
 * @desc	:	This Function handles all the op of the
 * 				AT+CGREG command. If Success then Command
 * 				executed successfully.
 * @arg		:	Argument passed for the cgreg write command
 * @ret		:	Success(0) or Fail(-1)
 * @todo	: 	Function need to be rewritten.
 *************************************************************************************************/
int process_cgreg_cmd(FlteAtCmdArg_t *FlteAtCmdArg)
{
	int i=0,n;
	char str[100];
	//Reading the response
	do	{n = read(FlteAtCmdArg->iModemFd, &str[i++], 1);} while (n > 0);
	/*Check The Command format we tried and switch */
	switch(FlteAtCmdArg->eFlteAtCmdFormat)
	{
		case eAT_CMD_FORMAT_DIRECT_COMMAND:						/*Response of AT+CGREG   docs/ATcommands.txt*/
			break;
		case eAT_CMD_FORMAT_GET_CUR_DATA:						/*Response of AT+CGREG? See the docs/ATcommands.txt*/
			if(str[RESP_INDEX_0]==10&&str[RESP_INDEX_1]==10&&str[RESP_INDEX_2]=='+'&&str[RESP_INDEX_3]=='C'											\
					&&str[RESP_INDEX_4]=='G'&&str[RESP_INDEX_5]=='R'&&str[RESP_INDEX_6]=='E'&&str[RESP_INDEX_7]=='G'
							&&str[RESP_INDEX_8]==':'&&str[RESP_INDEX_9]==' '&&(str[RESP_INDEX_10]=='1'||str[RESP_INDEX_10]=='2'
									||str[RESP_INDEX_10]=='0')&&str[RESP_INDEX_11]==','	&&str[RESP_INDEX_13]==10&&str[RESP_INDEX_14]==10)
			{
				/****Device Registration Status********/
				FlteDeviceInfo.eFlteDeviceRegStatus=str[12]-0x30;	//Get the Value of the Device registrtation Status
#if EN_DEBUG_MSGS
				printf("Device Registration Status is %d\n",FlteDeviceInfo.eFlteDeviceRegStatus);
#endif
				if(GetTheAck(&str[15])==0)							//Offset string holding the data
				{
					return 0;										//success
				}
				else
				{
					return -1;										//error
				}
			}
			break;
		case eAT_CMD_FORMAT_GET_ALL_POS_DATA:					//Response of AT_CGREG=?
			break;
	}

}

/**************************************************************************************************
 * Function Name		:	GetTheAck Function
 * @desc				:	Function for checking the ACk of certain commands
 * @param				:	response string received
 * @todo				:	Need To generalize the function. one function for each AT command is not
 * 							 good.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int GetTheAck(char *message)
{
	static int fail_wait_count=0;
	/*@todo:Rewrite as processing directily from the database function*/
	//printf("%x,%x,%x,%x,%x,%x\n",message[0],message[1],message[2],message[3],message[4],message[5]);
	if(message[RESP_INDEX_0]==AT_HEADER1&&message[RESP_INDEX_1]==AT_HEADER2&&message[RESP_INDEX_2]=='O'
			&&message[RESP_INDEX_3]=='K'&& message[RESP_INDEX_4]==AT_HEADER1&&message[RESP_INDEX_5]==AT_HEADER2)
	{
		fail_wait_count=0;
#if EN_DEBUG_MSGS
		printf("FLTE:Acknowledgement OK Received from Telit\n");
#endif
		return 0;
	}
	/*@todo:Rewrite as processing directily from the database function*/
	else if(message[RESP_INDEX_0]==AT_HEADER1&&message[RESP_INDEX_1]==AT_HEADER2&&
			message[RESP_INDEX_2]=='N'&&message[RESP_INDEX_3]=='O'&&message[RESP_INDEX_4]==' '
					&&message[RESP_INDEX_5]=='C'&&message[RESP_INDEX_6]=='A'&& message[RESP_INDEX_7]=='R'&&
					message[RESP_INDEX_8]=='R'&&message[RESP_INDEX_9]=='I'&&message[RESP_INDEX_10]=='E'&&
					message[RESP_INDEX_11]=='R' &&message[RESP_INDEX_12]==AT_HEADER1&&message[RESP_INDEX_13]==AT_HEADER2)
	{
#if EN_DEBUG_MSGS
		printf("FLTE:%s\n",message);
		printf("FLTE:No Carrier Information Received From Telit\n");
#endif
		fail_wait_count=0;
		return 1;
	}
	else if(message[RESP_INDEX_0]==AT_HEADER1&&message[RESP_INDEX_1]==AT_HEADER2)
	{
#if EN_DEBUG_MSGS
		//Some un expected result came. Send back as No_Carier. You may not get your esired result now
                printf("FLTE:%s\n",message);
                printf("FLTE:Unexpected Response Received From Telit\n");
		return 1;							//No carier as the call is not going to get connected now
#endif
		fail_wait_count=0;
	}
	else
	{
#if EN_DEBUG_MSGS
		printf("FLTE:Waiting for Telit Modem Response\n");
#endif
		fail_wait_count++;
		if(fail_wait_count>=MAX_FAIL_WAIT_TIME)
		{
			fail_wait_count=0;
#if EN_DEBUG_MSGS
		printf("FLTE:No Response  from Telit. \n");
#endif
		return 1;						//Send as a NO_CARRIER result
		}
		return -1;
	}
}



/**************************************************************************************************
 * Function Name		:	ReadnCheckAck Function
 * @desc				:	Function for reading & checking the ACK of certain commands
 * @param				:	Arguments of the command sent
 * @todo				:	Need To generalize the function. one ACK function should do
 * 							.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int ReadnCheckAck(FlteAtCmdArg_t *FLTE_AT_CMD_Arg)
{
	int i=RESET,n,status=RESET;
	char str[20];
	while(1)
	{
		/*Clearing for getting new data*/
		memset(str,0,20);
		i=0;
		do {
			n = read(FLTE_AT_CMD_Arg->iModemFd, &str[i++], 1);
		} while (n > 0);
		status=GetTheAck(str);
		/*@todo:We need to add a timeout also here. Infinite waiting is not recommended*/
		if(FLTE_AT_CMD_Arg->iPollFlag==eWAIT_FLAG_WAIT_FOR_ACK)
		{
			if(status!=-1)
			{
				//This means we got some response to our Command
				return status;
			}
			else
			{
				//Wait Here Till you are getting an ACK
				//return status;
				//normally out going call status wont return immediately
			}
		}
		else
		{
			//We can immediately return if command is not telling
			return status;
		}
	}
}

/**************************************************************************************************
 * Function Name		:	processprefix Function
 * @desc				:	Function for checking the prefix part of the response string
 * @param				:	response string received from the modem.
 * @todo				:	Need To generalize the function. one ACK function should do
 * 							.Workaround did for the ciou_flte demo.
 *************************************************************************************************/
int processprefix(unsigned char *message)
{
	int fd;
	LTEDevicesignalstrength_t LTEDeviceSignalStrength;
	if(message[RESP_INDEX_0]==AT_HEADER1&&message[RESP_INDEX_1]==AT_HEADER2&&message[RESP_INDEX_2]=='O'&&
			message[RESP_INDEX_3]=='K'&&message[RESP_INDEX_4]==AT_HEADER1&&message[RESP_INDEX_5]==AT_HEADER2)
	{
		/*This means We are OK only not complete string*/
		fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
		if(fd<0)
		{
#if EN_DEBUG_MSGS
			printf("No Device Found.\n");
#endif
		}
		/*@todo: Add the check functionality */
		LTEDeviceSignalStrength.SignalStrength=RESET;
		ioctl(fd,WRITE_DEVICE_SIGNAL_STRENGTH,&LTEDeviceSignalStrength);
		close(fd);
		return 1;
	}
	else if(message[RESP_INDEX_0]==AT_HEADER1&&message[RESP_INDEX_1]==AT_HEADER2&&message[RESP_INDEX_2]=='#'&&
			message[RESP_INDEX_3]=='R'&& message[RESP_INDEX_4]=='F'&&message[RESP_INDEX_5]=='S')
	{
		/*This means We are getting the complete string*/
		return 0;
	}
	else
	{
		return -1;
	}
}

/**************************************************************************************************
 * Function Name		:	stringtoint Function
 * @desc				:	Function for converting the string to integer
 * @param				:	string to be converted to integer
 * @todo				:	Function is OK. But moved to utils.c (It is not app related func)
 *************************************************************************************************/
int stringtoint(char *str)
{
	int i,len,num=0,multiplier=1;
	len=strlen(str);
	for(i=RESET;i<len;i++)
	{
		num+=(str[(len-1)-i]-0x30)*multiplier;
		multiplier*=10;
	}
	return num;
}





/**************************************************************************************************
 * Function Name		:	processSignalStrengthData Function
 * @desc				:	Function for getting the signal strength info from the resp string
 * @param				:	string to be processed for the signal strength info
 * @todo				:	Need to optimize this function as this is always getting executed
 *************************************************************************************************/
int processSignalStrengthData(unsigned char *str)
{
	int i=RESET,comma_cnt=RESET,rsrp_index=RESET,signal_strength=RESET,imsiindex;
	char signalstrength[6],imsinumber[20];
	int fd;
	LTEDevicesignalstrength_t LTEDeviceSignalStrength;
	eSignalStrengthState_t eSignalStrengthState=RESET;
	while(str[i]!=END_OF_STRING)
	{
		if(str[i]==',')
		{
			comma_cnt++;
			if (comma_cnt==COMMA_OFFSET_FOR_RSRP)
			{
				eSignalStrengthState=eRSRP;
			}
#if 0			/*Commented IT is not recommented that read the same IMSI every
				time when you are reading the Signal strength. One time only needed
				So IMSI should be in seperate function */
			if (comma_cnt==12)
			{
				eSignalStrengthState=eIMSI;
			}
#endif

		}
		switch(eSignalStrengthState)
		{
		case eRSRP:						//Get the signal strength and change the state
			eSignalStrengthState=eRSSI;
			memset(signalstrength,0,6);
			for(rsrp_index=1;str[i+rsrp_index]!=',';rsrp_index++)
			{
				signalstrength[rsrp_index-1]=str[i+rsrp_index];
			}
			signalstrength[rsrp_index]=str[i+rsrp_index];
			//printf("Signal strength in string %s\n",signalstrength);
			signal_strength=stringtoint(&signalstrength[1]);
			//printf("Signal strength converted %d\n",signal_strength);
			/*Opening File descriptor*/
			/*@todo: To be done using the database functions*/
			fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
			if(fd<0)
			{
#if EN_DEBUG_MSGS
				printf("No Device Found.\n");
#endif
			}
			LTEDeviceSignalStrength.SignalStrength=signal_strength;
			ioctl(fd,WRITE_DEVICE_SIGNAL_STRENGTH,&LTEDeviceSignalStrength);
			close(fd);
			if(signal_strength<=FLTE_SIGNAL_STRENGTH_FAIR)
			{
				system("echo 3 > /dev/fifo_led");
			}
			else if (signal_strength>FLTE_SIGNAL_STRENGTH_FAIR && signal_strength<=FLTE_SIGNAL_STRENGTH_MEDIUM)
			{
				system("echo 2 > /dev/fifo_led");
			}
			else if(signal_strength> FLTE_SIGNAL_STRENGTH_MEDIUM)
			{
				system ("echo 1 > /dev/fifo_led");
			}
			//printf("Signal strength - %d\n",signal_strength);
			break;
		case eIMSI:/*This should not get called in this function. keeping as a back up*/
			eSignalStrengthState=eNetNameAsc;
			memset(imsinumber,RESET,20);
			for(imsiindex=1;str[i+imsiindex]!=',';imsiindex++)
			{
				imsinumber[imsiindex-1]=str[i+imsiindex];
			}
			imsinumber[imsiindex]=str[i+imsiindex];
			/*@todo: To be done using the database functions*/
			fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
			if(fd<0)
			{
#if EN_DEBUG_MSGS
				printf("No Device Found.\n");
#endif
			}

			strcpy_func(DeviceInfo.FLTEimsiNo,imsinumber);
#if EN_DEBUG_MSGS
			printf("IMSI:- %s\n",DeviceInfo.FLTEimsiNo);
#endif
			//LTEDeviceSignalStrength.SignalStrength=signal_strength;
			ioctl(fd,WRITE_DEVICE_IMSI_NUMBER,&DeviceInfo);
			close(fd);
			break;
		default:
			break;
		}
		i++;
	}
}


/**************************************************************************************************
 * Function Name		:	ProcessString Function
 * @desc				:	Function for processing the string : especially for rfsts command
 * @param				:	string to be processed for the rfsts command
 * @todo				:	Need to optimize this function as this is always getting executed
 *************************************************************************************************/
int ProcessString(unsigned char *str)
{
	int status;
	status=processprefix(str);
	if(status==RESET)/*RFSTS Resp string received*/
	{
		//printf("Complete String received \n");
		processSignalStrengthData(str);
	}
	else if (status==SET)/*Only OK received, Antenna May be not connected*/
	{
#if EN_DEBUG_MSGS
		printf("Received OK only\n");
#endif
		system ("echo 1 > /dev/fifo_led");
	}
	else
	{
#if EN_DEBUG_MSGS
		printf("Error in searching\n");
#endif
	}
}



/**************************************************************************************************
 * Function Name		:	CheckSignalStrengthACK Function
 * @desc				:	Function for responsible for reading the response data
 * @param				:	string to be processed for the rfsts command
 * @todo				:	Need to optimize this function as this is always getting executed
 *************************************************************************************************/
int CheckSignalStrengthACK(FlteAtCmdArg_t *FLTE_AT_CMD_Arg)
{
	int i=RESET,n,status=RESET;
	char str[150];								/*@todo: Dummy added for demo*/
	memset(str,RESET,150);							//On Every try this should be cleared
	i=0;
	do {
		n = read(FLTE_AT_CMD_Arg->iModemFd, &str[i++], 1);
	} while (n > 0);
	status=ProcessString(str);
	//printf("Signal Strength Query Response is \n%s\n",str);
}



/**************************************************************************************************
 * Function Name		:	GettheSignalstrength Function
 * @desc				:	Function for responsible for executing the commands
 * @param				:	FLTE_AT_CMD_Arg
 * @todo				:	mapped already to the database function
 *************************************************************************************************/
int GettheSignalstrength(FlteAtCmdArg_t *FLTE_AT_CMD_Arg)
{
	//printf("Executing Signal search\n",FLTE_TELIT_ATCommands[FLTE_AT_CMD_Arg->eFlteAtCmdType].pFlteAtCmdName);
	write(FLTE_AT_CMD_Arg->iModemFd,FLTE_TELIT_ATCommands[FLTE_AT_CMD_Arg->eFlteAtCmdType].pFlteAtCmdName,
			strlen(FLTE_TELIT_ATCommands[FLTE_AT_CMD_Arg->eFlteAtCmdType].pFlteAtCmdName)+1);
	return CheckSignalStrengthACK(FLTE_AT_CMD_Arg);
}


/**************************************************************************************************
 * Function Name		:	ProcessCommand Function
 * @desc				:	Function for responsible for executing the commands
 * @param				:	FLTE_AT_CMD_Arg
 * @todo				:	mapped already to the database function
 *************************************************************************************************/
int ProcessCommand(FlteAtCmdArg_t *FLTE_AT_CMD_Arg)
{
	if(FLTE_AT_CMD_Arg->iArgflag==eARG_FLAG_NO_USER_ARGS)
	{
		//printf("Command Name printing-- %s \n",FLTE_TELIT_ATCommands[FLTE_AT_CMD_Arg->eFlteAtCmdType].pFlteAtCmdName);
		write(FLTE_AT_CMD_Arg->iModemFd,FLTE_TELIT_ATCommands[FLTE_AT_CMD_Arg->eFlteAtCmdType].pFlteAtCmdName,
				strlen(FLTE_TELIT_ATCommands[FLTE_AT_CMD_Arg->eFlteAtCmdType].pFlteAtCmdName)+1);
		return ReadnCheckAck(FLTE_AT_CMD_Arg);
	}
	else if(FLTE_AT_CMD_Arg->iArgflag==eARG_FLAG_ATTACH_USER_ARGS)
	{
		write(FLTE_AT_CMD_Arg->iModemFd,FLTE_AT_CMD_Arg->arg,strlen(FLTE_AT_CMD_Arg->arg)+1);
		return ReadnCheckAck(FLTE_AT_CMD_Arg);
	}
}


/**************************************************************************************************
 * Function Name		:	printATCommands Function
 * @desc				:	Function for responsible for executing the commands
 * @param				:
 * @todo				:	Dummy function added for checking the database,need to be removed
 *************************************************************************************************/
void printATCommands()
{
	int i;
#if EN_DEBUG_MSGS
	printf("Printing AT Commandsn");
#endif
	for (i=0;i<2;i++)
	{
		if(FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_TEST_CMD].FlteAtPosResponses.str[i]!=NULL)
		{
#if EN_DEBUG_MSGS
			printf("First Possible value is %s\n",
					FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_TEST_CMD].FlteAtPosResponses.str[i]);
#endif
		}
		else
		{
#if EN_DEBUG_MSGS
			printf("Breaking in loop\n");
#endif
			break;
		}
	}
}


/**************************************************************************************************
 * Function Name		:	ProcessTestComand Function
 * @desc				:	Function for responsible for executing the commands
 * @param				:	FlteAtCmdArg_t
 * @todo				:	Dummy function added for checking the database,need to be removed
 * 							after moving the cgreg functionality
 *************************************************************************************************/
int ProcessTestComand(FlteAtCmdArg_t *FLTE_AT_CMD_Arg)
{
	int n,i;
	char d;
	char str[100],cmd[3];
	 int index=RESET;
	 fd_set rdset;
	 int status;

	switch(FLTE_AT_CMD_Arg->eFlteAtCmdType)
	{
		case eFLTE_AT_CMD_TYPE_TEST_CMD:
#if EN_DEBUG_MSGS
			//printf("FLTE:Test command\n");
#endif
			write(FLTE_AT_CMD_Arg->iModemFd,FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_TEST_CMD].pFlteAtCmdName,
					strlen(FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_TEST_CMD].pFlteAtCmdName));
			status=FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_TEST_CMD].pfflte_get_ack(FLTE_AT_CMD_Arg);
#if EN_DEBUG_MSGS
			if(status==0)
				printf("FLTE: Test Command ACK Success\n");
			else
				printf("FLTE: Test Command ACK Failed\n");
#endif
			break;
#if 0
			for(i=0;FLTE_TELIT_ATCommands[FLTE_AT_CMD_TEST_CMD].pFLTE_AT_POS_RESPONSES.str[i]!=NULL;i++)
			{
				if(strncmp(str,FLTE_TELIT_ATCommands[FLTE_AT_CMD_TEST_CMD].pFLTE_AT_POS_RESPONSES.str[i],strlen(FLTE_TELIT_ATCommands[FLTE_AT_CMD_TEST_CMD].pFLTE_AT_POS_RESPONSES.str[i])))
				{
					printf("Matched %s\n",FLTE_TELIT_ATCommands[FLTE_AT_CMD_TEST_CMD].pFLTE_AT_POS_RESPONSES.str[i]);
					printf("data came is %s\n",str);
					break;
				}
			}
#endif
			break;
		case eFLTE_AT_CMD_TYPE_REG_STATUS:
			set_cgreg_parameter(FLTE_AT_CMD_Arg);
			status=FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_REG_STATUS].pfflte_get_response(FLTE_AT_CMD_Arg);
#if EN_DEBUG_MSGS
			printf("FLTE: Set CGREG notification\n");
#endif
			write(FLTE_AT_CMD_Arg->iModemFd,FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_REG_STATUS].pFlteAtCmdGetCurValue,
					strlen(FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_REG_STATUS].pFlteAtCmdGetCurValue)+1);
			status=FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_REG_STATUS].pfflte_get_response(FLTE_AT_CMD_Arg);
#if EN_DEBUG_MSGS
			printf("FLTE: Get Registration Value\n");
#endif
			break;

		case eFLTE_AT_CMD_TYPE_ANS_CALL:
#if EN_DEBUG_MSGS
			printf("FLTE:Answering Call\n");
#endif
			write(FLTE_AT_CMD_Arg->iModemFd,FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_ANS_CALL].pFlteAtCmdName,
					strlen(FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_ANS_CALL].pFlteAtCmdName));
			status=FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_ANS_CALL].pfflte_get_ack(FLTE_AT_CMD_Arg);
#if EN_DEBUG_MSGS
			if(status==0)
				printf("FLTE: Ans Call ACK Success\n");
			else
				printf("FLTE: Ans Call ACK Failed\n");
#endif
			break;

	}
}
/**************************************************************************************************
 * Function Name		:	CallSignalStrengthHandler Function
 * @desc				:	Function for responsible for calling the database func
 * @param				:	FlteAtCmdArg_t
 * @todo				:	Function seems OK. You can remove the todo
 *************************************************************************************************/
int CallSignalStrengthHandler()
{
	int status;
	FlteAtCmdArg_t FLTEArgsInfo;
	FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_CHECK_SIGNAL_STRENGTH;
	FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
	FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
	status=FLTE_TELIT_ATCommands[FLTEArgsInfo.eFlteAtCmdType].pfflte_at_process_cmd(&FLTEArgsInfo);
}




/**************************************************************************************************
 * Function Name		:	SendEventToModem Function
 * @desc				:	Function for responsible for calling the database func for certain cmds
 * @param				:	FAXStruct_t
 * @todo				:	Function seems OK. You can remove the todo
 *************************************************************************************************/
int SendEventToModem(FAXStruct_t *FAXEventInfo )
{
	FlteAtCmdArg_t FLTEArgsInfo;
	int status;
	switch(FAXEventInfo->FAXEventName)
	{
		case eEVENT_FAX_DISCONNECTED:
			FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_DISCONNECT;
			FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
			FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
			FLTEArgsInfo.iArgflag=eARG_FLAG_NO_USER_ARGS;
			FLTEArgsInfo.iPollFlag=eWAIT_FLAG_NO_WAIT_FOR_ACK;
			status=FLTE_TELIT_ATCommands[FLTEArgsInfo.eFlteAtCmdType].pfflte_at_process_cmd(&FLTEArgsInfo);
			if (status==RESET)
				return 0;
			else
				return -1;
			break;
		case eEVENT_FAX_OUT_CALL:
			FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_OUT_CALL;
			FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
			FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
			FLTEArgsInfo.iArgflag=eARG_FLAG_ATTACH_USER_ARGS;
			FLTEArgsInfo.iPollFlag=eWAIT_FLAG_WAIT_FOR_ACK;
			memset(FLTEArgsInfo.arg,RESET,20);
			strncat(FLTEArgsInfo.arg,FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_OUT_CALL].pFlteAtCmdName,
					strlen(FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_OUT_CALL].pFlteAtCmdName)+1);
			strncat(FLTEArgsInfo.arg,FAXEventInfo->FAXOutGoingEvent.onumber,
					strlen(FAXEventInfo->FAXOutGoingEvent.onumber)+1);
			strncat(FLTEArgsInfo.arg,";\r",2);
			//printf("printing Command - %s\n",FLTEArgsInfo.arg);
			status=FLTE_TELIT_ATCommands[eFLTE_AT_CMD_TYPE_OUT_CALL].pfflte_at_process_cmd(&FLTEArgsInfo);
			if (status==0)
				return 0;
			else
				return -1;
			break;
	}
}

/**************************************End OF FILE************************************************/

