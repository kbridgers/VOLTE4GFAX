
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file teridian_fxo.c
   \date 2007-07-13
   \brief This file contain implementation of FXO.

   It initializes 73M1X66 chipset and uses PCM or channel mapping
   and timeslot usage. Basically when connection to 73M1X66 is started phone
   channel must be mapped to pcm channel and then pcm connection between this
   pcm channels and pcm channel on 73M1X66 is started.
*/

#include "ifx_types.h"
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#ifdef FXO
#ifdef TERIDIAN_FXO
/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef HAVE_DRV_TERIDIAN_HEADERS
   #include "73m1966_io.h"
#else
   #error "drv_teridian is missing"
#endif /* HAVE_DRV_TERIDIAN_HEADERS */

#ifdef HAVE_DRV_TAPI_HEADERS
   #include "drv_tapi_io.h"
#else
    #error "drv_tapi is missing"
#endif /* HAVE_DRV_TAPI_HEADERS */

#include "tapidemo.h"
#include "pcm.h"

#include "teridian_fxo.h"

#ifdef USE_FILESYSTEM
/** File holding coefficients. */
IFX_char_t* sBBD_CRAM_File_TER[2] = {"ter1x66_bbd.bin", "ter1x66_bbd_fxo.bin"};
#endif /* USE_FILESYSTEM */

/* ============================= */
/* Global Structures             */
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
   Configure PCM interface for Teridian device, ...

   \param pFxoDevice - pointer to FXO device structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR

   \remark
*/
IFX_return_t TERIDIAN_CfgPCMInterface(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   IFX_TAPI_PCM_IF_CFG_t pcm_if;

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

   if (IFX_SUCCESS != TD_IOCTL(pFxoDevice->nDevFD, IFX_TAPI_PCM_IF_CFG_SET,
                         (IFX_int32_t) &pcm_if, TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err configure PCM interface (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      TERIDIAN_GetLastErr(pFxoDevice);
      return IFX_ERROR;
   }

   /* set PCM frequency - only for main board*/
   /* oPCM_Data.nPCM_Rate = pcm_if.nDCLFreq; */

   return IFX_SUCCESS;
} /* TERIDIAN_CfgPCMInterface() */
#ifdef USE_FILESYSTEM
/**
   Find BBD_CRAM

   \param pCpuDevice - pointer to CPU device
   \param pszPath - path to file
   \param pszFullFilename - path with filename

   \return  Return IFX_TRUE is BBD_CRAM file is found or IFX_FALSE if not
*/
IFX_boolean_t TERIDIAN_FindBBD_CRAM(IFX_char_t* pszPath,
                                    IFX_char_t* pszFullFilename)
{
   TD_PTR_CHECK(pszPath, IFX_FALSE);
   TD_PARAMETER_CHECK((0 == strlen(pszPath)), strlen(pszPath), IFX_FALSE);

   if ((IFX_NULL != sBBD_CRAM_File_TER[0]) &&
       (0 != strlen(sBBD_CRAM_File_TER[0])))
   {
      /* Find BBD_CRAM file with default new name */
      if(IFX_SUCCESS == Common_CreateFullFilename(sBBD_CRAM_File_TER[0],
                                                  pszPath, pszFullFilename))
      {
         if(IFX_SUCCESS == Common_CheckFileExists(pszFullFilename))
         {
            /* Find BBD_CRAM file with default name */
            return IFX_TRUE;
         }
      }
   }

   if ((IFX_NULL != sBBD_CRAM_File_TER[1]) &&
       (0 != strlen(sBBD_CRAM_File_TER[1])))
   {
      /* Find BBD_CRAM file with default old name */
      if(IFX_SUCCESS == Common_CreateFullFilename(sBBD_CRAM_File_TER[1],
                                                  pszPath, pszFullFilename))
      {
         if(IFX_SUCCESS == Common_CheckFileExists(pszFullFilename))
         {
            /* Find BBD_CRAM file with alternative name */
            return IFX_TRUE;
         }
      }
   }
   return IFX_FALSE;
} /* TERIDIAN_pszBBD_CRAM_File */
#endif /* USE_FILESYSTEM */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Initialize 73M1X66 channels

   \param pFxoDevice - pointer to FXO device structure
   \param psDownloadPath - string with download path
   \param pFxo - pointer to single FXO channel structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TERIDIAN_InitChannel(TAPIDEMO_DEVICE_FXO_t *pFxoDevice,
                                  IFX_char_t* psDownloadPath,
                                  FXO_t* pFxo)
{
   M1966_CH_INIT_STRUCT_t terInit;
   IFX_TAPI_CH_INIT_t terTapiChInit;
   IFX_TAPI_LINE_TYPE_CFG_t line_type;
   IFX_TAPI_LINE_VOLUME_t terVolume;
   IFX_uint32_t ret = IFX_SUCCESS;
#ifdef USE_FILESYSTEM
   IFX_uint8_t *pCountryCode = IFX_NULL;
   IFX_size_t nSize = 0;
   IFX_char_t pszFullFilename[MAX_PATH_LEN] = {0};
   /* True if required BIN file is found, else FALSE */
   IFX_boolean_t bCountryCodeIsSet = IFX_FALSE;
#endif

   /* check input arguments */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);
   TD_PTR_CHECK(psDownloadPath, IFX_ERROR);
   TD_PTR_CHECK(pFxo, IFX_ERROR);

   memset(&terInit, 0, sizeof(M1966_CH_INIT_STRUCT_t));
   memset(&terTapiChInit, 0, sizeof(IFX_TAPI_CH_INIT_t));
#ifdef USE_FILESYSTEM
   COM_RCV_FILES_GET(TYPE_TERIDIAN, ITM_RCV_FILE_BBD_FXO,
                     &pCountryCode, &nSize);

   /* Find BBD file with default name */
   if (IFX_NULL != pCountryCode ||
       IFX_TRUE == TERIDIAN_FindBBD_CRAM(psDownloadPath, pszFullFilename))
   {
      /* if BBD was not received before */
      if (IFX_NULL == pCountryCode)
      {
         ret = TD_OS_FileLoad(pszFullFilename, &pCountryCode, &nSize);

         /* if failed, the use default */
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, reading BBD file %s. Using default country settings.\n",
                  pszFullFilename));
            /* In some of versions of Teridian driver the country code is passed
               by IFX_TAPI_CH_INIT_t->nCountry whilst in other by
               M1966_CH_INIT_STRUCT_t->country_code which is passed
               by IFX_TAPI_CH_INIT_t->pProc. Tapidemo supports both solutions. */
            terTapiChInit.nCountry = pFxoDevice->nCountry;
            terInit.country_code = pFxoDevice->nCountry;
            bCountryCodeIsSet = IFX_TRUE;
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
                  ("Filesystem BBD binary used for Teridian FXO (%s).\n",
                   pszFullFilename));
         }
      }/* Find BBD file with default name */

      if (IFX_NULL != pCountryCode)
      {
         terTapiChInit.nCountry = atoi((IFX_char_t *)pCountryCode);
         terInit.country_code = atoi((IFX_char_t *)pCountryCode);
         bCountryCodeIsSet = IFX_TRUE;
         if (COM_RCV_FILES_CHECK_IF_FREE_MEM(pCountryCode))
         {
            /* release memory that is no longer needed */
            TD_OS_MemFree(pCountryCode);
         }
         pCountryCode = IFX_NULL;
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("BBD binary %s is not found.\n"
            "Using default country settings.\n",
            sBBD_CRAM_File_TER[0]));
      terTapiChInit.nCountry = pFxoDevice->nCountry;
      terInit.country_code = pFxoDevice->nCountry;
      bCountryCodeIsSet = IFX_TRUE;
   }
#else
   terTapiChInit.nCountry = pFxoDevice->nCountry;
   terInit.country_code = pFxoDevice->nCountry;
#endif

   terTapiChInit.nMode = IFX_TAPI_INIT_MODE_PCM_PHONE;
   terTapiChInit.pProc = &terInit;
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
      ("Country code used for Teridian FXO - %d.\n",
       terInit.country_code));

   ret = TD_IOCTL(pFxo->nFxoCh_FD, IFX_TAPI_CH_INIT, &terTapiChInit,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (ret == IFX_ERROR)
   {
      if (0 == pFxo->nFxoCh)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("%s No %d IFX_TAPI_CH_INIT failed for ch %d.\n"
               "   It is possible that device node is created, "
               "but device itself isn't connected.\n",
               pFxoDevice->pFxoTypeName, pFxoDevice->nDevNum, pFxo->nFxoCh));
         return IFX_ERROR;
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("%s No %d IFX_TAPI_CH_INIT failed for ch %d. (File: %s, line: %d)\n",
            pFxoDevice->pFxoTypeName, pFxoDevice->nDevNum, pFxo->nFxoCh,
            __FILE__, __LINE__));
      TERIDIAN_GetLastErr(pFxoDevice);
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
           ("IFX_TAPI_LINE_TYPE_SET failed, %s No %d, ch %d."
            " (File: %s, line: %d)\n",
            pFxoDevice->pFxoTypeName, pFxoDevice->nDevNum, pFxo->nFxoCh,
            __FILE__, __LINE__));
      TERIDIAN_GetLastErr(pFxoDevice);
      return (IFX_ERROR);
   }

   memset(&terVolume, 0, sizeof(IFX_TAPI_LINE_VOLUME_t));
   terVolume.nGainRx = 0;
   terVolume.nGainTx = 0;
   /* Set volume */
   ret = TD_IOCTL(pFxo->nFxoCh_FD, IFX_TAPI_PHONE_VOLUME_SET, &terVolume,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("IFX_TAPI_PHONE_VOLUME_SET failed, %s No %d ch %d."
             " (File: %s, line: %d)\n",
             pFxoDevice->pFxoTypeName, pFxoDevice->nDevNum, pFxo->nFxoCh,
             __FILE__, __LINE__));
      TERIDIAN_GetLastErr(pFxoDevice);
      return (IFX_ERROR);
   }

   return IFX_SUCCESS;
}

/**
   Display 73M1X66 version

   \param  pFxoDevice - pointer to FXO device structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TERIDIAN_GetVersions(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   /* value 80 is the same as in drivers code and example */
   IFX_char_t Version[80];

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);

   if (IFX_SUCCESS == TD_IOCTL(pFxoDevice->nDevFD, IFX_TAPI_VERSION_GET,
                         (IFX_char_t*) &Version[0],
                          TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                      ("Teridian %d Version: %s\n",
                       pFxoDevice->nDevNum, Version));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Teridian %d version could not be read. (File: %s, line: %d)\n",
            pFxoDevice->nDevNum, __FILE__, __LINE__));
      TERIDIAN_GetLastErr(pFxoDevice);
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

/**
   Display Teridian last error.

   \param pFxoDevice - pointer to FXO device structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TERIDIAN_GetLastErr(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   IFX_TAPI_Error_t Error;
#if 0
   IFX_int32_t i = 0;
#endif

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);

   /* clean structure */
   memset(&Error, 0, sizeof(IFX_TAPI_Error_t));

   if (IFX_SUCCESS == TD_IOCTL(pFxoDevice->nDevFD, IFX_TAPI_LASTERR,
                         (IFX_int32_t)&Error, TD_DEV_NOT_SET, TD_CONN_ID_DBG))
   {
      if (Error.nCode != -1)
      {
         /* we have additional information */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
               ("Error Code 0x%X occured\n", Error.nCode));
#if 0
         for (i = 0; i < ERRNO_CNT; ++i)
         {
            if (drvErrnos[i] == (Error.nCode & 0xffff))
            {
                TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
                      ("%s\n", drvErrStrings[i]));
            }
         }
         for (i = 0; i < Error.nCnt; ++i)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
                 ("%s:%d Code 0x%X\n", Error.stack[i].sFile,
                  Error.stack[i].nLine, Error.stack[i].nCode));
         }
#endif
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
            ("Err, Failed to get last error. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

/**
   Initialize Teridian device.

   \param pFxoDevice - pointer to FXO device structure
   \param pBoard - pointer to board structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TERIDIAN_DevInit(TAPIDEMO_DEVICE_FXO_t* pFxoDevice,
                              BOARD_t* pBoard)
{
   IFX_int32_t nDrvDbgLevel;

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pFxoDevice->nCountry = TERIDIAN_DEFAULT_COUNTRY;

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

   /* todo: Report set ? */
   if (IFX_SUCCESS != TD_IOCTL(pFxoDevice->nDevFD, IFX_TAPI_DEBUG_REPORT_SET,
                         nDrvDbgLevel, TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TERIDIAN_GetLastErr(pFxoDevice);
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
        ("ioctl IFX_TAPI_DEBUG_REPORT_SET ok.\n"));

   return IFX_SUCCESS;
}
#endif /* TERIDIAN_FXO */
#endif /* FXO */

/** ---------------------------------------------------------------------- */



