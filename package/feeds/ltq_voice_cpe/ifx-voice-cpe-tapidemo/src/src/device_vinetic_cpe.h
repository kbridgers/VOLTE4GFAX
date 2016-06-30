#ifndef _DEVICE_VINETIC_CPE_H
#define _DEVICE_VINETIC_CPE_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : device_vinetic_cpe.h
   Description :
 ****************************************************************************
   \file device_vinetic_cpe.h

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#ifdef HAVE_DRV_VINETIC_HEADERS
    #include "vinetic_io.h"       
#else
    #error "drv_vinetic is missing"
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
extern IFX_char_t* sPRAMFile_CPE;
extern IFX_char_t* sPRAMFile_CPE_Old;
extern IFX_char_t* sDRAMFile_CPE;
extern IFX_char_t* sBBD_CRAM_File_CPE;
extern IFX_char_t* sBBD_CRAM_File_CPE_Old;
#endif /* USE_FILESYSTEM */

extern IFX_char_t* sCH_DEV_NAME_CPE;
extern IFX_char_t* sCTRL_DEV_NAME_CPE;
#ifdef EASY3332
extern IFX_char_t* sSYS_DEV_NAME_CPE;
#endif /* EASY3332 */

extern VINETIC_IO_INIT VineticDevInitStruct;
/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t DEVICE_VINETIC_CPE_Init(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t DEVICE_VINETIC_CPE_BasicInit(BOARD_t* pBoard);
IFX_return_t DEVICE_VINETIC_CPE_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath,
                                             IFX_boolean_t hExtBoard);
IFX_return_t DEVICE_VINETIC_CPE_Register(TAPIDEMO_DEVICE_CPU_t* pCpuDevice);

#endif /* _DEVICE_VINETIC_CPE_H */
