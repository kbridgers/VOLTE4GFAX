#ifndef _ANALOG_H
#define _ANALOG_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : analog.h
   Description : This file enchance tapi demo with ALM support.
 ****************************************************************************
   \file analog.h

   \remarks

   \note Changes:
*******************************************************************************/

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
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_int32_t ALM_GetFD_OfCh(BOARD_t* pBoard, IFX_int32_t nPhoneCh);
IFX_return_t ALM_MapToPhone(IFX_int32_t nPhoneCh,
                            IFX_int32_t nAddPhoneCh,
                            TD_MAP_ADD_REMOVE_t fDoMapping,
                            BOARD_t* pBoard, IFX_uint32_t nSeqConnId);


#endif /* _ANALOG_H */

