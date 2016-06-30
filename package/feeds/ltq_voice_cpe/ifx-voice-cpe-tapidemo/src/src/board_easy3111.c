/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file board_easy3111.c
   \date 2006-10-01
   \brief Interface implementation for easy3111 board.

   Application needs it to initialize board, prepare structures for this board
   and set mode in which board will work (using PCM, GPIO, ...).
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"
#include "board_easy3201.h"
#include "board_easy3111.h"

#ifdef EASY3111
#include "pcm.h"
#include "voip.h"
#endif /* EASY3111 */
/* ============================= */
/* Defines                       */
/* ============================= */

/* Board EASY3111 uses the same function as a board EASY3201 */
#define BOARD_Easy3111_CfgPCMInterfaceSlave BOARD_Easy3201_CfgPCMInterfaceSlave

#ifdef HAVE_DRV_EASY3201_HEADERS
#define BOARD_Easy3111_Reset BOARD_Easy3201_Reset
#endif /* HAVE_DRV_EASY3201_HEADERS */

#if defined (EASY3201) || defined (EASY3201_EVS)
/** this is the second device of this type (/dev/dxt20)
 *  first one is on main board (EASY3201) */
#define TD_EASY3111_DEVICE_NUMBER 2
#else /* EASY3201 */
/** this is the first device of this type (/dev/dxt10) */
#define TD_EASY3111_DEVICE_NUMBER 1
#endif /* EASY3201 */

/* ============================= */
/* Local variable definition     */
/* ============================= */


/** How channels are represented by device, socket.
    Device /dev/vin11 will get status, data from phone channel 0 and
    data channel 0. If at startup phone channel 0 is mapped to data channel 1
    then, phone will send actions to dev/vin11 and data will send actions to
    /dev/vin12.
    -1 means no channel at startup */

/** Max phones on EASY3111 (analog channels)
    It is used only for STARTUP_MAPPING_EASY3111[] table.
    Number of analog channels used in program is read from drivers
    using IFX_TAPI_CAP_LIST  */
enum { MAX_PHONES_EASY3111 = 2 };

/** Name of board */
static const char* BOARD_NAME_EASY3111 = "EASY3111";

/** Holds default phone channel to data, pcm channel mappings
    defined at startup. */
static const STARTUP_MAP_TABLE_t STARTUP_MAPPING_EASY3111[MAX_PHONES_EASY3111] =
{
   /* IMORTANT - for this board this is default mapping done by drivers,
      to change this mapping channels(phone and PCM) must be unmapped first */
   /** phone, data, pcm */
   {  0,  0,  0  },
   {  1,  1,  1  }
   /* others are free and can be used later for conferencing */
};

#if defined (EASY3201) || defined (EASY3201_EVS)
/** Device number to detect, device number 1 is on main board */
static IFX_int32_t nDeviceToDetect = 2;
#else /* EASY3201 */
/** Device number to detect, main board uses different device */
static IFX_int32_t nDeviceToDetect = 1;
#endif /* EASY3201 */

/** Detect boards */
static IFX_boolean_t nDetectDevice = IFX_TRUE;

#ifdef EASY3111
extern TD_ENUM_2_NAME_t TD_rgCallTypeName[];
#endif /* EASY3111 */

#ifdef EASY3111
/** IRQ number for EASY 3111 */
#define TD_EASY_3111_IRQ               0x20
#endif /* EASY3111 */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t BOARD_Easy3111_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath);
IFX_return_t BOARD_Easy3111_PrepareNames(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3111_InitSystem(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3111_GetCapabilities(BOARD_t* pBoard,
                                            IFX_char_t* pPath);
IFX_void_t BOARD_Easy3111_RemoveBoard(BOARD_t* pBoard);
IFX_return_t BOARD_Easy3111_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg);

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
IFX_return_t BOARD_Easy3111_PrepareBoard(BOARD_t* pBoard, IFX_char_t* pPath)
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
#ifndef EASY3111
   /* Set flag to use system FD */
   pBoard->nUseSys_FD = IFX_TRUE;
#else /* EASY3111 */
   /* Do not use system FD when extension for vmmc */
   pBoard->nUseSys_FD = IFX_FALSE;
#endif /* EASY3111 */

   /* allocate memory for CPU device structure */
   pDevice = TD_OS_MemCalloc(1, sizeof(TAPIDEMO_DEVICE_t));
   if (pDevice == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err Allocate memory for CPU Device. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Init structure for CPU device and add it to the board */
   pDevice->nType = TAPIDEMO_DEV_CPU;
   pCPUDevice = &pDevice->uDevice.stCpu;

   if (BOARD_Easy3111_PrepareNames(pBoard) == IFX_ERROR)
      return IFX_ERROR;

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
           ("ERR, CPU device not registered. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /** Startup mapping table */
   pBoard->pChStartupMapping = STARTUP_MAPPING_EASY3111;

   /* Basic device initialization */
   if (BOARD_Easy3111_InitSystem(pBoard) == IFX_ERROR)
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
   /* We don't need to set download path. It was done during inititialize main board */
   /* Just check if directory contains required file for extension board*/
   if (IFX_SUCCESS != Common_CheckDownloadPath(pPath, pCPUDevice, IFX_TRUE,
                                               TD_CONN_ID_INIT))
   {
      return IFX_ERROR;
   }
#endif /* VXWORKS */
#endif /* USE_FILESYSTEM */

   if (IFX_ERROR == DEVICE_DUSLIC_XT_setupChannel(pBoard, pPath, IFX_TRUE))
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


   if (IFX_ERROR == BOARD_Easy3111_GetCapabilities(pBoard, pPath))
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
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
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
            "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* All PCM channels are free at start */
   for (i = 0; i < pBoard->nMaxPCM_Ch; i++)
   {
      pBoard->fPCM_ChFree[i] = IFX_TRUE;
   }
#ifdef EASY3111
   /* reset values for all phones - needed when board is used as extension board
      for board other than EASY 3201 */
   for (i = 0; i < pBoard->nMaxPhones; i++)
   {
      pBoard->rgoPhones[i].oEasy3111Specific.fToMainConnActive = IFX_FALSE;
      pBoard->rgoPhones[i].oEasy3111Specific.nOnMainPCM_Ch = TD_NOT_SET;
      pBoard->rgoPhones[i].oEasy3111Specific.nOnMainPCM_ChFD = TD_NOT_SET;
      pBoard->rgoPhones[i].oEasy3111Specific.nTimeslot = TD_NOT_SET;
   }
#endif /* EASY3111 */
   if (IFX_SUCCESS != Common_Set_FDs(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, setting fds (File: %s, line: %d)\n",
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
IFX_return_t BOARD_Easy3111_PrepareNames(BOARD_t* pBoard)
{
   IFX_char_t tmp_buf[10] = {0};

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

    /*  strlen(string) + terminating null-character */
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCH_DEV_NAME_DXT)+1),
                      strlen(sCH_DEV_NAME_DXT), IFX_ERROR);
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sCTRL_DEV_NAME_DXT)+1),
                      strlen(sCTRL_DEV_NAME_DXT), IFX_ERROR);
#ifdef HAVE_DRV_EASY3201_HEADERS
   TD_PARAMETER_CHECK((DEV_NAME_LEN < strlen(sSYS_DEV_NAME_DXT)+1),
                      strlen(sSYS_DEV_NAME_DXT), IFX_ERROR);
#endif /* HAVE_DRV_EASY3201_HEADERS */

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

#ifdef HAVE_DRV_EASY3201_HEADERS
   /** System device name, like /dev/easy3201/[<board id], ... */
   /** BUT actually we only have one */
   memset(pBoard->pszSystemDevName, 0, DEV_NAME_LEN * sizeof(IFX_char_t));
   strncpy(pBoard->pszSystemDevName, sSYS_DEV_NAME_DXT,
           strlen(sSYS_DEV_NAME_DXT));
   pBoard->pszSystemDevName[strlen(sSYS_DEV_NAME_DXT)] = '\0';
#endif /* HAVE_DRV_EASY3201_HEADERS */

   return IFX_SUCCESS;
}

/**
   Get capabilities, basically number of phone, data and pcm channels.

   \param pBoard - pointer to board
   \param pPath - path to FW files for download

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy3111_GetCapabilities(BOARD_t* pBoard,
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

   if (pBoard->nMaxAnalogCh > MAX_PHONES_EASY3111)
   {
      /* If MAX_PHONES_EASY3111 is too small than STARTUP_MAPPING_EASY3111[] table
         has too small size and TAPIDEMO will from wrong place */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, MAX_PHONES_EASY3111=%d is too small (<%d)."
            " (File: %s, line: %d)\n",
            MAX_PHONES_EASY3111, pBoard->nMaxAnalogCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return ret;
}
/**
   Basic device initialization prior to DuSLIC-XT initialization.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      DEVICE_DUSLIC_XT_BasicInit for this board is done
         in BOARD_Easy3201_InitSystem()
*/
IFX_return_t BOARD_Easy3111_InitSystem(BOARD_t* pBoard)
{
   IFX_uint32_t nChipNum = 0;
#ifdef HAVE_DRV_EASY3201_HEADERS
   EASY3201_CPLD_GET_DEV_PARAM_t chip_param_dxt;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t   *pCpuDevice = IFX_NULL;
#else /* HAVE_DRV_EASY3201_HEADERS */
   IFX_int32_t nIrqNum;
#endif /* HAVE_DRV_EASY3201_HEADERS */

   /* check input parameters */
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

   /* set chip number */
   nChipNum = 1;
   /* Store chip count */
   pBoard->nChipCnt = 1;

#ifdef HAVE_DRV_EASY3201_HEADERS
   if (0 > pBoard->nSystem_FD)
   {
      /* Get system fd */
      pBoard->nSystem_FD = Common_Open(pBoard->pszSystemDevName, pBoard);

      if (0 > pBoard->nSystem_FD)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Invalid system file descriptor %s. (File: %s, line: %d)\n",
               pBoard->pszSystemDevName, __FILE__, __LINE__));
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

   /* activate chip reset */
   if (BOARD_Easy3111_Reset(pBoard, nChipNum, IFX_TRUE) != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err: Basic Device Initialization fails on device %d"
            "(File: %s, line: %d)\n",
            nChipNum, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&chip_param_dxt, 0, sizeof(EASY3201_CPLD_GET_DEV_PARAM_t));
   chip_param_dxt.nChipNum = nChipNum;

   /* read dxt device configuration parameters from board driver */
   if (TD_IOCTL(pBoard->nSystem_FD, FIO_EASY3201_GETBOARDPARAMS,
          (IFX_int32_t) &chip_param_dxt, TD_DEV_NOT_SET, TD_CONN_ID_INIT)
       != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }

   /* set Irq Line number */
   pCpuDevice->nIrqNum = chip_param_dxt.nIrqNum;

   /* clear reset to allow vinetic access */
   if (BOARD_Easy3111_Reset(pBoard, nChipNum, IFX_FALSE) != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Basic Device Initialization fails on device %d"
             "(File: %s, line: %d)\n",
             nChipNum, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#elif TD_HAVE_DRV_BOARD_HEADERS
   /* here should be part of code for EASY3201_EVS */
#else /* HAVE_DRV_EASY3201_HEADERS */
#if 0
   /* code left in case additional debug information will be needed */
   /* Set report to low */
   if (IFX_SUCCESS != TD_IOCTL(pBoard->nDevCtrl_FD, FIO_DXT_REPORT_SET, 0x1,
                         TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, set report level failed.\n"));
      return IFX_ERROR;
   }
#endif /* 0 */
   /* check if polling mode should be used */
   if (pBoard->pCtrl->pProgramArg->oArgFlags.nPollingMode)
   {
      nIrqNum = TD_USE_POLLING_MODE;
   }
   else
   {
      nIrqNum = TD_EASY_3111_IRQ;
   }
   if (IFX_SUCCESS != DEVICE_DUSLIC_XT_BasicInit(pBoard->nDevCtrl_FD,
                                                 nIrqNum))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, basic init failed."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* reset chip */
   if (TD_IOCTL(pBoard->nDevCtrl_FD, FIO_DXT_CHIP_RESET, 0, TD_DEV_NOT_SET,
          TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }
#endif /* HAVE_DRV_EASY3201_HEADERS */
   return IFX_SUCCESS;
} /* BOARD_Easy3111_InitSystem() */

/* ============================= */
/* Global function definition    */
/* ============================= */
#ifdef EASY3111
/**
   Activate PCM connection between main and extension board.

   \param pPhone  - pointer to phone structure
   \param pCtrl   - pointer to control structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t Easy3111_CallPCM_Activate(PHONE_t* pPhone, CTRL_STATUS_t* pCtrl)
{
   TAPIDEMO_PORT_DATA_t oPortData;
   IFX_boolean_t fWideBand = IFX_FALSE;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* debug trace */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: %s activate connection to main.\n",
          pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));
   /* check if resources are allocated */
   if (TD_NOT_SET == pPhone->oEasy3111Specific.nOnMainPCM_Ch)
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Resources are not allocated.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;

   }
   /* check if connection is active */
   if (IFX_TRUE == pPhone->oEasy3111Specific.fToMainConnActive)
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Connection to main already active.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;

   }

   /* free data channel */
   oPortData.nType = PORT_FXS;
   oPortData.uPort.pPhopne = pPhone;
   /* set WideBand if needed */
   if (TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
   }
   /* set activation of channel on extension board */
   if (IFX_SUCCESS != PCM_SetActivation(&oPortData,
                         PCM_GetFD_OfCh(pPhone->nPCM_Ch, pPhone->pBoard),
                         IFX_TRUE,
                         pPhone->oEasy3111Specific.nTimeslot,
                         pPhone->oEasy3111Specific.nTimeslot, fWideBand,
                         pPhone->nSeqConnId))
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to activate PCM channel %d on extension board.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPCM_Ch, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set activation of channel on main board,
      RX and TX timeslots numbers are switched
      (here those are the same nubers) */
   if (IFX_SUCCESS != PCM_SetActivation(&oPortData,
                         pPhone->oEasy3111Specific.nOnMainPCM_ChFD,
                         IFX_TRUE,
                         pPhone->oEasy3111Specific.nTimeslot,
                         pPhone->oEasy3111Specific.nTimeslot, fWideBand,
                         pPhone->nSeqConnId))
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to activate PCM channel %d on main board.\n"
             "(File: %s, line: %d)\n",
             pPhone->oEasy3111Specific.nOnMainPCM_Ch, __FILE__, __LINE__));
      /* deactivate PCM channel */
      PCM_SetActivation(&oPortData,
                        PCM_GetFD_OfCh(pPhone->nPCM_Ch, pPhone->pBoard),
                        IFX_FALSE,
                        pPhone->oEasy3111Specific.nTimeslot,
                        pPhone->oEasy3111Specific.nTimeslot, fWideBand,
                        pPhone->nSeqConnId);
      return IFX_ERROR;
   }
   /* set connection flag */
   pPhone->oEasy3111Specific.fToMainConnActive = IFX_TRUE;
   return IFX_SUCCESS;
}

/**
   Activate PCM connection between main and extension board.

   \param pPhone  - pointer to phone structure
   \param pCtrl   - pointer to control structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t Easy3111_CallPCM_Deactivate(PHONE_t* pPhone, CTRL_STATUS_t* pCtrl)
{
   TAPIDEMO_PORT_DATA_t oPortData;
   IFX_boolean_t fWideBand = IFX_FALSE;
   IFX_return_t nRet = IFX_SUCCESS;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* debug trace */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: %s deactivate connection to main.\n",
          pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));
   /* check if resources are allocated */
   if (TD_NOT_SET == pPhone->oEasy3111Specific.nOnMainPCM_Ch)
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Resources are not allocated.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;

   }
   /* check if connection is active */
   if (IFX_TRUE != pPhone->oEasy3111Specific.fToMainConnActive)
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Connection to main is not active.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;

   }

   /* free data channel */
   oPortData.nType = PORT_FXS;
   oPortData.uPort.pPhopne = pPhone;
   /* set WideBand if needed */
   if (IFX_TRUE == pPhone->fWideBand_PCM_Cfg)
   {
      fWideBand = IFX_TRUE;
   }
   /* set deactivation of channel on extension board */
   if (IFX_SUCCESS != PCM_SetActivation(&oPortData,
                         PCM_GetFD_OfCh(pPhone->nPCM_Ch, pPhone->pBoard),
                         IFX_FALSE,
                         pPhone->oEasy3111Specific.nTimeslot,
                         pPhone->oEasy3111Specific.nTimeslot, fWideBand,
                         pPhone->nSeqConnId))
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to deactivate PCM channel %d on extension board.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPCM_Ch, __FILE__, __LINE__));
      nRet = IFX_ERROR;
   }
   /* set activation of channel on main board,
      RX and TX timeslots numbers are switched
      (here those are the same nubers) */
   if (IFX_SUCCESS != PCM_SetActivation(&oPortData,
                         pPhone->oEasy3111Specific.nOnMainPCM_ChFD,
                         IFX_FALSE,
                         pPhone->oEasy3111Specific.nTimeslot,
                         pPhone->oEasy3111Specific.nTimeslot, fWideBand,
                         pPhone->nSeqConnId))
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to deactivate PCM channel %d on main board.\n"
             "(File: %s, line: %d)\n",
             pPhone->oEasy3111Specific.nOnMainPCM_Ch, __FILE__, __LINE__));
      nRet = IFX_ERROR;
   }
   /* set connection flag */
   pPhone->oEasy3111Specific.fToMainConnActive = IFX_FALSE;
   return nRet;
}

/**
   Check if additional PCM channel is needed for FXO call.

   \param pPhone  - pointer to phone structure
   \param pConn   - pointer to connection structure

   \return IFX_TRUE if PCM channel allocation is needed for this FXO call.

   \remark .
*/
IFX_boolean_t Easy3111_PCM_IsNeededForFXO(PHONE_t *pPhone, CONNECTION_t *pConn)
{
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_FALSE);
   TD_PTR_CHECK(pConn, IFX_FALSE);

   /* check if outgoing FXO call */
   if (FXO_CALL != pConn->nType || CALL_DIRECTION_TX != pPhone->nCallDirection)
   {
      return IFX_FALSE;
   }
   /* for SLIC121 PCM channel isn't needed */
   if (IFX_NULL != pPhone->pFXO &&
       FXO_TYPE_SLIC121 != pPhone->pFXO->pFxoDevice->nFxoType)
   {
      return IFX_TRUE;
   }
   return IFX_FALSE;
}

/**
   Gets resources needed to establish voice connection to extension board.

   \param pPhone  - pointer to phone structure
   \param pCtrl   - pointer to control structure
   \param pConn   - pointer to useed connection structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark On DuSLIC-xT channel mapping is not available.
           To establish voice connection PCM channels and PCM timeslots
           must be used. Connection between two phones on EASY 3111 must be done
           through vmmc board that is set as PCM master. Connection between
           two PCM channels using PCM timeslots can only be done
           if one is master and the second one is slave.
           On vmmc board 2 PCM channels are mapped together.
*/
IFX_return_t Easy3111_CallResourcesGet(PHONE_t *pPhone, CTRL_STATUS_t *pCtrl,
                                       CONNECTION_t *pConn)
{
   BOARD_t *pBoardMain;
   IFX_boolean_t fWideBand = IFX_FALSE;
   IFX_boolean_t fGetPCM_ForFXO = IFX_FALSE;
   IFX_return_t nRet = IFX_SUCCESS;
   IFX_int32_t nTimeslot = TD_NOT_SET;
   IFX_int32_t nOnMainPCM_Ch = TD_NOT_SET;
   IFX_int32_t nOnMainPCM_ChFD = TD_NOT_SET;
   IFX_int32_t nOnMainPCM_ChSecondary = TD_NOT_SET;
   IFX_int32_t nOnMainPCM_ChSecondaryFD = TD_NOT_SET;
   IFX_int32_t nOnMainDataCh = TD_NOT_SET;
   IFX_int32_t nOnMainDataChFD = TD_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* debug trace */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: %s get resources.\n",
          pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));
   /* check on which board phone is located */
   if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid board type %s for phone No %d.\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName, pPhone->nPhoneNumber,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check number of boards */
   if (pCtrl->nBoardCnt > 1)
   {
      /* this function is implemented for board configuration where
         DuSLIC-xT chip is located only on extension board */
      if (TYPE_DUSLIC_XT != pCtrl->rgoBoards[TD_MAIN_BOARD_NUMBER].nType)
      {
         pBoardMain = TD_GET_MAIN_BOARD(pCtrl);
      } /* if (TYPE_DUSLIC_XT != ...Boards[TD_MAIN_BOARD_NUMBER].nType) */
      else
      {
         /* print error trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d: Invalid board type %s.\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName,
                __FILE__, __LINE__));
         return IFX_ERROR;
      } /* if (TYPE_DUSLIC_XT != ...Boards[TD_MAIN_BOARD_NUMBER].nType) */
   } /* if (pCtrl->nBoardCnt > 1) */
   else
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid number of boards %d.\n"
             "(File: %s, line: %d)\n",
             pCtrl->nBoardCnt, __FILE__, __LINE__));
      return IFX_ERROR;
   } /* if (pCtrl->nBoardCnt > 1) */

   /* check if supported call type */
   if (LOCAL_CALL != pConn->nType &&
       EXTERN_VOIP_CALL != pConn->nType &&
       PCM_CALL != pConn->nType &&
       FXO_CALL != pConn->nType)
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Unhandled call type %s(%d).\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber,
             Common_Enum2Name(pConn->nType, TD_rgCallTypeName), pConn->nType,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if resources aren't already allocated */
   if (TD_NOT_SET != pPhone->oEasy3111Specific.nOnMainPCM_Ch)
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Resources already allocated (%d).\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->oEasy3111Specific.nOnMainPCM_Ch,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* set WideBand if needed */
   if (pBoardMain->pCtrl->pProgramArg->oArgFlags.nUsePCM_WB ||
       TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
   }
   /* get PCM channel on main board to communicate with extension board */
   if (IFX_SUCCESS == nRet)
   {
      /* get free PCM channel */
      nOnMainPCM_Ch = PCM_GetFreeCh(pBoardMain);
      if (NO_FREE_DATA_CH == nOnMainPCM_Ch)
      {
         /* print error trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, No free PCM channels available on board %s.\n"
                "(File: %s, line: %d)\n",
                pBoardMain->pszBoardName, __FILE__, __LINE__));
         /* reset value and set error */
         nOnMainPCM_Ch = TD_NOT_SET;
         nRet = IFX_ERROR;
      } /* get free PCM channel */
      else
      {
         /* reserve PCM channel */
         if (IFX_SUCCESS != PCM_ReserveCh(nOnMainPCM_Ch, pBoardMain,
                                          pPhone->nSeqConnId))
         {
            /* print error trace */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Failed to reserve PCM channel %d on %s.\n"
                   "(File: %s, line: %d)\n",
                   nOnMainPCM_Ch, pBoardMain->pszBoardName,
                   __FILE__, __LINE__));
            /* reset value and set error */
            nOnMainPCM_Ch = TD_NOT_SET;
            nRet = IFX_ERROR;
         }
         else
         {
            nOnMainPCM_ChFD = PCM_GetFD_OfCh(nOnMainPCM_Ch, pBoardMain);
            if (NO_DEVICE_FD == nOnMainPCM_ChFD)
            {
               /* print error trace */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, Failed to get FD of PCM channel %d on %s.\n"
                      "(File: %s, line: %d)\n",
                      nOnMainPCM_Ch, pBoardMain->pszBoardName,
                      __FILE__, __LINE__));
               /* set error */
               nRet = IFX_ERROR;
            }
         }
      } /* get free PCM channel */
   } /* if (IFX_SUCCESS == nRet) */
   /* get timeslot to connect with extension board */
   if (IFX_SUCCESS == nRet)
   {
      /* get TX timeslots, will be used for both TX and RX */
      nTimeslot = PCM_GetTimeslots(TS_RX, fWideBand, pPhone->nSeqConnId);
      if (TIMESLOT_NOT_SET == nTimeslot)
      {
         /* print error trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d failed to get timeslot.\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         nRet = IFX_ERROR;
      }
   } /* if (IFX_SUCCESS == nRet) */
   /* check if PCM is needed for FXO call */
   fGetPCM_ForFXO = Easy3111_PCM_IsNeededForFXO(pPhone, pConn);
   /* for PCM call and FXO outgoing call another PCM channel is needed */
   if ( (IFX_SUCCESS == nRet &&
         (IFX_TRUE == fGetPCM_ForFXO || PCM_CALL == pConn->nType)) )
   {
      /* get free PCM channel */
      nOnMainPCM_ChSecondary = PCM_GetFreeCh(pBoardMain);
      if (NO_FREE_DATA_CH == nOnMainPCM_ChSecondary)
      {
         /* print error trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, No free PCM channels available on board %s.\n"
                "(File: %s, line: %d)\n",
                pBoardMain->pszBoardName, __FILE__, __LINE__));
         /* reset value and set error */
         nOnMainPCM_ChSecondary = TD_NOT_SET;
         nRet = IFX_ERROR;
      } /* get free PCM channel */
      else
      {
         /* reserve PCM channel */
         if (IFX_SUCCESS != PCM_ReserveCh(nOnMainPCM_ChSecondary, pBoardMain,
                                          pPhone->nSeqConnId))
         {
            /* print error trace */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Failed to reserve PCM channel %d on %s.\n"
                   "(File: %s, line: %d)\n",
                   nOnMainPCM_ChSecondary, pBoardMain->pszBoardName,
                   __FILE__, __LINE__));
            /* reset value and set error */
            nOnMainPCM_ChSecondary = TD_NOT_SET;
            nRet = IFX_ERROR;
         }
         else
         {
            nOnMainPCM_ChSecondaryFD = PCM_GetFD_OfCh(nOnMainPCM_ChSecondary,
                                                      pBoardMain);
            if (NO_DEVICE_FD == nOnMainPCM_ChSecondaryFD)
            {
               /* print error trace */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, Failed to get FD of PCM channel %d on %s.\n"
                      "(File: %s, line: %d)\n",
                      nOnMainPCM_ChSecondary, pBoardMain->pszBoardName,
                      __FILE__, __LINE__));
               /* set error */
               nRet = IFX_ERROR;
            }
         }
      } /* get free PCM channel */
      /* check if no error */
      if (IFX_SUCCESS == nRet)
      {
         /* do mapping: PCM to PCM */
         if (IFX_SUCCESS != PCM_MapToPCM(nOnMainPCM_Ch, nOnMainPCM_ChSecondary,
                               IFX_TRUE, pBoardMain, pPhone->nSeqConnId))
         {
            /* print error trace */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, %s failed to map PCM ch %d to PCM ch %d.\n"
                   "(File: %s, line: %d)\n",
                   pBoardMain->pszBoardName, nOnMainPCM_Ch,
                   nOnMainPCM_ChSecondary, __FILE__, __LINE__));
            /* set error */
            nRet = IFX_ERROR;
         }
      } /* if (IFX_SUCCESS == nRet) */
   }  /* if (IFX_SUCCESS == nRet && (PCM_CALL || FXO_CALL)) */
   /* for external VoIP calls data channel is needed */
   if ((IFX_SUCCESS == nRet) &&
       (EXTERN_VOIP_CALL == pConn->nType))
   {
      /* get free data channel number */
      nOnMainDataCh = VOIP_GetFreeDataCh(pBoardMain, pPhone->nSeqConnId);
      if (NO_FREE_DATA_CH == nOnMainDataCh)
      {
         /* print error trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, No free data channels available on board %s.\n"
                "(File: %s, line: %d)\n",
                pBoardMain->pszBoardName, __FILE__, __LINE__));
         nOnMainDataCh = TD_NOT_SET;
         nRet = IFX_ERROR;
      }
      else
      {
         /* reserve data channel */
         if (IFX_SUCCESS != VOIP_ReserveDataCh(nOnMainDataCh, pBoardMain,
                                               pPhone->nSeqConnId))
         {
            /* print error trace */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Failed to reserve data channel %d on %s.\n"
                   "(File: %s, line: %d)\n",
                   nOnMainDataCh, pBoardMain->pszBoardName,
                   __FILE__, __LINE__));
            nOnMainDataCh = TD_NOT_SET;
            nRet = IFX_ERROR;
         }
         else
         {
            nOnMainDataChFD = VOIP_GetFD_OfCh(nOnMainDataCh, pBoardMain);
            if (NO_DEVICE_FD == nOnMainDataChFD)
            {
               /* print error trace */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, Failed to get FD of data channel %d on %s.\n"
                      "(File: %s, line: %d)\n",
                      nOnMainDataCh, pBoardMain->pszBoardName,
                      __FILE__, __LINE__));
               /* set error */
               nRet = IFX_ERROR;
            }
         }
      }
      /* check if no error */
      if (IFX_SUCCESS == nRet)
      {
         /* do mapping: PCM to PCM */
         if (IFX_SUCCESS != PCM_MapToData(nOnMainPCM_Ch, nOnMainDataCh,
                               IFX_TRUE, pBoardMain, pPhone->nSeqConnId))
         {
            /* print error trace */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, %s failed to map PCM ch %d to data ch %d.\n"
                   "(File: %s, line: %d)\n",
                   pBoardMain->pszBoardName, nOnMainPCM_Ch,
                   nOnMainDataCh, __FILE__, __LINE__));
            /* set error */
            nRet = IFX_ERROR;
         }
      } /* if (IFX_SUCCESS == nRet) */
   } /* if (IFX_SUCCESS == nRet && EXTERN_VOIP_CALL) */
   if (IFX_SUCCESS == nRet)
   {
      /* save allocated resources data */
      pPhone->oEasy3111Specific.nOnMainPCM_Ch = nOnMainPCM_Ch;
      pPhone->oEasy3111Specific.nOnMainPCM_Ch_Bak = nOnMainPCM_Ch;
      pPhone->oEasy3111Specific.nOnMainPCM_ChFD = nOnMainPCM_ChFD;
      pPhone->oEasy3111Specific.nTimeslot = nTimeslot;
      if (LOCAL_CALL == pConn->nType)
      {
         pConn->nUsedCh = nOnMainPCM_Ch;
         pConn->nUsedCh_FD = nOnMainPCM_ChFD;
      }
      else if (PCM_CALL == pConn->nType || IFX_TRUE == fGetPCM_ForFXO)
      {
         pConn->nUsedCh = nOnMainPCM_ChSecondary;
         pConn->nUsedCh_FD = nOnMainPCM_ChSecondaryFD;
      }
      else if (EXTERN_VOIP_CALL == pConn->nType)
      {
         pConn->nUsedCh = nOnMainDataCh;
         pConn->nUsedCh_FD = nOnMainDataChFD;
#ifdef TAPI_VERSION4
         if (pBoardMain->fUseSockets)
#endif
         {
            pConn->nUsedSocket = VOIP_GetSocket(nOnMainDataCh, pBoardMain,
                                                pConn->bIPv6_Call);
         }
#ifdef TAPI_VERSION4
         if (phone->pBoard->fUseSockets)
#endif /* TAPI_VERSION4 */
         {
            /* for non EASY 3111 it is done during TAPIDEMO_HandleCallSeq(),
               for TX direction it is done for all phones during
               state transition ST_Dialing_Dialing_Ext() */
            if ((!pCtrl->pProgramArg->oArgFlags.nQos ||
                 pCtrl->pProgramArg->oArgFlags.nQosSocketStart) &&
                CALL_DIRECTION_RX == pPhone->nCallDirection)
            {
               TD_SOCK_PortSet(&pConn->oUsedAddr,
                               VOIP_GetSocketPort(pConn->nUsedSocket, pCtrl));
            }
         }
      }
      /* print list of allocated resources */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Following resources from %s allocated:\n"
             "%s-PCM channel %d\n"
             "%s-Timeslot RX %d and TX %d\n",
             pPhone->nPhoneNumber, pBoardMain->pszBoardName,
             pPhone->pBoard->pIndentPhone, nOnMainPCM_Ch,
             pPhone->pBoard->pIndentPhone, nTimeslot, nTimeslot));
      /* check if secondary PCM channel is allocated */
      if (TD_NOT_SET != nOnMainPCM_ChSecondary)
      {
         /* debug trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("%s-PCM channel %d\n",
                pPhone->pBoard->pIndentPhone, nOnMainPCM_ChSecondary));
      }
      /* check if data channel is allocated */
      if (TD_NOT_SET != nOnMainDataCh)
      {
         /* debug trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("%s-Data channel %d\n",
                pPhone->pBoard->pIndentPhone, nOnMainDataCh));
      }
      return IFX_SUCCESS;
   }
   else
   {
      /* release allocated resources if an error occured */
      if (TD_NOT_SET != nOnMainDataCh)
      {
         VOIP_FreeDataCh(nOnMainDataCh, pBoardMain, pPhone->nSeqConnId);
      }
      if (TD_NOT_SET != nOnMainPCM_ChSecondary)
      {
         PCM_FreeCh(nOnMainPCM_ChSecondary, pBoardMain);
      }
      if (TD_NOT_SET != nTimeslot)
      {
         PCM_FreeTimeslots(nTimeslot, pPhone->nSeqConnId);
      }
      if (TD_NOT_SET != nOnMainPCM_Ch)
      {
         PCM_FreeCh(nOnMainPCM_Ch, pBoardMain);
      }
   }
   return IFX_ERROR;
}

/**
   Release resources needed to establish voice connection to extension board.

   \param pPhone  - pointer to phone structure
   \param pCtrl   - pointer to control structure
   \param pConn   - pointer to useed connection structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t Easy3111_CallResourcesRelease(PHONE_t *pPhone, CTRL_STATUS_t *pCtrl,
                                           CONNECTION_t *pConn)
{
   BOARD_t *pBoardMain;
   IFX_boolean_t fGetPCM_ForFXO = IFX_FALSE;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* debug trace */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: %s release resources.\n",
          pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));
   /* check on which board phone is located */
   if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid board type %s for phone No %d.\n"
             "(File: %s, line: %d)\n",
             pPhone->pBoard->pszBoardName, pPhone->nPhoneNumber,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check number of boards */
   if (pCtrl->nBoardCnt > 1)
   {
      /* this function is implemented for board configuration where
         DuSLIC-xT chip is located only on extension board */
      if (TYPE_DUSLIC_XT != pCtrl->rgoBoards[TD_MAIN_BOARD_NUMBER].nType)
      {
         pBoardMain = TD_GET_MAIN_BOARD(pCtrl);
      } /* if (TYPE_DUSLIC_XT != ...Boards[TD_MAIN_BOARD_NUMBER].nType) */
      else
      {
         /* print error trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Invalid board type %s for phone No %d.\n"
                "(File: %s, line: %d)\n",
                pPhone->pBoard->pszBoardName, pPhone->nPhoneNumber,
                __FILE__, __LINE__));
         return IFX_ERROR;
      } /* if (TYPE_DUSLIC_XT != ...Boards[TD_MAIN_BOARD_NUMBER].nType) */
   } /* if (pCtrl->nBoardCnt > 1) */
   else
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid number of boards %d.\n"
             "(File: %s, line: %d)\n",
             pCtrl->nBoardCnt, __FILE__, __LINE__));
      return IFX_ERROR;
   } /* if (pCtrl->nBoardCnt > 1) */
   /* check if supported call type */
   if (LOCAL_CALL != pConn->nType &&
       EXTERN_VOIP_CALL != pConn->nType &&
       PCM_CALL != pConn->nType &&
       FXO_CALL != pConn->nType &&
       /* for unknown call type only release PCM channel and free timeslot,
         same as for LOCAL_CALL */
       UNKNOWN_CALL_TYPE != pConn->nType)
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Unhandled call type %s(%d).\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber,
             Common_Enum2Name(pConn->nType, TD_rgCallTypeName), pConn->nType,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* if PCM channel is set then resources were allocated */
   if (TD_NOT_SET != pPhone->oEasy3111Specific.nOnMainPCM_Ch)
   {
      /* check if connection was deactivated */
      if (IFX_TRUE == pPhone->oEasy3111Specific.fToMainConnActive)
      {
         /* print error trace */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d: Connection must be deactivated first.\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check if PCM was needed for FXO call */
      fGetPCM_ForFXO = Easy3111_PCM_IsNeededForFXO(pPhone, pConn);
      /* free additional resources according to call type */
      if (PCM_CALL == pConn->nType || IFX_TRUE == fGetPCM_ForFXO)
      {
         PCM_MapToPCM(pPhone->oEasy3111Specific.nOnMainPCM_Ch, pConn->nUsedCh,
                      IFX_FALSE, pBoardMain, pPhone->nSeqConnId);
         PCM_FreeCh(pConn->nUsedCh, pBoardMain);
      }
      else if (EXTERN_VOIP_CALL == pConn->nType)
      {
         PCM_MapToData(pPhone->oEasy3111Specific.nOnMainPCM_Ch, pConn->nUsedCh,
                       IFX_FALSE, pBoardMain, pPhone->nSeqConnId);
         VOIP_FreeDataCh(pConn->nUsedCh, pBoardMain, pPhone->nSeqConnId);
         pConn->nUsedSocket = TD_NOT_SET;
      }
      /* free allocated PCM channel */
      PCM_FreeCh(pPhone->oEasy3111Specific.nOnMainPCM_Ch, pBoardMain);
      pPhone->oEasy3111Specific.nOnMainPCM_Ch = TD_NOT_SET;
      pPhone->oEasy3111Specific.nOnMainPCM_ChFD = TD_NOT_SET;
      /* free timeslots */
      PCM_FreeTimeslots(pPhone->oEasy3111Specific.nTimeslot, pPhone->nSeqConnId);
      pPhone->oEasy3111Specific.nTimeslot = TD_NOT_SET;
      /* reset used channel data */
      pConn->nUsedCh = TD_NOT_SET;
      pConn->nUsedCh_FD = TD_NOT_SET;
      return IFX_SUCCESS;
   }
   else
   {
      /* print error trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Unable to release resorces,"
             " invalid resource number.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
   return IFX_ERROR;
}

/**
   Deactivate connection to main and release resources.

   \param pPhone  - pointer to phone structure
   \param pCtrl   - pointer to control structure
   \param pConn   - pointer to useed connection structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t Easy3111_ClearPhone(PHONE_t *pPhone, CTRL_STATUS_t *pCtrl,
                                 CONNECTION_t *pConn)
{
   IFX_return_t nRet = IFX_SUCCESS;

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* disconect with main */
      if (IFX_TRUE == pPhone->oEasy3111Specific.fToMainConnActive)
      {
         if (IFX_ERROR == Easy3111_CallPCM_Deactivate(pPhone, pCtrl))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: Conn to main deactivation failed.\n"
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
            nRet = IFX_ERROR;
         }
      }
      if (TD_NOT_SET != pPhone->oEasy3111Specific.nOnMainPCM_Ch)
      {
         /* release resources */
         if (IFX_ERROR == Easy3111_CallResourcesRelease(pPhone, pCtrl, pConn))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: Resource release failed.\n"
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
            nRet = IFX_ERROR;
         }
      }
   }
   return nRet;
}
#endif /* EASY3111 */

/**
   Release board structure (reserve memory, inicalize, ...).

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark Must be called before application exit.
*/
IFX_void_t BOARD_Easy3111_RemoveBoard(BOARD_t* pBoard)
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
   Set board structure, do basic initialization, initialize device and PCM IF.

   \param pBoard - pointer to board
   \param pProgramArg - pointer to program arguments

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t BOARD_Easy3111_Init(BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg)
{
   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   if (BOARD_Easy3111_PrepareBoard(pBoard, pProgramArg->sPathToDwnldFiles)
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
#ifdef EASY3111
   /* check if PCM master is set */
   if (!pProgramArg->oArgFlags.nPCM_Master)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, PCM isn't set as master for main board,\n"
             "     calls on %s not available.\n",
             pBoard->pszBoardName));
   }
#endif /* EASY3111 */
   if (BOARD_Easy3111_CfgPCMInterfaceSlave(pBoard) == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("PCM initialization failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
#ifdef EASY3111
/* board uses data channels from main,
   if main support T.38 then phones from extension can use T.38 */
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   {
      BOARD_t* pBoardMain = TD_GET_MAIN_BOARD(pBoard->pCtrl);
      pBoard->nT38_Support = pBoardMain->nT38_Support;
   }
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38)  && defined(HAVE_T38_IN_FW)) */
#endif /* EASY3111 */

   return IFX_SUCCESS;
}

/**
   Register board.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
*/
IFX_return_t BOARD_Easy3111_Register(BOARD_t* pBoard)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pBoard->pszBoardName = BOARD_NAME_EASY3111;
   pBoard->Init = BOARD_Easy3111_Init;
   pBoard->RemoveBoard = BOARD_Easy3111_RemoveBoard;

   return IFX_SUCCESS;
} /* BOARD_Easy3111_Register */

/*
   Detect node required by extension board EASY3111.

   \param  pCtrl  - pointer to status control structure

   \return board ID
 */
IFX_int32_t BOARD_Easy3111_DetectNode(CTRL_STATUS_t* pCtrl)
{
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   IFX_char_t pszDevNameWithPath[MAX_DEV_NAME_LEN_WITH_NUMBER];
   IFX_int32_t nLen;
   IFX_int32_t nDevNum;
   IFX_int32_t fd = -1;

   /* Check if board should be detected */
   if (IFX_TRUE != nDetectDevice)
   {
      /* Don't detect this board anymore */
      return NO_EXT_BOARD;
   }

   /* Max number of board is limited */
   if (nDeviceToDetect > TD_EASY3111_DEVICE_NUMBER)
   {
      nDetectDevice = IFX_FALSE;
      return NO_EXT_BOARD_DETECT;
   }

   /* clean up string */
   memset(pszDevNameWithPath, 0,
          sizeof(IFX_char_t) * MAX_DEV_NAME_LEN_WITH_NUMBER);

   /* set string */
   nLen = snprintf(pszDevNameWithPath, MAX_DEV_NAME_LEN_WITH_NUMBER,
                   "%s%d0", sCTRL_DEV_NAME_DXT, nDeviceToDetect);
   /* check return value */
   if (0 > nLen || MAX_DEV_NAME_LEN_WITH_NUMBER <= nLen)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Unable to create string - it's too long (size: %d).\n"
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
} /* BOARD_Easy3111_DetectNode */



