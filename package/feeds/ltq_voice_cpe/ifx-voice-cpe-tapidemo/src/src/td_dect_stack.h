#ifndef _TD_DECT_STACK_H_
#define _TD_DECT_STACK_H_
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
    \file  td_dect_stack.h
    \date  2010-01-29
    \brief DECT stack handling implementation for tapidemo application.

   This file includes methods which initialize and handle DECT stack module.
*/


#ifdef LINUX
#include "tapidemo_config.h"
#endif
#include "ifx_types.h"

#ifdef TD_DECT

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
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>

/* Workaround - IFX_TRUE and IFX_FALSE are defined twice time.
   In file ifx_types.h and ifx_common_defs.h */
#define IFX_TRUE
#define IFX_FALSE
#include "ifx_common_defs.h"
#undef IFX_TRUE
#undef IFX_FALSE

#include "IFX_DECT_Stack.h"
#include "IFX_DECT_MU.h"
#include "IFX_DECT_CSU.h"


#include "common.h"

extern IFX_int32_t g_nDectDebugLevelInternal;
extern TD_ENUM_2_NAME_t TD_rgDectEventName[];

/** event message for all channels is send */
#define TD_DECT_STACK_ALL_CH           0x0F0F

#define TD_DECT_STACK_MODE             IFX_DECT_ASYNC_MODE

#define TD_DECT_PAGEKEY_DRV              "/dev/pb"
#define IFX_DECT_REG_KEY_PRESS_DUR     5000  /* in ms*/
#define IFX_DECT_REG_DURATION          60000 /* in ms*/
#ifdef TD_DECT_DEBUG_ENABLE

/** used for debug TRACES that can be removed in near future,
    they were used for development */
#define TD_DECT_DEBUG(name, level, message) \
   do \
   { \
      if (g_nDectDebugLevelInternal != DBG_LEVEL_OFF) \
      { \
         TD_TRACE(name,level, TD_CONN_ID_DECT, message); \
      } \
   } \
   while (0)
#else /* TD_DECT_DEBUG */
#define TD_DECT_DEBUG(name, level, message)
#endif /* TD_DECT_DEBUG */

IFX_return_t TD_DECT_STACK_SetCtrlPointer(TD_DECT_STACK_CTRL_t* pDectStackCtrl);
IFX_return_t TD_DECT_STACK_CliSocketOpen(CTRL_STATUS_t* pCtrl);
/* Declarations of functions that were copied from dect agent START */
IFX_return_t IFX_DECT_ProcessPagekeyMessage(
                       uint32 uiPagingKeyPressedDur //in ms
                                 );
/* Declarations of functions that were copied from dect agent STOP */
IFX_boolean_t TD_DECT_STACK_IsPagingActive(IFX_void_t);

IFX_return_t TD_DECT_STACK_PipeOpen(CTRL_STATUS_t* pCtrl);

IFX_return_t TD_DECT_STACK_SendToTD(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                    TD_DECT_EVENT_t nEvent,
                                    IFX_int32_t nHandset,
                                    TD_DECT_STACK_DATA_t* pData);

/* callback functions MU */
IFX_int32_t TD_DECT_STACK_MU_Notify(x_IFX_DECT_MU_NotifyInfo *pNotifyInfo);
IFX_int32_t TD_DECT_STACK_MU_ModemReset();

/* callback functions CSU */
e_IFX_Return TD_DECT_STACK_CallInitiate(uint32 uiCallHdl,
                                        uchar8 ucHandsetId,
                                        x_IFX_DECT_CSU_CallParams *pxCallParams,
                                        e_IFX_DECT_ErrReason *peStatus,
                                        uint32 *puiPrivateData);

e_IFX_Return TD_DECT_STACK_CallAccept(uint32 uiCallHdl,
                                      x_IFX_DECT_CSU_CallParams *pxCallParams,
                                      uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_CallAnswer(uint32 uiCallHdl,
                                      x_IFX_DECT_CSU_CallParams *pxCallParams,
                                      uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_CallRelease(uint32 uiCallHdl,
                                       x_IFX_DECT_CSU_CallParams *pxCallParams,
                                       e_IFX_DECT_RelType eReason,
                                       uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_InfoReceived(uint32 uiCallHdl,
                                        x_IFX_DECT_CSU_CallParams *pxCallParams,
                                        uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_ServiceChange(uint32 uiCallHdl,
                                         x_IFX_DECT_CSU_CallParams *pxCallParams,
                                         e_IFX_DECT_CSU_CodecChangeType eType,
                                         uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_VoiceModify(uint32 uiCallHdl,
                                       x_IFX_DECT_CSU_CallParams *pxCallParams,
                                       e_IFX_DECT_CSU_VoiceEventType eType,
                                       uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_CipherCfm(uint32 uiCallHdl,
                                     x_IFX_DECT_CSU_CallParams *pxCallParams,
                                     uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_CallToggle(uint32 uiSrcCallHdl,
                                      uint32 uiDstCallHdl,
                                      x_IFX_DECT_CSU_CallParams *pxCallParams,
                                      e_IFX_DECT_ErrReason *peStatus,
                                      uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_CallHold(uint32 uiCallHdl,
                                    x_IFX_DECT_CSU_CallParams *pxCallParams,
                                    e_IFX_DECT_ErrReason *peStatus,
                                    uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_CallResume(uint32 uiCallHdl,
                                      x_IFX_DECT_CSU_CallParams *pxCallParams,
                                      e_IFX_DECT_ErrReason *peStatus,
                                      uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_CallTransfer(uint32 uiSrcCallHdl,
                                        uint32 uiDstCallHdl,
                                        x_IFX_DECT_CSU_CallParams *pxCallParams,
                                        e_IFX_DECT_ErrReason *peStatus,
                                        uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_CallConference(uint32 uiSrcCallHdl,
                                          uint32 uiDstCallHdl,
                                          x_IFX_DECT_CSU_CallParams *pxCallParams,
                                          e_IFX_DECT_ErrReason *peStatus,
                                          uint32 uiPrivateData);
#ifndef TD_DECT_BW_COMP_BEFORE_3_1_1_3
e_IFX_Return TD_DECT_STACK_InterceptAccept(uint32 uiCallHdl,
                                          x_IFX_DECT_CSU_CallParams *pxCallParams,
                                          uint32 uiPrivateData);

e_IFX_Return TD_DECT_STACK_IntrudeInfoRecv(uint32 uiCallHdl,
                                          x_IFX_DECT_CSU_CallParams *pxCallParams,
                                          uint32 uiPrivateData);
#endif /* TD_DECT_BW_COMP_BEFORE_3_1_1_3 */
/* end of callback functions */


IFX_return_t TD_DECT_STACK_ChannelToHandsetInit(CTRL_STATUS_t* pCtrl,
                                                BOARD_t* pBoard);

IFX_return_t TD_DECT_STACK_AddHandset(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                      IFX_int32_t nHandset);
IFX_return_t TD_DECT_STACK_AddCallHdl(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                      IFX_int32_t nHandset, IFX_int32_t nCallHdl);

IFX_int32_t TD_DECT_STACK_GetChannelOfHandset(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nHandset);
IFX_int32_t TD_DECT_STACK_GetChannelOfCallHdl(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nCallHdl);

IFX_int32_t TD_DECT_STACK_GetHandsetOfCallHdl(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nCallHdl);
IFX_int32_t TD_DECT_STACK_GetHandsetOfChannel(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nCh);

IFX_int32_t TD_DECT_STACK_GetCallHdlOfHandset(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nHandsetId);
IFX_int32_t TD_DECT_STACK_GetCallHdlOfChannel(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nCh);

IFX_return_t TD_DECT_STACK_BMCRegParamGet(x_IFX_DECT_BMCRegParams *pxBmc);

IFX_return_t TD_DECT_STACK_RcFreqParamGet(IFX_uint8_t *pucFreqTx,
                                          IFX_uint8_t *pucFreqRx,
                                          IFX_uint8_t *pucFreqRange);

IFX_return_t TD_DECT_STACK_OscTrimParamGet(x_IFX_DECT_OscTrimVal *pxOscTrimVal);

IFX_return_t TD_DECT_STACK_GaussianValGet(uint16 *pucGaussianVal);

IFX_return_t TD_DECT_STACK_BS_PinGet(char8 *pcBasePin);
IFX_return_t TD_DECT_STACK_TPCParamGet(x_IFX_DECT_TransmitPowerParam *pxTPCParams);

IFX_return_t TD_DECT_STACK_SetDownloadPath(CTRL_STATUS_t* pCtrl);

IFX_void_t TD_DECT_STACK_InitDownload(IFX_int32_t iStatus);

IFX_return_t TD_DECT_STACK_RFPI_Get(char8 *pcRFPI, CTRL_STATUS_t* pCtrl);

IFX_return_t TD_DECT_STACK_DbgLevelSet(CTRL_STATUS_t* pCtrl,
                                       x_IFX_DECT_StackInitCfg* pxInitCfg);
#endif /* TD_DECT */

#endif /* _TD_DECT_STACK_H_ */

