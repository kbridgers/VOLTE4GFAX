/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_dect_stack.c
   \date 2010-01-29
   \brief DECT stack handling implementation for tapidemo application.

   This file includes methods which initialize and handle DECT stack module.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "td_dect.h"
#include "td_dect_stack.h"

#include "IFX_DECT_USU.h"
#include "IFX_DECT_CSU.h"
#include "IFX_TLIB_Timlib.h"
#include "td_timer.h"

#include "td_dect_config.h"

#ifdef TD_DECT

/** \todo redefinitions for DECT NG should be from some included file */
#define IFX_DECTNG_CODEC_G726_32       0x02
#define IFX_DECTNG_CODEC_G722_64       0x03
#define IFX_DECTNG_CODEC_G711_A        0x04
#define IFX_DECTNG_CODEC_G711_U        0x05
#define IFX_DECTNG_CODEC_G729_32       0x06

/** number of digits table */
#define TD_MAX_DIGITS 12

/** cosic FW file name */
#define COSIC_FW  "COSICFw.BIN"

/** BMC FW file name */
#define BMC_FW  "BMCFw.BIN"

/** default path to FW files */
#define TD_DECT_FW_DOWNLOAD_PATH  "/opt/ifx/downloads/"

/** socket name to send data to Production Test Tool */
#define TD_DECT_SOCKET_TO_CLI_NAME     "/tmp/ToCli"

/** socket name to receive data from Production Test Tool */
#define TD_DECT_SOCKET_FROM_CLI_NAME   "/tmp/FromCli"

/** string representation of default PIN code */
#define TD_DECT_STACK_DEFAULT_BASE_PIN "0000"

/** default RFPI number */
#define TD_DECT_STACK_DEFAULT_RFPI     {0x00, 0x50, 0x04, 0x01, 0x00}

void IFX_TIM_TimeoutHandler(x_IFX_TimerInfo *pxTimerInfo);

/* ============================= */
/* Global Structures             */
/* ============================= */

/** names of TD_DECT_EVENT_t */
TD_ENUM_2_NAME_t TD_rgDectEventName[] =
{
   {TD_DECT_EVENT_NONE, "TD_DECT_EVENT_NONE"},
   {TD_DECT_EVENT_HOOK_OFF, "TD_DECT_EVENT_HOOK_OFF"},
   {TD_DECT_EVENT_HOOK_ON, "TD_DECT_EVENT_HOOK_ON"},
   {TD_DECT_EVENT_FLASH, "TD_DECT_EVENT_FLASH"},
   {TD_DECT_EVENT_DIGIT, "TD_DECT_EVENT_DIGIT"},
   {TD_DECT_EVENT_REGISTER, "TD_DECT_EVENT_REGISTER"},
   {TD_DECT_EVENT_WIDEBAND_ENABLE, "TD_DECT_EVENT_WIDEBAND_ENABLE"},
   {TD_DECT_EVENT_WIDEBAND_DISABLE, "TD_DECT_EVENT_WIDEBAND_DISABLE"},
   {TD_DECT_EVENT_DECT_WORKING, "TD_DECT_EVENT_DECT_WORKING"},
   {TD_DECT_EVENT_END, "TD_DECT_EVENT_END"},
   {TD_MAX_ENUM_ID, "TD_DECT_EVENT_t"}
};

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* to simplify using function global pointer is defined,
   this variable should only be used in this file */
static TD_DECT_STACK_CTRL_t* g_pDectStackCtrl = IFX_NULL;

/* internal DBG level */
IFX_int32_t g_nDectDebugLevelInternal = DBG_LEVEL_HIGH;

/* print call params of x_IFX_DECT_CSU_CallParams structure */
#define TD_TRACE_CALL_PARAMS(pxxCallParams) \
   do \
   { \
      /* debug trace */ \
      TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW, \
                    ("STACK CALLBACK: call params:\n" \
                     "\t bwideband %d\n" \
                     "\t uiIEHandler %d\n" \
                     "\t bIsPresentation %d\n" \
                     "\t bHookFlash %d\n" \
                     "\t eCallType %d\n" \
                     "\t bIsAllCallsReleased %d\n" \
                     "\t uiSignal %d\n" \
                     "\t ucLineId %d\n" \
                     "\t bSingleCallLine %d\n" \
                     "\t isInternal %d\n" \
                     "\t ucPeerHandsetId %d\n" \
                     "\t uiFlag %d\n", \
                     pxxCallParams->bwideband, \
                     pxxCallParams->uiIEHandler, \
                     pxxCallParams->bIsPresentation, \
                     pxxCallParams->bHookFlash, \
                     pxxCallParams->eCallType, \
                     pxxCallParams->bIsAllCallsReleased, \
                     pxxCallParams->uiSignal, \
                     pxxCallParams->ucLineId, \
                     pxxCallParams->bSingleCallLine, \
                     pxxCallParams->isInternal, \
                     pxxCallParams->ucPeerHandsetId, \
                     pxxCallParams->uiFlag)); \
   } \
   while (0)

/* Definitions of functions that were copied from dect agent START */
STATIC uchar8 vucCurPagingPPCount = 0;

IFX_return_t IFX_DECT_ProcessPagekeyMessage(
                uint32 uiPagingKeyPressedDur
                                           )
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: IFX_DECT_ProcessPagekeyMessage\n"));
   uchar8 bPagingOn = IFX_FALSE;
   if( uiPagingKeyPressedDur < IFX_DECT_REG_KEY_PRESS_DUR )
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
            ("User Pressed Paging Key. Initiating Paging....\n" ));
      bPagingOn = (vucCurPagingPPCount)?IFX_FALSE:IFX_TRUE ;
      if(bPagingOn == IFX_TRUE)
      {
         IFX_DECT_MU_PageAllHandsets((IFX_uint8_t*)"paging",
                                     (IFX_uint8_t*)"paging");
         vucCurPagingPPCount ++;
      }
      else
      {
         IFX_DECT_MU_PageCancel();
         vucCurPagingPPCount --;
      }
   }
   else
   {  /*Event is for registration */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
            ("User Pressed Registration Key. Allowing Registration....\n" ));

      if( 0 == vucCurPagingPPCount )
      {
         IFX_DECT_MU_RegistrationAllow(IFX_DECT_REG_DURATION,NULL);
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
               ("Could Not Start Registration Mode. Base Is Paging Handsets\n" ));
      }
   }
   return IFX_SUCCESS;
}

/**
   Check if paging handsets is active.

   /param none

   \return IFX_TRUE if paging is active, otherwise IFX_TRUE
*/
IFX_boolean_t TD_DECT_STACK_IsPagingActive(IFX_void_t)
{
   if (0 == vucCurPagingPPCount)
   {
      return IFX_FALSE;
   }
   return IFX_TRUE;
}

/**
   Save pointer program DECT stack control structure.

   /param pDectStackCtrl   - pointer DECT stack control structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_STACK_SetCtrlPointer(TD_DECT_STACK_CTRL_t* pDectStackCtrl)
{
   if (IFX_NULL == pDectStackCtrl)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, invalid input parameter.\n"
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   g_pDectStackCtrl = pDectStackCtrl;
   return IFX_SUCCESS;
}

/**
   Open sockets to communicate with Production Test Tool.

   /param pCtrl      - pointer control structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_STACK_CliSocketOpen(CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t arg1=0;
   struct sockaddr_un vFILE_name;
   IFX_int32_t visize;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: TD_DECT_STACK_CliSocketOpen\n"));
   /* open from socket */
   pCtrl->viFromCliFd = socket (PF_UNIX, SOCK_DGRAM,(int32) NULL);
   if (pCtrl->viFromCliFd < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT STACK: failed to open FROM socket. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* set socket parameters */
   vFILE_name.sun_family = AF_UNIX;
   strcpy (vFILE_name.sun_path, TD_DECT_SOCKET_FROM_CLI_NAME);
   visize = offsetof (struct sockaddr_un, sun_path) +strlen (vFILE_name.sun_path) + 1;
   /* bind from socket */
   if (bind (pCtrl->viFromCliFd, (struct sockaddr *) &vFILE_name, visize) < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT STACK: failed to bind FROM socket. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set socket as non blocking */
   arg1 = fcntl (pCtrl->viFromCliFd, F_GETFL, NULL);
   arg1 |= O_NONBLOCK;
   fcntl (pCtrl->viFromCliFd, F_SETFL, arg1);
   visize = offsetof (struct sockaddr_un, sun_path)
      +strlen(vFILE_name.sun_path) + 1;

   /* socket to be post to the CLI (Production Test Tool) */
   pCtrl->oDectStackCtrl.nToCliFd = socket (PF_UNIX, SOCK_DGRAM,(int32) NULL);
   if (pCtrl->oDectStackCtrl.nToCliFd < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT STACK: failed to open TO socket. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set socket parameters */
   pCtrl->oDectStackCtrl.oToCliFILE_name.sun_family = AF_UNIX;
   strcpy (pCtrl->oDectStackCtrl.oToCliFILE_name.sun_path,
           TD_DECT_SOCKET_TO_CLI_NAME);
   pCtrl->oDectStackCtrl.nToCliSize = offsetof(struct sockaddr_un, sun_path)
      + strlen(pCtrl->oDectStackCtrl.oToCliFILE_name.sun_path) + 1;

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: sockets to communicate with "
                  "Production Test Tool opened\n"));

   return IFX_SUCCESS;
}

/* Declarations of functions that were copied from dect agent STOP */

/**
   Open FDs for pipes to communnicate between threads.

   \param  pCtrl - pointer to status control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_PipeOpen(CTRL_STATUS_t* pCtrl)
{
   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: Open communication pipe.\n"));

   strcpy(pCtrl->oMultithreadCtrl.oPipe.oDect.rgName,
          DECT_PIPE);
   if (IFX_SUCCESS != Common_PreparePipe(&pCtrl->oMultithreadCtrl.oPipe.oDect))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, Failed to prepare pipe for DECT.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   };


   pCtrl->nDectPipeFdOut =
      pCtrl->oMultithreadCtrl.oPipe.oDect.rgFd[TD_PIPE_OUT];

   pCtrl->oDectStackCtrl.nDectPipeFdIn =
      pCtrl->oMultithreadCtrl.oPipe.oDect.rgFd[TD_PIPE_IN];
   pCtrl->oDectStackCtrl.pDectPipeFpIn =
      pCtrl->oMultithreadCtrl.oPipe.oDect.rgFp[TD_PIPE_IN];
   return IFX_SUCCESS;

}

/**
   Open FDs for pipes to communnicate between threads.

   \param  pDectStackCtrl  - pointer to DECT status control structure
   \param  nEvent          - event number
   \param  nHandset        - handset ID/number
   \param  pData           - pointer to mesage data, if IFX_NULL then no
                             additional data to send

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_SendToTD(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                    TD_DECT_EVENT_t nEvent,
                                    IFX_int32_t nHandset,
                                    TD_DECT_STACK_DATA_t* pData)
{
   TD_DECT_MSG_t oDectMsg = {0};
   IFX_int32_t nLen;

   /* check input parameters */
   TD_PTR_CHECK(pDectStackCtrl, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: TD_DECT_STACK_SendToTD "
                  "(nEvent %s(%d), nHandset %d)\n",
                  Common_Enum2Name(nEvent, TD_rgDectEventName),
                  nEvent, nHandset));
   /* check if data is set */
   if (IFX_NULL != pData)
   {
      /* copy data */
      memcpy(&oDectMsg.oData, pData, sizeof(TD_DECT_STACK_DATA_t));
   }

   /* set message data */
   oDectMsg.nDectEvent = nEvent;
   if (TD_DECT_STACK_ALL_CH == nHandset)
   {
      oDectMsg.nDectCh = TD_DECT_STACK_ALL_CH;
   }
   else
   {
      /* get channel number */
      oDectMsg.nDectCh = TD_DECT_STACK_GetChannelOfHandset(pDectStackCtrl,
                                                           nHandset);
   }
   /* check if DECT channel for handset ID was found */
   if (IFX_ERROR != oDectMsg.nDectCh)
   {
      /* send data */
      nLen = fwrite(&oDectMsg, sizeof(TD_DECT_MSG_t), 1,
                    pDectStackCtrl->pDectPipeFpIn);
      if (nLen < 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
               ("Err, failed to write to pipe (fd %d). Error on pipe: %s\n"
                "(File: %s, line: %d)\n",
                pDectStackCtrl->nDectPipeFdIn, strerror(errno),
                __FILE__, __LINE__));
      }
      else
      {
         return IFX_SUCCESS;
      }
   } /* if (IFX_ERROR != oDectMsg.nDectCh) */
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, Unable to get channel number of handset %d\n"
             "(File: %s, line: %d)\n",
             nHandset, __FILE__, __LINE__));
   } /* if (IFX_ERROR != oDectMsg.nDectCh) */
   return IFX_ERROR;
}

/**
   Handle notify events from Managment Unit - callback function.

   \param  pNotifyInfo     - pointer to notify structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_int32_t TD_DECT_STACK_MU_Notify(x_IFX_DECT_MU_NotifyInfo *pNotifyInfo)
{
   /* check input parameter */
   TD_PTR_CHECK(pNotifyInfo, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_MU_Notify\n"));

   /* check which event was received */
   switch(pNotifyInfo->eEvent)
   {
      case IFX_DECT_MU_REGISTERED:
         return TD_DECT_STACK_AddHandset(g_pDectStackCtrl,
                                         pNotifyInfo->ucHandSet);
         break;
      case IFX_DECT_MU_ATTACHED:
         if (IFX_DECTNG_CODEC_G722_64 == pNotifyInfo->ucCodec_Pri1 ||
             IFX_DECTNG_CODEC_G722_64 == pNotifyInfo-> ucCodec_Pri2 ||
             IFX_DECTNG_CODEC_G722_64 == pNotifyInfo->ucCodec_Pri3)
         {
            /* for now this message is unhandled by tapidemo */
            if (IFX_ERROR == TD_DECT_STACK_SendToTD(g_pDectStackCtrl,
                                TD_DECT_EVENT_WIDEBAND_ENABLE,
                                pNotifyInfo->ucHandSet, IFX_NULL))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                     ("Err, Failed to send\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
         }
         /* send information about new handset attached */
         return TD_DECT_STACK_SendToTD(g_pDectStackCtrl,
                                       TD_DECT_EVENT_REGISTER,
                                       pNotifyInfo->ucHandSet, IFX_NULL);
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
               ("STACK CALLBACK: MU_Notify: Unhandled event received "
                "(event %d, handset %d)\n",
                pNotifyInfo->eEvent, pNotifyInfo->ucHandSet));
         return IFX_SUCCESS;
         break;
   }
}

/**
   Modem reset callback from Managment Unit - callback function.
   End connection for all DECT phones.

   \param  none

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_int32_t TD_DECT_STACK_MU_ModemReset()
{
   IFX_int32_t nCh;
   IFX_int32_t nHandsetId;

   /* check if control structure is set */
   TD_PTR_CHECK(g_pDectStackCtrl, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_MU_ModemReset\n"));

   /* for all DECT channels(handsets) */
   for (nCh=0; nCh<g_pDectStackCtrl->nDectChNum; nCh++)
   {
      /* get handset */
      nHandsetId = TD_DECT_STACK_GetHandsetOfChannel(g_pDectStackCtrl, nCh);
      /* check if handset is set */
      if (TD_NOT_SET != nHandsetId)
      {
         /* send mesage */
         if (IFX_ERROR == TD_DECT_STACK_SendToTD(g_pDectStackCtrl,
                            TD_DECT_EVENT_HOOK_ON, nHandsetId, IFX_NULL))
         {
            return IFX_ERROR;
         }
      }
   } /* for all channels */
   return IFX_SUCCESS;
}

/**
   Call initiate from Call Service Unit - callback function.
   Adds call handle to handset ID. Sets Private data. Generates HOOK_OFF event
   message for tapidemo.

   \param  uiCallHdl    - pointer to call structure
   \param  ucHandsetId  - handset ID/number
   \param  pxCallParams - pointer to call parameters
   \param  peStatus     - pointer to reason enum data
   \param  puiPrivateData  - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CallInitiate(uint32 uiCallHdl,
                                        uchar8 ucHandsetId,
                                        x_IFX_DECT_CSU_CallParams *pxCallParams,
                                        e_IFX_DECT_ErrReason *peStatus,
                                        uint32 *puiPrivateData)
{
   TD_DECT_STACK_DATA_t oData = {{0}};
   /* ceck input parameters */
   TD_PTR_CHECK(pxCallParams, IFX_ERROR);
   TD_PTR_CHECK(peStatus, IFX_ERROR);
   TD_PTR_CHECK(puiPrivateData, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CallInitiate "
                  "(call %d, handset %d)\n",
                  uiCallHdl, ucHandsetId));
   /* print call parameters */
   TD_TRACE_CALL_PARAMS(pxCallParams);
   /* data that is set here will be recived in callback function */
   *puiPrivateData = ucHandsetId;
   /* add call handle to handset ID */
   TD_DECT_STACK_AddCallHdl(g_pDectStackCtrl, ucHandsetId, uiCallHdl);

   /* send hook off event */
   return TD_DECT_STACK_SendToTD(g_pDectStackCtrl,
                                 TD_DECT_EVENT_HOOK_OFF,
                                 ucHandsetId,
                                 &oData);
}

/**
   Call accepted from Call Service Unit - callback function.

   \param  uiCallHdl    - pointer to call structure
   \param  pxCallParams - pointer to call parameters
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CallAccept(uint32 uiCallHdl,
                                      x_IFX_DECT_CSU_CallParams *pxCallParams,
                                      uint32 uiPrivateData)
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CallAccept "
                  "(call %d, private %d)\n",
                  uiCallHdl, uiPrivateData));
   TD_TRACE_CALL_PARAMS(pxCallParams);

   return IFX_SUCCESS;
}

/**
   Call anwsered from Call Service Unit - callback function.
   Start voice transmission between phones (send off-hook event).

   \param  uiCallHdl    - pointer to call structure
   \param  pxCallParams - pointer to call parameters
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CallAnswer(uint32 uiCallHdl,
                                      x_IFX_DECT_CSU_CallParams *pxCallParams,
                                      uint32 uiPrivateData)
{
   TD_DECT_STACK_DATA_t oData = {{0}};

   TD_PTR_CHECK(pxCallParams, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CallAnswer "
                  "(call %d, private %d)\n",
                  uiCallHdl, uiPrivateData));
   TD_TRACE_CALL_PARAMS(pxCallParams);

   /* send off-hook event */
   return TD_DECT_STACK_SendToTD(g_pDectStackCtrl,
                                 TD_DECT_EVENT_HOOK_OFF,
                                 uiPrivateData,
                                 &oData);
}

/**
   Call released from Call Service Unit - callback function.
   End call between phones (send on-hook event).

   \param  uiCallHdl    - pointer to call structure
   \param  pxCallParams - pointer to call parameters
   \param  eReason      - reason of call end
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CallRelease(uint32 uiCallHdl,
                                       x_IFX_DECT_CSU_CallParams *pxCallParams,
                                       e_IFX_DECT_RelType eReason,
                                       uint32 uiPrivateData)
{
   IFX_int32_t nCh;

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CallRelease "
                  "(call %d, private %d, reason %d)\n",
                  uiCallHdl, uiPrivateData, eReason));
   TD_TRACE_CALL_PARAMS(pxCallParams);

   /* get channel */
   nCh = TD_DECT_STACK_GetChannelOfCallHdl(g_pDectStackCtrl,
                                                uiCallHdl);
   if (IFX_ERROR == nCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, Failed to get channel number for call handle %d\n"
             "(File: %s, line: %d)\n",
             uiCallHdl, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: IFX_DECT_CSU_VoiceModify(%d,  &xCallParams,"
                  " IFX_DECT_STOP_VOICE, %d)\n",
                  uiCallHdl,  nCh));
   /* stop voice - cannot be done later because call data is reseted after this
      callback */
   if (IFX_SUCCESS != IFX_DECT_CSU_VoiceModify(uiCallHdl, pxCallParams,
                                               IFX_DECT_STOP_VOICE, nCh))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, IFX_DECT_CSU_VoiceModify failed\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* send on-hook event */
   return TD_DECT_STACK_SendToTD(g_pDectStackCtrl,
                                 TD_DECT_EVENT_HOOK_ON,
                                 uiPrivateData,
                                 IFX_NULL);
}

/**
   Info received from Call Service Unit - callback function.
   Generate dialed digits events and flash-hook event.

   \param  uiCallHdl    - pointer to call structure
   \param  pxCallParams - pointer to call parameters
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_InfoReceived(uint32 uiCallHdl,
                                        x_IFX_DECT_CSU_CallParams *pxCallParams,
                                        uint32 uiPrivateData)
{
   TD_DECT_STACK_DATA_t oData = {{0}};
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_InfoReceived "
                  "(call %d, private %d)\n",
                  uiCallHdl, uiPrivateData));
   TD_TRACE_CALL_PARAMS(pxCallParams);

   /* tables to change from ascii to numbering used by TAPI  */
   IFX_char_t aChars[TD_MAX_DIGITS]={'1','2','3',
                                     '4','5','6',
                                     '7','8','9',
                                     '*','0','#'};
   IFX_int32_t aDigits[TD_MAX_DIGITS]={DIGIT_ONE, DIGIT_TWO, DIGIT_THREE,
                                       DIGIT_FOUR, DIGIT_FIVE, DIGIT_SIX,
                                       DIGIT_SEVEN, DIGIT_EIGHT, DIGIT_NINE,
                                       DIGIT_STAR, DIGIT_ZERO, DIGIT_HASH};
   /* copy digits */
   {
      /* counters */
      IFX_int32_t i, j;
      /* print dialed digits */
      for (i=0; i<IFX_DECT_MAX_DIGITS; i++)
      {
         if (((i % 5) == 0) && (i!=0))
         {
            TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW, ("\n"));
         }
         TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                       ("0x%02X ", pxCallParams->acRecvDigits[i]));
      }
      TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW, ("\n"));

      /* (IFX_DECT_MAX_DIGITS - 1) last array element must be '\0'
         copy data from call params and change them from ascii to TAPI format */
      for (i=0; i<(IFX_DECT_MAX_DIGITS - 1); i++)
      {
         if ('\0' == pxCallParams->acRecvDigits[i])
         {
            oData.acRecvDigits[i] = 0;
         }
         /* search in ascii table */
         for (j=0; j<TD_MAX_DIGITS; j++)
         {
            if (pxCallParams->acRecvDigits[i] == aChars[j])
            {
               oData.acRecvDigits[i] = aDigits[j];
               break;
            }
         }
      } /* for (i=0; i<(IFX_DECT_MAX_DIGITS - 1); i++) */
   } /* copy digits */

   /* send event */
   return TD_DECT_STACK_SendToTD(g_pDectStackCtrl,
                                 (IFX_FALSE == pxCallParams->bHookFlash)?
                                 TD_DECT_EVENT_DIGIT : TD_DECT_EVENT_FLASH,
                                 uiPrivateData,
                                 &oData);
}

/**
   Service change from Call Service Unit - callback function.

   \param  uiCallHdl    - pointer to call structure
   \param  pxCallParams - pointer to call parameters
   \param  eType        - codec change type
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_ServiceChange(uint32 uiCallHdl,
                                         x_IFX_DECT_CSU_CallParams *pxCallParams,
                                         e_IFX_DECT_CSU_CodecChangeType eType,
                                         uint32 uiPrivateData)
{
   x_IFX_DECT_CSU_CallParams xCallParams={0};
   IFX_int32_t nHwVoiceChannel;

   /* check input parameters */
   TD_PTR_CHECK(pxCallParams, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_ServiceChange "
                  "(call %d, private %d, type %d)\n",
                  uiCallHdl, uiPrivateData, eType));
   TD_TRACE_CALL_PARAMS(pxCallParams);

   /* check type */
   switch(eType)
   {
      case IFX_DECT_CSU_SC_ACCEPT:
         /* get voice channel */
         nHwVoiceChannel = TD_DECT_STACK_GetChannelOfCallHdl(g_pDectStackCtrl,
                                                             uiCallHdl);
         /* check returned voice channel */
         if (IFX_ERROR == nHwVoiceChannel)
         {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                     ("Err, Failed to get voice channel for call handle %d.\n"
                      "(File: %s, line: %d)\n",
                      uiCallHdl, __FILE__, __LINE__));
               return IFX_ERROR;
         }
         /* debug trace */
         TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                       ("STACK CALLBACK: TD_DECT_STACK_ServiceChange: "
                        "ACCEPTED\n"));

         IFX_DECT_CSU_VoiceModify(uiCallHdl, &xCallParams, IFX_DECT_START_VOICE,
                                  nHwVoiceChannel);
         break;
      case IFX_DECT_CSU_SC_REJECT:
         /* debug trace */
          TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                        ("STACK CALLBACK: TD_DECT_STACK_ServiceChange: "
                         "REJECTED\n"));
         break;
      default :
            /* debug trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("STACK CALLBACK: TD_DECT_STACK_ServiceChange unhandled type %d\n",
                eType));
         return IFX_ERROR;
         break;
   }

   return IFX_SUCCESS;
}

/**
   Voice modify from Call Service Unit - callback function.

   \param  uiCallHdl    - pointer to call structure
   \param  pxCallParams - pointer to call parameters
   \param  eType        - voice event type
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_VoiceModify(uint32 uiCallHdl,
                                       x_IFX_DECT_CSU_CallParams *pxCallParams,
                                       e_IFX_DECT_CSU_VoiceEventType eType,
                                       uint32 uiPrivateData)
{
   IFX_int32_t nHwVoiceChannel;
   /* check input parameters */
   TD_PTR_CHECK(pxCallParams, IFX_ERROR);
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_VoiceModify "
                  "(call %d, private %d, type %d)\n",
                  uiCallHdl, uiPrivateData, eType));
   TD_TRACE_CALL_PARAMS(pxCallParams);

   /* check event type */
   switch(eType)
   {
      case IFX_DECT_START_VOICE:
         TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                       ("STACK CALLBACK: TD_DECT_STACK_VoiceModify start\n"));

         nHwVoiceChannel = TD_DECT_STACK_GetChannelOfCallHdl(g_pDectStackCtrl,
                                                             uiCallHdl);
         /* check returned voice channel */
         if (IFX_ERROR == nHwVoiceChannel)
         {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                     ("Err, Failed to get voice channel for call handle %d.\n"
                      "(File: %s, line: %d)\n",
                      uiCallHdl, __FILE__, __LINE__));
               return IFX_ERROR;
         }
         IFX_DECT_CSU_VoiceModify(uiCallHdl, pxCallParams, IFX_DECT_START_VOICE,
                                  nHwVoiceChannel);

         break;
      case IFX_DECT_STOP_VOICE:
         TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                       ("STACK CALLBACK: TD_DECT_STACK_VoiceModify stop\n"));
         break;
      default :
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("STACK CALLBACK: TD_DECT_STACK_VoiceModify unhandled type %d\n",
                eType));
         return IFX_ERROR;
         break;
   }
   return IFX_SUCCESS;
}

/**
   Cipher change from Call Service Unit - callback function.

   \param  uiCallHdl    - pointer to call structure
   \param  pxCallParams - pointer to call parameters
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CipherCfm(uint32 uiCallHdl,
                                     x_IFX_DECT_CSU_CallParams *pxCallParams,
                                     uint32 uiPrivateData)
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CipherCfm "
                  "(call %d, private %d)\n",
                  uiCallHdl, uiPrivateData));
   return IFX_SUCCESS;
}

/**
   Call togle from Call Service Unit - callback function.

   \param  uiSrcCallHdl - pointer to source call structure
   \param  uiDstCallHdl - pointer to destination call structure
   \param  pxCallParams - pointer to call parameters
   \param  peStatus     - pointer to error reason
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CallToggle(uint32 uiSrcCallHdl,
                                      uint32 uiDstCallHdl,
                                      x_IFX_DECT_CSU_CallParams *pxCallParams,
                                      e_IFX_DECT_ErrReason *peStatus,
                                      uint32 uiPrivateData)
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CallToggle"
                  "(src call %d, dst call %d, private %d)\n",
                  uiSrcCallHdl, uiDstCallHdl, uiPrivateData));
   return IFX_ERROR;
}

/**
   Call hold from Call Service Unit - callback function.

   \param  uiCallHdl    - pointer to source call structure
   \param  pxCallParams - pointer to call parameters
   \param  peStatus     - pointer to error reason
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CallHold(uint32 uiCallHdl,
                                    x_IFX_DECT_CSU_CallParams *pxCallParams,
                                    e_IFX_DECT_ErrReason *peStatus,
                                    uint32 uiPrivateData)
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CallHold"
                  "(call %d, private %d)\n",
                  uiCallHdl, uiPrivateData));
   return IFX_ERROR;
}

/**
   Call resume from Call Service Unit - callback function.

   \param  uiCallHdl    - pointer to source call structure
   \param  pxCallParams - pointer to call parameters
   \param  peStatus     - pointer to error reason
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CallResume(uint32 uiCallHdl,
                                      x_IFX_DECT_CSU_CallParams *pxCallParams,
                                      e_IFX_DECT_ErrReason *peStatus,
                                      uint32 uiPrivateData)
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CallResume"
                  "(call %d, private %d)\n",
                  uiCallHdl, uiPrivateData));
   return IFX_ERROR;
}

/**
   Call transfer from Call Service Unit - callback function.

   \param  uiSrcCallHdl - pointer to source call structure
   \param  uiDstCallHdl - pointer to destination call structure
   \param  pxCallParams - pointer to call parameters
   \param  peStatus     - pointer to error reason
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CallTransfer(uint32 uiSrcCallHdl,
                                        uint32 uiDstCallHdl,
                                        x_IFX_DECT_CSU_CallParams *pxCallParams,
                                        e_IFX_DECT_ErrReason *peStatus,
                                        uint32 uiPrivateData)
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CallTransfer"
                  "(src call %d, dst call %d, private %d)\n",
                  uiSrcCallHdl, uiDstCallHdl, uiPrivateData));
   return IFX_ERROR;
}

/**
   Call transfer from Call Service Unit - callback function.

   \param  uiSrcCallHdl - pointer to source call structure
   \param  uiDstCallHdl - pointer to destination call structure
   \param  pxCallParams - pointer to call parameters
   \param  peStatus     - pointer to error reason
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_CallConference(uint32 uiSrcCallHdl,
                                          uint32 uiDstCallHdl,
                                          x_IFX_DECT_CSU_CallParams *pxCallParams,
                                          e_IFX_DECT_ErrReason *peStatus,
                                          uint32 uiPrivateData)
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_CallConference"
                  "(src call %d, dst call %d, private %d)\n",
                  uiSrcCallHdl, uiDstCallHdl, uiPrivateData));
   return IFX_ERROR;
}

#ifndef TD_DECT_BW_COMP_BEFORE_3_1_1_3
/**
   callback function.

   \param  uiCallHdl    - pointer to call structure
   \param  pxCallParams - pointer to call parameters
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_InterceptAccept(uint32 uiCallHdl,
                                          x_IFX_DECT_CSU_CallParams *pxCallParams,
                                          uint32 uiPrivateData)
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_InterceptAccept"
                  "(call %d, private %d)\n",
                  uiCallHdl, uiPrivateData));
   return IFX_SUCCESS;
}

/**
   callback function.

   \param  uiCallHdl    - pointer to call structure
   \param  pxCallParams - pointer to call parameters
   \param  uiPrivateData   - private data used by the application,
                             not used by the stack, just passed in callback
                             functions, can be pointer or number

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
e_IFX_Return TD_DECT_STACK_IntrudeInfoRecv(uint32 uiCallHdl,
                                          x_IFX_DECT_CSU_CallParams *pxCallParams,
                                          uint32 uiPrivateData)
{
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("STACK CALLBACK: TD_DECT_STACK_IntrudeInfoRecv"
                  "(call %d, private %d)\n",
                  uiCallHdl, uiPrivateData));
   return IFX_SUCCESS;
}
#endif /* TD_DECT_BW_COMP_BEFORE_3_1_1_3 */

/**
   Allocate memory for table with handset ID and call handles.

   \param  pCtrl  - pointer to status control structure
   \param  pBoard - pointer to board structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_ChannelToHandsetInit(CTRL_STATUS_t* pCtrl,
                                                BOARD_t* pBoard)
{
   IFX_int32_t nCh;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* check number of DECT channels */
   if (pBoard->nMaxDectCh > 0)
   {
      /* allocate memory */
      pCtrl->oDectStackCtrl.pChToHandset =
         TD_OS_MemCalloc(pBoard->nMaxDectCh, sizeof(TD_CHANNEL_TO_HANDSET_t));
      if (IFX_NULL == pCtrl->oDectStackCtrl.pChToHandset)
      {
         /* failed to allocate memory */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
               ("Err, %s Failed to allocate memory (%d)\n"
                "(File: %s, line: %d)\n",
                pBoard->pszBoardName, pBoard->nMaxDectCh, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* reset structure */
      for (nCh=0; nCh<pBoard->nMaxDectCh; nCh++)
      {
         pCtrl->oDectStackCtrl.pChToHandset[nCh].nCallHdl = TD_NOT_SET;
         pCtrl->oDectStackCtrl.pChToHandset[nCh].nHandsetId = TD_NOT_SET;
      }
      /* set number of DECT channels */
      pCtrl->oDectStackCtrl.nDectChNum = pBoard->nMaxDectCh;
   } /* check number of DECT channels */
   else
   {
      /* invalid number of data channels */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, %s DECT Invalid number of dect channels (%d)\n"
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, pBoard->nMaxDectCh,
             __FILE__, __LINE__));
      return IFX_ERROR;
   } /* check number of DECT channels */

   return IFX_SUCCESS;
}

/**
   Add handset ID/number.

   \param  pDectStackCtrl  - pointer to dect status control structure
   \param  nHandset        - ID/number of handset to add

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_AddHandset(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                      IFX_int32_t nHandset)
{
   IFX_int32_t nCh;

   /* check input parameters */
   TD_PTR_CHECK(pDectStackCtrl, IFX_ERROR);
   TD_PTR_CHECK(pDectStackCtrl->pChToHandset, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: TD_DECT_STACK_AddHandset\n"));
   /* check number of DECT channels */
   if (pDectStackCtrl->nDectChNum > 0)
   {
      for (nCh=0; nCh<pDectStackCtrl->nDectChNum; nCh++)
      {
         if (nHandset == pDectStackCtrl->pChToHandset[nCh].nHandsetId)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT handset already added (handest %d, channel %d)\n"
                   "(File: %s, line: %d)\n",
                   nHandset, nCh,
                   __FILE__, __LINE__));
            return IFX_ERROR;
         }
         if (TD_NOT_SET == pDectStackCtrl->pChToHandset[nCh].nHandsetId)
         {
            pDectStackCtrl->pChToHandset[nCh].nHandsetId = nHandset;
            TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                          ("DECT STACK: DECT handset %d uses DECT channel %d\n",
                           nHandset, nCh));
            return IFX_SUCCESS;
         }
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, no free channel for DECT handset %d (MAX DECT CHANNEL %d)\n"
             "(File: %s, line: %d)\n",
             nHandset, pDectStackCtrl->nDectChNum,
             __FILE__, __LINE__));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT Invalid number of dect channels (%d)\n"
             "(File: %s, line: %d)\n",
             pDectStackCtrl->nDectChNum,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_ERROR;
}

/**
   Add call handle to handset ID/number.

   \param  pDectStackCtrl  - pointer to dect status control structure
   \param  nHandset        - ID/number of handset
   \param  nCallHdl        - call handle number(pointer)

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_AddCallHdl(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                      IFX_int32_t nHandset, IFX_int32_t nCallHdl)
{
   IFX_int32_t nCh;

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: TD_DECT_STACK_AddCallHdl %d\n", nCallHdl));
   /* check input parameters */
   TD_PTR_CHECK(pDectStackCtrl, IFX_ERROR);
   TD_PTR_CHECK(pDectStackCtrl->pChToHandset, IFX_ERROR);

   /* check number of DECT channels */
   if (pDectStackCtrl->nDectChNum > 0)
   {
      for (nCh=0; nCh<pDectStackCtrl->nDectChNum; nCh++)
      {
         if (nHandset == pDectStackCtrl->pChToHandset[nCh].nHandsetId)
         {
            /* debug trace */
            TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                          ("DECT STACK: ch %d, handset %d, call %d\n",
                           nCh, nHandset, nCallHdl));
            pDectStackCtrl->pChToHandset[nCh].nCallHdl = nCallHdl;
            return IFX_SUCCESS;
         }
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, handset %d not registered to add nCallHdl %d\n"
             "(File: %s, line: %d)\n",
             nHandset, nCallHdl,
             __FILE__, __LINE__));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT Invalid number of dect channels (%d)\n"
             "(File: %s, line: %d)\n",
             pDectStackCtrl->nDectChNum,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_ERROR;
}

/**
   Get channel of according to handset ID/number.

   \param  pDectStackCtrl  - pointer to dect status control structure
   \param  nHandset        - ID/number of handset

   \return DECT channel number or IFX_ERROR if unable to get channel number
*/
IFX_int32_t TD_DECT_STACK_GetChannelOfHandset(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nHandset)
{
   IFX_int32_t nCh;

   /* check input parameters */
   TD_PTR_CHECK(pDectStackCtrl, IFX_ERROR);
   TD_PTR_CHECK(pDectStackCtrl->pChToHandset, IFX_ERROR);

   /* check number of DECT channels */
   if (pDectStackCtrl->nDectChNum > 0)
   {
      /* for all channels */
      for (nCh=0; nCh<pDectStackCtrl->nDectChNum; nCh++)
      {
         if (nHandset == pDectStackCtrl->pChToHandset[nCh].nHandsetId)
         {
            /* channel was found */
            return nCh;
         }
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT Invalid number of dect channels (%d)\n"
             "(File: %s, line: %d)\n",
             pDectStackCtrl->nDectChNum,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_ERROR;
}

/**
   Get channel of according to call handle.

   \param  pDectStackCtrl  - pointer to dect status control structure
   \param  nCallHdl        - call handle

   \return DECT channel number or IFX_ERROR if unable to get channel number
*/
IFX_int32_t TD_DECT_STACK_GetChannelOfCallHdl(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nCallHdl)
{
   IFX_int32_t nCh;

   /* check input parameters */
   TD_PTR_CHECK(pDectStackCtrl, IFX_ERROR);
   TD_PTR_CHECK(pDectStackCtrl->pChToHandset, IFX_ERROR);

   /* check number of DECT channels */
   if (pDectStackCtrl->nDectChNum > 0)
   {
      /* for all channels */
      for (nCh=0; nCh<pDectStackCtrl->nDectChNum; nCh++)
      {
         if (nCallHdl == pDectStackCtrl->pChToHandset[nCh].nCallHdl)
         {
            return nCh;
         }
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT Invalid number of dect channels (%d)\n"
             "(File: %s, line: %d)\n",
             pDectStackCtrl->nDectChNum,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_ERROR;
}

/**
   Get handset ID/number of according to call handle.

   \param  pDectStackCtrl  - pointer to dect status control structure
   \param  nCallHdl        - call handle

   \return handset ID/number or IFX_ERROR if unable to get handset
*/
IFX_int32_t TD_DECT_STACK_GetHandsetOfCallHdl(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nCallHdl)
{
   IFX_int32_t nCh;

   /* check input parameters */
   TD_PTR_CHECK(pDectStackCtrl, IFX_ERROR);
   TD_PTR_CHECK(pDectStackCtrl->pChToHandset, IFX_ERROR);

   /* check number of DECT channels */
   if (pDectStackCtrl->nDectChNum > 0)
   {
      for (nCh=0; nCh<pDectStackCtrl->nDectChNum; nCh++)
      {
         if (nCallHdl == pDectStackCtrl->pChToHandset[nCh].nCallHdl)
         {
            return pDectStackCtrl->pChToHandset[nCh].nHandsetId;
         }
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, no handset for nCallHdl %d\n"
             "(File: %s, line: %d)\n",
             nCallHdl, __FILE__, __LINE__));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT Invalid number of dect channels (%d)\n"
             "(File: %s, line: %d)\n",
             pDectStackCtrl->nDectChNum,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_ERROR;
}

/**
   Get handset ID/number of according to DECT channel number.

   \param  pDectStackCtrl  - pointer to dect status control structure
   \param  nCh             - DECT channel number

   \return handset ID/number or IFX_ERROR if unable to get handset
*/
IFX_int32_t TD_DECT_STACK_GetHandsetOfChannel(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nCh)
{
   /* check input parameters */
   TD_PTR_CHECK(pDectStackCtrl, IFX_ERROR);
   TD_PTR_CHECK(pDectStackCtrl->pChToHandset, IFX_ERROR);
   TD_PARAMETER_CHECK((0>nCh), nCh, IFX_ERROR);
   if (pDectStackCtrl->nDectChNum <= 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, Invalid number of dect channels (%d)\n"
             "(File: %s, line: %d)\n",
             pDectStackCtrl->nDectChNum,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   TD_PARAMETER_CHECK((pDectStackCtrl->nDectChNum<=nCh), nCh, IFX_ERROR);

   /* check number of DECT channels */
   return pDectStackCtrl->pChToHandset[nCh].nHandsetId;
}

/**
   Get call handle of according to handset ID/number.

   \param  pDectStackCtrl  - pointer to dect status control structure
   \param  nHandsetId      - handset ID/number

   \return call handle or IFX_ERROR if unable to get call handle
*/
IFX_int32_t TD_DECT_STACK_GetCallHdlOfHandset(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nHandsetId)
{
   IFX_int32_t nCh;

   /* check input parameters */
   TD_PTR_CHECK(pDectStackCtrl, IFX_ERROR);
   TD_PTR_CHECK(pDectStackCtrl->pChToHandset, IFX_ERROR);

   /* check number of DECT channels */
   if (pDectStackCtrl->nDectChNum > 0)
   {
      for (nCh=0; nCh<pDectStackCtrl->nDectChNum; nCh++)
      {
         if (nHandsetId == pDectStackCtrl->pChToHandset[nCh].nHandsetId)
         {
            return pDectStackCtrl->pChToHandset[nCh].nCallHdl;
         }
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, no handset for nCallHdl %d\n"
             "(File: %s, line: %d)\n",
             nHandsetId, __FILE__, __LINE__));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT Invalid number of dect channels (%d)\n"
             "(File: %s, line: %d)\n",
             pDectStackCtrl->nDectChNum,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_ERROR;
}

/**
   Get call handle of according to DECT channel number.

   \param  pDectStackCtrl  - pointer to dect status control structure
   \param  nCh             - DECT channel number

   \return call handle or IFX_ERROR if unable to get call handle
*/
IFX_int32_t TD_DECT_STACK_GetCallHdlOfChannel(TD_DECT_STACK_CTRL_t* pDectStackCtrl,
                                              IFX_int32_t nCh)
{
   /* check input parameters */
   TD_PTR_CHECK(pDectStackCtrl, IFX_ERROR);
   TD_PTR_CHECK(pDectStackCtrl->pChToHandset, IFX_ERROR);
   TD_PARAMETER_CHECK((0>nCh), nCh, IFX_ERROR);
   if (pDectStackCtrl->nDectChNum <= 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, Invalid number of dect channels (%d)\n"
             "(File: %s, line: %d)\n",
             pDectStackCtrl->nDectChNum,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   TD_PARAMETER_CHECK((pDectStackCtrl->nDectChNum<=nCh), nCh, IFX_ERROR);

   /* check number of DECT channels */
   return pDectStackCtrl->pChToHandset[nCh].nCallHdl;
}

/**
   Set and print Burst Mode Control register parameters.

   \param  pxBmc     - pointer to Burst Mode Control register parameters

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_BMCRegParamGet(x_IFX_DECT_BMCRegParams *pxBmc)
{
   /* check input parameters */
   TD_PTR_CHECK(pxBmc, IFX_ERROR);

   /* get BMC parameters from configuration file */
   if (IFX_SUCCESS != TD_DECT_CONFIG_RcBmcGet(pxBmc))
   {
      /* set default values */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("DECT STACK: Unable to get BMC values - using default\n"));
      /* RSSI Free level value */
      pxBmc->ucRSSIFreeLevel= 47;
      /* RSSI Busy level value */
      pxBmc->ucRSSIBusyLevel=178;
      /* Bearer Change limit for RSSI */
      pxBmc->ucBearerChgLim=125;
      /* Delay Register for RXDSG */
      pxBmc->ucDelayReg=0;
      /* 129;  Default Antenna Settings */
      pxBmc->ucDefaultAntenna= 153;
      /* Control REgister for DRON */
      pxBmc->ucCNTUPCtrlReg=104;
      /* Windows open sync free mode */
      pxBmc->ucWOPNSF=32;
      /* Window width sync-free mode */
      pxBmc->ucWWSF=8;
      /* End of Clock-recovery reset */
      pxBmc->ucReserved_0=255;
      /* PLL settle time */
      pxBmc->ucReserved_1=255;
      /* 0x03 disable , 0x0B - enable */
      pxBmc->ucReserved_2=0x03;
      /* RXDSG deactivate bit timer */
      pxBmc->ucReserved_3=0;
      /* General Mode control 2 register */
      pxBmc->ucSYNCMCtrlReg=4;
      /* Handover evaluation pereiod */
      pxBmc->ucHandOverEvalper=164;
   }

   /* print Burst Mode Control register parameters */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: Using BMC Params:\n"
                  "\t-xBMCPrams.ucRSSIFreeLevel =%d (RSSI Free level value)\n"
                  "\t-xBMCPrams.ucRSSIBusyLevel = %d (RSSI Busy level value)\n"
                  "\t-xBMCPrams.ucBearerChgLim = %d (Bearer Change limit for RSSI)\n"
                  "\t-xBMCPrams.ucDelayReg = %d (Delay Register for RXDSG)\n"
                  "\t-xBMCPrams.ucDefaultAntenna = %d (Default Antenna Settings)\n"
                  "\t-xBMCPrams.ucCNTUPCtrlReg = %d (Control Register for DRON)\n"
                  "\t-xBMCPrams.ucWOPNSF = %d (Windows open sync free mode)\n",
                  pxBmc->ucRSSIFreeLevel, pxBmc->ucRSSIBusyLevel,
                  pxBmc->ucBearerChgLim, pxBmc->ucDelayReg,
                  pxBmc->ucDefaultAntenna, pxBmc->ucCNTUPCtrlReg,
                  pxBmc->ucWOPNSF));

   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("\t-xBMCPrams.ucWWSF = %d (Window width sync-free mode)\n"
                  "\t-xBMCPrams.ucReserved_0 = %d (End of Clock-recovery reset)\n"
                  "\t-xBMCPrams.ucReserved_1 = %d (PLL settle time)\n"
                  "\t-xBMCPrams.ucReserved_2 = 0x%02X (0x03 disable , 0x0B - enable)\n"
                  "\t-xBMCPrams.ucReserved_3 = %d (RXDSG deactivate bit timer)\n"
                  "\t-xBMCPrams.ucSYNCMCtrlReg = %d (General Mode control 2 register)\n"
                  "\t-xBMCPrams.ucHandOverEvalper = %d (Handover evaluation period)\n", pxBmc->ucWWSF,
                  pxBmc->ucReserved_0,
                  pxBmc->ucReserved_1, pxBmc->ucReserved_2,
                  pxBmc->ucReserved_3, pxBmc->ucSYNCMCtrlReg,
                  pxBmc->ucHandOverEvalper));

   return IFX_SUCCESS;
}

/**
   Set and print frequency (Mute control Registers) parameters.

   \param pucFreqTx     pointer to frequency for TX.
   \param pucFreqRx     pointer to frequency for RX.
   \param pucFreqRange  pointer to frequency range.

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_RcFreqParamGet(IFX_uint8_t *pucFreqTx,
                                          IFX_uint8_t *pucFreqRx,
                                          IFX_uint8_t *pucFreqRange)
{
   /* check input parameters */
   TD_PTR_CHECK(pucFreqTx, IFX_ERROR);
   TD_PTR_CHECK(pucFreqRx, IFX_ERROR);
   TD_PTR_CHECK(pucFreqRange, IFX_ERROR);

   /* get frequency parameters from configuration file */
   if (IFX_SUCCESS != TD_DECT_CONFIG_RcFreqGet(pucFreqTx,
                                               pucFreqRx,
                                               pucFreqRange))
   {
      /* set default values */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("DECT STACK: Unable to get frequency parameters - "
             "using default\n"));
      /* frequency for TX */
      *pucFreqTx = 0x00;
      /* frequency for RX */
      *pucFreqRx = 0x00;
      /* frequency range */
      *pucFreqRange = 0x09;
   }

   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: Mute control Registers:"
                  " Freq TX 0x%02X, Freq RX 0x%02X, Freq range 0x%02X\n",
                  *pucFreqTx, *pucFreqRx, *pucFreqRange));

   return IFX_SUCCESS;
}

/**
   Set and print Oscillator Trimming values.

   \param  pxBmc     - pointer to Oscillator Trimming values

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_OscTrimParamGet(x_IFX_DECT_OscTrimVal *pxOscTrimVal)
{
   uint16 uiOscTrimValue;
   uchar8 ucP10Status;

   /* check input parameters */
   TD_PTR_CHECK(pxOscTrimVal, IFX_ERROR);

   /* get Oscillator Trimming values from configuration file */
   if (IFX_SUCCESS == TD_DECT_CONFIG_RcOscGet(&uiOscTrimValue, &ucP10Status))
   {
      pxOscTrimVal->ucOscTrimValHI=uiOscTrimValue>>8;
      pxOscTrimVal->ucOscTrimValLOW =uiOscTrimValue&0xFF;
      pxOscTrimVal->aucCheckSum[0]= ucP10Status;
   }
   else
   {
      /* set default values */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("DECT STACK: Unable to get Oscillator Trimming values - "
             "using default\n"));
      /* OSC Trim value - high byte*/
      pxOscTrimVal->ucOscTrimValHI = 0;
      /* OSC trim value - low byte*/
      pxOscTrimVal->ucOscTrimValLOW = 47;
      /* Checksum */
      strcpy((IFX_char_t*)pxOscTrimVal->aucCheckSum,"0");
   }
   /* print Oscillator Trimming values */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: OscTrimValHI %d, OscTrimValLOW %d, CheckSum %s\n",
                  pxOscTrimVal->ucOscTrimValHI, pxOscTrimVal->ucOscTrimValLOW,
                  pxOscTrimVal->aucCheckSum));
   return IFX_SUCCESS;
}

/**
   Set and print gaussian values.

   \param  pucGaussianVal  - pointer to variable that will be holding gaussian
                             values

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_GaussianValGet(uint16 *pucGaussianVal)
{
   TD_PTR_CHECK(pucGaussianVal, IFX_ERROR);

   /* get gaussian value from configuration file */
   if (IFX_SUCCESS != TD_DECT_CONFIG_RcGfskGet(pucGaussianVal))
   {
      /* set default values */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("DECT STACK: Unable to get gaussian values - using default\n"));
      *pucGaussianVal = 175;
   }

   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: GAUSSIAN VALUE: %d\n",
                  *pucGaussianVal));

   return IFX_SUCCESS;
}

/**
   Set and print DECT PIN number.

   \param  pcBasePin - pointer to array that will be holding PIN

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_BS_PinGet(char8 *pcBasePin)
{
   TD_PTR_CHECK(pcBasePin, IFX_ERROR);

   /* set pin number */
   strcpy(pcBasePin, TD_DECT_STACK_DEFAULT_BASE_PIN);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
         ("DECT STACK: BASE PIN: %s\n", pcBasePin));

   return IFX_SUCCESS;
}

/**
   Get Transmit Power Parameters.

   \param pxTPCParams   pointer to Transmit Power Parameters

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_TPCParamGet(x_IFX_DECT_TransmitPowerParam *pxTPCParams)
{
   /* get TCP parameter from configuration file */
   if (IFX_SUCCESS != TD_DECT_CONFIG_RcTpcGet(pxTPCParams))
   {
      /* set default values */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("DECT STACK: Unable to get Transmit Power Parameters - "
             "using default\n"));
      pxTPCParams->ucTuneDigitalRef = 71;
      pxTPCParams->ucPABiasRef = 55;
      pxTPCParams->ucPowerOffset[0] = 48;
      pxTPCParams->ucPowerOffset[1] = 60;
      pxTPCParams->ucPowerOffset[2] = 65;
      pxTPCParams->ucPowerOffset[3] = 68;
      pxTPCParams->ucPowerOffset[4] = 69;
      pxTPCParams->ucTD1 = 76;
      pxTPCParams->ucTD2 = 71;
      pxTPCParams->ucTD3 = 66;
      pxTPCParams->ucPA1 = 33;
      pxTPCParams->ucPA2 = 55;
      pxTPCParams->ucPA3 = 80;
      pxTPCParams->ucSWPowerMode = 0;
      pxTPCParams->ucTXPOW_0 = 0;
      pxTPCParams->ucTXPOW_1 = 0;
      pxTPCParams->ucTXPOW_2 = 0;
      pxTPCParams->ucTXPOW_3 = 0;
      pxTPCParams->ucTXPOW_4 = 0;
      pxTPCParams->ucTXPOW_5 = 0;
      pxTPCParams->ucDBPOW = 0;
      pxTPCParams->ucTuneDigital = 0;
      pxTPCParams->ucTxBias = 0;
   }

   return IFX_SUCCESS;
}

/**
   Set and print RFPI number. Each base should have different RFPI to avoid
   registering problems. First 4 bytes of RFPI are default,
   last byte is the same as last byte of used by tapidemo IP number
   (to avoid using the same RFPI number).

   \param  pcRFPI    - pointer to array that will be holding RFPI
   \param  pCtrlI    - pointer to tapidemo status control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_RFPI_Get(char8 *pcRFPI, CTRL_STATUS_t* pCtrl)
{
   IFX_int8_t acRFPI[IFX_DECT_MAX_RFPI_LEN] = TD_DECT_STACK_DEFAULT_RFPI;
   IFX_int8_t nTempBuf;

   TD_PTR_CHECK(pcRFPI, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->pProgramArg, IFX_ERROR);

#if 0
   /* get RFPI from configuration file */
   TD_DECT_CONFIG_RcRfpiGet(pcRFPI);
   /* using default RFPI value */
#endif /* 0 */
   /* copy default RFPI number */
   memcpy(pcRFPI, acRFPI, sizeof(IFX_int8_t) * IFX_DECT_MAX_RFPI_LEN);
   /* get lowest byte of IP address */
   nTempBuf = TD_SOCK_AddrIPv4Get(&pCtrl->pProgramArg->oMy_IP_Addr) &
      TD_IP_LOWEST_BYTE_MASK;
   /* set one RFPI byte to the same number as last IP byte,
      first 3 bits and 4 last bits must be set to 0 */
   pcRFPI[3] = (char8)nTempBuf;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
         ("DECT STACK: RFPI: 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n",
          pcRFPI[0], pcRFPI[1], pcRFPI[2], pcRFPI[3], pcRFPI[4]));

   return IFX_SUCCESS;
}

/**
   Set download path for DECT FW files.

   \param  pCtrl     - pointer to tapidemo status control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_SetDownloadPath(CTRL_STATUS_t* pCtrl)
{
   IFX_char_t psFile[MAX_PATH_LEN];

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->pProgramArg, IFX_ERROR);

   /* set download path */
   pCtrl->oDectStackCtrl.psPathToDwnldFiles =
      pCtrl->pProgramArg->sPathToDwnldFiles;

   /* Find PRAM_FW file with default new name */
   if(IFX_SUCCESS == Common_CreateFullFilename(COSIC_FW,
                        pCtrl->oDectStackCtrl.psPathToDwnldFiles, psFile))
   {
      if(IFX_SUCCESS == Common_CheckFileExists(psFile))
      {
         /* Find PRAM_FW file with default name */
         return IFX_SUCCESS;
      }
   }
   /* set download path */
   pCtrl->oDectStackCtrl.psPathToDwnldFiles = TD_DECT_FW_DOWNLOAD_PATH;

   return IFX_SUCCESS;
}

/**
   Download firmware files to the Cosic Modem.

   \param  iStatus   - initialization status

   \return none
*/
IFX_void_t TD_DECT_STACK_InitDownload(IFX_int32_t iStatus)
{
   TD_DECT_STACK_DATA_t oData = {{0}};
   IFX_int32_t i = 0;

   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: TD_DECT_STACK_InitDownload %d\n", iStatus));

   /* check if path is set */
   if (IFX_NULL == g_pDectStackCtrl)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT STACK: unable to download firmawre g_pDectStackCtrl "
             "not set. \n(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return;
   }
   /* check if download finished */
   if (IFX_FW_DOWNLOAD_SUCCESS == iStatus)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
            ("DECT STACK: DECT working status received.\n"));
      /* send DECT working event */
      TD_DECT_STACK_SendToTD(g_pDectStackCtrl,
                             TD_DECT_EVENT_DECT_WORKING,
                             TD_DECT_STACK_ALL_CH,
                             &oData);
      return;

   }
   else if (IFX_SUCCESS == iStatus)
   {
      i = system("grep 'App' /proc/driver/dect/gw-stats");
      if(i != 0)
      {
         if (IFX_NULL == g_pDectStackCtrl->psPathToDwnldFiles)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT STACK: unable to download firmawre downloads path "
                   "not set. \n(File: %s, line: %d)\n",
                  __FILE__, __LINE__));
            return;
         }
         TD_OS_SecSleep(3);
         /* sleep here was needed, without it it didn't work, maybe newer version
            will work without it */
         /* print FW filenames and used path */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT STACK: download files: %s and %s from %s.\n",
               BMC_FW, COSIC_FW, g_pDectStackCtrl->psPathToDwnldFiles));

         if (IFX_SUCCESS != IFX_DECT_USU_ModemFirmwareDownload(
                              g_pDectStackCtrl->psPathToDwnldFiles,
                              BMC_FW, COSIC_FW))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT STACK: uIFX_DECT_USU_ModemFirmwareDownload failed.\n"
                  "(File: %s, line: %d)\n",
                  __FILE__, __LINE__));
         }
      }
   }

   return;
}

/**
   Set debug level according to tapidemo debug level.

   \param  pCtrl     - pointer to tapidemo status control structure
   \param  pxInitCfg - pointer to DECT initialization structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_DECT_STACK_DbgLevelSet(CTRL_STATUS_t* pCtrl,
                                       x_IFX_DECT_StackInitCfg* pxInitCfg)
{
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pxInitCfg, IFX_ERROR);

   /* setting debug level and type */
   pxInitCfg->iDbgType = IFX_DBG_TYPE_CONSOLE;
   switch(pCtrl->pProgramArg->nDectDbgLevel)
   {
   case DBG_LEVEL_OFF:
      pxInitCfg->iDbgLvl = IFX_DBG_LVL_NONE;
      pxInitCfg->iDbgType = IFX_DBG_TYPE_NONE;
      break;
   case DBG_LEVEL_HIGH:
      pxInitCfg->iDbgLvl = IFX_DBG_LVL_ERROR;
      break;
   case DBG_LEVEL_NORMAL:
      pxInitCfg->iDbgLvl = IFX_DBG_LVL_NORMAL;
      break;
   case DBG_LEVEL_LOW:
      pxInitCfg->iDbgLvl = IFX_DBG_LVL_HIGH;
      break;
   default:
      pxInitCfg->iDbgLvl = IFX_DBG_LVL_ERROR;
      break;
   }
   /* set internal DBG level */
   /* g_nDectDebugLevelInternal = pCtrl->pProgramArg->nDectDbgLevel; */

   /* note that DECT DBG level values are different than the ones used
      by tapidemo, check ifx_debug.h for more details */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
         ("DECT STACK: debug level %d, debug type %d\n",
          pxInitCfg->iDbgLvl, pxInitCfg->iDbgType));
   return IFX_SUCCESS;
}

#endif /* TD_DECT */

