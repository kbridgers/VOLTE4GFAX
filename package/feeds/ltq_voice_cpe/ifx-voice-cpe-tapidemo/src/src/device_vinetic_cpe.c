/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file device_vinetic_cpe.c
   \date 2006-10-01
   \brief Interface implementation for VINETIC CPE board.

*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"

#ifdef HAVE_DRV_VINETIC_HEADERS
   #include "drv_vinetic_errno.h"
#else
    #error "drv_vinetic is missing"
#endif

#ifndef USE_FILESYSTEM
/** Default FW file includes, to be provided */

#include "vin-cpe_bbd_fxs.c"
#include "vin-cpe_pDRAMfw.c"
#include "vin-cpe_pPRAMfw.c"
#endif /* USE_FILESYSTEM */

#include "device_vinetic_cpe.h"

/* ============================= */
/* Defines                       */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Names of access modes */
static const IFX_char_t* VIN_ACCESS_MODES[] =
{
   "VIN_ACCESS_SPI",
   "VIN_ACCESS_SCI",
   "VIN_ACCESS_PAR_16BIT",
   "VIN_ACCESS_PAR_8BIT",
   "VIN_ACCESS_PARINTEL_MUX16",
   "VIN_ACCESS_PARINTEL_MUX8",
   "VIN_ACCESS_PARINTEL_DMUX8",
   "VIN_ACCESS_PARINTEL_DMUX8_BE",
   "VIN_ACCESS_PARINTEL_DMUX8_LE",
   "VIN_ACCESS_PAR_8BIT_V2",
   IFX_NULL
};

/* ============================= */
/* Global variable definition    */
/* ============================= */


#ifdef USE_FILESYSTEM
/** Prepare file names for CPE */
IFX_char_t* sPRAMFile_CPE = "voice_vin-cpe_firmware.bin";
IFX_char_t* sPRAMFile_CPE_Old = "pramfw.bin";
IFX_char_t* sDRAMFile_CPE = "dramfw.bin";
/** File holding coefficients. */
IFX_char_t* sBBD_CRAM_File_CPE = "vin-cpe_bbd.bin";
IFX_char_t* sBBD_CRAM_File_CPE_Old = "vin-cpe_bbd_fxs.bin";
#endif /* USE_FILESYSTEM */

/** Device names */
IFX_char_t* sCH_DEV_NAME_CPE = "/dev/vin";
IFX_char_t* sCTRL_DEV_NAME_CPE = "/dev/vin";
#ifdef EASY3332
IFX_char_t* sSYS_DEV_NAME_CPE = "/dev/easy3332/";
#endif /* EASY3332 */

/** Holding device dependent data */
VINETIC_IO_INIT VineticDevInitStruct = {0};


/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t DEVICE_VINETIC_CPE_Fillin_FW_Ptr(BOARD_TYPE_t nChipType,
                                              IFX_void_t* pDrvInit,
                                              TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                              IFX_char_t* pPath);
IFX_void_t DEVICE_VINETIC_CPE_Release_FW_Ptr(IFX_void_t* pDrvInit);
IFX_return_t DEVICE_VINETIC_CPE_GetVersions(BOARD_t* pBoard);
IFX_void_t DEVICE_VINETIC_CPE_DecodeErrnoForDrv(IFX_int32_t nCode,
                                                IFX_uint32_t nSeqConnId);

/* ============================= */
/* Local function definition     */
/* ============================= */
/**
   Fills in firmware pointers.

   \param pDrvInit - handle to VINETIC_IO_INIT structure, not nil
   \param pCpuDevice - pointer to CPU device
   \param pPath - path to FW files for download

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR

   \remarks
     In case no firmware binaries is found in the file system or no file system
     is used, default binaries will be used
*/
IFX_return_t DEVICE_VINETIC_CPE_Fillin_FW_Ptr(BOARD_TYPE_t nChipType,
                                              IFX_void_t* pDrvInit,
                                              TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                              IFX_char_t* pPath)
{
   VINETIC_IO_INIT* vinit = (VINETIC_IO_INIT *) pDrvInit;
#ifdef USE_FILESYSTEM
   IFX_return_t ret;
   IFX_uint8_t* pPram = IFX_NULL;
   IFX_uint8_t* pDram = IFX_NULL;
   IFX_uint8_t* pBBD = IFX_NULL;
   IFX_char_t pszFullFilename[MAX_PATH_LEN] = {0};
   IFX_boolean_t bFoundBinFile = IFX_FALSE;
#endif /* USE_FILESYSTEM */

   TD_PTR_CHECK(vinit, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   if ((vinit->pBBDbuf != IFX_NULL) && (vinit->pPRAMfw != IFX_NULL) &&
       (vinit->pDRAMfw != IFX_NULL))
   {
      /* fw and bbd files already downloaded. If there is need to
         download new fw and bbd files, first current buffers
         must be released - DEVICE_VINETIC_CPE_Release_FW_Ptr()*/
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
           ("FW and BBD files already downloaded.\n"));
      return IFX_SUCCESS;
   }

   /* Set all entries to nil */
   memset(vinit, 0, sizeof(VINETIC_IO_INIT));

#ifdef USE_FILESYSTEM
   COM_RCV_FILES_GET(nChipType, ITM_RCV_FILE_FW,
                     &vinit->pPRAMfw, &vinit->pram_size);

   if (IFX_NULL == vinit->pPRAMfw)
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

   COM_RCV_FILES_GET(nChipType, ITM_RCV_FILE_FW_DRAM,
                     &vinit->pDRAMfw, &vinit->dram_size);
   if (IFX_NULL == vinit->pDRAMfw)
   {
      /* Reading DRAM_FW file */
      /* Reset flag */
      bFoundBinFile = IFX_FALSE;
      /* Find DRAM_FW file with default name */
      ret = Common_CreateFullFilename(pCpuDevice->pszDRAM_FW_File,
                                      pPath, pszFullFilename);
      if (IFX_SUCCESS == ret)
      {
         ret = Common_CheckFileExists(pszFullFilename);
         if (IFX_SUCCESS == ret)
         {
            /* Fill in DRAM_FW buffers from binary */
            ret = TD_OS_FileLoad(pszFullFilename, &pDram,
                                 (IFX_size_t *) &vinit->dram_size);

            /* Release DRAM_FW memory if applicable */
            if ((IFX_ERROR == ret) || (IFX_NULL == pDram))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                    ("Err, reading firmware DRAM file %s.\n"
                     "(File: %s, line: %d)\n",
                     pCpuDevice->pszDRAM_FW_File, __FILE__, __LINE__));

               if (pDram != IFX_NULL)
               {
                  TD_OS_MemFree(pDram);
                  pDram = IFX_NULL;
               }
               return IFX_ERROR;
            }
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Filesystem firmware DRAM binary used (%s).\n",
                      pszFullFilename));

               vinit->pDRAMfw = pDram;
               bFoundBinFile = IFX_TRUE;
            }
         }
      }
      if( IFX_TRUE != bFoundBinFile)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, firmware DRAM file %s is not found.\n"
                "(File: %s, line: %d)\n",
                pCpuDevice->pszDRAM_FW_File, __FILE__, __LINE__));
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
   else if (IFX_NULL == vinit->pBBDbuf)
   {
      /* Reading BBD_CRAM file */
      /* Find BBD_CRAM file with default name */
      /* First check for new name, next for old one */
      bFoundBinFile = Common_FindBBD_CRAM(pCpuDevice, pPath, pszFullFilename);
      if(IFX_TRUE == bFoundBinFile)
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

#else /* USE_FILESYSTEM */

   /* If file system support isn't used, use default PRAM/DRAM/BBD bins */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("Default firmware PRAM binaries used.\n"));
   vinit->pPRAMfw = (IFX_uint8_t *) vinetic_fw_pram_cpe;
   vinit->pram_size = sizeof(vinetic_fw_pram_cpe);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("Default firmware DRAM binaries used.\n"));
   vinit->pDRAMfw = (IFX_uint8_t *) vinetic_fw_dram_cpe;
   vinit->dram_size = sizeof(vinetic_fw_dram_cpe);

   if (IFX_FALSE != g_bBBD_Dwld)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Default BBD binaries used.\n"));
      vinit->pBBDbuf = (IFX_uint8_t *) bbd_buf_cpe;
      vinit->bbd_size = sizeof(bbd_size_cpe);
   }
   else
   {
      vinit->pBBDbuf = IFX_NULL;
      vinit->bbd_size = 0;
   }
#endif /* USE_FILESYSTEM */
   return IFX_SUCCESS;
} /* Fillin_FW_Ptr() */


/**
   Release in firmware pointers.

   \param pDrvInit - handle to VINETIC_IO_INIT structure, not nil

   \return none
*/
IFX_void_t DEVICE_VINETIC_CPE_Release_FW_Ptr(IFX_void_t* pDrvInit)
{
   VINETIC_IO_INIT* vinit = (VINETIC_IO_INIT *) pDrvInit;

   TD_PTR_CHECK(vinit,);

#ifdef USE_FILESYSTEM

   /* Pram fw was read from file, as default isn't set: free memory */
   if (vinit->pPRAMfw != IFX_NULL)
   {
      if (COM_RCV_FILES_CHECK_IF_FREE_MEM(vinit->pPRAMfw))
      {
         TD_OS_MemFree(vinit->pPRAMfw);
      }
      vinit->pPRAMfw = IFX_NULL;
   }

   /* Dram fw was read from file, as default isn't set: free memory */
   if (vinit->pDRAMfw != IFX_NULL)
   {
      if (COM_RCV_FILES_CHECK_IF_FREE_MEM(vinit->pDRAMfw))
      {
         TD_OS_MemFree(vinit->pDRAMfw);
      }
      vinit->pDRAMfw = IFX_NULL;
   }

   if (vinit->pBBDbuf != IFX_NULL)
   {
      if (COM_RCV_FILES_CHECK_IF_FREE_MEM(vinit->pBBDbuf))
      {
         TD_OS_MemFree(vinit->pBBDbuf);
      }
      vinit->pBBDbuf = IFX_NULL;
   }

#endif /* USE_FILESYSTEM */

} /* Release_FW_Ptr() */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Read version after succesfull initialization and display it.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \param pBoard - pointer to board

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t DEVICE_VINETIC_CPE_GetVersions(BOARD_t* pBoard)
{
   IFX_return_t ret;
   VINETIC_IO_VERSION vinVers;
   IFX_uint8_t* pVersionNumber;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&vinVers, 0, sizeof(VINETIC_IO_VERSION));

   ret = TD_IOCTL(pBoard->nDevCtrl_FD, FIO_VINETIC_VERS, &vinVers,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS == ret)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("VINETIC [version 0x%02X, type 0x%02X, channels %d] ready!\n",
          vinVers.nChip, vinVers.nType, vinVers.nChannel));
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("FW Version %d.%d\n",
          vinVers.nEdspVers, vinVers.nEdspIntern));

      pVersionNumber = (IFX_uint8_t*) &vinVers.nDrvVers;
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("Driver version %d.%d.%d.%d\n",
          pVersionNumber[0],
          pVersionNumber[1],
          pVersionNumber[2],
          pVersionNumber[3]));

      COM_GET_FW_VERSION_VCPE(vinVers);
   }
   else
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                      ("VINETIC version could not be read.\n"));
   }

   return ret;
} /* DEVICE_VINETIC_CPE_GetVersions() */

/**
   Basic device initialization.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

*/
IFX_return_t DEVICE_VINETIC_CPE_BasicInit(BOARD_t* pBoard)
{
   VINETIC_BasicDeviceInit_t vin_init;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t   *pCpuDevice = IFX_NULL;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if ((pDevice = Common_GetDevice_CPU(pBoard->pDevice)) == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not found. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pCpuDevice = &pDevice->uDevice.stCpu;

   /* configure vinetic device */
   memset(&vin_init, 0, sizeof(VINETIC_BasicDeviceInit_t));
   vin_init.AccessMode = pCpuDevice->AccessMode;
   vin_init.nBaseAddress = pCpuDevice->nBaseAddress;
   vin_init.nIrqNum = pCpuDevice->nIrqNum;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("Configure VINETIC device with access mode %s, "
          "base address 0x%0X, IRQ 0x%0X\n",
          VIN_ACCESS_MODES[vin_init.AccessMode],
          (int) vin_init.nBaseAddress, vin_init.nIrqNum));

   if (IFX_SUCCESS != TD_IOCTL(pBoard->nDevCtrl_FD, FIO_VINETIC_BASICDEV_INIT,
                         (IFX_int32_t) &vin_init, TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Device initialization.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t DEVICE_VINETIC_CPE_Init(BOARD_t* pBoard, IFX_char_t* pPath)
{
#ifdef TD_USE_CHANNEL_INIT
   IFX_TAPI_CH_INIT_t Init;
#endif /* TD_USE_CHANNEL_INIT */
   IFX_int32_t i = 0;
   TAPIDEMO_DEVICE_t      *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t  *pCpuDevice = IFX_NULL;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if ((pDevice = Common_GetDevice_CPU(pBoard->pDevice)) == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not found. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pCpuDevice = &pDevice->uDevice.stCpu;

#ifdef TD_USE_CHANNEL_INIT
   DEVICE_VINETIC_CPE_Fillin_FW_Ptr(pBoard->nType, pCpuDevice->pDevDependStuff,
                                    pCpuDevice, pPath);

   /* Set tapi init structure */
   memset(&Init, 0, sizeof(IFX_TAPI_CH_INIT_t));
   Init.nMode = IFX_TAPI_INIT_MODE_VOICE_CODER;
   Init.pProc = (IFX_void_t*) pCpuDevice->pDevDependStuff;
#endif /* TD_USE_CHANNEL_INIT */

   /* Initialize all tapi channels */
   for (i = 0; i <= pBoard->nMaxCoderCh; i++)
   {
      if (pBoard->nMaxCoderCh == i)
      {
         /* Read version */
         DEVICE_VINETIC_CPE_GetVersions(pBoard);
         break;
      }

#ifdef TD_USE_CHANNEL_INIT
      /* Initialize all system channels */
      if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i], IFX_TAPI_CH_INIT,
                            (IFX_int32_t) &Init,
                            TD_DEV_NOT_SET, TD_CONN_ID_INIT))
      {
         break;
      }
#endif /* TD_USE_CHANNEL_INIT */

      /* Set appropriate feeding on all (analog) line channels */
      if (i < pBoard->nMaxAnalogCh)
      {
         /* Set line in standby */
         if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i], IFX_TAPI_LINE_FEED_SET,
                               IFX_TAPI_LINE_FEED_STANDBY,
                               TD_DEV_NOT_SET, TD_CONN_ID_INIT))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, set line to standby for ch %d, (File: %s, line: %d)\n",
                  (int) i, __FILE__, __LINE__));
            break;
         }
      }
   } /* for */

   DEVICE_VINETIC_CPE_Release_FW_Ptr(pCpuDevice->pDevDependStuff);

   if (pBoard->nMaxCoderCh != i)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Init failed for ch %d, (File: %s, line: %d)\n",
            (int) i, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* DEVICE_VINETIC_CPE_Init() */

/**
   Handles the DRIVER error codes and stack.

   \param nCode - error code
   \param nSeqConnId  - Seq Conn ID

   \remarks
*/
IFX_void_t DEVICE_VINETIC_CPE_DecodeErrnoForDrv(IFX_int32_t nCode,
                                                IFX_uint32_t nSeqConnId)
{
#if 0
   /* Not supported in VINETIC at the moment. */
   IFX_int32_t i = 0;


   for (i = 0; i < VINETIC_ERRNO_CNT; ++i)
   {
      if (VINETIC_drvErrnos[i] == (nCode & 0xFFFF))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("LL Err: %s\n", VINETIC_drvErrStrings[i]));
         break;
      }
   }
#endif
}

/**
   Return last error number for vinetic driver.

   \param nFd  -  channel file descriptor

   \return number of error if successful, otherwise IFX_ERROR.
   \remarks
*/
IFX_return_t DEVICE_VINETIC_CPE_GetErrnoNum(IFX_int32_t nFd)
{
   IFX_int32_t nLastErr = 0, ret;

   ret = TD_IOCTL(nFd, FIO_VINETIC_LASTERR, (IFX_int32_t) &nLastErr,
            TD_DEV_NOT_SET, TD_CONN_ID_ERR);

   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Failed to get error number for fd %d, (File: %s, line: %d)\n",
             nFd, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return nLastErr;

}

#ifdef TD_USE_CHANNEL_INIT
/**
   Downloads fw and bbd files and calls IFX_TAPI_CH_INIT for first channel.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download
   \param hExtBoard - true if set it for extension board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks It is necessary to call this function to have working the
    IFX_TAPI_CAP_LIST.
*/
IFX_return_t DEVICE_VINETIC_CPE_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath,
                                             IFX_boolean_t hExtBoard)
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

   ret = DEVICE_VINETIC_CPE_Fillin_FW_Ptr(pBoard->nType,
                                          pCpuDevice->pDevDependStuff,
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
      /* Don't print error trace for extension board because it may not be error,
         e.g. when extension board node is created, but board isn't connected
         to the main board */
      if (!hExtBoard)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, ioctl cmd IFX_TAPI_CH_INIT, ch 0 "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
      }
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
   \param hExtBoard - true if set it for extension board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t DEVICE_VINETIC_CPE_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath,
                                             IFX_boolean_t hExtBoard)
{
   VINETIC_IO_INIT* pVineticInit;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t nDevFd = -1;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t *pCpuDevice = IFX_NULL;
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

   ret = DEVICE_VINETIC_CPE_Fillin_FW_Ptr(pBoard->nType,
                                          pCpuDevice->pDevDependStuff,
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
           ("Err, Invalid device file descriptor."
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Set tapi init structure */
   pVineticInit = (VINETIC_IO_INIT*) pCpuDevice->pDevDependStuff;
   ret = TD_IOCTL(nDevFd, FIO_VINETIC_INIT, (IFX_int32_t) pVineticInit,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS != ret)
   {
      TD_OS_DeviceClose(nDevFd);
      if (!hExtBoard)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, on ioctl cmd FIO_VINETIC_INIT. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
      return IFX_ERROR;
   }
   /* start TAPI - reserve all resources needed by TAPI
      for this specific device. */
   oStartConfig.nMode = IFX_TAPI_INIT_MODE_VOICE_CODER;
   if (IFX_SUCCESS != TD_IOCTL(nDevFd, IFX_TAPI_DEV_START,
                         (IFX_int32_t) &oStartConfig,
                         TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to start device. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      TD_OS_DeviceClose(nDevFd);
      return IFX_ERROR;
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
IFX_return_t DEVICE_VINETIC_CPE_Register(TAPIDEMO_DEVICE_CPU_t* pCpuDevice)
{
   /* check input arguments */
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   pCpuDevice->Version = DEVICE_VINETIC_CPE_GetVersions;
   pCpuDevice->DecodeErrno = DEVICE_VINETIC_CPE_DecodeErrnoForDrv;
   pCpuDevice->GetErrno = DEVICE_VINETIC_CPE_GetErrnoNum;

   pCpuDevice->FillinFwPtr = DEVICE_VINETIC_CPE_Fillin_FW_Ptr;
   pCpuDevice->ReleaseFwPtr = DEVICE_VINETIC_CPE_Release_FW_Ptr;

   return IFX_SUCCESS;
}

