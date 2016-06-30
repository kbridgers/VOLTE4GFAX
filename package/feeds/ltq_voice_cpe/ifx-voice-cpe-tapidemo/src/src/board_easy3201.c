/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file board_easy3201.c
   \date 2006-10-01
   \brief Interface implementation for easy3201 board.

   Application needs it to initialize board, prepare structures for this board
   and set mode in which board will work (using PCM, GPIO, ...).
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"
#include "board_easy3201.h"
#include "pcm.h"


/* ============================= */
/* Defines                       */
/* ============================= */

#ifdef TD_HAVE_DRV_BOARD_HEADERS
/** IRQ number for DxT on EASY3201_EVS */
#define TD_EASY3201_EVS_DXT_MAIN_IRQ_NUMBER 0xC4
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

/* ============================= */
/* Local variable definition     */
/* ============================= */


/** How channels are represented by device, socket.
    Device /dev/vin11 will get status, data from phone channel 0 and
    data channel 0. If at startup phone channel 0 is mapped to data channel 1
    then, phone will send actions to dev/vin11 and data will send actions to
    /dev/vin12.
    -1 means no channel at startup */

/** Max phones on EASY3201 (analog channels)
    It is used only for STARTUP_MAPPING_EASY3201[] table.
    Number of analog channels used in program is read from drivers
    using IFX_TAPI_CAP_LIST */
enum { MAX_PHONES_EASY3201 = 2 };

#ifdef TD_HAVE_DRV_BOARD_HEADERS
/** Name of board */
static const char* BOARD_NAME_EASY3201 = "EASY3201_EVS";
#else /* TD_HAVE_DRV_BOARD_HEADERS */
/** Name of board */
static const char* BOARD_NAME_EASY3201 = "EASY3201";
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

/** Holds default phone channel to data, pcm channel mappings
    defined at startup. */
static const STARTUP_MAP_TABLE_t STARTUP_MAPPING_EASY3201[MAX_PHONES_EASY3201] =
{
   /* IMORTANT - for this board this is default mapping done by drivers,
      to change this mapping channels(phone and PCM) must be unmapped first */
   /** phone, data, pcm */
   {  0,  0,  0  },
   {  1,  1,  1  }
   /* others are free and can be used later for conferencing */
};


/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t BOARD_Easy3201_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t BOARD_Easy3201_PrepareNames(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3201_CfgPCMInterfaceSlave(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3201_GetCapabilities(BOARD_t* pBoard,
                                            IFX_char_t* pPath);
#ifdef HAVE_DRV_EASY3201_HEADERS
IFX_return_t BOARD_Easy3201_Set_CPLD_Reg_For_PCM(IFX_boolean_t fMaster,
                                                 BOARD_t* pBoard,
                                                 IFX_boolean_t fLocalLoop);
#endif /* HAVE_DRV_EASY3201_HEADERS */

IFX_void_t BOARD_Easy3201_RemoveBoard(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3201_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg);

/* ============================= */
/* Local function definition     */
/* ============================= */

/**
   Prepare board structure (reserve memory, inicialize, ...).

   \param pBoard - pointer to Board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy3201_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath)
{
   IFX_int32_t i = 0;
   TAPIDEMO_DEVICE_t* pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t* pCPUDevice = IFX_NULL;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   /* First 2 variables (id, type) set before this func is called */

   /* File descriptors not open */
   pBoard->nSystem_FD = -1;
   pBoard->nDevCtrl_FD = -1;
   /* Set flag to use system FD */
   pBoard->nUseSys_FD = IFX_TRUE;

   /* allocate memory for CPU device structure */
   pDevice = TD_OS_MemCalloc(1, sizeof(TAPIDEMO_DEVICE_t));
   if (pDevice == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Allocate memory for CPU Device. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (BOARD_Easy3201_PrepareNames(pBoard) == IFX_ERROR)
      return IFX_ERROR;

   /* Init structure for CPU device and add it to the board */
   pDevice->nType = TAPIDEMO_DEV_CPU;
   pCPUDevice = &pDevice->uDevice.stCpu;
#ifdef USE_FILESYSTEM
   /** FW files */
   pCPUDevice->pszDRAM_FW_File = sDRAMFile_DXT;
   /* for now chip version specific name is not set */
   pCPUDevice->pszPRAM_FW_File[0] = "";
   pCPUDevice->pszPRAM_FW_File[1] = sPRAMFile_DXT_DEFAULT;
   pCPUDevice->pszPRAM_FW_File[2] = sPRAMFile_DXT_Old;
   pCPUDevice->nPRAM_FW_FileNum = 3;
   pCPUDevice->bPRAM_FW_FileOptional = IFX_TRUE;
   pCPUDevice->pszBBD_CRAM_File[0] = sBBD_CRAM_File_DXT;
   pCPUDevice->pszBBD_CRAM_File[1] = sBBD_CRAM_File_DXT_Old;
#endif /* USE_FILESYSTEM */

   if (Common_AddDevice(pBoard, pDevice) == IFX_ERROR)
   {
      TD_OS_MemFree(pDevice);
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not added to the board. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (DEVICE_DUSLIC_XT_Register(pCPUDevice) == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not registered. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /** Startup mapping table */
   pBoard->pChStartupMapping = STARTUP_MAPPING_EASY3201;

   /* Basic device initialization */
   if (BOARD_Easy3201_InitSystem(pBoard) == IFX_ERROR)
   {
       return IFX_ERROR;
   }

/* #warning sleep used between BasicInit and getting chip version
   because of initialization time. */
IFXOS_SecSleep(2);
   DEVICE_DUSLIC_XT_SetVersions(pBoard, pCPUDevice);

#ifdef USE_FILESYSTEM
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

   if (IFX_ERROR == DEVICE_DUSLIC_XT_setupChannel(pBoard, pPath, IFX_FALSE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Channel 0 is not initialized. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      TD_OS_DeviceClose(pBoard->nDevCtrl_FD);
      return IFX_ERROR;
   }

   if (IFX_ERROR == BOARD_Easy3201_GetCapabilities(pBoard, pPath))
   {
      return IFX_ERROR;
   }

   if (IFX_ERROR == Common_SetPhonesAndUsedChannlesNumber(pBoard,
                                                          TD_CONN_ID_INIT))
   {
      return IFX_ERROR;
   }

   pBoard->nCh_FD = TD_OS_MemCalloc(pBoard->nUsedChannels, sizeof(IFX_int32_t));
   if (pBoard->nMaxCoderCh > 0)
   {
      pBoard->pDataChStat = TD_OS_MemCalloc(pBoard->nMaxCoderCh,
                                            sizeof(VOIP_DATA_CH_t));
      pBoard->rgnSockets = TD_OS_MemCalloc(pBoard->nMaxCoderCh,
                                           sizeof(IFX_int32_t));
      if ((pBoard->pDataChStat == IFX_NULL) || (pBoard->rgnSockets == IFX_NULL))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Allocate memory to prepare board. "
                "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   pBoard->rgoPhones = TD_OS_MemCalloc(pBoard->nMaxPhones, sizeof(PHONE_t));
   if ((pBoard->rgoPhones == IFX_NULL) || (pBoard->nCh_FD == IFX_NULL))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Allocate memory to prepare board. "
             "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
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

   for (i = 0; i < pBoard->nUsedChannels; i++)
   {
       pBoard->nCh_FD[i] = -1;
   }

   /* Prepare array of free PCM channels  */
   pBoard->fPCM_ChFree =
      TD_OS_MemCalloc(pBoard->nMaxPCM_Ch, sizeof(IFX_boolean_t));

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

   if (IFX_SUCCESS != Common_Set_FDs(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err setting fds. (File: %s, line: %d)\n",
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

   \remark Must be called before application exit.
*/
IFX_return_t BOARD_Easy3201_PrepareNames(BOARD_t* pBoard)
{
   IFX_char_t tmp_buf[10] = {0};

    /*  strlen(string) + terminating null-character */
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCH_DEV_NAME_DXT)+1),
                      strlen(sCH_DEV_NAME_DXT), IFX_ERROR);
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCTRL_DEV_NAME_DXT)+1),
                      strlen(sCTRL_DEV_NAME_DXT), IFX_ERROR);
#if defined (HAVE_DRV_EASY3201_HEADERS) || defined (TD_HAVE_DRV_BOARD_HEADERS)
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sSYS_DEV_NAME_DXT)+1),
                      strlen(sSYS_DEV_NAME_DXT), IFX_ERROR);
#endif /* HAVE_DRV_EASY3201_HEADERS || TD_HAVE_DRV_BOARD_HEADERS */

   /** Device name without numbers, like /dev/vmmc, /dev/vin, ... */
   memset(pBoard->pszChDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszChDevName, sCH_DEV_NAME_DXT, strlen(sCH_DEV_NAME_DXT));
   pBoard->pszChDevName[strlen(sCH_DEV_NAME_DXT)] = '\0';

   /** Device control name numbers, like /dev/vmmc[board id]0, ... */
   memset(pBoard->pszCtrlDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszCtrlDevName, sCTRL_DEV_NAME_DXT,
           strlen(sCTRL_DEV_NAME_DXT));
   pBoard->pszCtrlDevName[strlen(sCTRL_DEV_NAME_DXT)] = '\0';
   snprintf(tmp_buf, 2, "%d", (int) pBoard->nID);
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
   snprintf(tmp_buf, 2, "%d", 0);
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

#if defined (HAVE_DRV_EASY3201_HEADERS) || defined (TD_HAVE_DRV_BOARD_HEADERS)
   /** System device name, like /dev/easy3201/[<board id], ... */
   /** BUT actually we only have one */
   memset(pBoard->pszSystemDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszSystemDevName, sSYS_DEV_NAME_DXT,
           strlen(sSYS_DEV_NAME_DXT));
   pBoard->pszSystemDevName[strlen(sSYS_DEV_NAME_DXT)] = '\0';
#endif /* HAVE_DRV_EASY3201_HEADERS || TD_HAVE_DRV_BOARD_HEADERS */

   return IFX_SUCCESS;
}

#ifndef STREAM_1_1
/**
   Configure PCM interface, can define clock, slave mode, ...

   \param pBoard  - pointer to board

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR

   \remark for Easy3201 slave mode is used.
*/
IFX_return_t BOARD_Easy3201_CfgPCMInterfaceSlave(BOARD_t* pBoard)
{
   IFX_TAPI_PCM_IF_CFG_t pcm_if;
   IFX_return_t ret;

   TD_PTR_CHECK(pBoard, IFX_FALSE);

   memset(&pcm_if, 0, sizeof(IFX_TAPI_PCM_IF_CFG_t));

   /* for Duslic-Xt only slave mode can be used */
   pcm_if.nOpMode = IFX_TAPI_PCM_IF_MODE_SLAVE_AUTOFREQ;

   pcm_if.nDCLFreq = IFX_TAPI_PCM_IF_DCLFREQ_2048;
   pcm_if.nDoubleClk = IFX_DISABLE;

   pcm_if.nSlopeTX = IFX_TAPI_PCM_IF_SLOPE_RISE;
   pcm_if.nSlopeRX = IFX_TAPI_PCM_IF_SLOPE_FALL;
   pcm_if.nOffsetTX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nOffsetRX = IFX_TAPI_PCM_IF_OFFSET_NONE;
   pcm_if.nDrive = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   pcm_if.nShift = IFX_DISABLE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("%s: Configure PCM interface as slave, %s kHz\n",
          pBoard->pszBoardName, TAPIDEMO_PCM_FREQ_STR[pcm_if.nDCLFreq]));

   ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_PCM_IF_CFG_SET,
            (IFX_int32_t) &pcm_if, TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (ret != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, configure PCM interface "
            "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* set PCM frequency
      #warning for EASY3201 and EASY3201_EVS the frequency in reality
      is set with the CPLD!! be carefull with updating value below! */
   g_oPCM_Data.nPCM_Rate = pcm_if.nDCLFreq;

   return IFX_SUCCESS;
} /*  */

#else /* !STREAM_1_1 */

IFX_return_t BOARD_Easy3201_CfgPCMInterfaceSlave(BOARD_t* pBoard)
{
   return IFX_SUCCESS;
} /*  */
#endif /* !STREAM_1_1 */

/**
   Get capabilities, basically number of phone, data and pcm channels.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy3201_GetCapabilities(BOARD_t* pBoard,
                                            IFX_char_t* pPath)
{
   IFX_return_t ret = IFX_ERROR;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   ret = Common_GetCapabilities(pBoard, TD_CONN_ID_INIT);
   if (ret == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Capabilities not read. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
   }

   if (pBoard->nMaxAnalogCh > MAX_PHONES_EASY3201)
   {
      /* If MAX_PHONES_EASY3201 is too small than STARTUP_MAPPING_EASY3201[]
         table has too small size and TAPIDEMO will from wrong place */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, MAX_PHONES_EASY3201 is too small. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return ret;
}

/**
   Basic device initialization prior to DuSLIC-XT initialization.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy3201_InitSystem(BOARD_t* pBoard)
{
#if defined (TD_HAVE_DRV_BOARD_HEADERS) || defined (HAVE_DRV_EASY3201_HEADERS)
   IFX_int32_t nDevFd;
   IFX_int32_t nIrqNum;
   IFX_uint32_t nChipNum = 0;
#endif /* TD_HAVE_DRV_BOARD_HEADERS || HAVE_DRV_EASY3201_HEADERS */
#ifdef HAVE_DRV_EASY3201_HEADERS
   IFX_uint32_t nMaxChipNum = 0;
   EASY3201_CPLD_CONFIG_t config_dxt;
   EASY3201_CPLD_GET_DEV_PARAM_t chip_param_dxt;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t   *pCpuDevice = IFX_NULL;
   IFX_char_t pszDevName[DEV_NAME_LEN];
#endif /* HAVE_DRV_EASY3201_HEADERS */
#ifdef TD_HAVE_DRV_BOARD_HEADERS
   BOARD_CPLD_RESET_t reset_dxt = {0};
#endif /* TD_HAVE_DRV_BOARD_HEADERS */
   IFX_boolean_t we_are_master = IFX_FALSE;
   IFX_boolean_t use_pcm_local_loop = IFX_FALSE;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pBoard->pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard->pCtrl->pProgramArg, IFX_ERROR);

   if (pBoard->pCtrl->pProgramArg->oArgFlags.nPCM_Master)
   {
      we_are_master = IFX_TRUE;
   }

   if (pBoard->pCtrl->pProgramArg->oArgFlags.nPCM_Loop)
   {
      use_pcm_local_loop = IFX_TRUE;
   }

   if (0 > pBoard->nDevCtrl_FD)
   {
      pBoard->nDevCtrl_FD = Common_Open(pBoard->pszCtrlDevName, pBoard);
      if (0 > pBoard->nDevCtrl_FD)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Invalid device control file descriptor %s.\n"
               "(File: %s, line: %d)\n",
               pBoard->pszCtrlDevName, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

#ifdef HAVE_DRV_EASY3201_HEADERS
   if (0 > pBoard->nSystem_FD)
   {
      /* Get system fd */
      pBoard->nSystem_FD = Common_Open(pBoard->pszSystemDevName, IFX_NULL);

      if (0 > pBoard->nSystem_FD)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Invalid system file descriptor. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
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


   memset(&config_dxt, 0, sizeof(config_dxt));
   config_dxt.nAccessMode = SYSTEM_AM_DEFAULT_DXT;
   config_dxt.nClkRate = TAPIDEMO_CLOCK_RATE;

   /* set system configuration, CPLD acces mode, ... */
   if (IFX_ERROR == TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3201_CONFIG,
                       (IFX_int32_t) &config_dxt,
                       TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      return IFX_ERROR;
   }

   /* Go on with initialization, but only for chip 0 */
   if ((pBoard->nID - 1) == 0)
   {
      nChipNum = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3201_INIT, 0,
                          TD_DEV_NOT_SET, TD_CONN_ID_INIT);
      if (nChipNum < 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, with ioctl cmd FIO_EASY3201_INIT."
               "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   else
   {
      nChipNum = 1;
   }
   /* Store chip count
      For TAPI V3 there's only one device per board */
   pBoard->nChipCnt = 1;

   /* Second device is located on extension board */
   nMaxChipNum = nChipNum;

   /* for all chips */
   for (nChipNum = 0; nChipNum < nMaxChipNum; nChipNum++)
   {
      if (nChipNum > 0)
      {
         /* get device name to open */
         sprintf(pszDevName, "%s%d%d", sCTRL_DEV_NAME_DXT, (nChipNum +1), 0);
         nDevFd = Common_Open(pszDevName, pBoard);
         if (0 > nDevFd)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, Invalid device control file descriptor %s.\n"
                  "(File: %s, line: %d)\n",
                  pszDevName, __FILE__, __LINE__));
            continue;
         }
      }
      /* initializing first chip */
      else
      {
         /* set file descriptor */
         nDevFd = pBoard->nDevCtrl_FD;
      }

      /* activate chip reset */
      if (BOARD_Easy3201_Reset(pBoard, nChipNum, IFX_TRUE) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Basic Device Initialization fails on device %d"
               "(File: %s, line: %d)\n",
               nChipNum, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      memset(&chip_param_dxt, 0, sizeof(chip_param_dxt));
      chip_param_dxt.nChipNum = nChipNum;

      /* read dxt device configuration parameters from board driver */
      if (TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3201_GETBOARDPARAMS,
             (IFX_int32_t) &chip_param_dxt, nChipNum, TD_CONN_ID_INIT)
          != IFX_SUCCESS)
      {
         return IFX_ERROR;
      }
      /* if initializing first chip */
      if (0 == nChipNum)
      {
         /* set Irq Line number */
         pCpuDevice->nIrqNum = chip_param_dxt.nIrqNum;
      }

      /* check if polling mode should be used */
      if (pBoard->pCtrl->pProgramArg->oArgFlags.nPollingMode)
      {
         nIrqNum = TD_USE_POLLING_MODE;
      }
      else
      {
         nIrqNum = chip_param_dxt.nIrqNum;
      }

      if (DEVICE_DUSLIC_XT_BasicInit(nDevFd, nIrqNum) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Basic Device Initialization fails "
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* CPLD config only for the first chip */
      if (0 == nChipNum)
      {
         if (BOARD_Easy3201_Set_CPLD_Reg_For_PCM(we_are_master, pBoard,
                                         use_pcm_local_loop) != IFX_SUCCESS)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, PCM initialization failed. (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
      /* clear reset to allow access */
      if (BOARD_Easy3201_Reset(pBoard, nChipNum, IFX_FALSE) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Basic Device Initialization fails on device %d"
                "(File: %s, line: %d)\n",
                nChipNum, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* for all chips */
#endif /* HAVE_DRV_EASY3201_HEADERS */

#ifdef TD_HAVE_DRV_BOARD_HEADERS
   if (0 > pBoard->nSystem_FD)
   {
      /* Get system fd */
      pBoard->nSystem_FD = Common_Open(pBoard->pszSystemDevName, IFX_NULL);

      if (0 > pBoard->nSystem_FD)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Invalid system file descriptor. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   /* Store chip count */
   pBoard->nChipCnt = 1;

   /* activate chip reset */
   reset_dxt.nChipNum = nChipNum;
   reset_dxt.nResetMode = BOARD_CPLD_RESET_ACTIVE;
   if (TD_IOCTL(pBoard->nSystem_FD, FIO_BOARD_RESETCHIP,
               (IFX_int32_t) &reset_dxt, TD_DEV_NOT_SET, TD_CONN_ID_INIT)
       != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }

   /* set file descriptor */
   nDevFd = pBoard->nDevCtrl_FD;

   /* check if polling mode should be used */
   if (pBoard->pCtrl->pProgramArg->oArgFlags.nPollingMode)
   {
      nIrqNum = TD_USE_POLLING_MODE;
   }
   else
   {
      nIrqNum = TD_EASY3201_EVS_DXT_MAIN_IRQ_NUMBER;
   }

   if (DEVICE_DUSLIC_XT_BasicInit(nDevFd, nIrqNum) != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Basic Device Initialization fails "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* CPLD config only for the first chip */
   if (0 == nChipNum)
   {
      if (PCM_ConfigBoardDrv(we_are_master, pBoard, use_pcm_local_loop)
          != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, PCM config with board driver failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   /* deactivate chip reset line */
   reset_dxt.nChipNum = nChipNum;
   reset_dxt.nResetMode = BOARD_CPLD_RESET_DEACTIVE;
   if (TD_IOCTL(pBoard->nSystem_FD, FIO_BOARD_RESETCHIP,
               (IFX_int32_t) &reset_dxt, TD_DEV_NOT_SET, TD_CONN_ID_INIT)
       != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

   return IFX_SUCCESS;
} /* BOARD_Easy3201_InitSystem() */
#ifdef HAVE_DRV_EASY3201_HEADERS
/**
   Configure CPLD register for PCM initialization.

   \param fMaster - flag indicating if this board will be master (1) or
                    slave (0)
   \param pBoard - pointer to board
   \param fLocalLoop - 1 if PCM local loop is used, otherwise nothing (0).

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called after system inicialization.
*/
IFX_return_t BOARD_Easy3201_Set_CPLD_Reg_For_PCM(IFX_boolean_t fMaster,
                                                 BOARD_t* pBoard,
                                                 IFX_boolean_t fLocalLoop)
{
   IFX_uint8_t reg2_value = 0;
   IFX_uint8_t reg1_value = 0;
   EASY3201_CPLD_REG_t reg_dxt = {0};
   IFX_return_t ret = IFX_SUCCESS;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if (!fLocalLoop)
   {
      memset(&reg_dxt, 0, sizeof(EASY3201_CPLD_REG_t));

      reg_dxt.nRegOffset = CMDREG1_OFF_DXT;
      /* Read register */
      ret = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3201_CPLDREG_READ,
                     (IFX_int32_t) &reg_dxt, TD_DEV_NOT_SET, TD_CONN_ID_INIT);

      if (ret == IFX_SUCCESS)
      {
         reg1_value = reg_dxt.nRegVal;
      }
      else
      {
         return IFX_ERROR;
      }

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
           ("Read CMDREG%d 0x%x\n", 1, reg1_value));
   }

   memset(&reg_dxt, 0, sizeof(EASY3201_CPLD_REG_t));

   reg_dxt.nRegOffset = CMDREG2_OFF_DXT;
   /* Read register */
   ret = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3201_CPLDREG_READ,
                  (IFX_int32_t) &reg_dxt, TD_DEV_NOT_SET, TD_CONN_ID_INIT);

   if (ret == IFX_SUCCESS)
   {
      reg2_value = reg_dxt.nRegVal;
   }
   else
   {
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("Read CMDREG%d 0x%x\n", 2, reg2_value));

   if (fLocalLoop)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW,  TD_CONN_ID_INIT, ("Set PCM LOOP\n"));
      reg2_value |= PCM_LOOP;
   }
   else
   {
      if (fMaster)
      {
         reg1_value |= PCM_MASTER; /* clock master - bit 3 */
         reg2_value |= PCM_SEND_CLK;
         reg2_value |= PCMON_DXT; /* PCM if active - bit 4 */
      }
      else
      {
         reg1_value &= ~PCM_MASTER; /* clock slave - bit 3 */
         reg1_value |= 3 << 1; /* PCM connector reset - bit 1,2 */
         reg2_value &= ~PCM_SEND_CLK; /* PCM Connector is clock input - bit 3 */
         reg2_value |= PCMON_DXT; /* PCM if active - bit 4 */
      }
   }

   if (!fLocalLoop)
   {
      memset(&reg_dxt, 0, sizeof(EASY3201_CPLD_REG_t));

      reg_dxt.nRegOffset = CMDREG1_OFF_DXT;
      reg_dxt.nRegVal = reg1_value;

      /* Write register */
      ret = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3201_CPLDREG_WRITE,
                     (IFX_int32_t) &reg_dxt, TD_DEV_NOT_SET, TD_CONN_ID_INIT);

      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, writing value 0x%0X to CPLD register. "
               "(File: %s, line: %d)\n", reg_dxt.nRegVal, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
           ("Write CMDREG%d 0x%x\n", 1, reg1_value));
   }

   memset(&reg_dxt, 0, sizeof(EASY3201_CPLD_REG_t));

   reg_dxt.nRegOffset = CMDREG2_OFF_DXT;
   reg_dxt.nRegVal = reg2_value;

   /* Write register */
   ret = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3201_CPLDREG_WRITE,
                  (IFX_int32_t) &reg_dxt, TD_DEV_NOT_SET, TD_CONN_ID_INIT);

   if (ret != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, writing value 0x%0X to CPLD register. "
            "(File: %s, line: %d)\n", reg_dxt.nRegVal, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
        ("Write CMDREG%d 0x%x\n", 2, reg2_value));

   return IFX_SUCCESS;
} /* BOARD_Easy3201_Set_CPLD_Reg_For_PCM() */
#endif /* HAVE_DRV_EASY3201_HEADERS */

/* ============================= */
/* Global function definition    */
/* ============================= */
/**
   Release board structure (reserve memory, inicalize, ...).

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called before application exit.
*/
IFX_void_t BOARD_Easy3201_RemoveBoard(BOARD_t* pBoard)
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
   Read version after succesfull initialization and display it.

   \param pBoard - pointer to board
   \param pProgramArg - pointer to program arguments

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t BOARD_Easy3201_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg)
{
   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if (BOARD_Easy3201_PrepareBoard(pBoard, pProgramArg->sPathToDwnldFiles)
       == IFX_ERROR)
   {
       return IFX_ERROR;
   }

   /* Initialize chip */
   if (DEVICE_DUSLIC_XT_Init(pBoard, pProgramArg->sPathToDwnldFiles)
       == IFX_ERROR)
   {
       return IFX_ERROR;
   }

   if (BOARD_Easy3201_CfgPCMInterfaceSlave(pBoard) == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, PCM initialization failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

#ifdef HAVE_DRV_EASY3201_HEADERS
/**
   Set/Clear reset of device.

   \param pBoard - pointer to Board
   \param nArg - id of device to reset
   \param hSetReset - IFX_TRUE - set reset, IFX_FALSE - clear reset

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t BOARD_Easy3201_Reset(BOARD_t* pBoard,
                                  IFX_int32_t nArg,
                                  IFX_boolean_t hSetReset)
{
   EASY3201_CPLD_RESET_t reset_dxt;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t nDevNum = nArg;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&reset_dxt, 0, sizeof(EASY3201_CPLD_RESET_t));

   reset_dxt.nChipNum = nDevNum;
   if (hSetReset)
   {
      reset_dxt.nResetMode = EASY3201_CPLD_RESET_ACTIVE;
   }
   else
   {
      reset_dxt.nResetMode = EASY3201_CPLD_RESET_DEACTIVE;
   }
   ret = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3201_RESETCHIP,
                  (IFX_int32_t) &reset_dxt, nDevNum, TD_CONN_ID_INIT);
   if (ret != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, iocl cmd FIO_EASY3201_RESETCHIP, dev no: %d, SetReset: %d\n",
             nDevNum, hSetReset));
   }
   return ret;
}
#endif /* HAVE_DRV_EASY3201_HEADERS */
/**
   Register board.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
*/
IFX_return_t BOARD_Easy3201_Register(BOARD_t* pBoard)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pBoard->pszBoardName = BOARD_NAME_EASY3201;
   pBoard->Init = BOARD_Easy3201_Init;
   pBoard->RemoveBoard = BOARD_Easy3201_RemoveBoard;

   return IFX_SUCCESS;
}


