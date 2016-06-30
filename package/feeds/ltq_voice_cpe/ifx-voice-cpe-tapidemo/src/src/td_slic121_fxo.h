

#ifndef _TD_SLIC121_FXO_H
#define _TD_SLIC121_FXO_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_slic121_fxo.h
   \brief Function prototypes for SLIC121 FXO.

*/

#ifdef FXO
#ifdef SLIC121_FXO

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


/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t TD_SLIC121_FXO_DevInit(TAPIDEMO_DEVICE_FXO_t* pFxoDevice,
                                    BOARD_t* pBoard);
IFX_return_t TD_SLIC121_FXO_InitChannel(TAPIDEMO_DEVICE_FXO_t *pFxoDevice,
                                        IFX_char_t* psDownloadPath,
                                        FXO_t* pFxo);
IFX_return_t TD_SLIC121_FXO_GetVersions(TAPIDEMO_DEVICE_FXO_t* pFxoDevice);
#endif /* SLIC121_FXO */
#endif /* FXO */

#endif /* _TD_SLIC121_FXO_H */
