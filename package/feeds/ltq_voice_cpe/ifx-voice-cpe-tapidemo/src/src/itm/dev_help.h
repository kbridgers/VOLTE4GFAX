#ifndef _DEV_HELP_H
#define _DEV_HELP_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : debug_help.h
   Date        : 2008-05-09
   Description : This file contains developer helping functions.
 ****************************************************************************
   \file dev_help.h

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

extern IFX_return_t TD_DEV_PrintMappedDataCh(BOARD_t* pBoard);
extern IFX_return_t TD_DEV_PrintConnections(BOARD_t* pBoard);

IFX_return_t TD_DEV_IoctlSetNameOfFd(IFX_int32_t nFd, IFX_char_t* pszFdName,
                                     BOARD_t *pBoard);
IFX_return_t TD_DEV_IoctlCleanUp(IFX_void_t);
IFX_int32_t TD_DEV_Ioctl(IFX_int32_t nFd, IFX_int32_t nIoctl, IFX_uint32_t nArg,
                         IFX_char_t* pszIoctl, IFX_uint16_t nDev,
                         IFX_uint32_t nSeqConnId, IFX_uint32_t nErrHandling,
                         IFX_char_t* pszFile, IFX_int32_t nLine);
IFX_char_t* TD_DEV_IoctlGetNameOfFd(IFX_int32_t nFd, IFX_uint32_t nSeqConnId);

#endif /* _DEV_HELP_H */

