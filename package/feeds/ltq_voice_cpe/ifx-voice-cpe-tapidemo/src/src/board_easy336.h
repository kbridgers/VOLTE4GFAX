#ifndef _BOARD_EASY336_H
#define _BOARD_EASY336_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/****************************************************************************
   Module      : board_easy336.h
   Description :
 ****************************************************************************
   \file board_easy336.h

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "device_svip.h"
#include "lib_svip_rm.h"

/* ============================= */
/* Debug interface               */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/** Max phones on SVIP (analog channels) */
enum { MAX_PHONES_SVIP = 16 };
/** Max data channels on SVIP */
enum { MAX_CODER_CH_SVIP = 16 };
/** Max PCM channels on SVIP */
enum { MAX_PCM_CH_SVIP = 16 };

/* ============================= */
/* Global Variables              */
/* ============================= */

enum { SYSTEM_AM_DEFAULT_SVIP = 0 };

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_int32_t BOARD_Easy336_Reset(BOARD_t* pBoard,
                                IFX_int32_t nArg,
                                IFX_boolean_t hSetReset);
IFX_return_t BOARD_Easy336_Register(BOARD_t* pBoard);

IFX_return_t BOARD_Easy336_FillChannels(BOARD_t* pBoard);
IFX_return_t BOARD_Easy336_PCM_ConfigSet(BOARD_t* pBoard,
                                          RM_SVIP_PCM_CFG_t* pcm_cfg);
IFX_boolean_t BSVIP_EventOnDataChExists(PHONE_t* pPhone,
                                        IFX_TAPI_EVENT_t* pEvent);
IFX_boolean_t BSVIP_EventOnPCMChExists(BOARD_t* pBoard,
                                       PHONE_t* pPhone,
                                       IFX_TAPI_EVENT_t* pEvent);
IFX_return_t BOARD_Easy336_Init_RM(CTRL_STATUS_t* pCtrl,
                                   RM_SVIP_PCM_CFG_t* pPCM_cfg);
#endif /* _BOARD_EASY336_H */

