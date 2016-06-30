
#ifndef _TAPIDEMO_H
#define _TAPIDEMO_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : tapidemo.h
   Description  :
 ****************************************************************************
   \file tapidemo.h

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "common.h"

#include "./itm/com_client.h"
#include "./itm/dev_help.h"

#ifdef EASY336
/** PCM configuration */
extern RM_SVIP_PCM_CFG_t pcm_cfg;
#endif /* EASY336 */

/* TESTING - flag used when tapidemo starts on different versions. Moved
   to makefile.am, in VxWorks add this flag to compiler options */
/*#define STREAM_1_1*/ /* Default is version 1.2 - common voice api */
/* If flag above is not set version 1.1 is used */

/* ============================= */
/* Global Defines                */
/* ============================= */

#ifdef DEBUG
#define deb(msg) printf msg;
#else
#define deb(msg)
#endif


/** time out for select in miliseconds (50ms=0.05s) */
#define  SELECT_TIMEOUT_MSEC     50

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/** max length of strings in PrintCallProgress */
#define PCP_MAX_STR_LEN 80

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/** IDs for board combination */
enum _BOARD_COMB_ID
{
   NO_EXT_BOARD = 0,
   ONE_CPE = NO_EXT_BOARD,
   /* reserved value */
   BOARD_COMB_RESERVED = 1,
   ONE_VMMC = 2,
   ONE_VMMC_ONE_CPE = 3,
   ONE_DXT = 4,
   TWO_DXTS = 5,
   ONE_VMMC_VINAX = 6,
   ONE_VMMC_EASY508XX = 7,
   X_SVIP = 8,
   X_SVIP_X_XT16 = 9,
   X_XT16 = 10,
   ONE_EASY80910 = 11,
   UNSUPPORTED_COMB
};

/** Max user name length. */
enum { MAX_USERNAME_LEN = 20 };

/** Time [ms] which must pass for ONHOOK to be valid over FXO line. */
enum { TIMEOUT_FOR_ONHOOK = 7000 };

/** Time [ms] which must pass for waiting on answer from second phone
   during external call */ 
enum { TIMEOUT_FOR_EXTERNAL_CALL = 5000 };

/** Default Voice Activity Detection (VAD) configuration */
enum { DEFAULT_VAD_CFG = IFX_TAPI_ENC_VAD_NOVAD };

/** Default Voice Activity Detection (VAD) configuration */
enum { DEFAULT_LEC_CFG = IFX_TAPI_WLEC_TYPE_NE };

#ifdef TD_DECT
/** Default Echo Canceller type (EC) for DECT channels */
enum { DEFAULT_DECT_EC_CFG = IFX_TAPI_EC_TYPE_ES };
#endif /* TD_DECT */



/* Holding board combination names. */
extern const IFX_char_t* BOARD_COMB_NAMES[MAX_COMBINATIONS];

/* ============================= */
/* Global variable definition    */
/* ============================= */

/** Holding trace redirection settings */
extern TRACE_REDIRECTION_CTRL_t oTraceRedirection;

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t TAPIDEMO_SetAction(CTRL_STATUS_t* pCtrl,
                                PHONE_t* pPhone,
                                CONNECTION_t* pConn,
                                IFX_int32_t nAction,
                                IFX_uint32_t nSeqConnId);

extern TD_OS_socket_t TAPIDEMO_InitAdminSocket(IFX_void_t);

extern IFX_return_t TAPIDEMO_SetDefaultAddr(PROGRAM_ARG_t* pProgramArg);

extern IFX_void_t TAPIDEMO_ClearPhone(CTRL_STATUS_t* pCtrl,
                                      PHONE_t* pPhone,
                                      CONNECTION_t* pConn,
                                      INTERNAL_EVENTS_t nInternalEvent,
                                      BOARD_t* pBoard);
#endif /* _SYSTEM_H */

