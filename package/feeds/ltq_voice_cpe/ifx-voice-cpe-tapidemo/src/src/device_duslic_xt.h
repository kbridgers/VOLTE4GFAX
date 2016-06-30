#ifndef _DEVICE_DUSLIC_XT_H
#define _DEVICE_DUSLIC_XT_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : device_danube.h
   Description :
 ****************************************************************************
   \file device_duslic_xt.h

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#ifdef HAVE_DRV_DUSLICXT_HEADERS
    #include "drv_dxt_io.h"
#else
    #error "drv_dxt is missing"
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
extern IFX_char_t* sPRAMFile_DXT_DEFAULT;
extern IFX_char_t* sPRAMFile_DXT_Old;
extern IFX_char_t* sDRAMFile_DXT;
extern IFX_char_t* sBBD_CRAM_File_DXT;
extern IFX_char_t* sBBD_CRAM_File_DXT_Old;
#endif /* USE_FILESYSTEM */

extern IFX_char_t* sCH_DEV_NAME_DXT;
extern IFX_char_t* sCTRL_DEV_NAME_DXT;
#if defined (HAVE_DRV_EASY3201_HEADERS) || defined (TD_HAVE_DRV_BOARD_HEADERS)
extern IFX_char_t* sSYS_DEV_NAME_DXT;
#endif /* HAVE_DRV_EASY3201_HEADERS || TD_HAVE_DRV_BOARD_HEADERS */

extern DXT_IO_Init_t DuslicXtDevInitStruct_V1_3;
extern DXT_IO_Init_t DuslicXtDevInitStruct_V1_4;
extern IFX_boolean_t bDxT_RomFW;

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t DEVICE_DUSLIC_XT_SetVersions(BOARD_t* pBoard,
                                          TAPIDEMO_DEVICE_CPU_t* pCpuDevice);
IFX_return_t DEVICE_DUSLIC_XT_Init(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t DEVICE_DUSLIC_XT_BasicInit(IFX_int32_t nDevFD,
                                        IFX_int32_t nIrqNum);
IFX_return_t DEVICE_DUSLIC_XT_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath,
                                           IFX_boolean_t hExtBoard);
IFX_return_t DEVICE_DUSLIC_XT_Register(TAPIDEMO_DEVICE_CPU_t* pCpuDevice);

#endif /* _DEVICE_DUSLIC_XT_H */
