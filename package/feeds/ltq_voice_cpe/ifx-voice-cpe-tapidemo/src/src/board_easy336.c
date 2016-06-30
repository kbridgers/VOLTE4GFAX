/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file board_easy336.c
   \date 2009-02-20
   \brief Interface implementation for EASY336 board.

   Application needs it to initialize board, prepare structures for this board
   and set mode in which board will work (using PCM, GPIO, ...).
*/
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#ifdef EASY336

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"
#include "board_easy336.h"

/* #include "drv_vmmc_strerrno.h" */
#include "lib_svip_rm.h"
#include "pcm.h"
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   #include "td_t38.h"
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

/* ============================= */
/* Defines                       */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Name of board */
static const char* BOARD_NAME_EASY336 = "EASY336";

#if 0 /* GPIO functions not used */
/** Handle to the GPIO port */
static IFX_int32_t GPIO_PortFd = -1;
/** GPIO port name */
static IFX_char_t* GPIO_PORT_NAME = "/dev/port";
#endif /* 0 GPIO functions not used */


/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t BOARD_Easy336_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t BOARD_Easy336_PrepareNames(BOARD_t* pBoard);
IFX_return_t BOARD_Easy336_GetCapabilities(BOARD_t* pBoard,
                                           IFX_char_t* pPath);
#if 0 /* GPIO functions not used */
IFX_return_t BOARD_Easy336_OpenGPIO_Port(IFX_void_t);
IFX_return_t BOARD_Easy336_CloseGPIO_Port(IFX_void_t);
#endif /* 0 GPIO functions not used */
IFX_void_t BOARD_Easy336_RemoveBoard(BOARD_t* pBoard);
IFX_return_t BOARD_Easy336_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg);

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
IFX_return_t BOARD_Easy336_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath)
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
   pBoard->nUseSys_FD = IFX_FALSE;

   /* allocate memory for CPU device structure */
   pDevice = TD_OS_MemCalloc(1, sizeof(TAPIDEMO_DEVICE_t));
   if (pDevice == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("ERR Allocate memory for CPU Device. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Init structure for CPU device and add it to the board */
   pDevice->nType = TAPIDEMO_DEV_CPU;
   pCPUDevice = &pDevice->uDevice.stCpu;
   /** Pointer to device dependent structure */
   pCPUDevice->pDevDependStuff = (IFX_void_t *) &SvipDevInitStruct;
   pCPUDevice->AccessMode = SYSTEM_AM_DEFAULT_SVIP;

#ifdef USE_FILESYSTEM
   /** FW files */
   pCPUDevice->pszBBD_CRAM_File[0] = sBBD_CRAM_File_SVIP_R;
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
   if (IFX_ERROR ==  Common_SetDownloadPath(pPath, pBoard, pCPUDevice))
   {
      /* Can't set download path */
      TD_OS_MemFree(pDevice);
      return IFX_ERROR;
   }
#endif /* VXWORKS */
#endif /* USE_FILESYSTEM */

   if (Common_AddDevice(pBoard, pDevice) == IFX_ERROR)
   {
      TD_OS_MemFree(pDevice);
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("ERR, CPU device not added to the board. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (DEVICE_SVIP_Register(pCPUDevice) == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("ERR, CPU device not registered. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (BOARD_Easy336_PrepareNames(pBoard) == IFX_ERROR)
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

   Common_SetChipCount(pBoard);

   if (IFX_ERROR == BOARD_Easy336_GetCapabilities(pBoard, pPath))
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
                  ("Err, Allocate memory to prepare board. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
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
      TODO: memeber pBoard->fPCM_ChFree should be removed for SVIP and XT16 */
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

   ret = BOARD_Easy336_FillChannels(pBoard);

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
IFX_return_t BOARD_Easy336_PrepareNames(BOARD_t* pBoard)
{
   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /** Device control name numbers, like /dev/vmmc[board id]0, ... */
   strncpy(pBoard->pszCtrlDevName, sCTRL_DEV_NAME_SVIP,
           strlen(sCTRL_DEV_NAME_SVIP));
   return IFX_SUCCESS;
}

/**
   Get capabilities, basically number of phone, data and pcm channels.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy336_GetCapabilities(BOARD_t* pBoard,
                                           IFX_char_t* pPath)
{
   IFX_return_t ret = IFX_ERROR;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   /* init first channel */
   ret = DEVICE_SVIP_SetupChannel(pBoard, pPath);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, DEVICE_SVIP_setupChannel failed. (File: %s, line: %d)\n",
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
IFX_return_t BOARD_Easy336_CfgPCMInterface(BOARD_t* pBoard,
                                           IFX_boolean_t hMaster)
{
   IFX_int32_t i, j = 0;
   IFX_TAPI_PCM_IF_CFG_t pcm_if;
   IFX_return_t ret;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&pcm_if, 0, sizeof(IFX_TAPI_PCM_IF_CFG_t));
   /* set PCM configuration */
   pcm_if.nDCLFreq =  IFX_TAPI_PCM_IF_DCLFREQ_4096;
   pcm_if.nDoubleClk = IFX_DISABLE;
   pcm_if.nSlopeTX = IFX_TAPI_PCM_IF_SLOPE_RISE;
   pcm_if.nSlopeRX = IFX_TAPI_PCM_IF_SLOPE_FALL;
   pcm_if.nOffsetTX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nOffsetRX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nDrive = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   pcm_if.nShift = IFX_DISABLE;

   for (i = 0; i < pBoard->nChipCnt; i++)
   {

      /* Only first device will be configured as PCM master. */
      if (i==0 && hMaster)
      {
         pcm_if.nOpMode = IFX_TAPI_PCM_IF_MODE_MASTER;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
               ("%s: Configure PCM interface as master %s kHz on %s dev %d\n",
                pBoard->pszBoardName,
                TAPIDEMO_PCM_FREQ_STR[pcm_if.nDCLFreq], pBoard->pszBoardName, i));
      }
      else
      {
         pcm_if.nOpMode = IFX_TAPI_PCM_IF_MODE_SLAVE_AUTOFREQ;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
               ("%s: Configure PCM interface as slave %s kHz on %s dev %d\n",
                pBoard->pszBoardName,
                TAPIDEMO_PCM_FREQ_STR[pcm_if.nDCLFreq], pBoard->pszBoardName, i));
      }

#ifdef TAPI_VERSION4
      /* set device number */
      pcm_if.dev = i;
#endif /* TAPI_VERSION4 */

      for (j = 0; j < RM_SVIP_NUM_HIGHWAYS; j++)
      {
         pcm_if.nHighway = j;
         ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_PCM_IF_CFG_SET,
                        (IFX_int32_t) &pcm_if, i, TD_CONN_ID_INIT);
         if (ret != IFX_SUCCESS)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, configure PCM interface on board. "
                   "(File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
         }
      } /* for all highways */
   } /* for all chips/devices */

   /* set PCM frequency */
   g_oPCM_Data.nPCM_Rate = pcm_if.nDCLFreq;

   return IFX_SUCCESS;
} /* BOARD_Easy50712_CfgPCMInterface() */

#else /* !STREAM_1_1 */

IFX_return_t BOARD_Easy336_CfgPCMInterface(BOARD_t* pBoard,
                                           IFX_boolean_t hMaster)
{
   return IFX_SUCCESS;
} /* PCM_Init() */
#endif /* !STREAM_1_1 */

#if 0 /* GPIO functions not used */
/**
   Open GPIO port.

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t BOARD_Easy336_OpenGPIO_Port(IFX_void_t)
{
   /* Device: mknod /dev/danube-port c 249 0, but look
      cat /proc/devices for correct major number. */
   if (GPIO_PortFd < 0)
   {
      GPIO_PortFd = TD_OS_DeviceOpen(GPIO_PORT_NAME);

      if (GPIO_PortFd < 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err open SVIP GPIO port (%s), (File: %s, line: %d)\n",
                GPIO_PORT_NAME, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
                ("SVIP GPIO port (%s) opened\n", GPIO_PORT_NAME));
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
             ("SVIP GPIO port (%s) already opened\n", GPIO_PORT_NAME));
   }

   return IFX_SUCCESS;
}


/**
   Close GPIO port.

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t BOARD_Easy336_CloseGPIO_Port(IFX_void_t)
{
   /* Device: mknod /dev/danube-port c 249 0, but look
      cat /proc/devices for correct major number. */
   if (GPIO_PortFd >= 0)
   {
      if (TD_OS_DeviceClose(GPIO_PortFd) == IFX_ERROR)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, closing SVIP GPIO port %s, (File: %s, line: %d)\n",
                GPIO_PORT_NAME, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("SVIP GPIO port closed\n"));
         GPIO_PortFd = -1;
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,  TD_CONN_ID_INIT,
         ("SVIP GPIO port already closed\n"));
   }

   return IFX_SUCCESS;
}
#endif /* 0 GPIO functions not used */

/**
   Release board structure (reserve memory, inicalize, ...).

   \param pBoard - pointer to board

   \remark Must be called before application exit.
*/
IFX_void_t BOARD_Easy336_RemoveBoard(BOARD_t* pBoard)
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
   BOARD_Easy336_CloseGPIO_Port();
#endif /* 0 GPIO functions not used */
   if (IFX_NULL != pBoard->pCtrl)
   {
      if (IFX_NULL != pBoard->pCtrl->pConnID_LinID_Cnt)
      {
         TD_OS_MemFree(pBoard->pCtrl->pConnID_LinID_Cnt);
         pBoard->pCtrl->pConnID_LinID_Cnt = IFX_NULL;
      }
   }
}

/**
   Init board.

   \param pBoard - pointer to board
   \param pProgramArg - pointer to program arguments

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy336_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg)
{
   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   if (BOARD_Easy336_PrepareBoard(pBoard, pProgramArg->sPathToDwnldFiles) == IFX_ERROR)
   {
      return IFX_ERROR;
   }

   /* Initialize chip */
   if (DEVICE_SVIP_Init(pBoard, pProgramArg->sPathToDwnldFiles) == IFX_ERROR)
   {
      return IFX_ERROR;
   }
   /* Initialize the PCM Interface. Must do this, otherwise access to
      extension board or FXO will not work. */
   if (BOARD_Easy336_CfgPCMInterface(pBoard,
                                     pProgramArg->oArgFlags.nPCM_Master) == IFX_ERROR)
   {
      return IFX_ERROR;
   }

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   if (pBoard->nT38_Support)
   {
      /** Get capabilities of the T.38 stack implementation. */
      if (TD_T38_Init(pBoard) != IFX_ERROR)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
               ("TAPI Fax T.38 initilized successfully.\n"));
      }
      else
      {
         if (pBoard->nT38_Support)
         {
            return IFX_ERROR;
         }
      }
   }
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38)  && defined(HAVE_T38_IN_FW)) */

#ifdef FXO
/* FXO initialization */
#endif /* FXO */

   return IFX_SUCCESS;
}

/*
   Set/Clear reset of device by using GPIO port.

   \param pBoard - pointer to Board
   \param nArg - devices to which access is done by GPIO
   \param hSetReset - IFX_TRUE - set reset, IFX_FALSE - clear reset

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
   \remark function not implemented for EASY336
 */
IFX_int32_t BOARD_Easy336_Reset(BOARD_t* pBoard,
                                IFX_int32_t nArg,
                                IFX_boolean_t hSetReset)
{
   return IFX_SUCCESS;
}

/**
   Set PCM master mode.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t BOARD_Easy336_SetPCM_Master_overGPIO(IFX_void_t)
{
   return IFX_SUCCESS;
}

/**
   Register board.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
*/
IFX_return_t BOARD_Easy336_Register(BOARD_t* pBoard)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pBoard->pszBoardName = BOARD_NAME_EASY336;
   pBoard->Init = BOARD_Easy336_Init;
   pBoard->RemoveBoard = BOARD_Easy336_RemoveBoard;

   return IFX_SUCCESS;
}

#ifdef EASY336
/**
   Check if event exists on data channel that is connected to specified phone

   \param pPhone - pointer to PHONE
   \param pEvent - pointer to event

   \return IFX_TRUE - event exists, IFX_FALSE - event doesn't exist.
*/
IFX_boolean_t BSVIP_EventOnDataChExists(PHONE_t* pPhone,
                                        IFX_TAPI_EVENT_t* pEvent)
{
   SVIP_RM_Status_t RM_Ret;
   RM_SVIP_RU_t codRU;
   IFX_int32_t i;

   for (i = 0; i < pPhone->nVoipIDs; i++)
   {
      RM_Ret = SVIP_RM_VoipIdRUCodGet(pPhone->pVoipIDs[i], &codRU);
      if (RM_Ret == SVIP_RM_Success &&
          codRU.module != IFX_TAPI_MODULE_TYPE_NONE &&
          pEvent->dev == codRU.nDev &&
          pEvent->ch == codRU.nCh)
         return IFX_TRUE;
   }

   return IFX_FALSE;
}

/**
   Check if event exists on PCM channel that is connected to specified phone

   \param pBoard - pointer to BOARD
   \param pPhone - pointer to PHONE
   \param pEvent - pointer to event

   \return IFX_TRUE - event exists, IFX_FALSE - event doesn't exist.
*/
IFX_boolean_t BSVIP_EventOnPCMChExists(BOARD_t* pBoard,
                                       PHONE_t* pPhone,
                                       IFX_TAPI_EVENT_t* pEvent)
{
   SVIP_RM_Status_t RM_Ret;
   RM_SVIP_RU_t sigDetRU;
   IFX_TAPI_DEV_TYPE_t devType;

   switch (pBoard->nType)
   {
      case TYPE_SVIP:
         devType = IFX_TAPI_DEV_TYPE_VIN_SUPERVIP;
         break;
      case TYPE_XT16:
         devType = IFX_TAPI_DEV_TYPE_VIN_XT16;
         break;
      default:
         devType = IFX_TAPI_DEV_TYPE_NONE;
   }

   RM_Ret = SVIP_RM_LineIdRUSigDetGet(pPhone->lineID, &sigDetRU);
   if (RM_Ret == SVIP_RM_Success &&
       sigDetRU.module != IFX_TAPI_MODULE_TYPE_NONE &&
       sigDetRU.devType == devType &&
       pEvent->dev == sigDetRU.nDev &&
       pEvent->ch == sigDetRU.nCh)
      return IFX_TRUE;

   return IFX_FALSE;
}
#endif

/**
   Fill channel structures with device and channel numbers.

   \param pBoard - pointer to Board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t BOARD_Easy336_FillChannels(BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_CAP_t CapList;
   IFX_int32_t nIores;
   IFX_int32_t i, j, codecs = 0, phones = 0;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   for (i = 0; i < pBoard->nChipCnt; i++)
   {
      /*list codecs*/
      memset(&CapList, 0, sizeof(IFX_TAPI_CAP_t));
      CapList.dev = i;
      CapList.captype = IFX_TAPI_CAP_TYPE_CODECS;
      nIores = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_CHECK,
                        (IFX_int32_t) &CapList, CapList.dev, TD_CONN_ID_INIT);
      switch (nIores)
      {
         case 1:
#ifdef EASY336
            if (CapList.cap == 0)
               CapList.cap = MAX_CODER_CH_SVIP;
#endif
            for (j = 0; j < CapList.cap; j++, codecs++)
            {
               pBoard->pDataChStat[codecs].nDev = i;
               pBoard->pDataChStat[codecs].nCh = j;
               pBoard->pDataChStat[codecs].nUDP_Port = VOICE_UDP_PORT + codecs;
            }
            break;
         case 0:
            break;
         case -1:
            return IFX_ERROR;
      }
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
#ifndef XT16
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

} /*BSVIP_FillChannels*/

/**
   Set PCM configuration.

   \param pBoard - pointer to board.
   \param pcm_cfg - pointer to PCM configuration.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
   \remark
      Memory for member "pDevsSVIP" of pcm_cfg allocated by procedure and
      should be freed by caller afterwards.
*/
IFX_return_t BOARD_Easy336_PCM_ConfigSet(BOARD_t* pBoard,
                                         RM_SVIP_PCM_CFG_t* pcm_cfg)
{
   RM_SVIP_PCM_DEV_t *pPCM_DevSVIP;
   IFX_int32_t i;

   pPCM_DevSVIP = TD_OS_MemAlloc(sizeof(RM_SVIP_PCM_DEV_t) * pBoard->nChipCnt);
   if (pPCM_DevSVIP == IFX_NULL)
   {
      return IFX_ERROR;
   }

   for (i = 0; i < pBoard->nChipCnt; i++)
   {
      /* PCM highways #0 of all SVIP devices are connected to board PCM highway
       * #0. */
      pPCM_DevSVIP[i].nHwDev[0] = 0;
      /* PCM highways #1 of all SVIP devices are connected to
      board PCM highway #1. */
      pPCM_DevSVIP[i].nHwDev[1] = 1;
      /* PCM highways #2 of all SVIP devices are connected to
      board PCM highway #2. */
      pPCM_DevSVIP[i].nHwDev[2] = 2;
      /* PCM highways #3 of all SVIP devices are connected to
      board PCM highway #3. */
      pPCM_DevSVIP[i].nHwDev[3] = 3;
   }

   /* Set the time slots for board PCM highways. */
   for (i = 0; i < RM_SVIP_NUM_HIGHWAYS; i++)
   {
      /* All PCM highways time slots can be used starting
      with time slot 0. */
      pcm_cfg->board.nHwSlot[i].nTsStart = 0;
#ifndef STREAM_1_1
      /* Based on the used PCM frequency of PCM highways,
      we can use till time slot from table. */
      pcm_cfg->board.nHwSlot[i].nTsStop =
         TAPIDEMO_PCM_FREQ_MAX_TIMESLOT_NUMBER[g_oPCM_Data.nPCM_Rate] - 1;
#else /* STREAM_1_1 */
      /* Based on the used PCM frequency of PCM highways,
      we can use till time slot 64. */
      pcm_cfg->board.nHwSlot[i].nTsStop = 63;
#endif /* STREAM_1_1 */
   }

   /* set the device PCM highway array pointer to our local
   structure object. */
   pcm_cfg->pDevsSVIP = pPCM_DevSVIP;

   return IFX_SUCCESS;
}

/**
   Initialize SVIP reasource manager.

   \param  pCtrl   - pointer to status control structure

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t BOARD_Easy336_Init_RM(CTRL_STATUS_t* pCtrl,
                                   RM_SVIP_PCM_CFG_t* pPCM_cfg)
{
   SVIP_RM_Status_t RM_Ret;
   RM_SVIP_RU_t RU;
   IFX_int32_t i, j, lineID;


   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPCM_cfg, IFX_ERROR);

   /* Initialize SVIP/xT-16 resource manager */
   /* Here first board is SVIP, second - xT-16 */
   RM_Ret = SVIP_RM_Init(pCtrl->rgoBoards[0].nDevCtrl_FD,
                         pCtrl->rgoBoards[0].nChipCnt,
                         pCtrl->rgoBoards[1].nDevCtrl_FD,
                         pCtrl->rgoBoards[1].nChipCnt,
                         pPCM_cfg);
   if (RM_Ret != SVIP_RM_Success)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Resource manager init failed with error code %d. "
             "(File: %s, line: %d)\n",
             RM_Ret, __FILE__, __LINE__));
      return IFX_ERROR;
   } /* if (RM_Ret != SVIP_RM_Success) */

   for (i = 0; i < pCtrl->nBoardCnt; i++)
   {
      RU.module = IFX_TAPI_MODULE_TYPE_ALM;
      RU.devType = IFX_TAPI_DEV_TYPE_VIN_SUPERVIP;
      for (j = 0; j < pCtrl->rgoBoards[i].nMaxAnalogCh; j++)
      {
         RU.module = IFX_TAPI_MODULE_TYPE_ALM;
         RU.nDev = pCtrl->rgoBoards[i].rgoPhones[j].nDev;
         RU.nCh = pCtrl->rgoBoards[i].rgoPhones[j].nPhoneCh;
         if (pCtrl->rgoBoards[i].nType == TYPE_SVIP)
            RU.devType = IFX_TAPI_DEV_TYPE_VIN_SUPERVIP;
         else if (pCtrl->rgoBoards[i].nType == TYPE_XT16)
            RU.devType = IFX_TAPI_DEV_TYPE_VIN_XT16;
         RM_Ret = SVIP_RM_RULineIdGet(&RU, &lineID);
         if (RM_Ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("SVIP_RM_RULineIdGet failed for device %d, channel %d with "
                   "error code %d. "
                   "(File: %s, line: %d)\n",
                   RU.nDev, RU.nCh, RM_Ret, __FILE__, __LINE__));
            return IFX_ERROR;
         } /* if (RM_Ret != SVIP_RM_Success) */
         pCtrl->rgoBoards[i].rgoPhones[j].lineID = lineID;
      } /* for */
   } /* for */
   /* allocate memory to count LineIDs */
   pCtrl->pConnID_LinID_Cnt = TD_OS_MemCalloc(pCtrl->nSumPhones,
                                     sizeof(TD_CONN_ID_LINE_ID_CNT_t));
   /* check if meory allocated */
   if (IFX_NULL == pCtrl->pConnID_LinID_Cnt)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to allocate memory (Phones %d, size %d)"
             "File: %s, line: %d)\n",
             pCtrl->nSumPhones, sizeof(TD_CONN_ID_LINE_ID_CNT_t),
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* resset structure data */
   for (i=0; i<pCtrl->nSumPhones; i++)
   {
      pCtrl->pConnID_LinID_Cnt[i].nConnID = TD_NOT_SET;
      pCtrl->pConnID_LinID_Cnt[i].nLineID_Cnt = 0;
   }

   return IFX_SUCCESS;
}

#endif /* EASY336 */
