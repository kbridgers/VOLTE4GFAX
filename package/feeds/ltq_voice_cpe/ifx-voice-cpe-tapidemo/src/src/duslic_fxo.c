

/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file duslic_fxo.c
   \date 2006-11-28
   \brief This file contain implementation of DuSLIC FXO.

   It initializes DuSLIC/Clare (FW, channels, ...) and PCM.
   Basically when connection to DuSLIC is started phone
   channel must be mapped to pcm channel and then pcm connection between this
   pcm channels and pcm channel on DuSLIC is started.
*/
#ifdef LINUX
   #include "tapidemo_config.h"
#endif

#include "ifx_types.h"
#include "tapidemo.h"

#ifdef FXO
#ifdef DUSLIC_FXO

/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef HAVE_DRV_DUSLIC_HEADERS
    #include "drv_duslic_io.h"
#else
    #error "drv_duslic is missing"
#endif

#ifdef HAVE_DRV_TAPI_HEADERS
    #include "drv_tapi_io.h"
#else
    #error "drv_tapi is missing"
#endif
#include "danube_bbd_fxo.c"
#include "pcm.h"
#include "voip.h"
#include "duslic_fxo.h"

#ifdef DANUBE
#include "board_easy50712.h"
#endif

#ifdef OLD_BSP
   #include "asm/danube/port.h"
#else
   #include "asm/ifx/ifx_gpio.h"
#endif


/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/**
   Configure PCM interface for Duslic device, ...

   \param pFxoDevice - pointer to FXO device structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR

   \remark
*/
IFX_return_t DUSLIC_CfgPCMInterface(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   IFX_TAPI_PCM_IF_CFG_t pcm_if;
   IFX_return_t ret;

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);

   memset(&pcm_if, 0, sizeof(IFX_TAPI_PCM_IF_CFG_t));

   /* Use PCM clock slave mode */
   pcm_if.nOpMode = IFX_TAPI_PCM_IF_MODE_SLAVE;

   pcm_if.nDCLFreq = IFX_TAPI_PCM_IF_DCLFREQ_2048;
   pcm_if.nDoubleClk = IFX_DISABLE;

   pcm_if.nSlopeTX = IFX_TAPI_PCM_IF_SLOPE_RISE;
   pcm_if.nSlopeRX = IFX_TAPI_PCM_IF_SLOPE_FALL;
   pcm_if.nOffsetTX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nOffsetRX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nDrive = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   pcm_if.nShift = IFX_DISABLE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("Configure PCM interface as slave, %s kHz\n",
          TAPIDEMO_PCM_FREQ_STR[pcm_if.nDCLFreq]));

   ret = TD_IOCTL(pFxoDevice->nDevFD, IFX_TAPI_PCM_IF_CFG_SET,
                  (IFX_int32_t) &pcm_if, TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (ret != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, configure PCM interface. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      DUSLIC_GetLastErr(pFxoDevice);
      return IFX_ERROR;
   }

   /* set PCM frequency - only for main board*/
   /* g_oPCM_Data.nPCM_Rate = pcm_if.nDCLFreq; */

   return IFX_SUCCESS;
} /* DUSLIC_CfgPCMInterface() */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Initialize DUSLIC channel

   \param pFxoDevice - pointer to FXO device structure
   \param pFxo - pointer to single FXO channel structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t DUSLIC_InitChannel(TAPIDEMO_DEVICE_FXO_t *pFxoDevice,
                                FXO_t* pFxo)
{
   IFX_return_t ret = IFX_SUCCESS;
   DUS_IO_INIT_t dusIoInit;
   IFX_TAPI_CH_INIT_t dusTapiChInit;
   IFX_TAPI_LINE_TYPE_CFG_t line_type;

   /* check input arguments */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);
   TD_PTR_CHECK(pFxo, IFX_ERROR);

   memset(&dusIoInit, 0, sizeof(DUS_IO_INIT_t));
   memset(&dusTapiChInit, 0, sizeof(IFX_TAPI_CH_INIT_t));

   dusIoInit.cram.ac_coef        = ac_coef;
   dusIoInit.cram.dc_coef        = dc_coef;
   dusIoInit.cram.gen_coef       = gen_coef;
   dusIoInit.cram.gen_coef_size  = 0x6;
   dusIoInit.cram.nFormat        = DUS_IO_CRAM_FORMAT_15;

   dusTapiChInit.nMode = IFX_TAPI_INIT_MODE_PCM_PHONE;
   dusTapiChInit.pProc = &dusIoInit;
   ret = TD_IOCTL(pFxo->nFxoCh_FD, IFX_TAPI_CH_INIT, &dusTapiChInit,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("IFX_TAPI_CH_INIT failed for ch %d. (File: %s, line: %d)\n",
            (int) pFxo->nFxoCh, __FILE__, __LINE__));
      DUSLIC_GetLastErr(pFxoDevice);
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
        ("ioctl IFX_TAPI_CH_INIT ok for ch %d.\n", pFxo->nFxoCh));

   memset(&line_type, 0, sizeof(IFX_TAPI_LINE_TYPE_CFG_t));
   line_type.lineType = IFX_TAPI_LINE_TYPE_FXO;
   line_type.nDaaCh = 0;
   /* Set line type */
   ret = TD_IOCTL(pFxo->nFxoCh_FD, IFX_TAPI_LINE_TYPE_SET, &line_type,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, ioctl with IFX_TAPI_LINE_TYPE_FXO failed."
            " (File: %s, line: %d)\n", __FILE__, __LINE__));
      DUSLIC_GetLastErr(pFxoDevice);
      return (IFX_ERROR);
   }

   return ret;
}

/**
   Display DUSLIC version

   \param  pFxoDevice - pointer to FXO device structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/

IFX_return_t DUSLIC_GetVersions(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   IFX_return_t ret;
   DUS_IO_Version_t duslicVers;
   IFX_uint8_t* pVersionNumber;

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);

   memset(&duslicVers, 0, sizeof(DUS_IO_Version_t));

   ret = TD_IOCTL(pFxoDevice->nDevFD, FIO_DUS_VERS,
            (IFX_int32_t) &duslicVers, TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS == ret)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("DUSLIC %d [version 0x%02X, type 0x%02X, channels %d] ready!\n",
          pFxoDevice->nDevNum,
          duslicVers.nChip, duslicVers.nType, duslicVers.nChannel));

      pVersionNumber = (IFX_uint8_t*) &duslicVers.nDrvVers;
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                      ("DRV Version %d.%d.%d.%d",
                       pVersionNumber[0],
                       pVersionNumber[1],
                       pVersionNumber[2],
                       pVersionNumber[3]));

      pVersionNumber = (IFX_uint8_t*) &duslicVers.nTapiVers;
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                      (" and included TAPI version %d.%d.%d.%d\n",
                       pVersionNumber[0],
                       pVersionNumber[1],
                       pVersionNumber[2],
                       pVersionNumber[3]));
   }
   else
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                      ("DUSLIC %d version could not be read.\n",
                       pFxoDevice->nDevNum));
   }

   return ret;
}

/**
   Display DUSLIC last error.

   \param pFxoDevice - pointer to FXO device structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t DUSLIC_GetLastErr(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   IFX_return_t ret;
   IFX_int32_t nDuslicErr = 0;

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);

   ret = TD_IOCTL(pFxoDevice->nDevFD, FIO_DUS_LASTERR,
            (IFX_int32_t) &nDuslicErr, TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
           ("DUSLIC %d last error is: 0x%0X\n",
            pFxoDevice->nDevNum, (int) nDuslicErr));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Could not get DUSLIC %d error. (File: %s, line: %d)\n",
             pFxoDevice->nDevNum, __FILE__, __LINE__));
   }

   return ret;
}

/**
   Initialize DUSLIC device.

   \param pFxoDevice - pointer to FXO device structure
   \param pBoard - pointer to board structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t DUSLIC_DevInit(TAPIDEMO_DEVICE_FXO_t* pFxoDevice,
                            BOARD_t* pBoard)
{
   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   IFX_uint32_t nRet = IFX_SUCCESS;
   IFX_int32_t nDrvDbgLevel;
   DUS_BasicDeviceInit_t dusInit;
   IFX_int32_t nGPIO_PortFd = NO_GPIO_FD;

   pFxoDevice->nCountry = DUSLIC_DEAFAULT_COUNTRY;

   nGPIO_PortFd = Common_GPIO_OpenPort(TD_GPIO_PORT_NAME);
   if (NO_GPIO_FD == nGPIO_PortFd)
   {
      return IFX_ERROR;
   }

   TD_GPIO_DUSLIC_ResetConfig(nGPIO_PortFd, nRet);
   if (IFX_SUCCESS != nRet)
   {
      Common_GPIO_ClosePort(TD_GPIO_PORT_NAME, nGPIO_PortFd);
      return IFX_ERROR;
   }

   TD_GPIO_DUSLIC_Reset(nGPIO_PortFd, nRet, IFX_TRUE);
   if (IFX_SUCCESS != nRet)
   {
      Common_GPIO_ClosePort(TD_GPIO_PORT_NAME, nGPIO_PortFd);
      return IFX_ERROR;
   }

   memset(&dusInit, 0, sizeof(DUS_BasicDeviceInit_t));
   dusInit.AccessMode = DUS_ACCESS_SPI;
#if DANUBE
   dusInit.nIrqNum = DUSLIC_EASY50712_INT_NUM_IM2_IRL31;
#else
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, DUSLIC FXO is supported only for Danube."
          "(File: %s, line: %d)\n",,
          __FILE__, __LINE__));
   return IFX_ERROR;
#endif
   nRet = TD_IOCTL(pFxoDevice->nDevFD, FIO_DUS_BASICDEV_INIT, &dusInit,
             TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s No %d: Ioctl FIO_DUS_BASICDEV_INIT failed. "
             "(File: %s, line: %d)\n",
             pFxoDevice->pFxoTypeName, pFxoDevice->nDevNum,
             __FILE__, __LINE__));

      Common_GPIO_ClosePort(TD_GPIO_PORT_NAME, nGPIO_PortFd);
      return IFX_ERROR;
   }

   TD_GPIO_DUSLIC_Reset(nGPIO_PortFd, nRet, IFX_FALSE);
   if (IFX_SUCCESS != nRet)
   {
      Common_GPIO_ClosePort(TD_GPIO_PORT_NAME, nGPIO_PortFd);
      return IFX_ERROR;
   }

   if (IFX_SUCCESS != Common_GPIO_ClosePort(TD_GPIO_PORT_NAME,
                                            nGPIO_PortFd))
   {
      return IFX_ERROR;
   }

   /* check debug level */
   if (DBG_LEVEL_NORMAL > pBoard->pCtrl->pProgramArg->nDbgLevel)
   {
      /* set normal debug level for driver */
      nDrvDbgLevel = 0x02;
   }
   else
   {
      /* set high debug level for driver */
      nDrvDbgLevel = 0x03;
   }

   nRet = TD_IOCTL(pFxoDevice->nDevFD, FIO_DUS_REPORT_SET, nDrvDbgLevel,
             TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s No %d: Ioctl FIO_DUS_REPORT_SET failed. "
             "(File: %s, line: %d)\n",
             pFxoDevice->pFxoTypeName, pFxoDevice->nDevNum,
             __FILE__, __LINE__));
      DUSLIC_GetLastErr(pFxoDevice);
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* DUSLIC_DevInit */

#endif /* DUSLIC_FXO */
#endif /* FXO */
/** ---------------------------------------------------------------------- */



