#ifndef _EVENT_HANDLING_H
#define _EVENT_HANDLING_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : event_handling.h
   Date        : 2006-03-28
   Description : This file contains general function declaration for
                 handling events.

******************************************************************************
   \file event_handling.h

   \remarks This file contains general function declaration for handling events.

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

/** Flag to check event on all channels */
enum
{
   CHECK_ALL_CH = -1
};
/** Flag to check event on all devices */
enum
{
   CHECK_ALL_DEV = -1
};

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t EVENT_Check(IFX_int32_t nDevCtrl_FD, BOARD_t* pBoard);

IFX_return_t EVENT_Handle(PHONE_t *pPhone, CTRL_STATUS_t* pCtrl,
                          BOARD_t* pBoard);
IFX_boolean_t EVENT_DTMF_Dialing(IFX_int32_t nDataCh, BOARD_t* pBoard);
IFX_return_t EVENT_GetOwnerOfEvent(BOARD_t* pBoard, IFX_int32_t nDevCtrl_FD,
                                   PHONE_t** ppPhone, CONNECTION_t** ppConn,
                                   IFX_boolean_t* pbIsFxo, FXO_t** ppFXO);
IFX_return_t EVENT_InternalEventHandle(CTRL_STATUS_t* pCtrl);
IFX_return_t EVENT_FXO_Handle(FXO_t* pFXO, CTRL_STATUS_t* pCtrl,
                              BOARD_t* pBoard);
#if !defined(STREAM_1_1)
IFX_return_t EVENT_Handle_DataCH(IFX_int32_t nDataCh, PHONE_t *pPhone,
                                  BOARD_t* pBoard, FXO_t* pFXO);
#endif /* !defined(STREAM_1_1) */

#ifdef TAPI_VERSION4
IFX_return_t EVENT_CalibrationEnable(BOARD_t* pBoard, IFX_uint32_t nSeqConnId);
IFX_return_t EVENT_DetectionInit(CTRL_STATUS_t* pCtrl);
IFX_return_t EVENT_DetectionDeInit(CTRL_STATUS_t* pCtrl);
#endif /* TAPI_VERSION4 */

#endif /* _EVENT_HANDLING_H */

