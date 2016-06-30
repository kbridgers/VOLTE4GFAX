

/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file device_vmmc.c
   \date 2006-10-01
   \brief Interface implementation for VMMC device.

   Application needs it to initialize device.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"

#ifdef HAVE_DRV_VMMC_HEADERS
    #include "drv_vmmc_errno.h"
    #include "drv_vmmc_strerrno.h"
#else
    #error "drv_vmmc is missing"
#endif

#include "device_vmmc.h"

/* ============================= */
/* Defines                       */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */


/* ============================= */
/* Global variable definition    */
/* ============================= */


#ifdef USE_FILESYSTEM

#ifdef TD_FW_FILE
   /* overwrite default FW file name - simplifies adding new board */
   IFX_char_t* sPRAMFile_VMMC = TD_FW_FILE;
#endif /* TD_FW_FILE */

#ifdef TD_BBD_FILE
   /* overwrite default BBD file name - simplifies adding new board */
   IFX_char_t* sBBD_CRAM_File_VMMC = TD_BBD_FILE;
#endif /* TD_BBD_FILE */

#ifdef DANUBE
      /** Prepare file names for DANUBE */
   #ifndef TD_FW_FILE
      IFX_char_t* sPRAMFile_VMMC = "voice_danube_firmware.bin";
   #endif /* TD_FW_FILE */
      IFX_char_t* sPRAMFile_VMMC_Old = "danube_firmware.bin";
      IFX_char_t* sDRAMFile_VMMC = "";
      /** File holding coefficients. */
   #ifndef TD_BBD_FILE
      IFX_char_t* sBBD_CRAM_File_VMMC = "danube_bbd.bin";
   #endif /* TD_BBD_FILE */
      IFX_char_t* sBBD_CRAM_File_VMMC_Old = "danube_bbd_fxs.bin";
#elif AR9
      /** Prepare file names for AR9 */
   #ifndef TD_FW_FILE
      IFX_char_t* sPRAMFile_VMMC = "voice_ar9_firmware.bin";
   #endif /* TD_FW_FILE */
      IFX_char_t* sPRAMFile_VMMC_Old = "ar9_firmware.bin";
      IFX_char_t* sDRAMFile_VMMC = "";
      /** File holding coefficients. */
   #ifndef TD_BBD_FILE
      IFX_char_t* sBBD_CRAM_File_VMMC = "ar9_bbd.bin";
   #endif /* TD_BBD_FILE */
      IFX_char_t* sBBD_CRAM_File_VMMC_Old = "ar9_bbd_fxs.bin";
#elif TD_XWAY_XRX300
      /** Prepare file names for AR9 */
   #ifndef TD_FW_FILE
      IFX_char_t* sPRAMFile_VMMC = "voice_xrx300_firmware.bin";
   #endif /* TD_FW_FILE */
      IFX_char_t* sPRAMFile_VMMC_Old = "xrx300_firmware.bin";
      IFX_char_t* sDRAMFile_VMMC = "";
      /** File holding coefficients. */
   #ifndef TD_BBD_FILE
      IFX_char_t* sBBD_CRAM_File_VMMC = "xrx300_bbd.bin";
   #endif /* TD_BBD_FILE */
      IFX_char_t* sBBD_CRAM_File_VMMC_Old = "xrx300_bbd_fxs.bin";
#elif VINAX
      /** Prepare file names for VINAX */
   #ifndef TD_FW_FILE
      IFX_char_t* sPRAMFile_VMMC = "voice_vinax_firmware.bin";
   #endif /* TD_FW_FILE */
      IFX_char_t* sPRAMFile_VMMC_Old = "firmware.bin";
      IFX_char_t* sDRAMFile_VMMC = "";
      /** File holding coefficients. */
   #ifndef TD_BBD_FILE
      IFX_char_t* sBBD_CRAM_File_VMMC = "bbd.bin";
   #endif /* TD_BBD_FILE */
      IFX_char_t* sBBD_CRAM_File_VMMC_Old = "";
#elif VR9
      /** Prepare file names for VR9 */
   #ifndef TD_FW_FILE
      IFX_char_t* sPRAMFile_VMMC = "voice_vr9_firmware.bin";
   #endif /* TD_FW_FILE */
      IFX_char_t* sPRAMFile_VMMC_Old = "vr9_firmware.bin";
      IFX_char_t* sDRAMFile_VMMC = "";
      /** File holding coefficients. */
   #ifndef TD_BBD_FILE
      IFX_char_t* sBBD_CRAM_File_VMMC = "vr9_bbd.bin";
   #endif /* TD_BBD_FILE */
      IFX_char_t* sBBD_CRAM_File_VMMC_Old = "vr9_bbd_fxs.bin";
#else /* VMMC_BOARD_NAME */
#endif /* VMMC_BOARD_NAME */

#endif /* USE_FILESYSTEM */

/** Device names */
IFX_char_t* sCH_DEV_NAME_VMMC = "/dev/vmmc";
IFX_char_t* sCTRL_DEV_NAME_VMMC = "/dev/vmmc";
IFX_char_t* sSYS_DEV_NAME_VMMC = "";

/** Holding device dependent data */
VMMC_IO_INIT VmmcDevInitStruct = {0};

/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t DEVICE_Vmmc_Fillin_FW_Ptr(BOARD_TYPE_t nChipType,
                                       IFX_void_t* pDrvInit,
                                       TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                       IFX_char_t* pPath);
IFX_void_t DEVICE_Vmmc_Release_FW_Ptr(IFX_void_t* pDrvInit);
IFX_return_t DEVICE_Vmmc_GetVersions(BOARD_t* pBoard);
IFX_return_t DEVICE_Vmmc_Init(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_void_t DEVICE_Vmmc_DecodeErrnoForDrv(IFX_int32_t nCode,
                                         IFX_uint32_t nSeqConnId);
IFX_return_t DEVICE_Vmmc_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath);

#ifdef TD_USE_CHANNEL_INIT
IFX_return_t DEVICE_Vmmc_SetupChannelUseChInit(BOARD_t* pBoard, IFX_char_t* pPath);
#else /* TD_USE_CHANNEL_INIT */
IFX_return_t DEVICE_Vmmc_SetupChannelUseDevStart(BOARD_t* pBoard, IFX_char_t* pPath);
#endif /* TD_USE_CHANNEL_INIT */


/* ============================= */
/* Local function definition     */
/* ============================= */
/**
   Fills in firmware pointers.

   \param pDrvInit - handle to VMMC_IO_INIT structure, should not be null
   \param pCpuDevice - pointer to CPU device
   \param pPath - path to FW files for download

   \return IFX_SUCCES on ok, otherwise IFX_ERROR

   \remarks
*/
IFX_return_t DEVICE_Vmmc_Fillin_FW_Ptr(BOARD_TYPE_t nChipType,
                                       IFX_void_t* pDrvInit,
                                       TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                       IFX_char_t* pPath)
{
   VMMC_IO_INIT* vinit = (VMMC_IO_INIT *) pDrvInit;
#ifdef USE_FILESYSTEM
   IFX_return_t ret = IFX_SUCCESS;
   IFX_uint8_t *pPram = IFX_NULL;
   IFX_uint8_t *pBBD = IFX_NULL;
   IFX_char_t pszFullFilename[MAX_PATH_LEN] = {0};
   IFX_boolean_t bFoundBinFile = IFX_FALSE;
#endif /* USE_FILESYSTEM */

   /* check input arguments */
   TD_PTR_CHECK(vinit, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   if ((vinit->pBBDbuf != IFX_NULL) && (vinit->pPRAMfw != IFX_NULL))
   {
      /* fw and bbd files already downloaded. If there is need to
         download new fw and bbd files, first current buffers
         must be released - DEVICE_Vmmc_Release_FW_Ptr()*/
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
           ("FW and BBD files already downloaded.\n"));
      return IFX_SUCCESS;
   }

#ifdef USE_FILESYSTEM
   COM_RCV_FILES_GET(nChipType, ITM_RCV_FILE_FW,
                     &vinit->pPRAMfw, &vinit->pram_size);
   if (vinit->pPRAMfw == IFX_NULL)
   {
      /* Reading PRAM_FW file */
      /* Find PRAM_FW file with default names */
      /* First check for new name, next for old one */
      bFoundBinFile = Common_FindPRAM_FW(pCpuDevice, pPath, pszFullFilename);
      if(IFX_TRUE == bFoundBinFile)
      {
         /* Fill in PRAM_FW buffers from binary */
         ret = TD_OS_FileLoad(pszFullFilename, &pPram,
                              (IFX_size_t *) &vinit->pram_size);
         /* Release PRAM_FW memory if applicable */
         if ((IFX_ERROR == ret) || (IFX_NULL == pPram))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, reading firmware file %s.\n(File: %s, line: %d)\n",
                  pszFullFilename, __FILE__, __LINE__));

            if (pPram != IFX_NULL)
            {
               TD_OS_MemFree(pPram);
               pPram = IFX_NULL;
            }
            return IFX_ERROR;
         }
         else
         {
            vinit->pPRAMfw = pPram;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Filesystem firmware binary used (%s).\n",
                  pszFullFilename));
         } /* if ((IFX_ERROR == ret_fwram) ... */
      } /* if(IFX_TRUE == bFileFound) */
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, firmware file %s is not found.\n(File: %s, line: %d)\n",
               pCpuDevice->pszPRAM_FW_File[0], __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   COM_RCV_FILES_GET(nChipType, ITM_RCV_FILE_BBD_FXS,
                     &vinit->pBBDbuf, &vinit->bbd_size);
   /* if BBD file downloaded for test then overwrite the BBD anyway */
   if ((IFX_FALSE == g_bBBD_Dwld) && (IFX_NULL == vinit->pBBDbuf))
   {
      vinit->pBBDbuf = IFX_NULL;
      vinit->bbd_size = 0;
   }
   else if (vinit->pBBDbuf == IFX_NULL)
   {
      /* Reading BBD_CRAM file */
      /* Find BBD_CRAM file with default name */
      /* First check for new name, next for old one */
      bFoundBinFile = Common_FindBBD_CRAM(pCpuDevice, pPath, pszFullFilename);
      if (IFX_SUCCESS == ret)
      {
         /* Fill in BBD_CRAM buffers from binary */
         ret = TD_OS_FileLoad(pszFullFilename, &pBBD,
                              (IFX_size_t *) &vinit->bbd_size);
         /* Release BBD_CRAM memory if applicable */
         if ((IFX_ERROR == ret) || (IFX_NULL == pBBD))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, reading BBD file %s.\n(File: %s, line: %d)\n",
                  pszFullFilename, __FILE__, __LINE__));

            if (pBBD != IFX_NULL)
            {
               TD_OS_MemFree(pBBD);
               pBBD = IFX_NULL;
            }
            return IFX_ERROR;
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
                     ("Filesystem BBD binary used (%s).\n", pszFullFilename));
            vinit->pBBDbuf = pBBD;
            bFoundBinFile = IFX_TRUE;
         }
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, BBD file %s is not found.\n(File: %s, line: %d)\n",
                pCpuDevice->pszBBD_CRAM_File[0], __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

#endif /* USE_FILESYSTEM */

  return IFX_SUCCESS;
} /* DEVICE_Vmmc_Fillin_FW_Ptr */


/**
   Release firmware pointers.

   \param pDrvInit - handle to VMMC_IO_INIT structure

   \return none

   \remarks
*/
IFX_void_t DEVICE_Vmmc_Release_FW_Ptr(IFX_void_t* pDrvInit)
{
   VMMC_IO_INIT* vinit = (VMMC_IO_INIT *) pDrvInit;

   /* check input arguments */
   TD_PTR_CHECK(vinit,);

   /* pram file was read from file, as default isn't set: free memory */
   if (vinit->pPRAMfw != IFX_NULL)
   {
      if (COM_RCV_FILES_CHECK_IF_FREE_MEM(vinit->pPRAMfw))
      {
         TD_OS_MemFree(vinit->pPRAMfw);
      }
      vinit->pPRAMfw = IFX_NULL;
   }

   if (vinit->pBBDbuf != IFX_NULL)
   {

      if (COM_RCV_FILES_CHECK_IF_FREE_MEM(vinit->pBBDbuf))
      {
         TD_OS_MemFree(vinit->pBBDbuf);
      }
      vinit->pBBDbuf = IFX_NULL;
   }

} /* DEVICE_Vmmc_Release_FW_Ptr() */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Read version after succesfull initialization and display it.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t DEVICE_Vmmc_GetVersions(BOARD_t* pBoard)
{
   IFX_return_t ret;
   VMMC_IO_VERSION vinVers;

   memset(&vinVers, 0, sizeof(VMMC_IO_VERSION));

   ret = TD_IOCTL(pBoard->nDevCtrl_FD, FIO_GET_VERS, (IFX_int32_t) &vinVers,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS == ret)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("VMMC [version 0x%02X, type 0x%02X, channels %d] ready!\n",
          vinVers.nChip, vinVers.nType, vinVers.nChannel));
#ifndef TD_3_CPTD_SUPPORT_DISABLED
      /* #warning (VMMC_IO_VERSION).nEDSPHotFix was added on Thu Jun 09 2011,
         but there was no define that could be checked -
         here some backward compatibility can be lost. */
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("FW Version %d.%d.%d\n",
          vinVers.nEdspVers, vinVers.nEdspIntern, vinVers.nEDSPHotFix));
#else /* TD_3_CPTD_SUPPORT_DISABLED */
/**
   Here TD_3_CPTD_SUPPORT_DISABLED flag has second purpose as nEDSPHotFix
   was implemented not so long before multiple CPTD on one channel feature.
  */
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("FW Version %d.%d\n",
          vinVers.nEdspVers, vinVers.nEdspIntern));
#endif /* TD_3_CPTD_SUPPORT_DISABLED */
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("VMMC Driver version %d.%d.%d.%d\n",
          (int) (vinVers.nDrvVers >> 24) & 0xFF,
          (int) (vinVers.nDrvVers >> 16) & 0xFF,
          (int) (vinVers.nDrvVers >> 8)  & 0xFF,
          (int) (vinVers.nDrvVers)       & 0xFF));

      COM_GET_FW_VERSION_VMMC(vinVers);

   }
   else
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                      ("VMMC version could not be read.\n"));
   }

   return ret;
} /* DEVICE_Vmmc_GetVersions() */

/**
   Init device.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t DEVICE_Vmmc_Init(BOARD_t* pBoard, IFX_char_t* pPath)
{
   IFX_TAPI_CH_INIT_t Init = {0};
   IFX_int32_t i = 0;
   TAPIDEMO_DEVICE_t      *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t  *pCpuDevice = IFX_NULL;
   IFX_int32_t nMode;
   IFX_return_t ret = IFX_SUCCESS;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

#ifdef TAPI_VERSION4
   if (!pBoard->fSingleFD)
#endif
   {
      /* set TAPI debug level */
      ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_DEBUG_REPORT_SET,
               (IFX_int32_t)pBoard->pCtrl->pProgramArg->nTapiDbgLevel,
               TD_DEV_NOT_SET, TD_CONN_ID_INIT);

      if (IFX_SUCCESS != ret)
      {
         return IFX_ERROR;
      }
   }

   if ((pDevice = Common_GetDevice_CPU(pBoard->pDevice)) == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not found. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pCpuDevice = &pDevice->uDevice.stCpu;

#ifndef TD_USE_CHANNEL_INIT
   if(g_pITM_Ctrl->oVerifySytemInit.fEnabled)
#endif /* TD_USE_CHANNEL_INIT */
   {
      DEVICE_Vmmc_Fillin_FW_Ptr(pBoard->nType, pCpuDevice->pDevDependStuff,
                                pCpuDevice, pPath);

      /* Set tapi init structure */
      memset(&Init, 0, sizeof(IFX_TAPI_CH_INIT_t));
      Init.nMode = IFX_TAPI_INIT_MODE_VOICE_CODER;
      Init.pProc = (IFX_void_t*) pCpuDevice->pDevDependStuff;
   }

   if(g_pITM_Ctrl->oVerifySytemInit.fEnabled)
   {
      /* Read version */
      DEVICE_Vmmc_GetVersions(pBoard);
      COM_VERIFY_SYSTEM_INIT(pBoard, Init);
   }
   else
   {
      for (i = 0; i <= pBoard->nMaxCoderCh; i++)
      {

         if (pBoard->nMaxCoderCh == i)
         {
            /* Read version */
            DEVICE_Vmmc_GetVersions(pBoard);
            break;
         }
#ifdef TD_USE_CHANNEL_INIT
            /* Initialize all system channels */
#ifdef TAPI_VERSION4
         if (!pBoard->fSingleFD)
#endif
         {
            if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i], IFX_TAPI_CH_INIT,
                                  (IFX_int32_t) &Init,
                                  TD_DEV_NOT_SET, TD_CONN_ID_INIT))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Err, used ch %d "
                      "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
               break;
            }
         } /* (!pBoard->fSingleFD */
#endif /* TD_USE_CHANNEL_INIT */

         /* Set appropriate feeding on all (analog) line channels */
         if (i < pBoard->nMaxAnalogCh)
         {
            /* Set line in standby */
#ifdef TAPI_VERSION4
            if (!pBoard->fSingleFD)
#endif /* TAPI_VERSION4 */
            {
#ifdef TD_PPD
               if (pBoard->pCtrl->pProgramArg->oArgFlags.nDisablePpd)
               {
                  nMode = IFX_TAPI_LINE_FEED_STANDBY;
               }
               else
               {
                  if (IFX_SUCCESS != Common_SetPpdCfg(pBoard, i, TD_CONN_ID_INIT))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                           ("ERR, Common_SetPpdCfg() failed, ch %d  "
                            "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
                     break;
                  }
                  nMode = IFX_TAPI_LINE_FEED_PHONE_DETECT;
               }
#else
               nMode = IFX_TAPI_LINE_FEED_STANDBY;
#endif /* TD_PPD */
               if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i],
                                     IFX_TAPI_LINE_FEED_SET, nMode,
                                     TD_DEV_NOT_SET, TD_CONN_ID_INIT))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                        ("Err, used ch ch %d  "
                         "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
                  break;
               }
            } /* if (!pBoard->fSingleFD) */
         } /* if (i < pBoard->nMaxAnalogCh) */
#ifdef SLIC121_FXO
         /* by default FXO channels are in active state after loading firmware.
            Line mode is set to disabled to save the power and to avoid event
            detection by this channel when Tapidemo is started with teridian
            FXO. */
         /* for SLIC121 FXO channels are "placed" after FXS channels */
         else if (i >= pBoard->nMaxAnalogCh &&
                  i < (pBoard->nMaxAnalogCh + pBoard->nMaxFxo))
         {
            IFX_TAPI_FXO_LINE_MODE_t lineMode;

            /* reset structure */
            memset (&lineMode, 0, sizeof(IFX_TAPI_FXO_LINE_MODE_t));
            /* disable FXO channel to prevent event detection */
            lineMode.mode = IFX_TAPI_FXO_LINE_MODE_DISABLED;
            /* set line mode */
            if (IFX_SUCCESS !=  TD_IOCTL(pBoard->nCh_FD[i],
                                   IFX_TAPI_FXO_LINE_MODE_SET, &lineMode,
                                   TD_DEV_NOT_SET, TD_CONN_ID_INIT))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Err, used ch %d, FD %d, max analog %d, max FXO %d  "
                      "(File: %s, line: %d)\n",
                      i, pBoard->nCh_FD[i], pBoard->nMaxAnalogCh,
                      pBoard->nMaxFxo, __FILE__, __LINE__));
            } /* set line mode */
         } /* if FXO channel */
#endif /* SLIC121_FXO */
      } /* for */
   }

   DEVICE_Vmmc_Release_FW_Ptr(pCpuDevice->pDevDependStuff);

   if(!g_pITM_Ctrl->oVerifySytemInit.fEnabled)
   {
      if (pBoard->nMaxCoderCh != i)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Init failed for ch %d, (File: %s, line: %d)\n",
                (int) i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return IFX_SUCCESS;
} /* DEVICE_Vmmc_Init() */

/**
   Handles the DRIVER error codes and stack.

   \param nCode - error code
   \param nSeqConnId  - Seq Conn ID

   \remarks
*/
IFX_void_t DEVICE_Vmmc_DecodeErrnoForDrv(IFX_int32_t nCode,
                                         IFX_uint32_t nSeqConnId)
{
   IFX_int32_t i = 0;

   for (i = 0; i < VMMC_ERRNO_CNT; ++i)
   {
      if (VMMC_drvErrnos[i] == (nCode & 0xFFFF))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               (TD_LOW_LEVEL_ERR_FORMAT_STR, VMMC_drvErrStrings[i]));
         break;
      }
   }
}

/**
   Return last error number for vmmc driver.

   \param nFd  -  channel file descriptor

   \return number of error if successful, otherwise IFX_ERROR.
   \remarks
*/
IFX_return_t DEVICE_Vmmc_GetErrnoNum(IFX_int32_t nFd)
{
   IFX_int32_t nLastErr = 0, ret;

   ret = TD_IOCTL(nFd, FIO_LASTERR, (IFX_int32_t) &nLastErr,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);

   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Failed to get error number for fd %d, (File: %s, line: %d)\n",
             nFd, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return nLastErr;
}

/**
   Downloads fw and bbd files to TAPI and starts TAPI.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t DEVICE_Vmmc_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath)
{
#ifdef TD_USE_CHANNEL_INIT
   return DEVICE_Vmmc_SetupChannelUseChInit(pBoard, pPath);
#else
   return DEVICE_Vmmc_SetupChannelUseDevStart(pBoard, pPath);
#endif /* TD_USE_CHANNEL_INIT */
}

#ifdef TD_USE_CHANNEL_INIT
/**
   Downloads fw and bbd files and calls IFX_TAPI_CH_INIT for first channel.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks It is necessary to call this function to have working the
    IFX_TAPI_CAP_LIST.
*/
IFX_return_t DEVICE_Vmmc_SetupChannelUseChInit(BOARD_t* pBoard, IFX_char_t* pPath)
{
   IFX_TAPI_CH_INIT_t Init;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_char_t sdev_name[MAX_DEV_NAME_LEN_WITH_NUMBER];
   IFX_int32_t nCh_FD = -1;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t *pCpuDevice = IFX_NULL;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   if ((pDevice = Common_GetDevice_CPU(pBoard->pDevice)) == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not found. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pCpuDevice = &pDevice->uDevice.stCpu;

   ret = DEVICE_Vmmc_Fillin_FW_Ptr(pBoard->nType, pCpuDevice->pDevDependStuff,
                                   pCpuDevice, pPath);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, required binaries files are not downloaded.\n"
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* open channel 0, 10 max len of int32 as string */
   if(MAX_DEV_NAME_LEN_WITH_NUMBER > (strlen(pBoard->pszChDevName) + 2 * 10))
   {
      sprintf(sdev_name, "%s%d%d", (char *) pBoard->pszChDevName,
              (int) pBoard->nID, 1);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Invalid index of array 'sdev_name'. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   nCh_FD = Common_Open(sdev_name, pBoard);
   if (0 > nCh_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Invalid channel (ch 0 - %s) file descriptor."
            "(File: %s, line: %d)\n",
            sdev_name, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Set tapi init structure */
   memset(&Init, 0, sizeof(IFX_TAPI_CH_INIT_t));
   Init.nMode = IFX_TAPI_INIT_MODE_VOICE_CODER;
   Init.pProc = (IFX_void_t*) pCpuDevice->pDevDependStuff;

   ret = TD_IOCTL(nCh_FD, IFX_TAPI_CH_INIT, (IFX_int32_t) &Init,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS != ret)
   {
      TD_OS_DeviceClose(nCh_FD);
      return IFX_ERROR;
   }

   TD_OS_DeviceClose(nCh_FD);

   return IFX_SUCCESS;
}

#else /* TD_USE_CHANNEL_INIT */

/**
   Downloads fw and bbd files to TAPI and starts TAPI.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t DEVICE_Vmmc_SetupChannelUseDevStart(BOARD_t* pBoard, IFX_char_t* pPath)
{
   VMMC_IO_INIT oVmmcInit;
   IFX_return_t ret = IFX_SUCCESS;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t *pCpuDevice = IFX_NULL;
   IFX_int32_t nDevFd;
   VMMC_DWLD_t oBBD_Data;
   IFX_TAPI_DEV_START_CFG_t oStartConfig = {0};

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   if ((pDevice = Common_GetDevice_CPU(pBoard->pDevice)) == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not found. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pCpuDevice = &pDevice->uDevice.stCpu;

   ret = DEVICE_Vmmc_Fillin_FW_Ptr(pBoard->nType, pCpuDevice->pDevDependStuff,
                                   pCpuDevice, pPath);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, required binaries files are not downloaded.\n"
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* open device file descriptor */
   nDevFd = Common_Open(pBoard->pszCtrlDevName, pBoard);
   if (0 > nDevFd)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to open device %s fd. (File: %s, line: %d)\n",
             pBoard->pszCtrlDevName, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* stop TAPI - all resources allocated by TAPI for this specific device
      are freed */
   if (IFX_SUCCESS != TD_IOCTL(nDevFd, IFX_TAPI_DEV_STOP, NO_PARAM,
                         TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to stop. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      TD_OS_DeviceClose(nDevFd);
      return IFX_ERROR;
   }
   /* Set tapi init structure */
   memset(&oVmmcInit, 0, sizeof(VMMC_IO_INIT));
   oVmmcInit.pPRAMfw = ((VMMC_IO_INIT*)pCpuDevice->pDevDependStuff)->pPRAMfw;
   oVmmcInit.pram_size = ((VMMC_IO_INIT*)pCpuDevice->pDevDependStuff)->pram_size;
   if (IFX_SUCCESS != TD_IOCTL(nDevFd, FIO_FW_DOWNLOAD, &oVmmcInit,
                         TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed download FW. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      TD_OS_DeviceClose(nDevFd);
      return IFX_ERROR;
   }
   /* start TAPI - reserve all resources needed by TAPI
      for this specific device. */
   oStartConfig.nMode = IFX_TAPI_INIT_MODE_VOICE_CODER;
   if (IFX_SUCCESS != TD_IOCTL(nDevFd, IFX_TAPI_DEV_START, &oStartConfig,
                         TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to start device. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      TD_OS_DeviceClose(nDevFd);
      return IFX_ERROR;
   }
   /* Set tapi init structure */
   memset(&oBBD_Data, 0, sizeof(VMMC_DWLD_t));
   oBBD_Data.buf = ((VMMC_IO_INIT*)pCpuDevice->pDevDependStuff)->pBBDbuf;
   oBBD_Data.size = ((VMMC_IO_INIT*)pCpuDevice->pDevDependStuff)->bbd_size;
   if (IFX_NULL != oBBD_Data.buf)
   {
      if (IFX_SUCCESS != TD_IOCTL(nDevFd, FIO_BBD_DOWNLOAD, &oBBD_Data,
                            TD_DEV_NOT_SET, TD_CONN_ID_INIT))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Failed download BBD. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         TD_OS_DeviceClose(nDevFd);
         return IFX_ERROR;
      }
   }
   TD_OS_DeviceClose(nDevFd);
   return IFX_SUCCESS;
}
#endif /* TD_USE_CHANNEL_INIT */

/**
   Register device to the board.

   \param pCpuDevice - pointer to device

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
*/
IFX_return_t DEVICE_Vmmc_Register(TAPIDEMO_DEVICE_CPU_t* pCpuDevice)
{
   /* check input arguments */
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   pCpuDevice->Version = DEVICE_Vmmc_GetVersions;
   pCpuDevice->DecodeErrno = DEVICE_Vmmc_DecodeErrnoForDrv;
   pCpuDevice->GetErrno = DEVICE_Vmmc_GetErrnoNum;
   pCpuDevice->FillinFwPtr = DEVICE_Vmmc_Fillin_FW_Ptr;
   pCpuDevice->ReleaseFwPtr = DEVICE_Vmmc_Release_FW_Ptr;
   return IFX_SUCCESS;
}


