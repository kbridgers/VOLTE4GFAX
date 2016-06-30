#ifndef _BOARD_EASY3111_H
#define _BOARD_EASY3111_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : board_easy3111.h
   Description :
 ****************************************************************************
   \file board_easy3111.h

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "device_duslic_xt.h"

/* ============================= */
/* Debug interface               */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/* Board ID */
#define BOARD_ID 2

/* ============================= */
/* Global Variables              */
/* ============================= */

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
#ifdef EASY3111
IFX_return_t Easy3111_CallPCM_Activate(PHONE_t* pPhone, CTRL_STATUS_t* pCtrl);
IFX_return_t Easy3111_CallPCM_Deactivate(PHONE_t* pPhone, CTRL_STATUS_t* pCtrl);
IFX_return_t Easy3111_CallResourcesGet(PHONE_t *pPhone, CTRL_STATUS_t *pCtrl,
                                       CONNECTION_t *pConn);
IFX_return_t Easy3111_CallResourcesRelease(PHONE_t *pPhone, CTRL_STATUS_t *pCtrl,
                                           CONNECTION_t *pConn);
IFX_return_t Easy3111_ClearPhone(PHONE_t *pPhone, CTRL_STATUS_t *pCtrl,
                                 CONNECTION_t *pConn);
#endif /* EASY3111 */
IFX_return_t BOARD_Easy3111_Register(BOARD_t* pBoard);
IFX_int32_t BOARD_Easy3111_DetectNode(CTRL_STATUS_t* pCtrl);

#endif /* _BOARD_EASY3111_H */
