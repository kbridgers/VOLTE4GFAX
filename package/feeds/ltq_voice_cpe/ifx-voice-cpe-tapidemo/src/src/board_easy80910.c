/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file board_easy80910.c
   \date 2009-09-25
   \brief Interface implementation for EASY80910 boards.

   Application needs it to initialize board, prepare structures for this board
   and set mode in which board will work (using PCM, GPIO, ...).
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   #include "td_t38.h"
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
#include "board_easy80910.h"
#ifdef FXO
   #include "common_fxo.h"
#endif /* FXO */
#include "pcm.h"


#ifdef TD_DECT
#include "td_dect.h"
#endif /* TD_DECT */

/* ============================= */
/* Defines                       */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */


/** How channels are represented by device, socket.
    Device /dev/vmmc11 will get status, data from phone channel 0 and
    data channel 0. If at startup phone channel 0 is mapped to data channel 1
    then, phone will send actions to dev/vmmc11 and data will send actions to
    /dev/vmmc12.
    -1 means no channel at startup */

/** Max phones on VMMC (analog channels)
    It is used only for STARTUP_MAPPING_VMMC[] table.
    Number of analog channels used in program is read from drivers
    using IFX_TAPI_CAP_LIST */
enum { MAX_PHONES_VMMC = 2 };

/** Number of PCM highways */
enum { PCM_HIGHWAYS = 1 };

/** Name of board */
static const char* BOARD_NAME_EASY80910 = "EASY80910";

/** Holds default phone channel to data, pcm channel mappings
    defined at startup. */
static const STARTUP_MAP_TABLE_t STARTUP_MAPPING_VMMC[MAX_PHONES_VMMC] =
{
   /** phone, data, pcm */
   {  0,  0,  0 },
   {  1,  1,  1 }
};

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t BOARD_Easy80910_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t BOARD_Easy80910_PrepareNames(BOARD_t* pBoard);
IFX_return_t BOARD_Easy80910_CfgPCMInterface(BOARD_t* pBoard,
                                             IFX_boolean_t hMaster);
IFX_return_t BOARD_Easy80910_GetCapabilities(BOARD_t* pBoard,
                                             IFX_char_t* pPath);
IFX_void_t BOARD_Easy80910_RemoveBoard(BOARD_t* pBoard);
IFX_return_t BOARD_Easy80910_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg);

/* ============================= */
/* Local function definition     */
/* ============================= */
/**
   Prepare board structure (reserve memory, inicialize, ...).

   \param pBoard - pointer to Board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called after system inicialization.
*/
IFX_return_t BOARD_Easy80910_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath)
{
   IFX_int32_t i = 0;
   TAPIDEMO_DEVICE_t* pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t* pCPUDevice = IFX_NULL;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   /* First three variables (id, type, sub type) set before this func is called */

   /* File descriptors not open */
   pBoard->nSystem_FD = -1;
   pBoard->nDevCtrl_FD = -1;
   pBoard->nUseSys_FD = IFX_FALSE;

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
   pCPUDevice->pDevDependStuff = (IFX_void_t *) &VmmcDevInitStruct;
#ifdef USE_FILESYSTEM
   /** FW files */
   pCPUDevice->pszDRAM_FW_File = sDRAMFile_VMMC;
   pCPUDevice->pszPRAM_FW_File[0] = sPRAMFile_VMMC;
   pCPUDevice->pszPRAM_FW_File[1] = sPRAMFile_VMMC_Old;
   pCPUDevice->pszPRAM_FW_File[2] = "voice_ar9_firmware.bin";
   pCPUDevice->pszPRAM_FW_File[3] = "ar9_firmware.bin";
   pCPUDevice->nPRAM_FW_FileNum = 4;
   pCPUDevice->bPRAM_FW_FileOptional = IFX_FALSE;
   pCPUDevice->pszBBD_CRAM_File[0] = sBBD_CRAM_File_VMMC;
   pCPUDevice->pszBBD_CRAM_File[1] = sBBD_CRAM_File_VMMC_Old;
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
           ("Err, CPU device not added to the board. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }


   if (DEVICE_Vmmc_Register(pCPUDevice) == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not registered. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (BOARD_Easy80910_PrepareNames(pBoard) == IFX_ERROR)
      return IFX_ERROR;

   /** Startup mapping table */
   pBoard->pChStartupMapping = STARTUP_MAPPING_VMMC;

   if (IFX_ERROR == BOARD_Easy80910_GetCapabilities(pBoard, pPath))
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
      pBoard->nCh_FD = TD_OS_MemCalloc(pBoard->nUsedChannels,
                                       sizeof(IFX_int32_t));
      pBoard->rgnSockets = TD_OS_MemCalloc(pBoard->nMaxCoderCh,
                                           sizeof(IFX_int32_t));
   } /* if (pBoard->nMaxCoderCh > 0) */

   pBoard->rgoPhones = TD_OS_MemCalloc(pBoard->nMaxPhones, sizeof(PHONE_t));

   if ((pBoard->pDataChStat == IFX_NULL) || (pBoard->nCh_FD == IFX_NULL)
       || (pBoard->rgnSockets == IFX_NULL) || (pBoard->rgoPhones == IFX_NULL))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Allocate memory to prepare board. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   for (i = 0; i < pBoard->nUsedChannels; i++)
   {
      pBoard->nCh_FD[i] = -1;
   }
   for (i = 0; i < pBoard->nMaxCoderCh; i++)
   {
      pBoard->rgnSockets[i] = -1;
   }
#ifdef TD_IPV6_SUPPORT
   /* allocate memory for IPv6 socket */
   if (IFX_SUCCESS != Common_SocketAllocateIPv6(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Allocate memory to for IPv6 sockets. "
             "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* TD_IPV6_SUPPORT */

   if (pBoard->nMaxPCM_Ch > 0)
   {
      /* Prepare array of free PCM channels */
      pBoard->fPCM_ChFree = TD_OS_MemCalloc(pBoard->nMaxPCM_Ch,
                                            sizeof(IFX_boolean_t));

      if (pBoard->fPCM_ChFree == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Allocate memory for PCM channels. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* All PCM channels are free at start */
      for (i = 0; i < pBoard->nMaxPCM_Ch; i++)
      {
          pBoard->fPCM_ChFree[i] = IFX_TRUE;
      }
   }

   /* Store chip count */
   pBoard->nChipCnt = 1;

   /* Sets file descriptors */
   if (IFX_SUCCESS != Common_Set_FDs(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, setting fds. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   pBoard->pChCustomMapping = IFX_NULL;

   COM_MOD_VERIFY_SYSTEM_INIT_PREPARE(pBoard->pCtrl);

   return IFX_SUCCESS;
}

/**
   Prepare device/channels names.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy80910_PrepareNames(BOARD_t* pBoard)
{
   IFX_char_t tmp_buf[10] = {0};

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

    /*  strlen(string) + terminating null-character */
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCH_DEV_NAME_VMMC)+1),
                      strlen(sCH_DEV_NAME_VMMC), IFX_ERROR);
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCTRL_DEV_NAME_VMMC)+1),
                      strlen(sCTRL_DEV_NAME_VMMC), IFX_ERROR);
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sSYS_DEV_NAME_VMMC)+1),
                      strlen(sSYS_DEV_NAME_VMMC), IFX_ERROR);

   /** Device name without numbers, like /dev/vmmc, /dev/vin, ... */
   memset(pBoard->pszChDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszChDevName, sCH_DEV_NAME_VMMC,
           strlen(sCH_DEV_NAME_VMMC));
   pBoard->pszChDevName[strlen(sCH_DEV_NAME_VMMC)] = '\0';

   /** Device control name numbers, like /dev/vmmc[board id]0, ... */
   memset(pBoard->pszCtrlDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszCtrlDevName, sCTRL_DEV_NAME_VMMC,
           strlen(sCTRL_DEV_NAME_VMMC));
   pBoard->pszCtrlDevName[strlen(sCTRL_DEV_NAME_VMMC)] = '\0';
   snprintf(tmp_buf, (10 - 1), "%d0", (int) pBoard->nID);
   if (DEV_NAME_LEN > (strlen(tmp_buf) + strlen(pBoard->pszCtrlDevName)))
   {
      strncat(pBoard->pszCtrlDevName, tmp_buf, strlen(tmp_buf));
      pBoard->pszCtrlDevName[DEV_NAME_LEN-1] = '\0';
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Invalid index of array 'pszCtrlDevName'. "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
   }

   /** System device name, like /dev/easy3332/[board id], ... */
   memset(pBoard->pszSystemDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszSystemDevName, sSYS_DEV_NAME_VMMC,
           strlen(sSYS_DEV_NAME_VMMC));
   pBoard->pszSystemDevName[strlen(sSYS_DEV_NAME_VMMC)] = '\0';
   memset(tmp_buf, 0, 10);
   snprintf(tmp_buf, (10 -1), "%d", (int) pBoard->nID - 1);
   if (DEV_NAME_LEN > (strlen(tmp_buf) + strlen(pBoard->pszSystemDevName)))
   {
      strncat(pBoard->pszSystemDevName, tmp_buf, strlen(tmp_buf));
      pBoard->pszSystemDevName[DEV_NAME_LEN-1] = '\0';
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Invalid index of array 'pszSystemDevName'. "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
   }

   return IFX_SUCCESS;
}

/**
   Get capabilities, basically number of phone, data and pcm channels.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy80910_GetCapabilities(BOARD_t* pBoard,
                                             IFX_char_t* pPath)
{
   IFX_return_t ret = IFX_ERROR;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   pBoard->nDevCtrl_FD = Common_Open(pBoard->pszCtrlDevName, pBoard);
   if (0 > pBoard->nDevCtrl_FD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Invalid device control file descriptor %s.\n"
            "(File: %s, line: %d)\n",
            pBoard->pszCtrlDevName, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   ret = DEVICE_Vmmc_setupChannel(pBoard, pPath);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Channel 0 is not initialized. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      TD_OS_DeviceClose(pBoard->nDevCtrl_FD);
      return IFX_ERROR;
   }

   ret = Common_GetCapabilities(pBoard, TD_CONN_ID_INIT);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Capabilities not read. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
   }

   TD_OS_DeviceClose(pBoard->nDevCtrl_FD);
   pBoard->nDevCtrl_FD = -1;
   if (pBoard->nMaxAnalogCh > MAX_PHONES_VMMC)
   {
      /* If MAX_PHONES_VMMC is too small than STARTUP_MAPPING_VMMC[] table
         has too small size and TAPIDEMO will from wrong place */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, MAX_PHONES_VMMC is too small. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return ret;
}

#ifndef STREAM_1_1
/**
   Configure PCM interface, can define clock, master/slave mode, ...

   \param pBoard  - pointer to board
   \param hMaster  - IFX_TRUE - master mode, IFX_FALSE - slave mode

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR

   \remark Should be also used to enable clock if extension board will be used.
*/
IFX_return_t BOARD_Easy80910_CfgPCMInterface(BOARD_t* pBoard,
                                             IFX_boolean_t hMaster)
{
   IFX_int32_t j;
   IFX_TAPI_PCM_IF_CFG_t pcm_if;
   IFX_return_t ret;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&pcm_if, 0, sizeof(IFX_TAPI_PCM_IF_CFG_t));
   if (hMaster)
   {
      pcm_if.nOpMode = IFX_TAPI_PCM_IF_MODE_MASTER;
   }
   else
   {
      pcm_if.nOpMode = IFX_TAPI_PCM_IF_MODE_SLAVE_AUTOFREQ;
   }
   pcm_if.nDCLFreq = IFX_TAPI_PCM_IF_DCLFREQ_2048;
   pcm_if.nDoubleClk = IFX_DISABLE;

   pcm_if.nSlopeTX = IFX_TAPI_PCM_IF_SLOPE_RISE;
   pcm_if.nSlopeRX = IFX_TAPI_PCM_IF_SLOPE_FALL;
   pcm_if.nOffsetTX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nOffsetRX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nDrive = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   pcm_if.nShift = IFX_DISABLE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
        ("%s: Configure PCM interface as %s, %s kHz\n",
         pBoard->pszBoardName,
         (hMaster ? "master":"slave"), TAPIDEMO_PCM_FREQ_STR[pcm_if.nDCLFreq]));


   for (j = 0; j < PCM_HIGHWAYS; j++)
   {
      ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_PCM_IF_CFG_SET,
                     (IFX_int32_t) &pcm_if, TD_DEV_NOT_SET, TD_CONN_ID_INIT);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, configure PCM interface "
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* for */

   /* set PCM frequency */
   g_oPCM_Data.nPCM_Rate = pcm_if.nDCLFreq;

   return IFX_SUCCESS;
} /* BOARD_Easy80910_CfgPCMInterface() */

#else /* !STREAM_1_1 */

IFX_return_t BOARD_Easy80910_CfgPCMInterface(BOARD_t* pBoard,
                                             IFX_boolean_t hMaster)
{
   return IFX_SUCCESS;
} /* PCM_Init() */
#endif /* !STREAM_1_1 */


/**
   Release board structure (reserve memory, inicalize, ...).

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called before application exit.
*/
IFX_void_t BOARD_Easy80910_RemoveBoard(BOARD_t* pBoard)
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

   if (pBoard->nCh_FD != IFX_NULL)
   {
      TD_OS_MemFree(pBoard->nCh_FD);
      pBoard->nCh_FD = IFX_NULL;
   }

   if (pBoard->rgnSockets != IFX_NULL)
   {
      TD_OS_MemFree(pBoard->rgnSockets);
      pBoard->rgnSockets = IFX_NULL;
   }
#ifdef TD_IPV6_SUPPORT
   if (pBoard->rgnSocketsIPv6 != IFX_NULL)
   {
      TD_OS_MemFree(pBoard->rgnSocketsIPv6);
      pBoard->rgnSocketsIPv6 = IFX_NULL;
   }
#endif /* TD_IPV6_SUPPORT */

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
}

/**
   Init board.

   \param pBoard - pointer to board
   \param pProgramArg - pointer to program arguments

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy80910_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg)
{
    /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   if (BOARD_Easy80910_PrepareBoard(pBoard, pProgramArg->sPathToDwnldFiles) == IFX_ERROR)
   {
       return IFX_ERROR;
   }

   /* Initialize chip */
   if (DEVICE_Vmmc_Init(pBoard, pProgramArg->sPathToDwnldFiles) == IFX_ERROR)
   {
       return IFX_ERROR;
   }

   if (pBoard->nMaxPCM_Ch > 0)
   {
      /* Initialize the PCM Interface. Must do this, otherwise access to
         extension board or FXO will not work. */
      if (BOARD_Easy80910_CfgPCMInterface(pBoard,
                                          pProgramArg->oArgFlags.nPCM_Master) == IFX_ERROR)
      {
          return IFX_ERROR;
      }
   }
   else
   {
      if ((pProgramArg->oArgFlags.nPCM_Master == 1) ||
          (pProgramArg->oArgFlags.nPCM_Slave == 1) ||
          (pProgramArg->oArgFlags.nPCM_Loop == 1))
      {
         pProgramArg->oArgFlags.nPCM_Master = 0;
         pProgramArg->oArgFlags.nPCM_Slave = 0;
         pProgramArg->oArgFlags.nPCM_Loop = 0;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
                 ("PCM not supported.\n"));
      }
   }

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /** Get capabilities of the T.38 stack implementation. It is enough to read
       capabilities only from one channel because they are the same for
       all channels. */
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

#ifdef TD_DECT
   /* check if DECT support is on */
   if (!pProgramArg->oArgFlags.nNoDect)
   {
      if (IFX_SUCCESS != TD_DECT_Init(pBoard->pCtrl, pBoard))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, TD_DECT_Init failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
      TD_DECT_CurrTimerExpirySelect(pBoard->pCtrl);
   }
#endif /* TD_DECT */
#ifdef FXO
   /* FXO */
   if (pProgramArg->oArgFlags.nFXO)
   {
      if (IFX_SUCCESS != FXO_Setup(pBoard))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, FXO setup. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
#endif /* FXO */

   return IFX_SUCCESS;
}

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Register board.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
*/
IFX_return_t BOARD_Easy80910_Register(BOARD_t* pBoard)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pBoard->pszBoardName = BOARD_NAME_EASY80910;
   pBoard->Init = BOARD_Easy80910_Init;
   pBoard->RemoveBoard = BOARD_Easy80910_RemoveBoard;

   return IFX_SUCCESS;
}


