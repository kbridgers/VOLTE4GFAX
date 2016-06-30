#ifndef _BOARD_EASY50712_H
#define _BOARD_EASY50712_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : board_easy50712.h
   Description :
 ****************************************************************************
   \file board_easy50712.h

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
#define SYSTEM_AM_DEFAULT_VMMC 0

/** IRQ number definition for DuSLIC on EASY50712. Basically is it taken from
    asm/danube/irq.h. */
#define DUSLIC_EASY50712_INT_NUM_IM2_IRL31 (0 + 64 + 31)

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t BOARD_Easy50712_Register(BOARD_t* pBoard);

#endif /* _BOARD_EASY50712_H */

