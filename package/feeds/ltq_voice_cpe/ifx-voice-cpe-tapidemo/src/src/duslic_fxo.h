
#ifndef _DUSLIC_FXO_H
#define _DUSLIC_FXO_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file duslic_fxo.h
   \date 2007-05-31
   \brief Function prototypes for duslic - FXO.

*/

#ifdef FXO
#ifdef DUSLIC_FXO

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */
/** default country number for duslic */
#define DUSLIC_DEAFAULT_COUNTRY     0

/* ============================= */
/* Macros & Definitions    */
/* ============================= */

#ifdef DANUBE

#define TD_GPIO_PORT_NAME "/dev/danube-port"

 /* 
   Set GPIO configuration for active/deactive reset. 
   
   \param nFd - GPIO port file descriptor.
   \param nRet- Return value: IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
#ifdef OLD_BSP
#define TD_GPIO_DUSLIC_ResetConfig(nFd, nRet) \
{ \
   do { \
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_DIR, 0, 13, 1, \
         TD_GPIO_MODULE_TAPI_DEMO, nRet);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_OD, 0, 13, 1, \
         TD_GPIO_MODULE_TAPI_DEMO, nRet);\
   } while (0);\
   if (IFX_SUCCESS != nRet) \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR, \
         ("Err, Reset config failed.\n")); \
}
#else
#define TD_GPIO_DUSLIC_ResetConfig(nFd, nRet) \
{ \
   do { \
      nRet = Common_GPIO_ReservePin(nFd, 0, 13, TD_GPIO_MODULE_TAPI_DEMO);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_DIR, 0, 13, 1, \
         TD_GPIO_MODULE_TAPI_DEMO, nRet);\
      if (IFX_SUCCESS != nRet) break;\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_OD, 0, 13, 1, \
         TD_GPIO_MODULE_TAPI_DEMO, nRet);\
      if (IFX_SUCCESS != nRet) break;\
      nRet = Common_GPIO_FreePin(nFd, 0, 13, TD_GPIO_MODULE_TAPI_DEMO);\
   } while (0);\
   if (IFX_SUCCESS != nRet) \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR, \
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
#define TD_GPIO_DUSLIC_Reset(nFd, nRet, nActive) \
{ \
   if (IFX_SUCCESS == nRet)\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_OUTPUT, 0, 13,\
                        (nActive == IFX_TRUE)? 0 : 1,\
                         TD_GPIO_MODULE_TAPI_DEMO ,nRet);\
   if (IFX_SUCCESS != nRet)\
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR, \
         ("Err, %s reset failed.\n", \
            (nActive == IFX_TRUE)? "Activate" : "Deactivate")); \
}
#else
#define TD_GPIO_DUSLIC_Reset(nFd, nRet, nActive) \
{ \
   nRet = Common_GPIO_ReservePin(nFd, 0, 13, IFX_GPIO_MODULE_TAPI_DEMO);\
   if (IFX_SUCCESS == nRet)\
      TD_GPIO_IOCTL_PARM(nFd, IFX_GPIO_IOC_OUTPUT, 0, 13,\
                        (nActive == IFX_TRUE)? 0 : 1,\
                         TD_GPIO_MODULE_TAPI_DEMO ,nRet);\
   if (IFX_SUCCESS == nRet)\
      nRet = Common_GPIO_FreePin(nFd, 0, 13, TD_GPIO_MODULE_TAPI_DEMO);\
   if (IFX_SUCCESS != nRet)\
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR, \
         ("Err, %s reset failed.\n", \
          (nActive == IFX_TRUE)? "Activate" : "Deactivate")); \
}
#endif /* OLD_BSP */
#endif /* DANUBE */


/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t DUSLIC_CfgPCMInterface(TAPIDEMO_DEVICE_FXO_t* pFxoDevice);
IFX_return_t DUSLIC_InitChannel(TAPIDEMO_DEVICE_FXO_t *pFxoDevice,
                                FXO_t* pFxo);
IFX_return_t DUSLIC_GetVersions(TAPIDEMO_DEVICE_FXO_t* pFxoDevice);
IFX_return_t DUSLIC_GetLastErr(TAPIDEMO_DEVICE_FXO_t* pFxoDevice);
IFX_return_t DUSLIC_DevInit(TAPIDEMO_DEVICE_FXO_t* pFxoDevice,
                            BOARD_t* pBoard);

#endif /* DUSLIC_FXO */
#endif /* FXO */

#endif /* _DUSLIC_FXO_H */
