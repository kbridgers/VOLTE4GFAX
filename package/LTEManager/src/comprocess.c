/**************************************************************************************************
 * Name			:comprocess.c (Process Specific to serial communication)
 *
 *  Created on	: 10-Feb-2015
 *      Author	: sanju
 *************************************************************************************************/

/*Standard Defines*/
#include <stdio.h>
#include "comprocess.h"
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
/*Custome Defines*/
#include "config_device.h"
#include "ATCommands.h"
#include "config_file.h"

/*Not Using Now To be deleted*/
typedef enum
{
	AT_STATE_INITIALISING,
	AT_STATE_ERROR,
	AT_STATE_INIT_SUCCESS,
	AT_INIT_FAILURE,
	AT_INIT_PORT_NOT_RESPONDING,			//AT is not giving any echo back
	AT_STATE_ACTIVE,
}ComProcessState_t;

typedef enum
{
	CALL_NONE,
	CALL_FAX,
	CALL_VOICE
}CallType_t;

CallType_t FLTECallType;
int FLTECallFlag;
#if !ENABLE_AUTOFAX
	int FLTEIncomingCallFlag=RESET;
#endif
int FLTECallTimer;

#define BUFSIZE 20
#define STO STDOUT_FILENO
#define STI STDIN_FILENO
//#define FIFO_FILE       "MYFIFO1"

#include "term.h"
#include "termios.h"
#include "error_defines.h"
FLTE_STRUCT_ERROR_info_t ErrorInfo;
extern const FLTE_STRUCT_ERROR_type_t error_defines[];
typedef struct
{
	char start;
	char device_id;
	char checksum;
	char stop;
}pingpkt_t;



/*Pipe Receiver.c*/
#include <sys/stat.h>
#include <signal.h>
#include <sys/time.h>

#define FAX_FIFO "/tmp/FAX_FIFO"
#define LTE_FIFO "/tmp/LTE_FIFO"

typedef enum
{
	eFAX_EVENT_ON_HOOK,
	eFAX_EVENT_OFF_HOOK,
	eFAX_EVENT_DTMF_DETECTED,
	eFAX_EVENT_INIT_SUCCESS,					//Init Success Event
	eFAX_EVENT_INIT_FAILED,
	eFAX_EVENT_NO_EVENT,
	eFAX_EVENT_PKT_FAILED,
	eFAX_EVENT_INCOMING_CALL,
	eFAX_FAX_CALL_DETECTED,
}FAXEventType_t;							//This is the Event from the Tapi aplication to LTE application

typedef struct
{
	FAXEventType_t FAXEventType;
	unsigned char FAXEventData;					//Data like key 1,2,3,4 etc.
}FAXEvent_t;

typedef struct
{
	unsigned char PacketStart;
	FAXEvent_t FAXData;
	unsigned char PacketStop;
}FAXEventPacket_t;

#define FAX_DATA_PACKET_START '%'
#define FAX_DATA_PACKET_END	'*'

FlteDeviceInfo_t FlteDeviceInfo;
#if ENABLE_PING
int ping_fifo_fd;
#endif
typedef enum
{
	PKT_IDLE,
	PKT_START,
	PKT_EVENT_TYPE,
	PKT_EVENT_DATA,
	PKT_EVENT_STOP

}PktState_t;

#if 0
void timer_handler (int signum)
{
 static int count = 0;
//++gVal;
 printf ("timer expired %d times\n", ++count);
}
#endif
struct sigaction sa;
struct itimerval timer;


/*DTMF Tone index*/
 typedef enum
 {
	 DTMF_1=1,
	 DTMF_2,
	 DTMF_3,
	 DTMF_4,
	 DTMF_5,
	 DTMF_6,
	 DTMF_7,
	 DTMF_8,
	 DTMF_9,
	 DTMF_STAR,
	 DTMF_0,
	 DTMF_HASH

 }key_index_t;

 unsigned char get_the_ascii_char(unsigned char data)
 {
	 switch(data)
	 {
	 	 case DTMF_1:
	 	 case DTMF_2:
	 	 case DTMF_3:
	 	 case DTMF_4:
	 	 case DTMF_5:
	 	 case DTMF_6:
	 	 case DTMF_7:
	 	 case DTMF_8:
	 	 case DTMF_9:
	 		 return data+0x30;					//Direct returning Char data
	 		 break;
	 	 case DTMF_0:
	 		 return '0';
	 		 break;
	 	 case DTMF_STAR:
	 		 return '*';
	 		 break;
	 	 case DTMF_HASH:
	 		 return '#';
	 		 break;
	 }
 }

#include "ATCommands.h"
#include <termios.h>

 /*************************************************************************************************
  * @func_name		:	open_modem_device
  * @desc			:	Opening the ttyUSB pot of the modem
  * @param			:	NULL
  * @return			:	status
  ************************************************************************************************/
 int open_modem_device()
 {
 	struct termios options;
 	//Add the Modem  Init Section Section Now
 	FlteDeviceInfo.iModemFd = open(MODEMDEVICE, O_RDWR | O_NOCTTY|O_NDELAY);
	if (FlteDeviceInfo.iModemFd < 0) {
		//perror("No device Found\n");
		ErrorInfo.ErrorType=FLTE_APP_ERROR_MODEM_DEVICE_BUSY_OR_NOT_FOUND;
		error_defines[ErrorInfo.ErrorType].FLTE_APP_ERROR_ACTION(ErrorInfo);
		/*No Matter in Continuing*/
		/*When We are exiting Kill the tapidemopipe application also*/
		system("killall FAXManager");
		exit(0);
		return -1;
	}
	fcntl(FlteDeviceInfo.iModemFd,F_SETFL,O_RDWR);
	/* get the current options */
	tcgetattr(FlteDeviceInfo.iModemFd, &options);
	/* set raw input, 1 second timeout */
	options.c_cflag     |= (CLOCAL | CREAD);
	options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag     &= ~OPOST;
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME] = 10;
	speed_t baudrate = B115200; //termios.h: typedef unsigned long speed_t;
	cfsetospeed(&options,baudrate);
	cfsetispeed(&options,baudrate);
	//options.c_ispeed=115200;
	//options.c_ospeed=115200;
	tcsetattr(FlteDeviceInfo.iModemFd, TCSANOW, &options);
	return 0;
 }


#if 0

/*It is Disabling the timer*/
 void disable_timer()
 {
	 timer.it_value.tv_sec = 0;
	 timer.it_value.tv_usec = 0;
	 /* ... and every 250 msec after that. */
	 timer.it_interval.tv_sec = 1;
	 timer.it_interval.tv_usec = 0;
	 setitimer (ITIMER_REAL, &timer, NULL);
 }
/*It is enabling the timer*/
 void enable_timer()
 {
	 timer.it_value.tv_sec = 1;
	 timer.it_value.tv_usec = 0;
	 /* ... and every 250 msec after that. */
	 timer.it_interval.tv_sec = 1;
	 timer.it_interval.tv_usec = 0;
	 setitimer (ITIMER_REAL, &timer, NULL);
 }
#endif

 /*************************************************************************************************
  * @func_name		:	processPacket
  * @desc			:	processing the packet from the tapidemo application
  * @param			:	data. processed one character only once
  * @return			:	FAXEvent_t. Return an event if event processing success
  ************************************************************************************************/
FAXEvent_t processPacket(char data)
{
	static PktState_t PktState=PKT_IDLE;
	static FAXEvent_t FAXEvent;

	static int eventType=eFAX_EVENT_NO_EVENT;
	if(data==FAX_DATA_PACKET_START&&PktState==PKT_IDLE)
	{
		PktState=PKT_START;
		FAXEvent.FAXEventType=eFAX_EVENT_NO_EVENT;
	}
	switch(PktState)
	{
		case  PKT_START:			//Starting the packet
			PktState=PKT_EVENT_TYPE;
			break;
		case  PKT_EVENT_TYPE:		//Event Types
			//FAXEvent.FAXEventType=data;
			eventType=data;
			PktState=PKT_EVENT_DATA;
			break;
		case  PKT_EVENT_DATA:		//Data related with the event
			FAXEvent.FAXEventData=data;
			PktState=PKT_EVENT_STOP;
			break;
		case  PKT_EVENT_STOP:		//Stop Packet
			if(data==FAX_DATA_PACKET_END)
			{
				//Your data here
				//Your Packet is fine
				//break;
				FAXEvent.FAXEventType=eventType;
#if EN_DEBUG_MSGS
				printf("FLTE:Event Type is %d\n",eventType);
#endif
				eventType=eFAX_EVENT_NO_EVENT;

			}
			else
			{
				//return the error message
				FAXEvent.FAXEventType=eFAX_EVENT_NO_EVENT;			//Something Went wrong dont receive the packet
			}
			PktState=PKT_IDLE;
			break;
		case PKT_IDLE:				//Idle Condition
			break;
		default:
#if EN_DEBUG_MSGS
			printf("FLTE: Unexpected entry\n");
#endif
			break;
	}
	return FAXEvent;
}


#define MAX_NUM_COUNT 		12

/*************************************************************************************************
 * @func_name		:	ProcessFAXEvent
 * @desc			:	processing the event received from the tapidemo application
 * @param			:	FAXEvent_t,FAXStruct_t
 * @return			:	status
 ************************************************************************************************/
int ProcessFAXEvent(FAXEvent_t FAXEvent,FAXStruct_t *FAXInfo)
{
	static int fire_timer_flag=1,disable_timer_flag=0,dtmf_number_cnt=0;
	static unsigned char numbers[MAX_NUM_COUNT+1];


	switch(FAXEvent.FAXEventType)
	{
		case eFAX_EVENT_ON_HOOK:
			//disable_timer();									//Onhook detected I dont need any timer now
			//numbers[MAX_NUM_COUNT+1]=0;
			memset(FAXInfo->FAXOutGoingEvent.onumber,RESET,MAX_NUMBER_LENGTH+1);
			fire_timer_flag=RESET;
			dtmf_number_cnt=RESET;
			system("echo 6 > /dev/fifo_led");				//Please PUT LED in PWR_ON state when off hook
			//timeout=NULL;
#if EN_DEBUG_MSGS
			printf("FLTE:On hook Event From FAXManager\n");
#endif
			break;
		case eFAX_EVENT_OFF_HOOK:
			fire_timer_flag=RESET;
			dtmf_number_cnt=RESET;
			FLTECallFlag=RESET;
			//numbers[MAX_NUM_COUNT+1]=0;
			memset(FAXInfo->FAXOutGoingEvent.onumber,0,MAX_NUM_COUNT+1);
#if EN_DEBUG_MSGS
			printf("FLTE:Off hook Event From Tapi Application\n");
#endif
			//	timeout=NULL;
			FLTECallTimer=RESET;
			FLTECallType=CALL_NONE;
			break;
		case eFAX_FAX_CALL_DETECTED:
#if EN_DEBUG_MSGS
			printf("eFAX_FAX_CALL_DETECTED- LTEManager\n");
#endif
			break;
		case eFAX_EVENT_DTMF_DETECTED:
			 /* Configure the timer to expire after 250 msec... */
			if(MAX_NUM_COUNT>dtmf_number_cnt)
			{
				if(fire_timer_flag==RESET)				//Only first time
				{
				//	enable_timer();
				}
				FAXInfo->FAXOutGoingEvent.onumber[dtmf_number_cnt]=get_the_ascii_char(FAXEvent.FAXEventData);
				dtmf_number_cnt++;
				disable_timer_flag=RESET;
				//timeout->=
#if EN_DEBUG_MSGS
				printf("FLTE: DTMF detected From Tapi Application- %d\n",FAXEvent.FAXEventData);
#endif
			}
			else
			{
				if(disable_timer_flag==RESET)
				{
					disable_timer_flag=SET;
#if EN_DEBUG_MSGS
					printf("Dialling Number with telit Module %s: No more keys allowed\n",numbers);
#endif
				}
			}
			 /* ... and every 250 msec after that. */
			// timer.it_interval.tv_sec = 1;
			// timer.it_interval.tv_usec = 0;

			fire_timer_flag=SET;
			break;
		case eFAX_EVENT_INIT_SUCCESS:
#if EN_DEBUG_MSGS
			printf("Init Success Event From Tapi Application\n");
#endif
			break;
		case eFAX_EVENT_PKT_FAILED:
#if EN_DEBUG_MSGS
			printf("Pkt Event failed From Tapi Application\n");
#endif
			break;
		case eFAX_EVENT_INIT_FAILED:
#if EN_DEBUG_MSGS
			printf("Event init failed From Tapi Application\n");
#endif
			break;
	}
	return 0;
}

#if ENABLE_PING
int sendPing()
{
	unsigned char str[4];
	str[0]='$';									/*FAX_DATA_PACKET_START-Add later*/
	str[1]=LTE_MANAGER_PROCESSID<<4|1;			/*FaxEvent.FAXEventType;*/
	str[2]='$'^str[1]^'#';						/*FaxEvent.FAXEventData;*/
	str[3]='#';									/*FAX_DATA_PACKET_END;*/
	//write(FlteDeviceInfo.iFAXfifofd,"Hello",PACKET_SIZE);
	write(ping_fifo_fd,str,4);
}
#endif



/*************************************************************************************************
 * @func_name		:	waitforevent
 * @desc			:	select based wait mechanism for all the fd's
 * @param			:	FAXStruct_t
 * @return			:	eEventName_t (Received event)
 ************************************************************************************************/
eEventName_t waitforevent(FAXStruct_t *FaxInfo)
{
	int status,n,time_flag=0,SignalstrengthCheckFlag=0;
	char d;
	fd_set rdset;
	FlteAtCmdArg_t FLTEArgsInfo;
    struct timeval timeout= {10, 0};
    FAXEvent_t FAXEvent;
    /*Timeout Section*/
    SignalstrengthCheckFlag=SET;
	while(1)
	{
		FD_ZERO(&rdset);
		FD_SET(FaxInfo->modem_fifo_fd, &rdset);
		FD_SET(FaxInfo->fax_fifo_fd, &rdset);
		status=select(FaxInfo->fax_fifo_fd+2, &rdset, NULL, NULL,
				SignalstrengthCheckFlag?&timeout:FaxInfo->FAXOutGoingEvent.timeout);
		//printf("Event Detected\n");
		if ( status< 0)
		{
#if EN_DEBUG_MSGS
			printf("select failed: %d : %s", errno, strerror(errno));
#endif
			if(errno!=4)					/*Timer May get fired during the event*/
				exit(128);
		}
		FaxInfo->FAXEventName=eEVENT_NO_EVENT;
		//printf("Event Came : Select return is: %d :\n",status);
		if ( FD_ISSET(FaxInfo->fax_fifo_fd, &rdset) )
		{
			do {
				n = read(FaxInfo->fax_fifo_fd, &d, 1);
				FAXEvent=processPacket(d);
				if(FAXEvent.FAXEventType!=eFAX_EVENT_NO_EVENT)
				{
					ProcessFAXEvent(FAXEvent,FaxInfo);
					if(FAXEvent.FAXEventType==eFAX_EVENT_DTMF_DETECTED)
					{
						//printf("Setting Timeout\n");
						SignalstrengthCheckFlag=RESET;
						FaxInfo->FAXOutGoingEvent.dtmftimeout.tv_sec=LTEMANAGER_CALL_TIMEOUT_S;		/*@todo:Timeout configure*/
						FaxInfo->FAXOutGoingEvent.timeout=&FaxInfo->FAXOutGoingEvent.dtmftimeout;
					}
					else if (FAXEvent.FAXEventType==eFAX_EVENT_ON_HOOK)
					{
						SignalstrengthCheckFlag=SET;
#if !ENABLE_AUTOFAX
						FLTEIncomingCallFlag=RESET;
#endif
						timeout.tv_sec=LTEMANAGER_INITIAL_DTMF_TIMEOUT_S;
#if EN_DEBUG_MSGS
						printf("Disconnecting the Call\n");
#endif
						FaxInfo->FAXEventName=eEVENT_FAX_DISCONNECTED;
						FLTECallType=CALL_NONE;			//clear the Call FLAG
						FLTECallFlag=0;

					}
					else if(FAXEvent.FAXEventType==eFAX_EVENT_OFF_HOOK)
					{
#if !ENABLE_AUTOFAX
						if(FLTEIncomingCallFlag==SET)
						{
							//It means the host FAX machine answerd the Call Manuall or Auto
							AnswerCall();
							FLTEIncomingCallFlag=RESET;
						}
#endif
					}
					else if (FAXEvent.FAXEventType==eFAX_FAX_CALL_DETECTED)
					{
						//SignalstrengthCheckFlag=1;
						//timeout.tv_sec=5;
						if(FLTECallType==CALL_VOICE)
						{
#if EN_DEBUG_MSGS
							printf("Disconnecting the timer\n");
#endif
							FaxInfo->FAXEventName=eEVENT_FAX_CALL_OK;
						}
						else
						{
							//Probably trying the tone with the DTMF tones.
							FLTECallFlag=SET;
						}
					}

					else
					{
#if EN_DEBUG_MSGS
						printf("Setting NULL\n");
#endif
						FaxInfo->FAXOutGoingEvent.timeout=NULL;
					}
					FAXEvent.FAXEventType=eFAX_EVENT_NO_EVENT;
				}
			} while (n < 0);
		}
#if 1
		/*Checking For Modem Character recieved or not.. For notifications like "RING","CGREG" etc*/
		if ( FD_ISSET(FaxInfo->modem_fifo_fd, &rdset) )
		{
			int i=RESET,n,status=RESET;
			char str[20];
			memset(str,0,20);							//On Every try this should be cleared
#if EN_DEBUG_MSGS
			printf("Telit Notification came\n");
#endif
			i=RESET;
			do {
				n = read(FaxInfo->modem_fifo_fd, &str[i++], 1);
			} while (n > 0);
			/*@todo :str[0]& str[1] is the response header. We will add later*/
			if(str[LTE_MANAGER_NOTIFICATION_INDEX]=='R'&&str[LTE_MANAGER_NOTIFICATION_INDEX+1]=='I'
					&&str[LTE_MANAGER_NOTIFICATION_INDEX+2]=='N'&&str[LTE_MANAGER_NOTIFICATION_INDEX+3]=='G')
			{
				FAXEvent_t event;
				event.FAXEventType=eFAX_EVENT_INCOMING_CALL;
#if !ENABLE_AUTOFAX
				FLTEIncomingCallFlag=1;
#endif
				system("echo 5 > /dev/fifo_led");
#if EN_DEBUG_MSGS
				printf("FLTE:Incoming call \n",str);
#endif
				event.FAXEventData=RESET;
				FLTECallType=CALL_VOICE;			//Added for incoming voice call rejection
				system("echo RING > /tmp/LTE_FIFO");

				//sendEventToPipe(event);
			}
			/*Checking the notification messages also*/
			else if (str[LTE_MANAGER_NOTIFICATION_INDEX]=='+'&&str[LTE_MANAGER_NOTIFICATION_INDEX+1]=='C'
					&&str[LTE_MANAGER_NOTIFICATION_INDEX+2]=='G'&&str[LTE_MANAGER_NOTIFICATION_INDEX+3]=='R'
							&&str[LTE_MANAGER_NOTIFICATION_INDEX+4]=='E'&&str[LTE_MANAGER_NOTIFICATION_INDEX+5]=='G'
									&&str[LTE_MANAGER_NOTIFICATION_INDEX+6]==':'&& str[LTE_MANAGER_NOTIFICATION_INDEX+7]==' ')
			{
				FlteDeviceInfo.eFlteDeviceRegStatus=str[10]-0x30;
				if(FlteDeviceInfo.eFlteDeviceRegStatus!=FLTE_REG_STATUS_OK)
				{
					/*Device Registration Failed*/
					system("echo 4 > /dev/fifo_led");
				}
				else
				{
					/*Device Registration Success Again*/
					system("echo 6 > /dev/fifo_led");
				}
			}
#if EN_DEBUG_MSGS
			//printf("Characters %x,%x,%x,%x,%x,%x,%x\n",str[0],str[1],str[2],str[3],str[4],str[5],str[6]);
			printf("FLTE:Received notification is : %s :",str);
#endif
		}
#endif

		/*Whether select exited because of timeout??*/
		if(status==FLTE_EVENT_TIMEOUT)			//select Exited because of the timeout only
		{						//Once timeout detected Now there is no timeout only dialling through AT commands
			if(SignalstrengthCheckFlag==RESET)
			{
				FaxInfo->FAXOutGoingEvent.timeout=NULL;
				FaxInfo->FAXEventName=eEVENT_FAX_OUT_CALL;
			}
			else
			{
				CallSignalStrengthHandler();
#if ENABLE_PING
				sendPing();
#endif
				timeout.tv_sec=FLTE_SIGNAL_STRENGTH_TIMEOUT;
				if(FLTECallType!=CALL_NONE)
				{
					FLTECallTimer+=FLTE_SIGNAL_STRENGTH_TIMEOUT;
				}
				if(FLTECallType==CALL_VOICE&&FLTECallTimer>=FLTE_VOICE_CALL_REJECT_TIMEOUT)
				{
#if EN_DEBUG_MSGS
					printf("FLTE: Voice Call Rejecting \n");
#endif
					FaxInfo->FAXEventName=eEVENT_DISCONNECT_CALL;
					FLTECallType=CALL_NONE;
				}
			}
		}
		//return time_flag;
		if(FaxInfo->FAXEventName!=eEVENT_NO_EVENT)
		{
#if EN_DEBUG_MSGS
			//printf("Break:No Event\n");
#endif
			break;										/*One Main event is yet to process go out and process*/
		}
	}
	/*Returning the Event*/
	return FaxInfo->FAXEventName;
}




/*************************************************************************************************
 * @func_name		:	OpenFAXPipe
 * @desc			:	Opening the FAX pipe
 * @param			:	void
 * @return			:	return status;
 ************************************************************************************************/
int OpenFAXPipe()
{
	FlteDeviceInfo.iFAXfifofd = open("/tmp/LTE_FIFO", O_WRONLY|O_APPEND);
#if EN_DEBUG_MSGS
	printf("LTE FiFo opened\n");
#endif
	if (FlteDeviceInfo.iFAXfifofd<0)
	{
#if EN_DEBUG_MSGS
		printf("Error in opening the FiFO\n");
		printf("FIFO Creation Failed\n");
#endif
		system("killall FAXManager");
#if EN_DEBUG_MSGS
		printf("Restart Needed \n");
#endif
		system("echo 4 > /dev/fifo_led");
#if EN_DEBUG_MSGS
		printf("Error In opening the Fifo\n");
#endif
		exit(0);
	}
	fcntl(FlteDeviceInfo.iFAXfifofd,F_SETFL,0);
	return 1;
}

#if 1
/*************************************************************************************************
 * @func_name		:	sendEventToPipe
 * @desc			:	RING Event to FAX Pipe
 * @param			:	FAXEvent_t
 * @return			:	return status;
 ************************************************************************************************/
int sendEventToPipe(FAXEvent_t FaxEvent)
{
	FAXEventPacket_t FaxEventPacket;
	unsigned char str[4];
	str[0]='R';					/*FAX_DATA_PACKET_START-Add later*/
	str[1]='I';						/*FaxEvent.FAXEventType;*/
	str[2]='N';						/*FaxEvent.FAXEventData;*/
	str[3]='G';						/*FAX_DATA_PACKET_END;*/
	//write(FlteDeviceInfo.iFAXfifofd,"Hello",PACKET_SIZE);
	write(FlteDeviceInfo.iFAXfifofd,str,4);
}


#endif
/*************************************************************************************************
 * @func_name		:	close_LTE_Pipe
 * @desc			:	Closing pipe
 * @param			:	FAXEvent_t
 * @return			:	return status;
 ************************************************************************************************/
int close_LTE_Pipe()
{
	close(FlteDeviceInfo.iFAXfifofd);				//Closing the pipe
}

void AnswerCall()
{
	FlteAtCmdArg_t FLTEArgsInfo;
	FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_ANS_CALL;
	FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
	FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
	ProcessTestComand(&FLTEArgsInfo);
}


/*************************************************************************************************
 * @func_name		:	ATcommand_process
 * @desc			:	MAin AT Command process entry. forked from main file
 * @param			:
 * @return			:	null
 ************************************************************************************************/
int ATcommand_process()
{
	int status,lte_fifo_fd,lte_device_fd,n;
	fd_set rdset, wrset;
	FlteAtCmdArg_t FLTEArgsInfo;
	pingpkt_t pngpkt;
	//int fax_fifo;
	/*Timer Exit*/
	FAXEvent_t FAXEvent;
	char d;
	FAXStruct_t FAXEventInfo;
	struct termios options;
	FLTECallType=CALL_NONE;
#if ENABLE_PING	/*@todo:Add Later*/
	ping_fifo_fd= open("PING_FIFO", O_WRONLY | O_APPEND);
	fcntl(ping_fifo_fd,F_SETFL,0);
#endif
	//printf("After opening\n");
//LTE FIFO
#if 0		/*@todo:Add Later*/
	lte_fifo_fd = open("LTE_FIFO", O_RDONLY);
	if (lte_fifo_fd<0)
	{
		ErrorInfo.ErrorType=FLTE_APP_ERROR_FAILED_TO_OPEN_PING_PIPE;
		error_defines[FLTE_APP_ERROR_FAILED_TO_OPEN_PING_PIPE].FLTE_APP_ERROR_ACTION(ErrorInfo);
	}
	fcntl(lte_fifo_fd,F_SETFL,0);
#endif
	OpenFAXPipe();
	FAXEventInfo.lte_fifo_fd=FlteDeviceInfo.iFAXfifofd;
#if EN_DEBUG_MSGS
	printf("Lte Fifo opened %d\n",FAXEventInfo.lte_fifo_fd);
#endif
	//signal(SIGALRM, timer_handler);
	fcntl(FAXEventInfo.lte_fifo_fd,F_SETFL,0);
	open_modem_device();
	FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_TEST_CMD;
	FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
	FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
	ProcessTestComand(&FLTEArgsInfo);
	FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_REG_STATUS;
	FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
	FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_GET_CUR_DATA;
	ProcessTestComand(&FLTEArgsInfo);
	if(FlteDeviceInfo.eFlteDeviceRegStatus!=FLTE_REG_STATUS_OK)
	{
#if EN_DEBUG_MSGS
		printf("Device Registration Failed : 2 \n");
#endif
		system("killall FAXManager");
#if EN_DEBUG_MSGS
		printf("Restart Needed \n");
#endif
		system("echo 4 > /dev/fifo_led");
		while(1);
		exit(0);
	}
/*@todo:Rearrange Added for Demo*/
	int fax_fifo,lte_fifo;
	fax_fifo = open(FAX_FIFO, O_RDONLY);
	if (fax_fifo<0)
	{
#if EN_DEBUG_MSGS
		printf("FIFO Creation Failed\n");
#endif
		system("killall FAXManager");
#if EN_DEBUG_MSGS
		printf("Restart Needed \n");
#endif
		system("echo 4 > /dev/fifo_led");
#if EN_DEBUG_MSGS
		printf("Error In opening the Fifo\n");
#endif
		exit(0);
	}
	FAXEventInfo.fax_fifo_fd=fax_fifo;
	FAXEventInfo.modem_fifo_fd=FlteDeviceInfo.iModemFd;
	fcntl(fax_fifo,F_SETFL,0);
/*@todo:Rearrange Added for Demo*/
	 struct timeval *tv=NULL;
	 struct timeval timeout= {LTEMANAGER_INITIAL_DTMF_TIMEOUT_S, LTEMANAGER_INITIAL_DTMF_TIMEOUT_MS};
	 tv=&timeout;
	 //Assigning Null
	 FAXEventInfo.FAXOutGoingEvent.timeout=NULL;
	 //Desired DTMF Timeout
	 FAXEventInfo.FAXOutGoingEvent.dtmftimeout.tv_sec=LTEMANAGER_INITIAL_DTMF_TIMEOUT_S;
	 FAXEventInfo.FAXOutGoingEvent.dtmftimeout.tv_usec=LTEMANAGER_INITIAL_DTMF_TIMEOUT_MS;
	 /*@todo: Dont use system command use write instead after demo*/
	 system("echo 6 > /dev/fifo_led");				//6 Means the initialisation Successful
	 FLTEArgsInfo.eFlteAtCmdType=eFLTE_AT_CMD_TYPE_TEST_CMD;
	 FLTEArgsInfo.iModemFd=FlteDeviceInfo.iModemFd;
	 FLTEArgsInfo.eFlteAtCmdFormat=eAT_CMD_FORMAT_DIRECT_COMMAND;
	 ProcessTestComand(&FLTEArgsInfo);
	 getImeiImsiFaxNo();
#if ENABLE_AUTOFAX
	 setAutoAnswer();
#endif
	 while(1)
	 {
		 /*Wait Here till an event comes. Why we need to waste the CPU time in polling*/
		 FAXEventInfo.FAXEventName=waitforevent(&FAXEventInfo);
#if EN_DEBUG_MSGS
		 printf("FLTE: Event Came\n");
#endif
		 /*Process the Ned Event Received*/
		 switch(FAXEventInfo.FAXEventName)
		 {
		 	 case eEVENT_LTE_INCOMING_CALL:							/*RING message from the LTE module*/
		 		 break;
		 	 case eEVENT_FAX_OUT_CALL:								/*Event from the FAX application like DTMF tone*/
#if EN_DEBUG_MSGS
		 		 printf("FLTE:Dialling Number- %s : Timeout Reached\n",FAXEventInfo.FAXOutGoingEvent.onumber);
#endif
		 		 system("echo 5 > /dev/fifo_led");
		 		 status=SendEventToModem(&FAXEventInfo);
		 		 if(status!=0)
		 		 {
#if EN_DEBUG_MSGS
		 			 printf("FLTE:Outgoing Failed\n");
#endif
		 			 system("echo 6 > /dev/fifo_led");
		 		 }
		 		 else
		 		 {
		 			 if(FLTECallFlag==SET)
		 			 {
#if EN_DEBUG_MSGS
		 				printf("FLTE:Call Changed to FAX Automatically\n");
#endif
		 				FLTECallType=CALL_FAX;
		 			 }
		 			 else
		 			 {
#if EN_DEBUG_MSGS
		 				 printf("Call changed to Voice Auto\n");
#endif
		 				 FLTECallType=CALL_VOICE;
		 			 }

		 			FLTECallTimer=RESET;
		 			 /*@todo: Dont use system command use write instead*/
		 			 system("echo 5 > /dev/fifo_led");				//6 Means the initialisation Successful
		 			 //printf("Outgoing  Succesfully\n");
		 		 }
		 		 /*After the processing Assign No Event*/
		 		 FAXEventInfo.FAXEventName=eEVENT_NO_EVENT;
		 		 break;
		 	 case eEVENT_FAX_CALL_OK:							/*Auto Message from the LTE module*/
		 		 /*I got A FAX call info from tapi so dont cut the call proceed*/
		 		 /**/
#if EN_DEBUG_MSGS
		 		 printf("call type changed  to FAX\n");
#endif
		 		 FLTECallType=CALL_FAX;
		 		 break;
		 	 case eEVENT_DISCONNECT_CALL:
#if EN_DEBUG_MSGS
		 		 printf("FLTE:Disconnecting the Call\n");
#endif
		 		 FAXEventInfo.FAXEventName=eEVENT_FAX_DISCONNECTED;
		 		 status=SendEventToModem(&FAXEventInfo);
		 		 if(status!=0)
		 		 {
#if EN_DEBUG_MSGS
		 			 printf("Error In Disconnecting\n");
#endif

		 		 }
		 		 else
		 		 {
		 			 ;//printf("Disconnected Succesfully\n");
		 		 }
		 		 FAXEventInfo.FAXEventName=eEVENT_NO_EVENT;
		 		 break;
		 	 case eEVENT_LTE_CARRIER_LOST:							/*Auto Message from the LTE module*/
		 		 break;
		 	 case eEVENT_FAX_DISCONNECTED:
		 		 status=SendEventToModem(&FAXEventInfo);
		 		 if(status!=0)
		 		 {
#if EN_DEBUG_MSGS
		 			 printf("Error In Disconnecting\n");
#endif
		 		 }
		 		 else
		 		 {
		 			 ;//printf("Disconnected Succesfully\n");
		 		 }
		 		 FAXEventInfo.FAXEventName=eEVENT_NO_EVENT;
		 		 break;
		 	 case eEVENT_NO_EVENT:
		 		 break;

		 }
	 }
	 close(fax_fifo);
	 /*@todo: Need to close Modem also*/
	 return 0;
}


