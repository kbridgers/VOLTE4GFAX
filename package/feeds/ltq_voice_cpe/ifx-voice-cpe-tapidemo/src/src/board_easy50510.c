/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file board_easy50510.c
   \date 2006-10-01
   \brief Interface implementation for EASY50510 board.

   Application needs it to initialize board, prepare structures for this board
   and set mode in which board will work (using PCM, GPIO, ...).
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"
#include "board_easy50510.h"
#include "board_easy50712.h"

/* ============================= */
/* Defines                       */
/* ============================= */

#define TD_EASY50510_DEVICE_NUMBER 1
#define TD_EASY50510_MAX_CHANNEL_NUMBER 8

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

/** Number of PCM highways */
enum { PCM_HIGHWAYS = 1 };

/** Name of board */
static const char* BOARD_NAME_EASY50510 = "EASY50510";

/** Holds default phone channel to data, pcm channel mappings
    defined at startup. */
static const STARTUP_MAP_TABLE_t STARTUP_MAPPING_CPE[MAX_PHONES_CPE] =
{
   /** phone, data, pcm */
   {  0,  0,  0  },
   {  1,  1,  1  }
   /* others are free and can be used later for conferencing */
};

/** Device number to detect */
static IFX_int32_t nDeviceToDetect = 1;

/** Detect boards */
static IFX_boolean_t nDetectDevice = IFX_TRUE;

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t BOARD_Easy50510_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t BOARD_Easy50510_PrepareNames(BOARD_t* pBoard);
IFX_return_t BOARD_Easy50510_InitSystem(BOARD_t* pBoard, IFX_int32_t nGPIO_PortFd);
IFX_return_t BOARD_Easy50510_GetCapabilities(BOARD_t* pBoard,
                                             IFX_char_t* pPath);
IFX_void_t BOARD_Easy50510_RemoveBoard(BOARD_t* pBoard);
IFX_return_t BOARD_Easy50510_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg);

/* ============================= */
/* Local function definition     */
/* ============================= */
/**
   Prepare board structure (reserve memory, inicialize, ...).

   \param pBoard - pointer to Board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t BOARD_Easy50510_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPath, IFX_ERROR);

   IFX_int32_t i = 0;
   TAPIDEMO_DEVICE_t* pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t* pCPUDevice = IFX_NULL;
   IFX_int32_t nGPIO_PortFd = NO_GPIO_FD;
   IFX_return_t nRet = IFX_SUCCESS;

   /* Variables id and type set before this func is called */

   /* File descriptors not open */
   pBoard->nSystem_FD = -1;
   pBoard->nDevCtrl_FD = -1;
   /* Set flag NOT to use system FD */
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
   pCPUDevice->pDevDependStuff = (IFX_void_t *) &VineticDevInitStruct;
   pCPUDevice->AccessMode = ACCESS_MODE;
   pCPUDevice->nBaseAddress = BASE_ADDRESS;
   pCPUDevice->nIrqNum = IRQ_NUMBER;
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
   /* We don't need to set download path. It was done during inititialize main board */
   /* Just check if directory contains required file for extension board*/
   if (IFX_SUCCESS != Common_CheckDownloadPath(pPath, pCPUDevice, IFX_TRUE,
                                               TD_CONN_ID_INIT))
   {
      return IFX_ERROR;
   }
#endif /* VXWORKS */
#endif /* USE_FILESYSTEM */

   if (IFX_SUCCESS != Common_AddDevice(pBoard, pDevice))
   {
      TD_OS_MemFree(pDevice);
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not added to the board. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (IFX_SUCCESS != DEVICE_VINETIC_CPE_Register(pCPUDevice))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not registered. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (IFX_SUCCESS != BOARD_Easy50510_PrepareNames(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Can't prepare names. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /** Startup mapping table */
   pBoard->pChStartupMapping = STARTUP_MAPPING_CPE;

   /* Open GPIO port fd */
   nGPIO_PortFd  = Common_GPIO_OpenPort(TD_GPIO_PORT_NAME);
   if (NO_GPIO_FD == nGPIO_PortFd)
   {
      return IFX_ERROR;
   }

   /* Set PCM configuration (master, ...) over GPIO port */
   /* karol - check if it is necessary. */
   TD_GPIO_EASY50510_SetPCM_Master(nGPIO_PortFd, nRet);
   if (IFX_SUCCESS != nRet)
   {
      Common_GPIO_ClosePort(TD_GPIO_PORT_NAME, nGPIO_PortFd);
      return IFX_ERROR;
   }

   /* Set pin direction to output */
   TD_GPIO_EASY50510_ResetConfig(nGPIO_PortFd, nRet);
   if (IFX_SUCCESS != nRet)
   {
      Common_GPIO_ClosePort(TD_GPIO_PORT_NAME, nGPIO_PortFd);
      return IFX_ERROR;
   }

   /* Basic device initialization */
   if (IFX_SUCCESS != BOARD_Easy50510_InitSystem(pBoard, nGPIO_PortFd))
   {
      Common_GPIO_ClosePort(TD_GPIO_PORT_NAME, nGPIO_PortFd);;
      return IFX_ERROR;
   }

   if (IFX_SUCCESS != Common_GPIO_ClosePort(TD_GPIO_PORT_NAME, nGPIO_PortFd))
   {
      return IFX_ERROR;
   }

   if (IFX_SUCCESS != DEVICE_VINETIC_CPE_setupChannel(pBoard, pPath, IFX_TRUE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("%s: Unable to initialize extension board.\n"
            "%sDevice node %s is set without extension board being connected\n"
            "%sor initalization error occured.\n",
            pBoard->pszBoardName, pBoard->pIndentBoard, pBoard->pszCtrlDevName,
            pBoard->pIndentBoard));
      TD_OS_DeviceClose(pBoard->nDevCtrl_FD);
      return IFX_ERROR;
   }

   if (IFX_SUCCESS != BOARD_Easy50510_GetCapabilities(pBoard, pPath))
   {
      return IFX_ERROR;
   }

   if (IFX_SUCCESS != Common_SetPhonesAndUsedChannlesNumber(pBoard,
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
            ("Err, Allocate memory for PCM channels. "
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
            ("Err, Setting fds. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

/**
   Prepare device/channels names.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called before application exit.
*/
IFX_return_t BOARD_Easy50510_PrepareNames(BOARD_t* pBoard)
{
   IFX_char_t tmp_buf[10] = {0};

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

    /*  strlen(string) + terminating null-character */
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCH_DEV_NAME_CPE)+1),
                      strlen(sCH_DEV_NAME_CPE), IFX_ERROR);
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCTRL_DEV_NAME_CPE)+1),
                      strlen(sCTRL_DEV_NAME_CPE), IFX_ERROR);

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

   return IFX_SUCCESS;
}

#ifndef STREAM_1_1
/**
   Configure PCM interface, can define clock, master/slave mode, ...

   \param pBoard  - pointer to board
   \param hMaster  - IFX_TRUE - master mode, IFX_FALSE - slave mode

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR

   \remark
*/
IFX_return_t BOARD_Easy50510_CfgPCMInterface(BOARD_t* pBoard,
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
               ("Err, configure PCM interface (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* for */

   /* set PCM frequency - should be only set for main board - not extension */
   /* g_oPCM_Data.nPCM_Rate = pcm_if.nDCLFreq; */

   return IFX_SUCCESS;
} /* BOARD_Easy50510_CfgPCMInterface() */

#else /* !STREAM_1_1 */

IFX_return_t BOARD_Easy50510_CfgPCMInterface(BOARD_t* pBoard,
                                             IFX_boolean_t hMaster)
{
   return IFX_SUCCESS;
} /* BOARD_Easy50510_CfgPCMInterface() */
#endif /* !STREAM_1_1 */

/**
   Get capabilities, basically number of phone, data and pcm channels.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy50510_GetCapabilities(BOARD_t* pBoard,
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
   Basic initialization prior to vinetic initialization.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      - In case of 8bit SPI, for the CPE system, base address  should be set to
        0x1F if not otherwise programmed by bootstrapping
      - Caller must make sure that the access mode is  supported
*/
IFX_return_t BOARD_Easy50510_InitSystem(BOARD_t* pBoard, IFX_int32_t nGPIO_PortFd)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((NO_GPIO_FD >= nGPIO_PortFd), nGPIO_PortFd, IFX_ERROR);

   IFX_return_t nRet = IFX_SUCCESS;

   /* Store chip count */
   pBoard->nChipCnt = 1;
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

   /* activate chip reset */
   TD_GPIO_EASY50510_Reset(nGPIO_PortFd, nRet, IFX_TRUE);
   if (nRet != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }

   if (IFX_SUCCESS != DEVICE_VINETIC_CPE_BasicInit(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Basic Device Initialization fails"
            "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* clear reset to allow vinetic access */
   TD_GPIO_EASY50510_Reset(nGPIO_PortFd, nRet, IFX_FALSE);
   if (nRet != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* BOARD_Easy50510_InitSystem */

/* ============================= */
/* Global function definition    */
/* ============================= */
/**
   Release board structure (reserve memory, inicalize, ...).

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called before application exit.
*/
IFX_void_t BOARD_Easy50510_RemoveBoard(BOARD_t* pBoard)
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
IFX_return_t BOARD_Easy50510_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   if (IFX_SUCCESS !=
       BOARD_Easy50510_PrepareBoard(pBoard, pProgramArg->sPathToDwnldFiles))
   {
      return IFX_ERROR;
   }

   /* Initialize chip */
   if (IFX_SUCCESS !=
       DEVICE_VINETIC_CPE_Init(pBoard, pProgramArg->sPathToDwnldFiles))
   {
      return IFX_ERROR;
   }

   /* Initialize the PCM Interface. This is extension board
      so set PCM slave mode. */
   if (IFX_SUCCESS != BOARD_Easy50510_CfgPCMInterface(pBoard, IFX_FALSE))
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Register board.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
*/
IFX_return_t BOARD_Easy50510_Register(BOARD_t* pBoard)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pBoard->pszBoardName = BOARD_NAME_EASY50510;
   pBoard->Init = BOARD_Easy50510_Init;
   pBoard->RemoveBoard = BOARD_Easy50510_RemoveBoard;

   return IFX_SUCCESS;
}

 /*
   Detect node required by extension board EASY50510.

   \param  pCtrl  - pointer to status control structure

   \return board ID
 */
IFX_int32_t BOARD_Easy50510_DetectNode(CTRL_STATUS_t* pCtrl)
{
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   IFX_char_t pszDevNameWithPath[MAX_DEV_NAME_LEN_WITH_NUMBER];
   IFX_int32_t nLen;
   IFX_int32_t fd = -1;
   IFX_int32_t nDevNum;

   /* Check if board should be detected */
   if (IFX_TRUE != nDetectDevice)
   {
      /* Don't detect this board anymore */
      return NO_EXT_BOARD;
   }

   /* Max number of board is limited */
   if (nDeviceToDetect > TD_EASY50510_DEVICE_NUMBER)
   {
      nDetectDevice = IFX_FALSE;
      return NO_EXT_BOARD_DETECT;
   }

   /* Search for board node */

   /* clean up string */
   memset(pszDevNameWithPath, 0,
          sizeof(IFX_char_t) * MAX_DEV_NAME_LEN_WITH_NUMBER);

   /* set string */
   nLen = snprintf(pszDevNameWithPath, MAX_DEV_NAME_LEN_WITH_NUMBER,
                   "%s%d0", sCTRL_DEV_NAME_CPE, nDeviceToDetect);

   /* check return value */
   if ((0 > nLen) || (MAX_DEV_NAME_LEN_WITH_NUMBER <= nLen))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Error: Unable to create string - it's too long (size: %d).\n"
             "(File: %s, line: %d)\n",
             nLen, __FILE__, __LINE__));
      return NO_EXT_BOARD_DETECT;
   }
   /* Try to open node */
   fd = Common_Open(pszDevNameWithPath, IFX_NULL);
   if (0 <= fd)
   {
      nDevNum = nDeviceToDetect;
      /* Increase device number for next board */
      nDeviceToDetect++;
      TD_OS_DeviceClose(fd);
      return nDevNum;
   }

   /* Don' detect this board anymore */
   nDetectDevice = IFX_FALSE;

   return NO_EXT_BOARD_DETECT;
} /* BOARD_Easy50510_DetectNode */


