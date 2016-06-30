#ifndef _BOARD_EASY3332_H
#define _BOARD_EASY3332_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : board_easy3332.h
   Description :
 ****************************************************************************
   \file board_easy3332.h

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#ifdef LINUX
   #include "tapidemo_config.h"
#endif

#ifdef HAVE_DRV_EASY3332_HEADERS
    #include "easy3332_io.h"      
#else
    #error "drv_easy3332 is missing"
#endif    
    
#include "device_vinetic_cpe.h"

/* ============================= */
/* Debug interface               */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Global Variables              */
/* ============================= */
typedef enum
{
   SYSTEM_AM_8MOT_CPE = EASY3332_ACCESS_8BIT_MOTOROLA,
   SYSTEM_AM_8INTMUX_CPE = EASY3332_ACCESS_8BIT_INTELMUX,
   SYSTEM_AM_8INTDEMUX_CPE = EASY3332_ACCESS_8BIT_INTELDEMUX,
   SYSTEM_AM_8SPI_CPE = EASY3332_ACCESS_SPI
} SYS_ACCESS_MODE_CPE;

enum { SYSTEM_AM_DEFAULT_CPE = SYSTEM_AM_8MOT_CPE };

/* Clock rate according to enum
   \ref EASY3332_ClockRate_e */
#define TAPIDEMO_CLOCK_RATE 0

/* offset of EASY3332 register used to setup PCM, cpld cmd 1 register offset */
#define CMDREG1_OFF_CPE                    EASY3332_CPLD_CMDREG1_OFF
#define CMDREG2_OFF_CPE                    EASY3332_CPLD_CMDREG2_OFF
#define CMDREG3_OFF_CPE                    EASY3332_CPLD_CMDREG3_OFF
#define CMDREG4_OFF_CPE                    EASY3332_CPLD_CMDREG4_OFF
#define CMDREG5_OFF_CPE                    EASY3332_CPLD_CMDREG5_OFF

/* define if master - 1 or slave - 0 */
enum { MASTER_CPE = 0x010 }; /* bit 4 */
/* master will have clock OUT - 1, slave will have clock IN - 0 */
enum { PCMEN_MASTER_CPE = 0x020}; /* bit 5 */
/* both will have 1 */
enum { PCMON_CPE = 0x040}; /* bit 6 */


/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t BOARD_Easy3332_Register(BOARD_t* pBoard);

#endif /* _BOARD_EASY3332_H */
