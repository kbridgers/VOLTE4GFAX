
#ifndef _DEVICE_VMMC_H
#define _DEVICE_VMMC_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : device_vmmc.h
   Description :
 ****************************************************************************
   \file device_vmmc.h

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

#ifdef HAVE_DRV_VMMC_HEADERS
    #include "vmmc_io.h"
#else
    #error "drv_vmmc is missing"
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
extern IFX_char_t* sPRAMFile_VMMC;
extern IFX_char_t* sPRAMFile_VMMC_Old;
extern IFX_char_t* sDRAMFile_VMMC;
extern IFX_char_t* sBBD_CRAM_File_VMMC;
extern IFX_char_t* sBBD_CRAM_File_VMMC_Old;

#endif /* USE_FILESYSTEM */

extern IFX_char_t* sCH_DEV_NAME_VMMC;
extern IFX_char_t* sCTRL_DEV_NAME_VMMC;
extern IFX_char_t* sSYS_DEV_NAME_VMMC;

extern VMMC_IO_INIT VmmcDevInitStruct;
/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t DEVICE_Vmmc_Register(TAPIDEMO_DEVICE_CPU_t* pCpuDevice);
IFX_return_t DEVICE_Vmmc_Init(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t DEVICE_Vmmc_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath);

#ifndef TD_USE_CHANNEL_INIT
IFX_return_t DEVICE_Vmmc_SetupChannelUseDevStart(BOARD_t* pBoard, IFX_char_t* pPath);
#endif /* TD_USE_CHANNEL_INIT */

#endif /* _DEVICE_VMMC_H */
