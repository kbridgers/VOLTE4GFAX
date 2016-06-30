


/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : lib_tapi_signal.c
   Desription  :
*******************************************************************************/
/**
   \file lib_tapi_signal.c
   \date
   \brief Enabling/disable signal detections, functions to prepare
          phone for fax/modem transmission.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef VXWORKS
   #include "vxWorks.h"
   #include <vxWorks.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <ioLib.h>
   #include <sys/ioctl.h>
   #include <logLib.h>
   #include <string.h>
#endif /* VXWORKS */

#include "ifx_types.h"
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#include "lib_tapi_signal.h"
#include "tapidemo.h"
#include "state_trans.h"
#include "voip.h"
#include "abstract.h"

/* ============================= */
/* Defines                       */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

#ifdef TD_FAX_MODEM
/** Array used to assign names to the signal values reported by the TAPI */
static TD_ENUM_2_NAME_t TD_rgSig[] =
{
   {IFX_TAPI_SIG_NONE, "IFX_TAPI_SIG_NONE"},
   {IFX_TAPI_SIG_DISRX, "IFX_TAPI_SIG_DISRX"},
   {IFX_TAPI_SIG_DISTX, "IFX_TAPI_SIG_DISTX"},
   {IFX_TAPI_SIG_DIS, "IFX_TAPI_SIG_DIS"},
   {IFX_TAPI_SIG_CEDRX, "IFX_TAPI_SIG_CEDRX"},
   {IFX_TAPI_SIG_CEDTX, "IFX_TAPI_SIG_CEDTX"},
   {IFX_TAPI_SIG_CED, "IFX_TAPI_SIG_CED"},
   {IFX_TAPI_SIG_CNGFAXRX, "IFX_TAPI_SIG_CNGFAXRX"},
   {IFX_TAPI_SIG_CNGFAXTX, "IFX_TAPI_SIG_CNGFAXTX"},
   {IFX_TAPI_SIG_CNGFAX, "IFX_TAPI_SIG_CNGFAX"},
   {IFX_TAPI_SIG_CNGMODRX, "IFX_TAPI_SIG_CNGMODRX"},
   {IFX_TAPI_SIG_CNGMODTX, "IFX_TAPI_SIG_CNGMODTX"},
   {IFX_TAPI_SIG_CNGMOD, "IFX_TAPI_SIG_CNGMOD"},
   {IFX_TAPI_SIG_PHASEREVRX, "IFX_TAPI_SIG_PHASEREVRX"},
   {IFX_TAPI_SIG_PHASEREVTX, "IFX_TAPI_SIG_PHASEREVTX"},
   {IFX_TAPI_SIG_PHASEREV, "IFX_TAPI_SIG_PHASEREV"},
   {IFX_TAPI_SIG_AMRX, "IFX_TAPI_SIG_AMRX"},
   {IFX_TAPI_SIG_AMTX, "IFX_TAPI_SIG_AMTX"},
   {IFX_TAPI_SIG_AM, "IFX_TAPI_SIG_AM"},
   {IFX_TAPI_SIG_TONEHOLDING_ENDRX, "IFX_TAPI_SIG_TONEHOLDING_ENDRX"},
   {IFX_TAPI_SIG_TONEHOLDING_ENDTX, "IFX_TAPI_SIG_TONEHOLDING_ENDTX"},
   {IFX_TAPI_SIG_TONEHOLDING_END, "IFX_TAPI_SIG_TONEHOLDING_END"},
   {IFX_TAPI_SIG_CEDENDRX, "IFX_TAPI_SIG_CEDENDRX"},
   {IFX_TAPI_SIG_CEDENDTX, "IFX_TAPI_SIG_CEDENDTX"},
   {IFX_TAPI_SIG_CEDEND, "IFX_TAPI_SIG_CEDEND"},
   {IFX_TAPI_SIG_CPTD, "IFX_TAPI_SIG_CPTD"},
   {IFX_TAPI_SIG_V8BISRX, "IFX_TAPI_SIG_V8BISRX"},
   {IFX_TAPI_SIG_V8BISTX, "IFX_TAPI_SIG_V8BISTX"},
   {IFX_TAPI_SIG_CIDENDTX, "IFX_TAPI_SIG_CIDENDTX"},
   {IFX_TAPI_SIG_DTMFTX, "IFX_TAPI_SIG_DTMFTX"},
   {IFX_TAPI_SIG_DTMFRX, "IFX_TAPI_SIG_DTMFRX"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_SIG_t"}
};

/** Extended signals names */
static TD_ENUM_2_NAME_t TD_rgSigExt[] =
{
   {IFX_TAPI_SIG_EXT_NONE, "IFX_TAPI_SIG_EXT_NONE"},
   {IFX_TAPI_SIG_EXT_V21LRX, "IFX_TAPI_SIG_EXT_V21LRX"},
   {IFX_TAPI_SIG_EXT_V21LTX, "IFX_TAPI_SIG_EXT_V21LTX"},
   {IFX_TAPI_SIG_EXT_V21L, "IFX_TAPI_SIG_EXT_V21L"},
   {IFX_TAPI_SIG_EXT_V18ARX, "IFX_TAPI_SIG_EXT_V18ARX"},
   {IFX_TAPI_SIG_EXT_V18ATX, "IFX_TAPI_SIG_EXT_V18ATX"},
   {IFX_TAPI_SIG_EXT_V18A, "IFX_TAPI_SIG_EXT_V18A"},
   {IFX_TAPI_SIG_EXT_V27RX, "IFX_TAPI_SIG_EXT_V27RX"},
   {IFX_TAPI_SIG_EXT_V27TX, "IFX_TAPI_SIG_EXT_V27TX"},
   {IFX_TAPI_SIG_EXT_V27, "IFX_TAPI_SIG_EXT_V27"},
   {IFX_TAPI_SIG_EXT_BELLRX, "IFX_TAPI_SIG_EXT_BELLRX"},
   {IFX_TAPI_SIG_EXT_BELLTX, "IFX_TAPI_SIG_EXT_BELLTX"},
   {IFX_TAPI_SIG_EXT_BELL, "IFX_TAPI_SIG_EXT_BELL"},
   {IFX_TAPI_SIG_EXT_V22RX, "IFX_TAPI_SIG_EXT_V22RX"},
   {IFX_TAPI_SIG_EXT_V22TX, "IFX_TAPI_SIG_EXT_V22TX"},
   {IFX_TAPI_SIG_EXT_V22, "IFX_TAPI_SIG_EXT_V22"},
   {IFX_TAPI_SIG_EXT_V22ORBELLRX, "IFX_TAPI_SIG_EXT_V22ORBELLRX"},
   {IFX_TAPI_SIG_EXT_V22ORBELLTX, "IFX_TAPI_SIG_EXT_V22ORBELLTX"},
   {IFX_TAPI_SIG_EXT_V22ORBELL, "IFX_TAPI_SIG_EXT_V22ORBELL"},
   {IFX_TAPI_SIG_EXT_V32ACRX, "IFX_TAPI_SIG_EXT_V32ACRX"},
   {IFX_TAPI_SIG_EXT_V32ACTX, "IFX_TAPI_SIG_EXT_V32ACTX"},
   {IFX_TAPI_SIG_EXT_V32AC, "IFX_TAPI_SIG_EXT_V32AC"},
   {IFX_TAPI_SIG_EXT_CASBELLRX, "IFX_TAPI_SIG_EXT_CASBELLRX"},
   {IFX_TAPI_SIG_EXT_CASBELLTX, "IFX_TAPI_SIG_EXT_CASBELLTX"},
   {IFX_TAPI_SIG_EXT_CASBELL, "IFX_TAPI_SIG_EXT_CASBELL"},
   {IFX_TAPI_SIG_EXT_V21HRX, "IFX_TAPI_SIG_EXT_V21HRX"},
   {IFX_TAPI_SIG_EXT_V21HTX, "IFX_TAPI_SIG_EXT_V21HTX"},
   {IFX_TAPI_SIG_EXT_V21H, "IFX_TAPI_SIG_EXT_V21H"},
   {IFX_TAPI_SIG_EXT_VMD, "IFX_TAPI_SIG_EXT_VMD"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_SIG_EXT_t"}
};

#ifdef TAPI_VERSION4
/** structure to map signal to event */
static SIGNAL_TO_EVENT_t rgoSigToEvt[] =
{
   {IFX_TAPI_SIG_DIS, IFX_TAPI_EVENT_FAXMODEM_DIS},
   {IFX_TAPI_SIG_CED, IFX_TAPI_EVENT_FAXMODEM_CED},
   {IFX_TAPI_SIG_CNGFAX, IFX_TAPI_EVENT_FAXMODEM_CNGFAX},
   {IFX_TAPI_SIG_CNGMOD, IFX_TAPI_EVENT_FAXMODEM_CNGMOD},
   {IFX_TAPI_SIG_PHASEREV, IFX_TAPI_EVENT_FAXMODEM_PR},
   {IFX_TAPI_SIG_AM, IFX_TAPI_EVENT_FAXMODEM_AM},
   {IFX_TAPI_SIG_TONEHOLDING_END, IFX_TAPI_EVENT_FAXMODEM_HOLDEND},
   /* should be {IFX_TAPI_SIG_CEDEND, IFX_TAPI_EVENT_FAXMODEM_CEDEND},
      instead of {IFX_TAPI_SIG_CEDEND, TD_TAPI_SIG_NOT_USED},
      when fax/modem support was implemented for SVIP board,
      enableing event IFX_TAPI_EVENT_FAXMODEM_CEDEND for TAPI_V4 was not
      possible with IFX_TAPI_EVENT_ENABLE */
   {IFX_TAPI_SIG_CEDEND, TD_TAPI_SIG_NOT_USED},
   {IFX_TAPI_SIG_V8BISRX, IFX_TAPI_EVENT_FAXMODEM_V8BIS},
   {IFX_TAPI_SIG_V8BISTX, TD_TAPI_SIG_NOT_USED},
   {TD_MAX_ENUM_ID, TD_TAPI_SIG}
};
/** structure to map extended signal to event */
static SIGNAL_TO_EVENT_t rgoSigToEvtExt[] =
{
   {IFX_TAPI_SIG_EXT_V21L, IFX_TAPI_EVENT_FAXMODEM_V21L},
   {IFX_TAPI_SIG_EXT_V18A, IFX_TAPI_EVENT_FAXMODEM_V18A},
   {IFX_TAPI_SIG_EXT_V27, IFX_TAPI_EVENT_FAXMODEM_V27},
   {IFX_TAPI_SIG_EXT_BELL, IFX_TAPI_EVENT_FAXMODEM_BELL},
   {IFX_TAPI_SIG_EXT_V22, IFX_TAPI_EVENT_FAXMODEM_V22},
   {IFX_TAPI_SIG_EXT_V22ORBELL, IFX_TAPI_EVENT_FAXMODEM_V22ORBELL},
   {IFX_TAPI_SIG_EXT_V32AC, IFX_TAPI_EVENT_FAXMODEM_V32AC},
   {IFX_TAPI_SIG_EXT_CASBELL, IFX_TAPI_EVENT_FAXMODEM_CAS_BELL},
   {IFX_TAPI_SIG_EXT_V21H, IFX_TAPI_EVENT_FAXMODEM_V21H},
   {TD_MAX_ENUM_ID, TD_TAPI_SIG_EXT}
};
#endif /* TAPI_VERSION4 */

/** codec used for fax/modem transmission */
#define TD_FAX_MODEM_CODEC_TYPE           IFX_TAPI_ENC_TYPE_ALAW_VBD
/** frame length used for fax/modem transmission */
#define TD_FAX_MODEM_FRAME_LEN            IFX_TAPI_COD_LENGTH_10
#endif /* defined TD_FAX_MODEM */

/* ============================= */
/* Local function definition     */
/* ============================= */

#ifdef TD_FAX_MODEM
#ifdef TAPI_VERSION4

/**
   Get event number corresponding to given signal.

   \param pSig          - signals to check (can be more then one, but function
                        only checks the first one and removes it from pSig)
   \param pEventIdNum   - output value - number of event that has been found
   \param pSigToEvt     - table with signal numbers and corresponding events
                        numbers
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t SIGNAL_SignalToEvent(IFX_uint32_t* pSig,
                                  IFX_TAPI_EVENT_ID_t* pEventIdNum,
                                  SIGNAL_TO_EVENT_t* pSigToEvt,
                                  IFX_uint32_t nSeqConnId)
{
   IFX_uint32_t nMask = 1;
   IFX_uint32_t nSingleSig;
   SIGNAL_TO_EVENT_t* pSigToEvtTmp;

   /* check input parameters */
   TD_PTR_CHECK(pSig, IFX_ERROR);
   TD_PTR_CHECK(pEventIdNum, IFX_ERROR);
   TD_PTR_CHECK(pSigToEvt, IFX_ERROR);

   /* check if any signal is set */
   if (0 == *pSig)
   {
      return IFX_ERROR;
   }
   /* until highest bit was tested */
   while (0 != nMask)
   {
      /* check if bit is set */
      nSingleSig = nMask & *pSig;
      if (0 != nSingleSig)
      {
         /* reset bit from mask */
         *pSig &= ~nMask;
         pSigToEvtTmp = pSigToEvt;
         /* untill last element of this table is reached */
         while (TD_MAX_ENUM_ID != pSigToEvtTmp->nSigNum)
         {
            /* check signal number in table */
            if (nSingleSig == pSigToEvtTmp->nSigNum)
            {
               /* for some signals event is not set */
               if (TD_TAPI_SIG_NOT_USED == pSigToEvtTmp->nEventId)
               {
                  /* check next signal from pSig */
                  break;
               }
               /* set event ID number - return SUCCESS - event was found */
               *pEventIdNum = pSigToEvtTmp->nEventId;
               return IFX_SUCCESS;
            } /* if (nSingleSig == pSigToEvtTmp->nSigNum) */
            /* go to next element */
            pSigToEvtTmp++;
         } /* while (TD_MAX_ENUM_ID != pSigToEvtTmp->nSigNum) */
         /* if signal was not found print error */
         if (TD_MAX_ENUM_ID != pSigToEvtTmp->nSigNum &&
             TD_TAPI_SIG_NOT_USED != pSigToEvtTmp->nEventId)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, Failed to get event for signal %s (%d). "
                   "(File: %s, line: %d)\n",
                   (TD_TAPI_SIG_EXT == pSigToEvtTmp->nEventId) ?
                   Common_Enum2Name(nSingleSig, TD_rgSigExt) :
                   (TD_TAPI_SIG == pSigToEvtTmp->nEventId) ?
                   Common_Enum2Name(nSingleSig, TD_rgSig) :
                   "(error getting signal type)",
                   nSingleSig, __FILE__, __LINE__));
         } /* if (TD_MAX_ENUM_ID != pSigToEvtTmp->nSigNum) */
      } /* if (0 != nSingleSig) */
      nMask = nMask << 1;
   } /* while (0 != nMask) */

   return IFX_ERROR;
}

/**
   Enable fax/modem event detection on phone.

   \param pPhone  - pointer to PHONE
   \param nEnDis  - enable/disable flag
   \param pSignal - pointer to signal data
   \param pConn   - pointer to phone connection

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t SIGNAL_EventDetection(PHONE_t* pPhone,
                                   IFX_enDis_t nEnDis,
                                   IFX_TAPI_SIG_DETECTION_t *pSignal,
                                   CONNECTION_t* pConn)
{
   IFX_TAPI_EVENT_t tapiEvent = {0};
   IFX_return_t ioctlRet = IFX_ERROR;
   BOARD_t *pBoard = IFX_NULL;
   IFX_uint32_t nSigTemp;
   SIGNAL_TO_EVENT_t* pSigToEvt;
   IFX_TAPI_EVENT_ID_t nEventId;
#ifdef EASY336
   SVIP_RM_Status_t ret = SVIP_RM_Success;
   RM_SVIP_RU_t RU;
#endif /* EASY336 */
   IFX_int32_t i;

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pSignal, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
#ifndef EASY336
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);
#endif /* EASY336 */

   if ((IFX_ENABLE != nEnDis) && (IFX_DISABLE != nEnDis))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid nEnDis=%d value. (File: %s, line: %d)\n",
             nEnDis, __FILE__, __LINE__));
      return IFX_ERROR;
   }

#ifdef EASY336

   /* get resource data (device and channel) */
   ret = SVIP_RM_VoipIdRUCodGet(pConn->voipID, &RU);
   if (ret == SVIP_RM_Success)
   {
      pBoard = ABSTRACT_GetBoard(pPhone->pBoard->pCtrl, RU.devType);
      if (pBoard == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Unable to get board (Phone No: %d, DevType %d)"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, RU.devType, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      tapiEvent.dev = RU.nDev;
      tapiEvent.ch = RU.nCh;
      tapiEvent.module = RU.module;
   }
   else
   {
      /* Failed to get reasource */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to get resource (ret %d, type %d). "
             "(File: %s, line: %d)\n",
             ret, RU.devType, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#else /* EASY336 */
   pBoard = pPhone->pBoard;

   tapiEvent.dev = pPhone->nDev;
   tapiEvent.ch = pPhone->nPhoneCh;
   tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
#endif /* EASY336 */

   tapiEvent.data.fax_sig.external = 1;
   tapiEvent.data.fax_sig.internal = 1;
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: %s signal detectors:\n",
          pPhone->nPhoneNumber, IFX_ENABLE == nEnDis ? "Enable" : "Disable"));
   /* for both signal normal and extended */
   for (i=0; i<2; i++)
   {
      if (0 == i)
      {
         nSigTemp = pSignal->sig;
         pSigToEvt = rgoSigToEvt;
      }
      else if (1 == i)
      {
         nSigTemp = pSignal->sig_ext;
         pSigToEvt = rgoSigToEvtExt;
      }
      else
      {
         /* invalid number of signal */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Invalid number of signals - %d. (File: %s, line: %d)\n",
                i, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      while (IFX_SUCCESS == SIGNAL_SignalToEvent(&nSigTemp, &nEventId,
                               pSigToEvt, pPhone->nSeqConnId))
      {
         tapiEvent.id = nEventId;
         if (IFX_ENABLE == nEnDis)
         {
            ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_ENABLE,
                          (IFX_int32_t) &tapiEvent,
                          tapiEvent.dev, pPhone->nSeqConnId);
         }
         else if (IFX_DISABLE == nEnDis)
         {
            ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_DISABLE,
                          (IFX_int32_t) &tapiEvent,
                          tapiEvent.dev, pPhone->nSeqConnId);
         }

         if (ioctlRet != IFX_SUCCESS)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, ioctl failed for %s (File: %s, line: %d)\n",
                   Common_Enum2Name(nEventId, TD_rgTapiEventName),
                   __FILE__, __LINE__));
            return IFX_ERROR;
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                  ("%s- %s\n", pPhone->pBoard->pIndentPhone,
                   Common_Enum2Name(nEventId, TD_rgTapiEventName)));
         }
      }
   }
   return IFX_SUCCESS;
}
#else /* TAPI_VERSION4 */
/**
   Helper function to display the name assigned to a signal value

   \param   pSignal  -  pointer to structure wth signals
   \param   pTraceIndention - used to make indention of signal list - to make
                              better looking traces
   \param nSeqConnId  - Seq Conn ID

   \return  status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t SIGNAL_PrintNames(IFX_TAPI_SIG_DETECTION_t *pSignal,
                               IFX_char_t* pTraceIndention,
                               IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_SIG_DETECTION_t stSignal={0};
   IFX_uint32_t nTempSignal=0;
   IFX_uint32_t nMask = 0;

   TD_PTR_CHECK(pSignal, IFX_ERROR)
   memcpy((IFX_char_t *)&stSignal, (IFX_char_t *)pSignal,
           sizeof(IFX_TAPI_SIG_DETECTION_t));

   /* set first bit in signal mask */
   nMask = 1;
   while (stSignal.sig != 0)
   {
      /* check if nMask flag is set */
      nTempSignal = stSignal.sig & nMask;
      if (nTempSignal != 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("%s- %s\n", pTraceIndention,
                Common_Enum2Name(nTempSignal, TD_rgSig)));
      }
      /* remove this signal flag */
      stSignal.sig &= ~nMask;
      /* set mask to next signal flag */
      nMask <<= 1;
   } /* while() */

   /* set first bit in signal mask */
   nMask = 1;
   while (stSignal.sig_ext != 0)
   {
      /* check if nMask flag is set */
      nTempSignal = stSignal.sig_ext & nMask;
      if (nTempSignal != 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("%s- %s\n", pTraceIndention,
                Common_Enum2Name(nTempSignal, TD_rgSigExt)));
      }
      /* remove this signal flag */
      stSignal.sig_ext &= ~nMask;
      /* set mask to next signal flag */
      nMask <<= 1;
   } /* while() */

   return IFX_SUCCESS;
} /* SIGNAL_PrintNames() */
#endif /* TAPI_VERSION4 */
#endif /* TD_FAX_MODEM */

/* ============================= */
/* Global function definition    */
/* ============================= */

#ifdef TAPI_VERSION4
/**
   Function to enable the detection of a signal and report via TAPI exception.

   \param   pConn  - pointer to phone connection
   \param   pSignal  -  pointer to signal to enable
   \param   pPhone   - pointer to phone

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
*/
IFX_return_t SIGNAL_EnableDetection(CONNECTION_t* pConn,
                                    IFX_TAPI_SIG_DETECTION_t *pSignal,
                                    PHONE_t* pPhone)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_FAX_MODEM
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pSignal, IFX_ERROR);
   /* check if there is any signal for enabling. */
   if ((pSignal->sig != 0) || (pSignal->sig_ext != 0))
   {
      ret = SIGNAL_EventDetection(pPhone, IFX_ENABLE, pSignal, pConn);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: function %s called, but signal is not set\n",
             pPhone->nPhoneNumber, __FUNCTION__));
   }
#endif /* TD_FAX_MODEM */
   return ret;
} /* SIGNAL_EnableDetection() */

/**
   Function to disable the detection of a signal

   \param   pConn    - pointer to phone connection
   \param   pSignal  -  pointer to signal to disable
   \param   pPhone   - pointer to phone

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
*/
IFX_return_t SIGNAL_DisableDetection(CONNECTION_t* pConn,
                                     IFX_TAPI_SIG_DETECTION_t *pSignal,
                                     PHONE_t* pPhone)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_FAX_MODEM
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pSignal, IFX_ERROR);

   /* check if there is any signal for disabling. */
   if ((pSignal->sig != 0) || (pSignal->sig_ext != 0))
   {
      ret = SIGNAL_EventDetection(pPhone, IFX_DISABLE, pSignal, pConn);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: function %s called, but signal is not set\n",
             pPhone->nPhoneNumber, __FUNCTION__));
   }
#endif /* TD_FAX_MODEM */
   return ret;
} /* SIGNAL_DisableDetection() */
#else /* TAPI_VERSION4 */

/**
   Function to enable the detection of a signal and report via TAPI exception.

   \param   pConn    - pointer to phone connection
   \param   pSignal  -  pointer to signal to enable
   \param   pPhone   - pointer to phone

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
*/
IFX_return_t SIGNAL_EnableDetection(CONNECTION_t* pConn,
                                    IFX_TAPI_SIG_DETECTION_t *pSignal,
                                    PHONE_t* pPhone)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_FAX_MODEM
   BOARD_t *pUsedBoard;

   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pSignal, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Enable signal detectors:\n", pPhone->nPhoneNumber));

#ifdef EASY3111
   /* EASY 3111 uses data channels from main board */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pUsedBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }

   SIGNAL_PrintNames(pSignal, pUsedBoard->pIndentPhone, pPhone->nSeqConnId);

   /* check if there is any signal for enabling. */
   if ((pSignal->sig != 0) || (pSignal->sig_ext != 0))
   {
      ret = TD_IOCTL(pConn->nUsedCh_FD, IFX_TAPI_SIG_DETECT_ENABLE, pSignal,
               TD_DEV_NOT_SET, pPhone->nSeqConnId);
   }
#endif /* TD_FAX_MODEM */
   return ret;
} /* SIGNAL_EnableDetection() */

/**
   Function to disable the detection of a signal

   \param   pConn    - pointer to phone connection
   \param   pSignal  -  pointer to signal to disable
   \param   pPhone   - pointer to phone

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
*/
IFX_return_t SIGNAL_DisableDetection(CONNECTION_t* pConn,
                                     IFX_TAPI_SIG_DETECTION_t *pSignal,
                                     PHONE_t* pPhone)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_FAX_MODEM
   BOARD_t *pUsedBoard;

   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pSignal, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Disable signal detectors:\n", pPhone->nPhoneNumber));

#ifdef EASY3111
   /* EASY 3111 uses data channels from main board */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pUsedBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }

   SIGNAL_PrintNames(pSignal, pUsedBoard->pIndentPhone, pPhone->nSeqConnId);


   /* check if there is any signal for disabling. */
   if ((pSignal->sig != 0) || (pSignal->sig_ext != 0))
   {
      ret = TD_IOCTL(pConn->nUsedCh_FD, IFX_TAPI_SIG_DETECT_DISABLE, pSignal,
               TD_DEV_NOT_SET, pPhone->nSeqConnId);
   }
#endif /* TD_FAX_MODEM */
   return ret;
} /* SIGNAL_DisableDetection() */
#endif /* TAPI_VERSION4 */

/**
   Function to switch a transmission channel to a uncompressed codec.

   \param   pPhone   -  pointer to PHONE_t
   \param   pConn    - pointer to phone connection

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
   This function switches a transmission channel to G711 coded, sets
   a fixed jitter buffer, disables silence suppresion and comfort noise
   generation. Further event transmission and auto suppresion are turned off.
*/
IFX_return_t SIGNAL_ClearChannel(PHONE_t *pPhone, CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_FAX_MODEM
#ifdef TAPI_VERSION4
   IFX_TAPI_ENC_VAD_CFG_t oVadCfg;
#endif /* TAPI_VERSION4 */
   VOIP_DATA_CH_t *pOurCodec = IFX_NULL;
   IFX_TAPI_JB_CFG_t tapi_jb_conf;
   BOARD_t *pBoard = IFX_NULL;
   IFX_int32_t nDataChFD = -1;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_uint32_t nIoctlArg = TD_NOT_SET;

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);
#ifdef TAPI_VERSION4
   pBoard = ABSTRACT_GetBoard(pPhone->pBoard->pCtrl,
                              IFX_TAPI_DEV_TYPE_VIN_SUPERVIP);
#else /* TAPI_VERSION4 */
#ifdef EASY3111
   /* EASY 3111 uses data channels from main board */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pBoard = pPhone->pBoard;
   }
#endif /* TAPI_VERSION4 */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* check if coder channels are avaible and structure allocated */
   if (0 >= pBoard->nMaxCoderCh || IFX_NULL == pBoard->pDataChStat)
   {
      return ret;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: SIGNAL_ClearChannel()\n", pPhone->nPhoneNumber));
#ifdef TAPI_VERSION4
#ifdef EASY336
   pOurCodec = Common_GetCodec(pBoard->pCtrl, pConn->voipID, pPhone->nSeqConnId);
   if (pBoard->fSingleFD)
   {
      nDataChFD = pBoard->nDevCtrl_FD;
   }
#endif /* EASY336 */
#else /* TAPI_VERSION4 */
   pOurCodec = &pBoard->pDataChStat[pConn->nUsedCh];
   nDataChFD = pConn->nUsedCh_FD;
#endif /* TAPI_VERSION4 */
   if (IFX_NULL == pOurCodec)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to get codec. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (pOurCodec->fCodecStarted == IFX_TRUE)
   {
      /* switch to fax/modem codec */
      pOurCodec->nCodecType = TD_FAX_MODEM_CODEC_TYPE;
      pOurCodec->nFrameLen = TD_FAX_MODEM_FRAME_LEN;
      pOurCodec->fStartCodec = IFX_TRUE;

      /* stop currently started code */
      if (VOIP_StopCodec(pConn->nUsedCh, pConn, pBoard, pPhone) == IFX_ERROR)
         return IFX_ERROR;
      /* start codec with new configuration */
      if (VOIP_StartCodec(pConn->nUsedCh, pConn, pBoard, pPhone) == IFX_ERROR)
         return IFX_ERROR;

      /* set fixed jitter buffer */
      memset(&tapi_jb_conf, 0x00, sizeof(IFX_TAPI_JB_CFG_t));

      tapi_jb_conf.nJbType      =  IFX_TAPI_JB_TYPE_FIXED;
      tapi_jb_conf.nPckAdpt     =  IFX_TAPI_JB_PKT_ADAPT_DATA;
      tapi_jb_conf.nScaling     =  0x16;   /* Scaling factor */
      tapi_jb_conf.nInitialSize =  0x0190; /* Inital JB size 50 ms */
      tapi_jb_conf.nMinSize     =  0x00A0; /* Min.   JB size 20 ms */
      tapi_jb_conf.nMaxSize     =  0x0320; /* Max.   JB size 100 ms */
#ifdef TAPI_VERSION4
      TD_SET_CH_AND_DEV(tapi_jb_conf, pOurCodec->nCh, pOurCodec->nDev);
      nDevTmp = tapi_jb_conf.dev;
#endif /* TAPI_VERSION4 */

      ret = TD_IOCTL(nDataChFD, IFX_TAPI_JB_CFG_SET,
               (IFX_int32_t) &tapi_jb_conf, nDevTmp, pPhone->nSeqConnId);

      TD_RETURN_IF_ERROR(ret);

#ifdef TAPI_VERSION4
      TD_SET_CH_AND_DEV(oVadCfg, pOurCodec->nCh, pOurCodec->nDev);
      nDevTmp = oVadCfg.dev;
      /* Disable silence suppression and confort noise generation */
      oVadCfg.vadMode = IFX_TAPI_ENC_VAD_NOVAD;
      nIoctlArg = (IFX_uint32_t) &oVadCfg;
#else /* TAPI_VERSION4 */
      nIoctlArg = IFX_TAPI_ENC_VAD_NOVAD;
#endif /* TAPI_VERSION4 */

      /* Disable silence suppression and confort noise generation */
      ret = TD_IOCTL(nDataChFD, IFX_TAPI_ENC_VAD_CFG_SET, nIoctlArg,
               nDevTmp, pPhone->nSeqConnId);

      TD_RETURN_IF_ERROR(ret);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Codec not started. (File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
   COM_ITM_TRACES(pBoard->pCtrl, pPhone, pConn,
                  IFX_NULL, TTP_CLEAR_CHANNEL, pPhone->nSeqConnId);
#endif /* TD_FAX_MODEM */
   return ret;
} /* SIGNAL_ClearChannel() */

/**
   Function to disable the non-linear-processor

   \param   pPhone      -  pointer to PHONE

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
*/
IFX_return_t SIGNAL_DisableNLP(PHONE_t *pPhone, CTRL_STATUS_t* pCtrl)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_FAX_MODEM
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_int32_t nPhoneFd = -1;
   IFX_TAPI_WLEC_CFG_t tapi_lec_conf;

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   memset(&tapi_lec_conf, 0, sizeof(IFX_TAPI_WLEC_CFG_t));
#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
      nPhoneFd = pPhone->pBoard->nDevCtrl_FD;
   }
   nDevTmp = pPhone->nDev;
   TD_SET_CH_AND_DEV(tapi_lec_conf, pPhone->nPhoneCh, nDevTmp);
#else /* TAPI_VERSION4 */
   nPhoneFd = pPhone->nPhoneCh_FD;
#endif /* TAPI_VERSION4 */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: SIGNAL_DisableNLP()\n", pPhone->nPhoneNumber));

   /* Switch the NLP off*/
   tapi_lec_conf.nType = pCtrl->pProgramArg->nLecCfg;
   tapi_lec_conf.bNlp = IFX_TAPI_WLEC_NLP_OFF;
   /* use default values */
   tapi_lec_conf.nNBFEwindow = 0;
   tapi_lec_conf.nNBNEwindow = 0;
   tapi_lec_conf.nWBNEwindow = 0;

   /* no LEC setting for DxT */
   if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
   {
      ret = TD_IOCTL(nPhoneFd, IFX_TAPI_WLEC_PHONE_CFG_SET,
               (IFX_int32_t) &tapi_lec_conf, nDevTmp, pPhone->nSeqConnId);
      TD_RETURN_IF_ERROR(ret);
   }
   else
   {
      /* no LEC settings for DxT */
      ret = IFX_SUCCESS;
   }

   COM_ITM_TRACES(pCtrl, pPhone, &pPhone->rgoConn[0],
                  IFX_NULL, TTP_DISABLE_NLP, pPhone->nSeqConnId);

#endif /* TD_FAX_MODEM */
   return ret;
} /* SIGNAL_DisableNLP() */

/**
   Setup initial signal detection for fax/modem transmission.

   \param   pPhone   - pointer to PHONE_t
   \param   pConn    - pointer to phone connection

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
   This function is called when the calling subscriber moves to active state.
*/
IFX_return_t SIGNAL_Setup(PHONE_t* pPhone, CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_FAX_MODEM

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* signals that detection should be enabled */
   pPhone->stSignal.sig = TD_SIGNAL_LIST | IFX_TAPI_SIG_DIS;
   pPhone->stSignal.sig_ext = TD_SIGNAL_LIST_EXT;

   ret = SIGNAL_EnableDetection(pConn, &pPhone->stSignal, pPhone);
#endif /* TD_FAX_MODEM */
   return ret;
} /* SIGNAL_Setup() */

/**
   reset signal handler
   This function is called e.g. on hook interrupts

   \param   pPhone   - pointer to PHONE_t
   \param   pConn    - pointer to phone connection

   \return  status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t SIGNAL_HandlerReset(PHONE_t* pPhone, CONNECTION_t* pConn)
{
   IFX_return_t ret= IFX_SUCCESS;
#ifdef TD_FAX_MODEM

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   ret = SIGNAL_DisableDetection(pConn, &pPhone->stSignal, pPhone);
   pPhone->stSignal.sig = 0;
   pPhone->stSignal.sig_ext = 0;
#endif /* TD_FAX_MODEM */
   return ret;
} /* SIGNAL_HandlerReset() */

/**
   Function to restore transmission channel from a uncompressed codec.

   \param   pPhone      -  pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
   This function switches a transmission channel to codec used previously by it
*/
IFX_return_t SIGNAL_RestoreChannel(PHONE_t *pPhone, CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_FAX_MODEM
   IFX_TAPI_ENC_CFG_t cod_type_parm = {0};
#ifdef TAPI_VERSION4
   IFX_TAPI_ENC_VAD_CFG_t oVadCfg;
#endif /* TAPI_VERSION4 */
   BOARD_t *pBoard = IFX_NULL;
   VOIP_DATA_CH_t *pOurCodec = IFX_NULL;
   IFX_int32_t nDataChFD = -1, nPhoneChFD = -1;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_uint32_t nIoctlArg = TD_NOT_SET;

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);

#ifdef TAPI_VERSION4
#ifdef EASY336
   pBoard = ABSTRACT_GetBoard(pPhone->pBoard->pCtrl,
                              IFX_TAPI_DEV_TYPE_VIN_SUPERVIP);
#endif /* EASY336 */
#else /* TAPI_VERSION4 */
#ifdef EASY3111
   /* EASY 3111 uses data channels from main board */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pBoard = pPhone->pBoard;
   }
#endif /* TAPI_VERSION4 */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* check if coder channels are avaible and structure allocated */
   if (0 >= pBoard->nMaxCoderCh || IFX_NULL == pBoard->pDataChStat)
   {
      return ret;
   }
#ifdef TAPI_VERSION4
#ifdef EASY336
   pOurCodec = Common_GetCodec(pBoard->pCtrl, pConn->voipID, pPhone->nSeqConnId);
#endif /* EASY336 */
   if (pBoard->fSingleFD)
   {
      nDataChFD = pBoard->nDevCtrl_FD;
      nPhoneChFD = pPhone->pBoard->nDevCtrl_FD;
   }
#else /* TAPI_VERSION4 */
   pOurCodec = &pBoard->pDataChStat[pConn->nUsedCh];
   nDataChFD = pConn->nUsedCh_FD;
   nPhoneChFD = pPhone->nPhoneCh_FD;
#endif /* TAPI_VERSION4 */
   if (IFX_NULL == pOurCodec)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to get codec. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Check if channel setting were changed. */
   if (IFX_TRUE != pOurCodec->fRestoreChannel)
   {
      return ret;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Restore Channel\n", pPhone->nPhoneNumber));

   /* Restore previous codec settings */
   pOurCodec->nCodecType = pOurCodec->nPrevCodecType;
   pOurCodec->nFrameLen = pOurCodec->nPrevFrameLen;
   pOurCodec->fStartCodec = IFX_TRUE;

   cod_type_parm.nEncType = pOurCodec->nPrevCodecType;
   cod_type_parm.nFrameLen = pOurCodec->nPrevFrameLen;
#ifdef TAPI_VERSION4
   TD_SET_CH_AND_DEV(cod_type_parm, pOurCodec->nCh, pOurCodec->nDev);
   nDevTmp = pOurCodec->nDev;
#endif /* TAPI_VERSION4 */
   /* Change codec to previous. */
   ret = TD_IOCTL(nDataChFD, IFX_TAPI_ENC_CFG_SET,(int) &cod_type_parm,
            nDevTmp, pPhone->nSeqConnId);

#ifdef TAPI_VERSION4
   TD_SET_CH_AND_DEV(pOurCodec->tapi_jb_PrevConf,
                     pOurCodec->nCh, pOurCodec->nDev);
   nDevTmp = pOurCodec->tapi_jb_PrevConf.dev;
#endif /* TAPI_VERSION4 */
   /* Restore previous settings for Jitter buffer */
   ret = TD_IOCTL(nDataChFD, IFX_TAPI_JB_CFG_SET,
            (IFX_int32_t) &pOurCodec->tapi_jb_PrevConf,
            nDevTmp, pPhone->nSeqConnId);
#ifdef TAPI_VERSION4
   TD_SET_CH_AND_DEV(pOurCodec->oPrevLECcfg, pPhone->nPhoneCh, pPhone->nDev);
   nDevTmp = pOurCodec->oPrevLECcfg.dev;
#endif /* TAPI_VERSION4 */
   /* no LEC settings for DxT */
   if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
   {
      /* Restore previous settings for LEC */
      ret = TD_IOCTL(nPhoneChFD, IFX_TAPI_WLEC_PHONE_CFG_SET,
               (IFX_int32_t) &pOurCodec->oPrevLECcfg,
               nDevTmp, pPhone->nSeqConnId);
   }
   else
   {
      /* no LEC settings for DxT */
      ret = IFX_SUCCESS;
   }

   COM_ITM_TRACES(pBoard->pCtrl, pPhone, pConn,
                  IFX_NULL, TTP_LEC_ENABLE, pPhone->nSeqConnId);

   /* restore VAD settings */
#ifdef TAPI_VERSION4
   TD_SET_CH_AND_DEV(oVadCfg, pOurCodec->nCh, pOurCodec->nDev);
   nDevTmp = oVadCfg.dev;
   nIoctlArg = (IFX_uint32_t) &oVadCfg;
   oVadCfg.vadMode = pBoard->pCtrl->pProgramArg->nVadCfg;
#else /* TAPI_VERSION4 */
   nIoctlArg = pBoard->pCtrl->pProgramArg->nVadCfg;
#endif /* TAPI_VERSION4 */

   /* Restore VAD settings */
   ret = TD_IOCTL(nDataChFD, IFX_TAPI_ENC_VAD_CFG_SET, nIoctlArg,
            nDevTmp, pPhone->nSeqConnId);

#if 0
   /* Enable high-pass filter */
   ret = TD_IOCTL(nDataChFD, IFX_TAPI_COD_DEC_HP_SET, IFX_TRUE,
            nDevTmp, pPhone->nSeqConnId);
   TD_RETURN_IF_ERROR(ret);
#endif

   if (IFX_SUCCESS == ret)
   {
      pOurCodec->fRestoreChannel = IFX_FALSE;

      COM_ITM_TRACES(pBoard->pCtrl, pPhone, pConn,
                     IFX_NULL, TTP_RESTORE_CHANNEL, pPhone->nSeqConnId);
   }
#endif /* TD_FAX_MODEM */
   return ret;
} /* SIGNAL_RestoreChannel() */

/**
   Store current settings like used coder, LEC configuration etc. It is used
   for fax/modem to restore configuration when fax/modem transmision is
   finished.

   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t SIGNAL_StoreSettings(PHONE_t *pPhone, CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_FAX_MODEM
   BOARD_t *pBoard = IFX_NULL;
   VOIP_DATA_CH_t *pOurCodec = IFX_NULL;
   IFX_int32_t nPhoneChFD = -1;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);

   /* get board */
#ifdef TAPI_VERSION4
#ifdef EASY336
   pBoard = ABSTRACT_GetBoard(pPhone->pBoard->pCtrl,
                              IFX_TAPI_DEV_TYPE_VIN_SUPERVIP);
#endif /* EASY336 */
#else /* TAPI_VERSION4 */
#ifdef EASY3111
   /* EASY 3111 uses data channels from main board */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pBoard = pPhone->pBoard;
   }
#endif /* TAPI_VERSION4 */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* check if coder channels are avaible and structure allocated */
   if ((0 >= pBoard->nMaxCoderCh) || (IFX_NULL == pBoard->pDataChStat))
   {
      return ret;
   }

#ifdef TAPI_VERSION4
#ifdef EASY336
   pOurCodec = Common_GetCodec(pBoard->pCtrl, pConn->voipID, pPhone->nSeqConnId);
#endif /* EASY336 */
   if (pBoard->fSingleFD)
   {
      nPhoneChFD = pPhone->pBoard->nDevCtrl_FD;
   }
#else /* TAPI_VERSION4 */
   pOurCodec = &pBoard->pDataChStat[pConn->nUsedCh];
   nPhoneChFD = pPhone->nPhoneCh_FD;
#endif /* TAPI_VERSION4 */
   if (IFX_NULL == pOurCodec)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to get codec. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Save current configuration*/
   pOurCodec->fRestoreChannel = IFX_TRUE;

   /* Save codec and length */
   pOurCodec->nPrevCodecType = pOurCodec->nCodecType;
   pOurCodec->nPrevFrameLen = pOurCodec->nFrameLen;
#ifdef TAPI_VERSION4
   TD_SET_CH_AND_DEV(pOurCodec->oPrevLECcfg, pPhone->nPhoneCh, pPhone->nDev);
   nDevTmp = pOurCodec->oPrevLECcfg.dev;
#endif /* TAPI_VERSION4 */
   /* no LEC settings for DxT */
   if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
   {
      /* Save LEC settings of phone ch in data channel statistics*/
      ret = TD_IOCTL(nPhoneChFD, IFX_TAPI_WLEC_PHONE_CFG_GET,
               &pOurCodec->oPrevLECcfg, nDevTmp, pPhone->nSeqConnId);
   }
   else
   {
      /* no LEC settings for DxT */
      ret = IFX_SUCCESS;
   }
#endif /* TD_FAX_MODEM */

   return ret;
} /* SIGNAL_StoreSettings() */

#if defined(TAPI_VERSION4) && defined(EASY336)
#if 0
IFX_return_t SIGNAL_GetCoderRu(RM_SVIP_RU_t* pCodRu, PHONE_t* pPhone)
{
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCodRu, IFX_ERROR);
   TD_PARAMETER_CHECK((1 != pPhone->nVoipIDs), pPhone->nVoipIDs);

   if (IFX_ERROR == SVIP_RM_VoipIdRUCodGet(pPhone->pVoipIDs[pPhone->nVoipIDs-1],
                                            pCodRu))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: SVIP_RM_VoipIdRUCodGet failed. "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }

}
#endif /* 0 */
#endif /* defined(TAPI_VERSION4) && defined(EASY336) */

