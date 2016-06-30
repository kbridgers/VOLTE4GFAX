
#ifndef _DEVICE_VXT_H
#define _DEVICE_VXT_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : device_vxt.h
   Description :
 ****************************************************************************
   \file device_vxt.h

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#include "./itm/com_client.h"

#include "drv_vxt_io.h"
#include "drv_vxt_strerrno.h"

/* ============================= */
/* Debug interface               */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Global Variables              */
/* ============================= */
#ifdef USE_FILESYSTEM
extern IFX_char_t* sBBD_CRAM_File_VXT_R;
extern IFX_char_t* sBBD_CRAM_File_VXT_S;
extern IFX_char_t* sBBD_CRAM_File_VXT_P;
#endif /* USE_FILESYSTEM */

extern IFX_char_t* sCTRL_DEV_NAME_VXT;
#ifdef TD_HAVE_DRV_BOARD_HEADERS
extern IFX_char_t* sSYS_DEV_NAME_VXT;
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

extern VXT_IO_Init_t VxtDevInitStruct;
/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t DEVICE_VXT_Register(TAPIDEMO_DEVICE_CPU_t* pCpuDevice);
IFX_return_t DEVICE_VXT_Init(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t DEVICE_VXT_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath);

#ifndef TD_USE_CHANNEL_INIT
IFX_return_t DEVICE_VXT_SetupChannelUseDevStart(BOARD_t* pBoard, IFX_char_t* pPath);
#endif /* TD_USE_CHANNEL_INIT */

#endif /* _DEVICE_VXT_H */

