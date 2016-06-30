#ifndef _BOARD_EASY508XX_H
#define _BOARD_EASY508XX_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : board_easy508xx.h
   Description :
 ****************************************************************************
   \file board_easy508xx.h

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
enum { SYSTEM_AM_DEFAULT_VMMC_EASY508XX = 0 };

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t BOARD_Easy508XX_Register(BOARD_t* pBoard);

#endif /* _BOARD_EASY508XX_H */

