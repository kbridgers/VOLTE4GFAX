

/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#if defined(XT16) || defined(WITH_VXT)
/**
   \file device_vxt.c
   \date 2009-03-19
   \brief Interface implementation for vxt device.

   Application needs it to initialize device.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"
#include "device_vxt.h"
#include "ifxos_time.h"
#include "event_handling.h"

/* ============================= */
/* Defines                       */
/* ============================= */

/* ============================= */
/* Imported functions            */
/* ============================= */
#ifdef EVENT_COUNTER
void EventSeqNoCheck(BOARD_t *pBoard, IFX_TAPI_EVENT_t *pEvent);
#endif

/* ============================= */
/* Global variable definition    */
/* ============================= */

#ifdef USE_FILESYSTEM
/** File holding coefficients. */
IFX_char_t* sBBD_CRAM_File_VXT_R = "bbd_vxt_r.bin";
IFX_char_t* sBBD_CRAM_File_VXT_S = "bbd_vxt_s.bin";
IFX_char_t* sBBD_CRAM_File_VXT_P = "bbd_vxt_p.bin";
#endif /* USE_FILESYSTEM */

/** Device names */
IFX_char_t* sCTRL_DEV_NAME_VXT = "/dev/vxt";

#ifdef TD_HAVE_DRV_BOARD_HEADERS
IFX_char_t* sSYS_DEV_NAME_VXT = "/dev/easy33016";
#endif /* TD_HAVE_DRV_BOARD_HEADERS */
/** Holding device dependent data */
VXT_IO_Init_t VxtDevInitStruct = {0};

/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t DEVICE_VXT_Fillin_FW_Ptr(BOARD_TYPE_t nChipType,
                                      IFX_void_t* pDrvInit,
                                      TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                      IFX_char_t* pPath);
IFX_void_t DEVICE_VXT_Release_FW_Ptr(IFX_void_t* pDrvInit);
IFX_return_t DEVICE_VXT_GetVersions(BOARD_t* pBoard);
IFX_return_t DEVICE_VXT_Init(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_void_t DEVICE_VXT_DecodeErrnoForDrv(IFX_int32_t nCode,
                                        IFX_uint32_t nSeqConnId);
IFX_return_t DEVICE_VXT_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath);

#ifdef TD_USE_CHANNEL_INIT
IFX_return_t DEVICE_VXT_SetupChannelUseChInit(BOARD_t* pBoard,
                                              IFX_char_t* pPath);
#else /* TD_USE_CHANNEL_INIT */
IFX_return_t DEVICE_VXT_SetupChannelUseDevStart(BOARD_t* pBoard,
                                                IFX_char_t* pPath);
#endif /* TD_USE_CHANNEL_INIT */

/* ============================= */
/* Local function definition     */
/* ============================= */
/**
   Fills in firmware pointers.

   \param pDrvInit - handle to VXT_IO_Init_t structure, should not be null
   \param pCpuDevice - pointer to CPU device
   \param pPath - path to FW files for download

   \return IFX_SUCCES on ok, otherwise IFX_ERROR

   \remarks
*/
IFX_return_t DEVICE_VXT_Fillin_FW_Ptr(BOARD_TYPE_t nChipType,
                                      IFX_void_t* pDrvInit,
                                      TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                      IFX_char_t* pPath)
{
#ifdef USE_FILESYSTEM
   VXT_IO_Init_t* vinit = (VXT_IO_Init_t *) pDrvInit;
   IFX_int32_t ret_bbd = IFX_ERROR;
   static IFX_uint8_t *p_bbd = IFX_NULL;
   IFX_boolean_t bFoundBinFile = IFX_FALSE;
   IFX_char_t pszFullFilename[MAX_PATH_LEN] = {0};

   /* check input arguments */
   TD_PTR_CHECK(vinit, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   if (vinit->pBBDbuf != IFX_NULL)
   {
      /* fw and bbd files already downloaded. If there is need to
         download new fw and bbd files, first current buffers
         must be released - DEVICE_VXT_Release_FW_Ptr()*/
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
           ("FW and BBD files already downloaded.\n"));
      return IFX_SUCCESS;
   }
#ifdef VXWORKS
   pPath = "/ramDrv:0/";
#endif
   COM_RCV_FILES_GET(nChipType, ITM_RCV_FILE_BBD_FXS,
                     &vinit->pBBDbuf, &vinit->bbd_size);
   if ((IFX_FALSE == g_bBBD_Dwld) && (IFX_NULL == vinit->pBBDbuf))
   {
      vinit->pBBDbuf = IFX_NULL;
      vinit->bbd_size = 0;
   }
   else if (vinit->pBBDbuf == IFX_NULL)
   {
      /* Find BBD_CRAM file with default name */
      ret_bbd = Common_CreateFullFilename(pCpuDevice->pszBBD_CRAM_File[0],
                                          pPath, pszFullFilename);
      if (IFX_SUCCESS == ret_bbd)
      {
         /* fill in DRAM and PRAM buffers from binary */
            ret_bbd = TD_OS_FileLoad(pszFullFilename, &p_bbd,
                                     (IFX_size_t *) &vinit->bbd_size);
         /* release bbd memory if applicable */
         if (IFX_ERROR == ret_bbd)
         {
#ifdef VXWORKS
            /* RAM file system access failed, try ROM file system */
            pPath = "/romfs/";
            ret_bbd = Common_CreateFullFilename(pCpuDevice->pszBBD_CRAM_File[0],
                                            pPath, pszFullFilename);

            if (IFX_SUCCESS == ret_bbd)
            {
               /* fill in DRAM and PRAM buffers from binary */
               ret_bbd = TD_OS_FileLoad(pszFullFilename, &p_bbd,
                                        (IFX_size_t *) &vinit->bbd_size);
               /* release bbd memory if applicable */
               if (IFX_ERROR == ret_bbd)
               {
#endif
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                        ("Err, reading file %s. (File: %s, line: %d)\n",
                         pCpuDevice->pszBBD_CRAM_File[0], __FILE__, __LINE__));

                  if (p_bbd != IFX_NULL)
                  {
                     TD_OS_MemFree(p_bbd);
                     p_bbd = IFX_NULL;
                  }
#ifdef VXWORKS
               } else if (p_bbd != IFX_NULL)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                         ("Filesystem BBD binary used (%s%s)\n",
                          pPath, pCpuDevice->pszBBD_CRAM_File[0]));
                  vinit->pBBDbuf = p_bbd;
                  bFoundBinFile = IFX_TRUE;
               }
            }
#endif
         }
         else if (p_bbd != IFX_NULL)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
                   ("Filesystem BBD binary used (%s%s)\n",
                    pPath, pCpuDevice->pszBBD_CRAM_File[0]));
            vinit->pBBDbuf = p_bbd;
               bFoundBinFile = IFX_TRUE;
         }
      }
      if (!bFoundBinFile)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, BBD file %s is not found.\n(File: %s, line: %d)\n",
                  pCpuDevice->pszBBD_CRAM_File[0], __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
#endif /* USE_FILESYSTEM */

  return IFX_SUCCESS;
}


/**
   Release firmware pointers.

   \param pDrvInit - handle to VXT_IO_Init_t structure

   \return none

   \remarks
*/
IFX_void_t DEVICE_VXT_Release_FW_Ptr(IFX_void_t* pDrvInit)
{
#if 1 /* debug RMi */
   VXT_IO_Init_t* vinit = (VXT_IO_Init_t *) pDrvInit;

   /* check input arguments */
   TD_PTR_CHECK(vinit,);

#ifdef USE_FILESYSTEM
   if (vinit->pBBDbuf != IFX_NULL)
   {
      if (COM_RCV_FILES_CHECK_IF_FREE_MEM(vinit->pBBDbuf))
      {
         TD_OS_MemFree(vinit->pBBDbuf);
      }
      vinit->pBBDbuf = IFX_NULL;
   }
#endif /* USE_FILESYSTEM */
#endif
} /* DEVICE_VXT_Release_FW_Ptr() */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Read version after succesfull initialization and display it.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark for this device function not implemented
*/
IFX_return_t DEVICE_VXT_GetVersions(BOARD_t* pBoard)
{
   return Common_DevVersionGet(pBoard, TD_CONN_ID_INIT);
} /* DEVICE_VXT_GetVersions() */

/**
   Init device.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t DEVICE_VXT_Init(BOARD_t* pBoard, IFX_char_t* pPath)
{
   IFX_TAPI_CH_INIT_t Init;
   IFX_TAPI_LINE_FEED_t lineFeed;
   IFX_TAPI_RING_CADENCE_t ring;
   IFX_int32_t i = 0;
   /* pattern of 3 s ring and 1 s pause */
   char data[10] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0x00,0x00 };
   /* initial 1 s ring and 1,8 s non ringing signal before ringing */
   char initial[7] ={0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00};
   IFX_return_t ret;
   IFX_TAPI_CALIBRATION_t calibration;
   IFX_TAPI_EVENT_t event;
   /* used to check if previous part of initialization was successfull */
   IFX_int32_t* pChannelTable;
   TAPIDEMO_DEVICE_t      *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t  *pCpuDevice = IFX_NULL;

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

#ifndef TD_USE_CHANNEL_INIT
   /* structure must be set for verify system initialization modular test */
   if(g_pITM_Ctrl->oVerifySytemInit.fEnabled)
#endif /* TD_USE_CHANNEL_INIT */
   {
      DEVICE_VXT_Fillin_FW_Ptr(pBoard->nType, pCpuDevice->pDevDependStuff,
                               pCpuDevice, pPath);

      /* Set tapi init structure */
      memset(&Init, 0, sizeof(IFX_TAPI_CH_INIT_t));
      Init.nMode = IFX_TAPI_INIT_MODE_PCM_PHONE;
      Init.pProc = (IFX_void_t*) pCpuDevice->pDevDependStuff;
   }

   if(g_pITM_Ctrl->oVerifySytemInit.fEnabled)
   {
      /* Read version */
      DEVICE_VXT_GetVersions(pBoard);
      COM_VERIFY_SYSTEM_INIT(pBoard, Init);
   }
   else
   {
      /* allocate memmory */
      pChannelTable = (IFX_int32_t*)TD_OS_MemCalloc(pBoard->nMaxAnalogCh,
                                           sizeof(IFX_int32_t));
      TD_PTR_CHECK(pChannelTable, IFX_ERROR);

      memset (&ring, 0, sizeof(IFX_TAPI_RING_CADENCE_t));
      /* set size in bits */
      memcpy (&ring.data[0], data, sizeof (data));
      ring.nr = sizeof(data) * 8;

      /* set the ring sequence */
      memcpy (&ring.initial[0], initial, sizeof (initial));
      ring.initialNr = sizeof(initial) * 8;

      /* Initialize all tapi channels */
      for (i = 0; i <= pBoard->nMaxAnalogCh; i++)
      {
         /* after all channels are initialized */
         if (pBoard->nMaxAnalogCh == i)
         {
            /* Read version */
            DEVICE_VXT_GetVersions(pBoard);
            break;
         }

#ifdef TD_USE_CHANNEL_INIT
         /* Initialize all system channels */
         Init.dev = pBoard->rgoPhones[i].nDev;
         Init.ch = pBoard->rgoPhones[i].nPhoneCh;
         ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CH_INIT,
                  (IFX_int32_t) &Init, Init.dev, TD_CONN_ID_INIT);
         if (IFX_SUCCESS != ret)
         {
            pChannelTable[i] = IFX_ERROR;
         }
         else
#endif /* TD_USE_CHANNEL_INIT */
         {
            pChannelTable[i] = IFX_SUCCESS;
         }
      } /* for all tapi channels*/
      if (IFX_SUCCESS != EVENT_CalibrationEnable(pBoard, TD_CONN_ID_INIT))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, %s failed to enable detection of Calibration Events. "
                "(File: %s, line: %d)\n",
                pBoard->pszBoardName, __FILE__, __LINE__));
      }
      /* Start analog line calibration process for all tapi channels */
      for (i = 0; i < pBoard->nMaxAnalogCh; i++)
      {
         /* if previous step of initialiation failed then skip this one */
         if (IFX_ERROR == pChannelTable[i])
         {
            continue;
         }
         memset(&calibration, 0, sizeof(IFX_TAPI_CALIBRATION_t));
         /* Calibrate all system channels */
         calibration.dev = pBoard->rgoPhones[i].nDev;
         calibration.ch = pBoard->rgoPhones[i].nPhoneCh;
         ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CALIBRATION_START,
                  (IFX_int32_t) &calibration, calibration.dev, TD_CONN_ID_INIT);
         if (IFX_SUCCESS != ret)
         {
            pChannelTable[i] = IFX_ERROR;;
         }
      } /* for all tapi channels*/
      /* wait for calibration process end */
      IFXOS_MSecSleep(SVIP_XT16_WAIT_CALIBRATION_POLLTIME * pBoard->nMaxAnalogCh);
      /* Check if callibration end event was received for all tapi channels */
      for (i = 0; i < pBoard->nMaxAnalogCh; i++)
      {
         /* if previous step of initialiation failed then skip this one */
         if (IFX_ERROR == pChannelTable[i])
         {
            continue;
         }
         /* check if callibration end event was received */
         do
         {
            memset(&event, 0, sizeof(IFX_TAPI_EVENT_t));
            event.ch = pBoard->rgoPhones[i].nPhoneCh;
            event.dev = pBoard->rgoPhones[i].nDev;
            ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_GET,
                     (IFX_int32_t) &event, event.dev, TD_CONN_ID_INIT);
            if (IFX_TAPI_EVENT_NONE == event.id)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Err, Didn't receive \"calibration end\" event for "
                      "dev %d ch %d - line mode will not be set. "
                      "(File: %s, line: %d)\n",
                      pBoard->rgoPhones[i].nDev, pBoard->rgoPhones[i].nPhoneCh,
                      __FILE__, __LINE__));
               pChannelTable[i] = IFX_ERROR;
            }
#ifdef EVENT_COUNTER
            else
            {
               EventSeqNoCheck(pBoard, &event);
            }
#endif /* EVENT_COUNTER */
            if ((IFX_TAPI_EVENT_CALIBRATION_END == event.id) &&
                (IFX_TAPI_EVENT_CALIBRATION_SUCCESS != event.data.calibration))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Err, Received \"calibration end\" event with error for "
                      "dev %d ch %d. "
                      "(File: %s, line: %d)\n",
                      event.dev, event.ch, __FILE__, __LINE__));
               pChannelTable[i] = IFX_ERROR;
            }
         }
         while ((IFX_TAPI_EVENT_CALIBRATION_END != event.id) &&
                (IFX_TAPI_EVENT_NONE != event.id));

      } /* for all tapi channels*/
      /* Set appropriate feeding on all (analog) line channels */
      for (i = 0; i < pBoard->nMaxAnalogCh; i++)
      {
         /* if previous step of initialiation failed then skip this one */
         if (IFX_ERROR == pChannelTable[i])
         {
            continue;
         }
         /* Set appropriate feeding on all (analog) line channels */
         /* Set line in standby */
         lineFeed.dev = pBoard->rgoPhones[i].nDev;
         lineFeed.ch = pBoard->rgoPhones[i].nPhoneCh;
         lineFeed.lineMode = IFX_TAPI_LINE_FEED_STANDBY;
         ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_LINE_FEED_SET,
                  (IFX_int32_t) &lineFeed, lineFeed.dev, TD_CONN_ID_INIT);
         if (IFX_SUCCESS != ret)
         {
            continue;
         }
         ring.dev = pBoard->rgoPhones[i].nDev;
         ring.ch = pBoard->rgoPhones[i].nPhoneCh;
         ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_RING_CADENCE_HR_SET,
                  (IFX_int32_t) &ring, ring.dev, TD_CONN_ID_INIT);
         if (ret != IFX_SUCCESS)
         {
            continue;
         }
         IFXOS_MSecSleep(100);
      } /* for all tapi channels*/

      /* release allocated memory */
      TD_OS_MemFree(pChannelTable);
   }

   DEVICE_VXT_Release_FW_Ptr(pCpuDevice->pDevDependStuff);

   if(!g_pITM_Ctrl->oVerifySytemInit.fEnabled)
   {
      if (pBoard->nMaxAnalogCh != i)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Init failed for dev %d, ch %d, (File: %s, line: %d)\n",
               pBoard->rgoPhones[i].nDev, pBoard->rgoPhones[i].nPhoneCh,
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return IFX_SUCCESS;
} /* DEVICE_VXT_Init() */

/**
   Handles the DRIVER error codes and stack.

   \param nCode - error code
   \param nSeqConnId  - Seq Conn ID

   \remarks
*/
IFX_void_t DEVICE_VXT_DecodeErrnoForDrv(IFX_int32_t nCode,
                                        IFX_uint32_t nSeqConnId)
{
   IFX_int32_t i = 0;

   for (i = 0; i < VXT_ERRNO_CNT; ++i)
   {
      if (VXT_drvErrnos[i] == (nCode & 0xFFFF))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
              ("LL Err: %s\n", VXT_drvErrStrings[i]));
         break;
      }
   }
} /* DEVICE_VXT_DecodeErrnoForDrv() */

/**
   Return last error number for driver.

   \param nFd  -  channel file descriptor

   \return number of error if successful, otherwise IFX_ERROR.
   \remarks
*/
IFX_return_t DEVICE_VXT_GetErrnoNum(IFX_int32_t nFd)
{
   return IFX_SUCCESS;
}

/**
   Starts TAPI.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks It is necessary to call this function to have working the
    IFX_TAPI_CAP_LIST.
*/
IFX_return_t DEVICE_VXT_setupChannel(BOARD_t* pBoard, IFX_char_t* pPath)
{
#ifdef TD_USE_CHANNEL_INIT
   return DEVICE_VXT_SetupChannelUseChInit(pBoard, pPath);
#else
   return DEVICE_VXT_SetupChannelUseDevStart(pBoard, pPath);
#endif /* TD_USE_CHANNEL_INIT */
} /* DEVICE_VXT_setupChannel() */

#ifdef TD_USE_CHANNEL_INIT
/**
   Calls IFX_TAPI_CH_INIT for first channel.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks It is necessary to call this function to have working the
    IFX_TAPI_CAP_LIST.
*/
IFX_return_t DEVICE_VXT_SetupChannelUseChInit(BOARD_t* pBoard, IFX_char_t* pPath)
{
   IFX_return_t ret;
   IFX_TAPI_CH_INIT_t Init;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t *pCpuDevice = IFX_NULL;
   IFX_int32_t i;

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
#ifdef USE_FILESYSTEM
   /* get BBD */
   ret = DEVICE_VXT_Fillin_FW_Ptr(pBoard->nType, pCpuDevice->pDevDependStuff,
                                  pCpuDevice, pPath);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, required binaries files are not downloaded.\n"
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* USE_FILESYSTEM */
#ifdef TAPI_VERSION4
   /* reset TAPI for each device */
   for (i = 0; i < pBoard->nChipCnt; ++i)
#endif /* TAPI_VERSION4 */
   {
      /* Set tapi init structure */
      memset(&Init, 0, sizeof(IFX_TAPI_CH_INIT_t));
#ifdef TAPI_VERSION4
      Init.dev = i;
      Init.ch = 0;
#endif /* TAPI_VERSION4 */
      Init.nMode = IFX_TAPI_INIT_MODE_PCM_PHONE;
      Init.pProc = (IFX_void_t*) pCpuDevice->pDevDependStuff;

      ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CH_INIT,
               (IFX_int32_t) &Init, Init.dev, TD_CONN_ID_INIT);
      if (IFX_SUCCESS != ret)
      {
         if (0 != Init.dev)
         {
            /* if first device was successfuly initialized
               then on error skip the rest */
            pBoard->nChipCnt = nDevTmp;
            continue;
         }
         return IFX_ERROR;
      }
   }

   return IFX_SUCCESS;
}

#else /* TD_USE_CHANNEL_INIT */

/**
   Calls IFX_TAPI_CH_INIT for first channel.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks It is necessary to call this function to have working the
    IFX_TAPI_CAP_LIST.
*/
IFX_return_t DEVICE_VXT_SetupChannelUseDevStart(BOARD_t* pBoard, IFX_char_t* pPath)
{
   IFX_return_t ret = IFX_SUCCESS;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t *pCpuDevice = IFX_NULL;
#ifdef USE_FILESYSTEM
   VXT_IO_BBD_t oBBD = {0};
   IFX_TAPI_VERSION_CH_ENTRY_t versionChEntry;
   IFX_char_t sSlickName[] = "SmartSLIC-";
#endif /* USE_FILESYSTEM */
   IFX_uint32_t i;
   IFX_TAPI_DEV_START_CFG_t oParamConfig = {0};
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

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

#ifdef TAPI_VERSION4
   /* reset TAPI for each device */
   for (i = 0; i < pBoard->nChipCnt; ++i)
#endif /* TAPI_VERSION4 */
   {
      /* start TAPI - reserve all resources needed by TAPI
         for this specific device. */
      memset(&oParamConfig, 0, sizeof(IFX_TAPI_DEV_START_CFG_t));
      oParamConfig.nMode = IFX_TAPI_INIT_MODE_PCM_PHONE;
#ifdef TAPI_VERSION4
      oParamConfig.dev = i;
      nDevTmp = oParamConfig.dev;
#endif /* TAPI_VERSION4 */

      ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_DEV_START,
               &oParamConfig, nDevTmp, TD_CONN_ID_INIT);

      if (IFX_SUCCESS != ret )
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Failed to start device %d. (File: %s, line: %d)\n",
                nDevTmp, __FILE__, __LINE__));
         if (0 != nDevTmp)
         {
            /* if first device was successfuly initialized
               then on error skip the rest */
            pBoard->nChipCnt = nDevTmp;
            continue;
         }
         return IFX_ERROR;
      }
#ifdef USE_FILESYSTEM
      /* Choose BBD file according to SLIC type */
      memset (&versionChEntry, 0, sizeof (IFX_TAPI_VERSION_CH_ENTRY_t));
#ifdef TAPI_VERSION4
      versionChEntry.dev = i;
      nDevTmp = versionChEntry.dev;
#endif /* TAPI_VERSION4 */

      ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_VERSION_CH_ENTRY_GET,
               &versionChEntry, nDevTmp, TD_CONN_ID_INIT);
      if (IFX_SUCCESS != ret )
      {
         return IFX_ERROR;
      }
      ret = memcmp (sSlickName, versionChEntry.versionEntry.name, (sizeof (sSlickName)-1));
      if (ret != 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Unknown SLIC type. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;

      }
      switch (versionChEntry.versionEntry.name[10])
      {
      case 'R':
         pCpuDevice->pszBBD_CRAM_File[0] = sBBD_CRAM_File_VXT_R;
         break;
      case 'S':
         pCpuDevice->pszBBD_CRAM_File[0] = sBBD_CRAM_File_VXT_S;
         break;
      case 'P':
         pCpuDevice->pszBBD_CRAM_File[0] = sBBD_CRAM_File_VXT_P;
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Unknown SLIC type. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get BBD */
      ret = DEVICE_VXT_Fillin_FW_Ptr(pBoard->nType, pCpuDevice->pDevDependStuff,
                                     pCpuDevice, pPath);
      if (ret == IFX_ERROR)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, required binaries files are not downloaded.\n"
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* set BBD structure - bBroadcast = IFX_TRUE to download BBD for
         all channels */
      oBBD.bBroadcast = IFX_TRUE;
      oBBD.buf = ((VXT_IO_Init_t*)pCpuDevice->pDevDependStuff)->pBBDbuf;
      oBBD.size = ((VXT_IO_Init_t*)pCpuDevice->pDevDependStuff)->bbd_size;
#ifdef TAPI_VERSION4
      oBBD.dev = i;
      nDevTmp = oBBD.dev;
#endif /* TAPI_VERSION4 */
      if (IFX_NULL != oBBD.buf)
      {
         /* download BBD */
         if (IFX_SUCCESS != TD_IOCTL(pBoard->nDevCtrl_FD, FIO_VXT_BBD_DOWNLOAD,
                               &oBBD, nDevTmp, TD_CONN_ID_INIT))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, Failed to download BBD. (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
#endif /* USE_FILESYSTEM */
   }

   return IFX_SUCCESS;
} /* DEVICE_VXT_SetupChannelUseDevStart() */
#endif /* defined(TD_USE_CHANNEL_INIT */

/**
   Register device to the board.

   \param pCpuDevice - pointer to device

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
*/
IFX_return_t DEVICE_VXT_Register(TAPIDEMO_DEVICE_CPU_t* pCpuDevice)
{
   /* check input arguments */
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   pCpuDevice->Version = DEVICE_VXT_GetVersions;
   pCpuDevice->DecodeErrno = DEVICE_VXT_DecodeErrnoForDrv;
   pCpuDevice->GetErrno = DEVICE_VXT_GetErrnoNum;

   pCpuDevice->FillinFwPtr = DEVICE_VXT_Fillin_FW_Ptr;
   pCpuDevice->ReleaseFwPtr = DEVICE_VXT_Release_FW_Ptr;

   return IFX_SUCCESS;
}

#endif /* defined(XT16) || defined(WITH_VXT) */
