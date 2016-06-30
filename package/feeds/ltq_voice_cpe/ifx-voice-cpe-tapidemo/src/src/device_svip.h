#ifndef _DEVICE_SVIP_H
#define _DEVICE_SVIP_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : device_svip.h
   Description :
 ****************************************************************************
   \file device_svip.h

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

#ifdef HAVE_DRV_SVIP_HEADERS
    #include "svip_io.h"
#else
    #error "drv_svip is missing"
#endif

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
extern IFX_char_t* sBBD_CRAM_File_SVIP_R;
extern IFX_char_t* sBBD_CRAM_File_SVIP_P;
extern IFX_char_t* sBBD_CRAM_File_SVIP_S;
#endif /* USE_FILESYSTEM */

extern IFX_char_t* sCTRL_DEV_NAME_SVIP;

extern SVIP_IO_INIT SvipDevInitStruct;
/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t DEVICE_SVIP_Register(TAPIDEMO_DEVICE_CPU_t* pCpuDevice);
IFX_return_t DEVICE_SVIP_Init(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t DEVICE_SVIP_SetupChannel(BOARD_t* pBoard, IFX_char_t* pPath);

#ifndef TD_USE_CHANNEL_INIT
IFX_return_t DEVICE_SVIP_SetupChannelUseDevStart(BOARD_t* pBoard,
                                                 IFX_char_t* pPath);
#endif /* TD_USE_CHANNEL_INIT */

#endif /* _DEVICE_SVIP_H */
