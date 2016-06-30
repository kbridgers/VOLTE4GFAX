#ifndef _BOARD_XWAY_XRX300_H
#define _BOARD_XWAY_XRX300_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : board_xwayXRX300.h
   Description :
 ****************************************************************************
   \file board_xwayXRX300.h

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
enum { SYSTEM_AM_DEFAULT_VMMC_XWAY_XRX300 = 0 };

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t BOARD_XwayXRX300_Register(BOARD_t* pBoard);

#endif /* _BOARD_XWAY_XRX300_H */

