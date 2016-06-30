#ifndef _BOARD_EASY3201_H
#define _BOARD_EASY3201_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : board_easy3201.h
   Description :
 ****************************************************************************
   \file board_easy3201.h

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#ifdef EASY3201
#ifdef HAVE_DRV_EASY3201_HEADERS
    #include "drv_easy3201_io.h"
#else
    #error "drv_easy3201 is missing"
#endif
#endif /* EASY3201 */

#ifdef EASY3201_EVS
#ifdef TD_HAVE_DRV_BOARD_HEADERS
    #include "drv_board_io.h"
#else /* TD_HAVE_DRV_BOARD_HEADERS */
    #error "drv_board is missing"
#endif /* TD_HAVE_DRV_BOARD_HEADERS */
#endif /* EASY3201_EVS */

#include "device_duslic_xt.h"


/* ============================= */
/* Debug interface               */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Global Variables              */
/* ============================= */
#ifdef HAVE_DRV_EASY3201_HEADERS
typedef enum
{
   SYSTEM_AM_SPI_DXT = EASY3201_CPLD_ACCESS_SPI,
   SYSTEM_AM_HD_AUDIO_DXT = EASY3201_CPLD_ACCESS_HD_AUDIO,
   SYSTEM_AM_HD_AUDIO_TEST = EASY3201_CPLD_ACCESS_HD_AUDIO_TEST,
   SYSTEM_AM_TESTMODE = EASY3201_CPLD_ACCESS_TESTMODE
} SYS_ACCESS_MODE_DXT;
enum { SYSTEM_AM_DEFAULT_DXT = SYSTEM_AM_SPI_DXT };
enum { SYSTEM_AM_UNSUPPORTED_DXT = SYSTEM_AM_TESTMODE + 1 };

/** Clock rate according to enum
      \ref EASY3201_CLOCK_RATE_e */
#define TAPIDEMO_CLOCK_RATE            EASY3201_CPLD_CLK_2048_KHZ

/* offset of EASY3201 register used to setup PCM, cpld cmd 1 register offset */
#define CMDREG1_OFF_DXT                EASY3201_CPLD_CMDREG1_OFF

/** flag for PCM master, cmdreg 1. For PCM slave just set it to 0. */
enum { PCM_MASTER = 0x08}; /* bit 3 */

/* offset of EASY3201 register used to setup PCM, cpld cmd 2 register offset */
#define CMDREG2_OFF_DXT                EASY3201_CPLD_CMDREG2_OFF

/** flag for PCM LOOP is enabled, cmdreg 2 */
enum { PCM_LOOP = 0x80 }; /* bit 7 */

/** Send clock out (PCMen) if we are PCM master, otherwise set to 0, cmdreg 2 */
enum { PCM_SEND_CLK = 0x08 }; /* bit 3 */

/** PCM interface active, otherwise set to 0, cmdreg 2 */
enum { PCMON_DXT = 0x10 }; /* bit 4 */
#endif /* HAVE_DRV_EASY3201_HEADERS */

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

#ifdef HAVE_DRV_EASY3201_HEADERS
IFX_return_t BOARD_Easy3201_Reset(BOARD_t* pBoard,
                                  IFX_int32_t nArg,
                                  IFX_boolean_t hSetReset);
#endif /* HAVE_DRV_EASY3201_HEADERS */

IFX_return_t BOARD_Easy3201_CfgPCMInterfaceSlave(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3201_Register(BOARD_t* pBoard);

IFX_return_t BOARD_Easy3201_InitSystem(BOARD_t* pBoard);

#endif /* _BOARD_EASY3201_H */

