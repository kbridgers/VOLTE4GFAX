/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : lib_tapi_signal.h
   Desription  : TAPI signal handling.
*******************************************************************************/
#ifndef _LIB_TAPI_SIGNAL_H
#define _LIB_TAPI_SIGNAL_H

#include "ifx_types.h"
#include "drv_tapi_io.h"
#ifdef LINUX
#include "tapidemo_config.h"
#endif
#include "common.h"

/** list of signals to enable/disable from standard list without DIS */
#define TD_SIGNAL_LIST        IFX_TAPI_SIG_CNGMOD | IFX_TAPI_SIG_CED | \
   IFX_TAPI_SIG_CEDEND | IFX_TAPI_SIG_AM | IFX_TAPI_SIG_V8BISRX | \
   IFX_TAPI_SIG_V8BISTX | IFX_TAPI_SIG_CNGFAX | IFX_TAPI_SIG_PHASEREV

/** list of signals to enable/disable from extended list without DIS */
#define TD_SIGNAL_LIST_EXT    IFX_TAPI_SIG_EXT_V21L | IFX_TAPI_SIG_EXT_V18A | \
   IFX_TAPI_SIG_EXT_V27 | IFX_TAPI_SIG_EXT_BELL | IFX_TAPI_SIG_EXT_V22 | \
   IFX_TAPI_SIG_EXT_V22ORBELL | IFX_TAPI_SIG_EXT_V32AC | \
   IFX_TAPI_SIG_EXT_CASBELL | IFX_TAPI_SIG_EXT_V21H

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

#ifdef TAPI_VERSION4
/** structure to hold event signal pairs */
typedef struct SIGNAL_TO_EVENT_t_
{
   /** signal number */
   IFX_uint32_t nSigNum;
   /** event number */
   IFX_TAPI_EVENT_ID_t nEventId;
} SIGNAL_TO_EVENT_t;

/** type of signal list: standard or extended */
typedef enum TD_TAPI_SIG_t_
{
   /** from standard list of signals */
   TD_TAPI_SIG,
   /** from extended list of signals */
   TD_TAPI_SIG_EXT,
   /** list of signals is not set */
   TD_TAPI_SIG_NOT_USED = -1
}TD_TAPI_SIG_t;
#endif /* TAPI_VERSION4 */

/* ============================= */
/* Global function declaration   */
/* ============================= */

extern IFX_return_t SIGNAL_EnableDetection(CONNECTION_t* pConn, 
                                           IFX_TAPI_SIG_DETECTION_t *pSignal,
                                           PHONE_t* pPhone);
extern IFX_return_t SIGNAL_DisableDetection(CONNECTION_t* pConn, 
                                            IFX_TAPI_SIG_DETECTION_t *pSignal,
                                            PHONE_t* pPhone);
extern IFX_return_t SIGNAL_ClearChannel(PHONE_t *pPhone, CONNECTION_t* pConn);
extern IFX_return_t SIGNAL_DisableNLP(PHONE_t *pPhone, CTRL_STATUS_t* pCtrl);

extern IFX_return_t SIGNAL_Setup(PHONE_t* pPhone, CONNECTION_t* pConn);
extern IFX_return_t SIGNAL_HandlerReset(PHONE_t* pPhone, CONNECTION_t* pConn);
extern IFX_return_t SIGNAL_RestoreChannel(PHONE_t *pPhone, CONNECTION_t* pConn);
extern IFX_return_t SIGNAL_StoreSettings(PHONE_t *pPhone, CONNECTION_t* pConn);

#endif /* _LIB_TAPI_SIGNAL_H */

