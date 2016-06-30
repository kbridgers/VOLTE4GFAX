/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_t38.c
   \date 200x-xx-xx
   \brief FAX T.38 implementation.

   This file provides functions for fax T.38 transmission.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "lib_tapi_signal.h"
#include "td_t38.h"
#include "voip.h"
#include "qos.h"
#ifdef TAPI_VERSION4
#include "abstract.h"
#endif /* TAPI_VERSION4 */

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))

/** table with association enum value -> name for IFX_TAPI_T38_PROTOCOL_t. */
TD_ENUM_2_NAME_t TD_rgT38Protocol_Name[] =
{
   {IFX_TAPI_T38_TCP, "IFX_TAPI_T38_TCP"},
   {IFX_TAPI_T38_UDP, "IFX_TAPI_T38_UDP"},
#ifndef TAPI_VERSION4 /* enum IFX_TAPI_T38_TCP_UDP not defined for tapi4 yet */
   {IFX_TAPI_T38_TCP_UDP, "IFX_TAPI_T38_TCP_UDP"},
#endif /* TAPI_VERSION4 */
   {TD_MAX_ENUM_ID, "IFX_TAPI_T38_PROTOCOL_t"}
};

/** table with association enum value -> name for IFX_TAPI_T38_RMM_t. */
TD_ENUM_2_NAME_t TD_rgT38RMM_Name[] =
{
   {IFX_TAPI_T38_LOC_TCF, "IFX_TAPI_T38_LOC_TCF"},
   {IFX_TAPI_T38_TRANS_TCF, "IFX_TAPI_T38_TRANS_TCF"},
#ifndef TAPI_VERSION4
   /* enum IFX_TAPI_T38_LOC_TRANS_TCF not defined for tapi4 yet */
   {IFX_TAPI_T38_LOC_TRANS_TCF, "IFX_TAPI_T38_LOC_TRANS_TCF"},
#endif /* TAPI_VERSION4 */
   {TD_MAX_ENUM_ID, "IFX_TAPI_T38_RMM_t"}
};

/** table with association enum value -> name for
    IFX_TAPI_T38_FACSIMILE_CNVT_t. */
TD_ENUM_2_NAME_t TD_rgT38FacsimileCNVT_Name[] =
{
   {IFX_TAPI_T38_HFBMR, "IFX_TAPI_T38_HFBMR"},
   {IFX_TAPI_T38_HTMMR, "IFX_TAPI_T38_HTMMR"},
   {IFX_TAPI_T38_TJBIG, "IFX_TAPI_T38_TJBIG"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_T38_FACSIMILE_CNVT_t"}
};
/** table with association enum value -> name for
    IFX_TAPI_T38_SESS_STATE_t. */
TD_ENUM_2_NAME_t rgSesStateName[] =
{
   {IFX_TAPI_T38_NEG, "IFX_TAPI_T38_NEG"},
   {IFX_TAPI_T38_MOD, "IFX_TAPI_T38_MOD"},
   {IFX_TAPI_T38_DEM, "IFX_TAPI_T38_DEM"},
   {IFX_TAPI_T38_TRANS, "IFX_TAPI_T38_TRANS"},
   {IFX_TAPI_T38_PP, "IFX_TAPI_T38_PP"},
   {IFX_TAPI_T38_INT, "IFX_TAPI_T38_INT"},
   {IFX_TAPI_T38_DCN, "IFX_TAPI_T38_DCN"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_T38_SESS_STATE_t"}
};
/** table with association enum value -> name for
    IFX_TAPI_T38_SESS_FLAGS_t. */
TD_ENUM_2_NAME_t rgSesFlagsName[] =
{
   {IFX_TAPI_T38_SESS_FEC, "IFX_TAPI_T38_SESS_FEC"},
   {IFX_TAPI_T38_SESS_RED, "IFX_TAPI_T38_SESS_RED"},
   {IFX_TAPI_T38_SESS_ECM, "IFX_TAPI_T38_SESS_ECM"},
   {IFX_TAPI_T38_SESS_T30COMPL, "IFX_TAPI_T38_SESS_T30COMPL"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_T38_SESS_FLAGS_t"}
};
/** table with association enum value -> name for
    IFX_TAPI_T38_FDPSTD_t. */
TD_ENUM_2_NAME_t rgSesFDPSTDName[] =
{
   {IFX_TAPI_T38_FDPSTD_V27_2400, "V27TER_2400"},
   {IFX_TAPI_T38_FDPSTD_V27_4800, "V27TER_4800"},
   {IFX_TAPI_T38_FDPSTD_V29_7200, "V29_7200"},
   {IFX_TAPI_T38_FDPSTD_V29_9600, "V29_9600"},
   {IFX_TAPI_T38_FDPSTD_V17_7200, "V17_7200"},
   {IFX_TAPI_T38_FDPSTD_V17_9600, "V17_9600"},
   {IFX_TAPI_T38_FDPSTD_V17_12000, "V17_12000"},
   {IFX_TAPI_T38_FDPSTD_V17_14400, "V17_14400"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_T38_FDPSTD_t"}
};
   /** table with association enum value -> name for IFX_TAPI_T38_EC_MODE_t. */
TD_ENUM_2_NAME_t TD_rgT38EcMode_Name[] =
{
   {IFX_TAPI_T38_RED, "IFX_TAPI_T38_RED"},
   {IFX_TAPI_T38_FEC, "IFX_TAPI_T38_FEC"},
   {TD_MAX_ENUM_ID, ""}
};
   /** table with association enum value -> name for IFX_TAPI_T38_EC_CAP_t. */
TD_ENUM_2_NAME_t TD_rgT38EcCaP_Name[] =
{
   {IFX_TAPI_T38_CAP_RED, "IFX_TAPI_T38_CAP_RED"},
   {IFX_TAPI_T38_CAP_FEC, "IFX_TAPI_T38_CAP_FEC"},
#ifndef TAPI_VERSION4
   /* enum IFX_TAPI_T38_CAP_RED_FEC not defined for tapi4 yet */
   {IFX_TAPI_T38_CAP_RED_FEC, "IFX_TAPI_T38_CAP_RED_FEC"},
#endif /* TAPI_VERSION4 */
   {TD_MAX_ENUM_ID, "IFX_TAPI_T38_EC_CAP_t"}
};

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */

#ifdef TAPI_VERSION4
/**
   Get coder channel and device number according to voip ID of connection.

   \param pCtrl         - pointer to control structure
   \param pConn         - pointer to phone connection
   \param pChannelNum   - pointer to channel number (OUTPUT)
   \param pDeviceNum    - pointer to device number (OUTPUT)
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_T38_GetVoipIdChAndDev(CTRL_STATUS_t* pCtrl,
                                      CONNECTION_t* pConn,
                                      IFX_uint16_t *pChannelNum,
                                      IFX_uint16_t *pDeviceNum,
                                      IFX_uint32_t nSeqConnId)
{
   VOIP_DATA_CH_t *pOurCodec = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pChannelNum, IFX_ERROR);
   TD_PTR_CHECK(pDeviceNum, IFX_ERROR);

   /* */
#ifdef EASY336
   pOurCodec = Common_GetCodec(pCtrl, pConn->voipID, nSeqConnId);
#endif /* EASY336 */

   if (IFX_NULL == pOurCodec)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Unable to get coder data. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   *pChannelNum = pOurCodec->nCh;
   *pDeviceNum = pOurCodec->nDev;
   return IFX_SUCCESS;
}

/**
   Get FD used for coder module.

   \param pCtrl         - pointer to control structure

   \return FD number on ok, otherwise IFX_ERROR
*/
IFX_int32_t TD_T38_GetCodFd(CTRL_STATUS_t* pCtrl)
{
   BOARD_t* pBoard;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   pBoard = ABSTRACT_GetBoard(pCtrl, IFX_TAPI_DEV_TYPE_VIN_SUPERVIP);

   /* check if structure is set */
   if (IFX_NULL == pBoard)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, Failed to get valid board. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (pBoard->fSingleFD)
      return pBoard->nDevCtrl_FD;
   else
      return IFX_ERROR;
}
#endif /* TAPI_VERSION4 */

#ifdef TD_T38_FAX_TEST
/**
   Setup structure with default fax data pump(fdp) configuration.

   \param  pBoard - pointer to board structure
   \param  bGetFromDrivers - if IFX_TRUE then read settings from drivers,
                             if else then use default settings

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_FdpCfgSetStruct(BOARD_t* pBoard,
                                    IFX_boolean_t bGetFromDrivers)
{
   IFX_TAPI_T38_FDP_CFG_t* pT38FdpCfg;
   IFX_int32_t nDataFd;
   IFX_return_t nRet = IFX_ERROR;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* get pointer to structure */
   pT38FdpCfg = &pBoard->oT38_Config.oT38FdpCfg;

   memset(pT38FdpCfg, 0, sizeof (IFX_TAPI_T38_FDP_CFG_t));

   if (IFX_TRUE == bGetFromDrivers)
   {
      /* use firts file descriptor(data channel) */
      nDataFd = Common_GetFD_OfCh(pBoard, 0);
      if (0 <= pBoard->nMaxCoderCh)
      {
         /* get configuration */
         nRet = TD_IOCTL(nDataFd, IFX_TAPI_T38_FDP_CFG_GET, pT38FdpCfg,
                   TD_DEV_NOT_SET, TD_CONN_ID_INIT);
         if (IFX_SUCCESS == nRet)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
                  ("FAX T.38: Read out of FAX Data Pump configuration "
                   "successfull.\n"));
#if 0
   /* this data is also printed when ioctl() is called to set this setting,
      code below left for future use (to debug if needed) */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                  ("   - MOBSZ %d,\n   - MOBSM %d,\n"
                   "   - MOBRD %d,\n   - DMBSD %d.\n",
                   pT38FdpCfg->nMobsz, pT38FdpCfg->nMobsm,
                   pT38FdpCfg->nMobrd, pT38FdpCfg->nDmbsd));
#endif /* 0 */
            /* FDP settings were successfully read */
            return nRet;
         }
         /* in case of error reset structure */
         memset(pT38FdpCfg, 0, sizeof (IFX_TAPI_T38_FDP_CFG_t));
      }
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("%s: Using default T.38 FDP configuration.\n",
          pBoard->pszBoardName));
   /* Fax Data Pump parameters can optionally be adjusted */
   /* dev and ch used for TAPI_V4, for TAPI_V3 set both to 0
   pT38FdpCfg->dev = 0;
   pT38FdpCfg->ch = 0;  */
   /* Modulation buffer size. */
   pT38FdpCfg->nMobsz = TD_T38_FAX_DEF_FDP_MOBSZ;
   /* Required modulation buffer fill level to start modulation */
   pT38FdpCfg->nMobsm = TD_T38_FAX_DEF_FDP_MOBSM;
   /* Required modulation buffer fill level for generation of data request */
   pT38FdpCfg->nMobrd = TD_T38_FAX_DEF_FDP_MOBRD;
   /* Required demodulation buffer level */
   pT38FdpCfg->nDmbsd = TD_T38_FAX_DEF_FDP_DMBSD;

#if 0
   /* this data is also printed when ioctl() is called to set this setting,
      code below left for future use (to debug if needed) */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("   - MOBSZ %d,\n   - MOBSM %d,\n"
          "   - MOBRD %d,\n   - DMBSD %d.\n",
          pT38FdpCfg->nMobsz, pT38FdpCfg->nMobsm,
          pT38FdpCfg->nMobrd, pT38FdpCfg->nDmbsd));
#endif /* 0 */

   return IFX_SUCCESS;
}

/**
   Setup fax data pump configuration(fdp) for given data channel.

   \param  pPhone - pointer to PHONE
   \param  nFaxCh  - FAX channel

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_FdpCfgSet(PHONE_t* pPhone, IFX_int32_t nFaxCh)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_T38_FDP_CFG_t* pT38FdpCfg;
   IFX_int32_t nFd = IFX_ERROR;
   BOARD_t *pUsedBoard;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pUsedBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }
   TD_PARAMETER_CHECK(((nFaxCh < 0) || (nFaxCh >= pUsedBoard->nMaxCoderCh)),
                      nFaxCh, IFX_ERROR);

   /* get pointer to structure */
   pT38FdpCfg = &pUsedBoard->oT38_Config.oT38FdpCfg;

   pT38FdpCfg->ch = 0;
   pT38FdpCfg->dev = 0;
#ifdef TAPI_VERSION4
   nFd = TD_T38_GetCodFd(pUsedBoard->pCtrl);
#else /* TAPI_VERSION4 */
   nFd = Common_GetFD_OfCh(pUsedBoard, nFaxCh);
#endif /* TAPI_VERSION4 */
   TD_PARAMETER_CHECK((nFd == IFX_ERROR), nFd, IFX_ERROR);
#ifdef TAPI_VERSION4
   /* assumption is made that first connection is used */
   if (IFX_ERROR == TD_T38_GetVoipIdChAndDev(pUsedBoard->pCtrl,
                       &pPhone->rgoConn[0], &pT38FdpCfg->ch, &pT38FdpCfg->dev,
                       pPhone->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Failed to get coder channel and device. "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* TAPI_VERSION4 */
   nDevTmp = pT38FdpCfg->dev;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Setting FAX Data Pump configuration:\n",
          pPhone->nPhoneNumber));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("%s- MOBSZ %d,\n%s- MOBSM %d,\n"
          "%s- MOBRD %d,\n%s- DMBSD %d.\n",
          pPhone->pBoard->pIndentPhone, pT38FdpCfg->nMobsz,
          pPhone->pBoard->pIndentPhone, pT38FdpCfg->nMobsm,
          pPhone->pBoard->pIndentPhone, pT38FdpCfg->nMobrd,
          pPhone->pBoard->pIndentPhone, pT38FdpCfg->nDmbsd));

   ret = TD_IOCTL(nFd, IFX_TAPI_T38_FDP_CFG_SET, (IFX_int32_t) pT38FdpCfg,
            nDevTmp, pPhone->nSeqConnId);
   if (IFX_SUCCESS != ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, nFaxCh));
   }

   return ret;
}

/**
   Setup structure with default fax stack session configuration.

   \param  pBoard - pointer to board structure
   \param  nUDPErrCorr - UDP error correction method
   \param  bGetFromDrivers - if IFX_TRUE then read settings from drivers,
                             if else then use default settings

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_CfgSetStruct(BOARD_t* pBoard,
                                 IFX_TAPI_T38_EC_MODE_t nUDPErrCorr,
                                 IFX_boolean_t bGetFromDrivers)
{
   IFX_TAPI_T38_FAX_CFG_t* pT38Cfg;
   IFX_int32_t nDataFd;
   IFX_return_t nRet = IFX_ERROR;
#if 0
   IFX_int32_t i;
   IFX_char_t stTempNsx[TD_MAX_NAME] = {0};
#endif
   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* get pointer to structure */
   pT38Cfg = &pBoard->oT38_Config.oT38FaxCfg;

   memset(pT38Cfg, 0, sizeof (IFX_TAPI_T38_FAX_CFG_t));

   if (IFX_TRUE == bGetFromDrivers)
   {
      /* use firts file descriptor(data channel) */
      nDataFd = VOIP_GetFD_OfCh(0, pBoard);
      if (0 <= pBoard->nMaxCoderCh)
      {
         /* get configuration */
         nRet = TD_IOCTL(nDataFd, IFX_TAPI_T38_CFG_GET, pT38Cfg,
                   TD_DEV_NOT_SET, TD_CONN_ID_INIT);
         if (IFX_SUCCESS == nRet)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
                  ("FAX T.38: Read out of FAX channel configuration "
                   "successfull.\n"));
#if 0
      TD_T38_GET_NSX_STRING(stTempNsx, i, pT38Cfg->aNsx, pT38Cfg->nNsxLen,
                            TD_CONN_ID_INIT);
      /* this data is also printed when ioctl() is called to set this setting,
         code below left for future use (to debug if needed) */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("   - option mask %d,\n"
             "   - desired output power level -%d dBm,\n"
             "   - data wait time %d ms,\n"
             "   - gain: RX %d, TX %d,\n"
             "   - IFPSI %d,\n",
             pT38Cfg->OptionMask, pT38Cfg->nDbm, pT38Cfg->nDWT,
             pT38Cfg->nGainRx, pT38Cfg->nGainTx, pT38Cfg->nIFPSI));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("   - modulation auto start time %d,\n"
             "   - spoofing insert time %d,\n"
             "   - number FEC calculation packets %d,\n"
             "   - number of recovery packets %d, high speed %d, low speed %d,\n"
             "   - NSX %s (len %d).\n",
             pT38Cfg->nModAutoStartTime, pT38Cfg->nSpoofAutoInsTime,
             pT38Cfg->nPktFec, pT38Cfg->nPktRecovInd,
             pT38Cfg->nPktRecovHiSpeed, pT38Cfg->nPktRecovLoSpeed,
             stTempNsx, pT38Cfg->nNsxLen));
#endif /* 0 */
            /* FDP settings were successfully read */
            return nRet;
         }
         /* in case of error reset structure */
         memset(pT38Cfg, 0, sizeof (IFX_TAPI_T38_FAX_CFG_t));
      }
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("%s: Using default T.38 stack session configuration.\n",
          pBoard->pszBoardName));

   if (IFX_NULL == pT38Cfg)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Structure pT38Cfg is not initialized."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* set gain level */
   pT38Cfg->nGainRx = TD_T38_FAX_DEF_CFG_GAIN_RX;
   pT38Cfg->nGainTx = TD_T38_FAX_DEF_CFG_GAIN_TX;
   /* The T.38 options define protocol features and can be ORed */
   pT38Cfg->OptionMask = TD_T38_FAX_DEF_CFG_OPT_MASK;
   /* IFP packets send interval */
   pT38Cfg->nIFPSI = TD_T38_FAX_DEF_CFG_IFPSI;
   /* Desired output power level */
   pT38Cfg->nDbm = TD_T38_FAX_DEF_CFG_OUT_POW_LEVEL;
   if (IFX_TAPI_T38_RED == nUDPErrCorr)
   {
      /* Number of additional recovery data packets sent via high speed
      FAX transmissions */
      pT38Cfg->nPktRecovHiSpeed = TD_T38_FAX_DEF_CFG_RED_PCK_RECOV_HIG;
      /* Number of additional recovery data packets sent via low speed
      FAX transmissions */
      pT38Cfg->nPktRecovLoSpeed = TD_T38_FAX_DEF_CFG_RED_PCK_RECOV_LOW;
      /* Number of packets to calculate FEC */
      pT38Cfg->nPktFec = TD_T38_FAX_DEF_CFG_RED_PCK_FEC;
   }
   else if (IFX_TAPI_T38_FEC == nUDPErrCorr)
   {
      /* FEC error correction method is negotiated */
      /* Number of additional recovery data packets sent via high speed
      FAX transmissions */
      /* 2->1 taking into account IP network bandwidth
      Example for 1 channel: rate 14400bps, 20ms packets interval
      With nPktRecovHiSpeed = 0, bandwidth = 29.6kbps;
      with nPktRecovHiSpeed = 1, bandwidth = 47.2kbps*/
      pT38Cfg->nPktRecovHiSpeed = TD_T38_FAX_DEF_CFG_FEC_PCK_RECOV_HIG;
      /* Number of additional recovery data packets sent via low speed
      FAX transmissions */
      pT38Cfg->nPktRecovLoSpeed = TD_T38_FAX_DEF_CFG_FEC_PCK_RECOV_LOW;
      /* Number of packets to calculate FEC */
      pT38Cfg->nPktFec = TD_T38_FAX_DEF_CFG_FEC_PCK_FEC;
   }
   /* Number of additional recovery T30_INDICATOR packets */
   pT38Cfg->nPktRecovInd = TD_T38_FAX_DEF_CFG_PCK_RECOV_IND;
   /* Data wait time (500 ms). */
   pT38Cfg->nDWT = TD_T38_FAX_DEF_CFG_DWT;
   /* Timeout for start of T.38 modulation (2000 ms) */
   pT38Cfg->nModAutoStartTime = TD_T38_FAX_DEF_CFG_MOD_START_TIME;
   /* Time to insert spoofing during automatic modulation (4000 ms) */
   pT38Cfg->nSpoofAutoInsTime = TD_T38_FAX_DEF_CFG_SPOOF_INS_TIME;
   if (TD_T38_FAX_DEF_CFG_NSX_LEN > IFX_TAPI_T38_NSXLEN)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Invalid NSX length %d, can not be more than %d."
             "(File: %s, line: %d)\n",
             TD_T38_FAX_DEF_CFG_NSX_LEN, IFX_TAPI_T38_NSXLEN,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Length (bytes) of valid data in the 'aNsx' field */
   pT38Cfg->nNsxLen = TD_T38_FAX_DEF_CFG_NSX_LEN;
   /* Data bytes of NSX field */
   /* Set fields that do not match any country coding according to T.35 */
   memcpy(pT38Cfg->aNsx, TD_T38_FAX_DEF_CFG_NSX, TD_T38_FAX_DEF_CFG_NSX_LEN);

#if 0
   TD_T38_GET_NSX_STRING(stTempNsx, i, pT38Cfg->aNsx, pT38Cfg->nNsxLen,
                         TD_CONN_ID_INIT);
      /* this data is also printed when ioctl() is called to set this setting,
         code below left for future use (to debug if needed) */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("   - option mask %d,\n"
          "   - desired output power level -%d dBm,\n"
          "   - data wait time %d ms,\n"
          "   - gain: RX %d, TX %d,\n"
          "   - IFPSI %d,\n",
          pT38Cfg->OptionMask, pT38Cfg->nDbm, pT38Cfg->nDWT,
          pT38Cfg->nGainRx, pT38Cfg->nGainTx, pT38Cfg->nIFPSI));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("   - modulation auto start time %d,\n"
          "   - spoofing insert time %d,\n"
          "   - number FEC calculation packets %d,\n"
          "   - number of recovery packets %d, high speed %d, low speed %d,\n"
          "   - NSX %s (len %d).\n",
          pT38Cfg->nModAutoStartTime, pT38Cfg->nSpoofAutoInsTime,
          pT38Cfg->nPktFec, pT38Cfg->nPktRecovInd,
          pT38Cfg->nPktRecovHiSpeed, pT38Cfg->nPktRecovLoSpeed,
          stTempNsx, pT38Cfg->nNsxLen));
#endif /* 0 */

   return IFX_SUCCESS;
}

/**
   Setup fax data pump configuration(fdp) for given data channel.

   \param  pPhone - pointer to PHONE
   \param  nFaxCh  - FAX channel

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_CfgSet(PHONE_t* pPhone, IFX_int32_t nFaxCh)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t nFd = -1;
   IFX_int32_t i;
   IFX_char_t stTempNsx[TD_MAX_NAME] = {0};
   IFX_TAPI_T38_FAX_CFG_t* pT38Cfg;
   BOARD_t *pUsedBoard;
   IFX_int32_t nTmpVal;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pUsedBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }
   TD_PARAMETER_CHECK(((nFaxCh < 0) || (nFaxCh >= pUsedBoard->nMaxCoderCh)),
                      nFaxCh, IFX_ERROR);

   /* get pointer to structure */
   pT38Cfg = &pUsedBoard->oT38_Config.oT38FaxCfg;

   /* needed only for tapi v4 */
   pT38Cfg->ch = 0;
   pT38Cfg->dev = 0;
#ifdef TAPI_VERSION4
   /* assumption is made that first connection is used */
   if (IFX_ERROR == TD_T38_GetVoipIdChAndDev(pUsedBoard->pCtrl,
                       &pPhone->rgoConn[0], &pT38Cfg->ch, &pT38Cfg->dev,
                       pPhone->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Failed to get coder channel and device. "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   nFd = TD_T38_GetCodFd(pUsedBoard->pCtrl);
#else /* TAPI_VERSION4 */
   nFd = Common_GetFD_OfCh(pUsedBoard, nFaxCh);
#endif /* TAPI_VERSION4 */
   nDevTmp = pT38Cfg->dev;
   TD_PARAMETER_CHECK((nFd == IFX_ERROR), nFd, IFX_ERROR);

   TD_T38_GET_NSX_STRING(stTempNsx, i, pT38Cfg->aNsx, pT38Cfg->nNsxLen, nTmpVal,
                         pPhone->nSeqConnId);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Setting FAX channel configuration:\n",
          pPhone->nPhoneNumber));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("%s- option mask %d,\n"
             "%s- desired output power level -%d dBm,\n"
             "%s- data wait time %d ms,\n"
             "%s- gain: RX %d, TX %d,\n"
             "%s- IFPSI %d,\n",
             pPhone->pBoard->pIndentPhone, pT38Cfg->OptionMask,
             pPhone->pBoard->pIndentPhone, pT38Cfg->nDbm,
             pPhone->pBoard->pIndentPhone, pT38Cfg->nDWT,
             pPhone->pBoard->pIndentPhone, pT38Cfg->nGainRx, pT38Cfg->nGainTx,
             pPhone->pBoard->pIndentPhone, pT38Cfg->nIFPSI));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("%s- modulation auto start time %d,\n"
             "%s- spoofing insert time %d,\n"
             "%s- number FEC calculation packets %d,\n"
             "%s- number of recovery packets %d, high speed %d, low speed %d,\n"
             "%s- NSX %s (len %d).\n",
             pPhone->pBoard->pIndentPhone, pT38Cfg->nModAutoStartTime,
             pPhone->pBoard->pIndentPhone, pT38Cfg->nSpoofAutoInsTime,
             pPhone->pBoard->pIndentPhone, pT38Cfg->nPktFec,
             pPhone->pBoard->pIndentPhone, pT38Cfg->nPktRecovInd,
             pT38Cfg->nPktRecovHiSpeed, pT38Cfg->nPktRecovLoSpeed,
             pPhone->pBoard->pIndentPhone, stTempNsx, pT38Cfg->nNsxLen));

   ret = TD_IOCTL(nFd, IFX_TAPI_T38_CFG_SET, (IFX_int32_t) pT38Cfg,
           nDevTmp, pPhone->nSeqConnId);
   if (IFX_SUCCESS != ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d\n",
          pPhone->nPhoneNumber));
   }
   return ret;
}
#endif /* TD_T38_FAX_TEST */

/**
   Get T.38 session statistics
   \param  pPhone - pointer to PHONE
   \param  nFaxCh  - FAX channel

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_GetStatistics(PHONE_t* pPhone, IFX_int32_t nFaxCh)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_T38_SESS_STATISTICS_t t38SessStat;
   IFX_int32_t nFd = -1;
   BOARD_t *pUsedBoard;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pUsedBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }
   TD_PARAMETER_CHECK(((nFaxCh < 0) || (nFaxCh >= pUsedBoard->nMaxCoderCh)),
                      nFaxCh, IFX_ERROR);

   memset(&t38SessStat, 0, sizeof (IFX_TAPI_T38_SESS_STATISTICS_t));

#ifdef TAPI_VERSION4
   nFd = TD_T38_GetCodFd(pUsedBoard->pCtrl);
#else /* TAPI_VERSION4 */
   nFd = Common_GetFD_OfCh(pUsedBoard, nFaxCh);
#endif /* TAPI_VERSION4 */
   TD_PARAMETER_CHECK((nFd == IFX_ERROR), nFd, IFX_ERROR);

   /* for now only one device is used - needed for TAPI4 */
   t38SessStat.dev = 0;
   t38SessStat.ch = 0;
#ifdef TAPI_VERSION4
   /* assumption is made that first connection is used */
   if (IFX_ERROR == TD_T38_GetVoipIdChAndDev(pPhone->pBoard->pCtrl,
                       &pPhone->rgoConn[0], &t38SessStat.ch, &t38SessStat.dev,
                       pPhone->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Failed to get coder channel and device. "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* TAPI_VERSION4 */
   nDevTmp = t38SessStat.dev;

   ret = TD_IOCTL(nFd, IFX_TAPI_T38_SESS_STATISTICS_GET,
           (IFX_int32_t)&t38SessStat,
           nDevTmp, pPhone->nSeqConnId);
   if (IFX_SUCCESS != ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d\n",
          pPhone->nPhoneNumber));
   }
   TD_RETURN_IF_ERROR(ret);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId, (
         "Phone No %d: Fax T.38 session statistics for ch %d:\n"
         "  - Fax Sessinon State %d (%s)\n"
         "  - T38 Ver : %d.%d\n"
         "  - Flags SessInfo : %d (%s)\n"
         "  - Standard (%s) %0X\n"
         "  - Lost Packets : %d\n"
         "  - Recovered Packets : %d\n",
         pPhone->nPhoneNumber,
         nFaxCh,
         t38SessStat.nFaxSessState,
         Common_Enum2Name(t38SessStat.nFaxSessState, rgSesStateName),
         t38SessStat.nT38VerMajor,
         t38SessStat.nT38VerMin,
         t38SessStat.SessInfo,
         Common_Enum2Name(t38SessStat.SessInfo, rgSesFlagsName),
         Common_Enum2Name(t38SessStat.nFdpStand, rgSesFDPSTDName),
         t38SessStat.nFdpStand,
         t38SessStat.nPktLost,
         t38SessStat.nPktRecov));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId, (
         "  - Max Lost Consecutive Packets : %d\n"
         "  - Number of TCF Respond with FTT :  %d\n"
         "  - Transfered pages quantity : %d\n"
         "  - Number of broken Non-ECM page lines : %d\n"
         "  - Number of broken Modulation of v21 frames : %d\n"
         "  - Number of broken ECM Frames Modulation : %d\n",
         t38SessStat.nPktGroupLost,
         t38SessStat.nFttRsp,
         t38SessStat.nPagesTx,
         t38SessStat.nLineBreak,
         t38SessStat.nV21FrmBreak,
         t38SessStat.nEcmFrmBreak));
   return ret;
} /* TD_T38_GetStatistics */

/**
   Setup environment for Fax T.38: disable all active Fax/Modem signal
   detectors; stop voice codec; switch WLEC=off, NLP=off;
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_SetUp(PHONE_t* pPhone, CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_WLEC_CFG_t tapi_lec_conf;
   IFX_int32_t nPhoneFD = -1;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /*
     - disable all active Fax/Modem signal detectors;
     - stop voice codec;
     - switch WLEC=off, NLP=off;
   */

   /* disable all active Fax/Modem signal detectors */
   ret = SIGNAL_HandlerReset(pPhone, pConn);

   if (IFX_SUCCESS == ret)
   {
      /* stop voice codec */
      ret = VOIP_StopCodec(pConn->nUsedCh, pConn, pPhone->pBoard, pPhone);
   }

   if (IFX_SUCCESS == ret)
   {
      COM_ITM_TRACES(pPhone->pBoard->pCtrl, pPhone, pConn,
                     IFX_NULL, TTP_T_38_CODEC_STOP, pPhone->nSeqConnId);
#ifndef TAPI_VERSION4
      /* Flush the upstream packet fifo on a channel. */
      ret = TD_IOCTL(pConn->nUsedCh_FD, IFX_TAPI_PKT_FLUSH, (IFX_int32_t)0,
              TD_DEV_NOT_SET, pPhone->nSeqConnId);
      if (IFX_SUCCESS != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("     Phone No %d, ch %d,\n",
             pPhone->nPhoneNumber, pConn->nUsedCh));
      }
      TD_RETURN_IF_ERROR(ret);
#endif /* TAPI_VERSION4 */
   }
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      COM_ITM_TRACES(pPhone->pBoard->pCtrl, pPhone, pConn,
                     IFX_NULL, TTP_LEC_DISABLED, pPhone->nSeqConnId);
   }
   else
#endif /* EASY3111 */
   {
      /* switch WLEC=off, NLP=off */
      if (IFX_SUCCESS == ret)
      {
         memset(&tapi_lec_conf, 0, sizeof(IFX_TAPI_WLEC_CFG_t));
         tapi_lec_conf.nType = IFX_TAPI_WLEC_TYPE_OFF;
         tapi_lec_conf.bNlp = IFX_TAPI_WLEC_NLP_OFF;
#ifdef TAPI_VERSION4
         tapi_lec_conf.ch = pPhone->nPhoneCh;
         tapi_lec_conf.dev = pPhone->nDev;
         nDevTmp = tapi_lec_conf.dev;
         if (pPhone->pBoard->fSingleFD)
         {
            nPhoneFD = pPhone->pBoard->nDevCtrl_FD;
         }
#else /* TAPI_VERSION4 */
         nPhoneFD = pPhone->nPhoneCh_FD;
#endif /* TAPI_VERSION4 */
         ret = TD_IOCTL(nPhoneFD, IFX_TAPI_WLEC_PHONE_CFG_SET,
                  (IFX_int32_t) &tapi_lec_conf, nDevTmp, pPhone->nSeqConnId);
         if (IFX_ERROR == ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("   Phone No %d: ioctl failed.\n",
                   pPhone->nPhoneNumber));
         }
         else
         {
            COM_ITM_TRACES(pPhone->pBoard->pCtrl, pPhone, pConn,
                           IFX_NULL, TTP_LEC_DISABLED, pPhone->nSeqConnId);
         }
      } /* if (IFX_SUCCESS == ret) switch WLEC=off, NLP=off */
   } /* if (TYPE_DUSLIC_XT == pPhone->pBoard->nType) */

   return ret;
} /* TD_T38_SetUp */

/**
   Gets Fax T.38 capabilities.

   \param  pBoard - pointer to board

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_GetCapabilities(BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_T38_CAP_t* pT38Cap;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* get structure and reset it */
   pT38Cap = &pBoard->stT38Cap;
   memset(pT38Cap, 0, sizeof (IFX_TAPI_T38_CAP_t));

   /* Read the capabilities of the T.38 stack implementation for the used
   firmware version */
   /* for now only one device is used - needed for TAPI4 */
   pT38Cap->dev = 0;
   pT38Cap->ch = 0;

   ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_T38_CAP_GET, pT38Cap,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   TD_RETURN_IF_ERROR(ret);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("T.38 capabilities for %s:\n"
          "   - FacConvOpt: %d (%s)\n"
          "   - nBitRateMax: %d\n"
          "   - nT38Ver: %d\n"
          "   - nTCPRateManagement: %d (%s)\n"
          "   - nUDPBuffSizeMax: %d\n",
          pBoard->pszBoardName,
          pT38Cap->FacConvOpt,
          Common_Enum2Name(pT38Cap->FacConvOpt, TD_rgT38FacsimileCNVT_Name),
          pT38Cap->nBitRateMax, pT38Cap->nT38Ver,
          pT38Cap->nTCPRateManagement,
          Common_Enum2Name(pT38Cap->nTCPRateManagement, TD_rgT38RMM_Name),
          pT38Cap->nUDPBuffSizeMax));

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("   - nUDPDatagramSizeMax: %d\n"
          "   - nUDPRateManagement: %d (%s)\n"
          "   - Protocol: %d (%s)\n"
          "   - UDPErrCorr: %d (%s)\n",
          pT38Cap->nUDPDatagramSizeMax,
          pT38Cap->nUDPRateManagement,
          Common_Enum2Name(pT38Cap->nUDPRateManagement, TD_rgT38RMM_Name),
          pT38Cap->Protocol,
          Common_Enum2Name(pT38Cap->Protocol, TD_rgT38Protocol_Name),
          pT38Cap->UDPErrCorr,
          Common_Enum2Name(pT38Cap->UDPErrCorr, TD_rgT38EcCaP_Name)));
   return ret;
} /* TD_T38_GetCapabilities */


/**
   Setup structure with default session configuration acording to capabilities.

   \param  pBoard - pointer to board structure
   \param  bGetFromDrivers - if IFX_TRUE then read settings from drivers,
                             if else then use default settings

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_SessCfgSetStruct(BOARD_t* pBoard)
{
   IFX_TAPI_T38_SESS_CFG_t* pT38SessCfg;
   IFX_TAPI_T38_CAP_t *pT38Cap = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* get session configuration structure and reset it */
   pT38SessCfg = &pBoard->oT38_Config.oT38SessCfg;
   memset(pT38SessCfg, 0, sizeof (IFX_TAPI_T38_SESS_CFG_t));

   pT38Cap = &pBoard->stT38Cap;

   /* set session parameters */
   pT38SessCfg->ch = 0;
   /* needed for TAPI4 */
   pT38SessCfg->dev = 0;

   pT38SessCfg->FacConvOpt = pT38Cap->FacConvOpt;
   pT38SessCfg->nBitRateMax = pT38Cap->nBitRateMax;
   if (0 != (pT38Cap->Protocol == IFX_TAPI_T38_UDP))
   {
      pT38SessCfg->nProtocol = IFX_TAPI_T38_UDP;
#ifdef TAPI_VERSION4
      /* for TAPI_V4 invalid value of pT38SessCfg->nRateManagement is returned
         by IFX_TAPI_T38_CAP_GET, using hardcoded value */
      pT38SessCfg->nRateManagement = IFX_TAPI_T38_TRANS_TCF;
#else /* TAPI_VERSION4 */
      if (0 != (pT38Cap->nUDPRateManagement & IFX_TAPI_T38_TRANS_TCF))
      {
         pT38SessCfg->nRateManagement = IFX_TAPI_T38_TRANS_TCF;
      }
      else if (0 != (pT38Cap->nUDPRateManagement & IFX_TAPI_T38_LOC_TCF))
      {
         pT38SessCfg->nRateManagement = IFX_TAPI_T38_LOC_TCF;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("%s: Invalid rate management method for UDP "
                "(nUDPRateManagement = %d). "
                "(File: %s, line: %d)\n",
                pBoard->pszBoardName, pT38Cap->nUDPRateManagement,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
#endif /* TAPI_VERSION4 */
   }
   else if (0!= (pT38Cap->Protocol & IFX_TAPI_T38_TCP))
   {
      pT38SessCfg->nProtocol = IFX_TAPI_T38_TCP;
      if (0 != (pT38Cap->nTCPRateManagement & IFX_TAPI_T38_LOC_TCF))
      {
         pT38SessCfg->nRateManagement = IFX_TAPI_T38_LOC_TCF;
      }
      else if (0 != (pT38Cap->nTCPRateManagement & IFX_TAPI_T38_TRANS_TCF))
      {
         pT38SessCfg->nRateManagement = IFX_TAPI_T38_TRANS_TCF;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("%s: Invalid rate management method for UDP "
                "(nTCPRateManagement = %d). "
                "(File: %s, line: %d)\n",
                pBoard->pszBoardName, pT38Cap->nTCPRateManagement,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("%s: Invalid transport protocol (Protocol = %d). "
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, pT38Cap->Protocol,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   pT38SessCfg->nT38Ver = pT38Cap->nT38Ver;
   pT38SessCfg->nUDPBuffSizeMax = pT38Cap->nUDPBuffSizeMax;
   pT38SessCfg->nUDPDatagramSizeMax = pT38Cap->nUDPDatagramSizeMax;
#ifdef TAPI_VERSION4
   /* for TAPI_V4 invalid value of pT38SessCfg->nUDPErrCorr is returned
      by IFX_TAPI_T38_CAP_GET, using hardcoded value */
   pT38SessCfg->nUDPErrCorr = IFX_TAPI_T38_RED;
#else /* TAPI_VERSION4 */
   if (0 != (pT38Cap->UDPErrCorr & IFX_TAPI_T38_CAP_FEC))
   {
      pT38SessCfg->nUDPErrCorr = IFX_TAPI_T38_FEC;
   }
   else if (0 != (pT38Cap->UDPErrCorr & IFX_TAPI_T38_CAP_RED))
   {
      pT38SessCfg->nUDPErrCorr = IFX_TAPI_T38_RED;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("%s: Invalid UDP error correction method "
             "(UDPErrCorr = %d). (File: %s, line: %d)\n",
             pBoard->pszBoardName, pT38Cap->UDPErrCorr,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* TAPI_VERSION4 */
#if 1
   /* this data is also printed when ioctl() is called to set this setting,
      code below left for future use (to debug if needed) */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("%s: T.38 session configuration:\n"
          "   - FacConvOpt: %d (%s)\n"
          "   - nBitRateMax: %d\n"
          "   - nProtocol: %d (%s)\n"
          "   - nRateManagement: %d (%s)\n"
          "   - nT38Ver: %d\n"
          "   - nUDPBuffSizeMax: %d\n"
          "   - nUDPDatagramSizeMax: %d\n"
          "   - nUDPErrCorr: %d (%s)\n",
          pBoard->pszBoardName,
          pT38SessCfg->FacConvOpt,
          Common_Enum2Name(pT38SessCfg->FacConvOpt, TD_rgT38FacsimileCNVT_Name),
          pT38SessCfg->nBitRateMax,
          pT38SessCfg->nProtocol,
          Common_Enum2Name(pT38SessCfg->nProtocol, TD_rgT38Protocol_Name),
          pT38SessCfg->nRateManagement,
          Common_Enum2Name(pT38SessCfg->nRateManagement, TD_rgT38RMM_Name),
          pT38SessCfg->nT38Ver,
          pT38SessCfg->nUDPBuffSizeMax,
          pT38SessCfg->nUDPDatagramSizeMax,
          pT38SessCfg->nUDPErrCorr,
          Common_Enum2Name(pT38SessCfg->nUDPErrCorr, TD_rgT38EcMode_Name)));
#endif

   return IFX_SUCCESS;
} /* TD_T38_Start */
#ifdef TAPI_VERSION4
/**
   Starts Fax T.38.

   \param  pPhone - pointer to PHONE
   \param  nFaxCh  - FAX channel

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_Start(PHONE_t* pPhone, IFX_int32_t nFaxCh)
{
   IFX_return_t ret = IFX_SUCCESS;
   CTRL_STATUS_t* pCtrl;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard->pCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK(((nFaxCh < 0) || (nFaxCh >= pPhone->pBoard->nMaxCoderCh)),
                      nFaxCh, IFX_ERROR);

   pCtrl = pPhone->pBoard->pCtrl;
#ifdef TD_T38_FAX_TEST
   /* set T.38 configuration */
   TD_T38_FdpCfgSet(pPhone, nFaxCh);
   TD_T38_CfgSet(pPhone, nFaxCh);
#endif
   /* assumption is made that only first connection is used */
   if (IFX_SUCCESS ==  VOIP_FaxStart(pCtrl, &pPhone->rgoConn[0],
                          ABSTRACT_GetBoard(pCtrl, IFX_TAPI_DEV_TYPE_VIN_SUPERVIP),
                          pPhone))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: T.38 session started\n",
             pPhone->nPhoneNumber));

      if (pPhone->rgoConn[0].nUsedCh == nFaxCh)
      {
         COM_ITM_TRACES(pPhone->pBoard->pCtrl, pPhone, &pPhone->rgoConn[0],
                        IFX_NULL, TTP_START_FAXT38_TRANSMISSION,
                        pPhone->nSeqConnId);
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Failed to start T.38 session. "
             "(File: %s, line: %d)\n",
              pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return ret;
} /* TD_T38_Start */

/**
   Stop T.38 session.
   \param  pPhone - pointer to PHONE
   \param  nFaxCh  - FAX channel

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_Stop(PHONE_t* pPhone, IFX_int32_t nFaxCh)
{
   IFX_return_t ret = IFX_SUCCESS;
   CTRL_STATUS_t* pCtrl;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard->pCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK(((nFaxCh < 0) || (nFaxCh >= pPhone->pBoard->nMaxCoderCh)),
                      nFaxCh, IFX_ERROR);

   pCtrl = pPhone->pBoard->pCtrl;

   /* assumption is made that only first connection is used */
   if (IFX_SUCCESS ==  VOIP_FaxStop(pCtrl, &pPhone->rgoConn[0],
                          ABSTRACT_GetBoard(pCtrl, IFX_TAPI_DEV_TYPE_VIN_SUPERVIP),
                          pPhone))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: T.38 session stopped\n",
             pPhone->nPhoneNumber));

      if (pPhone->rgoConn[0].nUsedCh == nFaxCh)
      {
         COM_ITM_TRACES(pPhone->pBoard->pCtrl, pPhone, &pPhone->rgoConn[0],
                        IFX_NULL, TTP_STOP_FAXT38_TRANSMISSION,
                        pPhone->nSeqConnId);
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Failed to stop T.38 session. "
             "(File: %s, line: %d)\n",
              pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return ret;
} /* TD_T38_Stop */
#else /* TAPI_VERSION4 */
/**
   Starts Fax T.38.

   \param  pPhone - pointer to PHONE
   \param  nFaxCh  - FAX channel

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_Start(PHONE_t* pPhone, IFX_int32_t nFaxCh)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_T38_SESS_CFG_t* pT38SessCfg;
   IFX_int32_t nFd = -1;
   BOARD_t *pUsedBoard;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pUsedBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }
   TD_PARAMETER_CHECK(((nFaxCh < 0) || (nFaxCh >= pUsedBoard->nMaxCoderCh)),
                      nFaxCh, IFX_ERROR);

#ifdef TD_T38_FAX_TEST
   /* set T.38 configuration */
   TD_T38_FdpCfgSet(pPhone, nFaxCh);
   TD_T38_CfgSet(pPhone, nFaxCh);
#endif

   pT38SessCfg = &pUsedBoard->oT38_Config.oT38SessCfg;

   pT38SessCfg->ch = 0;
   pT38SessCfg->dev = 0;
   nFd = Common_GetFD_OfCh(pUsedBoard, nFaxCh);
   TD_PARAMETER_CHECK((nFd == IFX_ERROR), nFd, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: T.38 session configuration (fax ch %d):\n"
          "   - FacConvOpt: %d (%s)\n"
          "   - nBitRateMax: %d\n"
          "   - nProtocol: %d (%s)\n"
          "   - nRateManagement: %d (%s)\n"
          "   - nT38Ver: %d\n"
          "   - nUDPBuffSizeMax: %d\n"
          "   - nUDPDatagramSizeMax: %d\n"
          "   - nUDPErrCorr: %d (%s)\n",
          (IFX_int32_t) pPhone->nPhoneNumber,
          nFaxCh,
          pT38SessCfg->FacConvOpt,
          Common_Enum2Name(pT38SessCfg->FacConvOpt, TD_rgT38FacsimileCNVT_Name),
          pT38SessCfg->nBitRateMax,
          pT38SessCfg->nProtocol,
          Common_Enum2Name(pT38SessCfg->nProtocol, TD_rgT38Protocol_Name),
          pT38SessCfg->nRateManagement,
          Common_Enum2Name(pT38SessCfg->nRateManagement, TD_rgT38RMM_Name),
          pT38SessCfg->nT38Ver,
          pT38SessCfg->nUDPBuffSizeMax,
          pT38SessCfg->nUDPDatagramSizeMax,
          pT38SessCfg->nUDPErrCorr,
          Common_Enum2Name(pT38SessCfg->nUDPErrCorr, TD_rgT38EcMode_Name)));

   /* Start session with negotiated T.38 capabilities t38SessCfg */
   ret = TD_IOCTL(nFd, IFX_TAPI_T38_SESS_START, (IFX_int32_t)pT38SessCfg,
            pT38SessCfg->dev, pPhone->nSeqConnId);
   if (IFX_SUCCESS != ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, nFaxCh));
   }
   TD_RETURN_IF_ERROR(ret);
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
         ("Phone No %d: T.38 session started\n",
          pPhone->nPhoneNumber));

   if (pPhone->rgoConn[0].nUsedCh == nFaxCh)
   {
      COM_ITM_TRACES(pPhone->pBoard->pCtrl, pPhone, &pPhone->rgoConn[0],
                     IFX_NULL, TTP_START_FAXT38_TRANSMISSION,
                     pPhone->nSeqConnId);
   }
   return ret;
} /* TD_T38_Start */

/**
   Stop T.38 session.
   \param  pPhone - pointer to PHONE
   \param  nFaxCh  - FAX channel

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_Stop(PHONE_t* pPhone, IFX_int32_t nFaxCh)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_T38_SESS_STOP_t t38SessStop;
   IFX_int32_t nFd = -1;
   BOARD_t *pUsedBoard;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pUsedBoard = TD_GET_MAIN_BOARD(pPhone->pBoard->pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }
   TD_PARAMETER_CHECK(((nFaxCh < 0) || (nFaxCh >= pUsedBoard->nMaxCoderCh)),
                      nFaxCh, IFX_ERROR);
   nFd = Common_GetFD_OfCh(pUsedBoard, nFaxCh);
   TD_PARAMETER_CHECK((nFd == IFX_ERROR), nFd, IFX_ERROR);

   /* for now only one device is used - needed for TAPI4 */
   t38SessStop.dev = 0;
   t38SessStop.ch = 0;

   ret = TD_IOCTL(nFd, IFX_TAPI_T38_SESS_STOP, (IFX_int32_t)&t38SessStop,
            t38SessStop.dev, pPhone->nSeqConnId);
   if (IFX_SUCCESS != ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("     Phone No %d, ch %d,\n", pPhone->nPhoneNumber, nFaxCh));
   }
   TD_RETURN_IF_ERROR(ret);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("Phone No %d: T.38 session stopped\n",
                pPhone->nPhoneNumber));

   if (pPhone->rgoConn[0].nUsedCh == nFaxCh)
   {
      COM_ITM_TRACES(pPhone->pBoard->pCtrl, pPhone, &pPhone->rgoConn[0],
                     IFX_NULL, TTP_STOP_FAXT38_TRANSMISSION,
                     pPhone->nSeqConnId);
   }
   return ret;
} /* TD_T38_Stop */
#endif /* TAPI_VERSION4 */
/**
   Set Fax T.38 configuration structures.

   \param  pBoard - pointer to board structure

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t TD_T38_Init(BOARD_t* pBoard)
{
   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if (IFX_SUCCESS != TD_T38_GetCapabilities(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s: failed to get T.38 capabilities. Disable T.38 support\n"
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, __FILE__, __LINE__));
      pBoard->nT38_Support = IFX_FALSE;
      return IFX_ERROR;
   }
   if (IFX_SUCCESS != TD_T38_SessCfgSetStruct(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s: failed to set session configuration."
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#ifdef TD_T38_FAX_TEST
   if (IFX_SUCCESS != TD_T38_FdpCfgSetStruct(pBoard, IFX_FALSE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s: failed to set fdp configuration."
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (IFX_SUCCESS != TD_T38_CfgSetStruct(pBoard,
                         pBoard->oT38_Config.oT38SessCfg.nUDPErrCorr, IFX_FALSE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s: failed to set fdp configuration."
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* TD_T38_FAX_TEST */
   return IFX_SUCCESS;
} /* TD_T38_GetCapabilities */

#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

