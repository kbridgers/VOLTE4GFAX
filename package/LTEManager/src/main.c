#include <stdio.h>
#include "unistd.h"
#include "comprocess.h"
#include "audioprocess.h"
#include "monitorprocess.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>

#define _GNU_SOURCE
#include <getopt.h>

/*Application includes*/
#include "term.h"
#include "termios.h"
#include "ping.h"
#include "config_file.h"
/*All the FIFO names*/
#define PING_FIFO       "/tmp/PING_FIFO"
#define LTE_FIFO        "/tmp/LTE_FIFO"
#define FAX_FIFO   		"/tmp/FAX_FIFO"

/*All the Corresponding keys*/
#define SYS_MSG_KEY	0x47e8		/*< Message key used by system server.*/


#define CHILD_PROCESS	0
#define ERROR_PROCESS	-1
#define PARENT_PROCESS  0xFF;


typedef enum
{
	STATE_INIT,
	STATE_INIT_SUCCESS,
	STATE_MONITOR,
	STATE_STATUS_MSG,
	STATE_ERROR,
	STATE_KILL_PROCESS,
}eMonitorStatus_t;


typedef enum
{
	FLAG_NOT_CREATED,
	FLAG_RUNNING,
	FLAG_KILLED,
	FLAG_NOT_RESPONDING,
}eProcessState_t;


#define STO STDOUT_FILENO
#define STI STDIN_FILENO

/**/
/*
 * We need 3 pipes to create
 * 1. Ping pipe To the MOnitor Application (Read in Monitor App Write From LTE and FAX App) Time interval should be maximum
 * 2. LTE Pipe To Read in LTE App Write from FAX App (Select wait on the LTE App).
 * 3. Audio Pipe yo Read in FAX app from the LTE App (Select wait on FAX app).
 * Make this basic frame work working.
 * */
#include "error_defines.h"

FLTE_STRUCT_ERROR_info_t ErrorInfo;
extern const FLTE_STRUCT_ERROR_type_t error_defines[];

int create_pipes()
{
	int status;
	/* Create the FIFO if it does not exist. We need to do this before the process getting spawned */
    umask(0);
    status=mknod(PING_FIFO, S_IFIFO|0666, 0);
    if(status<0&&errno!=17)
    {
    	ErrorInfo.ErrorType=FLTE_APP_ERROR_FAILED_TO_CREATE_PING_PIPE;
    	error_defines[FLTE_APP_ERROR_FAILED_TO_CREATE_PING_PIPE].FLTE_APP_ERROR_ACTION(ErrorInfo);
    	return -1;		//Failed
    }
    status=mknod(LTE_FIFO, S_IFIFO|0666, 0);
    if(status<0&&errno!=17)
    {
    	ErrorInfo.ErrorType=FLTE_APP_ERROR_FAILED_TO_CREATE_LTE_PIPE;
    	error_defines[FLTE_APP_ERROR_FAILED_TO_CREATE_LTE_PIPE].FLTE_APP_ERROR_ACTION(ErrorInfo);
    	return -1;		//Failed
    }

    status=mknod(FAX_FIFO, S_IFIFO|0666, 0);
    if(status<0&&errno!=17)
    {
    	ErrorInfo.ErrorType=FLTE_APP_ERROR_FAILED_TO_CREATE_AUDIO_PIPE;
    	error_defines[FLTE_APP_ERROR_FAILED_TO_CREATE_AUDIO_PIPE].FLTE_APP_ERROR_ACTION(ErrorInfo);
    	return -1;		//Failed
    }
    return 0;			//Success
}

void interrupt_handler()
{
	//CTRL+C event happened
	//Need to Kill all the remaining applications
#if EN_DEBUG_MSGS
	printf("Control + C happened: Please Kill all the remaining applications \n");
	//FD_SET(ping_fifo_fd, &rdset);
#endif
	system("killall FAXManager");
	exit(0);
}

/*********************************************************************
 * Name : Main.c
 *
 ********************************************************************/
int main()
{
	//pid_t child_id;
	int processflag;

	PingPacketData_t PingPacketData;
	eProcessState_t eATProcessFlag=FLAG_NOT_CREATED,eTAPIProcessFlag=FLAG_NOT_CREATED,eLEDProcessFlag=FLAG_NOT_CREATED;
	//First Process
	eMonitorStatus_t eMonitorStatus=STATE_INIT;
	int status,n;
	int SleepTimeout=0;
	int err=errno;
	int ping_fifo_fd;
	fd_set rdset, wrset;
	char d;
	/*Create System Pipes*/
	if(create_pipes()==-1)
	{
#if EN_DEBUG_MSGS
		printf("Pipe Creation Failed\n");
#endif
		system("echo 4 > /dev/fifo_led");
		return -1;							//No need to proceed further No IPC
	}
		/*Three Pipes Successfully created*/
    //Make the Communication Fine
    struct timeval tv = {1000, 0};
    //sleep(3);
    system("FAXManager &");			//Starting the tapidemo process on background
    /*We will do LED manager also the same way*/
   // system("/tmp/ledmanager &");
	while(1)
	{
		switch(eMonitorStatus)
		{
			case STATE_INIT:			//Initialisation State forking processes
				//Creating A pipe for the IPC
				if(FLAG_NOT_CREATED==eATProcessFlag||FLAG_KILLED==eATProcessFlag)	//Process not created or Process May get killed
				{
					processflag=fork();
					if(CHILD_PROCESS==processflag)		//Child process
					{
						/*Go to your process*/
						ATcommand_process();					//It wont return
						return -1;
					}
					else if(CHILD_PROCESS<processflag)		//Child process
					{
					    signal (SIGINT,interrupt_handler);
						eATProcessFlag=FLAG_RUNNING;
					}
					else
					{
						ErrorInfo.ErrorType=FLTE_APP_ERROR_FAILED_TO_CREATE_LTE_PROCESS;
						error_defines[FLTE_APP_ERROR_FAILED_TO_CREATE_LTE_PROCESS].FLTE_APP_ERROR_ACTION(ErrorInfo);
					}
				}
#if 0
				if(FLAG_NOT_CREATED==eTAPIProcessFlag||FLAG_KILLED==eTAPIProcessFlag)	//Process not created or Process May get killed
				{
					processflag=fork();
					if(CHILD_PROCESS==processflag)		//Child process
					{
						/*Go to Your process*/
						Audio_process();				//It should not return
						return -1;
					}
					else if(CHILD_PROCESS<processflag)		//Child process
					{
						eTAPIProcessFlag=FLAG_RUNNING;
					}
					else//Means it is a negative value
					{
						ErrorInfo.ErrorType=FLTE_APP_ERROR_FAILED_TO_CREATE_FAX_PROCESS;
						error_defines[FLTE_APP_ERROR_FAILED_TO_CREATE_FAX_PROCESS].FLTE_APP_ERROR_ACTION(ErrorInfo);
					}
				}

				if(FLAG_NOT_CREATED==eLEDProcessFlag||FLAG_KILLED==eLEDProcessFlag)	//Process not created or Process May get killed
				{
					processflag=fork();
					if(CHILD_PROCESS==processflag)		//Child process
					{
						/*Go to Your process*/
						//LED_process();				//It should not return Add later
						return -1;
					}
					else if(CHILD_PROCESS<processflag)		//Child process
					{
						eLEDProcessFlag=FLAG_RUNNING;
					}
					else//Means it is a negative value
					{
						ErrorInfo.ErrorType=FLTE_APP_ERROR_FAILED_TO_CREATE_LEDU_PROCESS;
						error_defines[FLTE_APP_ERROR_FAILED_TO_CREATE_LEDU_PROCESS].FLTE_APP_ERROR_ACTION(ErrorInfo);
					}
				}
#endif
				/****Opening the Ping fifo for read****/
				ping_fifo_fd = open(PING_FIFO, O_RDONLY);
				if (ping_fifo_fd<0)
				{
					ErrorInfo.ErrorType=FLTE_APP_ERROR_FAILED_TO_OPEN_PING_PIPE;
					error_defines[FLTE_APP_ERROR_FAILED_TO_OPEN_PING_PIPE].FLTE_APP_ERROR_ACTION(ErrorInfo);
				}
				fcntl(ping_fifo_fd,F_SETFL,0);

				if(FLAG_RUNNING==eTAPIProcessFlag&&FLAG_RUNNING==eATProcessFlag&&FLAG_RUNNING==eLEDProcessFlag)
				{
					eMonitorStatus=STATE_INIT_SUCCESS;
					//Some indication and logs. Log file should also be there
				}
				break;

			case STATE_INIT_SUCCESS:
				//Any Related indications

				eMonitorStatus=STATE_MONITOR;
				break;
			case STATE_MONITOR:
				//Main Monitor Status
				//Here this process will wait for the PING packet from the FAX amnager and the LTE application.
				//LED manager is a less prioriy app so we dont need any monitoring for that. We will add it Later.
				FD_ZERO(&rdset);
				FD_SET(ping_fifo_fd, &rdset);
				//Waiting on select function Exit will be either timeout or event//Only read event
				if (select(ping_fifo_fd+1, &rdset, NULL, NULL, &tv) < 0) {
					printf("select failed: %d : %s", errno, strerror(errno));
					exit(128);
				}
#if EN_DEBUG_MSGS
				printf("Ping Received\n");
#endif
				//Read Event Came
				if ( FD_ISSET(ping_fifo_fd, &rdset) ) {
					do {
						n = read(ping_fifo_fd, &d, 1);
						if(PING_PACKET_OK==process_packet(&d,&PingPacketData))
						{
#if EN_DEBUG_MSGS
							printf("Ping Packet Success From %d\n",PingPacketData.ProcessID);
#endif
							update_process_status(&PingPacketData);			//Update the database of the structure
							clear_ping_data(&PingPacketData);				//Clear the data back to ff
						}
					} while (n < 0);
				}

				//It Will Check All The process Status Whether it is in stuck state or not
				//Select Will exit after a certain ping timeout it will chek all the process status that time
				SleepTimeout=CheckAllProcessStatus();
				//We will check Ping packet details After making the FAX App and LTE App
				tv.tv_sec=10;
				tv.tv_usec=0;

				break;
			case STATE_STATUS_MSG:
				break;
			case STATE_ERROR:
				break;
			case STATE_KILL_PROCESS:
				break;

		}
	}
#if EN_DEBUG_MSGS
	printf("Killing the FAXManager application\n");
							//killing the application Which this application started
#endif
	return 0;
}

