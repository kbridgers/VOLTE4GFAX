#ifndef _STATE_TRANS_H
#define _STATE_TRANS_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : state_trans.h
   Description :
*******************************************************************************/
/**
   \file state_trans.h
   \date
   \brief Handling events for different states.
*/
/* ============================= */
/* Includes                      */
/* ============================= */

#include "common.h"

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */
/** check if wait flag is set and wait for user input */
#define STATE_MACHINE_WAIT(pCtrl) \
   if (pCtrl->pProgramArg->oArgFlags.nWait) \
   { \
   /* IPORTANT: scanf reads string ignoring white spaces at input start until, \
      next data is read until another white space, unread data will be read \
      when scanf is called again, tapidemo must be run without '&' \
      %*s rgument causes scanf to discard read data */ \
      scanf("%*s"); \
   }

/** check if call type is valid for fax/modem transmission */
#define TD_FAX_MODEM_CHECK_CALL_TYPE(m_pPhone, m_pConn) \
   if (EXTERN_VOIP_CALL != m_pConn->nType) \
   { \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, m_pPhone->nSeqConnId, \
         ("Phone No %d: Fax modem signal detected for wrong call type (%s)," \
          "%smust be %s.\n", \
          (IFX_int32_t) m_pPhone->nPhoneNumber, \
          Common_Enum2Name(m_pConn->nType, TD_rgCallTypeName), \
          m_pPhone->pBoard->pIndentPhone, \
          Common_Enum2Name(EXTERN_VOIP_CALL, TD_rgCallTypeName))); \
      return IFX_ERROR; \
   }

/* ============================= */
/* Global Variable declaration   */
/* ============================= */

extern TD_ENUM_2_NAME_t TD_rgIE_EventsName[];
extern TD_ENUM_2_NAME_t TD_rgStateName[];
extern TD_ENUM_2_NAME_t TD_rgCallTypeName[];

extern STATE_COMBINATION_t ST_rgFXSStates[];
extern STATE_COMBINATION_t ST_rgConfigStates[];


/* ============================= */
/* Global function declaration   */
/* ============================= */

extern IFX_return_t ST_HandleState_FXS(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);

extern IFX_return_t ST_HandleState_Config(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                          CONNECTION_t* pConn);

/* ============================= */
/* Local function declaration    */
/* ============================= */

IFX_return_t ST_FXS_DectNotRegistered_Register(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn);

IFX_return_t ST_FXS_Ready_HookOff(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                 CONNECTION_t* pConn);
IFX_return_t ST_FXS_Ready_FaultLineGkLow(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn);
IFX_return_t ST_FXS_xxx_FaultLineGkLow(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn);
IFX_return_t ST_FXS_Ready_Ringing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                 CONNECTION_t* pConn);

IFX_return_t ST_FXS_Ready_TestTone(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn);

IFX_return_t ST_FXS_OutDialtone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn);
IFX_return_t ST_FXS_OutDialtone_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn);

IFX_return_t ST_FXS_OutDialing_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);
IFX_return_t ST_FXS_OutDialing_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn);
IFX_return_t ST_FXS_OutDialing_ExtCallEstablished(CTRL_STATUS_t* pCtrl,
                                                  PHONE_t* pPhone,
                                                  CONNECTION_t* pConn);
IFX_return_t ST_FXS_OutDialing_ExtCallWrongNumber(CTRL_STATUS_t* pCtrl,
                                                  PHONE_t* pPhone,
                                                  CONNECTION_t* pConn);
IFX_return_t ST_FXS_OutDialing_ExtCallNoAnswer(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn);
IFX_return_t ST_FXS_OutDialing_ExtCallProceeding(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn);

IFX_return_t ST_FXS_Busytone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);

IFX_return_t ST_FXS_OutCalling_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);
IFX_return_t ST_FXS_OutCalling_Busy(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);
IFX_return_t ST_FXS_OutCalling_Ringback(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn);

IFX_return_t ST_FXS_OutRingback_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);
IFX_return_t ST_FXS_OutRingback_Active(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);

IFX_return_t ST_FXS_Active_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn);
IFX_return_t ST_FXS_Active_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn);
IFX_return_t ST_FXS_Active_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn);
IFX_return_t ST_FXS_Active_Conference(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);
IFX_return_t ST_FXS_Active_FaxModXxx(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn);
IFX_return_t ST_FXS_Active_DIS(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                               CONNECTION_t* pConn);
IFX_return_t ST_FXS_Active_T38Req(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn);

IFX_return_t ST_FXS_InRinging_Ready(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);
IFX_return_t ST_FXS_InRinging_HookOff(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);


IFX_return_t ST_FXS_CfDialtone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialtone_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                             CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialtone_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialtone_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn);

IFX_return_t ST_FXS_CfDialing_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialing_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                            CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialing_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialing_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialing_ExtCallEstablished(CTRL_STATUS_t* pCtrl,
                                                  PHONE_t* pPhone,
                                                  CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialing_ExtCallWrongNumber(CTRL_STATUS_t* pCtrl,
                                                  PHONE_t* pPhone,
                                                  CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialing_ExtCallNoAnswer(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfDialing_ExtCallProceeding(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn);

IFX_return_t ST_FXS_CfCalling_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfCalling_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                            CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfCalling_Busy(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfCalling_Ringback(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfCalling_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn);

IFX_return_t ST_FXS_CfRingback_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfRingback_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfRingback_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                             CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfRingback_Active(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);

IFX_return_t ST_FXS_CfBusytone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfBusytone_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfBusytone_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                             CONNECTION_t* pConn);

IFX_return_t ST_FXS_CfActive_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfActive_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                           CONNECTION_t* pConn);
IFX_return_t ST_FXS_CfActive_Conference(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn);

IFX_return_t ST_FXS_TestTone_HookOff(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn);
IFX_return_t ST_FXS_TestTone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);

IFX_return_t ST_FXS_FaxModem_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxModem_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                           CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxModem_DIS(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                 CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxModem_EndDataTrans(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                          CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxModem_T38Req(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);

IFX_return_t ST_FXS_FaxT38_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxT38_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxT38_EndDataTrans(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxT38_T38End(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn);

IFX_return_t ST_FXS_FaxT38Req_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxT38Req_EndConnection(CTRL_STATUS_t* pCtrl,
                                            PHONE_t* pPhone,
                                            CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxT38Req_EndDataTrans(CTRL_STATUS_t* pCtrl,
                                           PHONE_t* pPhone,
                                           CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxT38Req_T38xxx(CTRL_STATUS_t* pCtrl,
                                     PHONE_t* pPhone,
                                     CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxT38Req_T38End(CTRL_STATUS_t* pCtrl,
                                     PHONE_t* pPhone,
                                     CONNECTION_t* pConn);

IFX_return_t ST_FXS_FaxT38End_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxT38End_EndConnection(CTRL_STATUS_t* pCtrl,
                                            PHONE_t* pPhone,
                                            CONNECTION_t* pConn);
IFX_return_t ST_FXS_FaxT38End_T38End(CTRL_STATUS_t* pCtrl,
                                     PHONE_t* pPhone,
                                     CONNECTION_t* pConn);

IFX_return_t ST_Config_Ready_Peer(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn);
IFX_return_t ST_Config_Ready_Config(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn);
IFX_return_t ST_Config_Active_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn);
IFX_return_t ST_Config_Active_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn);

IFX_return_t ST_FXS_xxx_Ringing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                CONNECTION_t* pConn);
IFX_return_t ST_FXS_xxx_Active_KpiSockFail(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                           CONNECTION_t* pConn);

#endif /* _STATE_TRANS_H */


