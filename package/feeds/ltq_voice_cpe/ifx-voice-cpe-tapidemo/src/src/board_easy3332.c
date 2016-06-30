/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file board_easy3332.c
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
#include "board_easy3332.h"


/* ============================= */
/* Defines                       */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */


/** How channels are represented by device, socket.
    Device /dev/vin11 will get status, data from phone channel 0 and
    data channel 0. If at startup phone channel 0 is mapped to data channel 1
    then, phone will send actions to dev/vin11 and data will send actions to
    /dev/vin12.
    -1 means no channel at startup */

/** Max phones on CPE (analog channels)
    It is used only for STARTUP_MAPPING_CPE[] table.
    Number of analog channels used in program is read from drivers
    using IFX_TAPI_CAP_LIST */
enum { MAX_PHONES_CPE = 2 };

/** Name of board */
static const char* BOARD_NAME_EASY3332 = "EASY3332";

/** Holds default phone channel to data, pcm channel mappings
    defined at startup. */
static const STARTUP_MAP_TABLE_t STARTUP_MAPPING_CPE[MAX_PHONES_CPE] =
{
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
IFX_return_t BOARD_Easy3332_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t BOARD_Easy3332_PrepareNames(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3332_InitSystem(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3332_Set_CPLD_Reg_For_PCM(IFX_boolean_t fMaster,
                                                 BOARD_t* pBoard,
                                                 IFX_boolean_t fLocalLoop);
IFX_return_t BOARD_Easy3332_GetCapabilities(BOARD_t* pBoard,
                                            IFX_char_t* pPath);
IFX_void_t BOARD_Easy3332_RemoveBoard(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3332_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg);
IFX_return_t BOARD_Easy3332_Reset(BOARD_t* pBoard,
                                  IFX_int32_t nArg,
                                  IFX_boolean_t hSetReset);

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
IFX_return_t BOARD_Easy3332_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath)
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
           ("ERR Allocate memory for CPU Device. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Init structure for CPU device and add it to the board */
   pDevice->nType = TAPIDEMO_DEV_CPU;
   pCPUDevice = &pDevice->uDevice.stCpu;
   /** Pointer to device dependent structure */
   pCPUDevice->pDevDependStuff = (IFX_void_t *) &VineticDevInitStruct;
#ifdef USE_FILESYSTEM
   /** FW files */
   pCPUDevice->pszDRAM_FW_File = sDRAMFile_CPE;
   pCPUDevice->pszPRAM_FW_File[0] = sPRAMFile_CPE;
   pCPUDevice->pszPRAM_FW_File[1] = sPRAMFile_CPE_Old;
   pCPUDevice->nPRAM_FW_FileNum = 2;
   pCPUDevice->bPRAM_FW_FileOptional = IFX_FALSE;
   pCPUDevice->pszBBD_CRAM_File[0] = sBBD_CRAM_File_CPE;
   pCPUDevice->pszBBD_CRAM_File[1] = sBBD_CRAM_File_CPE_Old;
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

   if (DEVICE_VINETIC_CPE_Register(pCPUDevice) == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not registered. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (BOARD_Easy3332_PrepareNames(pBoard) == IFX_ERROR)
      return IFX_ERROR;

   /** Startup mapping table */
   pBoard->pChStartupMapping = STARTUP_MAPPING_CPE;

   /* Basic device initialization */
   if (BOARD_Easy3332_InitSystem(pBoard) == IFX_ERROR)
   {
       return IFX_ERROR;
   }

   if (IFX_ERROR == DEVICE_VINETIC_CPE_setupChannel(pBoard, pPath, IFX_FALSE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Channel 0 is not initialized. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      TD_OS_DeviceClose(pBoard->nDevCtrl_FD);
      return IFX_ERROR;
   }

   if (IFX_ERROR == BOARD_Easy3332_GetCapabilities(pBoard, pPath))
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
      /* \todo maybe check for max ch number available and use this number */
      pBoard->nCh_FD = TD_OS_MemCalloc(pBoard->nUsedChannels,
                                       sizeof(IFX_int32_t));
      pBoard->rgnSockets = TD_OS_MemCalloc(pBoard->nMaxCoderCh,
                                           sizeof(IFX_int32_t));
   }

   pBoard->rgoPhones = TD_OS_MemCalloc(pBoard->nMaxPhones, sizeof(PHONE_t));

   if ((pBoard->pDataChStat == IFX_NULL) || (pBoard->rgoPhones == IFX_NULL) ||
       (pBoard->nCh_FD == IFX_NULL) || (pBoard->rgnSockets == IFX_NULL))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Allocate memory to prepare board. "
             "(File: %s, line: %d)\n",
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

   if (IFX_SUCCESS != Common_Set_FDs(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, setting fds. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

/**
   Prepare device/channels names.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy3332_PrepareNames(BOARD_t* pBoard)
{
   IFX_char_t tmp_buf[10] = {0};

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

    /*  strlen(string) + terminating null-character */
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCH_DEV_NAME_CPE)+1),
                      strlen(sCH_DEV_NAME_CPE), IFX_ERROR);
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCTRL_DEV_NAME_CPE)+1),
                      strlen(sCTRL_DEV_NAME_CPE), IFX_ERROR);
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sSYS_DEV_NAME_CPE)+1),
                      strlen(sSYS_DEV_NAME_CPE), IFX_ERROR);

   /** Device name without numbers, like /dev/vmmc, /dev/vin, ... */
   memset(pBoard->pszChDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszChDevName, sCH_DEV_NAME_CPE, strlen(sCH_DEV_NAME_CPE));
   pBoard->pszChDevName[strlen(sCH_DEV_NAME_CPE)] = '\0';

   /** Device control name numbers, like /dev/vmmc[board id]0, ... */
   memset(pBoard->pszCtrlDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszCtrlDevName, sCTRL_DEV_NAME_CPE,
           strlen(sCTRL_DEV_NAME_CPE));
   pBoard->pszCtrlDevName[strlen(sCTRL_DEV_NAME_CPE)] = '\0';
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

   /** System device name, like /dev/easy3332/[<board id], ... */
   memset(pBoard->pszSystemDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszSystemDevName, sSYS_DEV_NAME_CPE,
           strlen(sSYS_DEV_NAME_CPE));
   pBoard->pszSystemDevName[strlen(sSYS_DEV_NAME_CPE)] = '\0';
   memset(tmp_buf, 0, 10);
   snprintf(tmp_buf, 2, "%d", (int) pBoard->nID - 1);
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
IFX_return_t BOARD_Easy3332_GetCapabilities(BOARD_t* pBoard,
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

   if (pBoard->nMaxAnalogCh > MAX_PHONES_CPE)
   {
      /* If MAX_PHONES_CPE is too small than STARTUP_MAPPING_CPE[] table
         has too small size and TAPIDEMO will from wrong place */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, MAX_PHONES_CPE is too small. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return ret;
}

/**
   Basic device initialization prior to vinetic initialization.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      - In case of 8bit SPI, for the CPE system, base address  should be set to
        0x1F if not otherwise programmed by bootstrapping
      - Caller must make sure that the access mode is  supported
*/
IFX_return_t BOARD_Easy3332_InitSystem(BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_uint32_t chipnum = 0, i = 0;
   IFX_int32_t nAccessMode;
   IFX_int32_t vin_access_mode = 0;
   EASY3332_Config_t config_cpe;
   EASY3332_GetDevParam_t chip_param_cpe;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t   *pCpuDevice = IFX_NULL;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

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

   nAccessMode = SYSTEM_AM_DEFAULT_CPE;
   memset(&config_cpe, 0, sizeof(config_cpe));
   config_cpe.nAccessMode = nAccessMode;
   config_cpe.nClkRate = TAPIDEMO_CLOCK_RATE;

   /* set system configuration, CPLD acces mode, ... */
   if (IFX_ERROR == TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3332_CONFIG,
                                (IFX_int32_t) &config_cpe,
                                TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      return IFX_ERROR;
   }
   /* go on with initialization */
   chipnum = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3332_INIT, 0,
                TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (chipnum < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, ioctl cmd FIO_EASY3332_INIT."
            "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Store chip count */
   pBoard->nChipCnt = chipnum;

   /* initialize each vinetic device */
   for (i = 0; i < chipnum; i++)
   {
      ret = IFX_ERROR;
      /* activate chip reset */
      if (BOARD_Easy3332_Reset(pBoard, i, IFX_TRUE) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Basic Device Initialization fails on device %d"
               "(File: %s, line: %d)\n",
               i, __FILE__, __LINE__));
         break;
      }

      memset(&chip_param_cpe, 0, sizeof(EASY3332_GetDevParam_t));

      /* read vinetic device configuration parameters from board driver */
      if (TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3332_GETBOARDPARAMS,
                   (IFX_int32_t) &chip_param_cpe, i, TD_CONN_ID_INIT)
          != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Basic Device Initialization fails on device %d"
               "(File: %s, line: %d)\n",
               i, __FILE__, __LINE__));
         break;
      }

      switch (nAccessMode)
      {
          case EASY3332_ACCESS_8BIT_MOTOROLA:
             vin_access_mode = VIN_ACCESS_PAR_8BIT;
          break;
          case EASY3332_ACCESS_8BIT_INTELMUX:
             vin_access_mode = VIN_ACCESS_PARINTEL_MUX8;
          break;
          case EASY3332_ACCESS_8BIT_INTELDEMUX:
             vin_access_mode = VIN_ACCESS_PARINTEL_DMUX8;
          break;
          case EASY3332_ACCESS_SPI:
             vin_access_mode = VIN_ACCESS_SPI;
          break;
          default:
             TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Err, Unknown Physical Access Mode 0x%X."
                   "(File: %s, line: %d)\n", nAccessMode,
                   __FILE__, __LINE__));
          break;
      }

      pCpuDevice->AccessMode = vin_access_mode;
      pCpuDevice->nBaseAddress = chip_param_cpe.nBaseAddrPhy;
      pCpuDevice->nIrqNum = chip_param_cpe.nIrqNum;

      if (DEVICE_VINETIC_CPE_BasicInit(pBoard) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Basic Device Initialization fails on device %d"
               "(File: %s, line: %d)\n",
               i, __FILE__, __LINE__));
         break;
      }

      /* clear reset to allow vinetic access */
      if (BOARD_Easy3332_Reset(pBoard, i, IFX_FALSE) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Basic Device Initialization fails on device %d"
               "(File: %s, line: %d)\n",
               i, __FILE__, __LINE__));
         break;
      }

      /* this loop processed successfully */
      ret = IFX_SUCCESS;
   } /* for (i = 0; i < chipnum; i++)*/

   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Basic Device Initialization fails on device %d"
            "(File: %s, line: %d)\n",
            i, __FILE__, __LINE__));
   }

   return ret;
} /* BOARD_Easy3332_InitSystem() */

/**
   Configure CPLD register for PCM initialization.

   \param fMaster - flag indicating if this board will be master (1) or
                    slave (0)
   \param pBoard - pointer to board
   \param fLocalLoop - 1 if PCM local loop is used, otherwise nothing (0).

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called after system inicialization.
*/
IFX_return_t BOARD_Easy3332_Set_CPLD_Reg_For_PCM(IFX_boolean_t fMaster,
                                                 BOARD_t* pBoard,
                                                 IFX_boolean_t fLocalLoop)
{
   IFX_uint8_t reg1_value = 0;
   EASY3332_Reg_t reg_cpe;
   IFX_return_t ret = IFX_SUCCESS;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&reg_cpe, 0, sizeof(EASY3332_Reg_t));

   reg_cpe.nRegOffset = CMDREG1_OFF_CPE;
   /* Read register */
   ret = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3332_CPLDREG_READ,
                  (IFX_int32_t) &reg_cpe, TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (ret == IFX_SUCCESS)
   {
      reg1_value = reg_cpe.nRegVal;
   }
   else
   {
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
        ("Read CMDREG%d 0x%x\n", 1, reg1_value));

   if (IFX_TRUE == fMaster)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Board mode set to PCM MASTER mode\n"));
      reg1_value |= MASTER_CPE;
      reg1_value |= PCMEN_MASTER_CPE;
      reg1_value |= PCMON_CPE;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Board mode set to PCM SLAVE mode\n"));
      reg1_value &= ~MASTER_CPE;
      reg1_value &= ~PCMEN_MASTER_CPE;
      reg1_value |= PCMON_CPE;
   }

   memset(&reg_cpe, 0, sizeof(EASY3332_Reg_t));

   reg_cpe.nRegOffset = CMDREG1_OFF_CPE;
   reg_cpe.nRegVal = reg1_value;
   /* Read register */
   ret = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3332_CPLDREG_WRITE,
                  (IFX_int32_t) &reg_cpe, TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (ret != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, writing value 0x%0X to CPLD register. "
            "(File: %s, line: %d)\n", reg_cpe.nRegVal, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
        ("Write CMDREG%d 0x%x\n", 1, reg1_value));

   return IFX_SUCCESS;
} /* BVINETIC_Set_CPLD_Reg_For_PCM() */

/* ============================= */
/* Global function definition    */
/* ============================= */
/**
   Release board structure (reserve memory, inicalize, ...).

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called before application exit.
*/
IFX_void_t BOARD_Easy3332_RemoveBoard(BOARD_t* pBoard)
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
      This must be done only after a successfull basic device initialization
*/
IFX_return_t BOARD_Easy3332_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg)
{
   IFX_boolean_t we_are_master = IFX_FALSE;
   IFX_boolean_t use_pcm_local_loop = IFX_FALSE;

    /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if (BOARD_Easy3332_PrepareBoard(pBoard, pProgramArg->sPathToDwnldFiles)
       == IFX_ERROR)
   {
       return IFX_ERROR;
   }

   /* Initialize chip */
   if (DEVICE_VINETIC_CPE_Init(pBoard, pProgramArg->sPathToDwnldFiles)
       == IFX_ERROR)
   {
       return IFX_ERROR;
   }

   /* Initialize PCM if used (just set values to some CPLD registers) */
   if ((pProgramArg->oArgFlags.nPCM_Master)
       || (pProgramArg->oArgFlags.nPCM_Slave))
   {
      if ((pBoard->nID - 1) == 0)
      {
         if (pProgramArg->oArgFlags.nPCM_Master)
         {
            we_are_master = IFX_TRUE;
         }

         if (pProgramArg->oArgFlags.nPCM_Loop)
         {
            use_pcm_local_loop = IFX_TRUE;
         }

         if (BOARD_Easy3332_Set_CPLD_Reg_For_PCM(we_are_master, pBoard,
                                         use_pcm_local_loop) != IFX_SUCCESS)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, PCM initialization. (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
   }
   return IFX_SUCCESS;
}

/**
   Set/Clear reset of device.

   \param pBoard - pointer to Board
   \param nArg - id of device to reset
   \param hSetReset - IFX_TRUE - set reset, IFX_FALSE - clear reset

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t BOARD_Easy3332_Reset(BOARD_t* pBoard,
                                  IFX_int32_t nArg,
                                  IFX_boolean_t hSetReset)
{
   EASY3332_Reset_t reset_cpe;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t nDevNum = nArg;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&reset_cpe, 0, sizeof(EASY3332_Reset_t));

   reset_cpe.nChipNum = nDevNum;
   if (hSetReset)
   {
      reset_cpe.nResetMode = EASY3332_RESET_ACTIVE;
   }
   else
   {
      reset_cpe.nResetMode = EASY3332_RESET_DEACTIVE;
   }
   ret = TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3332_RESETCHIP,
                  (IFX_int32_t) &reset_cpe, nDevNum, TD_CONN_ID_INIT);
   if (ret != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, iocl cmd FIO_EASY3332_RESETCHIP, dev no: %d, SetReset: %d"
            "(File: %s, line: %d)\n",
             nDevNum, hSetReset, __FILE__, __LINE__));
   }
   return ret;
}

/**
   Register board.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
*/
IFX_return_t BOARD_Easy3332_Register(BOARD_t* pBoard)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pBoard->pszBoardName = BOARD_NAME_EASY3332;
   pBoard->Init = BOARD_Easy3332_Init;
   pBoard->RemoveBoard = BOARD_Easy3332_RemoveBoard;

   return IFX_SUCCESS;
}

