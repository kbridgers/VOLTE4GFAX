#ifndef _CONFERENCE_H
#define _CONFERENCE_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : conference.h
   Date        : 2005-12-01
   Description : This file enchance tapi demo with conferencing.
   \file

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


/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t CONFERENCE_PutOnHold(IFX_void_t);
IFX_return_t CONFERENCE_ResumeOnHold(IFX_void_t);
IFX_return_t CONFERENCE_Start(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
IFX_return_t CONFERENCE_GetNewPeer(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
IFX_return_t CONFERENCE_PeerLocal_Add(CTRL_STATUS_t *pCtrl, PHONE_t *pPhone,
                                      PHONE_t *pDstPhone,CONNECTION_t* pConn);
IFX_return_t CONFERENCE_PeerExternal_Add(CTRL_STATUS_t *pCtrl, PHONE_t *pPhone,
                                         CONNECTION_t* pConn);
IFX_return_t CONFERENCE_PeerPCM_Add(CTRL_STATUS_t *pCtrl, PHONE_t *pPhone,
                                    CONNECTION_t* pConn);
#ifdef FXO
IFX_return_t CONFERENCE_PeerFXO_Add(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn, FXO_t* pFxo);
IFX_return_t CONFERENCE_PeerFXO_RemoveBusy(CTRL_STATUS_t* pCtrl, FXO_t* pFxo);
#endif /* FXO */
IFX_return_t CONFERENCE_RemovePeer(CTRL_STATUS_t *pCtrl, PHONE_t *pPhone,
                                   CONNECTION_t** ppConn, CONFERENCE_t *pConf);
IFX_return_t CONFERENCE_End(CTRL_STATUS_t *pCtrl, PHONE_t *pPhone,
                            CONFERENCE_t *pConf);

#endif /* _CONFERENCE_H */

