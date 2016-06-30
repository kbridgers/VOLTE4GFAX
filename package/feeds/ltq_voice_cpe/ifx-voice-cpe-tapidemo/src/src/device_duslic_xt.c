/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file device_duslic_xt.c
   \date 2006-10-01
   \brief Interface implementation for DANUBE board.

   Application needs it to initialize board, prepare structures for this board
   and set mode in which board will work (using PCM, GPIO, ...).
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"
#ifdef HAVE_DRV_DUSLICXT_HEADERS
    #include "drv_dxt_errno.h"
    #include "drv_dxt_strerrno.h"
#else
    #error "drv_dxt is missing"
#endif

#include "device_duslic_xt.h"

#ifndef USE_FILESYSTEM
/** Default FW file includes, to be provided */

#include "dxt_bbd_fxs.c"
#include "dxt_firmware.c"

#endif /* USE_FILESYSTEM */

/* #warning!! temporary solution - this struct should be included. */
typedef struct
{
   /** block based download buffer,
       big-endian aligned */
   IFX_uint8_t *buf;
   /** size of buffer in bytes */
   IFX_uint32_t size;
} bbd_format_t;

/* ============================= */
/* Defines                       */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Names of access modes */
static const IFX_char_t* DXT_ACCESS_MODES[] =
{
   "DXT_ACCESS_SPI",
   "DXT_ACCESS_HD_AUDIO",
   "DXT_ACCESS_HD_AUDIO_TEST",
   "DXT_ACCESS_TESTMODE",
   IFX_NULL
};

/* ============================= */
/* Global variable definition    */
/* ============================= */


#ifdef USE_FILESYSTEM
/** Prepare file names for DXT */
IFX_char_t* sPRAMFile_DXT_DEFAULT = "voice_dxt_firmware.bin";
IFX_char_t* sPRAMFile_DXT_Old = "dxt_firmware.bin";
/** chip version dependant FW name for chip version V1.3 */
IFX_char_t* sPRAMFile_DXT_V1_3 = "dxt_v1.3_firmware.bin";
/** chip version dependant FW name for chip version V1.4 */
IFX_char_t* sPRAMFile_DXT_V1_4 = "dxt_v1.4_firmware.bin";
IFX_char_t* sDRAMFile_DXT = "";
/** File holding coefficients. */
IFX_char_t* sBBD_CRAM_File_DXT = "dxt_bbd.bin";
IFX_char_t* sBBD_CRAM_File_DXT_Old = "dxt_bbd_fxs.bin";
#endif /* USE_FILESYSTEM */

/** Device names */
IFX_char_t* sCH_DEV_NAME_DXT = "/dev/dxt";
IFX_char_t* sCTRL_DEV_NAME_DXT = "/dev/dxt";

#ifdef HAVE_DRV_EASY3201_HEADERS
IFX_char_t* sSYS_DEV_NAME_DXT = "/dev/easy3201/0";
#endif /* HAVE_DRV_EASY3201_HEADERS */

#ifdef TD_HAVE_DRV_BOARD_HEADERS
IFX_char_t* sSYS_DEV_NAME_DXT = "/dev/easy3201evs";
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

/** Holding device dependent data */
DXT_IO_Init_t DuslicXtDevInitStruct_V1_3;
/** Holding device dependent data */
DXT_IO_Init_t DuslicXtDevInitStruct_V1_4;

/** Holding FW and CRAM, BBD configuration data */
#ifdef USE_FILESYSTEM
static IFX_uint8_t* pFW_Pram_V1_3 = IFX_NULL;
static IFX_uint8_t* pFW_Pram_V1_4 = IFX_NULL;
static IFX_uint8_t* pBBD_Cram = IFX_NULL;
static IFX_size_t nFW_Pram_Size_V1_3 = 0;
static IFX_size_t nFW_Pram_Size_V1_4 = 0;
static IFX_size_t nBBD_Cram_Size = 0;
#endif /* USE_FILESYSTEM */

/** FW already available in ROM for DxT */
IFX_boolean_t bDxT_RomFW = IFX_FALSE;

/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t DEVICE_DUSLIC_XT_Fillin_FW_Ptr(BOARD_TYPE_t nChipType,
                                            IFX_void_t* pDrvInit,
                                            TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                            IFX_char_t* pPath);
IFX_void_t DEVICE_DUSLIC_XT_Release_FW_Ptr(IFX_void_t* pDrvInit);
IFX_return_t DEVICE_DUSLIC_XT_GetVersions(BOARD_t* pBoard);
IFX_void_t DEVICE_DUSLIC_XT_DecodeErrnoForDrv(IFX_int32_t nCode,
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
IFX_return_t DEVICE_DUSLIC_XT_Fillin_FW_Ptr(BOARD_TYPE_t nChipType,
                                            IFX_void_t* pDrvInit,
                                            TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                            IFX_char_t* pPath)
{
   DXT_IO_Init_t* dxt_init = (DXT_IO_Init_t *) pDrvInit;
#ifdef USE_FILESYSTEM
   IFX_return_t ret;
   IFX_char_t pszFullFilename[MAX_PATH_LEN] = {0};
   IFX_boolean_t bFoundBinFile = IFX_FALSE;
   IFX_uint8_t* pFW_PramUsed = IFX_NULL;
   IFX_size_t nFW_Pram_SizeUsed = 0;
#endif /* USE_FILESYSTEM */

   TD_PTR_CHECK(dxt_init, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   if ((dxt_init->pBBDbuf != IFX_NULL) && (dxt_init->pPRAMfw != IFX_NULL))
   {

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
           ("FW and BBD files already downloaded.\n"));
      return IFX_SUCCESS;
   }
   /* Set all entries to nil */
   memset(dxt_init, 0, sizeof(DXT_IO_Init_t));

   /* do not update FW if bDxT_RomFW is set */
   if (IFX_TRUE == bDxT_RomFW)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
             ("Setting No FW Download flag\n"));
      dxt_init->nFlags |= DXT_NO_FW_DWLD;
#ifdef DXT_NO_ASDSP_DWLD
      dxt_init->nFlags |= DXT_NO_ASDSP_DWLD;
#endif /* DXT_NO_ASDSP_DWLD */
   }

#ifdef USE_FILESYSTEM
   if (TD_CHIP_TYPE_DXT_V1_3 == pCpuDevice->nChipSubType)
   {
      pFW_PramUsed = pFW_Pram_V1_3;
      nFW_Pram_SizeUsed = nFW_Pram_Size_V1_3;
   }
   else if (TD_CHIP_TYPE_DXT_V1_4 == pCpuDevice->nChipSubType)
   {
      pFW_PramUsed = pFW_Pram_V1_4;
      nFW_Pram_SizeUsed = nFW_Pram_Size_V1_4;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, Invalid chip version!!.\n(File: %s, line: %d)\n",
          __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Did we already load FW files? This check is performed if more than
      one same type of DXT chip are in environment. */
   if ((pFW_PramUsed != IFX_NULL) && (pBBD_Cram != IFX_NULL))
   {
      dxt_init->pPRAMfw = pFW_PramUsed;
      dxt_init->pram_size = nFW_Pram_SizeUsed;
      dxt_init->pBBDbuf = pBBD_Cram;
      dxt_init->bbd_size = nBBD_Cram_Size;
      return IFX_SUCCESS;
   }

   if (pFW_PramUsed != IFX_NULL)
   {
      TD_OS_MemFree(pFW_PramUsed);
      pFW_PramUsed = IFX_NULL;
      nFW_Pram_SizeUsed = 0;
   }

   if (pBBD_Cram != IFX_NULL)
   {
      TD_OS_MemFree(pBBD_Cram);
      pBBD_Cram = IFX_NULL;
      nBBD_Cram_Size = 0;
   }

   COM_RCV_FILES_GET(nChipType, ITM_RCV_FILE_FW,
                     &pFW_PramUsed, &nFW_Pram_SizeUsed);
   if (IFX_NULL != pFW_PramUsed)
   {
      dxt_init->pPRAMfw = pFW_PramUsed;
      dxt_init->pram_size = nFW_Pram_SizeUsed;
   }
   else
   {
      /* Reading PRAM_FW file */
      /* Find PRAM_FW file with default names. */
      /* First check for new name, next for old one */
      bFoundBinFile = Common_FindPRAM_FW(pCpuDevice, pPath, pszFullFilename);
      if(IFX_TRUE == bFoundBinFile)
      {
         /* Fill in PRAM_FW buffers from binary */
         ret = TD_OS_FileLoad(pszFullFilename, &pFW_PramUsed, &nFW_Pram_SizeUsed);
         /* Release PRAM_FW memory if applicable */
         if ((IFX_ERROR == ret) || (IFX_NULL == pFW_PramUsed))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, reading firmware file %s.\n(File: %s, line: %d)\n",
                  pszFullFilename, __FILE__, __LINE__));

            if (pFW_PramUsed != IFX_NULL)
            {
               TD_OS_MemFree(pFW_PramUsed);
               pFW_PramUsed = IFX_NULL;
            }
            if (TD_CHIP_TYPE_DXT_V1_3 == pCpuDevice->nChipSubType)
            {
               pFW_Pram_V1_3 = pFW_PramUsed;
               nFW_Pram_Size_V1_3 = nFW_Pram_SizeUsed;
            }
            else if (TD_CHIP_TYPE_DXT_V1_3 == pCpuDevice->nChipSubType)
            {
               pFW_Pram_V1_4 = pFW_PramUsed;
               nFW_Pram_Size_V1_4 = nFW_Pram_SizeUsed;
            }
            return IFX_ERROR;
         }
         else
         {
            dxt_init->pPRAMfw = pFW_PramUsed;
            dxt_init->pram_size = nFW_Pram_SizeUsed;

            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Filesystem firmware binary used (%s).\n",
                  pszFullFilename));
         } /* if ((IFX_ERROR == ret_fwram) ... */
      } /* if(IFX_TRUE == bFileFound) */
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
              ("Firmware file %s not found, "
               "assuming that it is not needed for this DxT device.\n",
               pCpuDevice->pszPRAM_FW_File[0]));
      }
   }
   /* set global pointers */
   if (TD_CHIP_TYPE_DXT_V1_3 == pCpuDevice->nChipSubType)
   {
      pFW_Pram_V1_3 = pFW_PramUsed;
      nFW_Pram_Size_V1_3 = nFW_Pram_SizeUsed;
   }
   else if (TD_CHIP_TYPE_DXT_V1_3 == pCpuDevice->nChipSubType)
   {
      pFW_Pram_V1_4 = pFW_PramUsed;
      nFW_Pram_Size_V1_4 = nFW_Pram_SizeUsed;
   }

   COM_RCV_FILES_GET(nChipType, ITM_RCV_FILE_BBD_FXS,
                     &pBBD_Cram, &nBBD_Cram_Size);

   /* if BBD file downloaded for test then overwrite the BBD anyway */
   if ((IFX_FALSE == g_bBBD_Dwld) && (IFX_NULL == pBBD_Cram))
   {
      dxt_init->pBBDbuf = IFX_NULL;
      dxt_init->bbd_size = 0;
   }
   else if (IFX_NULL != pBBD_Cram)
   {
      dxt_init->pBBDbuf = pBBD_Cram;
      dxt_init->bbd_size = nBBD_Cram_Size;
   }
   else
   {
      /* Reading BBD_CRAM file */
      /* Find BBD_CRAM file with default names. */
      /* First check for new name, next for old one */
      bFoundBinFile = Common_FindBBD_CRAM(pCpuDevice, pPath, pszFullFilename);
      if(IFX_TRUE == bFoundBinFile)
      {
         /* Fill in BBD_CRAM buffers from binary */
         ret = TD_OS_FileLoad(pszFullFilename, &pBBD_Cram, &nBBD_Cram_Size);

         /* Release BBD_CRAM memory if applicable */
         if ((IFX_ERROR == ret) || (IFX_NULL == pBBD_Cram))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, reading BBD file %s.\n(File: %s, line: %d)\n",
               pszFullFilename, __FILE__, __LINE__));

            if (pBBD_Cram != IFX_NULL)
            {
               TD_OS_MemFree(pBBD_Cram);
               pBBD_Cram = IFX_NULL;
            }
            return IFX_ERROR;
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
               ("Filesystem BBD binary used (%s).\n", pszFullFilename));
            dxt_init->pBBDbuf = pBBD_Cram;
            dxt_init->bbd_size = nBBD_Cram_Size;
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

   /* If file system support isn't used then use default PRAM bins */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         "Default firmware PRAM binaries used\n"));
   dxt_init->pPRAMfw = (IFX_uint8_t *) dxt_firmware;
   dxt_init->pram_size = sizeof(dxt_firmware);

   if (IFX_FALSE != g_bBBD_Dwld)
   {
      /* If file system support isn't used then use default BBD_CRAM bins */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
             ("Default BBD binaries used\n"));
      dxt_init->pBBDbuf = (IFX_uint8_t *) dxt_bbd_buf;
      dxt_init->bbd_size = sizeof(dxt_bbd_buf);
   }
   else
   {
      dxt_init->pBBDbuf = IFX_NULL;
      dxt_init->bbd_size = 0;
   }
#endif /* USE_FILESYSTEM */

   return IFX_SUCCESS;
} /* Fillin_FW_Ptr() */


/**
   Release in firmware pointers.

   \param pDrvInit - handle to VINETIC_IO_INIT structure, not nil
   \param pBoard - pointer to board

   \return none
*/
IFX_void_t DEVICE_DUSLIC_XT_Release_FW_Ptr(IFX_void_t* pDrvInit)
{
   DXT_IO_Init_t* dxt_init = (DXT_IO_Init_t *) pDrvInit;

   TD_PTR_CHECK(dxt_init,);

#ifdef USE_FILESYSTEM
   /* Pram fw was read from file, as default isn't set: free memory */
   if (pFW_Pram_V1_3 != IFX_NULL)
   {
      if (COM_RCV_FILES_CHECK_IF_FREE_MEM(pFW_Pram_V1_3))
      {
         TD_OS_MemFree(pFW_Pram_V1_3);
      }
      pFW_Pram_V1_3 = IFX_NULL;
      nFW_Pram_Size_V1_3 = 0;
   }
   /* Pram fw was read from file, as default isn't set: free memory */
   if (pFW_Pram_V1_4 != IFX_NULL)
   {
      if (COM_RCV_FILES_CHECK_IF_FREE_MEM(pFW_Pram_V1_4))
      {
         TD_OS_MemFree(pFW_Pram_V1_4);
      }
      pFW_Pram_V1_4 = IFX_NULL;
      nFW_Pram_Size_V1_4 = 0;
   }
   dxt_init->pPRAMfw = IFX_NULL;

   if (pBBD_Cram != IFX_NULL)
   {
      if (COM_RCV_FILES_CHECK_IF_FREE_MEM(pBBD_Cram))
      {
         TD_OS_MemFree(pBBD_Cram);
      }
      pBBD_Cram = IFX_NULL;
      nBBD_Cram_Size = 0;
   }
   dxt_init->pBBDbuf = IFX_NULL;

#endif /* USE_FILESYSTEM */

} /* DEVICE_DUSLIC_XT_Release_FW_Ptr() */

/* ============================= */
/* Global function definition    */
/* ============================= */
/**
   Read chip version (HW revision) after BasicInit,
   this data will be used for FW download.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \param pBoard - pointer to board
   \param pCpuDevice - pointer to control device structure

   \remark
*/
IFX_return_t DEVICE_DUSLIC_XT_SetVersions(BOARD_t* pBoard,
                                          TAPIDEMO_DEVICE_CPU_t* pCpuDevice)
{
   IFX_return_t ret;
   IFX_int32_t nDevFd;
   DXT_IO_Version_t dxtVers;

   /* open device file descriptor */
   nDevFd = Common_Open(pBoard->pszCtrlDevName, pBoard);
   if (0 > nDevFd)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Invalid device file descriptor."
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&dxtVers, 0, sizeof(DXT_IO_Version_t));

   ret = TD_IOCTL(nDevFd, FIO_DXT_VERS, (IFX_int32_t) &dxtVers,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   TD_OS_DeviceClose(nDevFd);
   if (IFX_SUCCESS == ret)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("DXT [HW revision 0x%02X - %s]\n",
          dxtVers.nHwRev,
          ((DXT_V_A22 == dxtVers.nHwRev) ? "v1.3" :
           (DXT_V_A23 == dxtVers.nHwRev) ? "v1.4" :
           "unknown - assuming that v1.3 is used")));
   }
   else
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("DXT version could not be read.\n"));
      return IFX_ERROR;
   }
   /* if 0 detected then treat as V1.3 */
   if (0 == dxtVers.nHwRev || DXT_V_A22 == dxtVers.nHwRev)
   {
      pCpuDevice->nChipSubType = TD_CHIP_TYPE_DXT_V1_3;
      pCpuDevice->pszPRAM_FW_File[0] = sPRAMFile_DXT_V1_3;
      /** Pointer to device dependent structure */
      pCpuDevice->pDevDependStuff = (IFX_void_t *) &DuslicXtDevInitStruct_V1_3;
   }
   else if (DXT_V_A23 == dxtVers.nHwRev)
   {
      pCpuDevice->nChipSubType = TD_CHIP_TYPE_DXT_V1_4;
      pCpuDevice->pszPRAM_FW_File[0] = sPRAMFile_DXT_V1_4;
      /** Pointer to device dependent structure */
      pCpuDevice->pDevDependStuff = (IFX_void_t *) &DuslicXtDevInitStruct_V1_4;
   }
   else
   {
      pCpuDevice->nChipSubType = TD_CHIP_TYPE_UNKNOWN;
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("Err, Unhandled DxT version was read!!!.\n"));
      return IFX_ERROR;
   }

   return ret;
} /* */

/**
   Read version after succesfull initialization and display it.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \param pBoard - pointer to board

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t DEVICE_DUSLIC_XT_GetVersions(BOARD_t* pBoard)
{
   IFX_return_t ret;
   DXT_IO_Version_t vinVers;
   IFX_uint8_t* pVersionNumber;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&vinVers, 0, sizeof(DXT_IO_Version_t));

   ret = TD_IOCTL(pBoard->nDevCtrl_FD, FIO_DXT_VERS, (IFX_int32_t) &vinVers,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS == ret)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                      ("DXT [HW revision 0x%02X, ASDSP revision 0x%02X, ID 0x%02X,"
                       " fxs ch %d, fxo ch %d] ready!\n"
                       "FW Version %d\n",
                       vinVers.nHwRev, vinVers.nAsdspRev, vinVers.nDevID,
                       vinVers.nFxsCh, vinVers.nFxoCh, vinVers.nFwRev));

      pVersionNumber = (IFX_uint8_t*) &vinVers.nDrvVers;
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                      ("Driver version %d.%d.%d.%d\n",
                       pVersionNumber[0],
                       pVersionNumber[1],
                       pVersionNumber[2],
                       pVersionNumber[3]));
   }
   else
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("DXT version could not be read.\n"));
   }

   return ret;
} /* DEVICE_DUSLIC_XT_GetVersions() */

/**
   Basic device initialization.

   \param nDevFD     - device file descriptor
   \param nIrqNum    - Irq Line number (-1 for polling mode)

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

*/
IFX_return_t DEVICE_DUSLIC_XT_BasicInit(IFX_int32_t nDevFD,
                                        IFX_int32_t nIrqNum)
{
   DXT_BasicDeviceInit_t dxt_init;

   TD_PARAMETER_CHECK((0 > nDevFD), nDevFD, IFX_ERROR);

   /* Reset device data in the duslic driver */
   if (TD_IOCTL(nDevFD, FIO_DXT_DEV_RESET, 0,
          TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }

   /* configure dxt device */
   memset(&dxt_init, 0, sizeof(DXT_BasicDeviceInit_t));
   dxt_init.nIrqNum = nIrqNum;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("Configure DXT device with IRQ 0x%0X (%d) and access mode %s\n",
          dxt_init.nIrqNum, dxt_init.nIrqNum, DXT_ACCESS_MODES[0]));

   if (TD_IOCTL(nDevFD, FIO_DXT_BASICDEV_INIT, (IFX_int32_t) &dxt_init,
          TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Device initialization.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCES on ok, otherwise IFX_ERROR

   \remarks
     In case no firmware binaries is found in the file system or no file system
     is used, default binaries will be used
*/
IFX_return_t DEVICE_DUSLIC_XT_Init(BOARD_t* pBoard, IFX_char_t* pPath)
{
#ifndef TD_USE_CHANNEL_INIT
   DXT_BBD_Download_t oBBD_dwnld = {0};
#endif /* TD_USE_CHANNEL_INIT */
   IFX_TAPI_CH_INIT_t Init = {0};
   IFX_int32_t i = 0;
   TAPIDEMO_DEVICE_t      *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t  *pCpuDevice = IFX_NULL;
   IFX_int32_t nMode;
 
   TD_PTR_CHECK(pBoard, IFX_ERROR);

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
      DEVICE_DUSLIC_XT_Fillin_FW_Ptr(pBoard->nType,
                                     pCpuDevice->pDevDependStuff,
                                     pCpuDevice, pPath);

      /* Set tapi init structure */
      memset(&Init, 0, sizeof(IFX_TAPI_CH_INIT_t));
      Init.nMode = IFX_TAPI_INIT_MODE_DEFAULT;
      Init.pProc = (IFX_void_t*) pCpuDevice->pDevDependStuff;
   }

   if(g_pITM_Ctrl->oVerifySytemInit.fEnabled)
   {
      DEVICE_DUSLIC_XT_GetVersions(pBoard);
      COM_VERIFY_SYSTEM_INIT(pBoard, Init);
   }
   else
   {
#if 0
      /* Set report to low */
      TD_IOCTL(pBoard->nDevCtrl_FD, FIO_DXT_REPORT_SET, 0x1,
         TD_DEV_NOT_SET, TD_CONN_ID_INIT);

      /* Run Chip access test on DUSLIC-XT, only used for testing purposes. */
      if (pBoard->nType == TYPE_DUSLIC_XT)
      {
         if (TD_IOCTL(pBoard->nDevCtrl_FD, FIO_DXT_TCA, 0x1000,
                TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, Test Chip Access, (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
#endif /* STREAM_1_1 */

      /* Initialize all tapi channels */
      for (i = 0; i <= pBoard->nMaxAnalogCh; i++)
      {
         if (pBoard->nMaxAnalogCh == i)
         {
            /* Read version */
            DEVICE_DUSLIC_XT_GetVersions(pBoard);
            break;
         }
#ifdef TD_USE_CHANNEL_INIT
         /* Initialize all system channels */
         if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i], IFX_TAPI_CH_INIT,
                               (IFX_int32_t) &Init,
                               TD_DEV_NOT_SET, TD_CONN_ID_INIT))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, TAPI INIT on ch %d, (File: %s, line: %d)\n",
                  (int) i, __FILE__, __LINE__));
            break;
         }
#else
         /* Download BBD */
         oBBD_dwnld.buf = ((DXT_IO_Init_t*)pCpuDevice->pDevDependStuff)->pBBDbuf;
         oBBD_dwnld.size = ((DXT_IO_Init_t*)pCpuDevice->pDevDependStuff)->bbd_size;
         if (IFX_NULL != oBBD_dwnld.buf)
         {
            if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i], FIO_DXT_BBD_DOWNLOAD,
                                        &oBBD_dwnld,
                                        TD_DEV_NOT_SET, TD_CONN_ID_INIT))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Err, Failed BBD download on ch %d. (File: %s, line: %d)\n",
                      i, __FILE__, __LINE__));
               break;
            }
         }
#endif /* TD_USE_CHANNEL_INIT */
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
         /* Set appropriate feeding on all (analog) line channels */
         /* Set line in standby */
         if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i],
                               IFX_TAPI_LINE_FEED_SET, nMode,
                               TD_DEV_NOT_SET, TD_CONN_ID_INIT))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, set line to standby for ch %d, (File: %s, line: %d)\n",
                  (int) i, __FILE__, __LINE__));
            break;
         }
      } /* for */
   }

   /* DEVICE_DUSLIC_XT_Release_FW_Ptr(pCpuDevice->pDevDependStuff); */

   if(!g_pITM_Ctrl->oVerifySytemInit.fEnabled)
   {
      if (pBoard->nMaxAnalogCh != i)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Init failed for ch %d, (File: %s, line: %d)\n",
               (int) i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return IFX_SUCCESS;
} /* DEVICE_DUSLIC_XT_Init() */

/**
   Handles the DRIVER error codes and stack.

   \param nCode - error code
   \param nSeqConnId  - Seq Conn ID

   \remarks
*/
IFX_void_t DEVICE_DUSLIC_XT_DecodeErrnoForDrv(IFX_int32_t nCode,
                                              IFX_uint32_t nSeqConnId)
{
   IFX_int32_t i = 0;

   for (i = 0; i < DXT_ERRNO_CNT; ++i)
   {
      if (DXT_drvErrnos[i] == (nCode & 0xFFFF))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               (TD_LOW_LEVEL_ERR_FORMAT_STR,
                DXT_drvErrStrings[i]));
         break;
      }
   }
}

/**
   Return last error number for DuSLIC-xT driver.

   \param nFd  -  channel file descriptor

   \return number of error if successful, otherwise IFX_ERROR.

   \remarks function not implemented - no FIO_DUSLIC_XT_LASTERR ioctl
*/
IFX_return_t DEVICE_DUSLIC_XT_GetErrnoNum(IFX_int32_t nFd)
{
   IFX_int32_t nLastErr = 0;
   /* IFX_int32_t ret;

   ret = TD_IOCTL(nFd, FIO_DUSLIC_XT_LASTERR, (IFX_int32_t) &nLastErr,
            TD_DEV_NOT_SET, TD_CONN_ID_ERR);

   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Failed to get error number for fd %d, (File: %s, line: %d)\n",
             nFd, __FILE__, __LINE__));
      return IFX_ERROR;
   } */
   return nLastErr;

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
IFX_return_t DEVICE_DUSLIC_XT_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath,
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

   ret = DEVICE_DUSLIC_XT_Fillin_FW_Ptr(pBoard->nType,
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
           ("Err, Invalid channel (ch 0) file descriptor."
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Set tapi init structure */
   memset(&Init, 0, sizeof(IFX_TAPI_CH_INIT_t));
   Init.nMode = IFX_TAPI_INIT_MODE_DEFAULT;
   Init.pProc = (IFX_void_t*) pCpuDevice->pDevDependStuff;
   ret = TD_IOCTL(nCh_FD, IFX_TAPI_CH_INIT, (IFX_int32_t) &Init,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);

   if (IFX_SUCCESS != ret)
   {
      /* Don't print error trace for extension board because it may not be error,
         e.g. when extension board node is created, but board isn't connected
         to the main board */
      if (!hExtBoard)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, on ioctl cmd IFX_TAPI_CH_INIT, ch 0 "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
      }
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
IFX_return_t DEVICE_DUSLIC_XT_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath,
                                           IFX_boolean_t hExtBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t *pCpuDevice = IFX_NULL;
   IFX_int32_t nDevFd;
   IFX_TAPI_DEV_START_CFG_t oStartConfig = {0};
   DXT_FW_Download_t oDxtDownload = {0};

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

   ret = DEVICE_DUSLIC_XT_Fillin_FW_Ptr(pBoard->nType,
                                        pCpuDevice->pDevDependStuff,
                                        pCpuDevice, pPath);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Error, required binaries files are not downloaded.\n"
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* open device file descriptor */
   nDevFd = Common_Open(pBoard->pszCtrlDevName, pBoard);
   if (0 > nDevFd)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Invalid device file descriptor."
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
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

   /* Download FW */
   oDxtDownload.nEdspFlags = ((DXT_IO_Init_t*)pCpuDevice->pDevDependStuff)->nFlags;
   if (TD_CHIP_TYPE_DXT_V1_4 == pCpuDevice->nChipSubType)
   {
      oDxtDownload.nEdspFlags |= DXT_NO_ASDSP_DWLD;
   }
   oDxtDownload.pPRAMfw = ((DXT_IO_Init_t*)pCpuDevice->pDevDependStuff)->pPRAMfw;
   oDxtDownload.pram_size = ((DXT_IO_Init_t*)pCpuDevice->pDevDependStuff)->pram_size;
   if (IFX_SUCCESS != TD_IOCTL(nDevFd, FIO_DXT_FW_DOWNLOAD,
                         &oDxtDownload,
                         TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {

      /* Don't print error trace for extension board because it may not be error,
         e.g. when extension board node is created, but board isn't connected
         to the main board */
      if (!hExtBoard)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Failed FW download. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
      TD_OS_DeviceClose(nDevFd);
      return IFX_ERROR;
   }
   /* start TAPI - reserve all resources needed by TAPI
      for this specific device. */
   oStartConfig.nMode = IFX_TAPI_INIT_MODE_DEFAULT;
   if (IFX_SUCCESS != TD_IOCTL(nDevFd, IFX_TAPI_DEV_START, &oStartConfig,
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
IFX_return_t DEVICE_DUSLIC_XT_Register(TAPIDEMO_DEVICE_CPU_t* pCpuDevice)
{
   /* check input arguments */
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   pCpuDevice->Version = DEVICE_DUSLIC_XT_GetVersions;
   pCpuDevice->DecodeErrno = DEVICE_DUSLIC_XT_DecodeErrnoForDrv;
   pCpuDevice->GetErrno = DEVICE_DUSLIC_XT_GetErrnoNum;

   pCpuDevice->FillinFwPtr = DEVICE_DUSLIC_XT_Fillin_FW_Ptr;
   pCpuDevice->ReleaseFwPtr = DEVICE_DUSLIC_XT_Release_FW_Ptr;

   return IFX_SUCCESS;
}


