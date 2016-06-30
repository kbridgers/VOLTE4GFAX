
#ifndef _QOS_H
#define _QOS_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : qos.h
   Date        : 2005-11-29
   Description : This file enhance tapi demo with quality of service
 ****************************************************************************
   \file qos.h

   \remarks

   \note Changes: Only supported in LINUX at the moment.
                  Look drv_qos.h for further info.
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/** UDP port to use for QOS */
enum
{
   /*UDP_QOS_PORT = 5002*/
   VOICE_UDP_QOS_PORT = VOICE_UDP_PORT
};

typedef enum
{
   TD_NO_ACTION = 0,
   /** Setup port address */
   TD_SET_ADDRESS_PORT = 1,
   TD_SET_ONLY_PORT = 2
} TD_QOS_ACTION_t;

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_void_t QOS_TurnServiceOn(PROGRAM_ARG_t *pProgramArg);
IFX_void_t QOS_InitializePairStruct(CTRL_STATUS_t* pCtrl,
                                    CONNECTION_t* pConn,
                                    IFX_int32_t nDevIdx);
IFX_void_t QOS_CleanUpSession(CTRL_STATUS_t* pCtrl, PROGRAM_ARG_t *pProgramArg);
IFX_boolean_t QOS_HandleService(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn,
                                IFX_uint32_t nSeqConnId);
IFX_return_t QOS_StartSession(CONNECTION_t* pConn,
                              PHONE_t* pPhone,
                              TD_QOS_ACTION_t nAction,
                              IFX_int32_t nDevIdx);
IFX_return_t QOS_StopSession(CONNECTION_t* pConn,
                             PHONE_t* pPhone);


#endif /* _QOS_H */
