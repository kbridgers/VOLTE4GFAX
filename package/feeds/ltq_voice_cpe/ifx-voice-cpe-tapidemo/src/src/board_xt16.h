#ifndef _BOARD_XT16_H
#define _BOARD_XT16_H
/****************************************************************************
                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 ****************************************************************************
   Module      : board_xt16.h
   Description :
 ****************************************************************************
   \file board_xt16.h

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "device_vxt.h"
#ifdef EASY336
#include "lib_svip_rm.h"
#endif

/* ============================= */
/* Debug interface               */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/** Number of maximum supported PCM highways by RM. */
#define RM_XT16_NUM_HIGHWAYS     2

/* ============================= */
/* Global Variables              */
/* ============================= */

enum { SYSTEM_AM_DEFAULT_XT16 = 0 };

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t BOARD_XT_16_Register(BOARD_t* pBoard);
IFX_return_t BOARD_XT_16_FillChannels(BOARD_t* pBoard);
#ifdef EASY336
IFX_return_t BOARD_XT_16_PCM_ConfigSet(BOARD_t* pBoard,
                                       RM_SVIP_PCM_CFG_t* pcm_cfg);
#endif


IFX_return_t BXT16_FillChannels(BOARD_t* pBoard);
#endif /* _BOARD_XT16_H */

