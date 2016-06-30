/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_dect.c
   \date 2010-01-28
   \brief DECT module implementation for tapidemo application.

   This file includes methods which initialize and handle DECT module.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "voip.h"
#include "pcm.h"
#include "drv_tapi_kpi_io.h"
#include "common.h"
#include "td_dect.h"
#include "td_dect_stack.h"
#include "state_trans.h"
#include "td_timer.h"

#include "td_dect_cli.h"

#include "IFX_DECT_CSU.h"

#include "IFX_DECT_DIAG.h"

#ifdef EASY50712_V3
#ifdef LINUX
   #include "./itm/control_pc.h"
#endif /* LINUX */
#endif /* EASY50712_V3 */

#ifdef TD_DECT

/* ============================= */
/* Local function declaration    */
/* ============================= */

IFX_return_t TD_DECT_Cfg(PHONE_t* pPhone);
IFX_return_t TD_DECT_ChannelActivate(PHONE_t* pPhone);
IFX_return_t TD_DECT_EncSet(PHONE_t* pPhone, TD_BAND_WIDE_NARROW_t nBand);
IFX_return_t TD_DECT_GetEventData(TD_OS_Pipe_t* nFp, TD_DECT_MSG_t* pDectMsgData);
IFX_return_t TD_DECT_GetPhoneOfChannel(IFX_int32_t nDectCh, BOARD_t* pBoard,
                                       PHONE_t** ppPhone);
TD_BAND_WIDE_NARROW_t TD_DECT_WideBandCheck(PHONE_t* pPhone,
                                            CONNECTION_t* pConn);
IFX_return_t TD_DECT_ConnectionActivate(PHONE_t* pPhone,
                                        TD_BAND_WIDE_NARROW_t nBandSetting);
IFX_return_t TD_DECT_KpiCfg(PHONE_t* pPhone, IFX_int32_t nKpiCh);
IFX_return_t TD_DECT_DialtoneTimeoutInit(CTRL_STATUS_t* pCtrl);
IFX_return_t TD_DECT_DialtoneTimeoutDeInit(CTRL_STATUS_t* pCtrl);

/* ============================= */
/* Local Defines                 */
/* ============================= */

/** max. size of paging key data */
#define TD_PAGING_KEY_DATA_SIZE_MAX    32

/** timeout (mseconds) 450 ms */
#define TD_DECT_DIALTONE_TIMEOUT_MSEC 450

/** timeout (mseconds) */
#define TD_DECT_INIT_TIMEOUT_MSEC  30000

/* ============================= */
/* Global function declaration   */
/* ============================= */

/** names of codecs with bandwidth */
TD_ENUM_2_NAME_t TD_rgDectEncName[] =
{
   {IFX_TAPI_DECT_ENC_TYPE_NONE, "not set"},
   {IFX_TAPI_DECT_ENC_TYPE_G711_ALAW, "G.711 A-Law 64 kbit/s (NB)"},
   {IFX_TAPI_DECT_ENC_TYPE_G711_MLAW, "G.711 u-Law 64 kbit/s (NB)"},
   {IFX_TAPI_DECT_ENC_TYPE_G726_32, "G.726 32 kbit/s (NB)"},
   {IFX_TAPI_DECT_ENC_TYPE_G722_64, "G.722 64 kbit/s (WB)"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_DECT_ENC_TYPE_t"}
};

/* ============================= */
/* Global function definition    */
/* ============================= */

extern e_IFX_Return
       IFX_DECT_MU_RegisterCallBks(IN x_IFX_DECT_MU_CallBks *pxCCCallBks);

/**
   Waits 30 seconds, during this time timer events are handled.

   \param pCtrl  - pointer to status control structure

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CurrTimerExpirySelect(CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t ret;
   IFXOS_socFd_t nFD_Max;
   TD_OS_socFd_set_t rfds, trfds;
   TD_DECT_MSG_t oDectMsgData;

   /* Timeout for select */
   IFX_int32_t nTimeOut;
   /* Time when we start waiting */
   IFX_time_t timeStartTimeout;
   /* Elapsed time */
   IFX_time_t timeElapseTime;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);


   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: TD_DECT_STACK_HandlePaging\n"));

   /* reset and set fd set */
   TD_OS_SocFdZero(&rfds);
   /* add timer fd */
   TD_OS_SocFdSet(pCtrl->nTimerFd, &rfds);
   nFD_Max = pCtrl->nTimerFd + 1;
   /* add Timer FIFO */
   TD_OS_SocFdSet(pCtrl->nTimerMsgFd, &rfds);
   if (nFD_Max <= pCtrl->nTimerMsgFd)
   {
      nFD_Max = pCtrl->nTimerMsgFd + 1;
   }
   /* add communication pipe */
   TD_OS_SocFdSet(pCtrl->nDectPipeFdOut, &rfds);
   if (nFD_Max <= pCtrl->nDectPipeFdOut)
   {
      nFD_Max = pCtrl->nDectPipeFdOut + 1;
   }

   /* Set time when we start wait */
   timeStartTimeout = TD_OS_ElapsedTimeMSecGet(0);

   /* do until timeout or error */
   while (1)
   {
      /* Check how long we have been waiting */
      timeElapseTime = TD_OS_ElapsedTimeMSecGet(timeStartTimeout);
      /* Set how long we will wait more */
      nTimeOut = TD_DECT_INIT_TIMEOUT_MSEC - timeElapseTime;
      if (nTimeOut < 0)
       {
         /* Require time elapsed. */
         nTimeOut = 0;
      }

      /* set/restore data */
      memcpy((void *) &trfds, (void*) &rfds, sizeof(trfds));
      /* wait for event or timeout */
      ret = TD_OS_SocketSelect(nFD_Max, &trfds, IFX_NULL, IFX_NULL,
                               (IFX_time_t) nTimeOut);
      /* check if timeout occured */
      if (ret == 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
               ("DECT: TD_DECT_CurrTimerExpirySelect timeout.\n"));
         return IFX_SUCCESS;
      } /* if ret == 0 */
      else if (ret > 0)
      {
         if (TD_OS_SocFdIsSet(pCtrl->nTimerFd, &trfds))
         {
            TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                          ("DECT: calling IFX_TLIB_CurrTimerExpiry\n"));

            IFX_TLIB_CurrTimerExpiry(0);
         } /* if (FD_ISSET(pCtrl->nTimerFd, &trfds)) */

         if (TD_OS_SocFdIsSet(pCtrl->nTimerMsgFd, &trfds))
         {
            IFX_TIM_TimerMsgReceived();
         }

         if (TD_OS_SocFdIsSet(pCtrl->nDectPipeFdOut, &trfds))
         {
            /* message received on pipe */
            if (IFX_SUCCESS != TD_DECT_GetEventData(
               pCtrl->oMultithreadCtrl.oPipe.oDect.rgFp[TD_PIPE_OUT],
                                                    &oDectMsgData))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                     ("Err, Failed to read from pipe. (File: %s, line: %d)\n",
                      __FILE__, __LINE__));

            }
            else
            {
               /* here only one type of message can be handled */
               if ((TD_DECT_STACK_ALL_CH == oDectMsgData.nDectCh) &&
                   (TD_DECT_EVENT_DECT_WORKING == oDectMsgData.nDectEvent))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
                        ("DECT: Initialization ended "
                         "(DECT working event received).\n"));
                  break;
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                        ("Err, Invalid message received ch %d, event %s(%d)."
                         " (File: %s, line: %d)\n",
                         oDectMsgData.nDectCh,
                         Common_Enum2Name(oDectMsgData.nDectEvent,
                                          TD_rgDectEventName),
                         oDectMsgData.nDectEvent,
                         __FILE__, __LINE__));
               }
            } /* if (TD_DECT_GetEventData()) */
         } /* if (FD_ISSET(pCtrl->nTimerFd, &trfds)) */
      } /* if ret == 0 */
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
               ("Err, select returned %d. (File: %s, line: %d)\n",
                ret, __FILE__, __LINE__));
         return IFX_ERROR;
      } /* if ret == 0 */
   } /* while (1) */
   return IFX_SUCCESS;
}

/**
   Handle paging key events.

   \param viPagingKeyFd  - paging key FD

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_HandlePaging(IFX_int32_t viPagingKeyFd)
{
   uint32 uiPageKeyPressDur;
   char8  aucPagingInfo[TD_PAGING_KEY_DATA_SIZE_MAX];

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: TD_DECT_STACK_HandlePaging\n"));
   if( TD_OS_DeviceRead(viPagingKeyFd, aucPagingInfo,
                        TD_PAGING_KEY_DATA_SIZE_MAX) > 0 )
   {
      uiPageKeyPressDur = atoi(aucPagingInfo);
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DECT,
            ("DECT: Buffer Received From Paging Driver = %s\n", aucPagingInfo));
      TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                    ("DECT: Actual Duration Of Key Pressed = %d\n",
                     uiPageKeyPressDur));
      IFX_DECT_ProcessPagekeyMessage(uiPageKeyPressDur * 100);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, Could Not Read Paging Data Properly (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Handle socket data.

   \param viFromCliFd  - FD to receive message from
   \param pCtrl        - pointer to control stucture

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
extern IFX_return_t TD_DECT_HandleCliCmd(IFX_int32_t viFromCliFd,
                                         CTRL_STATUS_t* pCtrl)
{
   x_IFX_CLI_Cmd xCLICmd={0};
   IFX_int32_t iRead=0, nToCliFD = 0, nToCliSize = 0;
   struct sockaddr* pToCliSockAddr = IFX_NULL;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: TD_DECT_STACK_HandleCliCmd\n"));

   nToCliFD = pCtrl->oDectStackCtrl.nToCliFd;
   nToCliSize = pCtrl->oDectStackCtrl.nToCliSize;
   pToCliSockAddr = (struct sockaddr*) &pCtrl->oDectStackCtrl.oToCliFILE_name;

   /* read from socket */
   iRead = read (viFromCliFd, &xCLICmd, sizeof (x_IFX_CLI_Cmd));
   if(iRead < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT: TD_DECT_STACK_HandleCliCmd Read Error. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if valid number of bytes was read */
   if (sizeof (x_IFX_CLI_Cmd) != iRead)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, DECT: invalid number of bytes was read %d (%d). "
             "(File: %s, line: %d)\n",
             iRead, sizeof (x_IFX_CLI_Cmd), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* handle read data */
   switch (xCLICmd.Event)
   {
      case IFX_CLI_DIAG_MODE:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set Modem Mode Req Arrived\n"));
         if (IFX_SUCCESS !=
             IFX_DECT_DIAG_ModemDiagnostics(xCLICmd.xCLI.ucIsDiag))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_ModemDiagnostics() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_MODEM_RESET:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Modem Restart Req Arrived\n"));
         if (IFX_SUCCESS != IFX_DECT_DIAG_ModemRestart())
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_ModemRestart() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_SET_BMC_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set BMC Req Arrived\n"));
         if (IFX_SUCCESS != IFX_DECT_DIAG_BmcSet(&xCLICmd.xCLI.xBMC))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_BmcSet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         if (IFX_SUCCESS != IFX_DECT_DIAG_ModemRestart())
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_ModemRestart() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_SET_OSC_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set Osc Req Arrived\n"));
         if (IFX_SUCCESS !=
             IFX_DECT_DIAG_OscTrimSet(xCLICmd.xCLI.xOsc.uiOscTrimValue,
                                      xCLICmd.xCLI.xOsc.ucP10Status))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_OscTrimSet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_SET_TBR6_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set TBR6 Req Arrived\n"));
         if (IFX_SUCCESS != IFX_DECT_DIAG_TBR06Mode(xCLICmd.xCLI.ucIsTBR6))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_TBR06Mode() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_SET_RFPI_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set RFPI Req Arrived\n"));
         if (IFX_SUCCESS != IFX_DECT_DIAG_RFPISet(xCLICmd.xCLI.acRFPI))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_RFPISet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         if (IFX_SUCCESS != IFX_DECT_DIAG_ModemRestart())
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_ModemRestart() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_SET_XRAM_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set XRAM Req Arrived: address %x, 5th byte 0x%02X\n",
                xCLICmd.xCLI.xRam.uiAddr, xCLICmd.xCLI.xRam.acBuffer[5]));
         if (IFX_SUCCESS != IFX_DECT_DIAG_XRAMSet(xCLICmd.xCLI.xRam.uiAddr,
                               xCLICmd.xCLI.xRam.acBuffer,
                               xCLICmd.xCLI.xRam.ucLength_Maccess))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_XRAMSet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_SET_GFSK_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set GFSK Req Arrived\n"));
         if (IFX_SUCCESS != IFX_DECT_DIAG_GfskSet(xCLICmd.xCLI.unGfskValue))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_GfskSet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_SET_RFMODE_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set RFMODE Req Arrived\n"));
         if (IFX_SUCCESS !=
             IFX_DECT_DIAG_RfTestModeSet(xCLICmd.xCLI.xRFMode.ucRFMode,
                                         xCLICmd.xCLI.xRFMode.ucChannelNumber,
                                         xCLICmd.xCLI.xRFMode.ucSlotNumber))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_RfTestModeSet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_SET_TPC_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set TPC Req Arrived\n"));
         if (IFX_SUCCESS != IFX_DECT_DIAG_TPCSet(&xCLICmd.xCLI.xTPCParams))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_TPCSet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_SET_FREQ_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Set Country Frequency Req Arrived\n"));
         if (IFX_SUCCESS !=
             IFX_DECT_DIAG_FreqOffSet(xCLICmd.xCLI.xFreq.ucFreqTx,
                                      xCLICmd.xCLI.xFreq.ucFreqRx,
                                      xCLICmd.xCLI.xFreq.ucFreqRange))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_FreqOffSet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_GET_BMC_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Get BMC Req Arrived\n"));
         if (IFX_SUCCESS != IFX_DECT_DIAG_BmcGet(&xCLICmd.xCLI.xBMC))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_BmcGet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         xCLICmd.Event = IFX_CLI_GET_BMC_IND;
         if (0 > sendto(nToCliFD, &xCLICmd, sizeof(xCLICmd), 0,
                        pToCliSockAddr, nToCliSize))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: sendto() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_GET_XRAM_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Get XRAM Req Arrived, address %x, length %d\n",
                xCLICmd.xCLI.xRam.uiAddr, xCLICmd.xCLI.xRam.ucLength_Maccess));
         if (IFX_SUCCESS != IFX_DECT_DIAG_XRAMGet(xCLICmd.xCLI.xRam.uiAddr,
                               xCLICmd.xCLI.xRam.acBuffer,
                               xCLICmd.xCLI.xRam.ucLength_Maccess))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_XRAMGet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         xCLICmd.Event = IFX_CLI_GET_XRAM_IND;
         if (0 > sendto(nToCliFD, &xCLICmd, sizeof(xCLICmd), 0,
                        pToCliSockAddr, nToCliSize))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: sendto() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      case IFX_CLI_GET_TPC_REQ:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT:DIAG: Get TPC Req Arrived\n"));
         if (IFX_SUCCESS != IFX_DECT_DIAG_TPCGet(&xCLICmd.xCLI.xTPCParams))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: IFX_DECT_DIAG_TPCGet() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         xCLICmd.Event = IFX_CLI_GET_TPC_IND;
         if (0 > sendto(nToCliFD, &xCLICmd, sizeof(xCLICmd), 0,
                        pToCliSockAddr, nToCliSize))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, DECT: sendto() failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
               ("Err, DECT: Unknown message parameter xCLICmd.Event = %d "
                "(File: %s, line: %d)\n",
                xCLICmd.Event, __FILE__, __LINE__));
         return IFX_ERROR;
         break;
   };
   return IFX_SUCCESS;
}

/**
   Start DECT handset registration.

   /param none

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_StartRegistration(IFX_void_t)
{
   /* check if paging in progress */
   if (IFX_FALSE == TD_DECT_STACK_IsPagingActive())
   {
      /* start registration process */
      if (IFX_SUCCESS == IFX_DECT_MU_RegistrationAllow(IFX_DECT_REG_DURATION,
                                                       NULL))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
               ("DECT: Start registration of DECT handset.\n"));
         return IFX_SUCCESS;
      }
      /* registration failed */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, IFX_DECT_MU_RegistrationAllow failed (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   } /* if (IFX_FALSE == TD_DECT_STACK_IsPagingActive()) */

   /* registration cannot be started during paging */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DECT,
         ("DECT: Unable to start registration, paging in progress.\n"));

   return IFX_SUCCESS;
}

/**
   Action for DECT phone when detected IE_HOOKON event for S_IN_RINGING state.
   Changes phone stops phone ringing, releas call.

   \param pPhone     - pointer to phone that changes state

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_ST_InRinging_Ready(PHONE_t* pPhone)
{
   IFX_int32_t nRet;
   IFX_int32_t nCallHdl;
   x_IFX_DECT_CSU_CallParams xCallParams = {0};

   /* check input parameter */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   nCallHdl = TD_DECT_STACK_GetCallHdlOfChannel(&pPhone->pBoard->pCtrl->oDectStackCtrl,
                                                pPhone->nDectCh);
   if (nCallHdl < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: failed to get call handle for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: IFX_DECT_CSU_CallRelease"
                  "(%d, &xCallParams, IFX_DECT_RELEASE_NORMAL)\n",
                  nCallHdl));
   /* release call */
   nRet = IFX_DECT_CSU_CallRelease(nCallHdl, &xCallParams,
                                   IFX_DECT_RELEASE_NORMAL);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d, IFX_DECT_CSU_CallRelease failed\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
   /* remove data channel if added */
   if (TD_NOT_SET != pPhone->nDataCh)
   {
      if (IFX_SUCCESS != TD_DECT_DataChannelRemove(pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d, TD_DECT_DataChannelRemove() failed\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
      }
   }
   return nRet;
}

/**
   Add PCM channel to DECT phone.

   \param pPhone     - pointer to phone

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_PCM_ChannelAdd(PHONE_t* pPhone)
{
   IFX_int32_t nPCM_Ch = TD_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   /* check if PCM channel isn't already set */
   if (TD_NOT_SET != pPhone->nPCM_Ch)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: TD_DECT_PCM_ChannelAdd - PCM channel already set %d\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->nPCM_Ch, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* get first free PCM channel */
   nPCM_Ch = PCM_GetFreeCh(pPhone->pBoard);
   if (NO_FREE_PCM_CH == nPCM_Ch)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, %s: unable to get free PCM channel\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName,  __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* reserve PCM channel */
   if (IFX_SUCCESS == PCM_ReserveCh(nPCM_Ch, pPhone->pBoard, pPhone->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Add PCM channel %d.\n",
             pPhone->nPhoneNumber, nPCM_Ch));
      pPhone->nPCM_Ch = nPCM_Ch;
      /* map PCM channel */
      return TD_DECT_MapToPCM(pPhone->nDectCh, pPhone->nPCM_Ch,
                TD_MAP_ADD, pPhone->pBoard, pPhone->nSeqConnId);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: PCM_ReserveCh(%d) - failed\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, nPCM_Ch, __FILE__, __LINE__));
      return IFX_ERROR;
   }
}

/**
   Remove PCM channel from DECT phone.

   \param pPhone     - pointer to phone

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_PCM_ChannelRemove(PHONE_t* pPhone)
{
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Remove PCM channel %d.\n",
          pPhone->nPhoneNumber, pPhone->nPCM_Ch));

   /* check if PCM channel is set */
   if (TD_NOT_SET == pPhone->nPCM_Ch)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: PCM channel is not set\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* unmap PCM channel */
   if (IFX_SUCCESS != TD_DECT_MapToPCM(pPhone->nDectCh, pPhone->nPCM_Ch,
                         TD_MAP_REMOVE, pPhone->pBoard, pPhone->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: TD_DECT_MapToPCM() failed\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
   /* free reserved channel */
   if (IFX_SUCCESS == PCM_FreeCh(pPhone->nPCM_Ch, pPhone->pBoard))
   {
      pPhone->nPCM_Ch = TD_NOT_SET;
      return IFX_SUCCESS;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, %s: unable to free pcm channel %d\n"
          "(File: %s, line: %d)\n",
          pPhone->pBoard->pszBoardName,  pPhone->nPCM_Ch,
          __FILE__, __LINE__));
   return IFX_ERROR;
}

/**
   Add data channel to DECT phone.

   \param pPhone     - pointer to phone

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_DataChannelAdd(PHONE_t* pPhone)
{
   IFX_int32_t nDataCh = TD_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   if (TD_NOT_SET != pPhone->nDataCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: TD_DECT_DataChannelAdd - data channel already set %d\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->nDataCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* get free data channel */
   nDataCh = VOIP_GetFreeDataCh(pPhone->pBoard, pPhone->nSeqConnId);
   if (NO_FREE_DATA_CH == nDataCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, %s: unable to get free data channel\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName,  __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* reserve data channel */
   if (IFX_SUCCESS == VOIP_ReserveDataCh(nDataCh, pPhone->pBoard,
                         pPhone->nSeqConnId))
   {
      /* get FD */
      pPhone->nDataCh_FD = VOIP_GetFD_OfCh(nDataCh, pPhone->pBoard);
      /* get socket if QoS is disabled */
      if (!pPhone->pBoard->pCtrl->pProgramArg->oArgFlags.nQos)
      {
         pPhone->nSocket = VOIP_GetSocket(nDataCh, pPhone->pBoard, IFX_FALSE);
      }
      /* check if data channel FD was set */
      if (NO_DEVICE_FD == pPhone->nDataCh_FD)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d: VOIP_GetFD_OfCh(%d) - failed\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, nDataCh, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: Add data channel %d.\n",
                pPhone->nPhoneNumber, nDataCh));
         pPhone->nDataCh = nDataCh;
         return TD_DECT_MapToData(pPhone->nDectCh, pPhone->nDataCh,
                   TD_MAP_ADD, pPhone->pBoard, pPhone->nSeqConnId);
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: VOIP_ReserveDataCh(%d) - failed\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, nDataCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
}

/**
   Remove data channel from DECT phone.

   \param pPhone     - pointer to phone

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_DataChannelRemove(PHONE_t* pPhone)
{
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Remove data channel %d.\n",
          pPhone->nPhoneNumber, pPhone->nDataCh));

   if (TD_NOT_SET == pPhone->nDataCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: data channel is not set\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* map channels */
   if (IFX_SUCCESS != TD_DECT_MapToData(pPhone->nDectCh, pPhone->nDataCh,
                         TD_MAP_REMOVE, pPhone->pBoard, pPhone->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: TD_DECT_MapToData() failed\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
   /* free data channel */
   if (IFX_SUCCESS == VOIP_FreeDataCh(pPhone->nDataCh, pPhone->pBoard,
                                      pPhone->nSeqConnId))
   {
      /* reset structure */
      pPhone->nDataCh = TD_NOT_SET;
      pPhone->nDataCh_FD = TD_NOT_SET;
      if (!pPhone->pBoard->pCtrl->pProgramArg->oArgFlags.nQos)
      {
         pPhone->nSocket = TD_NOT_SET;
      }
      return IFX_SUCCESS;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, %s: unable to free data channel %d\n"
          "(File: %s, line: %d)\n",
          pPhone->pBoard->pszBoardName,  pPhone->nDataCh, __FILE__, __LINE__));
   return IFX_ERROR;
}

 /**
   Start playing tone on DECT channel.

   \param pPhone     - pointer to PHONE.
   \param nTone      - tone to play.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_int32_t TD_DECT_TonePlay(PHONE_t* pPhone, IFX_int32_t nTone)
{
   IFX_return_t nRet = IFX_ERROR;

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   if (IFX_TRUE != pPhone->bDectChActivated)
   {
      /* debug trace */
      TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                    ("DECT: TD_DECT_TonePlay - DECT not activated\n"));
      return IFX_SUCCESS;
   }

   nRet = TD_DECT_ToneStop(pPhone);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d, TD_DECT_ToneStop failed\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: PhoneNo %d: TD_DECT_TonePlay %d\n",
                  pPhone->nPhoneNumber, nTone));
   /* play tone */
   nRet = TD_IOCTL(TD_DECT_GetFD_OfCh(pPhone->nDectCh, pPhone->pBoard),
             IFX_TAPI_TONE_DECT_PLAY, nTone,
             TD_DEV_NOT_SET, pPhone->nSeqConnId);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, pPhone->nDectCh));
   }

   return nRet;
}

 /**
   Stop playing tone on DECT channel.

   \param pPhone     - pointer to PHONE.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_int32_t TD_DECT_ToneStop(PHONE_t* pPhone)
{
   IFX_return_t nRet = IFX_ERROR;

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   if (IFX_TRUE != pPhone->bDectChActivated)
   {
      /* debug trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("DECT: Tone Stop - DECT not activated\n"));
      return IFX_SUCCESS;
   }
   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: Phone No %d: Tone Stop\n",
                  pPhone->nPhoneNumber));
   /* stop tone */
   nRet = TD_IOCTL(TD_DECT_GetFD_OfCh(pPhone->nDectCh, pPhone->pBoard),
             IFX_TAPI_TONE_DECT_STOP, 0,
             TD_DEV_NOT_SET, pPhone->nSeqConnId);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, pPhone->nDectCh));
   }

   return nRet;
}

/**
   Get file descriptor of device for DECT channel.

   \param nDectCh    - DECT channel number
   \param pBoard     - pointer to board where channel is placed

   \return device connected to this channel or NO_DEVICE_FD if none or error
*/
IFX_int32_t TD_DECT_GetFD_OfCh(IFX_int32_t nDectCh, BOARD_t* pBoard)
{
   /* check input parameters */
   TD_PTR_CHECK(pBoard, NO_DEVICE_FD);
   TD_PARAMETER_CHECK((0 >= pBoard->nMaxDectCh), pBoard->nMaxDectCh,
                      NO_DEVICE_FD);
   TD_PARAMETER_CHECK((0 > nDectCh), nDectCh, NO_DEVICE_FD);
   TD_PARAMETER_CHECK((nDectCh >= pBoard->nMaxDectCh), nDectCh, NO_DEVICE_FD);

   return Common_GetFD_OfCh(pBoard, nDectCh);
}

/**
   Map phone channel to DECT chanel.

   \param nDectCh    - target DECT channel
   \param nAnalogCh  - which phone channel to add
   \param fMapping   - flag if mapping should be done (TD_MAP_ADD),
                       or unmapping (TD_MAP_REMOVE)
   \param pBoard     - pointer to board
   \param nSeqConnId - Seq Conn ID

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_MapToAnalog(IFX_int32_t nDectCh, IFX_int32_t nAnalogCh,
                                 TD_MAP_ADD_REMOVE_t fMapping, BOARD_t* pBoard,
                                 IFX_uint32_t nSeqConnId)
{
   IFX_return_t nRet = IFX_ERROR;
   IFX_TAPI_MAP_DECT_t oMapDECT = {0};
   IFX_int32_t nDECT_FD;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* get FD */
   nDECT_FD = TD_DECT_GetFD_OfCh(nDectCh, pBoard);
   if (NO_DEVICE_FD == nDECT_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, %s failed to get fd for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (TD_MAP_ADD == fMapping)
   {
      /* debug trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("DECT: MAP dect ch %d and analog ch %d\n", nDectCh, nAnalogCh));
   }
   else if (TD_MAP_REMOVE == fMapping)
   {
      /* debug trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("DECT: UNMAP dect ch %d and analog ch %d\n", nDectCh, nAnalogCh));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Invalid input parameter fMapping(%d)\n"
             "(File: %s, line: %d)\n",
             fMapping, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   oMapDECT.nDstCh = nAnalogCh;
   oMapDECT.nChType = IFX_TAPI_MAP_TYPE_PHONE;

   if (TD_MAP_ADD == fMapping)
   {
      nRet = TD_IOCTL(nDECT_FD, IFX_TAPI_MAP_DECT_ADD, &oMapDECT,
                TD_DEV_NOT_SET, nSeqConnId);
   }
   else if (TD_MAP_REMOVE == fMapping)
   {
      nRet = TD_IOCTL(nDECT_FD, IFX_TAPI_MAP_DECT_REMOVE, &oMapDECT,
                TD_DEV_NOT_SET, nSeqConnId);
   }
   return nRet;
}

/**
   Map data channel to DECT chanel.

   \param nDectCh    - target DECT channel
   \param nDataCh    - which data channel to add
   \param fMapping   - flag if mapping should be done (TD_MAP_ADD),
                       or unmapping (TD_MAP_REMOVE)
   \param pBoard     - pointer to board
   \param nSeqConnId - Seq Conn ID

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_MapToData(IFX_int32_t nDectCh, IFX_int32_t nDataCh,
                               TD_MAP_ADD_REMOVE_t fMapping, BOARD_t* pBoard,
                               IFX_uint32_t nSeqConnId)
{
   IFX_return_t nRet = IFX_ERROR;
   IFX_TAPI_MAP_DATA_t oMapData = {0};
   IFX_int32_t nDataFD;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* get FD */
   nDataFD = VOIP_GetFD_OfCh(nDataCh, pBoard);
   if (NO_DEVICE_FD == nDataFD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, %s failed to get fd for data channel %d\n"
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, nDataCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (TD_MAP_ADD == fMapping)
   {
      /* debug trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("DECT: MAP dect ch %d and data ch %d\n", nDectCh, nDataCh));
   }
   else if (TD_MAP_REMOVE == fMapping)
   {
      /* debug trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("DECT: UNMAP dect ch %d and data ch %d\n", nDectCh, nDataCh));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Invalid input parameter fMapping(%d)\n"
             "(File: %s, line: %d)\n",
             fMapping, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   oMapData.nDstCh = nDectCh;
   oMapData.nChType = IFX_TAPI_MAP_TYPE_DECT;

   if (TD_MAP_ADD == fMapping)
   {
      nRet = TD_IOCTL(nDataFD, IFX_TAPI_MAP_DATA_ADD, &oMapData,
                TD_DEV_NOT_SET, nSeqConnId);
   }
   else if (TD_MAP_REMOVE == fMapping)
   {
      nRet = TD_IOCTL(nDataFD, IFX_TAPI_MAP_DATA_REMOVE, &oMapData,
                TD_DEV_NOT_SET, nSeqConnId);
   }
   return nRet;
}

/**
   Map PCM channel to DECT chanel.

   \param nDectCh    - target DECT channel
   \param nPCM_Ch    - which PCM channel to add
   \param fMapping   - flag if mapping should be done (TD_MAP_ADD),
                       or unmapping (TD_MAP_REMOVE)
   \param pBoard     - pointer to board
   \param nSeqConnId - Seq Conn ID

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_MapToPCM(IFX_int32_t nDectCh, IFX_int32_t nPCM_Ch,
                              TD_MAP_ADD_REMOVE_t fMapping, BOARD_t* pBoard,
                              IFX_uint32_t nSeqConnId)
{
   IFX_return_t nRet = IFX_ERROR;
   IFX_TAPI_MAP_PCM_t oMapPCM = {0};
   IFX_int32_t nPCM_FD;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* get FD */
   nPCM_FD = PCM_GetFD_OfCh(nPCM_Ch, pBoard);
   if (NO_DEVICE_FD == nPCM_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, %s failed to get fd for PCM channel %d\n"
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, nPCM_Ch, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (TD_MAP_ADD == fMapping)
   {
      /* debug trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("DECT: MAP dect ch %d and PCM ch %d\n", nDectCh, nPCM_Ch));
   }
   else if (TD_MAP_REMOVE == fMapping)
   {
      /* debug trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("DECT: UNMAP dect ch %d and PCM ch %d\n", nDectCh, nPCM_Ch));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Invalid input parameter fMapping(%d)\n"
             "(File: %s, line: %d)\n",
             fMapping, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   oMapPCM.nDstCh = nDectCh;
   oMapPCM.nChType = IFX_TAPI_MAP_TYPE_DECT;

   if (TD_MAP_ADD == fMapping)
   {
      nRet = TD_IOCTL(nPCM_FD, IFX_TAPI_MAP_PCM_ADD, &oMapPCM,
                TD_DEV_NOT_SET, nSeqConnId);
   }
   else
   {
      nRet = TD_IOCTL(nPCM_FD, IFX_TAPI_MAP_PCM_REMOVE, &oMapPCM,
                TD_DEV_NOT_SET, nSeqConnId);
   }
   return nRet;
}

/**
   Map DECT channel to DECT chanel.

   \param nDectCh    - target DECT channel
   \param nDectChToAdd  - which PCM channel to add
   \param fMapping   - flag if mapping should be done (TD_MAP_ADD),
                       or unmapping (TD_MAP_REMOVE)
   \param pBoard     - pointer to board
   \param nSeqConnId - Seq Conn ID

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_MapToDect(IFX_int32_t nDectCh, IFX_int32_t nDectChToAdd,
                               TD_MAP_ADD_REMOVE_t fMapping, BOARD_t* pBoard,
                               IFX_uint32_t nSeqConnId)
{
   IFX_return_t nRet = IFX_ERROR;
   IFX_TAPI_MAP_DECT_t oMapDECT = {0};
   IFX_int32_t nDECT_FD;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* get FD */
   nDECT_FD = TD_DECT_GetFD_OfCh(nDectCh, pBoard);
   if (NO_DEVICE_FD == nDECT_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, %s failed to get fd for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (TD_MAP_ADD == fMapping)
   {
      /* debug trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("DECT: MAP dect ch %d and dect ch %d\n", nDectCh, nDectChToAdd));
   }
   else if (TD_MAP_REMOVE == fMapping)
   {
      /* debug trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("DECT: UNMAP dect ch %d and dect ch %d\n", nDectCh, nDectChToAdd));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Invalid input parameter fMapping(%d)\n"
             "(File: %s, line: %d)\n",
             fMapping, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   oMapDECT.nDstCh = nDectChToAdd;
   oMapDECT.nChType = IFX_TAPI_MAP_TYPE_DECT;

   if (TD_MAP_ADD == fMapping)
   {
      nRet = TD_IOCTL(nDECT_FD, IFX_TAPI_MAP_DECT_ADD, &oMapDECT,
                TD_DEV_NOT_SET, nSeqConnId);
   }
   else
   {
      nRet = TD_IOCTL(nDECT_FD, IFX_TAPI_MAP_DECT_REMOVE, &oMapDECT,
                TD_DEV_NOT_SET, nSeqConnId);
   }
   return nRet;
}

 /**
   Configure dect channel - set decoder <-> encoder start delay and
   delay of decoder start after first packet arrival - both set to 0.

   \param pPhone     - pointer to PHONE.
   \param nTone      - tone to play.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
   \note currently not used
*/
IFX_return_t TD_DECT_Cfg(PHONE_t* pPhone)
{
   IFX_return_t nRet = IFX_ERROR;
   IFX_TAPI_DECT_CFG_t oDectCfg = {0};
   IFX_int32_t nDECT_FD;

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   /* debug trace */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("DECT: TD_DECT_Cfg\n"));

   /* Get FD */
   nDECT_FD = TD_DECT_GetFD_OfCh(pPhone->nDectCh, pPhone->pBoard);
   if (NO_DEVICE_FD == nDECT_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, %s failed to get fd for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName, pPhone->nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* configure dect channel*/
   nRet = TD_IOCTL(nDECT_FD, IFX_TAPI_DECT_CFG_SET, &oDectCfg,
             TD_DEV_NOT_SET, pPhone->nSeqConnId);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, pPhone->nDectCh));
   }

    return nRet;
}

 /**
   Activate DECT channel.

   \param pPhone     - pointer to PHONE.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_ChannelActivate(PHONE_t* pPhone)
{
   IFX_return_t nRet = IFX_ERROR;
   IFX_int32_t nDECT_FD;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   if (IFX_TRUE == pPhone->bDectChActivated)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: already acivated\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
   /* debug trace */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("DECT: Phone No %d: activate channel %d.\n",
          pPhone->nPhoneNumber, pPhone->nDectCh));
   /* get FD */
   nDECT_FD = TD_DECT_GetFD_OfCh(pPhone->nDectCh, pPhone->pBoard);
   if (NO_DEVICE_FD == nDECT_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, %s failed to get fd for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName, pPhone->nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* activate DECT */
   nRet = TD_IOCTL(nDECT_FD, IFX_TAPI_DECT_ACTIVATION_SET, IFX_ENABLE,
             TD_DEV_NOT_SET, pPhone->nSeqConnId);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, pPhone->nDectCh));
      return IFX_ERROR;
   }

   if (IFX_TRUE == pPhone->bDectChActivated)
   {
      /* if DECT already was activated then do not report error,
         channel propably was already activated */
      return IFX_SUCCESS;
   }
   /* DECT is activated - set flag */
   pPhone->bDectChActivated = IFX_TRUE;

   return nRet;
}

 /**
   Deactivate DECT channel.

   \param pPhone     - pointer to PHONE.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_ChannelDeActivate(PHONE_t* pPhone)
{
   IFX_return_t nRet = IFX_ERROR;
   IFX_int32_t nDECT_FD;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   if (IFX_FALSE == pPhone->bDectChActivated)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: already deacivated\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }

   /* debug trace */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("DECT: Phone  No %d: deactivate channel %d\n",
          pPhone->nPhoneNumber, pPhone->nDectCh));
   /* get FD */
   nDECT_FD = TD_DECT_GetFD_OfCh(pPhone->nDectCh, pPhone->pBoard);
   if (NO_DEVICE_FD == nDECT_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, %s failed to get fd for DECT channel %d.\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName, pPhone->nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* deactivate DECT */
   nRet = TD_IOCTL(nDECT_FD, IFX_TAPI_DECT_ACTIVATION_SET, IFX_DISABLE,
             TD_DEV_NOT_SET, pPhone->nSeqConnId);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, pPhone->nDectCh));
      return IFX_ERROR;
   }
   if (IFX_TRUE != pPhone->bDectChActivated)
   {
      /* if DECT already was deactivated then do not report error,
         channel propably was already deactivated */
      return IFX_SUCCESS;
   }
   /* dect is deactivated - reset flag */
   pPhone->bDectChActivated = IFX_FALSE;

   return nRet;
}

 /**
   Set DECT encoder.

   \param pPhone     - pointer to PHONE.
   \param nBandSetting  - band setting.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_EncSet(PHONE_t* pPhone, TD_BAND_WIDE_NARROW_t nBand)
{
   IFX_return_t nRet = IFX_ERROR;
   IFX_int32_t nDECT_FD;
   IFX_TAPI_DECT_ENC_CFG_SET_t oDectEncCfg = {0};

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: TD_DECT_EncSet\n"));
   /* get FD */
   nDECT_FD = TD_DECT_GetFD_OfCh(pPhone->nDectCh, pPhone->pBoard);
   if (NO_DEVICE_FD == nDECT_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, %s failed to get fd for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName, pPhone->nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* This is the default lenght being used */
   oDectEncCfg.nFrameLen = TD_DECT_FRAME_LEN_DEFAULT;

   switch(nBand)
   {
   case TD_BAND_WIDE:
      oDectEncCfg.nEncType = TD_DECT_ENCODER_WIDEWBAND_DEFAULT;
      break;
   case TD_BAND_NARROW:
      oDectEncCfg.nEncType = TD_DECT_ENCODER_NARROWBAND_DEFAULT;
      break;
   default:
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: parameter nBand (%d) invalid value\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, nBand, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   nRet = TD_IOCTL(nDECT_FD, IFX_TAPI_DECT_ENC_CFG_SET, &oDectEncCfg,
             TD_DEV_NOT_SET, pPhone->nSeqConnId);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, pPhone->nDectCh));
      return IFX_ERROR;
   }
   /* if successfull print used codec type */
   if (IFX_SUCCESS == nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: DECT ch %d set encoder %s\n",
             pPhone->nPhoneNumber, pPhone->nDectCh,
             Common_Enum2Name(oDectEncCfg.nEncType, TD_rgDectEncName)));
   }

   return nRet;
}

/**
   Set phones structure and configure KPI for DECT channels.

   \param pBoard     - pointer to BOARD.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_SetPhonesStructureAndKpi(BOARD_t* pBoard)
{
   IFX_int32_t nPhoneCnt = 0, nChNum, nKpiCh;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pBoard->rgoPhones, IFX_ERROR);

   /* for all phone channels */
   for (nChNum=0; nChNum < pBoard->nMaxAnalogCh; nChNum++, nPhoneCnt++)
   {
      pBoard->rgoPhones[nPhoneCnt].nDectCh = TD_NOT_SET;
      pBoard->rgoPhones[nPhoneCnt].nType = PHONE_TYPE_ANALOG;
      pBoard->rgoPhones[nPhoneCnt].bDectChActivated = IFX_FALSE;
   } /* for all dect channels */
   /* check if DECT channels are available */
   if (0 >= pBoard->nMaxDectCh)
   {
      return IFX_SUCCESS;
   }
   /* for all dect channels */
   for (nChNum=0, nKpiCh = 0;
         nChNum < pBoard->nMaxDectCh && nPhoneCnt < pBoard->nMaxPhones;
         nChNum++, nPhoneCnt++, nKpiCh++)
   {
      /* by default DECT phone has no data and PCM channels */
      pBoard->rgoPhones[nPhoneCnt].nDectCh = nChNum;
      pBoard->rgoPhones[nPhoneCnt].nDataCh = NO_CHANNEL;
      pBoard->rgoPhones[nPhoneCnt].nDataCh_FD = NO_CHANNEL_FD;
      pBoard->rgoPhones[nPhoneCnt].nPCM_Ch = NO_CHANNEL;
      pBoard->rgoPhones[nPhoneCnt].nPhoneCh = NO_CHANNEL;
      pBoard->rgoPhones[nPhoneCnt].nPhoneCh_FD = NO_CHANNEL_FD;
      pBoard->rgoPhones[nPhoneCnt].nSocket = NO_SOCKET;
      pBoard->rgoPhones[nPhoneCnt].nType = PHONE_TYPE_DECT;
      pBoard->rgoPhones[nPhoneCnt].bDectChActivated = IFX_FALSE;
      /* set phone number (counting from 1) */
      pBoard->rgoPhones[nPhoneCnt].nPhoneNumber = nPhoneCnt + 1;
      pBoard->rgoPhones[nPhoneCnt].pBoard = pBoard;
      pBoard->rgoPhones[nPhoneCnt].nSeqConnId = TD_CONN_ID_INIT;
      /* set KPI for DECT */
      if (IFX_SUCCESS != TD_DECT_KpiCfg(&pBoard->rgoPhones[nPhoneCnt],
                                        nKpiCh))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Phone No %d: TD_DECT_KpiCfg failed (%d)\n"
                "(File: %s, line: %d)\n",
                pBoard->rgoPhones[nPhoneCnt].nPhoneNumber, nChNum,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }

   } /* for all dect channels */
   return IFX_SUCCESS;
}

 /**
   Add dect FDs for select to wait on.

   \param nFd        - FD to read data from.
   \param pDectMsgData  - pointer to DECT message structure.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_GetEventData(TD_OS_Pipe_t* Fp, TD_DECT_MSG_t* pDectMsgData)
{
   IFX_int32_t nLen;
   IFX_int32_t nSize = sizeof(TD_DECT_MSG_t);

   /* check input paraemeters */
   TD_PTR_CHECK(Fp, IFX_ERROR);
   TD_PTR_CHECK(pDectMsgData, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: TD_DECT_GetEventData\n"));

   nLen = TD_OS_PipeRead(pDectMsgData, nSize, 1, Fp);
   if (0 > nLen)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Read error occured %d\n(File: %s, line: %d)\n",
             nLen, __FILE__, __LINE__));

      return IFX_ERROR;
   }

   /* debug trace */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_EVT,
         ("DECT: MSG received ch %d, event %s(%d)\n",
          pDectMsgData->nDectCh,
          Common_Enum2Name(pDectMsgData->nDectEvent, TD_rgDectEventName),
          pDectMsgData->nDectEvent));

   return IFX_SUCCESS;
}

 /**
   Get pointer to phone structure of phone that uses fiven DECT channel.

   \param nDectCh       - number of DECT channel.
   \param pBoard        - pointer to board where phone should be found.
   \param ppPhone       - pointer to phone structure pointer.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_GetPhoneOfChannel(IFX_int32_t nDectCh, BOARD_t* pBoard,
                                       PHONE_t** ppPhone)
{
   IFX_int32_t nPhoneCnt;
   TD_PTR_CHECK(ppPhone, IFX_ERROR);
   /* reset data */
   *ppPhone = IFX_NULL;

   /* for all phones */
   for (nPhoneCnt=0; nPhoneCnt<pBoard->nMaxPhones; nPhoneCnt++)
   {
      /* check if dect phone */
      if (PHONE_TYPE_DECT == pBoard->rgoPhones[nPhoneCnt].nType)
      {
         /* check if phone has the same channel number */
         if (nDectCh == pBoard->rgoPhones[nPhoneCnt].nDectCh)
         {
            /* set pointer */
            *ppPhone = &pBoard->rgoPhones[nPhoneCnt];
            return IFX_SUCCESS;
         }
      } /* check if dect phone */
   } /* for all phones */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
         ("Err, %s failed to get phone with DECT channel %d\n"
          "(File: %s, line: %d)\n",
          pBoard->pszBoardName, nDectCh, __FILE__, __LINE__));
   return IFX_ERROR;
}

 /**
   DECT phone went on-hook, deactivate DECT channel, stop playing tone,
   inform DECT stack.

   \param pPhone     - pointer to PHONE.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_HookOn(PHONE_t* pPhone)
{
   IFX_int32_t nRet = IFX_ERROR;
#if 0
   x_IFX_DECT_CSU_CallParams xCallParams = {0};
   IFX_int32_t nCallHdl;
#endif /* 0 */
   CONNECTION_t* pConn = IFX_NULL;
   IFX_int32_t i;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: Phone No %d: TD_DECT_HookOn\n",
                  pPhone->nPhoneNumber));
#if 0 /* voice stop moved to IFX_DECT_CSU_VoiceModify() */
   nCallHdl = TD_DECT_STACK_GetCallHdlOfChannel(&pPhone->pBoard->pCtrl->oDectStackCtrl,
                                                pPhone->nDectCh);
   if (nCallHdl < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: failed to get call handle for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->nDectCh, __FILE__, __LINE__));
   }
   else
   {
      TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                    ("DECT: IFX_DECT_CSU_VoiceModify(%d,  &xCallParams,"
                     " IFX_DECT_STOP_VOICE, %d)\n",
                     nCallHdl,  pPhone->nDectCh));
      /* stop voice */
      nRet = IFX_DECT_CSU_VoiceModify(nCallHdl, &xCallParams, IFX_DECT_STOP_VOICE,
                                      pPhone->nDectCh);
      if (nRet != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d, IFX_DECT_CSU_VoiceModify failed\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
      }
   }
#endif /* 0 */
   /* no longer need, left just in case, old version had it
   nRet = IFX_DECT_CSU_CallRelease(nCallHdl, &xCallParams,
                                   IFX_DECT_RELEASE_NORMAL);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d, IFX_DECT_CSU_CallRelease failed\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }*/
   /* check if DECT activated */
   if (IFX_TRUE == pPhone->bDectChActivated)
   {
      /* it should be already done in state machine, done to enable 'emergency'
         usage of this function to make sure tone is turned off */
      nRet = TD_DECT_ToneStop(pPhone);
      if (IFX_SUCCESS != nRet)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d, TD_DECT_ToneStop() failed\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
      } /* if (IFX_SUCCESS != nRet) */

      /* disable ec */
      if (IFX_SUCCESS != TD_DECT_SetEC(pPhone, IFX_FALSE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, phone No %d: TD_DECT_SetEC.(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* deactivate DECT channel */
      nRet = TD_DECT_ChannelDeActivate(pPhone);
      if (IFX_SUCCESS != nRet)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d, TD_DECT_ChannelDeActivate() failed\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
      } /* if (IFX_SUCCESS != nRet) */
   } /* check if DECT activated */
   if (IFX_SUCCESS != TD_DECT_DialtoneTimeoutRemove(pPhone->pBoard->pCtrl,
                                                    pPhone))
   {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d, TD_DECT_DialtoneTimeoutRemove() failed\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
   /* for all connections */
   for (i=0; i<MAX_PEERS_IN_CONF; i++)
   {
      /* get connection */
      pConn = &pPhone->rgoConn[i];
      /* check call type */
      if (LOCAL_CALL == pConn->nType)
      {
         /* reset wideband settings */
         pConn->oConnPeer.oLocal.fDectWideband = IFX_FALSE;
      } /* check call type */
   } /* for all connections */
   return nRet;
}

 /**
   Start ringing on DECT phone - initiate call.

   \param pPhone     - pointer to PHONE.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_Ring(PHONE_t* pPhone)
{
   IFX_int32_t nRet;
   x_IFX_DECT_CSU_CallParams xCallParams = {0};
   IFX_int32_t nHandsetId, nCallHdl = -1;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);


   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
         ("Phone No %d: Start Ringing on DECT phone.\n",
          pPhone->nPhoneNumber));

   nHandsetId = TD_DECT_STACK_GetHandsetOfChannel(&pPhone->pBoard->pCtrl->oDectStackCtrl,
                                                  pPhone->nDectCh);
   if (0 > nHandsetId)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: failed to get handset ID fo channel %d.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber,pPhone->nDectCh,
              __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if wideband */
   if (TD_BAND_WIDE == TD_DECT_WideBandCheck(pPhone, &pPhone->rgoConn[0]))
   {
      xCallParams.bwideband = 1;
   }
   else
   {
      xCallParams.bwideband = 0;
   }
   /* reset call handle (pointer) - we will get a new one in IFX_DECT_CSU_CallInitiate */
   nRet = IFX_DECT_CSU_CallInitiate (nHandsetId, &xCallParams, nHandsetId,
                                     (IFX_uint32_t*)&nCallHdl);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: IFX_DECT_CSU_CallInitiate failed.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
   /* add call handle */
   TD_DECT_STACK_AddCallHdl(&pPhone->pBoard->pCtrl->oDectStackCtrl, nHandsetId,
                            nCallHdl);

   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
      ("DECT: IFX_DECT_CSU_CallInitiate(%d, &xCallParams, %d, %d)\n",
       nHandsetId, nHandsetId, nCallHdl));

   return nRet;
}

 /**
   Check if wideband should be used.

   \param pPhone     - pointer to phone strusture.
   \param pConn      - pointer to phone connection structure.

   \return TD_BAND_WIDE if wideband should be used, otherwise TD_BAND_NARROW
*/
TD_BAND_WIDE_NARROW_t TD_DECT_WideBandCheck(PHONE_t* pPhone,
                                            CONNECTION_t* pConn)
{
   /* check input parameters */
   TD_PTR_CHECK(pPhone, TD_BAND_NARROW);
   TD_PTR_CHECK(pConn, TD_BAND_NARROW);

   /* check phone type */
   if (PHONE_TYPE_DECT != pPhone->nType)
   {
      return TD_BAND_NARROW;
   }
   /* check call type */
   if (LOCAL_CALL != pConn->nType)
   {
      return TD_BAND_NARROW;
   }
   /* check if wideband should be used */
   if (IFX_TRUE != pConn->oConnPeer.oLocal.fDectWideband)
   {
      return TD_BAND_NARROW;
   }
   /* use wideband */
   return TD_BAND_WIDE;
}

 /**
   Set wideband flags for connection structures.

   \param pPhone     - pointer to calling phone strusture.
   \param pPhone     - pointer to calling phone connection structure.
   \param pPhone     - pointer to called phone strusture.
   \param pPhone     - pointer to called phone connection structure.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_WideBandSet(PHONE_t* pPhone, CONNECTION_t* pConn,
                                 PHONE_t* pDstPhone, CONNECTION_t* pDstConn)
{
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pDstPhone, IFX_ERROR);
   TD_PTR_CHECK(pDstConn, IFX_ERROR);

   /* check call type - must be local call */
   if (LOCAL_CALL != pConn->nType || LOCAL_CALL != pDstConn->nType)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid call type for calling(%s) or called(%s) phone.\n"
             "(File: %s, line: %d)\n",
             Common_Enum2Name(pConn->nType, TD_rgCallTypeName),
             Common_Enum2Name(pDstConn->nType, TD_rgCallTypeName),
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check phones - both must be DECT */
   if (PHONE_TYPE_DECT != pPhone->nType || PHONE_TYPE_DECT != pDstPhone->nType)
   {
      return IFX_SUCCESS;
   }
   /* check both phones support wideband */
   if (IFX_TRUE != pPhone->fWideBand_Cfg ||
       IFX_TRUE != pDstPhone->fWideBand_Cfg)
   {
      return IFX_SUCCESS;
   }
   /* if one of channels already activated then use narrowband */
   if (IFX_TRUE == pPhone->bDectChActivated ||
       IFX_TRUE == pDstPhone->bDectChActivated)
   {
      return IFX_SUCCESS;
   }
   /* wideband will be used */
   pConn->oConnPeer.oLocal.fDectWideband = IFX_TRUE;
   pDstConn->oConnPeer.oLocal.fDectWideband = IFX_TRUE;

   return IFX_SUCCESS;
}

/**
   Activate DECT connection on DECT stack.

   \param pPhone     - pointer to PHONE.
   \param nBandSetting  - band setting.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_ConnectionActivate(PHONE_t* pPhone,
                                        TD_BAND_WIDE_NARROW_t nBandSetting)
{
   IFX_int32_t nRet = IFX_SUCCESS;
   x_IFX_DECT_CSU_CallParams xCallParams = {0};
   IFX_int32_t nCallHdl;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   /* get call handle */
   nCallHdl = TD_DECT_STACK_GetCallHdlOfChannel(&pPhone->pBoard->pCtrl->oDectStackCtrl,
                                                pPhone->nDectCh);

   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: Phone No %d: TD_DECT_ConnectionActivate\n",
                  pPhone->nPhoneNumber));

   if (nCallHdl < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: failed to get call handle for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check band settings */
   if (TD_BAND_WIDE == nBandSetting)
   {
      xCallParams.bwideband = 1;
   }
   else
   {
      xCallParams.bwideband = 0;
   }
   if ((CALL_DIRECTION_TX == pPhone->nCallDirection) &&
       (TD_BAND_WIDE != nBandSetting))
   {
      TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                    ("DECT: IFX_DECT_CSU_ServiceChange(%d, &xCallParams,"
                     " IFX_DECT_CSU_SC_REQUEST)\n",
                     nCallHdl));

      nRet = IFX_DECT_CSU_ServiceChange(nCallHdl, &xCallParams,
                                        IFX_DECT_CSU_SC_REQUEST);

   }
   else
   {
      TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                    ("DECT: IFX_DECT_CSU_VoiceModify(%d, &xCallParams,"
                     " IFX_DECT_START_VOICE, %d)\n",
                     nCallHdl, pPhone->nDectCh));

      nRet = IFX_DECT_CSU_VoiceModify(nCallHdl, &xCallParams,
                                      IFX_DECT_START_VOICE, pPhone->nDectCh);
   }

   return nRet;
}

 /**
   Activate DECT phone for new conection.

   \param pPhone     - pointer to PHONE.
   \param pConn      - pointer to connection structure.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_PhoneActivate(PHONE_t* pPhone, CONNECTION_t* pConn)
{
   TD_BAND_WIDE_NARROW_t nBandSetting = TD_BAND_NARROW;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT: Phone No %d: Activate phone\n",
                  pPhone->nPhoneNumber));

   /* get band setting */
   nBandSetting = TD_DECT_WideBandCheck(pPhone, pConn);
   /* set encoder */
   if (IFX_SUCCESS != TD_DECT_EncSet(pPhone, nBandSetting))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, phone No %d: TD_DECT_EncSet.(File: %s, line: %d)\n",
                pPhone->nPhoneNumber,
                __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* activate channel */
   if (IFX_SUCCESS != TD_DECT_ChannelActivate(pPhone))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, phone No %d: TD_DECT_ChannelActivate.(File: %s, line: %d)\n",
                pPhone->nPhoneNumber,
                __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* set ec */
   if (IFX_SUCCESS != TD_DECT_SetEC(pPhone, IFX_TRUE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, phone No %d: TD_DECT_SetEC.(File: %s, line: %d)\n",
                pPhone->nPhoneNumber,
                __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* activate connection */
   if (IFX_SUCCESS != TD_DECT_ConnectionActivate(pPhone, nBandSetting))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, phone No %d: TD_DECT_ConnectionActivate.(File: %s, line: %d)\n",
                pPhone->nPhoneNumber,
                __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (CALL_DIRECTION_TX == pPhone->nCallDirection)
   {
      /* remove timeout */
      if (IFX_SUCCESS != TD_DECT_DialtoneTimeoutRemove(pPhone->pBoard->pCtrl,
                                                       pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d: Failed to remove timeout.\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   return IFX_SUCCESS;
}

 /**
   Configure DECT KPI. Connect DECT chanel with KPI channel.

   \param pPhone     - pointer to PHONE.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_KpiCfg(PHONE_t* pPhone, IFX_int32_t nKpiCh)
{
   IFX_return_t nRet = IFX_ERROR;
   IFX_int32_t nDECT_FD;
   IFX_TAPI_KPI_CH_CFG_t xKpiChCfg = {0};

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   /* debug trace */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("DECT: Phone No %d: TD_DECT_KpiCfg\n", pPhone->nPhoneNumber));

   nDECT_FD = TD_DECT_GetFD_OfCh(pPhone->nDectCh, pPhone->pBoard);
   if (NO_DEVICE_FD == nDECT_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, %s failed to get fd for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName, pPhone->nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set configuration */
   xKpiChCfg.nStream = IFX_TAPI_KPI_STREAM_DECT;
   xKpiChCfg.nKpiCh = IFX_TAPI_KPI_GROUP2 | nKpiCh;
   /* configure KPI */
   nRet = TD_IOCTL(nDECT_FD, IFX_TAPI_KPI_CH_CFG_SET, &xKpiChCfg,
             TD_DEV_NOT_SET, pPhone->nSeqConnId);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, pPhone->nDectCh));
   }

   return nRet;
}

 /**
   Get DECT event data and get fot it phone and connection.

   \param nDataReadFd   - FD to read mesage data.
   \param pBoard        - pointer to board where phone should be found.
   \param ppPhone       - pointer to phone structure pointer.
   \param ppConn        - pointer to connection structure pointer.

   \return IFX_SUCCESS if event to handle, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_EventHandle(TD_OS_Pipe_t* pDataReadFp, BOARD_t* pBoard,
                                 PHONE_t** ppPhone, CONNECTION_t** ppConn)
{
   IFX_return_t nRet = IFX_SUCCESS;
   IFX_int32_t nDigitCnt;
   TD_DECT_MSG_t oDectMsgData = {0};

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(ppPhone, IFX_ERROR);
   TD_PTR_CHECK(ppConn, IFX_ERROR);

   /* debug trace */
   TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
      ("DECT: TD_DECT_EventHandle\n"));

   /* check if phone is set */
   if (IFX_NULL != (*ppPhone))
   {
      /* check if there are some unhandled digits */
      if (0 < (*ppPhone)->nDectDialNrCnt)
      {
         /* check if buffer is full */
         if ((MAX_DIALED_NUM - 2) <= (*ppPhone)->nDialNrCnt)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, (*ppPhone)->nSeqConnId,
                  ("Err, (*ppPhone)->nDialNrCnt %d\n(File: %s, line: %d)\n",
                   (*ppPhone)->nDialNrCnt, __FILE__, __LINE__));
            memset((*ppPhone)->pDectDialedNum, 0, MAX_DIALED_NUM);
            (*ppPhone)->nDectDialNrCnt = 0;
            (*ppPhone)->pDialedNum[(*ppPhone)->nDialNrCnt] = '\0';
            return IFX_ERROR;
         }
         else
         {
            /* set new digit to handle by state machine */
            (*ppPhone)->pDialedNum[(*ppPhone)->nDialNrCnt] =
               (*ppPhone)->pDectDialedNum[0];
            (*ppPhone)->nDialNrCnt++;
            /* move digits in array */
            for (nDigitCnt=0; nDigitCnt < (*ppPhone)->nDectDialNrCnt; nDigitCnt++)
            {
               (*ppPhone)->pDectDialedNum[nDigitCnt] =
                  (*ppPhone)->pDectDialedNum[nDigitCnt + 1];
            }
            /* decrease number of digits to be handled */
            (*ppPhone)->nDectDialNrCnt--;
            (*ppPhone)->pDectDialedNum[(*ppPhone)->nDectDialNrCnt] = '\0';
            (*ppPhone)->nIntEvent_FXS = IE_DIALING;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, (*ppPhone)->nSeqConnId,
                  ("Phone No %d: Digit 0x%02X, nDialNrCnt %d\n",
                   (*ppPhone)->nPhoneNumber,
                   (*ppPhone)->pDialedNum[(*ppPhone)->nDialNrCnt- 1],
                   (*ppPhone)->nDialNrCnt));
            return IFX_SUCCESS;
         }
      } /* if (0 < (*ppPhone)->nDectDialNrCnt) */
      else
      {
         return IFX_ERROR;
      } /* if (0 < (*ppPhone)->nDectDialNrCnt) */
   } /* if (IFX_NULL != (*ppPhone)) */

   if (IFX_SUCCESS != TD_DECT_GetEventData(pDataReadFp, &oDectMsgData))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, TD_DECT_GetEventData failed\n(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   } /* if (IFX_SUCCESS != TD_DECT_GetEventData()) */

   /* check channel number */
   if ((TD_NOT_SET == oDectMsgData.nDectCh) ||
       (pBoard->nMaxDectCh <= oDectMsgData.nDectCh))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, invalid DECT channel number %d\n(File: %s, line: %d)\n",
             oDectMsgData.nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get event owner */
   if (IFX_SUCCESS != TD_DECT_GetPhoneOfChannel(oDectMsgData.nDectCh, pBoard,
                                                ppPhone))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, Failed to get Phone for DECT channel number %d\n"
             "(File: %s, line: %d)\n",
             oDectMsgData.nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   } /* if (IFX_SUCCESS != TD_DECT_GetPhoneOfChannel()) */

   /* for DECT phones use first connection */
   *ppConn = &(*ppPhone)->rgoConn[0];
   if (IE_NONE != (*ppPhone)->nIntEvent_FXS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, (*ppPhone)->nSeqConnId,
            ("Err, Phone No %d: Phone event shoud be set to IE_NONE, it is %d\n"
             "(File: %s, line: %d)\n",
             (*ppPhone)->nPhoneNumber, (*ppPhone)->nIntEvent_FXS,
              __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check message event */
   switch (oDectMsgData.nDectEvent)
   {
   case TD_DECT_EVENT_REGISTER:
      (*ppPhone)->nIntEvent_FXS = IE_DECT_REGISTER;
      break;
   case TD_DECT_EVENT_HOOK_ON:
      (*ppPhone)->nIntEvent_FXS = IE_HOOKON;
      break;
   case TD_DECT_EVENT_HOOK_OFF:
      (*ppPhone)->nIntEvent_FXS = IE_HOOKOFF;
      break;
   case TD_DECT_EVENT_FLASH:
      (*ppPhone)->nIntEvent_FXS = IE_FLASH_HOOK;
      break;
   case TD_DECT_EVENT_DIGIT:
      nDigitCnt = 0;
      /* coppy all digits from received message */
      while(oDectMsgData.oData.acRecvDigits[nDigitCnt] != '\0')
      {
         /* copy first digit to phone structure */
         if (0 == nDigitCnt)
         {
            /* check array size */
            if ((MAX_DIALED_NUM - 2) <= (*ppPhone)->nDialNrCnt)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, (*ppPhone)->nSeqConnId,
                     ("Err, (*ppPhone)->nDialNrCnt %d\n(File: %s, line: %d)\n",
                      (*ppPhone)->nDialNrCnt, __FILE__, __LINE__));
               break;
            }
            /* copy digit */
            (*ppPhone)->pDialedNum[(*ppPhone)->nDialNrCnt] =
               oDectMsgData.oData.acRecvDigits[nDigitCnt];
            (*ppPhone)->nDialNrCnt++;
            /* make sure that digit is followed by '\0' */
            (*ppPhone)->pDialedNum[(*ppPhone)->nDialNrCnt] = '\0';
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, (*ppPhone)->nSeqConnId,
                  ("Phone No %d: Digit 0x%02X, nDialNrCnt %d\n",
                   (*ppPhone)->nPhoneNumber,
                   oDectMsgData.oData.acRecvDigits[nDigitCnt],
                   (*ppPhone)->nDialNrCnt));
         } /* if (0 == nDigitCnt) */
         else
         {
            /* copy rest of digits to be handled later - only one digit
               is handled at a time*/
            /* check buffer size */
            if ((IFX_DECT_MAX_DIGITS-2) <= nDigitCnt)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, (*ppPhone)->nSeqConnId,
                     ("Err, nDigitCnt %d\n(File: %s, line: %d)\n",
                      nDigitCnt, __FILE__, __LINE__));
               break;
            }
            /* check buffer size */
            if ((MAX_DIALED_NUM-2) <= (*ppPhone)->nDectDialNrCnt)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, (*ppPhone)->nSeqConnId,
                     ("Err, (*ppPhone)->nDectDialNrCnt %d.\n"
                      "(File: %s, line: %d)\n",
                      nDigitCnt, __FILE__, __LINE__));
               break;
            }
            /* copy digit and increase digit counter */
            (*ppPhone)->pDectDialedNum[(*ppPhone)->nDectDialNrCnt] =
               oDectMsgData.oData.acRecvDigits[nDigitCnt];
            (*ppPhone)->nDectDialNrCnt++;
            (*ppPhone)->pDectDialedNum[(*ppPhone)->nDectDialNrCnt] = '\0';
         } /* if (0 == nDigitCnt) */
         /* go to next digit */
         nDigitCnt++;
      } /* while() */
      /* reset digit buffer */
      if (0 == (*ppPhone)->nDialNrCnt)
      {
         (*ppPhone)->pDialedNum[(*ppPhone)->nDialNrCnt] = '\0';
      }
      (*ppPhone)->pDialedNum[(*ppPhone)->nDialNrCnt] = '\0';
      /* set event */
      (*ppPhone)->nIntEvent_FXS = IE_DIALING;
      /* check if conference is enabled */
      if (pBoard->pCtrl->pProgramArg->oArgFlags.nConference)
      {
         /* check if hash was send as single digit,
            this indicates conference start request from DECT handset */
         if (DIGIT_HASH == (*ppPhone)->pDialedNum[(*ppPhone)->nDialNrCnt - 1])
         {
            /* set event */
            (*ppPhone)->nIntEvent_FXS = IE_CONFERENCE;
         } /* if hash digit was pressed */
      } /* check if conference is enabled */
      break;
   case TD_DECT_EVENT_WIDEBAND_DISABLE:
      /* for this phone windeband is disabled */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, (*ppPhone)->nSeqConnId,
            ("Phone No %d: disable wideband calls between DECT phones.\n",
             (*ppPhone)->nPhoneNumber));
      (*ppPhone)->fWideBand_Cfg = IFX_FALSE;
      break;
   case TD_DECT_EVENT_WIDEBAND_ENABLE:
      /* for this phone windeband is enabled */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, (*ppPhone)->nSeqConnId,
            ("Phone No %d: enable wideband calls between DECT phones.\n",
             (*ppPhone)->nPhoneNumber));
      (*ppPhone)->fWideBand_Cfg = IFX_TRUE;
      break;
   default:
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, (*ppPhone)->nSeqConnId,
            ("DECT: TD_DECT_GetPhoneOfChannel unhandled event type %s (%d)\n",
             Common_Enum2Name(oDectMsgData.nDectEvent, TD_rgDectEventName),
             oDectMsgData.nDectEvent));
      break;

   }
   return nRet;
}

/**
   Set timeout for given phone.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to phone structure

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_DialtoneTimeoutAdd(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
   TD_DECT_DIALTONE_TIMEOUT_CTRL_t* pTimeoutCtrl;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pCtrl->oDectStackCtrl.nDectChNum),
                      pCtrl->oDectStackCtrl.nDectChNum, IFX_ERROR);
   TD_PARAMETER_CHECK((pPhone->nDectCh >= pCtrl->oDectStackCtrl.nDectChNum),
                      pCtrl->oDectStackCtrl.nDectChNum, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > pPhone->nDectCh), pPhone->nDectCh, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   /* get timeout control structure */
   pTimeoutCtrl = pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl;

   /* check flag value */
   if (IFX_TRUE == pTimeoutCtrl[pPhone->nDectCh].bTimeoutActive)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: invalid timeout flag value."
             " (File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get current time */
   pTimeoutCtrl[pPhone->nDectCh].timeDialtoneStart = TD_OS_ElapsedTimeMSecGet(0);
   pTimeoutCtrl[pPhone->nDectCh].bTimeoutActive = IFX_TRUE;

   /* set timeout flag */
   TD_TIMEOUT_START(pCtrl->nUseTimeoutSocket);

   return IFX_SUCCESS;
}

/**
   Reset timeout for given phone.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to phone structure

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_DialtoneTimeoutRemove(CTRL_STATUS_t* pCtrl,
                                           PHONE_t* pPhone)
{
   TD_DECT_DIALTONE_TIMEOUT_CTRL_t* pTimeoutCtrl;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pCtrl->oDectStackCtrl.nDectChNum),
                      pCtrl->oDectStackCtrl.nDectChNum, IFX_ERROR);
   TD_PARAMETER_CHECK((pPhone->nDectCh >= pCtrl->oDectStackCtrl.nDectChNum),
                      pCtrl->oDectStackCtrl.nDectChNum, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > pPhone->nDectCh), pPhone->nDectCh, IFX_ERROR);
   TD_DECT_PHONE_CHECK(pPhone, IFX_ERROR);

   /* get timeout control structure */
   pTimeoutCtrl = pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl;

   /* check flag value */
   if (IFX_FALSE == pTimeoutCtrl[pPhone->nDectCh].bTimeoutActive)
   {
      /* nothing to be done */
      return IFX_SUCCESS;
   }

   /* set timeout */
   TD_TIMEOUT_STOP(pCtrl->nUseTimeoutSocket);
   pTimeoutCtrl[pPhone->nDectCh].bTimeoutActive = IFX_FALSE;

   return IFX_SUCCESS;
}

/**
   Set time for select to wait.

   \param pCtrl      - pointer to status control structure
   \param pTimeOut   - pointer to current timeout for select

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_int32_t TD_DECT_DialtoneTimeoutSelectSet(CTRL_STATUS_t* pCtrl,
                                             IFX_time_t* pTimeOut)
{
   TD_DECT_DIALTONE_TIMEOUT_CTRL_t* pTimeoutCtrl;
   IFX_int32_t nDectCh;
   IFX_time_t nDiffMSec;
   IFX_time_t time_new;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pCtrl->oDectStackCtrl.nDectChNum),
                      pCtrl->oDectStackCtrl.nDectChNum, IFX_ERROR);

   /* get timeout control structure */
   pTimeoutCtrl = pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl;

   /* for all channels */
   for (nDectCh=0; nDectCh < pCtrl->oDectStackCtrl.nDectChNum; nDectCh++)
   {
      /* check if waiting for timeout */
      if (IFX_TRUE == pTimeoutCtrl[nDectCh].bTimeoutActive)
      {
         nDiffMSec =
            TD_OS_ElapsedTimeMSecGet(pTimeoutCtrl[nDectCh].timeDialtoneStart);

         /* check if timeout occured */
         if (nDiffMSec >= TD_DECT_DIALTONE_TIMEOUT_MSEC)
         {
            /* handle timeout if detected*/
            if (IFX_SUCCESS != TD_DECT_DialtoneTimeoutHandle(pCtrl,
                                  &pCtrl->rgoBoards[0], nDectCh))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                     ("Err, %s Failed to handle timeout.\n"
                      "(File: %s, line: %d)\n",
                      pCtrl->rgoBoards[0].pszBoardName,
                      __FILE__, __LINE__));
            }
         }
         else
         {
            /* Check if timeout should be smaller than it is currently set */
            time_new = TD_DECT_DIALTONE_TIMEOUT_MSEC - nDiffMSec;
            if (time_new < *pTimeOut)
            {
               *pTimeOut = time_new;
            }
         }
      } /* check if waiting for timeout */

   } /* for all channels */
   return IFX_SUCCESS;
}

/**
   Check and handle DECT dialtone timeout - activate DECT phone.

   \param pCtrl      - pointer to status control structure
   \param pBoard     - pointer to board structure
   \param nDect      - DECT channel

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_DialtoneTimeoutHandle(CTRL_STATUS_t* pCtrl,
                                           BOARD_t* pBoard,
                                           IFX_int32_t nDectCh)
{
   PHONE_t* pPhone;
   IFX_return_t nRet = IFX_SUCCESS;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pCtrl->oDectStackCtrl.nDectChNum),
                      pCtrl->oDectStackCtrl.nDectChNum, IFX_ERROR);

    /* reset phone pointer */
   pPhone = IFX_NULL;
   /* get phone of channel */
   if (IFX_SUCCESS != TD_DECT_GetPhoneOfChannel(nDectCh, pBoard, &pPhone))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, %s Failed to phone for DECT channel %d.\n"
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, nDectCh,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if in S_OUT_DIALTONE state */
   if (S_OUT_DIALTONE == pPhone->rgStateMachine[FXS_SM].nState)
   {
      /* activate DECT phone */
      if (IFX_SUCCESS != TD_DECT_PhoneActivate(pPhone,
                                               &pPhone->rgoConn[0]))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
               ("Err, Phone No %d: Failed to activate DECT phone.\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* play dialtone */
      TD_CPTG_PLAY_CHECK(nRet, pCtrl, pPhone, DIALTONE_IDX, "Dialtone");
   } /* check if in S_OUT_DIALTONE state */

   return nRet;
}

/**
   Initialize DECT timeout. This timeout is used to play dialtone
   on DECT phones. Tapidemo must wait for timeout to play dialtone on DECT,
   because there are two scenarios how call can be initialized and
   the difference between them is a callback (pfnInfoReceived) received
   after aproximately 300 msec.

   \param pCtrl      - pointer to status control structure

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_DialtoneTimeoutInit(CTRL_STATUS_t* pCtrl)
{
   TD_DECT_DIALTONE_TIMEOUT_CTRL_t* pTimeoutCtrl;
   IFX_int32_t nDectCh;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pCtrl->oDectStackCtrl.nDectChNum),
                      pCtrl->oDectStackCtrl.nDectChNum, IFX_ERROR);

   /* allocate memory */
   pTimeoutCtrl = TD_OS_MemCalloc(pCtrl->oDectStackCtrl.nDectChNum,
                         sizeof(TD_DECT_DIALTONE_TIMEOUT_CTRL_t));

   /* check if memory is allocated */
   if (IFX_NULL == pTimeoutCtrl)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to allocate memory.\n(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* for all channels */
   for (nDectCh=0; nDectCh < pCtrl->oDectStackCtrl.nDectChNum; nDectCh++)
   {
      /* reset flag */
      pTimeoutCtrl[nDectCh].bTimeoutActive = IFX_FALSE;
   } /* for all channels */

   /* save pointer */
   pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl = pTimeoutCtrl;

   return IFX_SUCCESS;
}
/**
   Deinitialize timeout structure.

   \param pCtrl      - pointer to status control structure

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_DialtoneTimeoutDeInit(CTRL_STATUS_t* pCtrl)
{
   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   if (IFX_NULL != pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl)
   {
      TD_OS_MemFree(pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl);
      pCtrl->oDectStackCtrl.pDialtoneTimeoutCtrl = IFX_NULL;
   }

   return IFX_SUCCESS;
}
/**
   Initialize DECT.

   \param pBoard     - pointer to BOARD.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_Init(CTRL_STATUS_t* pCtrl, BOARD_t* pBoard)
{
   x_IFX_DECT_MU_CallBks xMUCbs = {0};
   x_IFX_DECT_CSU_CallBks xCSUCbs = {0};
   x_IFX_DECT_StackInitCfg xInitCfg = {0};
#ifdef EASY50712_V3
#ifdef LINUX
   IFX_char_t *pCmd ="echo 1 > /sys/class/leds/vdsl2_reset/brightness";
   IFX_char_t rgCmdOutput[MAX_CMD_LEN];
#endif /* LINUX */
#endif /* EASY50712_V3 */

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
      ("############# %s DECT STACK INITIALIZATION start #############\n",
       pBoard->pszBoardName));
   /* DECT should be initialized only once */
   if (IFX_TRUE == pCtrl->oDectStackCtrl.nDectInitialized)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, DECT already initialized\n(File: %s, line: %d)\n",
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check number of DECT channels */
   if (0 >= pBoard->nMaxDectCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, %s Invalid number of DECT channels (%d).\n"
          "(File: %s, line: %d)\n",
          pBoard->pszBoardName, pBoard->nMaxDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set pointer to control structure */
   if (IFX_SUCCESS != TD_DECT_STACK_SetCtrlPointer(&pCtrl->oDectStackCtrl))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, %s, TD_DECT_SetCtrlPointer failed\n"
          "(File: %s, line: %d)\n",
          pBoard->pszBoardName, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set DECT channels table */
   if (IFX_SUCCESS != TD_DECT_STACK_ChannelToHandsetInit(pCtrl, pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, %s, TD_DECT_STACK_ChannelToHandsetInit failed\n"
          "(File: %s, line: %d)\n",
          pBoard->pszBoardName, __FILE__, __LINE__));
      return IFX_ERROR;
   }

#ifdef EASY50712_V3
#ifdef LINUX
   /* this is needed before starting the ifxsip to reset the cosic,
      Danube V3 specific action */
   if (IFX_SUCCESS == COM_ExecuteShellCommand(pCmd, rgCmdOutput))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("Called %s.\n", pCmd));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, %s, COM_ExecuteShellCommand(%s) failed\n"
          "(File: %s, line: %d)\n",
          pBoard->pszBoardName, pCmd, __FILE__, __LINE__));
   }
#endif /* LINUX */
#endif /* EASY50712_V3 */

   /* register Managment Unit callbacks... */
   xMUCbs.pfn_MU_Notify = TD_DECT_STACK_MU_Notify;
   xMUCbs.pfn_MU_ModemReset = TD_DECT_STACK_MU_ModemReset;
   IFX_DECT_MU_RegisterCallBks(&xMUCbs);

   /* register Call Service Unit callbacks... */
   xCSUCbs.pfnCallInitiate = TD_DECT_STACK_CallInitiate;
   xCSUCbs.pfnCallAccept = TD_DECT_STACK_CallAccept;
   xCSUCbs.pfnCallAnswer = TD_DECT_STACK_CallAnswer;
   xCSUCbs.pfnCallRelease = TD_DECT_STACK_CallRelease;
   xCSUCbs.pfnInfoReceived = TD_DECT_STACK_InfoReceived;
   xCSUCbs.pfnServiceChange = TD_DECT_STACK_ServiceChange;
#ifndef TD_DECT_BW_COMP_BEFORE_3_1_1_3
   xCSUCbs.pfnInterceptAccept = TD_DECT_STACK_InterceptAccept;
#endif /* TD_DECT_BW_COMP_BEFORE_3_1_1_3 */
   xCSUCbs.pfnVoiceModify = TD_DECT_STACK_VoiceModify;
#ifdef ENABLE_ENCRYPTION
   xCSUCbs.pfnCipherCfm = TD_DECT_STACK_CipherCfm;
#endif

#ifdef CAT_IQ2_0
   xCSUCbs.pfnCallToggle = TD_DECT_STACK_CallToggle;
   xCSUCbs.pfnCallHold = TD_DECT_STACK_CallHold;
   xCSUCbs.pfnCallResume = TD_DECT_STACK_CallResume;
   xCSUCbs.pfnCallTransfer = TD_DECT_STACK_CallTransfer;
   xCSUCbs.pfnCallConference = TD_DECT_STACK_CallConference;
#ifndef TD_DECT_BW_COMP_BEFORE_3_1_1_3
   xCSUCbs.pfnIntrudeInfoRecv = TD_DECT_STACK_IntrudeInfoRecv;
#endif /* TD_DECT_BW_COMP_BEFORE_3_1_1_3 */
#endif

   IFX_DECT_CSU_RegisterCallBks(&xCSUCbs);


   if(TD_DECT_STACK_MODE == IFX_DECT_ASYNC_MODE)
   {
      if (IFX_ERROR == TD_DECT_STACK_PipeOpen(pCtrl))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s, TD_DECT_STACK_PipeOpen failed\n"
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* if(TD_DECT_STACK_MODE == IFX_DECT_ASYNC_MODE) */
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, %s, only IFX_DECT_ASYNC_MODE supported by tapidemo(not %d).\n"
          "(File: %s, line: %d)\n",
          pBoard->pszBoardName, TD_DECT_STACK_MODE, __FILE__, __LINE__));
      return IFX_ERROR;
   } /* if(TD_DECT_STACK_MODE == IFX_DECT_ASYNC_MODE) */

   /* open paging key FD */
   pCtrl->viPagingKeyFd = TD_OS_DeviceOpen(TD_DECT_PAGEKEY_DRV);
   if (pCtrl->viPagingKeyFd <  0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_OS_DeviceOpen(%s) failed (%d)\n"
          "(File: %s, line: %d)\n",
          TD_DECT_PAGEKEY_DRV, pCtrl->viPagingKeyFd,
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* open CLI FD */
   if((TD_DECT_STACK_CliSocketOpen(pCtrl)) < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_STACK_CliSocketOpen failed (%d).\n "
          "(File: %s, line: %d)\n",
          pCtrl->viFromCliFd, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set number of handsets */
   xInitCfg.ucMaxHandsets = pCtrl->oDectStackCtrl.nDectChNum;

   TD_DECT_STACK_RFPI_Get((IFX_char_t*)xInitCfg.aucRfpi, pCtrl);

   /* set Burst Mode Control register parameters */
   if( IFX_SUCCESS != TD_DECT_STACK_BMCRegParamGet(&xInitCfg.xBMCPrams))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_STACK_BMCRegParamGet() failed.\n"
          "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* set frequency Mute control Registers */
   if( IFX_SUCCESS != TD_DECT_STACK_RcFreqParamGet(
                         &xInitCfg.xBMCPrams.ucGENMUTCTRL0,
                         &xInitCfg.xBMCPrams.ucGENMUTCTRL1,
                         &xInitCfg.xBMCPrams.ucEXTMUTCTRL0))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_STACK_OscTrimParamGet() failed.\n"
          "(File: %s, line: %d)\n", __FILE__, __LINE__));
       return IFX_ERROR;
   }

   /* set Oscillator Trimming */
   if( IFX_SUCCESS != TD_DECT_STACK_OscTrimParamGet(&xInitCfg.xOscTrimVal))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_STACK_OscTrimParamGet() failed.\n"
          "(File: %s, line: %d)\n", __FILE__, __LINE__));
       return IFX_ERROR;
   }
   /* set Gaussian Value */
   if( IFX_SUCCESS != TD_DECT_STACK_GaussianValGet(&xInitCfg.unGaussianVal) )
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_STACK_GaussianValGet() failed.\n"
          "(File: %s, line: %d)\n", __FILE__, __LINE__));
       return IFX_ERROR;
   }
   /* set pin number for DECT handset to register */
   if( IFX_SUCCESS != TD_DECT_STACK_BS_PinGet(xInitCfg.acBasePin) )
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_STACK_BS_PinGet() failed.\n"
          "(File: %s, line: %d)\n", __FILE__, __LINE__));
       return IFX_ERROR;
   }
   /* Set TPC parameters */
   if( IFX_SUCCESS != TD_DECT_STACK_TPCParamGet(&xInitCfg.xTPCPrams) )
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_STACK_TPCParamGet() failed.\n"
          "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set debug level */
   if (IFX_SUCCESS != TD_DECT_STACK_DbgLevelSet(pCtrl, &xInitCfg))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_STACK_DbgLevelSet() failed.\n"
          "(File: %s, line: %d)\n",
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set download path */
   if (IFX_SUCCESS != TD_DECT_STACK_SetDownloadPath(pCtrl))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_STACK_SetDownloadPath() failed.\n"
          "(File: %s, line: %d)\n",
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* no PP (dect handset) is registered */
   xInitCfg.ucNoOfReg = 0;

   memset(&xInitCfg.xRegList, 0, sizeof(x_IFX_DECT_FTRegList) * IFX_DECT_MAX_HS);

   /* Set FT Capability to GAP BASE since Tapidemo application
      does not support CATIQ 2.0 features*/
   xInitCfg.xFTCapabilities.aucFixedPartCap[0] =0x38;
   xInitCfg.xFTCapabilities.aucFixedPartCap[1] =0x41;
   xInitCfg.xFTCapabilities.aucFixedPartCap[2] =0x90;
   xInitCfg.xFTCapabilities.aucFixedPartCap[3] =0xCE;
   xInitCfg.xFTCapabilities.aucFixedPartCap[4] =0x00;

   xInitCfg.xFTCapabilities.aucExtendedFixedCap[0] =0x40;
   xInitCfg.xFTCapabilities.aucExtendedFixedCap[1] =0x01;
   xInitCfg.xFTCapabilities.aucExtendedFixedCap[2] =0x0;
   xInitCfg.xFTCapabilities.aucExtendedFixedCap[3] =0x0;
   xInitCfg.xFTCapabilities.aucExtendedFixedCap[4] =0x0;

   xInitCfg.xFTCapabilities.aucExtended2FixedCap[0] =0xC8;
   xInitCfg.xFTCapabilities.aucExtended2FixedCap[1] =0x0;
   xInitCfg.xFTCapabilities.aucExtended2FixedCap[2] =0x80;
   xInitCfg.xFTCapabilities.aucExtended2FixedCap[3] =0x0;
   xInitCfg.xFTCapabilities.aucExtended2FixedCap[4] =0x80;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
      ("DECT: FT Cap, Fixed:  0x%02X 0x%02X 0x%02X 0x%02X 0x%02X.\n",
       xInitCfg.xFTCapabilities.aucFixedPartCap[0],
       xInitCfg.xFTCapabilities.aucFixedPartCap[1],
       xInitCfg.xFTCapabilities.aucFixedPartCap[2],
       xInitCfg.xFTCapabilities.aucFixedPartCap[3],
       xInitCfg.xFTCapabilities.aucFixedPartCap[4]));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
      ("DECT: FT Cap, Extended 1:  0x%02X 0x%02X 0x%02X 0x%02X 0x%02X.\n",
       xInitCfg.xFTCapabilities.aucExtendedFixedCap[0],
       xInitCfg.xFTCapabilities.aucExtendedFixedCap[1],
       xInitCfg.xFTCapabilities.aucExtendedFixedCap[2],
       xInitCfg.xFTCapabilities.aucExtendedFixedCap[3],
       xInitCfg.xFTCapabilities.aucExtendedFixedCap[4]));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
      ("DECT: FT Cap, Extended 2:  0x%02X 0x%02X 0x%02X 0x%02X 0x%02X.\n",
       xInitCfg.xFTCapabilities.aucExtended2FixedCap[0],
       xInitCfg.xFTCapabilities.aucExtended2FixedCap[1],
       xInitCfg.xFTCapabilities.aucExtended2FixedCap[2],
       xInitCfg.xFTCapabilities.aucExtended2FixedCap[3],
       xInitCfg.xFTCapabilities.aucExtended2FixedCap[4]));

   /* run DECT stack */
   if(TD_DECT_STACK_MODE == IFX_DECT_ASYNC_MODE)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("DECT: Running in async mode.\n"));
      /* create DECT stack thread */
      IFX_DECT_Async_Init(&xInitCfg, TD_DECT_STACK_InitDownload);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, only IFX_DECT_ASYNC_MODE mode is supported\n"
          "(File: %s, line: %d)\n",
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* initialize timeout for dialtone */
   if (IFX_SUCCESS != TD_DECT_DialtoneTimeoutInit(pCtrl))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, TD_DECT_DialtoneTimeoutInit() failed\n"
          "(File: %s, line: %d)\n",
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* initialization was successfull */
   pCtrl->oDectStackCtrl.nDectInitialized = IFX_TRUE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
      ("############# %s DECT STACK INITIALIZED #############\n",
       pBoard->pszBoardName));

   return IFX_SUCCESS;
}

/**
   Clean up DECT - close FDs if opened.

   \param  pCtrl - pointer to status control structure

   \return status (IFX_SUCCESS or IFX_ERROR)

   \remark
*/
IFX_return_t TD_DECT_Clean(CTRL_STATUS_t* pCtrl)
{
   /* check input parameter */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   /* check if pProgramArg is set */
   if (IFX_NULL == pCtrl->pProgramArg)
   {
      return IFX_ERROR;
   }
   if (pCtrl->pProgramArg->oArgFlags.nNoDect)
   {
      /* no need to clean if DECT not used */
      return IFX_SUCCESS;
   }
   /* cleanup timeout data */
   TD_DECT_DialtoneTimeoutDeInit(pCtrl);

   /* Close pipes */
   if (0 <= pCtrl->nDectPipeFdOut)
   {
      if (IFX_SUCCESS !=
          TD_OS_PipeClose(pCtrl->oMultithreadCtrl.oPipe.oDect.rgFp[TD_PIPE_OUT]))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, TD_OS_PipeClose OUT failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      }
      pCtrl->nDectPipeFdOut = TD_NOT_SET;
   }
   if (0 <= pCtrl->oDectStackCtrl.nDectPipeFdIn)
   {
      if (IFX_SUCCESS !=
          TD_OS_PipeClose(pCtrl->oMultithreadCtrl.oPipe.oDect.rgFp[TD_PIPE_IN]))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, TD_OS_PipeClose IN failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      }

      pCtrl->oDectStackCtrl.nDectPipeFdIn = TD_NOT_SET;
   }
#ifdef LINUX
   remove(pCtrl->oMultithreadCtrl.oPipe.oDect.rgName);

   /*Added for cleaning the cli socket from temp directory*/
   system("rm -f /tmp/FromCli");
#endif /* LINUX */

   /* Close PAGEKEY_DRV FD */
   if (0 <= pCtrl->viPagingKeyFd)
   {
      TD_OS_DeviceClose(pCtrl->viPagingKeyFd);
      pCtrl->viPagingKeyFd = TD_NOT_SET;
   }
   /* close communication socket to receive data For Production Test Tool */
   if(0 <= pCtrl->viFromCliFd)
   {
      /* Stuff from DECT Cli are shared with other application, so there is no
         adaption with IFX OS */
	   close(pCtrl->viFromCliFd);
      pCtrl->viFromCliFd = TD_NOT_SET;
   }
   /* close communication socket to send data For Production Test Tool */
   if(0 <= pCtrl->oDectStackCtrl.nToCliFd)
   {
      /* Stuff from DECT Cli are shared with other application, so there is no
         adaption with IFX OS */
	   close(pCtrl->oDectStackCtrl.nToCliFd);
      pCtrl->oDectStackCtrl.nToCliFd = TD_NOT_SET;
   }

   /* free memory */
   if (pCtrl->oDectStackCtrl.pChToHandset != IFX_NULL)
   {
      TD_OS_MemFree(pCtrl->oDectStackCtrl.pChToHandset);
      pCtrl->oDectStackCtrl.pChToHandset= IFX_NULL;
   }

   return IFX_SUCCESS;
}

/**
   Set echo canceller (EC) on DECT channel.
   It is possible to enable/disable echo suppressor on DECT channel.

   \param pPhone - Pointer to PHONE.
   \param bEnable - Enable ES.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_SetEC(PHONE_t* pPhone, IFX_boolean_t bEnable)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t nDECT_FD;
   IFX_TAPI_DECT_EC_CFG_t dectEcConf = {0};

   TD_PTR_CHECK(pPhone, IFX_ERROR);

   /* Check if EC should be enabled. By default, EC is enabled.
      In order to disable EC, use command argument --es_dect=0 */
   if (IFX_TAPI_EC_TYPE_OFF == pPhone->pBoard->pCtrl->pProgramArg->nDectEcCfg)
   {
      /* Don't use EC for DECT channel. We don't enable it, so we don't need to
         disable it as well. */
      return IFX_SUCCESS;
   }

   /* Get DECT fd */
   nDECT_FD = TD_DECT_GetFD_OfCh(pPhone->nDectCh, pPhone->pBoard);
   if (NO_DEVICE_FD == nDECT_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, %s failed to get fd for DECT channel %d\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName, pPhone->nDectCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (bEnable)
   {
      /* Activate ES for DECT */
      dectEcConf.nType = pPhone->pBoard->pCtrl->pProgramArg->nDectEcCfg;

      ret = TD_IOCTL(nDECT_FD, IFX_TAPI_DECT_EC_CFG_SET,
               (IFX_int32_t) &dectEcConf, TD_DEV_NOT_SET, pPhone->nSeqConnId);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, ioctl failed, board: %s, dect ch %d, "
                "(File: %s, line: %d)\n",
                pPhone->pBoard->pszBoardName, pPhone->nDectCh,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Activate echo suppressor for DectCh %d.\n",
             pPhone->nPhoneNumber, pPhone->nDectCh));
   }
   else
   {
      /* Deactivate ES for DECT */
      dectEcConf.nType = IFX_TAPI_EC_TYPE_OFF;

      ret = TD_IOCTL(nDECT_FD, IFX_TAPI_DECT_EC_CFG_SET,
               (IFX_int32_t) &dectEcConf, TD_DEV_NOT_SET, pPhone->nSeqConnId);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, ioctl failed, board: %s, dect ch %d, "
                "(File: %s, line: %d)\n",
                pPhone->pBoard->pszBoardName, pPhone->nDectCh,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Deactivate echo suppressor for DectCh %d.\n",
             pPhone->nPhoneNumber, pPhone->nDectCh));
   }

   return IFX_SUCCESS;
}

#endif /* TD_DECT */


