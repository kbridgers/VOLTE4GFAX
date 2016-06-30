

#ifndef _TERIDIAN_FXO_H
#define _TERIDIAN_FXO_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file teridian_fxo.h
   \brief Function prototypes for 73M1X66 - FXO.

*/

#ifdef FXO
#ifdef TERIDIAN_FXO

/* ============================= */
/* Includes                      */
/* ============================= */

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
/* Global variable definition    */
/* ============================= */

/** Country selection for Teridian. Default value. Used when BBD file can't
    be read. */
#define TERIDIAN_DEFAULT_COUNTRY    71

/** Teridain IRQ number - not used. */
#define TERIDIAN_INT_NUM_IRQ        0

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t TERIDIAN_CfgPCMInterface(TAPIDEMO_DEVICE_FXO_t* pFxoDevice);
IFX_return_t TERIDIAN_InitChannel(TAPIDEMO_DEVICE_FXO_t *pFxoDevice,
                                  IFX_char_t* psDownloadPath,
                                  FXO_t* pFxo);
IFX_return_t TERIDIAN_GetVersions(TAPIDEMO_DEVICE_FXO_t* pFxoDevice);
IFX_return_t TERIDIAN_GetLastErr(TAPIDEMO_DEVICE_FXO_t* pFxoDevice);
IFX_return_t TERIDIAN_DevInit(TAPIDEMO_DEVICE_FXO_t* pFxoDevice,
                              BOARD_t* pBoard);
#endif /* TERIDIAN_FXO */
#endif /* FXO */

#endif /* _TERIDIAN_FXO_H */
