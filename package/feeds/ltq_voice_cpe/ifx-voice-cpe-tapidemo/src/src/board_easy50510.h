#ifndef _BOARD_EASY50510_H
#define _BOARD_EASY50510_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : board_easy50510.h
   Description :
 ****************************************************************************
   \file board_easy50510.h

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "device_vinetic_cpe.h"

/* ============================= */
/* Macros & Definitions    */
/* ============================= */

#ifdef DANUBE

#define TD_GPIO_PORT_NAME "/dev/danube-port"
/* 
   Set PCM master mode. 
   
   \param nFd - GPIO port file descriptor.
   \param nRet- Return value: IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
#ifdef OLD_BSP
#define TD_GPIO_EASY50510_SetPCM_Master(nFd, nRet) \
{ \
   do { \
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, DANUBE_PORT_IOCALTSEL0, 1, 8, 0, nRet);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, DANUBE_PORT_IOCALTSEL1, 1, 8, 1, nRet);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, DANUBE_PORT_IOCDIR, 1, 8, 1, nRet);\
   } while (0); \
   if (IFX_SUCCESS != nRet)\
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG, \
            ("Err, Set PCM configuration over GPIO port failed.\n"));\
}
#else
#define TD_GPIO_EASY50510_SetPCM_Master(nFd, nRet) \
{ \
   do { \
      nRet = Common_GPIO_ReservePin(nFd, 1, 8, IFX_GPIO_MODULE_TAPI_DEMO);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_ALTSEL0, 1, 8, 0, IFX_GPIO_MODULE_TAPI_DEMO ,nRet);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_ALTSEL1, 1, 8, 1, IFX_GPIO_MODULE_TAPI_DEMO ,nRet);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_DIR, 1, 8, 1, IFX_GPIO_MODULE_TAPI_DEMO ,nRet);\
      if (IFX_SUCCESS != nRet) break;\
      nRet = Common_GPIO_FreePin(nFd, 1, 8, IFX_GPIO_MODULE_TAPI_DEMO);\
   } while (0); \
   if (IFX_SUCCESS != nRet)\
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG, \
            ("Err, Set PCM configuration over GPIO port failed.\n"));\
}
#endif /* OLD_BSP */

/* 
   Set GPIO configuration for active/deactive reset. 
   
   \param nFd - GPIO port file descriptor.
   \param nRet- Return value: IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
#ifdef OLD_BSP
#define TD_GPIO_EASY50510_ResetConfig(nFd, nRet) \
{ \
   do { \
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, DANUBE_PORT_IOCDIR, 1, 4, 1, nRet);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, DANUBE_PORT_IOCOD, 1, 4, 1, nRet);\
   } while (0);\
   if (IFX_SUCCESS != nRet) \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG, \
         ("Err, Reset config failed.\n"));\
}
#else
#define TD_GPIO_EASY50510_ResetConfig(nFd, nRet) \
{ \
   do { \
      nRet = Common_GPIO_ReservePin(nFd, 1, 4, IFX_GPIO_MODULE_TAPI_DEMO);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_DIR, 1, 4, 1, IFX_GPIO_MODULE_TAPI_DEMO ,nRet);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_OD, 1, 4, 1, IFX_GPIO_MODULE_TAPI_DEMO ,nRet);\
      if (IFX_SUCCESS != nRet) break;\
      nRet = Common_GPIO_FreePin(nFd, 1, 4, IFX_GPIO_MODULE_TAPI_DEMO);\
   } while (0);\
   if (IFX_SUCCESS != nRet) \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG, \
         ("Err, Reset config failed.\n"));\
}
#endif /* OLD_BSP */

/* 
   Active/deactive reset of device.
   
   \param nFd - GPIO port file descriptor.
   \param nRet- Return value: IFX_SUCCESS on ok, otherwise IFX_ERROR

   \remark Before active/deactive reset, execute TD_GPIO_EASY50510_ResetConfig 
*/
#ifdef OLD_BSP
#define TD_GPIO_EASY50510_Reset(nFd, nRet, nActive) \
{ \
   if (IFX_SUCCESS == nRet)\
      TD_GPIO_IOCTL_PARM(nFd, DANUBE_PORT_IOCOUTPUT, 1, 4,\
                        (nActive == IFX_TRUE)? 0 : 1, nRet);\
   if (IFX_SUCCESS != nRet)\
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG, \
         ("Err, %s reset failed.\n",\
          (nActive == IFX_TRUE)? "Activate" : "Deactivate"));\
}
#else
#define TD_GPIO_EASY50510_Reset(nFd, nRet, nActive) \
{ \
   nRet = Common_GPIO_ReservePin(nFd, 1, 4, IFX_GPIO_MODULE_TAPI_DEMO);\
   if (IFX_SUCCESS == nRet)\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_OUTPUT, 1, 4,\
                        (nActive == IFX_TRUE)? 0 : 1,\
                         IFX_GPIO_MODULE_TAPI_DEMO ,nRet);\
   if (IFX_SUCCESS == nRet)\
      nRet = Common_GPIO_FreePin(nFd, 1, 4, IFX_GPIO_MODULE_TAPI_DEMO);\
   if (IFX_SUCCESS != nRet)\
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG, \
         ("Err, %s reset failed.\n",\
          (nActive == IFX_TRUE)? "Activate" : "Deactivate")); \
}
#endif /* OLD_BSP */
#else
/* Add here macros for other type of  boards */
#endif /* DANUBE */

/* ============================= */
/* Global Defines                */
/* ============================= */

/** Access mode for vinetic device. */
#define ACCESS_MODE 0    /* Set access mode to SPI */

/* IRQ number when using GPIO and SPI connection, using IM0 */
#define IRQ_NUMBER 26 

/* Base address when using GPIO and SPI connection */
#define BASE_ADDRESS 1 /* 0x1F - broadcast */

/* ============================= */
/* Global Variables              */
/* ============================= */

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t BOARD_Easy50510_Register(BOARD_t* pBoard);
IFX_return_t BOARD_Easy50510_DetectNode(CTRL_STATUS_t* pCtrl);

#endif /* _BOARD_EASY50510_H */
