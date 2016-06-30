/*******************************************************************************
                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

***************************************************************************** */

/**
   \file board_xt16.c
   \date 2008-05-13
   \brief Interface implementation for xT-16 board.

   Application needs it to initialize board, prepare structures for this board
   and set mode in which board will work (using PCM, GPIO, ...).
*/

#ifdef LINUX
#include "tapidemo_config.h"
#endif

#if (defined WITH_VXT) || (defined XT16)

#include "tapidemo.h"
#include "board_xt16.h"
#ifdef EASY336
   #include "board_easy336.h"
#endif

#include "common.h"
#include "pcm.h"

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Defines                       */
/* ============================= */


/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Name of board */
static const char* BOARD_NAME_XT_16 = "xT-16";

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */

IFX_return_t BOARD_XT_16_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t BOARD_XT_16_PrepareNames(BOARD_t* pBoard);
IFX_return_t BOARD_XT_16_GetCapabilities(BOARD_t* pBoard,
                                         IFX_char_t* pPath);
IFX_void_t BOARD_XT_16_RemoveBoard(BOARD_t* pBoard);
IFX_return_t BOARD_XT_16_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg);

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Prepare board structure (reserve memory, initialize, ...).

   \param pBoard - pointer to Board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called after system inicialization.
*/
IFX_return_t BOARD_XT_16_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath)
{
   IFX_int32_t i = 0;
   IFX_return_t ret = IFX_SUCCESS;
   TAPIDEMO_DEVICE_t* pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t* pCPUDevice = IFX_NULL;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);


   /* File descriptors not open */
   pBoard->nSystem_FD = -1;
   pBoard->nDevCtrl_FD = -1;
#ifdef TD_HAVE_DRV_BOARD_HEADERS
   pBoard->nUseSys_FD = IFX_TRUE;
#else /* TD_HAVE_DRV_BOARD_HEADERS */
   pBoard->nUseSys_FD = IFX_FALSE;
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

   /* allocate memory for CPU device structure */
   pDevice = TD_OS_MemCalloc(1, sizeof(TAPIDEMO_DEVICE_t));
   if (pDevice == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Allocate memory for CPU Device. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Init structure for CPU device and add it to the board */
   pDevice->nType = TAPIDEMO_DEV_CPU;
   pCPUDevice = &pDevice->uDevice.stCpu;
   /** Pointer to device dependent structure */
   pCPUDevice->pDevDependStuff = (IFX_void_t *) &VxtDevInitStruct;

   pCPUDevice->AccessMode = SYSTEM_AM_DEFAULT_XT16;

#ifdef USE_FILESYSTEM
   /** FW files */
   pCPUDevice->pszBBD_CRAM_File[0] = sBBD_CRAM_File_VXT_R;
   pCPUDevice->pszBBD_CRAM_File[1] = "";
   pCPUDevice->pszDRAM_FW_File = "";
   pCPUDevice->pszPRAM_FW_File[0] = "";
   pCPUDevice->pszPRAM_FW_File[1] = "";
   pCPUDevice->nPRAM_FW_FileNum = 0;
   pCPUDevice->bPRAM_FW_FileOptional = IFX_FALSE;
   pBoard->fBBD_Changed = IFX_FALSE;
   /*Set FW/BBD/DRAM filenames according to proper arguments.*/
   Common_SetFwFilenames(pBoard, pCPUDevice);

#ifndef VXWORKS
#ifdef WITH_VXT
   /* We don't need to set download path. It was done during inititialize main board */
   /* Just check if directory contains required file for extension board*/
   if (IFX_SUCCESS != Common_CheckDownloadPath(pPath, pCPUDevice, IFX_TRUE,
                                               TD_CONN_ID_INIT))
   {
      return IFX_ERROR;
   }
#else
   if (IFX_ERROR ==  Common_SetDownloadPath(pPath, pBoard, pCPUDevice))
   {
      /* Can't set download path */
      TD_OS_MemFree(pDevice);
      return IFX_ERROR;
   }
#endif /* WITH_VXT */
#endif /* VXWORKS */
#endif /* USE_FILESYSTEM */

   if (Common_AddDevice(pBoard, pDevice) == IFX_ERROR)
   {
      TD_OS_MemFree(pDevice);
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not added to the board. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (DEVICE_VXT_Register(pCPUDevice) == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not registered. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (BOARD_XT_16_PrepareNames(pBoard) == IFX_ERROR)
      return IFX_ERROR;


#ifdef TAPI_VERSION4
   /** file descriptor mode */
   pBoard->fSingleFD = IFX_TRUE;

   /** RTP streaming mode */
   pBoard->fUseSockets = IFX_FALSE;
#endif

   /* set global fds */
   /* for SVIP this set only device FD, channel FDs aren't used */
   if (IFX_SUCCESS != Common_Set_FDs(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, setting fds. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      Common_Close_FDs(pBoard);
      return IFX_ERROR;
   }
#ifdef TD_HAVE_DRV_BOARD_HEADERS
   if (PCM_ConfigBoardDrv(
          pBoard->pCtrl->pProgramArg->oArgFlags.nPCM_Master ?
          IFX_TRUE : IFX_FALSE, pBoard,
          pBoard->pCtrl->pProgramArg->oArgFlags.nPCM_Loop ?
          IFX_TRUE : IFX_FALSE) != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, PCM config with board driver failed. (File: %s, line: %d)\n",
         __FILE__, __LINE__));
      return IFX_ERROR;
   }
#else  /* TD_HAVE_DRV_BOARD_HEADERS */
#ifdef XT16
   if (
       (pBoard->pCtrl->pProgramArg->oArgFlags.nPCM_Master &&
        !pBoard->pCtrl->pProgramArg->oArgFlags.nPCM_Loop) ||
       (pBoard->pCtrl->pProgramArg->oArgFlags.nPCM_Slave)
       )
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Warning: PCM set to master/slave without board driver support,\n"
          "         without it additional PCM configuration is needed\n"
          "         to make PCM external calls.\n"));
   }
#endif /* XT16 */
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

   Common_SetChipCount(pBoard);
   if (pBoard->nChipCnt == 0)
   {
      /* xT16 is not present */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
              ("%s is not present.\n", pBoard->pszBoardName));
      return IFX_SUCCESS;
   }

   if (IFX_ERROR == BOARD_XT_16_GetCapabilities(pBoard, pPath))
   {
      return IFX_ERROR;
   }

   if (IFX_ERROR == Common_SetPhonesAndUsedChannlesNumber(pBoard,
                                                          TD_CONN_ID_INIT))
   {
      return IFX_ERROR;
   }

   if (pBoard->nMaxCoderCh > 0)
   {
      pBoard->pDataChStat = TD_OS_MemCalloc(pBoard->nMaxCoderCh,
                                            sizeof(VOIP_DATA_CH_t));

      if (IFX_NULL == pBoard->pDataChStat)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Allocate memory to prepare board. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
#ifdef TAPI_VERSION4
      if (!pBoard->fSingleFD)
#endif /* TAPI_VERSION4 */
      {
         pBoard->nCh_FD = TD_OS_MemCalloc(pBoard->nUsedChannels,
                                          sizeof(IFX_int32_t));
         if (pBoard->nCh_FD == IFX_NULL)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, Allocate memory to prepare board. (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
   }

   if (pBoard->nMaxAnalogCh > 0)
   {
      pBoard->rgoPhones = TD_OS_MemCalloc(pBoard->nMaxPhones, sizeof(PHONE_t));
      if (pBoard->rgoPhones == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Allocate memory to prepare board. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   /* memory allocated only for compability with other boards
      TODO: memeber pBoard->fPCM_ChFree should be removed for SVIP with XT16 */
   if (0 < pBoard->nMaxPCM_Ch)
   {
      /* Prepare array of free PCM channels  */
      pBoard->fPCM_ChFree = TD_OS_MemCalloc(pBoard->nMaxPCM_Ch,
                                            sizeof(IFX_boolean_t));

      if (pBoard->fPCM_ChFree == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, allocate memory for PCM channels. "
                "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* All PCM channels are free at start */
      for (i = 0; i < pBoard->nMaxPCM_Ch; i++)
      {
          pBoard->fPCM_ChFree[i] = IFX_TRUE;
      }
   }
   ret = BOARD_XT_16_FillChannels(pBoard);

   pBoard->pChCustomMapping = IFX_NULL;

   COM_MOD_VERIFY_SYSTEM_INIT_PREPARE(pBoard->pCtrl);
   return ret;
}

/**
   Prepare device/channels names.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_XT_16_PrepareNames(BOARD_t* pBoard)
{
   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
#ifdef TD_HAVE_DRV_BOARD_HEADERS
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sSYS_DEV_NAME_VXT)+1),
                      strlen(sSYS_DEV_NAME_VXT), IFX_ERROR);
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

   /** Device control name numbers, like /dev/vmmc[board id]0, ... */
   strncpy(pBoard->pszCtrlDevName, sCTRL_DEV_NAME_VXT,
           strlen(sCTRL_DEV_NAME_VXT));
#ifdef TD_HAVE_DRV_BOARD_HEADERS
   /** System device name, like /dev/easy3201/[<board id], ... */
   /** BUT actually we only have one */
   memset(pBoard->pszSystemDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszSystemDevName, sSYS_DEV_NAME_VXT,
           strlen(sSYS_DEV_NAME_VXT));
   pBoard->pszSystemDevName[strlen(sSYS_DEV_NAME_VXT)] = '\0';
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

   return IFX_SUCCESS;
}

/**
   Get capabilities, basically number of phone, data and pcm channels.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_XT_16_GetCapabilities(BOARD_t* pBoard,
                                         IFX_char_t* pPath)
{
   IFX_return_t ret = IFX_ERROR;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   /* init first channel */
   ret = DEVICE_VXT_setupChannel(pBoard, pPath);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, DEVICE_VXT_setupChannel failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   ret = Common_GetCapabilities(pBoard, TD_CONN_ID_INIT);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Capabilities not read. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
   }

   return ret;
}

#ifndef STREAM_1_1
/**
   Configure PCM interface, can define clock, master/slave mode, ...

   \param pBoard  - pointer to board
   \param hMaster  - IFX_TRUE - master mode, IFX_FALSE - slave mode

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR

   \remark
*/
IFX_return_t BOARD_XT_16_CfgPCMInterface(BOARD_t* pBoard,
                                         IFX_boolean_t hMaster)
{
   IFX_int32_t i, j;
   IFX_TAPI_PCM_IF_CFG_t pcm_if;
   IFX_return_t ret;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&pcm_if, 0, sizeof(IFX_TAPI_PCM_IF_CFG_t));
   /* set PCM configuration */
#ifdef TD_HAVE_DRV_BOARD_HEADERS
   pcm_if.nDCLFreq =  IFX_TAPI_PCM_IF_DCLFREQ_2048;
#else /* TD_HAVE_DRV_BOARD_HEADERS */
   pcm_if.nDCLFreq =  IFX_TAPI_PCM_IF_DCLFREQ_4096;
#endif /* TD_HAVE_DRV_BOARD_HEADERS */
   pcm_if.nDoubleClk = IFX_DISABLE;
   pcm_if.nSlopeTX = IFX_TAPI_PCM_IF_SLOPE_RISE;
   pcm_if.nSlopeRX = IFX_TAPI_PCM_IF_SLOPE_FALL;
   pcm_if.nOffsetTX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nOffsetRX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nDrive = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   pcm_if.nShift = IFX_DISABLE;

   for (i = 0; i < pBoard->nChipCnt; i++)
   {
      /* xT-16 can't be a PCM master */
      pcm_if.nOpMode = IFX_TAPI_PCM_IF_MODE_SLAVE_AUTOFREQ;
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("%s: Configure PCM interface as slave %s kHz on %s dev %d\n",
             pBoard->pszBoardName,
             TAPIDEMO_PCM_FREQ_STR[pcm_if.nDCLFreq], pBoard->pszBoardName, i));

#ifdef TAPI_VERSION4
      pcm_if.dev = i;
#endif /* TAPI_VERSION4 */

      for (j = 0; j < RM_XT16_NUM_HIGHWAYS; j++)
      {
         pcm_if.nHighway = j;
         ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_PCM_IF_CFG_SET,
                        (IFX_int32_t) &pcm_if, TD_DEV_NOT_SET, TD_CONN_ID_INIT);
         if (ret != IFX_SUCCESS)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, configure PCM interface on board - %d. "
                  "(File: %s, line: %d)\n",
                  j, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      } /* for all highways */
   } /* for all chips/devices */
   /* set PCM frequency */
   g_oPCM_Data.nPCM_Rate = pcm_if.nDCLFreq;

   return IFX_SUCCESS;
} /* BOARD_XT_16_CfgPCMInterface() */

#else /* !STREAM_1_1 */

IFX_return_t BOARD_XT_16_CfgPCMInterface(BOARD_t* pBoard,
                                           IFX_boolean_t hMaster)
{
   return IFX_SUCCESS;
} /* PCM_Init() */
#endif /* !STREAM_1_1 */

/**
   Release board structure (reserve memory, inicalize, ...).

   \param pBoard - pointer to board

   \remark Must be called before application exit.
*/
IFX_void_t BOARD_XT_16_RemoveBoard(BOARD_t* pBoard)
{
   if (pBoard->pDataChStat != IFX_NULL)
   {
      TD_OS_MemFree(pBoard->pDataChStat);
      pBoard->pDataChStat = IFX_NULL;
   }

   if (pBoard->rgoPhones != IFX_NULL)
   {
      TD_OS_MemFree(pBoard->rgoPhones);
      pBoard->rgoPhones = IFX_NULL;
   }

   if (pBoard->fPCM_ChFree != IFX_NULL)
   {
      TD_OS_MemFree(pBoard->fPCM_ChFree);
      pBoard->fPCM_ChFree = IFX_NULL;
   }

#ifdef USE_FILESYSTEM
   /* Remove pointers to FW, BBD and/or DRAM filenames.*/
   if (pBoard->pFWFileName != IFX_NULL)
   {
      TD_OS_MemFree(pBoard->pFWFileName);
      pBoard->pFWFileName = IFX_NULL;
   }
   if (pBoard->pDRAMFileName != IFX_NULL)
   {
      TD_OS_MemFree(pBoard->pDRAMFileName);
      pBoard->pDRAMFileName = IFX_NULL;
   }
   if (pBoard->pBBDFileName != IFX_NULL)
   {
      TD_OS_MemFree(pBoard->pBBDFileName);
      pBoard->pBBDFileName = IFX_NULL;
   }
#endif /* USE_FILESYSTEM */

   Common_RemoveDeviceList(pBoard);
#if 0 /* GPIO functions not used */
   BOARD_XT_16_CloseGPIO_Port();
#endif /* 0 GPIO functions not used */
}

/**
   Init board.

   \param pBoard - pointer to board
   \param pProgramArg - pointer to program arguments

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_XT_16_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg)
{
    /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   if (BOARD_XT_16_PrepareBoard(pBoard, pProgramArg->sPathToDwnldFiles) == IFX_ERROR)
   {
       return IFX_ERROR;
   }

   if (pBoard->nChipCnt == 0)
   {
      /* xT16 is not present. */
      return IFX_SUCCESS;
   }
   /* Initialize chip */
   if (DEVICE_VXT_Init(pBoard, pProgramArg->sPathToDwnldFiles) == IFX_ERROR)
   {
       return IFX_ERROR;
   }
   /* Initialize the PCM Interface. Must do this, otherwise access to
      extension board or FXO will not work. */
   if (BOARD_XT_16_CfgPCMInterface(pBoard,
           pProgramArg->oArgFlags.nPCM_Master) == IFX_ERROR)
   {
       return IFX_ERROR;
   }
#ifdef FXO
/* FXO initialization */
#endif /* FXO */

   return IFX_SUCCESS;
}

/**
   Register board.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
*/
IFX_return_t BOARD_XT_16_Register(BOARD_t* pBoard)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pBoard->pszBoardName = BOARD_NAME_XT_16;
   pBoard->Init = BOARD_XT_16_Init;
   pBoard->RemoveBoard = BOARD_XT_16_RemoveBoard;

   return IFX_SUCCESS;
}

#ifdef EASY336
/**
 * Set PCM configuration.

   \param pBoard - pointer to board.
   \param pcm_cfg - pointer to PCM configuration.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
   \remark
      Memory for member "pDevsXT16" of pcm_cfg allocated by procedure and
      should be freed by caller afterwards.
*/
IFX_return_t BOARD_XT_16_PCM_ConfigSet(BOARD_t* pBoard,
                                       RM_SVIP_PCM_CFG_t* pcm_cfg)
{
   RM_SVIP_PCM_DEV_t *pPCM_DevXT16;
   IFX_int32_t i = 0;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if (pBoard->nChipCnt <= 0)
   {
      /* In case of running Tapidemo compiled with --with-vxt, on
         SVIP with extension board XT16, Tapidemo enters this function. Because
         nChipCnt is equal 0, so nothing is done.
         Before using IFX OS, it works because malloc(0) return pointer.
         But TD_OS_MemAlloc(0) returns IFX_NULL */
      return IFX_SUCCESS;
   }

   pPCM_DevXT16 = TD_OS_MemAlloc(sizeof(RM_SVIP_PCM_DEV_t) * pBoard->nChipCnt);
   if (pPCM_DevXT16 == IFX_NULL)
   {
      return IFX_ERROR;
   }

   for (i = 0; i < pBoard->nChipCnt; i++)
   {
      /* PCM highways #0 of all xT16 devices are connected to
      board PCM highway #0. */
      pPCM_DevXT16[i].nHwDev[0] = 0;
      /* PCM highways #1 of all SVIP devices are connected to
      board PCM highway #1. */
      pPCM_DevXT16[i].nHwDev[1] = 1;
      /* XT16 devices do not have three PCM highways. */
      pPCM_DevXT16[i].nHwDev[2] = RM_SVIP_PCM_UNUSED;
      /* XT16 devices do not have four PCM highways. */
      pPCM_DevXT16[i].nHwDev[3] = RM_SVIP_PCM_UNUSED;
   }

   /* set the device PCM highway array pointer to our local
   structure object. */
   pcm_cfg->pDevsXT16 = pPCM_DevXT16;

   return IFX_SUCCESS;
}
#endif /* EASY336 */

/**
   Fill channel structures with device and channel numbers.

   \param pBoard - pointer to Board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t BOARD_XT_16_FillChannels(BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_CAP_t CapList;
   IFX_int32_t nIores;
   IFX_int32_t i, j, phones = 0;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   for (i = 0; i < pBoard->nChipCnt; i++)
   {
      /*list ALMs*/
      memset(&CapList, 0, sizeof(IFX_TAPI_CAP_t));
      CapList.dev = i;
      CapList.captype = IFX_TAPI_CAP_TYPE_PHONES;
      nIores = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_CHECK,
         (IFX_int32_t) &CapList, CapList.dev, TD_CONN_ID_INIT);
      switch (nIores)
      {
         case 1:
         for (j = 0; j < CapList.cap; j++, phones++)
         {
            pBoard->rgoPhones[phones].nDev = i;
            pBoard->rgoPhones[phones].nPhoneCh = j;
#ifdef EASY336
            pBoard->rgoPhones[phones].lineID = SVIP_RM_UNUSED;
            pBoard->rgoPhones[phones].pVoipIDs = IFX_NULL;
            pBoard->rgoPhones[phones].nVoipIDs = 0;
            pBoard->rgoPhones[phones].connID = SVIP_RM_UNUSED;
#endif
         }
         break;
         case 0:
            break;
         case -1:
            return IFX_ERROR;
      }
   }

   return ret;
} /*BOARD_XT_16_FillChannels*/

#endif /* (defined WITH_VXT) || (defined XT16) */

