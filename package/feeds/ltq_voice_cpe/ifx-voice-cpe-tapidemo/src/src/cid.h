#ifndef _CID_H
#define _CID_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : cid.h
   Date        : 2005-11-30
   Description : This file enchance tapi demo with CID.

   \file cid.h

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

enum
{
   /** Number of CID elements, we use CLI number, name, date */
   TD_CID_ELEM_COUNT  = 3,
   /** Index of element location */
   TD_CID_IDX_DATE    = 0,
   TD_CID_IDX_CLI_NUM = 1,
   TD_CID_IDX_NAME    = 2
};

enum 
{
   /** Maximum number of defined users (Simpsons) */
   CID_MAX_USERS = 4,
#ifdef TAPI_VERSION4
   /** Maximum number of standards */
   CID_MAX_STANDARDS = 5
#else /* TAPI_VERSION4 */
   /** Maximum number of standards */
   CID_MAX_STANDARDS = 7
#endif /* TAPI_VERSION4 */
};

typedef enum _CID_DISPLAY_MODE_t
{
   /** Display all cid data (date, CLI number, name). */
   CID_DISPLAY_ALL     = 0,
   /** Display sending cid data (CLI number, name). */
   CID_DISPALY_SEND    = 1,
   /** Display received cid data (CLI number, name). */
   CID_DISPALY_RECEIVE = 2,
   /** Print only CLI number and name */
   CID_DISPLAY_SIMPLE  = 3
} CID_DISPLAY_MODE_t;

/** Structure holding standard number and standard description */
typedef struct _CID_STANDARD_t
{
   IFX_int32_t  nNumber;
   IFX_char_t  *sDescription;
} CID_STANDARD_t;


/* ============================= */
/* Global Variable declaration   */
/* ============================= */

/** CID standards with short description */
extern TD_ENUM_2_NAME_t TD_rgCidStandardName[];
extern TD_ENUM_2_NAME_t TD_rgCidStandardNameShort[];

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t CID_SetupData(IFX_int32_t nIdx, PHONE_t* pPhone);
IFX_void_t CID_Display(PHONE_t* pPhone, IFX_TAPI_CID_MSG_t *pCID_Msg,
                       CID_DISPLAY_MODE_t nMode);
IFX_return_t CID_SetStandard(IFX_int32_t nCID_Standard, IFX_uint32_t nSeqConnId);
IFX_TAPI_CID_STD_t CID_GetStandard(IFX_void_t);
IFX_return_t CID_ConfDriver(PHONE_t* pPhone);
IFX_return_t CID_Send(IFX_uint32_t nDataCh_FD, PHONE_t* pPhone,
                      IFX_TAPI_CID_HOOK_MODE_t nHookState,
                      IFX_TAPI_CID_MSG_t *pCID_Msg, PHONE_t* pOrgPhone);
IFX_return_t CID_Release(IFX_TAPI_CID_MSG_t *pCID_Msg);
IFX_return_t CID_StartFSK(FXO_t* pFXO, IFX_int32_t nFD, IFX_TAPI_CID_HOOK_MODE_t eHookMode);
IFX_return_t CID_StopFSK(FXO_t* pFXO, IFX_int32_t nFD, IFX_int32_t nDataCh);
IFX_return_t CID_ReadData(FXO_t* pFXO, IFX_int32_t nDataCh, IFX_int32_t nFD,
                          IFX_TAPI_CID_MSG_t* pCID_Msg, IFX_int32_t nLength);
IFX_return_t CID_FromPeerSend(PHONE_t* pPhone, CONNECTION_t* pConn);
IFX_return_t CID_FromPeerMsgSet(PHONE_t* pPhone, TD_COMM_MSG_t* pMsg);
IFX_return_t CID_FromPeerMsgGet(PHONE_t* pPhone, TD_COMM_MSG_t* pMsg);


#endif /* _CID_H */

