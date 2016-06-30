/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : feature.c
   Date        : 2005-04-06
   Description : This file contains the implementation of the functions for
                 the tapi demo working with additional features:
                 - AGC ( Automated Gain Control )

   \file

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "feature.h"
#ifdef EASY336
#include "lib_svip_rm.h"
#include "abstract.h"
#endif

#include "pcm.h"

/* ============================= */
/* Local structures              */
/* ============================= */

/** AGC related stuff */

/** Action types */
enum
{
   NO_FEATURE_ACTION = -1,
   FEATURE_ACTION_DISABLE = 0,
   FEATURE_ACTION_ENABLE
};

/** Feature list */
enum
{
   NO_FEATURE = -1,
   FEATURE_AGC = 0
};


/* ============================= */
/* Global Structures             */
/* ============================= */


/* ============================= */
/* Global function declaration   */
/* ============================= */


/* ============================= */
/* Local variable definition     */
/* ============================= */


/* ============================= */
/* Local function definition     */
/* ============================= */


/* ============================= */
/* Global function definition    */
/* ============================= */


/* ----------------------------------------------------------------------------
                                 COMMON
 */


/* ----------------------------------------------------------------------------
                        AGC - Automated Gain Control, START
 */


/**
   Enable AGC (Automated Gain Control).

   \param nDataCh_FD - data channel file descriptor
   \param pPhone     - pointer to phone
   \param fEnable    - IFX_TRUE enable it, IFX_FALSE disable it

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t FEATURE_AGC_Enable(IFX_int32_t nDataCh_FD,
                                PHONE_t* pPhone,
                                IFX_boolean_t fEnable)
{
   IFX_return_t ret = IFX_FALSE;
   IFX_TAPI_ENC_AGC_MODE_t nMode;
#ifdef TAPI_VERSION4
   IFX_TAPI_ENC_AGC_ENABLE_t enable;
#endif /* TAPI_VERSION4 */
#ifdef EASY336
   SVIP_RM_Status_t RM_Ret;
   RM_SVIP_RU_t RU;
   BOARD_t *pBoard;
#endif /* EASY336 */

   TD_PARAMETER_CHECK((0 > nDataCh_FD), nDataCh_FD, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

   nMode = fEnable ? IFX_TAPI_ENC_AGC_MODE_ENABLE
                   : IFX_TAPI_ENC_AGC_MODE_DISABLE;

#ifndef TAPI_VERSION4
   ret = TD_IOCTL(nDataCh_FD, IFX_TAPI_ENC_AGC_ENABLE, nMode,
            TD_DEV_NOT_SET, pPhone->nSeqConnId);
#else /* TAPI_VERSION4 */
   memset(&enable, 0, sizeof(enable));

   enable.agcMode = nMode;
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      if (pPhone->nVoipIDs > 0)
      {
         RM_Ret = SVIP_RM_VoipIdRUCodGet(pPhone->pVoipIDs[pPhone->nVoipIDs - 1],
                                         &RU);
         if (RM_Ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, SVIP_RM_VoipIdRUCodGet returned %d (File: %s, "
               "line: %d)\n",
               RM_Ret, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         else
         if (RU.module != IFX_TAPI_MODULE_TYPE_NONE)
         {
            enable.dev = RU.nDev;
            enable.ch = RU.nCh;
            pBoard = ABSTRACT_GetBoard(pPhone->pBoard->pCtrl, RU.devType);
            if (IFX_NULL == pBoard)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, ABSTRACT_GetBoard() failed (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
               return IFX_ERROR;
            }
            nDataCh_FD = pBoard->nDevCtrl_FD;
         }
         else
            return IFX_ERROR;
      }
#endif /* EASY336 */
      ret = TD_IOCTL(nDataCh_FD, IFX_TAPI_ENC_AGC_ENABLE, (IFX_int32_t) &enable,
               enable.dev, pPhone->nSeqConnId);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, Unsupported configuration - unable to enable AGC "
          "(File: %s, line: %d)\n",
          __FILE__, __LINE__));
   }
#endif /* TAPI_VERSION4 */
   return ret;
} /* FEATURE_AGC_Enable() */



/**
   Configure AGC.

   \param  nDataCh_FD - data channel file descriptor (used as voipID for SVIP)
   \param  pPhone - pointer to phone
   \param  nAGC_CompareLvl - compare level
   \param  nAGC_MaxGain - maximum gain
   \param  nAGC_MaxAttenuation - maximum attenuation
   \param  nAGC_MinInputLvl - minimum input level

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t FEATURE_AGC_Cfg(IFX_int32_t nDataCh_FD,
                             PHONE_t* pPhone,
                             IFX_int32_t nAGC_CompareLvl,
                             IFX_int32_t nAGC_MaxGain,
                             IFX_int32_t nAGC_MaxAttenuation,
                             IFX_int32_t nAGC_MinInputLvl)
{
   IFX_return_t ret = IFX_FALSE;
   IFX_TAPI_ENC_AGC_CFG_t cfg_agc;
#ifdef EASY336
   SVIP_RM_Status_t RM_Ret;
   RM_SVIP_RU_t RU;
   BOARD_t *pBoard;
#endif /* EASY336 */
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input arguments */
   TD_PARAMETER_CHECK((0 > nDataCh_FD), nDataCh_FD, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

   memset(&cfg_agc, 0, sizeof(IFX_TAPI_ENC_AGC_CFG_t));

   /* By default use MIN values */
   if ((TAPIDEMO_AGC_CONFIG_COM_MIN <= nAGC_CompareLvl)
       && (TAPIDEMO_AGC_CONFIG_COM_MAX >= nAGC_CompareLvl))
   {
      cfg_agc.com = nAGC_CompareLvl;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Compare Level %d for AGC outside interval [%d..%d], will"
            " use MIN value\n",
            (int) nAGC_CompareLvl, (int) TAPIDEMO_AGC_CONFIG_COM_MIN,
            (int) TAPIDEMO_AGC_CONFIG_COM_MAX));
      cfg_agc.com = TAPIDEMO_AGC_CONFIG_COM_MIN;
   }

   if ((TAPIDEMO_AGC_CONFIG_GAIN_MIN <= nAGC_MaxGain)
       && (TAPIDEMO_AGC_CONFIG_GAIN_MAX >= nAGC_MaxGain))
   {
      cfg_agc.gain = nAGC_MaxGain;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Maximum Gain %d for AGC outside interval [%d..%d], will"
            " use MIN value\n",
           (int) nAGC_MaxGain, (int) TAPIDEMO_AGC_CONFIG_GAIN_MIN,
           (int) TAPIDEMO_AGC_CONFIG_GAIN_MAX));
      cfg_agc.gain = TAPIDEMO_AGC_CONFIG_GAIN_MIN;
   }

   if ((TAPIDEMO_AGC_CONFIG_ATT_MIN <= nAGC_MaxAttenuation)
       && (TAPIDEMO_AGC_CONFIG_ATT_MAX >= nAGC_MaxAttenuation))
   {
      cfg_agc.att = nAGC_MaxAttenuation;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Maximum Attenuation %d for AGC outside interval [%d..%d], will"
            " use MIN value\n",
            (int) nAGC_MaxAttenuation, (int) TAPIDEMO_AGC_CONFIG_ATT_MIN,
            (int) TAPIDEMO_AGC_CONFIG_ATT_MAX));
      cfg_agc.att = TAPIDEMO_AGC_CONFIG_ATT_MIN;
   }

   if ((TAPIDEMO_AGC_CONFIG_LIM_MIN <= nAGC_MinInputLvl)
       && (TAPIDEMO_AGC_CONFIG_LIM_MAX >= nAGC_MinInputLvl))
   {
      cfg_agc.lim = nAGC_MinInputLvl;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Minimum Input Level %d for AGC outside interval [%d..%d], will"
            " use MIN value\n",
            (int) nAGC_MinInputLvl, (int) TAPIDEMO_AGC_CONFIG_LIM_MIN,
            (int) TAPIDEMO_AGC_CONFIG_LIM_MAX));
      cfg_agc.lim = TAPIDEMO_AGC_CONFIG_LIM_MIN;
   }

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      if (pPhone->nVoipIDs > 0)
      {
         RM_Ret = SVIP_RM_VoipIdRUCodGet(
            pPhone->pVoipIDs[pPhone->nVoipIDs - 1], &RU);
         if (RM_Ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, SVIP_RM_VoipIdRUCodGet returned %d (File: %s, "
               "line: %d)\n",
               RM_Ret, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         else
         if (RU.module != IFX_TAPI_MODULE_TYPE_NONE)
         {
            cfg_agc.dev = RU.nDev;
            cfg_agc.ch = RU.nCh;
            pBoard = ABSTRACT_GetBoard(pPhone->pBoard->pCtrl, RU.devType);
            if (IFX_NULL == pBoard)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, ABSTRACT_GetBoard() failed (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
               return IFX_ERROR;
            }
            nDataCh_FD = pBoard->nDevCtrl_FD;
         }
         else
            return IFX_ERROR;
      } /* if (pPhone->nVoipIDs > 0 */
#endif /* EASY336 */
      nDevTmp = cfg_agc.dev;
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   ret = TD_IOCTL(nDataCh_FD, IFX_TAPI_ENC_AGC_CFG, (IFX_int32_t) &cfg_agc,
            nDevTmp, pPhone->nSeqConnId);

   return ret;
} /* FEATURE_AGC_Cfg() */

/* ----------------------------------------------------------------------------
                        AGC - Automated Gain Control, END
 */


/* ----------------------------------------------------------------------------
                         WideBAND/NarrowBAND support, BEGIN
 */


/**
   Configure WideBand or NarrowBand.

   \param  pPhone - phone on which action was selected
   \param  nDataCh_FD - data channel file descriptor
   \param  eLineType - line type to set
   \param  eCoderType - coder type to set
   \param  fWideBand - true if wideband will be set, otherwise narrowband will
                       be configured

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t FEATURE_SetSpeechRange(PHONE_t* pPhone,
                                    IFX_int32_t nDataCh_FD,
                                    IFX_TAPI_LINE_TYPE_t eLineType,
                                    IFX_TAPI_COD_TYPE_t eCoderType,
                                    IFX_boolean_t fWideBand)
{
   IFX_TAPI_LINE_TYPE_CFG_t line_type_parm = {0};
   IFX_TAPI_ENC_CFG_t cod_type_parm = {0};
   IFX_return_t ret = IFX_SUCCESS;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_int32_t nFdTmp = TD_NOT_SET;

#ifdef EASY336
   SVIP_RM_Status_t RM_Ret;
   RM_SVIP_RU_t RU = {0};
#endif /* EASY336 */

#ifdef EASY336
   if (pPhone->nVoipIDs > 0)
   {
      RM_Ret = SVIP_RM_VoipIdRUCodGet(
         pPhone->pVoipIDs[pPhone->nVoipIDs - 1], &RU);
      if (RM_Ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_RM_VoipIdRUCodGet returned %d "
                "(File: %s, line: %d)\n",
                RM_Ret, __FILE__, __LINE__));
         return IFX_ERROR;
      } /* if (RM_Ret != SVIP_RM_Success) */
   } /* if (pPhone->nVoipIDs > 0) */
#endif /* EASY336 */
   line_type_parm.lineType = eLineType;
   line_type_parm.nDaaCh = 0;
#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
      line_type_parm.dev = pPhone->nDev;
      line_type_parm.ch = pPhone->nPhoneCh;
      nDevTmp = line_type_parm.dev;
      nFdTmp = nDataCh_FD;
   } /* if (pPhone->pBoard->fSingleFD) */
   else
#endif /* TAPI_VERSION4 */
   {
      nFdTmp = pPhone->nPhoneCh_FD;
   } /* if (pPhone->pBoard->fSingleFD) */

   ret = TD_IOCTL(nFdTmp, IFX_TAPI_LINE_TYPE_SET,
            (IFX_int32_t) &line_type_parm, nDevTmp, pPhone->nSeqConnId);

   if (ret != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }
   /* Decoder adapts automatically */
   cod_type_parm.nEncType = eCoderType;
   /* Use 10 ms as default */
   cod_type_parm.nFrameLen = IFX_TAPI_COD_LENGTH_10;
   /* not used */
   /*cod_type_parm.AAL2BitPack = 0;*/
#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      cod_type_parm.dev = RU.nDev;
      cod_type_parm.ch = RU.nCh;
      nDevTmp = cod_type_parm.dev;
#endif
   } /* (pPhone->pBoard->fSingleFD) */
#endif

   ret = TD_IOCTL(nFdTmp, IFX_TAPI_ENC_CFG_SET, (int) &cod_type_parm,
            nDevTmp, pPhone->nSeqConnId);

   if (ret != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }

   if (fWideBand)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Set line type of ALM channel %d to WIDEBAND and "
             "encoder to G722 (64k), with frame length set to 10 ms\n",
             pPhone->nPhoneNumber, (int) pPhone->nPhoneCh));
   } /* if (fWideBand */
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Set line type of ALM channel %d to NARROWBAND and "
             "encoder to G711 (uLAW), with frame length set to 10 ms\n",
             pPhone->nPhoneNumber, (int) pPhone->nPhoneCh));
   } /* if (fWideBand */

return ret;
}

/**
   Configure WideBand or NarrowBand for PCM_LOCAL_CALL.

   \param  pPhone - phone on which action was selected
   \param  fWideBand - true if wideband will be set, otherwise narrowband will
                       be configured

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t FEATURE_SetPCM_SpeechRange(PHONE_t* pPhone,
                                        IFX_boolean_t fWideBand)
{
   IFX_return_t ret = IFX_SUCCESS;
   CONNECTION_t* pConn;
   PHONE_t* pOtherPeerPhone = IFX_NULL;
   CONNECTION_t* pOtherPeerConn = IFX_NULL;
   CONNECTION_t* pMasterConn = IFX_NULL;
   CONNECTION_t* pSlaveConn = IFX_NULL;
   TAPIDEMO_PORT_DATA_t oPortData;
   TAPIDEMO_PORT_DATA_t oOtherPeerPortData;

   /* check if phone in connection */
   if (pPhone->pBoard->nType == TYPE_DUSLIC_XT &&
       pPhone->rgoConn[0].nType == LOCAL_PCM_CALL &&
       pPhone->nConfIdx == NO_CONFERENCE &&
       pPhone->rgoConn[0].fActive == IFX_TRUE)
   {
      /* get connection structure */
      pConn = &pPhone->rgoConn[0];
      /* check if phone is set */
      if (IFX_NULL != pConn->oConnPeer.oLocal.pPhone)
      {
         pOtherPeerPhone = pConn->oConnPeer.oLocal.pPhone;
         /* check if phone in connection */
         if (pOtherPeerPhone->pBoard->nType == TYPE_DUSLIC_XT &&
             pOtherPeerPhone->rgoConn[0].nType == LOCAL_PCM_CALL &&
             pOtherPeerPhone->nConfIdx == NO_CONFERENCE &&
             pOtherPeerPhone->rgoConn[0].fActive == IFX_TRUE)
         {
            /* get connection structure */
            pOtherPeerConn = &pOtherPeerPhone->rgoConn[0];
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Phone No %d: phone must be in LOCAL_PCM_CALL connection to "
                   "change wideband setting. (File: %s, line: %d)\n",
                   pOtherPeerPhone->nPhoneNumber, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Failed to get peer.(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      oPortData.nType = PORT_FXS;
      oPortData.uPort.pPhopne = pPhone;

      oOtherPeerPortData.nType = PORT_FXS;
      oOtherPeerPortData.uPort.pPhopne = pOtherPeerPhone;

      /* check if changing wideband settings */
      if ((pPhone->fWideBand_PCM_Cfg == IFX_TRUE && fWideBand == IFX_FALSE) ||
          (pPhone->fWideBand_PCM_Cfg == IFX_FALSE && fWideBand == IFX_TRUE))
      {
         /* turn off WB/NB on this phone */
         PCM_SetActivation(&oPortData,
                           PCM_GetFD_OfCh(pConn->nUsedCh, pPhone->pBoard),
                           IFX_FALSE,
                           pConn->oPCM.nTimeslot_RX, pConn->oPCM.nTimeslot_TX,
                           !fWideBand, pPhone->nSeqConnId);

         /* check if changing wideband settings */
         if ((pOtherPeerPhone->fWideBand_PCM_Cfg == IFX_TRUE &&
              fWideBand == IFX_FALSE) ||
             (pOtherPeerPhone->fWideBand_PCM_Cfg == IFX_FALSE &&
              fWideBand == IFX_TRUE))
         {
            /* turn off WB/NB on this phone */
            PCM_SetActivation(&oOtherPeerPortData,
                              PCM_GetFD_OfCh(pOtherPeerConn->nUsedCh,
                                             pOtherPeerPhone->pBoard),
                              IFX_FALSE,
                              pOtherPeerConn->oPCM.nTimeslot_RX,
                              pOtherPeerConn->oPCM.nTimeslot_TX,
                              !fWideBand, pPhone->nSeqConnId);
            /* get connection master and slave connection */
            if (!pConn->oConnPeer.oLocal.fSlave)
            {
               pMasterConn = pConn;
               pSlaveConn = pOtherPeerConn;
            }
            else
            {
               pMasterConn = pOtherPeerConn;
               pSlaveConn = pConn;
            }
            /* need new timeslots - free the ones that were used*/
            PCM_FreeTimeslots(pMasterConn->oPCM.nTimeslot_RX, pPhone->nSeqConnId);
            PCM_FreeTimeslots(pMasterConn->oPCM.nTimeslot_TX, pPhone->nSeqConnId);

            /* Get two timeslots for this connection */
            pMasterConn->oPCM.nTimeslot_RX = PCM_GetTimeslots(TS_RX, fWideBand,
                                                              pPhone->nSeqConnId);
            pMasterConn->oPCM.nTimeslot_TX = PCM_GetTimeslots(TS_TX, fWideBand,
                                                              pPhone->nSeqConnId);

            /* set connection structures */
            pSlaveConn->oPCM.nTimeslot_RX = pMasterConn->oPCM.nTimeslot_TX;
            pSlaveConn->oPCM.nTimeslot_TX = pMasterConn->oPCM.nTimeslot_RX;



            /* turn on WB/NB on this phone */
            PCM_SetActivation(&oOtherPeerPortData,
                              PCM_GetFD_OfCh(pOtherPeerConn->nUsedCh,
                                             pOtherPeerPhone->pBoard),
                              IFX_TRUE,
                              pOtherPeerConn->oPCM.nTimeslot_RX,
                              pOtherPeerConn->oPCM.nTimeslot_TX,
                              fWideBand, pPhone->nSeqConnId);
            pOtherPeerPhone->fWideBand_PCM_Cfg = fWideBand;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Phone No %d: Speech range changed to %s.\n",
                   pOtherPeerPhone->nPhoneNumber,
                   fWideBand ? "WideBand" : "NarrowBand"));
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Phone No %d: Speech already changed to %s.\n",
                   pOtherPeerPhone->nPhoneNumber,
                   fWideBand ? "WideBand" : "NarrowBand"));
         }
         /* turn on WB/NB on this phone */
         PCM_SetActivation(&oPortData,
                           PCM_GetFD_OfCh(pConn->nUsedCh, pPhone->pBoard),
                           IFX_TRUE,
                           pConn->oPCM.nTimeslot_RX, pConn->oPCM.nTimeslot_TX,
                           fWideBand, pPhone->nSeqConnId);
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("Phone No %d: Speech range changed to %s.\n",
                pPhone->nPhoneNumber,
                fWideBand ? "WideBand" : "NarrowBand"));
         pPhone->fWideBand_PCM_Cfg = fWideBand;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("Phone No %d: Speech already changed to %s.\n",
                pOtherPeerPhone->nPhoneNumber,
                fWideBand ? "WideBand" : "NarrowBand"));
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: phone must be in LOCAL_PCM_CALL connection to "
             "change wideband setting. (File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return ret;
}

/* ----------------------------------------------------------------------------
                          WideBAND/NarrowBAND support, END
 */

