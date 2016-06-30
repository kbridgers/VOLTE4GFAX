/*
 * faxstruct.h
 *
 *  Created on: 07-Apr-2015
 *      Author: vvdnlt230
 */

#ifndef SRC_FAXSTRUCT_H_
#define SRC_FAXSTRUCT_H_


#define FAX_FIFO "/tmp/FAX_FIFO"
#define LTE_FIFO "/tmp/LTE_FIFO"
/*FAX structures*/
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

typedef enum
{
	LTE_EVENT_INCOMING_CALL,
	LTE_EVENT_ACK,
	LTE_EVENT_NO_CARRIER,
}LTEEventType_t;
/**/
typedef enum
{
	eFAX_ACKEVENT_ON_HOOK,
	eFAX_ACKEVENT_OFF_HOOK,
	eFAX_ACKEVENT_DTMF_DETECTED,
}LTEAckEventType_t;

/*Response from the LTE application*/
typedef struct
{
	LTEEventType_t LTEEventType;
	LTEAckEventType_t ACKFlag;					//If the Message type is ACk it will tell the ack for which message
}LTEEvent_t;

#define FAX_MANAGER_ENABLED 1
#define VOICE_CALL_REJECTION_ENABLED 1
int sendEventToPipe(FAXEvent_t FaxEvent);
int close_LTE_Pipe();
int open_LTE_Pipe();
int open_FAX_Pipe();
int fax_mgr_create_pipe();

#endif /* SRC_FAXSTRUCT_H_ */
