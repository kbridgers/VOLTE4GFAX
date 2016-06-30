/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : cid.c
   Date        : 2005-11-29
   Description : This file contains the implementation of the functions for
                 the tapi demo working with Caller ID.
   \file

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "cid.h"

/* ============================= */
/* Local structures              */
/* ============================= */

/** Structure holding CID for one user (phone number, date, user name) */
typedef struct _CID_USER_t
{
   IFX_uint32_t nMonth;
   IFX_uint32_t nDay;
   IFX_uint32_t nHour;
   IFX_uint32_t nMn;
   IFX_char_t sName[IFX_TAPI_CID_MSG_LEN_MAX];
   IFX_char_t sNumber[IFX_TAPI_CID_MSG_LEN_MAX];
} CID_USER_t;

/* ============================= */
/* Global Structures             */
/* ============================= */


/* ============================= */
/* Global function declaration   */
/* ============================= */


/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Current CID standard to use */
static IFX_TAPI_CID_STD_t eCurrCID_Std = IFX_TAPI_CID_STD_ETSI_FSK;

/** Predefined CID data for 4 users - not used */
const static CID_USER_t CID_USERS[CID_MAX_USERS] =
{
   {11, 3, 10, 44, "Lisa\0", "12345\0"},
   {11, 3, 10, 44, "Homer\0", "67890\0"},
   {11, 3, 10, 44, "March\0", "33333\0"},
   {11, 3, 10, 44, "Bart\0", "44444\0"}
};

/** names of IFX_TAPI_CID_STD_t */
TD_ENUM_2_NAME_t TD_rgCidStandardName[] =
{
   {IFX_TAPI_CID_STD_TELCORDIA, "TELCORDIA, Belcore, USA"},
   {IFX_TAPI_CID_STD_ETSI_FSK, "ETSI FSK, Europe"},
   {IFX_TAPI_CID_STD_ETSI_DTMF, "ETSI DTMF, Europe"},
   {IFX_TAPI_CID_STD_SIN, "SIN BT, Great Britain"},
   {IFX_TAPI_CID_STD_NTT, "NTT, Japan"},
#ifndef TAPI_VERSION4
   {IFX_TAPI_CID_STD_KPN_DTMF, "KPN DTMF, Denmark"},
   {IFX_TAPI_CID_STD_KPN_DTMF_FSK, "KPN DTMF+FSK, Denmark"},
#endif /* TAPI_VERSION4 */
   {TD_MAX_ENUM_ID, "IFX_TAPI_CID_STD_t"}
};
/** short names of IFX_TAPI_CID_STD_t */
TD_ENUM_2_NAME_t TD_rgCidStandardNameShort[] =
{
   {IFX_TAPI_CID_STD_TELCORDIA, "TELCORDIA"},
   {IFX_TAPI_CID_STD_ETSI_FSK, "ETSI FSK"},
   {IFX_TAPI_CID_STD_ETSI_DTMF, "ETSI DTMF"},
   {IFX_TAPI_CID_STD_SIN, "SIN BT"},
   {IFX_TAPI_CID_STD_NTT, "NTT"},
#ifndef TAPI_VERSION4
   {IFX_TAPI_CID_STD_KPN_DTMF, "KPN DTMF"},
   {IFX_TAPI_CID_STD_KPN_DTMF_FSK, "KPN DTMF+FSK"},
#endif /* TAPI_VERSION4 */
   {TD_MAX_ENUM_ID, "IFX_TAPI_CID_STD_t"}
};
/* ============================= */
/* Local function definition     */
/* ============================= */


/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   Prepare CID message.

   \param  nIdx - cid user index, by default its the same as channel number
   \param  pPhone - pointer to PHONE

   \return status (IFX_SUCCESS or IFX_ERROR)

   \remark
      This test assumes straights connections between phone and data channel
*/
IFX_return_t CID_SetupData(IFX_int32_t nIdx,
                           PHONE_t* pPhone)
{
   IFX_uint8_t element = 0;
   struct tm* date_time = IFX_NULL;
   time_t time_in_sec;
   char temp_string[IFX_TAPI_CID_MSG_LEN_MAX];

   /** \todo CID_SetupData() according to CID types we have different
             structure of CID messages */
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nIdx), nIdx, IFX_ERROR);

   /* allocate memory for message structure */
   if (IFX_NULL == pPhone->pCID_Msg)
   {
      pPhone->pCID_Msg = TD_OS_MemCalloc(1, sizeof(IFX_TAPI_CID_MSG_t));
      if (IFX_NULL == pPhone->pCID_Msg)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, TD_OS_MemCalloc for pPhone->pCID_Msg failed. "
                "(File: %s, line: %d)\n",
                 __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   /* allocate memory for data */
   if (IFX_NULL == pPhone->pCID_Msg->message)
   {
      pPhone->pCID_Msg->message =
         TD_OS_MemCalloc(TD_CID_ELEM_COUNT,sizeof(IFX_TAPI_CID_MSG_ELEMENT_t));
      if (IFX_NULL == pPhone->pCID_Msg->message)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, TD_OS_MemCalloc for pPhone->pCID_Msg->message failed. "
                "(File: %s, line: %d)\n",
                 __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   memset(&pPhone->pCID_Msg->message[0], 0, sizeof(pPhone->pCID_Msg->message));

   /* current calendar time in seconds */
   time_in_sec = time(IFX_NULL);

/*   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("%s%ju secs since the Epoch %d\n",
           asctime(localtime(&time_in_sec)),
                   (uintmax_t)time_in_sec));*/

   /* converts calendar time into broke-down time */
   date_time = localtime(&time_in_sec);

/*   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
          ("day %d, month %d, year %d, hour %d, min %d\n",
           date_time->tm_mday, date_time->tm_mon, date_time->tm_year,
           date_time->tm_hour, date_time->tm_min);*/


   /*if ((0 != CID_USERS[nIdx].nMonth) && (0 != CID_USERS[nIdx].nDay))*/
   if ((TD_CID_ELEM_COUNT > element) && (IFX_NULL != date_time))
   {
      pPhone->pCID_Msg->message[element].date.elementType =
         IFX_TAPI_CID_ST_DATE;

      /* Only 1 - 12 allowed */
      pPhone->pCID_Msg->message[element].date.month =
         ((date_time->tm_mon + 1) % 13);

      /* Only 1 - 31 allowed */
      pPhone->pCID_Msg->message[element].date.day = (date_time->tm_mday % 32);
      /* Only 0 - 23 allowed */
      pPhone->pCID_Msg->message[element].date.hour = (date_time->tm_hour % 24);
      /* Only 0 - 59 allowed */
      pPhone->pCID_Msg->message[element].date.mn = (date_time->tm_min % 60);
      element++;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, Setup date/time element in CID.\n"));
   }

   /* TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Setup number CLI element in CID.\n")); */

   /*if (IFX_NULL != CID_USERS[nIdx].sNumber)*/
   if (TD_CID_ELEM_COUNT > element)
   {
      pPhone->pCID_Msg->message[element].string.elementType =
         IFX_TAPI_CID_ST_CLI;

      /*snprintf(temp_string, IFX_TAPI_CID_MSG_LEN_MAX,
               "%d", pPhone->nPhoneNumber);*/

      sprintf(temp_string, "%d", (int) pPhone->nPhoneNumber);

      pPhone->pCID_Msg->message[element].string.len =
         strlen(temp_string);

      strncpy((char*)pPhone->pCID_Msg->message[element].string.element,
              temp_string,
              sizeof(pPhone->pCID_Msg->message[element].string.element));
      element++;
   }

   /*if (IFX_NULL != CID_USERS[nIdx].sName)*/
   if (TD_CID_ELEM_COUNT > element)
   {
      pPhone->pCID_Msg->message[element].string.elementType =
         IFX_TAPI_CID_ST_NAME;

      /*snprintf(temp_string, IFX_TAPI_CID_MSG_LEN_MAX,
               "%s#%d", pPhone->pBoard->pszBoardName, pPhone->nPhoneCh);*/
#ifndef TAPI_VERSION4
#ifdef TD_DECT
      if (PHONE_TYPE_DECT == pPhone->nType)
      {
         snprintf(temp_string, IFX_TAPI_CID_MSG_LEN_MAX, "%s#DECT%d",
                 pPhone->pBoard->pszBoardName, (int) pPhone->nDectCh);
      }
      else
#endif /* TD_DECT */
      {
         snprintf(temp_string, IFX_TAPI_CID_MSG_LEN_MAX, "%s#%d",
                 pPhone->pBoard->pszBoardName, (int) pPhone->nPhoneCh);
      }
#else /* TAPI_VERSION4 */
      snprintf(temp_string, IFX_TAPI_CID_MSG_LEN_MAX, "%s#%d,%d",
               pPhone->pBoard->pszBoardName,
               pPhone->nDev,
               pPhone->nPhoneCh);
#endif /* TAPI_VERSION4 */

      pPhone->pCID_Msg->message[element].string.len =
         strlen(temp_string);

      strncpy((char*)pPhone->pCID_Msg->message[element].string.element,
              temp_string,
              sizeof(pPhone->pCID_Msg->message[element].string.element));
      element++;
   }

   /* Put these elements in the cid message */
   if (0 < element)
   {
      pPhone->pCID_Msg->nMsgElements = element;
      return IFX_SUCCESS;
   }

   TD_TRACE (TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
          ("Err, Phone No %d: Setup CID - failed.\n(File: %s, line: %d)\n",
           pPhone->nPhoneNumber, __FILE__, __LINE__));

   return IFX_ERROR;
} /* CidSetupData() */


/**
   If nMode == CID_DISPLAY_ALL:
   Displays cid structure, which has 3 elements:
   1.) date,
   2.) CLI number
   3.) name
   If nMode == CID_DISPLAY_SEND:
   Displays:
     -sending phone number,
     -cid standard name,
     -CLI number and name

   \param pPhone     - pointer to PHONE
   \param pCID_Msg   - pointer CID message
   \param nMode      - display mode

   \return output on screen
*/
IFX_void_t CID_Display(PHONE_t* pPhone, IFX_TAPI_CID_MSG_t *pCID_Msg,
                       CID_DISPLAY_MODE_t nMode)
{
   /* check input arguments */
   TD_PTR_CHECK(pPhone,);
   TD_PTR_CHECK(pCID_Msg,);

   if ((0 > eCurrCID_Std) || (CID_MAX_STANDARDS <= eCurrCID_Std))
   {
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid CID standard %d. (File: %s, line: %d)\n",
             eCurrCID_Std, __FILE__, __LINE__));
      return;
   }
   switch (nMode)
   {
      case CID_DISPLAY_ALL:
         SEPARATE(pPhone->nSeqConnId);
         /*TAPIDEMO_PRINTF(pPhone->nSeqConnId,
          ("There are %d usefull CIDs\n", MAX_SYS_LINE_CH));*/
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("Element count %d\n", pCID_Msg->nMsgElements));
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("Day: %d, Month: %d, Hour: %d, Minutes: %d\n",
                pCID_Msg->message[TD_CID_IDX_DATE].date.day,
                pCID_Msg->message[TD_CID_IDX_DATE].date.month,
                pCID_Msg->message[TD_CID_IDX_DATE].date.hour,
                pCID_Msg->message[TD_CID_IDX_DATE].date.mn));

         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("CLI number is %s\n",
                pCID_Msg->message[TD_CID_IDX_CLI_NUM].string.element));

         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("Name is %s\n",
                pCID_Msg->message[TD_CID_IDX_NAME].string.element));
        SEPARATE(pPhone->nSeqConnId);
         break;
      case CID_DISPALY_SEND:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("Phone No %d: Sending CID (%s) data [%s, %s]\n",
                pPhone->nPhoneNumber,
                Common_Enum2Name(eCurrCID_Std,
                                 TD_rgCidStandardNameShort),
                pCID_Msg->message[TD_CID_IDX_NAME].string.element,
                pCID_Msg->message[TD_CID_IDX_CLI_NUM].string.element));
         break;
      case CID_DISPLAY_SIMPLE:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("Phone No %d: Set CID (%s) data [%s, %s]\n",
                pPhone->nPhoneNumber,
                Common_Enum2Name(eCurrCID_Std,
                                 TD_rgCidStandardNameShort),
                pCID_Msg->message[TD_CID_IDX_NAME].string.element,
                pCID_Msg->message[TD_CID_IDX_CLI_NUM].string.element));
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Wrong CID_Display mode %d\n", nMode));
         break;
   }

} /* Show_CID_Number() */


/**
   Set CID standard to use

   \param nCID_Standard - CID standard number

   \return IFX_SUCCESS if OK, IFX_ERROR if errors
*/
IFX_return_t CID_SetStandard(IFX_int32_t nCID_Standard, IFX_uint32_t nSeqConnId)
{
   /* set default value */
   IFX_TAPI_CID_STD_t cid_standard = eCurrCID_Std;

   if ((0 > nCID_Standard) || (CID_MAX_STANDARDS <= nCID_Standard))
   {
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Invalid input argument(s), will use default %s CID standard. "
             "(File: %s, line: %d)\n",
             Common_Enum2Name(cid_standard, TD_rgCidStandardName),
             __FILE__, __LINE__));
      /* Don't send error, but use default value, ETSI-FSK (Europe). */
   }
   else
   {
      cid_standard = (IFX_TAPI_CID_STD_t) (nCID_Standard);
   }

   eCurrCID_Std = cid_standard;

   return IFX_SUCCESS;
} /* CID_GetStandard() */


/**
   Return CID standard to that is used - this function should be used to read
   CID settings.

   \return CID standard
*/
IFX_TAPI_CID_STD_t CID_GetStandard(IFX_void_t)
{
   /* set standard */
   return eCurrCID_Std;
}


/**
   Configures driver for CID services

   \param pPhone - pointer to phone

   \return IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_ConfDriver(PHONE_t* pPhone)
{
   IFX_TAPI_CID_CFG_t cid_cfg;
   IFX_TAPI_CID_STD_TYPE_t oCID_Std;
   IFX_TAPI_CID_FSK_CFG_t  oFSKConf;
   IFX_TAPI_CID_DTMF_CFG_t  oDTMF_Conf;
   IFX_TAPI_CID_ABS_REASON_t oAbs = {0};
   IFX_return_t ret;
   IFX_int32_t nCh_FD;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* cid not used for DECT phones */
      return IFX_SUCCESS;
   }
#endif /* TD_DECT */
   if (pPhone->pBoard->nMaxCoderCh > 0)
   {
      nCh_FD = pPhone->nDataCh_FD;
   }
   else
   {
      nCh_FD = pPhone->nPhoneCh_FD;
   }
   TD_PARAMETER_CHECK((0 > nCh_FD), nCh_FD, IFX_ERROR);

   memset(&cid_cfg, 0, sizeof(cid_cfg));
   cid_cfg.nStandard = eCurrCID_Std;
   memset(&oCID_Std, 0, sizeof(IFX_TAPI_CID_STD_TYPE_t));

   if (IFX_TAPI_CID_STD_ETSI_FSK == cid_cfg.nStandard)
   {
      memset(&oFSKConf, 0, sizeof(IFX_TAPI_CID_FSK_CFG_t));
      /* add CID configuration */
      cid_cfg.cfg = &oCID_Std;
      /* set parameters */
      oCID_Std.etsiFSK.ackTone = 0;
      oCID_Std.etsiFSK.nAlertToneOffhook = 0;
      oCID_Std.etsiFSK.nAlertToneOnhook = 0;
      oCID_Std.etsiFSK.nETSIAlertNoRing = IFX_TAPI_CID_ALERT_ETSI_RP;
      oCID_Std.etsiFSK.nETSIAlertRing = IFX_TAPI_CID_ALERT_ETSI_RP;
      oCID_Std.etsiFSK.ringPulseTime = 500;
      /* add CID configuration */
      oCID_Std.etsiFSK.pFSKConf = &oFSKConf;
      /* set gains */
      oFSKConf.levelRX = -320;
      oFSKConf.levelTX = -100;
      oFSKConf.markRXOffhook = 55;
      oFSKConf.markRXOnhook = 150;
      oFSKConf.markTXOffhook = 80;
      oFSKConf.markTXOnhook = 180;
      oFSKConf.seizureRX = 200;
      oFSKConf.seizureTX = 300;
   }
   else if (IFX_TAPI_CID_STD_ETSI_DTMF == cid_cfg.nStandard)
   {
      if (IFX_TRUE == g_pITM_Ctrl->fITM_Enabled)
      {
         /* For test purpose start tone digit must be changed,
            testing enviroment accepts only digit 'D' as start tone,
            by default TAPI uses digit 'A',
            rest of the settings should be the same as TAPI default */
         cid_cfg.cfg = &oCID_Std;

         oCID_Std.etsiDTMF.pDTMFConf = &oDTMF_Conf;
         /* ITM uses 'D' as start digit */
         /* default 'A' in documantation standards accept 'A' and 'D' */
         oDTMF_Conf.startTone = 'D';
         oDTMF_Conf.stopTone = 'C'; /* default 'C' */
         oDTMF_Conf.infoStartTone = 'B'; /* default 'B' */
         oDTMF_Conf.redirStartTone = 'D'; /* default 'D' */
         oDTMF_Conf.digitTime = 70; /* default 50 */
         oDTMF_Conf.interDigitTime = 70; /* default 50 */

         oCID_Std.etsiDTMF.pABSCLICode = &oAbs;
         oAbs.len = 2;
         oAbs.unavailable[0] = '0';
         oAbs.unavailable[1] = '0';
         oAbs.priv[0] = '0';
         oAbs.priv[1] = '1';

         oCID_Std.etsiDTMF.nETSIAlertRing = IFX_TAPI_CID_ALERT_ETSI_FR;
         oCID_Std.etsiDTMF.nETSIAlertNoRing = IFX_TAPI_CID_ALERT_ETSI_RP;
         oCID_Std.etsiDTMF.nAlertToneOnhook = 0;
         oCID_Std.etsiDTMF.nAlertToneOffhook = 0;
         oCID_Std.etsiDTMF.ringPulseTime = 500;
         oCID_Std.etsiDTMF.ackTone = 'D';
      }
   }
#ifdef TAPI_VERSION4
   cid_cfg.dev = pPhone->nDev;
   cid_cfg.ch = pPhone->nPhoneCh;
   nDevTmp = cid_cfg.dev;
#endif /* TAPI_VERSION4 */

   ret= TD_IOCTL(nCh_FD, IFX_TAPI_CID_CFG_SET,
                 (IFX_int32_t) &cid_cfg, nDevTmp, pPhone->nSeqConnId);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, CID configuration set failed for this channel. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* CID_ConfDriver() */


/**
   Release CID.

   \param pCID_Msg - CID message to be cleaned up

   \return IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_Release(IFX_TAPI_CID_MSG_t* pCID_Msg)
{
   TD_PTR_CHECK(pCID_Msg, IFX_ERROR);

   if (pCID_Msg->message->transparent.data != IFX_NULL)
   {
      TD_OS_MemFree(pCID_Msg->message->transparent.data);
      pCID_Msg->message->transparent.data = IFX_NULL;
   }
   return IFX_SUCCESS;
} /* CID_Release() */


/**
   Sends a caller id sequence according to hook state, standard, ...

   \param nDataCh_FD - file descriptor of data channel, for Duslic-xT this is
                       phone channel file descriptor
   \param pDstPhone  - pointer to phone structure - destination phone
   \param nHookState - state of hook ONHOOK or OFFHOOK
   \param pCID_Msg   - pointer to CID message
   \param pOrgPhone  - pointer to phone structure - orginator phone
                       (not always available)

   \return IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_Send(IFX_uint32_t nDataCh_FD, PHONE_t* pDstPhone,
                      IFX_TAPI_CID_HOOK_MODE_t nHookState,
                      IFX_TAPI_CID_MSG_t *pCID_Msg, PHONE_t* pOrgPhone)
{
   IFX_int32_t i;
#ifdef TAPI_VERSION4
   IFX_TAPI_RING_t ring;
#endif
#if (defined(EASY336) || defined(XT16))
   IFX_boolean_t bCID_AS;
#endif
   IFX_return_t ret;
   struct tm* date_time = IFX_NULL;
   time_t time_in_sec;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_uint32_t nRing = 0;

   /* check input parameters */
   TD_PTR_CHECK(pDstPhone, IFX_ERROR);
   TD_PTR_CHECK(pCID_Msg, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pDstPhone->nType)
   {
      /* cid not used for DECT phones */
      return IFX_SUCCESS;
   }
#endif /* TD_DECT */

   /* check if custom CID for test purpose is set */
   COM_MOD_CID_DATA_SET(pCID_Msg, pDstPhone);

   /* Stop ringing before any cid transmission. */
#ifdef TAPI_VERSION4
   ring.ch = pDstPhone->nPhoneCh;
   ring.dev = pDstPhone->nDev;
   nDevTmp = ring.dev;
   nRing = (IFX_uint32_t)&ring;
#endif
   ret = TD_IOCTL(pDstPhone->nPhoneCh_FD, IFX_TAPI_RING_STOP, nRing,
                  nDevTmp, pDstPhone->nSeqConnId);

   COM_ITM_TRACES(pDstPhone->pBoard->pCtrl, pDstPhone, IFX_NULL,
                  (IFX_void_t*)ret, TTP_CID_RING_STOP, pDstPhone->nSeqConnId);

   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH,  pDstPhone->nSeqConnId,
            ("Err, CID: ring stop!\n"));
      return IFX_ERROR;
   }
   /* for all message elements */
   for (i=0; i< pCID_Msg->nMsgElements; i++)
   {
      /* check if sending time */
      if (IFX_TAPI_CID_ST_DATE ==
          pCID_Msg->message[i].date.elementType)
      {
         /* current calendar time in seconds */
         time_in_sec = time(IFX_NULL);
         /* converts calendar time into broke-down time */
         date_time = localtime(&time_in_sec);
         /* set current time */
         if (IFX_NULL != date_time)
         {
            /* Only 1 - 12 allowed */
            pCID_Msg->message[i].date.month = ((date_time->tm_mon + 1) % 13);
            /* Only 1 - 31 allowed */
            pCID_Msg->message[i].date.day = (date_time->tm_mday % 32);
            /* Only 0 - 23 allowed */
            pCID_Msg->message[i].date.hour = (date_time->tm_hour % 24);
            /* Only 0 - 59 allowed */
            pCID_Msg->message[i].date.mn = (date_time->tm_min % 60);
         } /* if (IFX_NULL != date_time) */
      } /* check if sending time */
   } /* for all message elements */
   /* Message type is call setup. */
   pCID_Msg->messageType = IFX_TAPI_CID_MT_CSUP;
   if (IFX_TAPI_CID_HM_OFFHOOK == nHookState)
   {
      /* Offhook cid */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW,  pDstPhone->nSeqConnId,
            ("CID: offhook tx\n"));
      pCID_Msg->txMode = IFX_TAPI_CID_HM_OFFHOOK;
   }
   else if (IFX_TAPI_CID_HM_ONHOOK == nHookState)
   {
      /* Onhook cid */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW,  pDstPhone->nSeqConnId,
            ("CID: onhook tx\n"));
      pCID_Msg->txMode = IFX_TAPI_CID_HM_ONHOOK;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pDstPhone->nSeqConnId,
            ("Err, CID: Using wrong nHookState parameter!(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   COM_ITM_TRACES(pDstPhone->pBoard->pCtrl, pDstPhone, IFX_NULL,
                  pCID_Msg, TTP_CID_MSG_TO_SEND, pDstPhone->nSeqConnId);

#if (defined(EASY336) || defined(XT16))
   if ((pCID_Msg->txMode == IFX_TAPI_CID_HM_ONHOOK &&
      CID_GetStandard() == IFX_TAPI_CID_STD_NTT) ||
      (pCID_Msg->txMode == IFX_TAPI_CID_HM_ONHOOK &&
      CID_GetStandard() != IFX_TAPI_CID_STD_NTT))
      bCID_AS = IFX_TRUE;
   else
      bCID_AS = IFX_FALSE;

   ret = Common_CID_Enable(pDstPhone, bCID_AS);
   if (ret == IFX_ERROR)
   {
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pDstPhone->nSeqConnId,
        ("Err, Common_CID_Enable failed. (File: %s, line: %d)\n",
         __FILE__, __LINE__));
      return IFX_ERROR;
   }
#else /* EASY336 || XT16 */
   COM_ITM_TRACES(pDstPhone->pBoard->pCtrl, pDstPhone, IFX_NULL,
                  IFX_SUCCESS, TTP_CID_ENABLE, pDstPhone->nSeqConnId);
#endif /* EASY336 || XT16 */

   /* This interfaces transmits CID message with handling of the complete
      sequence (alert, ack and ringing). */
#ifdef TAPI_VERSION4
   pCID_Msg->dev = pDstPhone->nDev;
   pCID_Msg->ch = pDstPhone->nPhoneCh;
   nDevTmp = pCID_Msg->dev;
#endif /* TAPI_VERSION4 */
#if 1
   /* Transmit the caller id */
   ret = TD_IOCTL(nDataCh_FD, IFX_TAPI_CID_TX_SEQ_START,
                 (IFX_int32_t) pCID_Msg, nDevTmp, pDstPhone->nSeqConnId);

   COM_ITM_TRACES(pDstPhone->pBoard->pCtrl, pDstPhone, IFX_NULL,
                  (IFX_void_t*)ret, TTP_CID_TX_SEQ_START, pDstPhone->nSeqConnId);

   if (IFX_ERROR == ret)
   {
      return IFX_ERROR;
   }
#else /* #if 1 */
   /* At the moment only SEQ START is used. */

   /* Otherwise, if only the FSK transmission is required */
   /* This interfaces transmits CID message. The message data and type
      information are given by IFX_TAPI_CID_MSG_t. Only the pure sending
      of FSK or DTMF data is done, no alert or ringing. */
   if (IFX_ERROR == TD_IOCTL(nDataCh_FD, IFX_TAPI_CID_TX_INFO_START,
                       (IFX_int32_t) pCID_Msg, nDevTmp, pDstPhone->nSeqConnId))
   {
      return IFX_ERROR;
   }
#endif /* #if 1 */
   if (IFX_NULL != pOrgPhone)
   {
      CID_Display(pOrgPhone, pCID_Msg, CID_DISPALY_SEND);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pDstPhone->nSeqConnId,
            ("Phone No %d: Playing CID\n",
             pDstPhone->nPhoneNumber));
      for (i=0; i<pCID_Msg->nMsgElements; i++)
      {
         switch (pCID_Msg->message[i].string.elementType)
         {
         case IFX_TAPI_CID_ST_DATE:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pDstPhone->nSeqConnId,
                  ("%sdate: m %d, d %d, h %d, mn %d\n",
                   pDstPhone->pBoard->pIndentPhone,
                   pCID_Msg->message[i].date.month,
                   pCID_Msg->message[i].date.day,
                   pCID_Msg->message[i].date.hour,
                   pCID_Msg->message[i].date.mn));
            break;
         case IFX_TAPI_CID_ST_CLI:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pDstPhone->nSeqConnId,
                  ("%snumber: %s\n",
                   pDstPhone->pBoard->pIndentPhone,
                   pCID_Msg->message[i].string.element));
            break;
         case IFX_TAPI_CID_ST_NAME:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pDstPhone->nSeqConnId,
                  ("%sname: %s\n",
                   pDstPhone->pBoard->pIndentPhone,
                   pCID_Msg->message[i].string.element));
            break;
         default:
            /* do nothing */
            break;
         }
      }
   }

   return IFX_SUCCESS;
} /* CID_Send() */


/**
   Start CID RX FSK receiver.

   \param pFXO - pointer to fxo
   \param nFD - data channel file descriptor
   \param eHookMode - hook mode

   \return IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_StartFSK(FXO_t* pFXO, IFX_int32_t nFD,
                          IFX_TAPI_CID_HOOK_MODE_t eHookMode)
{
   IFX_TAPI_CID_CFG_t cid_std;
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
#endif /* TAPI_VERSION4 */

   /* check input parameters */
   TD_PARAMETER_CHECK((nFD <= 0), nFD, IFX_ERROR);

   memset(&cid_std, 0, sizeof(cid_std));
   cid_std.nStandard = eCurrCID_Std;

#ifdef TAPI_VERSION4
   nDevTmp = cid_std.dev;

   ret = TD_IOCTL(nFD, IFX_TAPI_CID_CFG_SET, (IFX_int32_t) &cid_std,
            nDevTmp, pFXO->nSeqConnId);
#endif /* TAPI_VERSION4 */

   if (IFX_SUCCESS != ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, Could not configure CID for data channel %d."
          " (File: %s, line: %d)\n",
          nFD, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Start CID RX Engine */
   return TD_IOCTL(nFD, IFX_TAPI_CID_RX_START, (IFX_int32_t) eHookMode,
                   TD_DEV_NOT_SET, pFXO->nSeqConnId);
} /* CID_StartFSK() */


/**
   Stop CID RX FSK receiver.

   \param pFXO - pointer to fxo
   \param nFD - data channel file descriptor
   \param nDataCh - data channel number

   \return IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_StopFSK(FXO_t* pFXO, IFX_int32_t nFD, IFX_int32_t nDataCh)
{
   /* check input parameters */
   TD_PARAMETER_CHECK((nFD <= 0), nFD, IFX_ERROR);

   /* Stop CID RX Engine */
   if (IFX_SUCCESS == TD_IOCTL(nFD, IFX_TAPI_CID_RX_STOP, (IFX_int32_t) 0,
                               TD_DEV_NOT_SET, pFXO->nSeqConnId))
   {
      COM_ITM_TRACES(IFX_NULL, IFX_NULL, IFX_NULL, (IFX_void_t*) nDataCh,
                     TTP_CID_FSK_RX_STOP, pFXO->nSeqConnId);
      return IFX_SUCCESS;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO NO %d: CID_StopFSK() failed for data ch %d (fd %d).\n",
             pFXO->nFXO_Number, nDataCh, nFD));
      return IFX_ERROR;
   }
} /* CID_StopFSK() */


/**
   Prepare Transparent CID message

   \param pFXO - pointer to fxo
   \param pCID_Msg - CID message to be prepared
   \param pData - received data which was readed with FSK
   \param nLen - length of received data in bytes
   \param eHookMode - hook mode

   \return IFX_SUCCESS Transparent CID message created.
*/
IFX_return_t CID_PrepareTransparentMsg(FXO_t* pFXO, IFX_TAPI_CID_MSG_t* pCID_Msg,
                                       IFX_char_t* pData,
                                       IFX_int32_t nLen,
                                       IFX_TAPI_CID_HOOK_MODE_t eHookMode)
{
   /* check input parameters */
   TD_PTR_CHECK(pCID_Msg, IFX_ERROR);
   TD_PTR_CHECK(pData, IFX_ERROR);
   TD_PARAMETER_CHECK((nLen <= 0), nLen, IFX_ERROR);
   TD_PARAMETER_CHECK((nLen > IFX_TAPI_CID_TX_SIZE_MAX), nLen, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
         ("Prepare transparent CID msg\n"));

   pCID_Msg->message->transparent.elementType = IFX_TAPI_CID_ST_TRANSPARENT;
   pCID_Msg->message->transparent.len = nLen;
   if (pCID_Msg->message->transparent.data != IFX_NULL)
   {
      TD_OS_MemFree(pCID_Msg->message->transparent.data);
   }

   pCID_Msg->message->transparent.data =
      TD_OS_MemCalloc(nLen + 1, sizeof(IFX_char_t));

   if(IFX_NULL == pCID_Msg->message->transparent.data)
   {
       /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
           ("Err, Memory not allocated for CID message element. "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memcpy(pCID_Msg->message->transparent.data, pData, nLen);

   pCID_Msg->txMode = eHookMode;
   pCID_Msg->messageType = IFX_TAPI_CID_MT_CSUP;
   pCID_Msg->nMsgElements = 1;


   return IFX_SUCCESS;
}


/**
   Read received CID data.

   \param pFXO - pointer to fxo
   \param nDataCh - data channel number
   \param nFD - data channel file descriptor
   \param pCID_Msg - CID message
   \param nLength - length of retrieved data which will be read

   \return IFX_SUCCESS, data was read with CID RX FSK and transparent CID
           message was prepared, otherwise IFX_ERROR
*/
IFX_return_t CID_ReadData(FXO_t* pFXO, IFX_int32_t nDataCh, IFX_int32_t nFD,
                          IFX_TAPI_CID_MSG_t* pCID_Msg, IFX_int32_t nLength)
{
   IFX_return_t ret = IFX_ERROR;
   IFX_TAPI_CID_RX_DATA_t oCID_RX_Data;
   IFX_int32_t i = 0;
   IFX_char_t* received_data = IFX_NULL;
   IFX_int32_t rcv_data_size = 0;
   IFX_TAPI_CID_RX_STATUS_t oCID_Status = {0};

   /* check input parameters */
   TD_PTR_CHECK(pCID_Msg, IFX_ERROR);
   TD_PARAMETER_CHECK((nDataCh < 0), nDataCh, IFX_ERROR);
   TD_PARAMETER_CHECK((nFD <= 0), nFD, IFX_ERROR);

   memset (&oCID_Status, 0, sizeof (IFX_TAPI_CID_RX_STATUS_t));
   memset (&oCID_RX_Data, 0, sizeof (IFX_TAPI_CID_RX_DATA_t));

   ret = TD_IOCTL(nFD, IFX_TAPI_CID_RX_STATUS_GET, (IFX_int32_t) &oCID_Status,
                     TD_DEV_NOT_SET, pFXO->nSeqConnId);

   if ((ret != IFX_SUCCESS)
       || (oCID_Status.nError != IFX_TAPI_CID_RX_ERROR_NONE)/*
       || (oCID_Status.nStatus != IFX_TAPI_CID_RX_STATE_DATA_READY)*/)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
           ("Err, on IFX_TAPI_CID_RX_STATUS_GET. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   COM_ITM_TRACES(IFX_NULL, IFX_NULL, IFX_NULL,
                  (IFX_void_t*)nDataCh, TTP_CID_FSK_RX_STATUS_GET,
                  pFXO->nSeqConnId);

   received_data = TD_OS_MemCalloc(nLength, 1);
   TD_PTR_CHECK(received_data, IFX_ERROR);
   rcv_data_size = 0;

   do
   {
      ret = TD_IOCTL(nFD, IFX_TAPI_CID_RX_DATA_GET,
               (IFX_int32_t) &oCID_RX_Data, TD_DEV_NOT_SET, pFXO->nSeqConnId);

      if (ret != IFX_SUCCESS)
      {
         break;
      }

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
            ("Have read length %d of CID data:\n", oCID_RX_Data.nSize));

      if (oCID_RX_Data.nSize > 0 &&
          oCID_RX_Data.nSize < IFX_TAPI_CID_RX_SIZE_MAX)
      {
         memcpy(received_data + rcv_data_size, oCID_RX_Data.data,
                oCID_RX_Data.nSize);
         rcv_data_size += oCID_RX_Data.nSize;
         nLength -= oCID_RX_Data.nSize;
      }
      else
      {
          TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                ("Err, on IFX_TAPI_CID_RX_DATA_GET. (File: %s, line: %d)\n",
                __FILE__, __LINE__));

         break;
      }
   }
   while ((oCID_RX_Data.nSize > 0) && (nLength > 0));

   if (IFX_SUCCESS == ret)
   {
      COM_ITM_TRACES(IFX_NULL, IFX_NULL, IFX_NULL,
                     (IFX_void_t*)nDataCh, TTP_CID_FSK_RX_DATA_GET,
                     pFXO->nSeqConnId);
   }

   if (ret != IFX_ERROR)
   {
      ret = CID_PrepareTransparentMsg(pFXO, pCID_Msg, received_data,
                                      rcv_data_size, IFX_TAPI_CID_HM_ONHOOK);
   }

   COM_ITM_TRACES(IFX_NULL, IFX_NULL, IFX_NULL,
                  pCID_Msg, TTP_CID_MSG_RECEIVED_FSK, pFXO->nSeqConnId);

   if (ret != IFX_ERROR)
   {
      for (i = 0; i < rcv_data_size; i++)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
               ("0x%02X ", (IFX_uint8_t) received_data[i]));

         if ((i > 0) && ((i + 1) % 15 == 0))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId, ("\n"));
         }
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId, ("\n"));

      if (IFX_TRUE == g_pITM_Ctrl->fITM_Enabled)
      {
         /* nicer printout, CRC not included */
         for (i = 0; i < (rcv_data_size - 1); i++)
         {
            /* check if valid for printing digit */
            if ( ('0' <= ((IFX_uint8_t) received_data[i]) &&
                  '9' >= ((IFX_uint8_t) received_data[i]) ) ||
                 ('a' <= ((IFX_uint8_t) received_data[i]) &&
                  'z' >= ((IFX_uint8_t) received_data[i]) ) ||
                 ('A' <= ((IFX_uint8_t) received_data[i]) &&
                  'Z' >= ((IFX_uint8_t) received_data[i]) ) ||
                 (' ' == ((IFX_uint8_t) received_data[i]) ) )
            {
               /* print as char */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
                     ("'%c'  ", (IFX_char_t) received_data[i]));
            }
            else
            {
               /* print as number */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
                     ("0x%02X ", (IFX_uint8_t) received_data[i]));
            }
            /* print endline */
            if ((i > 0) && ((i + 1) % 15 == 0))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId, ("\n"));
            }
         }
      }
      /* last byte is CRC print as number */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
            ("0x%02X \n", (IFX_uint8_t) received_data[i]));
   }

   /* Free allocated memory */
   TD_OS_MemFree(received_data);

   return ret;
}

/**
   Reset peer name.

   \param pPhone - pointer to phone

   \return  IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_FromPeerReset(PHONE_t* pPhone)
{
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);

   /* check if peer name was set */
   if ('\0'!=  pPhone->szPeerName)
   {
      memset(pPhone->szPeerName, 0, TD_MAX_PEER_NAME_LEN);
   }
   return IFX_SUCCESS;
}

/**
   Send CID with external peer data.

   \param pPhone - pointer to phone
   \param pConn - pointer to connection

   \return  IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_FromPeerSend(PHONE_t* pPhone, CONNECTION_t* pConn)
{
   IFX_TAPI_CID_MSG_t oCID_Msg = {0};
   IFX_TAPI_CID_MSG_ELEMENT_t oCID_MsgElement[TD_CID_ELEM_COUNT];
   IFX_int32_t nUsedFd = TD_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   memset(oCID_MsgElement, 0 ,
          sizeof (IFX_TAPI_CID_MSG_ELEMENT_t) * TD_CID_ELEM_COUNT);
   /* set CID data */
   oCID_Msg.message = oCID_MsgElement;
   /* set number of elements */
   oCID_Msg.nMsgElements = TD_CID_ELEM_COUNT;

   /* set element type - Date and time presentation,
      it is set right before sending CID */
   oCID_MsgElement[TD_CID_IDX_DATE].date.elementType = IFX_TAPI_CID_ST_DATE;
   oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.elementType = IFX_TAPI_CID_ST_CLI;
   /* check call type */
   if (EXTERN_VOIP_CALL == pConn->nType && (IFX_TRUE != pConn->bIPv6_Call))
   {
      /* IFX_TAPI_CID_MSG_LEN_MAX is currently set to 50 so it is preety safe to
         assume that operation below will not try to copy more bytes than
         element size because this new string will be shorter than 20 bytes
         1 (0) + 3 (%03d) + 12 (%d) + 1 ('\0') = 17 */
      /* number that must be used for external call */
      snprintf((IFX_char_t*)oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.element,
               IFX_TAPI_CID_MSG_LEN_MAX - 1, "0%03d%d",
               /* get IP lowest byte */
               TD_SOCK_AddrIPv4Get(&pConn->oConnPeer.oRemote.oToAddr) &
               TD_IP_LOWEST_BYTE_MASK,
               pConn->oConnPeer.nPhoneNum);
   }
   else if (PCM_CALL == pConn->nType)
   {
      /* number that must be used for PCM call */
      snprintf((IFX_char_t*)oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.element,
               IFX_TAPI_CID_MSG_LEN_MAX - 1, "09%03d%d",
               /* get IP lowest byte */
               TD_SOCK_AddrIPv4Get(&pConn->oConnPeer.oPCM.oToAddr) &
               TD_IP_LOWEST_BYTE_MASK,
               pConn->oConnPeer.nPhoneNum);
   }
   else
   {
      /* default number of peer */
      snprintf((IFX_char_t*)oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.element,
               IFX_TAPI_CID_MSG_LEN_MAX - 1, "%d", pConn->oConnPeer.nPhoneNum);
   }
   oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.len =
      strlen((IFX_char_t*)oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.element);
   /* set element type - Calling line name. */
   oCID_MsgElement[TD_CID_IDX_NAME].string.elementType = IFX_TAPI_CID_ST_NAME;
   /* check if peer name is set */
   if ('\0' != pPhone->szPeerName[0])
   {
      strncpy((IFX_char_t*)oCID_MsgElement[TD_CID_IDX_NAME].string.element,
              pPhone->szPeerName, TD_MAX_PEER_NAME_LEN - 1);
      CID_FromPeerReset(pPhone);
   }
   else
   {
      /* check call type and set default name */
      if (EXTERN_VOIP_CALL == pConn->nType)
      {
         strncpy((IFX_char_t*)oCID_MsgElement[TD_CID_IDX_NAME].string.element,
                 "EXTERNAL VOIP", TD_MAX_PEER_NAME_LEN);
      }
      else if (PCM_CALL == pConn->nType)
      {
         strncpy((IFX_char_t*)oCID_MsgElement[TD_CID_IDX_NAME].string.element,
                 "EXTERNAL PCM", TD_MAX_PEER_NAME_LEN);
      }
      else
      {
         strncpy((IFX_char_t*)oCID_MsgElement[TD_CID_IDX_NAME].string.element,
                 "UNKNOW CALL TYPE", TD_MAX_PEER_NAME_LEN);
      }
   }
   /* get string length */
   oCID_MsgElement[TD_CID_IDX_NAME].string.len =
      strlen((IFX_char_t*)oCID_MsgElement[TD_CID_IDX_NAME].string.element);
#ifndef EASY336
   if (0 < pPhone->pBoard->nMaxCoderCh)
   {
      nUsedFd = pPhone->nDataCh_FD;
   }
   else
#endif /*  EASY336 */
   if (0 < pPhone->pBoard->nMaxAnalogCh)
   {
      nUsedFd = pPhone->nPhoneCh_FD;
   }
   /* send CID */
   if (IFX_SUCCESS !=  CID_Send(nUsedFd, pPhone, IFX_TAPI_CID_HM_ONHOOK,
                                &oCID_Msg, IFX_NULL))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: CID_Send failed. (File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Set peer name in message structure.

   \param pPhone - pointer to phone
   \param pMsg - pointer to message

   \return  IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_FromPeerMsgSet(PHONE_t* pPhone, TD_COMM_MSG_t* pMsg)
{
   IFX_int32_t nElement;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pMsg, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pCID_Msg, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pCID_Msg->message, IFX_ERROR);

   /* reset CID name */
   CID_FromPeerReset(pPhone);

   /** serch for element with name */
   for (nElement=0; nElement < pPhone->pCID_Msg->nMsgElements; nElement++)
   {
      if (IFX_TAPI_CID_ST_NAME ==
          pPhone->pCID_Msg->message[nElement].string.elementType)
      {
         if (TD_MAX_PEER_NAME_LEN <=
             pPhone->pCID_Msg->message[nElement].string.len)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: name string (%s) is too long (%d). "
                   "Only part of it will be used. (File: %s, line: %d)\n",
                   pPhone->nPhoneNumber,
                   pPhone->pCID_Msg->message[nElement].string.element,
                   pPhone->pCID_Msg->message[nElement].string.len,
                   __FILE__, __LINE__));
         }
         /* copy peer name */
         strncpy(pMsg->oData1.oCID.aPeerName,
                 (IFX_char_t*)pPhone->pCID_Msg->message[nElement].string.element,
                 TD_MAX_PEER_NAME_LEN - 1);
         pMsg->oData1.oCID.nType = TD_COMM_MSG_DATA_1_CID;
         return IFX_SUCCESS;
      }
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, Phone No %d: Failed to set peer name. (File: %s, line: %d)\n",
          pPhone->nPhoneNumber, __FILE__, __LINE__));
   return IFX_ERROR;

}

/**
   Get peer name from message structure and copy it to phone structure.

   \param pPhone - pointer to phone
   \param pMsg - pointer to message

   \return  IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_FromPeerMsgGet(PHONE_t* pPhone, TD_COMM_MSG_t* pMsg)
{
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pMsg, IFX_ERROR);

   /** check if peer name was set */
   if (TD_COMM_MSG_DATA_1_CID == pMsg->oData1.oCID.nType)
   {
      if ('\0'!=  pMsg->oData1.oCID.aPeerName)
      {
         strncpy(pPhone->szPeerName, pMsg->oData1.oCID.aPeerName,
                 TD_MAX_PEER_NAME_LEN - 1);
         return IFX_SUCCESS;
      }
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}


