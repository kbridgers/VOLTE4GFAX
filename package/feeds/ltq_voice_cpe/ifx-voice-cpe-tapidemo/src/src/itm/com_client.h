#ifndef _COM_CLIENT_H
#define _COM_CLIENT_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : com_client.h
   Date        : 2007-05-10
   Description : This file contains definitions of communication functions.
 ****************************************************************************
   \file com_client.h

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "control_pc.h"
#include "itm_version.h"

/* ============================= */
/* Global Defines                */
/* ============================= */

/** number of port used to receive commands from control PC */
#define COM_COMMAND_PORT    1234
/** number of port used to receive broadcast messages */
#define COM_BROADCAST_PORT    1235
/*#define COM_SENDING_PORT    9999*/
/* default port to sending messages to control PC */
#define COM_SENDING_PORT    514

#define COM_DIRECTION_OUT   IFX_TRUE
#define COM_DIRECTION_IN    IFX_FALSE

/** usual max length set for sending messages with COM_SendMessage() */
#define TD_COM_SEND_MSG_SIZE     1024

/** default debug level for ITM */
#define TD_ITM_DBG_LEVEL_DEFAULT             DBG_LEVEL_OFF

/**
   Prints a trace message without sending it through communication socket.

   \param name     - Name of the trace group
   \param level    - level of this message
   \param message  - a printf compatible formated string + opt. arguments

   \return

   \remark
      The string will be redirected via printf if level is higher or equal to
      the actual level for this trace group ( please see SetTraceLevel ).
*/
#define TRACE_LOCAL(name,level,message) \
      do \
      { \
          TD_OS_PRN_USR_DBG_NL(name, level, message); \
      } while(0)

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/* ============================= */
/* Global Variable declaration   */
/* ============================= */

/** global pointer to ITM control structure */
extern TD_ITM_CONTROL_t* g_pITM_Ctrl;

/* ============================= */
/* Global function declaration   */
/* ============================= */
#ifdef LINUX

extern TD_OS_socket_t COM_InitCommunication(IFX_uint32_t, IFX_boolean_t);
IFX_return_t COM_SendMessage(TD_OS_socket_t nSocket, IFX_char_t* msg,
                             IFX_uint32_t nSeqConnId);
extern IFX_void_t COM_CleanUpComSockets(IFX_void_t);
IFX_return_t COM_BrCastAddrSet(IFX_char_t* pIfName);
IFX_return_t COM_BrCastSocketOpen(IFX_void_t);
IFX_return_t COM_BrCastResponseSend(CTRL_STATUS_t* pCtrl,
                                    TD_OS_sockAddr_t *pAddrFrom);
extern IFX_return_t COM_ResetCommunicationSockets(CTRL_STATUS_t* pCtrl,
                                                 fd_set* pRFds);
/*extern IFX_return_t COM_SendTraceMessage(IFX_int32_t, IFX_char_t*);*/

#endif /* LINUX */

#endif /* _COM_CLIENT_H */
