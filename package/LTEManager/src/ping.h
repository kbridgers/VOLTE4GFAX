/*
 * ping.h
 *
 *  Created on: 30-Mar-2015
 *      Author: vvdnlt230
 */

#ifndef PING_H_
#define PING_H_



typedef enum
{
	PROCESS_STATE_PING_OK,					/*Last ping success*/
	PROCESS_STATE_ACTIVE,
	PROCESS_STATE_PING_FAILED,				/*Failed to get the ping packet last time*/
	PROCESS_STATE_PING_MAX_TIMES_FAILED,	/*Failed to get the ping Max number of times, Next ping means Respawn or indication*/
	PROCESS_STATE_INACTIVE,					/*If it is inactive do the Action now*/
}eProcessStatus_t;


typedef enum
{
	PING_PKT_PROCESSING,
	PING_PKT_FAILED,
	PING_PACKET_CHECKSUM_ERROR,
	PING_PACKET_OK,
	PING_PKT_IDLE,
}ePingPacketRet_t;

typedef enum
{
	PING_PKT_RECEIVED,
	PING_PKT_UPDATED,

}eProcessPingUpdate_t;
typedef struct
{
	eProcessStatus_t ProcessStatus;			//State of that Process
	eProcessPingUpdate_t Update_status;
	int ProcessTimeout;						/*After this much time from the success, no ping pkt means Failed*/
	unsigned char failurecount;				/*From last Success*/
	int Current_Process_timeout;			/*Timer count from the last ping timeout*/
}processinfo_t;


typedef enum
{
	PING_PKT_STATE_IDLE,
	PING_PKT_STATE_START,
	PING_PKT_STATE_DATA,
	PING_PKT_CHKSUM,
	PING_PKT_STOP,
}PingPktStat_t;

typedef struct
{
	unsigned char ProcessID;
	unsigned char ProcessState;			/*Process ID and the current state in the process*/
}PingPacketData_t;



#define START_PINGPKT '$'
#define STOP_PINGPKT '#'

#define MAX_PING_FAIL_COUNT 6


void update_process_status(PingPacketData_t *Pkt_data);
void clear_ping_data(PingPacketData_t *Pkt_data);
int CheckAllProcessStatus();
ePingPacketRet_t process_packet(char *rec_ch,PingPacketData_t * pingpacket);


#endif /* PING_H_ */
