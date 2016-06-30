#ifndef _TD_DECT_H_
#define _TD_DECT_H_
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
    \file  td_dect.h
    \date  2010-01-29
    \brief DECT module implementation for tapidemo application.

   This file includes methods which initialize and handle DECT module.
*/

#ifdef TD_DECT

#define TD_DECT_DEBUG_ENABLE                 1

#ifdef LINUX
#include "tapidemo_config.h"
#endif
#include "ifx_types.h"
#include "IFX_TLIB_Timlib.h"

#include <stdio.h>
#include <string.h>
/* According to POSIX 1003.1-2001 */
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Workaround - so IFX_TRUE and IFX_FALSE will not be defined for the second
   time in file ifx_types.h and ifx_common_defs.h. Workaround should be used
   until both ifx_types.h and ifx_common_defs.h must be used */
#define IFX_TRUE 
#define IFX_FALSE 
#include "ifx_common_defs.h"
#undef IFX_TRUE
#undef IFX_FALSE


#include "IFX_DECT_Stack.h"
#include "IFX_DECT_MU.h"
#include "IFX_DECT_CSU.h"

#include "common.h"

#define TD_DECT_PHONE_CHECK(m_pPhone, m_nReturn) \
   if (PHONE_TYPE_DECT != m_pPhone->nType) \
   { \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_pPhone->nSeqConnId, \
            ("Err, Phone No %d, not a DECT phone, unable to proceed.\n" \
             "(File: %s, line: %d)\n", \
             m_pPhone->nPhoneNumber, __FILE__, __LINE__)); \
      return m_nReturn; \
   }
/** DECT default frame length */
#define TD_DECT_FRAME_LEN_DEFAULT            IFX_TAPI_DECT_ENC_LENGTH_10
/** DECT default codec for WideBand */
#define TD_DECT_ENCODER_WIDEWBAND_DEFAULT    IFX_TAPI_DECT_ENC_TYPE_G722_64
/** DECT default codec for NarrowBand */
#define TD_DECT_ENCODER_NARROWBAND_DEFAULT   IFX_TAPI_DECT_ENC_TYPE_G726_32

IFX_return_t TD_DECT_CurrTimerExpirySelect(CTRL_STATUS_t* pCtrl);
IFX_return_t TD_DECT_ST_InRinging_Ready(PHONE_t* pPhone);

IFX_return_t TD_DECT_PCM_ChannelAdd(PHONE_t* pPhone);
IFX_return_t TD_DECT_PCM_ChannelRemove(PHONE_t* pPhone);

IFX_return_t TD_DECT_DataChannelAdd(PHONE_t* pPhone);
IFX_return_t TD_DECT_DataChannelRemove(PHONE_t* pPhone);

IFX_return_t TD_DECT_WideBandSet(PHONE_t* pPhone, CONNECTION_t* pConn,
                                 PHONE_t* pDstPhone, CONNECTION_t* pDstConn);

IFX_return_t TD_DECT_PhoneActivate(PHONE_t* pPhone, CONNECTION_t* pConn);

IFX_return_t TD_DECT_HandlePaging(IFX_int32_t viPagingKeyFd);
IFX_return_t TD_DECT_HandleCliCmd(IFX_int32_t viFromCliFd,
                                  CTRL_STATUS_t* pCtrl);
IFX_return_t TD_DECT_StartRegistration(IFX_void_t);

IFX_int32_t TD_DECT_TonePlay(PHONE_t* pPhone, IFX_int32_t nTone);
IFX_int32_t TD_DECT_ToneStop(PHONE_t* pPhone);

IFX_int32_t TD_DECT_GetFD_OfCh(IFX_int32_t nDectCh, BOARD_t* pBoard);

IFX_return_t TD_DECT_MapToAnalog(IFX_int32_t nDectCh, IFX_int32_t nAnalogCh,
                                 TD_MAP_ADD_REMOVE_t fMapping, BOARD_t* pBoard,
                                 IFX_uint32_t nSeqConnId);

IFX_return_t TD_DECT_MapToData(IFX_int32_t nDectCh, IFX_int32_t nDataCh,
                               TD_MAP_ADD_REMOVE_t fMapping, BOARD_t* pBoard,
                               IFX_uint32_t nSeqConnId);

IFX_return_t TD_DECT_MapToPCM(IFX_int32_t nDectCh, IFX_int32_t nPCM_Ch,
                              TD_MAP_ADD_REMOVE_t fMapping, BOARD_t* pBoard,
                              IFX_uint32_t nSeqConnId);

IFX_return_t TD_DECT_MapToDect(IFX_int32_t nDectCh, IFX_int32_t nDectChToAdd,
                               TD_MAP_ADD_REMOVE_t fMapping, BOARD_t* pBoard,
                               IFX_uint32_t nSeqConnId);

IFX_return_t TD_DECT_ChannelDeActivate(PHONE_t* pPhone);

IFX_return_t TD_DECT_SetPhonesStructureAndKpi(BOARD_t* pBoard);

IFX_return_t TD_DECT_HookOn(PHONE_t* pPhone);

IFX_return_t TD_DECT_Ring(PHONE_t* pPhone);

IFX_return_t TD_DECT_EventHandle(TD_OS_Pipe_t* pDataReadFp, BOARD_t* pBoard,
                                 PHONE_t** ppPhone, CONNECTION_t** ppConn);
IFX_return_t TD_DECT_DialtoneTimeoutAdd(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
IFX_return_t TD_DECT_DialtoneTimeoutRemove(CTRL_STATUS_t* pCtrl,
                                           PHONE_t* pPhone);
IFX_return_t TD_DECT_DialtoneTimeoutSelectSet(CTRL_STATUS_t* pCtrl,
                                              IFX_time_t* pTimeOut);
IFX_return_t TD_DECT_DialtoneTimeoutHandle(CTRL_STATUS_t* pCtrl,
                                           BOARD_t* pBoard,
                                           IFX_int32_t nDectCh);
IFX_return_t TD_DECT_Clean(CTRL_STATUS_t* pCtrl);

IFX_return_t TD_DECT_Init(CTRL_STATUS_t* pCtrl, BOARD_t* pBoard);

IFX_return_t TD_DECT_SetEC(PHONE_t* pPhone, IFX_boolean_t bEnable);

#endif /* TD_DECT */

#endif /* _TD_DECT_H_ */

