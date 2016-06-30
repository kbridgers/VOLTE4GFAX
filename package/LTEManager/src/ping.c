/*
 * ping.c
 *
 *  Created on: 30-Mar-2015
 *      Author: vvdnlt230
 */


#include "ping.h"
#include "error_defines.h"


extern const FLTE_STRUCT_ERROR_type_t error_defines[];		//Defined in Error_defines.c
static processinfo_t ProcessDB[3];
static FLTE_STRUCT_ERROR_info_t ErrorInfo;

//Update the info in the process database
void update_process_status(PingPacketData_t *Pkt_data)
{
	ProcessDB[Pkt_data->ProcessID].ProcessStatus=Pkt_data->ProcessState;	//Assigning the Current State
	ProcessDB[Pkt_data->ProcessID].Current_Process_timeout=0;				//Resetting Bcz we got one success
	ProcessDB[Pkt_data->ProcessID].failurecount=0;							//Failure count from last sucess means it is 0 now
	//ProcessDB[Pkt_data->ProcessID].ProcessTimeout;							//This is getting generaed in start-up
	ProcessDB[Pkt_data->ProcessID].Update_status=PING_PKT_RECEIVED;
}

void clear_ping_data(PingPacketData_t *Pkt_data)
{
	Pkt_data->ProcessID=0xff;						//Resetting Values
	Pkt_data->ProcessState=0xff;					//Resetting Values
}



int CheckAllProcessStatus()
{
	int next_wakeup_time=60,next_ping_time,i;				//60 is the MAx time
	for(i=0;i<3;i++)				//Check All three process staus
	{
		if(ProcessDB[i].Current_Process_timeout>=ProcessDB[i].ProcessTimeout)		//Then its the time to check the ping packet
		{
			ProcessDB[i].Current_Process_timeout=0;									//Reset
			if(ProcessDB[i].Update_status!=PING_PKT_RECEIVED)
			{
				ProcessDB[i].ProcessStatus=PROCESS_STATE_ACTIVE;
				ProcessDB[i].Update_status=PING_PKT_UPDATED;
				ProcessDB[i].failurecount=0;
			}
			else
			{
				ProcessDB[i].ProcessStatus=PROCESS_STATE_PING_FAILED;				//Last Ping Failed
				ProcessDB[i].failurecount++;
				if(ProcessDB[i].failurecount>MAX_PING_FAIL_COUNT)
				{
					ProcessDB[i].ProcessStatus=PROCESS_STATE_INACTIVE;			//Process is not reachable do something

				}
			}
		}
		else
		{
			//It is not the time to check the timeout
			next_ping_time=ProcessDB[i].ProcessTimeout-ProcessDB[i].Current_Process_timeout;
			if(next_wakeup_time>next_ping_time)
			{
				next_wakeup_time=next_ping_time;				/*Select should wakeup at next process ping timeout*/
			}
		}
	}
}





ePingPacketRet_t process_packet(char *rec_ch,PingPacketData_t * pingpacket)
{
	static PingPktStat_t PingPktStatus=PING_PKT_STATE_IDLE;
	static char checksum_pkt=0,checksum_cal=0,PktData=0;			//Checksum came with pkt and checksum calulated
	if(*rec_ch==START_PINGPKT)
	{
		PingPktStatus=PING_PKT_STATE_START;
		checksum_pkt=0;								//Clearing the checksum on start
		checksum_cal=0;
		PktData=0;
	}
	switch(PingPktStatus)
	{
		case PING_PKT_STATE_IDLE:
			break;
		case PING_PKT_STATE_START:
			PingPktStatus=PING_PKT_STATE_DATA;
			checksum_cal^=*rec_ch;
			break;
		case PING_PKT_STATE_DATA:
			PingPktStatus=PING_PKT_CHKSUM;
			checksum_cal^=*rec_ch;
			PktData=*rec_ch;
			break;
		case PING_PKT_CHKSUM:
			PingPktStatus=PING_PKT_STOP;
			//checksum_cal^=*rec_ch;
			checksum_pkt=*rec_ch;
			break;
		case PING_PKT_STOP:				//Attach The data
			PingPktStatus=PING_PKT_STATE_IDLE;
			checksum_cal^=*rec_ch;
			if(checksum_pkt==checksum_cal)
			{
				//Checksum success
				pingpacket->ProcessID=((PktData>>4)& 0xf);
				pingpacket->ProcessState=((PktData)& 0xf);
				return PING_PACKET_OK;							//Error in the Ping packet
			}
			else
			{
				/*Error print with the source process*/
				ErrorInfo.ErrorType=FLTE_APP_ERROR_DATA_CHKSUM_FAILED;
				ErrorInfo.cErrorDetails=(PktData>>4)&0xf;				//Assigning the Extra details
				error_defines[FLTE_APP_ERROR_DATA_CHKSUM_FAILED].FLTE_APP_ERROR_ACTION(ErrorInfo);
				return PING_PACKET_CHECKSUM_ERROR;
			}
			break;
	}
}




