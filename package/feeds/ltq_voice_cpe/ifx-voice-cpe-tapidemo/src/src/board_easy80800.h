#ifndef _BOARD_EASY80800_H
#define _BOARD_EASY80800_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : board_easy80800.h
   Description :
 ****************************************************************************
   \file board_easy80800.h

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
#define SYSTEM_AM_DEFAULT_VMMC_VINAX 0

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t BOARD_Easy80800_Register(BOARD_t* pBoard);

#endif /* _BOARD_EASY80800_H */

