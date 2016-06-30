#ifndef _BOARD_EASY80910_H
#define _BOARD_EASY80910_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : board_easy80910.h
   Description :
 ****************************************************************************
   \file board_easy80910.h

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "device_vmmc.h"

/* ============================= */
/* Debug interface               */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Global Variables              */
/* ============================= */
enum { SYSTEM_AM_DEFAULT_VMMC_EASY80910 = 0 };

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t BOARD_Easy80910_Reset(BOARD_t* pBoard,
                                   IFX_int32_t nArg,
                                   IFX_boolean_t hSetReset);
IFX_return_t BOARD_Easy80910_Register(BOARD_t* pBoard);

#endif /* _BOARD_EASY80910_H */

