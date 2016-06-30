/*
 * ATCommands.h
 *
 *  Created on: 31-Mar-2015
 *      Author: vvdnlt230
 */

#ifndef ATCOMMANDS_H_
#define ATCOMMANDS_H_

#include <stdint.h>
//#include <sys/stat.h>
//#include <signal.h>
#include <sys/time.h>

/*************************************************
 * @enum ATCommand Types
 * @desc Different Types of AT Commands
 ************************************************/
typedef enum
{
	eFLTE_AT_CMD_TYPE_TEST_CMD,							//Default Test Command AT
	eFLTE_AT_CMD_TYPE_REG_STATUS,
	eFLTE_AT_CMD_TYPE_DISCONNECT,
	eFLTE_AT_CMD_TYPE_OUT_CALL,
	eFLTE_AT_CMD_TYPE_CHECK_SIGNAL_STRENGTH,				/*IMSI,RSRP number here*/
	eFLTE_AT_CMD_TYPE_CHECK_FAX_NUMBER,						/*FAX number*/
	eFLTE_AT_CMD_TYPE_CHECK_IMEI_NUMBER,					/*Check The IMEI Number of the Device*/
	eFLTE_AT_CMD_TYPE_CHECK_IMSI_NUMBER,					/*Check The IMEI Number of the Device*/
	eFLTE_AT_CMD_TYPE_SET_AUTO_ANS,
	eFLTE_AT_CMD_TYPE_ANS_CALL,

}eFlteAtCmdType_t;
/************eFLTEATCmdType_t*******************/

/*************************************************
 * @enum ATCommand Format
 * @desc Different Formats of one AT Command
 ************************************************/
typedef enum
{
	eAT_CMD_FORMAT_DIRECT_COMMAND,					//Ex. AT
	eAT_CMD_FORMAT_GET_CUR_DATA,					//AT+CGREG?
	eAT_CMD_FORMAT_GET_ALL_POS_DATA,				//AT+CGREG=?
}eFlteAtCmdFormat_t;
/*********eFLTEATCmdFormat_t********************/
typedef enum
{
	eARG_FLAG_NO_USER_ARGS,
	eARG_FLAG_ATTACH_USER_ARGS,
}eArgFlag_t;

typedef enum
{
	eWAIT_FLAG_WAIT_FOR_ACK,
	eWAIT_FLAG_NO_WAIT_FOR_ACK,
}eWaitFlag_t;


/*Basic only*/
typedef struct
{
	eFlteAtCmdType_t eFlteAtCmdType;
	eFlteAtCmdFormat_t eFlteAtCmdFormat;				//Direct Command,Getting Curr value,Getting All values
	int32_t iModemFd;									//updating the structure so that the functions pointers can directly use it
	uint8_t arg[20];										//Number of arguments
	eArgFlag_t iArgflag;
	eWaitFlag_t iPollFlag;								//It will Wait there till we get any response from the device
//	uint8_t *Arg;										//String Array to the AT commands Arguments
//	uint8_t *ArgResponse;
}FlteAtCmdArg_t;


typedef enum
{
	eDEVICE_NOT_REGISTERD_NOT_TRYING,
	eDEVICE_REGISTERED_HOME,
	eDEVICE_NOT_REGISTERED_TRYING,
	eDEVICE_REGISTRATION_DENIED,
	eDEVICE_UNKNOWN,
	eDEVICE_REGISTERED_ROAMING,
}eDeviceRegStatus_t;


typedef struct
{
	eDeviceRegStatus_t eFlteDeviceRegStatus;						//Device info. All calls will be tried after this call only
	uint32_t iFlteCarrierSignalStrength;							//Current Signal strength
	uint32_t iFlteCarrierResStatus;										//These 2 needed for indication and the Response
	int32_t iModemFd;
	int32_t iFAXfifofd;
	uint8_t *pFlteIMSINumber;
	uint8_t *pDeviceNumber;												//Number which everyone should try
}FlteDeviceInfo_t;


typedef struct
{
		const char *str[10];
}FlteAtPosResponses_t;





/*Function Pointer declarations*/
typedef int (*pfFlte_Process_Cmd)(FlteAtCmdArg_t *);				//Function for processing the the Command
typedef int (*pfFlte_Get_Current_Value) (FlteAtCmdArg_t *);		//It will give you the Current Value Stored in this
typedef int (*pfFlte_Get_All_Pos_Value)(FlteAtCmdArg_t *);	//This Function is responsible for Geting all the possible values for he command
typedef int (*pfFlte_Process_Update_Response)(FlteDeviceInfo_t *);		//Some Valid Device informations
typedef int (*pfFlte_Get_Response)(FlteAtCmdArg_t *);		//Some Valid Device informations
typedef int (*pfFlte_Get_Ack)(FlteAtCmdArg_t *);
typedef int (*pfFlte_Set_Parameter)(FlteAtCmdArg_t *);	//This Function is responsible for Geting all the possible values for he command
/*End of Function pointer declarations*/

/***FlteCommand Structure****/
typedef struct
{
	eFlteAtCmdType_t eFlteAtCmdType;
	uint32_t iFlteAtCmdTimeout;								//Timeout for the AT Commands Necessary for some commands
	const uint8_t *pFlteAtCmdName;							//Actual Command format
	const uint8_t *pFlteAtCmdGetCurValue;
	const uint8_t *pFlteAtCmdGetAllValue;
	const uint8_t *pFLTE_AT_CMD_DESC;						//Command Description for the usage
	uint32_t iFlteAtNoResponses;
	FlteAtPosResponses_t FlteAtPosResponses;				//Possible responses of the command
	//Function Pointers
	pfFlte_Process_Cmd pfflte_at_process_cmd;
	pfFlte_Get_Current_Value pfflte_get_current_value;
	pfFlte_Get_All_Pos_Value pfflte_get_all_pos_value;
	pfFlte_Set_Parameter pfflte_set_parameter;
	pfFlte_Process_Update_Response pfflte_process_update_response;
	pfFlte_Get_Response pfflte_get_response;
	pfFlte_Get_Ack pfflte_get_ack;
}FlteAtCmdDetails_t;
/**End of the Command Structure***/


//Name format eEVENT_<FROM>_<Details>
typedef enum
{
	eEVENT_LTE_INCOMING_CALL,				/*Incloming call info from the LTE module*/
	eEVENT_LTE_CARRIER_LOST,				/*Carrier Lost information from the LTE module*/
	eEVENT_FAX_OUT_CALL,					/*Outgoing Call request from the FAX application*/
	eEVENT_NO_EVENT,
	eEVENT_FAX_DISCONNECTED,
	eEVENT_FAX_CALL_OK,
	eEVENT_DISCONNECT_CALL,
}eEventName_t;

#define MAX_NUMBER_LENGTH 12
typedef struct
{
	unsigned char inumber[MAX_NUMBER_LENGTH+1];
}FAXIncomingEvent_t;

typedef struct
{
	unsigned char onumber[MAX_NUMBER_LENGTH+1];
	struct timeval *timeout;					//Timeout in seconds
	struct timeval dtmftimeout;

}FAXOutgoingEvent_t;


typedef struct
{
	FAXIncomingEvent_t FAXIncomingEvent;
	FAXOutgoingEvent_t FAXOutGoingEvent;
	eEventName_t FAXEventName;
	int fax_fifo_fd;
	int lte_fifo_fd;
	int modem_fifo_fd;
}FAXStruct_t;

/**Device Info Related Structures********************************************************************/
#define MAX_DEVICE_NUM_LEN  20
#define MAX_IMSI_LEN  20
#define MAX_IMEI_LEN  20
#define OFFSET_IMEI 9
#define OFFSET_IMSI 9
#define OFFSET_FAXNO

#define MAX_AT_RESP_LENGHTH	20

typedef struct
{
	/*FAX Number Array*/
	unsigned char FLTEDeviceNo[MAX_DEVICE_NUM_LEN];
	/*IMSI Number Array*/
	unsigned char FLTEimsiNo[MAX_IMSI_LEN];
	/*IMEI Number Array*/
	unsigned char FLTEimeiNo[MAX_IMEI_LEN];
}LTEDeviceInfo_t;
/*************************************Only Signal Strength is used Now****************************/
typedef struct
{
	unsigned char SignalStrength;
	unsigned char SystemStatus;
}LTEDevicesignalstrength_t;;

/*Enum for the Device info process function*/
typedef enum
{
	RESP_IMEI,
	RESP_IMSI,
	RESP_FAXNO,
}RespType_t;

typedef enum
{
	eDATA_PLMN,
	eEARFCN,
	eRSRP,
	eRSSI,
	eRSSQ,
	eTAC,
	eTXPWR,
	eDRX,
	eMM,
	eRRC,
	eCID,
	eIMSI,
	eNetNameAsc,
	eSD,
	eABND
}eSignalStrengthState_t;


#define COMMA_OFFSET_FOR_RSRP 2
/**********************************End of Device info related definitions**************************/


#define AT_HEADER1	10
#define AT_HEADER2	10


/***************************Function Declarations***************************************************/
void printATCommands();
int ProcessTestComand(FlteAtCmdArg_t *FLTE_AT_CMD_Arg);
int GetTheAck(char *message);
int process_cgreg_cmd(FlteAtCmdArg_t *FlteAtCmdArg);
int ReadnCheckAck(FlteAtCmdArg_t *FLTE_AT_CMD_Arg);
int set_cgreg_parameter(FlteAtCmdArg_t *FlteAtCmdArg);
int ProcessCommand(FlteAtCmdArg_t *FLTE_AT_CMD_Arg);
int SendEventToModem(FAXStruct_t *FAXEventInfo );
int CallSignalStrengthHandler();
int GettheSignalstrength(FlteAtCmdArg_t *FLTE_AT_CMD_Arg);
int CheckSignalStrengthACK(FlteAtCmdArg_t *FLTE_AT_CMD_Arg);
int GettheFAXNum(FlteAtCmdArg_t *FlteAtCmdArg);
int GettheIMEINum(FlteAtCmdArg_t *FlteAtCmdArg);;
int GettheIMSINum(FlteAtCmdArg_t *FlteAtCmdArg);
void AnswerCall();


#endif /* ATCOMMANDS_H_ */
