#ifndef _ABSTRACT_H
#define _ABSTRACT_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : abstract.h
   Date        : 2005-11-30
   Description : This file makes abstraction.
 ******************************************************************************/
/**
   \file abstract.h
   \date 2006-01-25
   \brief Abstraction methods between application and board, chip.

   Functions here return right phone, according to phone number, socket,
   data or phone channel and also for the right board.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#ifdef LINUX
#include "tapidemo_config.h"
#endif
/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

#ifndef EASY336
const STARTUP_MAP_TABLE_t* ABSTRACT_GetStartupMapTbl(BOARD_t* pBoard);
#endif /* EASY336 */
PHONE_t* ABSTRACT_GetPHONE_OfPhoneCh(BOARD_t *pBoard,
                                     IFX_uint16_t nPhoneDev,
                                     IFX_int32_t nPhoneCh,
                                     IFX_uint32_t nSeqConnId);
PHONE_t* ABSTRACT_GetPHONE_OfDataCh(IFX_int32_t nDataCh,
                                    CONNECTION_t** pConn,
                                    BOARD_t* pBoard);
IFX_int32_t ABSTRACT_GetDEV_OfDataCh(CTRL_STATUS_t *pCtrl,
                                     IFX_int32_t nDataCh);
IFX_int32_t ABSTRACT_GetDEV_OfPhoneCh(CTRL_STATUS_t *pCtrl,
                                      IFX_int32_t nPhoneCh);
IFX_boolean_t ABSTRACT_EventsFromPhoneCh(IFX_int32_t nDevIdx,
                                         IFX_boolean_t fDTMF_Dialing);
PHONE_t* ABSTRACT_GetPHONE_OfSocket(IFX_int32_t nSocket,
                                    CONNECTION_t** pConn,
                                    BOARD_t* pBoard);
PHONE_t* ABSTRACT_GetPHONE_OfConn(const CTRL_STATUS_t* pCtrl,
                                  const CONNECTION_t* pConn);

IFX_return_t ABSTRACT_DefaultMapping(CTRL_STATUS_t *pCtrl, BOARD_t* pBoard,
                                     IFX_uint32_t nSeqConnId);
IFX_return_t ABSTRACT_UnmapDefaults(CTRL_STATUS_t *pCtrl, BOARD_t* pBoard,
                                    IFX_uint32_t nSeqConnId);

#ifndef EASY336
IFX_return_t ABSTRACT_CustomMapping(BOARD_t* pBoard,
                                    IFX_int32_t nMappingsNumber,
                                    IFX_uint32_t nSeqConnId);
IFX_return_t ABSTRACT_UnmapCustom(BOARD_t* pBoard, IFX_uint32_t nSeqConnId);
#endif /* EASY336 */
PHONE_t* ABSTRACT_GetPHONE_Of_PCM_Socket(CTRL_STATUS_t *pCtrl,
                                         IFX_int32_t nPCM_Socket,
                                         CONNECTION_t** pConn);
PHONE_t* ABSTRACT_GetPHONE_OfCalledNum(CTRL_STATUS_t* pCtrl,
                                       IFX_int32_t nCalledNum,
                                       IFX_uint32_t nSeqConnId);
FXO_t* ABSTRACT_GetFxo(IFX_int32_t nFxoNum, CTRL_STATUS_t* pCtrl);
TAPIDEMO_DEVICE_t* ABSTRACT_GetDeviceOfDevFd(IFX_int32_t nDevFd,
                                              CTRL_STATUS_t* pCtrl);
CONNECTION_t* ABSTRACT_GetConnOfPhone(PHONE_t* pPhone,
                                      CALL_TYPE_t nCallType,
                                      IFX_int32_t nCalledPhoneNum,
                                      IFX_int32_t nCallerPhoneNum,
                                      TD_OS_sockAddr_t* pCallerIPAddr,
                                      IFX_boolean_t* fFreeConn,
                                      IFX_uint32_t nSeqConnId);
CONNECTION_t* ABSTRACT_GetConnOfLocal(PHONE_t* pPhone, PHONE_t* pCalledPhone,
                                      CALL_TYPE_t nType);
#if (!defined EASY336 && !defined TAPI_VERSION4)
IFX_return_t ABSTRACT_GetConnOfDataCh(PHONE_t** ppPhone, CONNECTION_t** ppConn,
                                      BOARD_t* pBoard, IFX_int32_t nDataCh);
PHONE_t* ABSTRACT_GetPHONE_OfCalledChAndCallType(CTRL_STATUS_t* pCtrl,
                                                 IFX_int32_t nCalledCh,
                                                 IFX_int32_t nBoardIdx,
                                                 CALL_TYPE_t nCallType);
#endif /* (!defined EASY336 && !defined TAPI_VERSION4) */
BOARD_t* ABSTRACT_GetBoard(CTRL_STATUS_t* pCtrl, IFX_TAPI_DEV_TYPE_t devType);
#ifndef TAPI_VERSION4
IFX_int32_t ABSTRACT_GetDataChannelOfFD(IFX_int32_t nFd, BOARD_t* pBoard);
#endif /*  TAPI_VERSION4 */

IFX_return_t ABSTRACT_SeqConnID_Init(CTRL_STATUS_t* pCtrl);
IFX_uint32_t ABSTRACT_SeqConnID_Get(PHONE_t *pPhone, FXO_t* pFXO,
                                    CTRL_STATUS_t* pCtrl);
IFX_uint32_t ABSTRACT_SeqConnID_Set(PHONE_t *pPhone, FXO_t* pFXO,
                                    IFX_uint32_t nSeqConnId);
IFX_return_t ABSTRACT_SeqConnID_Reset(PHONE_t *pPhone, FXO_t* pFXO);

#endif /* _ABSTRACT_H */

