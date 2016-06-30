/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file common.c
   \date 2005-10-01
   \brief Common stuff like read file, open/close FDs and wrapper to board
          interface.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "td_osmap.h"
#include "td_ifxos_map.h"
#include "common.h"
#include "tapidemo.h"
#include "feature.h"
#include "voip.h"
#include "abstract.h"
#include "qos.h"
#include "state_trans.h"
#include "analog.h"
#include "pcm.h"
#ifdef LINUX
#include <sys/mman.h>
#endif
#ifdef VXWORKS
#include <sys/stat.h>
#include <ctype.h>
#include <pipeDrv.h>
#endif

/* Include board specific commands, structures */

#if EASY3332
#include "board_easy3332.h"
#endif /* EASY3332 */

#if EASY50510
#include "board_easy50510.h"
#endif /* EASY50510 */

#ifdef DANUBE
#include "board_easy50712.h"
#endif /* DANUBE */

#ifdef EASY336
#include "board_easy336.h"
#ifdef WITH_VXT
#include "board_xt16.h"
#endif /* WITH_VXT */
#endif /* EASY336 */

#ifdef XT16
#include "board_xt16.h"
#endif /* XT16 */

#ifdef VINAX
#include "board_easy80800.h"
#endif /* VINAX */

#ifdef AR9
#include "board_easy508xx.h"
#endif /* AR9 */

#ifdef TD_XWAY_XRX300
#include "board_xwayXRX300.h"
#endif /* TD_XWAY_XRX300 */

#ifdef VR9
#include "board_easy80910.h"
#endif /* VR9 */

#ifdef DXT
#if defined (EASY3201) || defined (EASY3201_EVS)
#include "board_easy3201.h"
#endif /* EASY3201 */
#include "board_easy3111.h"
#endif /* DXT */

#ifdef FXO
#include "common_fxo.h"
#endif /* FXO */

#ifdef TD_DECT
#include "td_dect.h"
#endif /* TD_DECT */

/* ============================= */
/* Defines                       */
/* ============================= */

/** Max trace lenght in one line,
    This value is used to check if traces aren't too long. */
enum { MAX_TRACE_LENGTH  = 75 };

/** Max length of trace indention. */
#define TD_MAX_INDENT      20

/** Max length of thread name. */
#define MAX_THREAD_NAME      100

/** Time [ms] to wait for "self-shutdown" of the user thread */
#define THREAD_SELF_SHUTDOWN_TIME 500

/** max number of mappings */
#define TD_MAX_LOCAL_MAPPINGS     32

/** Max word len. */
#define MAX_WORD_LEN      100

/* ============================= */
/* Local structures              */
/* ============================= */

/** mapping types for local phones */
typedef enum TD_LOCAL_MAPPING_TYPE_t_
{
   TD_MAP_DECT_2_DECT,
   TD_MAP_DECT_2_PCM,
   TD_MAP_DECT_2_PHONE,
   TD_MAP_PCM_2_PCM,
   TD_MAP_PCM_2_PHONE,
   TD_MAP_PHONE_2_PHONE,
   TD_MAP_UNKNOWN_2_UNKNOWN
} TD_LOCAL_MAPPING_TYPE_t;

/** mapping data */
typedef struct TD_LOCAL_MAPPING_t_
{
   /** first phone number */
   IFX_int32_t nPhoneA;
   /** first phone channel number */
   IFX_int32_t nChannelA;
   /** second phone number */
   IFX_int32_t nPhoneB;
   /** second phone channel number */
   IFX_int32_t nChannelB;
   /** mapping type */
   TD_LOCAL_MAPPING_TYPE_t nType;
   /** board where mapping is done */
   BOARD_t* pBoard;
} TD_LOCAL_MAPPING_t;

/* ============================= */
/* Local variable definition     */
/* ============================= */

IFX_char_t acTraceIndentTable[TD_MAX_INDENT];

/** names of line mode */
TD_ENUM_2_NAME_t TD_rgLineModeName[] =
{
   {IFX_TAPI_LINE_FEED_ACTIVE, "IFX_TAPI_LINE_FEED_ACTIVE"},
   {IFX_TAPI_LINE_FEED_ACTIVE_REV, "IFX_TAPI_LINE_FEED_ACTIVE_REV"},
   {IFX_TAPI_LINE_FEED_STANDBY, "IFX_TAPI_LINE_FEED_STANDBY"},
   {IFX_TAPI_LINE_FEED_HIGH_IMPEDANCE, "IFX_TAPI_LINE_FEED_HIGH_IMPEDANCE"},
   {IFX_TAPI_LINE_FEED_DISABLED, "IFX_TAPI_LINE_FEED_DISABLED"},
   {IFX_TAPI_LINE_FEED_NORMAL_AUTO, "IFX_TAPI_LINE_FEED_NORMAL_AUTO"},
   {IFX_TAPI_LINE_FEED_REVERSED_AUTO, "IFX_TAPI_LINE_FEED_REVERSED_AUTO"},
   {IFX_TAPI_LINE_FEED_NORMAL_LOW, "IFX_TAPI_LINE_FEED_NORMAL_LOW"},
   {IFX_TAPI_LINE_FEED_REVERSED_LOW, "IFX_TAPI_LINE_FEED_REVERSED_LOW"},
   {IFX_TAPI_LINE_FEED_RING_BURST, "IFX_TAPI_LINE_FEED_RING_BURST"},
   {IFX_TAPI_LINE_FEED_RING_PAUSE, "IFX_TAPI_LINE_FEED_RING_PAUSE"},
   {IFX_TAPI_LINE_FEED_METER, "IFX_TAPI_LINE_FEED_METER"},
   {IFX_TAPI_LINE_FEED_ACTIVE_LOW, "IFX_TAPI_LINE_FEED_ACTIVE_LOW"},
   {IFX_TAPI_LINE_FEED_ACTIVE_BOOSTED, "IFX_TAPI_LINE_FEED_ACTIVE_BOOSTED"},
   {IFX_TAPI_LINE_FEED_ACT_TESTIN, "IFX_TAPI_LINE_FEED_ACT_TESTIN"},
   {IFX_TAPI_LINE_FEED_DISABLED_RESISTIVE_SWITCH,
      "IFX_TAPI_LINE_FEED_DISABLED_RESISTIVE_SWITCH"},
   {IFX_TAPI_LINE_FEED_PARKED_REVERSED, "IFX_TAPI_LINE_FEED_PARKED_REVERSED"},
   {IFX_TAPI_LINE_FEED_ACTIVE_RES_NORMAL,
      "IFX_TAPI_LINE_FEED_ACTIVE_RES_NORMAL"},
   {IFX_TAPI_LINE_FEED_ACTIVE_RES_REVERSED,
      "IFX_TAPI_LINE_FEED_ACTIVE_RES_REVERSED"},
   {IFX_TAPI_LINE_FEED_ACT_TEST, "IFX_TAPI_LINE_FEED_ACT_TEST"},
   {IFX_TAPI_LINE_FEED_NLT, "IFX_TAPI_LINE_FEED_NLT"},
   {IFX_TAPI_LINE_FEED_STANDBY_RES, "IFX_TAPI_LINE_FEED_STANDBY_RES"},
   {IFX_TAPI_LINE_FEED_ACTIVE_HIT, "IFX_TAPI_LINE_FEED_ACTIVE_HIT"},
   {IFX_TAPI_LINE_FEED_ACTIVE_HIR, "IFX_TAPI_LINE_FEED_ACTIVE_HIR"},
#ifdef TD_PPD
   {IFX_TAPI_LINE_FEED_PHONE_DETECT, "IFX_TAPI_LINE_FEED_PHONE_DETECT"},
#endif /* TD_PPD */
   {TD_MAX_ENUM_ID, "IFX_TAPI_LINE_MODE_t"}
};

#ifdef TAPI_VERSION4
const IFX_char_t* LINE_MODES[] =
{
   "IFX_TAPI_LINE_FEED_ACTIVE",
   "IFX_TAPI_LINE_FEED_ACTIVE_REV",
   "IFX_TAPI_LINE_FEED_STANDBY",
   "IFX_TAPI_LINE_FEED_HIGH_IMPEDANCE",
   "IFX_TAPI_LINE_FEED_DISABLED",
   "IFX_TAPI_LINE_FEED_GROUND_START",
   "IFX_TAPI_LINE_FEED_NORMAL_AUTO",
   "IFX_TAPI_LINE_FEED_REVERSED_AUTO",
   "IFX_TAPI_LINE_FEED_NORMAL_LOW",
   "IFX_TAPI_LINE_FEED_REVERSED_LOW",
   "IFX_TAPI_LINE_FEED_RING_BURST",
   "IFX_TAPI_LINE_FEED_RING_PAUSE",
   "IFX_TAPI_LINE_FEED_METER",
   "IFX_TAPI_LINE_FEED_ACTIVE_LOW",
   "IFX_TAPI_LINE_FEED_ACTIVE_BOOSTED",
   "IFX_TAPI_LINE_FEED_ACT_TESTIN",
   "IFX_TAPI_LINE_FEED_DISABLED_RESISTIVE_SWITCH",
   "IFX_TAPI_LINE_FEED_PARKED_REVERSED",
   "IFX_TAPI_LINE_FEED_ACTIVE_RES_NORMAL",
   "IFX_TAPI_LINE_FEED_ACTIVE_RES_REVERSED",
   "IFX_TAPI_LINE_FEED_ACT_TEST",
   "IFX_TAPI_LINE_FEED_NLT"
};
#else /* TAPI_VERSION4 */
const IFX_char_t* LINE_MODES[] =
{
   "IFX_TAPI_LINE_FEED_ACTIVE",
   "IFX_TAPI_LINE_FEED_ACTIVE_REV",
   "IFX_TAPI_LINE_FEED_STANDBY",
   "IFX_TAPI_LINE_FEED_HIGH_IMPEDANCE",
   "IFX_TAPI_LINE_FEED_DISABLED",
   "IFX_TAPI_LINE_FEED_GROUND_START",
   "IFX_TAPI_LINE_FEED_NORMAL_AUTO",
   "IFX_TAPI_LINE_FEED_REVERSED_AUTO",
   "IFX_TAPI_LINE_FEED_NORMAL_LOW",
   "IFX_TAPI_LINE_FEED_REVERSED_LOW",
   "IFX_TAPI_LINE_FEED_RING_BURST",
   "IFX_TAPI_LINE_FEED_RING_PAUSE",
   "IFX_TAPI_LINE_FEED_METER",
   "IFX_TAPI_LINE_FEED_ACTIVE_LOW",
   "IFX_TAPI_LINE_FEED_ACTIVE_BOOSTED",
   "IFX_TAPI_LINE_FEED_ACT_TESTIN",
   "IFX_TAPI_LINE_FEED_DISABLED_RESISTIVE_SWITCH",
   "IFX_TAPI_LINE_FEED_PARKED_REVERSED",
   "IFX_TAPI_LINE_FEED_ACTIVE_RES_NORMAL",
   "IFX_TAPI_LINE_FEED_ACTIVE_RES_REVERSED",
   "IFX_TAPI_LINE_FEED_ACT_TEST"
};
#endif /* TAPI_VERSION4 */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/** if set to IFX_FALSE then BBD file is not downloaded */
IFX_boolean_t g_bBBD_Dwld = IFX_TRUE;

#ifdef EVENT_LOGGER_DEBUG
/** global buffer for trace messages for event logger */
IFX_char_t g_buf_el[TD_COM_BUF_SIZE];
#endif /* EVENT_LOGGER_DEBUG */
/** IFX_TRUE - send traces to Event Logger. */
IFX_boolean_t g_bUseEL = IFX_FALSE;

#ifdef USE_FILESYSTEM
#ifndef VXWORKS
/** enums for setting downloads files (fw, bbd...) */
typedef enum _FW_PARAM_t
{
   TD_FW_FILE_MAIN,
   TD_DRAM_FILE_MAIN,
   TD_BBD_FILE_MAIN,
   TD_FW_FILE_VIN,
   TD_DRAM_FILE_VIN,
   TD_BBD_FILE_VIN,
   TD_FW_FILE_DXT,
   TD_DRAM_FILE_DXT,
   TD_BBD_FILE_DXT,
   TD_FW_FILE_SVIP,
   TD_DRAM_FILE_SVIP,
   TD_BBD_FILE_SVIP,
   TD_FW_FILE_VXT,
   TD_DRAM_FILE_VXT,
   TD_BBD_FILE_VXT,
   TD_FW_FILE_VMMC,
   TD_DRAM_FILE_VMMC,
   TD_BBD_FILE_VMMC,
   TD_FW_NONE
}FW_PARAM_t;

/** .tapidemorc variables for setting downloads files */
TD_ENUM_2_NAME_t TD_rgEnumFwName[] =
{
   {TD_FW_FILE_MAIN, "FW_FILE_MAIN"},
   {TD_DRAM_FILE_MAIN, "DRAM_FILE_MAIN"},
   {TD_BBD_FILE_MAIN, "BBD_FILE_MAIN"},
   {TD_FW_FILE_VIN, "FW_FILE_VIN"},
   {TD_DRAM_FILE_VIN, "DRAM_FILE_VIN"},
   {TD_BBD_FILE_VIN, "BBD_FILE_VIN"},
   {TD_FW_FILE_DXT, "FW_FILE_DXT"},
   {TD_DRAM_FILE_DXT, "DRAM_FILE_DXT"},
   {TD_BBD_FILE_DXT, "BBD_FILE_DXT"},
   {TD_FW_FILE_SVIP, "FW_FILE_SVIP"},
   {TD_DRAM_FILE_SVIP, "DRAM_FILE_SVIP"},
   {TD_BBD_FILE_SVIP, "BBD_FILE_SVIP"},
   {TD_FW_FILE_VXT, "FW_FILE_VXT"},
   {TD_DRAM_FILE_VXT, "DRAM_FILE_VXT"},
   {TD_BBD_FILE_VXT, "BBD_FILE_VXT"},
   {TD_FW_FILE_VMMC, "FW_FILE_VMMC"},
   {TD_DRAM_FILE_VMMC, "DRAM_FILE_VMMC"},
   {TD_BBD_FILE_VMMC, "BBD_FILE_VMMC"},
   {TD_FW_NONE, "Unknown"},
   {TD_MAX_ENUM_ID, "TD_rgEnumFwName"}
};
#endif /* VXWORKS */
#endif /* USE_FILESYSTEM */

const char * TAPIDEMO_PCM_FREQ_STR[] =
{
   "512", /* IFX_TAPI_PCM_IF_DCLFREQ_512 */
   "1024", /* IFX_TAPI_PCM_IF_DCLFREQ_1024 */
   "1536", /* IFX_TAPI_PCM_IF_DCLFREQ_1536 */
   "2048", /* IFX_TAPI_PCM_IF_DCLFREQ_2048 */
   "4096", /* IFX_TAPI_PCM_IF_DCLFREQ_4096 */
   "8192", /* IFX_TAPI_PCM_IF_DCLFREQ_8192 */
   "16384" /* IFX_TAPI_PCM_IF_DCLFREQ_16384 */
};

/** CPT names */
TD_ENUM_2_NAME_t TD_rgEnumToneIdx[] =
{
   {DIALTONE_IDX, "Dialtone"},
   {RINGBACK_TONE_IDX, "Ringback tone"},
   {BUSY_TONE_IDX, "Busy tone"},
   {TD_MAX_ENUM_ID, "TONE_IDX_t"}
};

/* table with error strings for Common_Enum2Name() function. */
IFX_char_t* TD_rgEnum2NameErrorCode[] =
{
   "enum value not found",
   "error, NULL pointer"
};

/* ============================= */
/* Local function declaration    */
/* ============================= */

IFX_return_t Common_JBStatistic(IFX_int32_t nDataCh,
                                IFX_int32_t nDev,
                                IFX_boolean_t fReset,
                                IFX_boolean_t fShow,
                                BOARD_t* pBoard);
#ifdef USE_FILESYSTEM
IFX_return_t Common_ParseOptionalFile(IFX_char_t* pFieldName,
                                      IFX_char_t* pFieldValue, TD_OS_File_t* fp,
                                      IFX_uint32_t nSeqConnId);
#endif /* USE_FILESYSTEM */

#ifdef TD_IPV6_SUPPORT
IFX_return_t Common_PrintAddrInfo(struct addrinfo *ai, IFX_uint32_t nSeqConnId);
#endif /* TD_IPV6_SUPPORT */
/* ============================= */
/* Local function definition     */
/* ============================= */

#ifdef USE_FILESYSTEM
/**
   Create full filename (path + file's name)

   \param \param pszFilename  - file's name
   \param pszPath - path to file
   \param pszFullFilename - path with filename

   \return  status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t Common_CreateFullFilename(IFX_char_t* pszFilename,
                                       IFX_char_t* pszPath,
                                       IFX_char_t* pszFullFilename)
{

   TD_PTR_CHECK(pszFilename, IFX_ERROR);
   TD_PTR_CHECK(pszPath, IFX_ERROR);
   TD_PARAMETER_CHECK((0 == strlen(pszFilename)), strlen(pszFilename), IFX_ERROR);
   TD_PARAMETER_CHECK((0 == strlen(pszPath)), strlen(pszPath), IFX_ERROR);

   if ((strlen(pszPath) + strlen(pszFilename)) < MAX_PATH_LEN)
   {
      strcpy(pszFullFilename, pszPath);
      strcat(pszFullFilename, pszFilename);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Path [%s] + Filename [%s] too long (max. %d). "
             "(File: %s, line: %d)\n",
             pszPath, pszFilename, MAX_PATH_LEN, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* Common_CreateFullFilename */

#ifndef VXWORKS
/**
   Sets FW, BBD and/or DRAM filenames if .tapidemorc file exists and contains
  proper parameter.

   \param pBoard              - board handle
   \param pCpuDevice          - pointer to device

   \return IFX_ERROR if there is a problem with reading or
           accessing .tapidemorc file, otherwise IFX_SUCCESS
   \remarks
*/
IFX_return_t Common_SetFwFilesFromTapidemorc(BOARD_t* pBoard,
                                             TAPIDEMO_DEVICE_CPU_t* pCpuDevice)
{
   /* Enum string with FW name */
   FW_PARAM_t nFwTypeEnum = TD_FW_NONE;
   FW_PARAM_t nDramTypeEnum = TD_FW_NONE;
   FW_PARAM_t nBbdTypeEnum = TD_FW_NONE;
   TD_OS_File_t* fp;
   /* Path to user home direction */
   IFX_char_t* pUserHomeDir;
   /* Path + option filename */
   IFX_char_t psOptionFilename[MAX_PATH_LEN];

   struct passwd* pwd;
   uid_t uid;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   /* Build path to option file .tapidemorc */
   /* Optional file must be kept in user home directory */
   /* Get the real user ID of the current process */
   uid = getuid();
   /* Get password file entry */
   pwd = getpwuid(uid);
   if (pwd != IFX_NULL)
   {
      /* Get user home directory */
      pUserHomeDir = pwd->pw_dir;
   }
   else
   {
      /* For Danube 2.4 function getpwuid() return NULL (can't find passwd file).
      So, we can use another way to get home directory.
      We don't use this solution for all boards, because it doesn't work on SVIP board */
      /* Get user home directory */
      pUserHomeDir =  getenv("HOME");
   }

   if (IFX_NULL == pUserHomeDir)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Err, Can't get user home directory: %s\n(File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* We add 2 to second argument because, we must remember about '/' and EOL */
   if (MAX_PATH_LEN < (strlen(pUserHomeDir) + strlen(DOWNLOAD_FILE_NAME) + 2))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Err, User home directory's path is too long.\n"
             " (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   sprintf(psOptionFilename, "%s/%s", pUserHomeDir, DOWNLOAD_FILE_NAME);

   /* Open option file */
   fp = TD_OS_FOpen(psOptionFilename, "r");
   if (IFX_NULL == fp)
   {
      /* Failed to open file */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Optional file %s is not available.\n", psOptionFilename));
      return IFX_ERROR;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Optional file %s is available.\n", psOptionFilename));
   }

   /* Get FW, DRAM and BBD types according to board type */
   switch (pBoard->nType)
   {
#if EASY3332 || EASY50510
      case TYPE_VINETIC:
         nFwTypeEnum = TD_FW_FILE_VIN;
         nDramTypeEnum = TD_DRAM_FILE_VIN;
         nBbdTypeEnum = TD_BBD_FILE_VIN;
      break;
#endif /* EASY3332 || EASY50510  */
#if DXT
      case TYPE_DUSLIC_XT:
         nFwTypeEnum = TD_FW_FILE_DXT;
         nDramTypeEnum = TD_DRAM_FILE_DXT;
         nBbdTypeEnum = TD_BBD_FILE_DXT;
      break;
#endif /* DXT */
#if EASY336
      case TYPE_SVIP:
         nFwTypeEnum = TD_FW_FILE_SVIP;
         nDramTypeEnum = TD_DRAM_FILE_SVIP;
         nBbdTypeEnum = TD_BBD_FILE_SVIP;
      break;
#endif /* EASY336 */
#if WITH_VXT || XT16
      case TYPE_XT16:
         nFwTypeEnum = TD_FW_FILE_VXT;
         nDramTypeEnum = TD_DRAM_FILE_VXT;
         nBbdTypeEnum = TD_BBD_FILE_VXT;
      break;
#endif /* XT16 */
#if DANUBE || AR9 || VINAX || VR9 || TD_XWAY_XRX300
      case TYPE_DANUBE:
      case TYPE_AR9:
      case TYPE_VINAX:
      case TYPE_VR9:
      case TYPE_XWAY_XRX300:
         nFwTypeEnum = TD_FW_FILE_VMMC;
         nDramTypeEnum = TD_DRAM_FILE_VMMC;
         nBbdTypeEnum = TD_BBD_FILE_VMMC;
      break;
#endif /* DANUBE || AR9 || VINAX || VR9 || TD_XWAY_XRX300 */
      default:
         /* Unknown board type - do nothing */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Unknow board type %d - %s\n(File: %s, line: %d)\n",
                pBoard->nType, pBoard->pszBoardName, __FILE__, __LINE__));
         return IFX_ERROR;
      break;
   } /* switch (pBoard->nType) */

   /** FW */
   /* If tapidemo hasn't started with --file_fw argument,
      set pszPRAM_FW_File[0] according to its value */
   if (strncmp(pBoard->pCtrl->pProgramArg->sFwFileName,
               "", sizeof(pBoard->pCtrl->pProgramArg->sFwFileName)) == 0)
   {
      /* If in .tapidemorc file parameter named pFwTypeString is set, then fill
         pszPRAM_FW_File[0] according to value of this parameter. */
      pBoard->pFWFileName = TD_OS_MemCalloc(MAX_PATH_LEN, sizeof(IFX_char_t));
      /* Rewind fp to the begining */
      rewind(fp);
      if (IFX_SUCCESS ==
          Common_ParseOptionalFile(Common_Enum2Name(nFwTypeEnum, TD_rgEnumFwName),
                                   pBoard->pFWFileName, fp, TD_CONN_ID_INIT))
      {
         pCpuDevice->pszPRAM_FW_File[0] = pBoard->pFWFileName;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
               ("Value from file %s changed FW filename to: %s \n",
                DOWNLOAD_FILE_NAME, pCpuDevice->pszPRAM_FW_File[0]));
      }
      else
      {
         rewind(fp);
         /* Otherwise check if in .tapidemorc file "FW_FILE_MAIN" parameter is set. */
         if (IFX_SUCCESS ==
             Common_ParseOptionalFile(Common_Enum2Name(TD_FW_FILE_MAIN, TD_rgEnumFwName),
                                      pBoard->pFWFileName, fp, TD_CONN_ID_INIT))
         {
            pCpuDevice->pszPRAM_FW_File[0] = pBoard->pFWFileName;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                  ("Value from file %s changed FW filename to: %s \n",
                   DOWNLOAD_FILE_NAME, pCpuDevice->pszPRAM_FW_File[0]));
         }
         else
         {
            TD_OS_MemFree(pBoard->pFWFileName);
            pBoard->pFWFileName = IFX_NULL;
         }
         /* Otherwise leave pszPRAM_FW_File[0] parameter untouched. */
      }
   }

   /** DRAM  */
   /* If tapidemo hasn't started with --file_dram argument, set pszDRAM_FW_File
      according to its value */
   if (strncmp(pBoard->pCtrl->pProgramArg->sDramFileName,
               "", sizeof(pBoard->pCtrl->pProgramArg->sDramFileName)) == 0)
   {
      /* If in .tapidemorc file parameter named pDramTypeString is set, then fill
         pszDRAM_FW_File[0] according to value of this parameter. */

      pBoard->pDRAMFileName = TD_OS_MemCalloc(MAX_PATH_LEN, sizeof(IFX_char_t));
      rewind(fp);
      if (IFX_SUCCESS ==
          Common_ParseOptionalFile(Common_Enum2Name(nDramTypeEnum, TD_rgEnumFwName),
                                   pBoard->pDRAMFileName, fp, TD_CONN_ID_INIT))
      {
         pCpuDevice->pszDRAM_FW_File = pBoard->pDRAMFileName;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
               ("Value from file %s changed DRAM filename to: %s \n",
                DOWNLOAD_FILE_NAME, pCpuDevice->pszDRAM_FW_File));
      }
      /* Otherwise check if in .tapidemorc file "DRAM_FILE_MAIN" parameter is set. */
      else
      {
         rewind(fp);
         if (IFX_SUCCESS ==
             Common_ParseOptionalFile(Common_Enum2Name(TD_DRAM_FILE_MAIN, TD_rgEnumFwName),
                                      pBoard->pDRAMFileName, fp, TD_CONN_ID_INIT))
         {
            pCpuDevice->pszDRAM_FW_File = pBoard->pDRAMFileName;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                  ("Value from file %s changed DRAM filename to: %s \n",
                   DOWNLOAD_FILE_NAME, pCpuDevice->pszDRAM_FW_File));
         }
         else
         {
            TD_OS_MemFree(pBoard->pDRAMFileName);
            pBoard->pDRAMFileName = IFX_NULL;
         }
         /* Otherwise leave pszDRAM_FW_File parameter untouched. */
      }
   }

   /** BBD  */
   /* If tapidemo hasn't started with --file_bbd argument, set pszBBD_CRAM_File[0]
      according to its value */
   if (strncmp(pBoard->pCtrl->pProgramArg->sBbdFileName,
               "", sizeof(pBoard->pCtrl->pProgramArg->sBbdFileName)) == 0)
   {
      /* If in .tapidemorc file parameter named pBbdTypeString is set, then fill
         pszBBD_CRAM_File[0] according to value of this parameter. */
      pBoard->pBBDFileName = TD_OS_MemCalloc(MAX_PATH_LEN, sizeof(IFX_char_t));
      rewind(fp);
      if (IFX_SUCCESS ==
          Common_ParseOptionalFile(Common_Enum2Name(nBbdTypeEnum, TD_rgEnumFwName),
                                   pBoard->pBBDFileName, fp, TD_CONN_ID_INIT))
      {
         pCpuDevice->pszBBD_CRAM_File[0] = pBoard->pBBDFileName;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
               ("Value from file %s changed BBD filename to: %s \n",
                DOWNLOAD_FILE_NAME, pCpuDevice->pszBBD_CRAM_File[0]));
      }
      /* Otherwise check if in .tapidemorc file "BBD_FILE_MAIN" parameter is set. */
      else
      {
         rewind(fp);
         if (IFX_SUCCESS ==
             Common_ParseOptionalFile(Common_Enum2Name(TD_BBD_FILE_MAIN, TD_rgEnumFwName),
                                      pBoard->pBBDFileName, fp, TD_CONN_ID_INIT))
         {
            pCpuDevice->pszBBD_CRAM_File[0] = pBoard->pBBDFileName;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                  ("Value from file %s changed BBD filename to: %s \n",
                   DOWNLOAD_FILE_NAME, pCpuDevice->pszBBD_CRAM_File[0]));
         }
         else
         {
            TD_OS_MemFree(pBoard->pBBDFileName);
            pBoard->pBBDFileName = IFX_NULL;
         }
         /* Otherwise leave pszBBD_CRAM_File[0] parameter untouched. */
      }
   }
   /* File descriptor is no longer needed */
   TD_OS_FClose(fp);

   return IFX_SUCCESS;
} /* Common_SetFwFilesFromTapidemorc */
#endif /* VXWORKS */

/**
   Sets FW, BBD and/or DRAM filenames if tapidemo started with --file_fw,
  --file_dram and/or --file_bbd arguments.

   \param pBoard              - board handle
   \param pCpuDevice          - pointer to device

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remarks
*/
IFX_return_t Common_SetFwFilenames(BOARD_t* pBoard,
                                   TAPIDEMO_DEVICE_CPU_t* pCpuDevice)
{
   BOARD_t *pMainBoard;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   /* get main board */
   pMainBoard = TD_GET_MAIN_BOARD(pBoard->pCtrl);

   /* if main board is set */
   if (IFX_NULL != pMainBoard)
   {
      /* Check if current board has the same type as main board, if current
         board is main board it also works*/
      if (pMainBoard->nType == pBoard->nType)
      {
         /** FW  */
         /* If tapidemo has started with --file_fw argument,
            set pszPRAM_FW_File[0] according to its value */
         if (strncmp(pBoard->pCtrl->pProgramArg->sFwFileName,
                     "", sizeof(pBoard->pCtrl->pProgramArg->sFwFileName)) != 0)
         {
            pCpuDevice->pszPRAM_FW_File[0] =
               pBoard->pCtrl->pProgramArg->sFwFileName;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                  ("FW filename has been set to: %s\n",
                   pCpuDevice->pszPRAM_FW_File[0]));
         }

         /** DRAM  */
         /* If tapidemo has started with --file_dram argument, set
            pszDRAM_FW_File according to its value */
         if (strncmp(pBoard->pCtrl->pProgramArg->sDramFileName,
                     "", sizeof(pBoard->pCtrl->pProgramArg->sDramFileName)) != 0)
         {
            pCpuDevice->pszDRAM_FW_File =
               pBoard->pCtrl->pProgramArg->sDramFileName;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                  ("DRAM filename has been set to: %s\n",
                   pCpuDevice->pszDRAM_FW_File));
         }

         /** BBD  */
         /* If tapidemo has started with --file_bbd argument, set
            pszBBD_CRAM_File[0] according to its value */
         if (strncmp(pBoard->pCtrl->pProgramArg->sBbdFileName,
                     "", sizeof(pBoard->pCtrl->pProgramArg->sBbdFileName)) != 0)
         {
            pCpuDevice->pszBBD_CRAM_File[0] =
               pBoard->pCtrl->pProgramArg->sBbdFileName;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                  ("BBD filename has been set to: %s\n",
                   pCpuDevice->pszBBD_CRAM_File[0]));
         }
      }
   }

#ifndef VXWORKS
   /* Sets FW, BBD and/or DRAM filenames if .tapidemorc file exists and contains
      proper parameter.*/
   Common_SetFwFilesFromTapidemorc(pBoard, pCpuDevice);
#endif /* VXWORKS */

   return IFX_SUCCESS;
} /* Common_SetFwFilenames */


/**
   Check if file / symbolic link exists.

   \param pszFullFilename  - binary file name with path

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
IFX_return_t Common_CheckFileExists(IFX_char_t* pszFullFilename)
{
   TD_OS_stat_t st;
   IFX_int32_t ret;

   TD_PTR_CHECK(pszFullFilename, IFX_ERROR);

   ret = TD_OS_Stat(pszFullFilename, &st);
   if(0 != ret)
   {
      /* Path is incorrect */
      return IFX_ERROR;
   }

   if(S_ISREG(st.st_mode))
   {
      /* Path focuses on regular file */
      return IFX_SUCCESS;
   }
   else if(S_ISLNK(st.st_mode))
   {
      /* Path focuses on symbolic link */
      return IFX_SUCCESS;
   }

   return IFX_ERROR;
} /* Common_CheckFileExists */

/**
   Find PRAM_FW

   \param pCpuDevice - pointer to CPU device
   \param pszPath - path to file
   \param pszFullFilename - path with filename

   \return  Return IFX_TRUE is PRAM_FW file is found or IFX_FALSE if not
*/

IFX_boolean_t Common_FindPRAM_FW(TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                 IFX_char_t* pszPath,
                                 IFX_char_t* pszFullFilename)
{
   IFX_int32_t i;

   TD_PTR_CHECK(pCpuDevice, IFX_FALSE);
   TD_PTR_CHECK(pszPath, IFX_FALSE);
   TD_PARAMETER_CHECK((0 == strlen(pszPath)), strlen(pszPath), IFX_FALSE);

   /* do check for all available file names */
   for (i=0; i<pCpuDevice->nPRAM_FW_FileNum; i++)
   {
      if ((IFX_NULL != pCpuDevice->pszPRAM_FW_File[i]) &&
          (0 != strlen(pCpuDevice->pszPRAM_FW_File[i])))
      {
         /* Find PRAM_FW file with default new name */
         if(IFX_SUCCESS == Common_CreateFullFilename(pCpuDevice->pszPRAM_FW_File[i],
                                                     pszPath, pszFullFilename))
         {
            if(IFX_SUCCESS == Common_CheckFileExists(pszFullFilename))
            {
               /* Find PRAM_FW file with default name */
               return IFX_TRUE;
            }
         }
      }
   } /* do check for all available file names */

   return IFX_FALSE;
} /* Common_FindPRAM_FW */

/**
   Find BBD_CRAM

   \param pCpuDevice - pointer to CPU device
   \param pszPath - path to file
   \param pszFullFilename - path with filename

   \return  Return IFX_TRUE is BBD_CRAM file is found or IFX_FALSE if not
*/
IFX_boolean_t Common_FindBBD_CRAM(TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                  IFX_char_t* pszPath,
                                  IFX_char_t* pszFullFilename)
{
   TD_PTR_CHECK(pCpuDevice, IFX_FALSE);
   TD_PTR_CHECK(pszPath, IFX_FALSE);
   TD_PARAMETER_CHECK((0 == strlen(pszPath)), strlen(pszPath), IFX_FALSE);

   if ((IFX_NULL != pCpuDevice->pszBBD_CRAM_File[0]) &&
       (0 != strlen(pCpuDevice->pszBBD_CRAM_File[0])))
   {
      /* Find BBD_CRAM file with default new name */
      if(IFX_SUCCESS == Common_CreateFullFilename(pCpuDevice->pszBBD_CRAM_File[0],
                                                  pszPath, pszFullFilename))
      {
         if(IFX_SUCCESS == Common_CheckFileExists(pszFullFilename))
         {
            /* Find BBD_CRAM file with default name */
            return IFX_TRUE;
         }
      }
   }

   if ((IFX_NULL != pCpuDevice->pszBBD_CRAM_File[1]) &&
       (0 != strlen(pCpuDevice->pszBBD_CRAM_File[1])))
   {
      /* Find BBD_CRAM file with default old name */
      if(IFX_SUCCESS == Common_CreateFullFilename(pCpuDevice->pszBBD_CRAM_File[1],
                                                  pszPath, pszFullFilename))
      {
         if(IFX_SUCCESS == Common_CheckFileExists(pszFullFilename))
         {
            /* Find BBD_CRAM file with alternative name */
            return IFX_TRUE;
         }
      }
   }

#ifdef VR9
   /* Temporary workaround for VR9 board:
      in the past ar9_bbd_fxs.bin was used for VR9 board.
      The workaround is required to keep compatibility */
   if (IFX_SUCCESS == Common_CreateFullFilename("ar9_bbd.bin",
                                                pszPath, pszFullFilename))
   {
      if (IFX_SUCCESS == Common_CheckFileExists(pszFullFilename))
      {
         return IFX_TRUE;
      }
   }
   /* Temporary workaround for VR9 board:
      in the past ar9_bbd_fxs.bin was used for VR9 board.
      The workaround is required to keep compatibility */
   if (IFX_SUCCESS == Common_CreateFullFilename("ar9_bbd_fxs.bin",
                                                pszPath, pszFullFilename))
   {
      if (IFX_SUCCESS == Common_CheckFileExists(pszFullFilename))
      {
         return IFX_TRUE;
      }
   }
#endif /* VR9 */

   return IFX_FALSE;
} /* Common_pszBBD_CRAM_File */

#ifndef VXWORKS
/**
   Check if download path has correct syntax.

   \param sPathToDwnldFiles   - pointer to path string

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
   \remarks Download path must be ended with '/'
*/
IFX_return_t Common_CheckDownloadPathSyntax(IFX_char_t* sPathToDwnldFiles)
{
   IFX_uint32_t nLen = strlen(sPathToDwnldFiles);

   if (nLen <= 0)
   {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
              ("Err, Invalid string %s\n(File: %s, line: %d)\n",
               sPathToDwnldFiles, __FILE__, __LINE__));
         return IFX_ERROR;
   }
   /* Check if path ends with '/' */
   if (sPathToDwnldFiles[nLen - 1] != '/')
   {
      /* Check if we can add '/' */
      if( (nLen + 1) > MAX_PATH_LEN )
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
              ("Download path is too long %s\n(File: %s, line: %d)\n",
               sPathToDwnldFiles, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      else
      {
         /* Add '/' */
         strcat(sPathToDwnldFiles, "/");
      }
   }
   return IFX_SUCCESS;
} /* Common_CheckDownloadPathSyntax */

/**
   Check download path.

   \param sPathToDwnldFiles   - pointer to path string
   \param pCpuDevice          - pointer to device
   \param bPrintTrace         - indicate if TRACE will be printed
   \param nSeqConnId          - Seq Conn ID

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
   \remarks Download path must indicate on directory with FW and BBD files.
*/
IFX_return_t Common_CheckDownloadPath(IFX_char_t* psPath,
                                      TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                      IFX_boolean_t bPrintTrace,
                                      IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_char_t psFile[MAX_PATH_LEN];
   /* Use to check if new download path is directory */
   TD_OS_stat_t st;

   TD_PTR_CHECK(psPath, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   /* Check if psPath is directory */
   if(-1 == TD_OS_Stat(psPath, &st))
   {
      if(errno == ENOENT)
      {
         /* Can't find directory/file */
         if(bPrintTrace)
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                  ("Can not find download path %s.\n", psPath));
         return IFX_ERROR;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
              ("Err, while checking download path: %s\n"
               "(File: %s, line: %d)\n",
               strerror(errno), __FILE__, __LINE__));
      }
      return IFX_ERROR;
   }

   if(!S_ISDIR(st.st_mode))
   {
      if(bPrintTrace)
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
              ("Download path %s doesn't indicate directory.\n", psPath));
      return IFX_ERROR;
   }

   /* do not check for file if bbd download is disabled */
   if (IFX_FALSE != g_bBBD_Dwld)
   {
      /* Check if download directory contains required bbd/BIN files */
      if (IFX_TRUE != Common_FindBBD_CRAM(pCpuDevice, psPath, psFile))
      {
         ret = IFX_ERROR;
      }
   }

   if ((IFX_SUCCESS == ret) &&
       (IFX_NULL != pCpuDevice->pszDRAM_FW_File) &&
       (0 != strlen(pCpuDevice->pszDRAM_FW_File)))
   {
      ret = Common_CreateFullFilename(pCpuDevice->pszDRAM_FW_File,
                                      psPath, psFile);
      if (IFX_SUCCESS == ret)
      {
         ret = Common_CheckFileExists(psFile);
      }
   }
#ifndef TAPI_VERSION4
   if (IFX_SUCCESS == ret)
   {
      if (IFX_TRUE != Common_FindPRAM_FW(pCpuDevice, psPath, psFile))
      {
         /* for DxT 1.4 FW file is not needed,
            done like this for backward compatibility */
         if (IFX_TRUE != pCpuDevice->bPRAM_FW_FileOptional)
         {
            ret = IFX_ERROR;
         }
      }
   }
#endif

   if (IFX_ERROR == ret)
   {
      if(bPrintTrace)
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("Download path %s does not contain the required files.\n",
                psPath));
   }

   return ret;
} /* Common_CheckDownloadPath */

/**
   Read value from file and verify it.

   \param pFieldName    - name of field to search
   \param pFieldValue   - value found for field name
   \param fp            - file descriptor
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
   \remarks
*/
IFX_return_t Common_ParseOptionalFile(IFX_char_t* pFieldName,
                                      IFX_char_t* pFieldValue, TD_OS_File_t* fp,
                                      IFX_uint32_t nSeqConnId)
{
   IFX_char_t pLine[MAX_LINE_LEN];
   IFX_char_t *pFirstChar;
   IFX_char_t *pLastChar;
   IFX_int32_t nCharDiff;

   TD_PTR_CHECK(pFieldName, IFX_ERROR);
   TD_PTR_CHECK(pFieldValue, IFX_ERROR);
   TD_PTR_CHECK(fp, IFX_ERROR);

   TD_PARAMETER_CHECK((0 >= strlen(pFieldName)), strlen(pFieldName), IFX_ERROR);

   /* Check every line from file */
   while(1)
   {
      memset(pLine, 0, MAX_LINE_LEN);

      /* Get line */
      TD_OS_FGets(pLine, sizeof(pLine), fp);

      /* Detected end of file, so stop checking */
      if(TD_OS_FEof(fp))
      {
         break;
      }

      /* Detected error, so stop checking */
      if(ferror(fp))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
              ("Err, while reading line: %s "
               "(File: %s, line: %d)\n",
               strerror(errno), __FILE__, __LINE__));
         break;
      }

      /* Beginning of line must start with field name */
      if (strncmp(pLine, pFieldName, strlen(pFieldName)) == 0)
      {
         /* Value field must be in quotation marks */
         /* Return a pointer to the first occurence of " */
         pFirstChar = strchr(pLine, '\"');
         /* Return a pointer to the last occurence of " */
         pLastChar = strrchr(pLine, '\"');

         if (pFirstChar == IFX_NULL || pLastChar == IFX_NULL)
         {
            /* Syntax is incorrect, continue searching correct value.*/
            continue;
         }
         nCharDiff = (pLastChar - pFirstChar);
         /* If the first occurance of " is also the last, it means it's wrong */
         if (nCharDiff == 0)
         {
             /* Syntax is incorrect, continue searching correct value.*/
            continue;
         }
         /* check if used  */
         if (nCharDiff >= MAX_PATH_LEN)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, Number of chars to copy is too big %d.\n "
                   "(File: %s, line: %d)\n",
                   (pLastChar - pFirstChar), __FILE__, __LINE__));
            continue;
         }

         memset(pFieldValue, 0, MAX_PATH_LEN);
         /* check source buffer length, we add 1 to first argument and 1 to
            second argument, because we don't want quotation marks at the
            beginining and end of a string */
         if (strlen (pFirstChar + 1) >  (nCharDiff + 1))
         {
            /* Syntax is incorrect, continue searching correct value. */
            continue;
         }
         /* We add 1 to second argument because we don't want quotation marks
            at the beginining and end of a string */
         strncpy(pFieldValue, pFirstChar + 1, nCharDiff );
         pFieldValue[nCharDiff - 1] = '\0';

         /* Verify if syntax is corrrect */
         /* In first part, we need to add 3 because of '=' and two '"' */
         /* In second part, we need to subtract 1,
            because at the end of line there is empy char */
         if( (strlen(pFieldName) + strlen(pFieldValue) + 3) != (strlen(pLine)-1) )
         {
            /* Syntax is incorrect, continue searching correct value. */
            continue;
         }

         return IFX_SUCCESS;
      }
   } /* while (1) */

   return IFX_ERROR;
} /* Common_ParseRcFile */

/**
   Check if path specified in file is valid and change path to downloads.

   \param sPathToDwnldFiles   - pointer to path string
   \param pBoard              - board handle
   \param pCpuDevice          - pointer to device
   \param nSeqConnId          - Seq Conn ID

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
   \remarks
*/
IFX_return_t Common_GetOptionalFile(IFX_char_t* sPathToDwnldFiles,
                                    BOARD_t* pBoard,
                                    TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                    IFX_uint32_t nSeqConnId)
{
   TD_OS_File_t* fp;
   IFX_return_t ret = IFX_ERROR;
   /* Path to user home direction */
   IFX_char_t* pUserHomeDir;
   /* Path + option filename */
   IFX_char_t psOptionFilename[MAX_PATH_LEN];
   /* New download path directory */
   IFX_char_t psDownloadDir[MAX_PATH_LEN];

   struct passwd* pwd;
   uid_t uid;

   TD_PTR_CHECK(sPathToDwnldFiles, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   memset(psDownloadDir, 0, MAX_PATH_LEN);

   /* Build path to option file .tapidemorc */
   /* Optional file must be kept in user home directory */
   /* Get the real user ID of the current process */
   uid = getuid();
   /* Get password file entry */
   pwd = getpwuid(uid);
   if (pwd != IFX_NULL)
   {
      /* Get user home directory */
      pUserHomeDir = pwd->pw_dir;
   }
   else
   {
      /* For Danube 2.4 function getpwuid() return NULL (can't find passwd file).
      So, we can use another way to get home directory.
      We don't use this solution for all boards, because it doesn't work on SVIP board */
      /* Get user home directory */
      pUserHomeDir =  getenv("HOME");
   }

   if (IFX_NULL == pUserHomeDir)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Err, Can't get user home directory: %s\n(File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* We add 2 to second argument because, we must remember about '/' and EOL */
   if (MAX_PATH_LEN < strlen(pUserHomeDir)+strlen(DOWNLOAD_FILE_NAME)+2)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Err, User home directory's path is too long.\n "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   sprintf(psOptionFilename, "%s/%s", pUserHomeDir, DOWNLOAD_FILE_NAME);

   /* Open option file */
   fp = TD_OS_FOpen(psOptionFilename, "r");
   if (IFX_NULL == fp)
   {
      /* Failed to open file */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Optional file %s is not available.\n", psOptionFilename));
      return IFX_ERROR;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Optional file %s is available.\n", psOptionFilename));
   }

   /* Read DOWNLOAD_VARIABLE value from file */
   ret = Common_ParseOptionalFile(DOWNLOAD_VARIABLE, psDownloadDir, fp,
                                  nSeqConnId);
   /* File descriptor is no longer needed */
   TD_OS_FClose(fp);
   /* Check if optional file was parsed successfuly */
   if (IFX_SUCCESS != ret)
   {
      /* Failed to read from file */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Failed to read \"%s\" from file %s.\n"
             "(File: %s, line: %d)\n",
             DOWNLOAD_VARIABLE, DOWNLOAD_FILE_NAME,
              __FILE__, __LINE__));

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Setting download path from %s: %s...fail.\n", DOWNLOAD_FILE_NAME,
             psDownloadDir));

      return IFX_ERROR;
   }

   if (IFX_SUCCESS != Common_CheckDownloadPathSyntax(psDownloadDir))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Err, Incorrected download path syntax.\n(File: %s, line: %d)\n",
              __FILE__, __LINE__));

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Setting download path from %s: %s...fail.\n", DOWNLOAD_FILE_NAME,
             psDownloadDir));
      return IFX_ERROR;
   }

   /* Check if binary files are available */
   if(IFX_SUCCESS == Common_CheckDownloadPath(psDownloadDir, pCpuDevice, IFX_TRUE,
                                              nSeqConnId))
   {
      /* All required BIN files are found. Set new path to download files. */
      strcpy(sPathToDwnldFiles, psDownloadDir);
      pBoard->pCtrl->pProgramArg->oArgFlags.nUseCustomDownloadsPath = IFX_TRUE;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Setting download path from %s: %s...fail.\n", DOWNLOAD_FILE_NAME,
             psDownloadDir));
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
         ("Setting download path from %s: %s...ok.\n", DOWNLOAD_FILE_NAME,
          psDownloadDir));

   return IFX_SUCCESS;
} /* Common_GetOptionFile */

/**
   Set downloads path.

   \param sPathToDwnldFiles   - pointer to path string
   \param pBoard              - board handle
   \param pCpuDevice          - pointer to device

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
   \remarks
*/
IFX_return_t Common_SetDownloadPath(IFX_char_t* sPathToDwnldFiles,
                                    BOARD_t* pBoard,
                                    TAPIDEMO_DEVICE_CPU_t* pCpuDevice)
{
   TD_PTR_CHECK(sPathToDwnldFiles, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);

   /* Check if user's custom download path is used */
   if (pBoard->pCtrl->pProgramArg->oArgFlags.nUseCustomDownloadsPath)
   {
      if (IFX_SUCCESS == Common_CheckDownloadPathSyntax(sPathToDwnldFiles))
      {
         if (IFX_SUCCESS == Common_CheckDownloadPath(sPathToDwnldFiles,
                                                     pCpuDevice, IFX_TRUE,
                                                     TD_CONN_ID_INIT))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
                  ("Setting user's download path: %s...ok.\n",
                   sPathToDwnldFiles));

            return IFX_SUCCESS;
         }
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Setting user's download path: %s...fail.\n", sPathToDwnldFiles));
      /* set for test info */
      COM_FAILED_TO_USE_CUSTOM_PATH(0);
   }

   /* Check download path from option file .tapidemorc */
   if (IFX_SUCCESS == Common_GetOptionalFile(sPathToDwnldFiles, pBoard,
                                             pCpuDevice, TD_CONN_ID_INIT))
   {
      return IFX_SUCCESS;
   }

   /* Check if default download path is valid */
   if (IFX_SUCCESS == Common_CheckDownloadPath(DEF_DOWNLOAD_PATH,
                                               pCpuDevice, IFX_FALSE,
                                               TD_CONN_ID_INIT))
   {
      strncpy(sPathToDwnldFiles, DEF_DOWNLOAD_PATH, MAX_PATH_LEN);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Setting default download path: %s...ok.\n", sPathToDwnldFiles));

      return IFX_SUCCESS;
   }

   /* Check if default alternative download path is valid */
   if (IFX_SUCCESS == Common_CheckDownloadPath(TD_ALTERNATE_DOWNLOAD_PATH,
                                               pCpuDevice, IFX_FALSE,
                                               TD_CONN_ID_INIT))
   {
      strncpy(sPathToDwnldFiles, TD_ALTERNATE_DOWNLOAD_PATH, MAX_PATH_LEN);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Setting default download path: %s...ok.\n", sPathToDwnldFiles));;

      return IFX_SUCCESS;
   }

   /* If we reach this point, it means application couldn't set download path */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("Setting default download path: %s...fail.\n", DEF_DOWNLOAD_PATH));

   return IFX_ERROR;
}
#endif /* VXWORKS */
#endif /* USE_FILESYSTEM */

/**
   Opens specified device.

   \param pszName - pointer to device name
   \param pBoard  - pointer to board with error handling for given device

   \return device file descriptor or -1 if fails to open.

   \remark
*/
IFX_return_t Common_Open(IFX_char_t* pszName, BOARD_t *pBoard)
{
   IFX_int32_t nFd;
   nFd = TD_OS_DeviceOpen(pszName);

   /* save FD name */
   TD_DEV_IoctlSetNameOfFd(nFd, pszName, pBoard);

   return nFd;
} /* Common_Open() */

#ifdef EASY336
/**
   Adds a voipID to the array.

   \param pPhone - pointer to phone
   \param nVoipID - voipID to add

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      Must be done at start.
*/
static IFX_return_t Common_VoipID_Add(PHONE_t* pPhone, IFX_int32_t nVoipID)
{
   IFX_int32_t *pVoipIDs;

   TD_PTR_CHECK(pPhone, IFX_ERROR);

   pVoipIDs = realloc (pPhone->pVoipIDs, ++pPhone->nVoipIDs *
      sizeof(IFX_int32_t));
   if (pVoipIDs == IFX_NULL)
   {
      return IFX_ERROR;
   }
   pPhone->pVoipIDs = pVoipIDs;
   pPhone->pVoipIDs[pPhone->nVoipIDs - 1] = nVoipID;

   return IFX_SUCCESS;
}

/**
   Removes a voipID from the array.

   \param pPhone - pointer to phone
   \param nVoipID - voipID to remove

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      Must be done at start.
*/
static IFX_return_t Common_VoipID_Remove(PHONE_t* pPhone, IFX_int32_t nVoipID)
{
   IFX_int32_t i;
   IFX_boolean_t bFound = IFX_FALSE;

   TD_PTR_CHECK(pPhone, IFX_ERROR);

   for (i = 0; i < pPhone->nVoipIDs; i++)
   {
      if (pPhone->pVoipIDs[i] == nVoipID)
      {
         pPhone->pVoipIDs[i] = pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
         bFound = IFX_TRUE;
         break;
      }
   }

   if (bFound)
   {
      pPhone->pVoipIDs = realloc (pPhone->pVoipIDs, --pPhone->nVoipIDs *
         sizeof(IFX_int32_t));
      return IFX_SUCCESS;
   }

      return IFX_ERROR;
}

/**
   Increase LineID count for given ConnID.

   \param pPhone - pointer to phone

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
static IFX_return_t Common_ConnID_LineID_CntAdd(PHONE_t* pPhone)
{
   IFX_int32_t i;
   CTRL_STATUS_t* pCtrl;
   TD_CONN_ID_LINE_ID_CNT_t* pFirstFree = IFX_NULL;
   TD_CONN_ID_LINE_ID_CNT_t* pTmp;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard->pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard->pCtrl->pConnID_LinID_Cnt, IFX_ERROR);
   TD_PARAMETER_CHECK((SVIP_RM_UNUSED == pPhone->connID), pPhone->connID,
                      IFX_ERROR);

   /* TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: CntAdd: Line ID %d, Conn ID %d\n",
          pPhone->nPhoneNumber, pPhone->lineID, pPhone->connID)); */
   /* set control structure pointer */
   pCtrl = pPhone->pBoard->pCtrl;

   /* max number of ConnIDs is equal to number of phones */
   for (i=0; i<pCtrl->nSumPhones; i++)
   {
      /* set pointer to current structure */
      pTmp = &pCtrl->pConnID_LinID_Cnt[i];
      /* check if conn ID is the same */
      if (pPhone->connID == pTmp->nConnID)
      {
         /* increase number of LineIDs */
         pTmp->nLineID_Cnt++;
         return IFX_SUCCESS;
      }
      /* get pointer to first free structure */
      if (IFX_NULL == pFirstFree && TD_NOT_SET == pTmp->nConnID)
      {
         pFirstFree = pTmp;
      }
   }
   /* if data for current ConnID was not found, then empty structure is used */
   if (IFX_NULL != pFirstFree)
   {
      /* set ConnID number */
      pFirstFree->nConnID = pPhone->connID;
      /* increase number of Line IDs,
         assumption is made that nLineID is set to 0 */
      pFirstFree->nLineID_Cnt++;
      return IFX_SUCCESS;
   }
   /* failed to increase LineID count */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, Phone No %d: ConnID %d, no more free structures."
          " (File: %s, line: %d)\n",
          pPhone->nPhoneNumber, pPhone->connID,
          __FILE__, __LINE__));
   return IFX_ERROR;
}

/**
   Decrease LineID count for given ConnID.

   \param pPhone - pointer to phone

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
static IFX_return_t Common_ConnID_LineID_CntRemove(PHONE_t* pPhone)
{
   IFX_int32_t i;
   CTRL_STATUS_t* pCtrl;
   TD_CONN_ID_LINE_ID_CNT_t* pTmp;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard->pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard->pCtrl->pConnID_LinID_Cnt, IFX_ERROR);
   TD_PARAMETER_CHECK((SVIP_RM_UNUSED == pPhone->connID), pPhone->connID,
                      IFX_ERROR);

   /* TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: CntRemove: Line ID %d, Conn ID %d\n",
          pPhone->nPhoneNumber, pPhone->lineID, pPhone->connID)); */

   /* set control structure pointer */
   pCtrl = pPhone->pBoard->pCtrl;

   /* max number of ConnIDs is equal to number of phones */
   for (i=0; i<pCtrl->nSumPhones; i++)
   {
      /* set pointer to current structure */
      pTmp = &pCtrl->pConnID_LinID_Cnt[i];
      /* check if ConnID is the same */
      if (pPhone->connID == pTmp->nConnID)
      {
         /* decrease number of LineIDs */
         pTmp->nLineID_Cnt--;
         /* check if any LineID was left */
         if (0 == pTmp->nLineID_Cnt)
         {
            pTmp->nConnID = TD_NOT_SET;
         }
         return IFX_SUCCESS;
      }
   }
   /* failed to decrease LineID count */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, Phone No %d: ConnID %d was not found."
          " (File: %s, line: %d)\n",
          pPhone->nPhoneNumber, pPhone->connID,
          __FILE__, __LINE__));
   return IFX_ERROR;
}
/**
   Check number of LineIDs assigned to ConnID.

   \param pPhone - pointer to phone

   \return number of LineIDs assigned to ConnID or IFX_ERROR.

   \remark
*/
static IFX_int32_t Common_ConnID_LineID_CntCheck(PHONE_t* pPhone)
{
   IFX_int32_t i;
   CTRL_STATUS_t* pCtrl;
   TD_CONN_ID_LINE_ID_CNT_t* pTmp;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard->pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard->pCtrl->pConnID_LinID_Cnt, IFX_ERROR);
   TD_PARAMETER_CHECK((SVIP_RM_UNUSED == pPhone->connID), pPhone->connID,
                      IFX_ERROR);

   /*TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: CntCheck: Line ID %d, Conn ID %d\n",
          pPhone->nPhoneNumber, pPhone->lineID, pPhone->connID)); */

   /* set control structure pointer */
   pCtrl = pPhone->pBoard->pCtrl;

   /* max number of ConnIDs is equal to number of phones */
   for (i=0; i<pCtrl->nSumPhones; i++)
   {
      /* set pointer to current structure */
      pTmp = &pCtrl->pConnID_LinID_Cnt[i];
      /* check if ConnID is the same */
      if (pPhone->connID == pTmp->nConnID)
      {
         return pTmp->nLineID_Cnt;
      }
   }
   /* LineID */
   return 0;
}
#endif /* EASY336 */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Sets file descriptors.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
      On error, CommonClose_FDs() must be called to close all open fd-s.
      On SVIP called twice. In first call it initializes device
      FD to be able to do capabilities listing. During this pass
      pBoard->nUsedChannels == 0 and channel FDs aren't initialized. In
      second call channel FDs get initialized.
*/
IFX_return_t Common_Set_FDs(BOARD_t* pBoard)
{
   IFX_uint8_t i = 0, opened = 0;
   IFX_char_t sdev_name[MAX_DEV_NAME_LEN_WITH_NUMBER];

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /*if ((TYPE_DANUBE != pBoard->nType) || (TYPE_VINAX != pBoard->nType))*/
   if (pBoard->nUseSys_FD)
   {
      /* for VMMC fdSys is not necessary. */
      if (0 > pBoard->nSystem_FD)
      {
         /* Get system fd
            for system fd (e.g. ESASY3201) there is no error handle
            in board structure */
         pBoard->nSystem_FD = Common_Open(pBoard->pszSystemDevName, IFX_NULL);

         if (0 > pBoard->nSystem_FD)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Invalid system file descriptor (%s). "
                  "(File: %s, line: %d)\n",
                  pBoard->pszSystemDevName, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         opened++;
      }
      else
      {
         opened++;
      }
   }
   else
   {
      /* As if we open it. */
      opened++;
   }

   if (0 > pBoard->nDevCtrl_FD)
   {
      /* get vinetic control fd */
      pBoard->nDevCtrl_FD = Common_Open(pBoard->pszCtrlDevName, pBoard);

      if (0 > pBoard->nDevCtrl_FD)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, Invalid device control file descriptor (%s).\n"
               "(File: %s, line: %d)\n",
               pBoard->pszCtrlDevName, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      opened++;
   }
   else
   {
      opened++;
   }

#ifdef TAPI_VERSION4
   if (!pBoard->fSingleFD)
#endif /* TAPI_VERSION4 */
   {
      for (i = 0; i < pBoard->nUsedChannels; i++)
      {
         if (0 > pBoard->nCh_FD[i])
         {
            /* len of pBoard->pszChDevName string + 2 * 10(max len of int) */
            if (MAX_DEV_NAME_LEN_WITH_NUMBER > strlen(pBoard->pszChDevName) + 2)
            {
               sprintf(sdev_name, "%s%d%d", (char *) pBoard->pszChDevName,
                                         (int) pBoard->nID,
                                         (int) (i + 1));
            }
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                    ("Err, Invalid index of array 'sdev_name'. "
                     "(File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            }

            pBoard->nCh_FD[i] = Common_Open(sdev_name, pBoard);

            if (0 > pBoard->nCh_FD[i])
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                    ("Err, Invalid channel file descriptor (ch %d, %s)."
                     "(File: %s, line: %d)\n",
                     i, sdev_name, __FILE__, __LINE__));
               return IFX_ERROR;
            }
            opened++;
         }
      }

      /* Check opened fds */
      if (opened != (pBoard->nUsedChannels + 2))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, One or more fds actually in use ... breaking."
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* if (!pBoard->fSingleFD) */
   /* len of pBoard->pszChDevName string + 10(max len of int) + 1('x' char) */
   if(MAX_DEV_NAME_LEN_WITH_NUMBER > (strlen(pBoard->pszChDevName) + 10 + 1))
   {
      sprintf(sdev_name, "%s%dx", (char *) pBoard->pszChDevName,
              (IFX_int32_t) pBoard->nID);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Invalid index of array 'sdev_name'. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("Device (%s) and channel (%s) fds opened.\n",
          pBoard->pszCtrlDevName, sdev_name));

   return IFX_SUCCESS;
} /* CommonSet_FDs() */


/**
   Close file descriptors.

   \param pBoard - pointer to board

   \return none
*/
IFX_void_t Common_Close_FDs(BOARD_t* pBoard)
{
   IFX_int32_t i = 0;

   TD_PTR_CHECK(pBoard,);

   /*if ((TYPE_DANUBE != pBoard->nType) || (TYPE_VINAX != pBoard->nType))*/
   if (pBoard->nUseSys_FD)
   {
      /* for VMMC fdSys is not necessary. */
      if (0 <= pBoard->nSystem_FD)
      {
         TD_OS_DeviceClose(pBoard->nSystem_FD);
         pBoard->nSystem_FD = -1;
      }
   }

   if (0 <= pBoard->nDevCtrl_FD)
   {
      TD_OS_DeviceClose(pBoard->nDevCtrl_FD);
      pBoard->nDevCtrl_FD = -1;
   }

#ifdef TAPI_VERSION4
   if (!pBoard->fSingleFD)
#endif /* TAPI_VERSION4 */
   {
      if (IFX_NULL != pBoard->nCh_FD)
      {
         for (i = 0; i < pBoard->nUsedChannels; i++)
         {
            if (0 > pBoard->nCh_FD[i])
               continue;

            TD_OS_DeviceClose(pBoard->nCh_FD[i]);
            pBoard->nCh_FD[i] = -1;
         } /* for */
      }
   } /* if (!pBoard->fSingleFD) */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("Device and channel fds closed.\n"));
} /* CommonClose_FDs() */

/**
   Return file descriptor for specific channel (it makes no difference if
                                                phone or data)

   \param pBoard - pointer to board
   \param nChNum - channel number (starting from 0)

   \return IFX_ERROR if err or device not open, otherwise file descriptor
*/
IFX_int32_t Common_GetFD_OfCh(BOARD_t* pBoard, IFX_int32_t nChNum)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nChNum), nChNum, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nUsedChannels < nChNum), nChNum, IFX_ERROR);
#ifdef TAPI_VERSION4
   if (pBoard->fSingleFD)
      return pBoard->nDevCtrl_FD;
   else
#endif
   {
      TD_PTR_CHECK(pBoard->nCh_FD, IFX_ERROR);
      return pBoard->nCh_FD[nChNum];
   }
} /* Common_GetFD_OfCh() */

#ifdef FXO
/**
   Return file descriptor for FXO device control fd.

   \param pCtrl - pointer to control structure
   \param pFxoDev - last checked FXO device, if IFX_NULL then return first FXO
                    device FD, otherwise return next FXO device FD
   \param fOmitSlic121Fxo  - omit the FXO SLIC121 device descriptor.

   \return IFX_ERROR if pFxoDev was IFX_NULL and no FXO device was found.
*/
IFX_int32_t Common_GetNextFxoDevCtrlFd(CTRL_STATUS_t* pCtrl,
                                       TAPIDEMO_DEVICE_FXO_t** ppFxoDev,
                                       IFX_boolean_t fOmitSlic121Fxo)
{
   TAPIDEMO_DEVICE_t* pDevice;
   IFX_int32_t nBoard;
   IFX_return_t nRet = IFX_SUCCESS;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(ppFxoDev, IFX_ERROR);

   if (IFX_NULL == *ppFxoDev)
   {
      /* if no FXO device will be found then return IFX_ERROR */
      nRet = IFX_ERROR;
   }

   /* for all boards */
   for (nBoard=0; nBoard < pCtrl->nBoardCnt; nBoard++)
   {
      for (pDevice = pCtrl->rgoBoards[nBoard].pDevice;
           IFX_NULL != pDevice;
           pDevice = pDevice->pNext)
      {
         /* check if FXO */
         if (TAPIDEMO_DEV_FXO != pDevice->nType)
            continue;

         /* Omit the FXO SLIC121 device descriptor if fOmitSlic121Fxo
            is IFX_TRUE. */
         if ((pDevice->uDevice.stFxo.nFxoType == FXO_TYPE_SLIC121) &&
             (fOmitSlic121Fxo == IFX_TRUE))
         {
            nRet = IFX_SUCCESS;
            continue;
         }

         /* if pFxoDev is reseted then return device FD */
         if (IFX_NULL == *ppFxoDev)
         {
            /* set new value of pointer */
            *ppFxoDev = &pDevice->uDevice.stFxo;
            return IFX_SUCCESS;
         }
         /* check if this is the last device checked last time */
         else if (*ppFxoDev == &pDevice->uDevice.stFxo)
         {
            *ppFxoDev = IFX_NULL;
         }
      } /* for all devices */
   } /* for all boards */

   return nRet;
} /* Common_GetNextFxoDevCtrlFd() */
#endif /* FXO */

/**
   Read version after succesfull initialization and display it.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
      This must be done only after a successfull basic device initialization
*/
IFX_return_t Common_GetVersions(BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;

   return ret;
} /* Common_GetVersions() */

/**
   Board registration.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t Common_RegisterBoard(BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_ERROR;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   switch (pBoard->nType)
   {
#if EASY3332
      case TYPE_VINETIC:
         ret = BOARD_Easy3332_Register(pBoard);
      break;
#endif /* EASY3332  */
#if EASY50510
      case TYPE_VINETIC:
         ret = BOARD_Easy50510_Register(pBoard);
      break;
#endif /* EASY50510  */
#if DANUBE
      case TYPE_DANUBE:
         ret = BOARD_Easy50712_Register(pBoard);
      break;
#endif /* DANUBE */
#if DXT
      case TYPE_DUSLIC_XT:
#if defined (EASY3201) || defined (EASY3201_EVS)
         if (1 == pBoard->nID)
         {
            /* The main board */
            ret = BOARD_Easy3201_Register(pBoard);
         }
         else
#endif /* defined (EASY3201) || defined (EASY3201_EVS) */
         {
            /*The extension board */
            ret = BOARD_Easy3111_Register(pBoard);
         }
      break;
#endif /* DXT */
#if VINAX
      case TYPE_VINAX:
         ret = BOARD_Easy80800_Register(pBoard);
      break;
#endif /* VINAX */
#if AR9
      case TYPE_AR9:
         ret = BOARD_Easy508XX_Register(pBoard);
      break;
#endif /* AR9 */
#if TD_XWAY_XRX300
      case TYPE_XWAY_XRX300:
         ret = BOARD_XwayXRX300_Register(pBoard);
      break;
#endif /* TD_XWAY_XRX300 */
#if EASY336
      case TYPE_SVIP:
         ret = BOARD_Easy336_Register(pBoard);
      break;
#ifdef WITH_VXT
      case TYPE_XT16:
         ret = BOARD_XT_16_Register(pBoard);
      break;
#endif /* WITH_VXT */
#endif /* EASY336 */
#if XT16
      case TYPE_XT16:
         ret = BOARD_XT_16_Register(pBoard);
      break;
#endif /* XT16 */
#if VR9
      case TYPE_VR9:
         ret = BOARD_Easy80910_Register(pBoard);
      break;
#endif /* VR9 */
      default:
         /* Unknown board type */
         ret = IFX_ERROR;
      break;
   }

   /* Set indention used by TRACE() */
   if (IFX_SUCCESS == ret)
   {
      /* Indention = length of board name + length of ": " */
      pBoard->pIndentBoard =
         Common_GetTraceIndention(strlen(pBoard->pszBoardName) + 2);

      /* Indention = length of phone name + length of phone number ": " */
      pBoard->pIndentPhone =
        Common_GetTraceIndention(strlen(TD_PHONE_STRING) + TD_PHONE_NUM_LEN);
   }

   return ret;
} /* Common_RegisterBoard() */

/**
   Handles the TAPI error codes and stack.

   \param nFD -    device file descriptor of system, which generated
                   the error (must not be fd of channel)
   \param pBoard - board where the error occured
   \param nDev   - device number where the error occured
   \remarks
*/
IFX_void_t Common_ErrorHandle(IFX_int32_t nFD, BOARD_t* pBoard,
                              IFX_uint16_t nDev)
{
   IFX_TAPI_Error_t error;
   IFX_int32_t i = 0;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_uint32_t code = 0;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t *pCpuDevice = IFX_NULL;

   /* check input arguments */
   TD_PTR_CHECK(pBoard,);

   memset(&error, 0, sizeof(IFX_TAPI_Error_t));
#ifdef TAPI_VERSION4
   error.nDev = nDev;
#endif
   ret = TD_IOCTL(nFD, IFX_TAPI_LASTERR, (IFX_int32_t) &error,
            nDev, TD_CONN_ID_ERR);

   if (ret != IFX_ERROR && error.nCode > 0)
   {
      /* we have additional information */
#ifdef TAPI_VERSION4
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
         ("%s: Error on device %2d\n"
         "%sError code 0x%X.\n",
         pBoard->pszBoardName, error.nDev,
         pBoard->pIndentBoard, error.nCode));
#else
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
         ("%s: Error code 0x%X.\n",
         pBoard->pszBoardName, error.nCode));
#endif
      if ((pDevice = Common_GetDevice_CPU(pBoard->pDevice)) == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
              ("Err, CPU device not found. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return;
      }
      pCpuDevice = &pDevice->uDevice.stCpu;
      if (error.nCnt >= IFX_TAPI_MAX_ERROR_ENTRIES)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
               ("Err, Max received errors count %d too big, showing %d errors. "
                "(File: %s, line: %d)\n",
                error.nCnt, IFX_TAPI_MAX_ERROR_ENTRIES, __FILE__, __LINE__));
      }
      for (i = 0; i < TD_GET_MIN(error.nCnt, IFX_TAPI_MAX_ERROR_ENTRIES); ++i)
      {
         code = (error.stack[i].nHlCode << 16) |
                 error.stack[i].nLlCode;

         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
               ("%s:%d HL Code 0x%X, LL Code 0x%X\n",
               error.stack[i].sFile,
               error.stack[i].nLine,
               error.stack[i].nHlCode,
               error.stack[i].nLlCode));
         pCpuDevice->DecodeErrno(code, TD_CONN_ID_ERR);
      } /* for */
   } /* if (ret != IFX_ERROR && error.nCode > 0) */
} /* Common_ErrorHandle */

/**
   Set number of used channels (used to check how many FDs should be opened).

   \param pBoard - pointer to board
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_SetPhonesAndUsedChannlesNumber(BOARD_t* pBoard,
                                                   IFX_uint32_t nSeqConnId)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);
#ifdef TD_DECT
   TD_PTR_CHECK(pBoard->pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard->pCtrl->pProgramArg, IFX_ERROR);
#endif /* TD_DECT */

   pBoard->nUsedChannels = (pBoard->nMaxCoderCh > pBoard->nMaxAnalogCh) ?
                            pBoard->nMaxCoderCh : pBoard->nMaxAnalogCh;
   /* set number of phones */
   pBoard->nMaxPhones = pBoard->nMaxAnalogCh;
#ifdef TD_DECT
   pBoard->nUsedChannels = (pBoard->nMaxDectCh > pBoard->nUsedChannels) ?
                            pBoard->nMaxDectCh : pBoard->nUsedChannels;
   /* check if DECT support is on */
   if (!pBoard->pCtrl->pProgramArg->oArgFlags.nNoDect)
   {
      /* increase number of phones by number of DECT channels */
      pBoard->nMaxPhones += pBoard->nMaxDectCh;
   }
#endif /* TD_DECT */
   pBoard->nUsedChannels = (pBoard->nMaxPCM_Ch > pBoard->nUsedChannels) ?
                            pBoard->nMaxPCM_Ch : pBoard->nUsedChannels;

   if (0 == pBoard->nUsedChannels)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, %s invalid number of channels "
             "(Analog %d, PCM %d, Data %d, DECT %d). "
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, pBoard->nMaxAnalogCh,
             pBoard->nMaxCoderCh, pBoard->nMaxPCM_Ch, pBoard->nMaxDectCh,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}
#ifndef STREAM_1_1
/**
   Get capabilities, basically number of phone, data and pcm channels.

   \param pBoard - pointer to board
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_GetCapabilities(BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
   IFX_TAPI_CAP_t CapList;
   IFX_int32_t nIores;
#else /* TAPI_VERSION4 */
   IFX_int32_t nCapNr = 0;
#if 0 /* if not USE_OLD_CAP_LIST - for now new version is disabled */
   IFX_TAPI_CAP_LIST_t capList;
#endif /* USE_OLD_CAP_LIST */
   IFX_TAPI_CAP_t *pCapList = IFX_NULL;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
#endif /* TAPI_VERSION4 */
   IFX_int32_t i = 0;

   /* Check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

#ifndef TAPI_VERSION4
   /* Get the cap list size */
   ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_NR, (int) &nCapNr,
                  nDevTmp, nSeqConnId);
   if (IFX_ERROR == ret)
   {
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
        ("Number of capabilities %d.\n", (int) nCapNr));

#ifdef VXWORKS
   if (0 == nCapNr)
   {
      /** \todo Under VxWorks a count 0 is received, strange ??? */
      nCapNr = 20;
   }
#endif /* VXWORKS */

   if (0 == nCapNr)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("0 capabilities available read from fd %d of ctrl ch, "
            " was TAPI initialized? \n",
            (int) pBoard->nDevCtrl_FD));
      return IFX_ERROR;
   }

   pCapList = TD_OS_MemAlloc(nCapNr * sizeof(IFX_TAPI_CAP_t));
   if (IFX_NULL == pCapList)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("Err, pCapList is null. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   memset(pCapList, 0, sizeof(nCapNr * sizeof(IFX_TAPI_CAP_t)));

#if 0 /* if not USE_OLD_CAP_LIST - for now new version is disabled */
   memset (&capList, 0, sizeof (capList));
#ifdef TAPI_VERSION4
   capList.dev = nDev;
   nDevTmp = CapList.dev;
#endif /* TAPI_VERSION4 */
   capList.nCap = nCapNr;
   capList.pList = pCapList;

   /* Get the cap list */
   ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_LIST, (int) &capList,
                   nDevTmp, nSeqConnId);
#else /* USE_OLD_CAP_LIST */

#ifdef TAPI_VERSION4
   pCapList->dev = nDev;
   nDevTmp = pCapList->dev;
#endif /* TAPI_VERSION4 */

   /* Get the cap list */
   ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_LIST, pCapList,
                   nDevTmp, nSeqConnId);
#endif /* USE_OLD_CAP_LIST */
   if (IFX_ERROR == ret)
   {
      TD_OS_MemFree(pCapList);
      return IFX_ERROR;
   }

   for (i = 0; i < nCapNr; i++)
   {
      switch (pCapList[i].captype)
      {
         case IFX_TAPI_CAP_TYPE_PCM:
            pBoard->nMaxPCM_Ch = pCapList[i].cap;
            break;
         case IFX_TAPI_CAP_TYPE_CODECS:
            pBoard->nMaxCoderCh = pCapList[i].cap;
            break;
         case IFX_TAPI_CAP_TYPE_PHONES:
            pBoard->nMaxAnalogCh = pCapList[i].cap;
            break;
         case IFX_TAPI_CAP_TYPE_DECT:
            pBoard->nMaxDectCh = pCapList[i].cap;
            break;
         case IFX_TAPI_CAP_TYPE_T38:
            pBoard->nMaxT38 = pCapList[i].cap;
            break;
#ifdef SLIC121_FXO
         case IFX_TAPI_CAP_TYPE_FXO:
            pBoard->nMaxFxo = pCapList[i].cap;
            break;
#endif /* SLIC121_FXO */

         default:
            break;
      }
   } /* for all cap.. */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
         ("Available channels: phone %d, data %d, PCM %d, T.38 %d, "
#ifdef SLIC121_FXO
          "FXO %d, "
#endif /* SLIC121_FXO */
          "DECT %d.\n",
          pBoard->nMaxAnalogCh, pBoard->nMaxCoderCh, pBoard->nMaxPCM_Ch,
          pBoard->nMaxT38,
#ifdef SLIC121_FXO
          pBoard->nMaxFxo,
#endif /* SLIC121_FXO */
          pBoard->nMaxDectCh));

   /* Free the allocated memory */
   TD_OS_MemFree(pCapList);
#if 0 /* if not USE_OLD_CAP_LIST - for now new version is disabled */
   capList.pList = IFX_NULL;
#endif /* USE_OLD_CAP_LIST */
#else /* TAPI_VERSION4 */
   for (i = 0; i < pBoard->nChipCnt; i++)
   {
      /*list codecs*/
      memset(&CapList, 0, sizeof(IFX_TAPI_CAP_t));
      CapList.dev = i;
      CapList.captype = IFX_TAPI_CAP_TYPE_CODECS;
      nIores = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_CHECK,
                        (IFX_int32_t) &CapList,
                        CapList.dev, nSeqConnId);
      switch (nIores)
      {
         case 1:
            /* Store summary data channels amount */
            pBoard->nMaxCoderCh += (IFX_uint8_t) CapList.cap;
            /* If amount of data channels is not equal to previous device */
            /* Check not executed for first device */
            if ((0 != i) && (pBoard->nDataChOnDev != CapList.cap))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
                  ("Warning, amount of data channels not equal"
		    "on dev %d and previous one.\n",i));
            }
            /* Store amount of data channels on dev */
            pBoard->nDataChOnDev = CapList.cap;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("%d data channels available on %s device %d.\n",
                CapList.cap, pBoard->pszBoardName, i));
            break;
         case 0:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("Capability type IFX_TAPI_CAP_TYPE_CODECS not supported.\n"));
            break;
         case -1:
            /* Error reading codec number */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, reading data channel number. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
               return IFX_ERROR;
      }
      /*list ALMs*/
      memset(&CapList, 0, sizeof(IFX_TAPI_CAP_t));
      CapList.dev = i;
      CapList.captype = IFX_TAPI_CAP_TYPE_PHONES;
      nIores = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_CHECK,
                        (IFX_int32_t) &CapList,
                        CapList.dev, nSeqConnId);
      switch (nIores)
      {
         case 1:
            pBoard->nMaxAnalogCh += CapList.cap;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("%d phone channels available on %s device %d.\n",
                CapList.cap, pBoard->pszBoardName, i));
            break;
         case 0:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("Capability type IFX_TAPI_CAP_TYPE_PHONES not supported.\n"));
            break;
         case -1:
            /* Error reading ALM number */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, reading phone channel number. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
            return IFX_ERROR;
      }
      /*list PCMs*/
      memset(&CapList, 0, sizeof(IFX_TAPI_CAP_t));
      CapList.dev = i;
      CapList.captype = IFX_TAPI_CAP_TYPE_PCM;
      nIores = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_CHECK,
                        (IFX_int32_t) &CapList,
                        CapList.dev, nSeqConnId);
      switch (nIores)
      {
         case 1:
            pBoard->nMaxPCM_Ch += CapList.cap;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("%d PCM channels available on %s device %d.\n",
                CapList.cap, pBoard->pszBoardName, i));
            break;
         case 0:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("Capability type IFX_TAPI_CAP_TYPE_PCM not supported.\n"));
            break;
         case -1:
            /* Error reading PCM number */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, reading PCM channel number. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
            return IFX_ERROR;
      }
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
      /*list T38s */
      memset(&CapList, 0, sizeof(IFX_TAPI_CAP_t));
      CapList.dev = i;
      CapList.captype = IFX_TAPI_CAP_TYPE_T38;
      nIores = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_CHECK,
                        (IFX_int32_t) &CapList,
                        CapList.dev, nSeqConnId);
      switch (nIores)
      {
         case 1:
            pBoard->nMaxT38 += CapList.cap;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("%d T.38 channels available on %s device %d.\n",
                CapList.cap, pBoard->pszBoardName, i));
            break;
         case 0:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("Capability type IFX_TAPI_CAP_TYPE_T38 not supported"
                "for device %d.\n", i));
            break;
         case -1:
            /* Error reading ALM number */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, reading T.38 channel number. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
            return IFX_ERROR;
      }
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
   }
#endif /* TAPI_VERSION4 */
   if (pBoard->nMaxT38 > 0)
   { /* T.38 supported*/
      pBoard->nT38_Support = IFX_TRUE;
   }

   return ret;
} /* Common_GetCapabilities() */

#else

IFX_return_t Common_GetCapabilities(BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   return IFX_SUCCESS;
} /* Common_GetCapabilities() */
#endif /* STREAM_1_1 */

/**
   Get FW and dev version with tapi ioctl.

   \param pBoard        - pointer to board
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_DevVersionGet(BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
#ifdef IFX_TAPI_VERSION_DEV_ENTRY_GET
   IFX_TAPI_VERSION_DEV_ENTRY_t oVersion;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
#ifdef TAPI_VERSION4
   for (nDevTmp = 0; nDevTmp < pBoard->nChipCnt; nDevTmp++)
   {
#endif /* TAPI_VERSION4 */
      memset (&oVersion, 0, sizeof (oVersion));
      /* from drivers code for svip nId:
         0 - FW
         1 - HW */
      oVersion.nId = 0;
#ifdef TAPI_VERSION4
      oVersion.dev = nDevTmp;
#endif /* TAPI_VERSION4 */

      while (1)
      {
         if (IFX_SUCCESS !=
             TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_VERSION_DEV_ENTRY_GET,
                &oVersion, nDevTmp, nSeqConnId))
         {
            return IFX_ERROR;
         }
         if ((0 == strlen(oVersion.versionEntry.name)) ||
            (0 == strlen(oVersion.versionEntry.version)))
            /* No more version entries found */
            break;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("%s: dev %d: %s version: %s\n",
             pBoard->pszBoardName, nDevTmp,
             oVersion.versionEntry.name, oVersion.versionEntry.version));
         /** save version number for ITM - only firsts device FW number */
         if (0 == nDevTmp &&
             0 == oVersion.nId)
         {
            COM_GET_FW_VERSION_TAPI_V4(oVersion.versionEntry.version);
         }
         /* go to next */
         oVersion.nId++;
      }
#ifdef TAPI_VERSION4
   }
#endif /* TAPI_VERSION4 */
#endif /* IFX_TAPI_VERSION_DEV_ENTRY_GET */
   return IFX_SUCCESS;
} /* Common_GetCapabilities() */

/**
   Get trace indention pointer.

   \param nSize - size of indention

   \return string with indention
*/

IFX_char_t* Common_GetTraceIndention(IFX_int32_t nSize)
{
   IFX_int32_t i;
   static IFX_boolean_t fStringIsSet = IFX_FALSE;

   if (IFX_FALSE == fStringIsSet)
   {
      /* Fill string with spaces. */
      for(i=0; i < TD_MAX_INDENT; i++)
      {
         acTraceIndentTable[i] = ' ';
      }
      /* set end of string */
      acTraceIndentTable[TD_MAX_INDENT - 1]='\0';
      fStringIsSet = IFX_TRUE;
   }

   /* check if valid size */
   if (0 > nSize)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, Invalid indention size %d, setting Indetation size to 0\n",
             nSize));
      nSize = 0;
   }
   /* indention must be less than TD_MAX_INDENT */
   if (nSize >= TD_MAX_INDENT)
   {
      return &acTraceIndentTable[0];
   }
   else
   {
      /* table elements are counted from 0 so decrease char address by 1 */
      return &acTraceIndentTable[TD_MAX_INDENT - nSize - 1];
   }
}

/*
    Sets codec according to dialed sequence.

    \param pBoard - pointer to the board
    \param pPhone - pointer to the phone
    \param nAction - which action was selected (dialed number)
    \param fConfigureAll - true if all phone on this baord will be configured,
                           otherwise (false) only this one will be configured,
    \param fConfigureConn - configure both phones in connection
   \param nSeqConnId    - Seq Conn ID

    \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t Common_SetCodec(BOARD_t* pBoard, PHONE_t* pPhone,
                             IFX_int32_t nAction, IFX_boolean_t fConfigureAll,
                             IFX_boolean_t fConfigureConn,
                             IFX_uint32_t nSeqConnId)
{
   IFX_int32_t nCodec = 0;
   IFX_int32_t nPacketLen = 0;
   IFX_int32_t i = 0;
#ifdef EASY336
   VOIP_DATA_CH_t* pCodec = IFX_NULL;
#endif
#if (!defined EASY336) && (!defined XT16)
   IFX_int32_t nDataCh = -1;
#endif /* (!defined EASY336) && (!defined XT16) */
#ifndef XT16
   CONNECTION_t* pConn = IFX_NULL;
#endif /* XT16 */
   IFX_TAPI_COD_AAL2_BITPACK_t nBitPack = IFX_TAPI_COD_RTP_BITPACK;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pBoard->pCtrl, IFX_ERROR);

   /* check if coder channels are avaible and structure allocated */
   if ((0 >= pBoard->nMaxCoderCh || IFX_NULL == pBoard->pDataChStat)
#ifdef EASY336
       /* if xt16 board is connected to svip board then resource manager
         alows external voip calls for phones on XT16 board */
       && (TYPE_XT16 != pBoard->nType)
#endif /* EASY336 */
      )
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, %s: Feature *%d - codec change not avaible - no coder channels\n",
             pBoard->pszBoardName, nAction));
      return IFX_ERROR;
   }
   /* check feature number */
   if ((nAction < 1000) || (nAction > 1999))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Invalid set codec feature number %d\n", nAction));
      return IFX_ERROR;
   }

   /* get number of codec */
   i = (nAction%1000)/10;
   i = i - (i%2);
   /* because more packet lengths can be added
      there are 20 DTMF codes for each codec */
   switch(i)
   {
      /* G.711 codecs */
      /* DTMF code *100x */
      case 0:
         nCodec = IFX_TAPI_ENC_TYPE_MLAW;
         break;
      /* DTMF code *102x */
      case 2:
         nCodec = IFX_TAPI_ENC_TYPE_ALAW;
         break;
      /* G.726 codecs */
      /* DTMF code *104x */
      case 4:
         nCodec = IFX_TAPI_ENC_TYPE_G726_16;
         break;
      /* DTMF code *106x */
      case 6:
         nCodec = IFX_TAPI_ENC_TYPE_G726_24;
         break;
      /* DTMF code *108x */
      case 8:
         nCodec = IFX_TAPI_ENC_TYPE_G726_32;
         break;
      /* DTMF code *110x */
      case 10:
         nCodec = IFX_TAPI_ENC_TYPE_G726_40;
         break;
      /* G.729 codec */
      case 12:
         nCodec = IFX_TAPI_ENC_TYPE_G729;
         break;
      /* AMR codecs */
      case 14:
         nCodec = IFX_TAPI_ENC_TYPE_AMR_4_75;
         break;
      case 16:
         nCodec = IFX_TAPI_ENC_TYPE_AMR_5_15;
         break;
      case 18:
         nCodec = IFX_TAPI_ENC_TYPE_AMR_5_9;
         break;
      case 20:
         nCodec = IFX_TAPI_ENC_TYPE_AMR_6_7;
         break;
      case 22:
         nCodec = IFX_TAPI_ENC_TYPE_AMR_7_4;
         break;
      case 24:
         nCodec = IFX_TAPI_ENC_TYPE_AMR_7_95;
         break;
      case 26:
         nCodec = IFX_TAPI_ENC_TYPE_AMR_10_2;
         break;
      case 28:
         nCodec = IFX_TAPI_ENC_TYPE_AMR_12_2;
         break;
      /* iLBC codecs */
      case 30:
         nCodec = IFX_TAPI_ENC_TYPE_ILBC_133;
         break;
      case 32:
         nCodec = IFX_TAPI_ENC_TYPE_ILBC_152;
         break;
      /* G.722 codec */
      case 34:
         nCodec = IFX_TAPI_ENC_TYPE_G722_64;
         break;
      /* G.722.1 codecs */
      case 36:
         nCodec = IFX_TAPI_ENC_TYPE_G7221_24;
         break;
      case 38:
         nCodec = IFX_TAPI_ENC_TYPE_G7221_32;
         break;
      /* linear codecs */
      case 40:
         nCodec = IFX_TAPI_ENC_TYPE_LIN16_8;
         break;
      case 42:
         nCodec = IFX_TAPI_ENC_TYPE_LIN16_16;
         break;
      case 44:
         nCodec = IFX_TAPI_ENC_TYPE_G723_53;
         break;
      case 46:
         nCodec = IFX_TAPI_ENC_TYPE_G723_63;
         break;
      case 48:
         nCodec = IFX_TAPI_ENC_TYPE_MLAW_VBD;
         break;
      case 50:
         nCodec = IFX_TAPI_ENC_TYPE_ALAW_VBD;
         break;
      case 52:
         nCodec = IFX_TAPI_ENC_TYPE_G728;
         break;
      /* DTMF code *1540x */
      case 54:
         nCodec = IFX_TAPI_ENC_TYPE_G729_E;
         break;
      /* G.726 codecs */
      /* DTMF code *156x */
      case 56:
         nCodec = IFX_TAPI_ENC_TYPE_G726_16;
         nBitPack = IFX_TAPI_COD_AAL2_BITPACK;
         break;
      /* DTMF code *158x */
      case 58:
         nCodec = IFX_TAPI_ENC_TYPE_G726_24;
         nBitPack = IFX_TAPI_COD_AAL2_BITPACK;
         break;
      /* DTMF code *160x */
      case 60:
         nCodec = IFX_TAPI_ENC_TYPE_G726_32;
         nBitPack = IFX_TAPI_COD_AAL2_BITPACK;
         break;
      /* DTMF code *162x */
      case 62:
         nCodec = IFX_TAPI_ENC_TYPE_G726_40;
         nBitPack = IFX_TAPI_COD_AAL2_BITPACK;
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, %s: Feature *%d - codec not avaible\n",
                pBoard->pszBoardName, nAction));
         return IFX_ERROR;
         break;
   } /* switch() */

   /* check if codec supported */
   if (IFX_SUCCESS != VOIP_CodecAvailabilityCheck(pBoard, nCodec, nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, %s: Feature *%d - codec not supported\n",
             pBoard->pszBoardName, nAction));
      return IFX_ERROR;
   }

   /* get packetisation length */
   i = nAction %20;
   switch (i)
   {
      case 0:
         nPacketLen = IFX_TAPI_COD_LENGTH_10;
         break;
      case 1:
         nPacketLen = IFX_TAPI_COD_LENGTH_20;
         break;
      case 2:
         nPacketLen = IFX_TAPI_COD_LENGTH_30;
         break;
      case 3:
         nPacketLen = IFX_TAPI_COD_LENGTH_40;
         break;
      case 4:
         nPacketLen = IFX_TAPI_COD_LENGTH_50;
         break;
      case 5:
         nPacketLen = IFX_TAPI_COD_LENGTH_60;
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, %s: Feature *%d - packet length not avaible\n",
                pBoard->pszBoardName, nAction));
         return IFX_ERROR;
   } /* switch (i) */

   if ((nCodec != 0) && (nPacketLen != 0))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Feature DTMF code: *%d - Codec change.\n", nAction));
      /* Configue all channels on this board */
      if (fConfigureAll)
      {
         /* check if board has data channels */
         if (0 >= pBoard->nMaxCoderCh)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, %s: Unable to change codec - no data channels avaible."
                   " (File: %s, line: %d)\n",
                   pBoard->pszBoardName, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("%s: Set codec for all data channels on this board:\n"
                "%s%s, %s\n",
                pBoard->pszBoardName,
                pBoard->pIndentBoard,
                pCodecName[nCodec], oFrameLen[nPacketLen]));

         for (i = 0; i < pBoard->nMaxCoderCh; ++i)
         {
            pBoard->pDataChStat[i].nCodecType = nCodec;
            pBoard->pDataChStat[i].nFrameLen = nPacketLen;
            pBoard->pDataChStat[i].fStartCodec = IFX_TRUE;
            pBoard->pDataChStat[i].nBitPack = nBitPack;
         } /* for */
      }
      /* Configure only this one */
      else
      {
         /* this feature doesn't work for conference */
#if (!defined EASY336) && (!defined XT16)
         pBoard->pDataChStat[pPhone->nDataCh].nCodecType = nCodec;
         pBoard->pDataChStat[pPhone->nDataCh].nFrameLen = nPacketLen;
         pBoard->pDataChStat[pPhone->nDataCh].fStartCodec = IFX_TRUE;
         pBoard->pDataChStat[pPhone->nDataCh].nBitPack = nBitPack;

         pConn = &pPhone->rgoConn[0];

         if ((IFX_NULL != pConn) &&
             (NO_CONFERENCE == pPhone->nConfIdx))
         {
            if ((EXTERN_VOIP_CALL == pConn->nType) ||
                ((LOCAL_CALL == pConn->nType) &&
                 (pBoard->pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)))
            {
               nDataCh = pPhone->rgoConn[0].nUsedCh;
               VOIP_StopCodec(nDataCh, pConn, pPhone->pBoard, pPhone);
               if (IFX_SUCCESS == VOIP_StartCodec(nDataCh, pConn,
                                                  pPhone->pBoard, pPhone))
               {
                  if(IFX_TAPI_ENC_TYPE_MAX > nCodec)
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
                           ("Phone No %d: Voice encoding: %s, %s\n",
                            pPhone->nPhoneNumber,
                            pCodecName[nCodec],
                            oFrameLen[nPacketLen]));
                  }
                  else
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                           ("Err, Invalid index of array 'pCodecName'. "
                            "(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
               }
               pConn->nFeatureID = nAction;
               if (IFX_TRUE == fConfigureConn)
               {
                  TAPIDEMO_SetAction(pBoard->pCtrl, pPhone, pConn,
                                     IE_CONFIG_PEER, pPhone->nSeqConnId);
               }
            }
         } /* if ((IFX_NULL != conn) */
#elif EASY336
         /* check if using Voip ID, connection in progress
            and no conference */
         if ((pPhone->nVoipIDs > 0) &&
             (NO_CONFERENCE == pPhone->nConfIdx) &&
             (0 < pPhone->nConnCnt))
         {
            /* get connection */
            pConn = &pPhone->rgoConn[0];
            /* check if extern call or local call using codecs */
            if ((EXTERN_VOIP_CALL == pConn->nType) ||
                ((LOCAL_CALL == pConn->nType) &&
                 (pBoard->pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)))
            {
               /* get pointer to codec structure */
               pCodec = Common_GetCodec(pBoard->pCtrl, pConn->voipID,
                                        nSeqConnId);
               if (pCodec != IFX_NULL)
               {
                  /* change codec and packetization */
                  pCodec->nCodecType = nCodec;
                  pCodec->nFrameLen = nPacketLen;
                  pCodec->fStartCodec = IFX_TRUE;
                  pCodec->nBitPack = nBitPack;
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                        ("Err, Phone No %d: Failed to get codec. "
                         "(File: %s, line: %d)\n",
                         pPhone->nPhoneNumber, __FILE__, __LINE__));
                  return IFX_ERROR;
               }
               /* for TAPI V4 number of data channel is not needed */
               VOIP_StopCodec(0, pConn, ABSTRACT_GetBoard(pBoard->pCtrl,
                  IFX_TAPI_DEV_TYPE_VIN_SUPERVIP), pPhone);
               if (IFX_SUCCESS == VOIP_StartCodec(0, pConn,
                   ABSTRACT_GetBoard(pBoard->pCtrl,
                                     IFX_TAPI_DEV_TYPE_VIN_SUPERVIP), pPhone))
               {
                  /* printout codec data */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
                        ("Phone No %d: Voice encoding: %s, %s\n",
                         pPhone->nPhoneNumber,
                         pCodecName[pCodec->nCodecType],
                         oFrameLen[pCodec->nFrameLen]));
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                        ("Err, Phone No %d: Failed to change codec. "
                         "(File: %s, line: %d)\n",
                         pPhone->nPhoneNumber, __FILE__, __LINE__));
                  return IFX_ERROR;
               }
               /* check if second phone should be configured */
               if (IFX_TRUE == fConfigureConn)
               {
                  pConn->nFeatureID = nAction;
                  TAPIDEMO_SetAction(pBoard->pCtrl, pPhone, pConn,
                                     IE_CONFIG_PEER, pPhone->nSeqConnId);
               }
            }
         } /* if ((pPhone->nVoipIDs > 0) && ... */
#endif /* EASY336 */
      } /* if (fConfigureAll) */
      return IFX_SUCCESS;
   }
   else
   {
      return IFX_ERROR;
   }
}

/*
    Configures additional features according to dialing sequence.

    \param pCtrl - pointer to the control status structure
    \param pBoard - pointer to the board
    \param pPhone - pointer to the phone
    \param nAction - which action was selected (dialed number)
    \param nDataCh_FD - file descriptor of the data channel
    \param fConfigureAll - true if all phone on this board will be configured
                           (selected feature must suppotr this),
                           otherwise (false) only this one will be configured,
    \param fConfigureConn - configure boh phones in connection
                            (must be supported by selected feature)
   \param nSeqConnId  - Seq Conn ID

    \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t Common_ConfigureTAPI(CTRL_STATUS_t* pCtrl,
                                  BOARD_t* pBoard,
                                  PHONE_t* pPhone,
                                  IFX_int32_t nAction,
                                  IFX_int32_t nDataCh_FD,
                                  IFX_boolean_t fConfigureAll,
                                  IFX_boolean_t fConfigureConn,
                                  IFX_uint32_t nSeqConnId)
{
   IFX_int32_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   CONNECTION_t* pConn = IFX_NULL;
   IFX_boolean_t bFeatureExist = IFX_FALSE;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

    /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Common_ConfigureTAPI() called with feature number %4d\n",
          nAction));

   if ((nAction >= 1000) && (nAction <= 1899))
   {
      /* feature exists */
      bFeatureExist = IFX_TRUE;
      /* Set codec */
      ret = Common_SetCodec(pBoard, pPhone, nAction,
                            fConfigureAll, fConfigureConn, nSeqConnId);
   } /* if nAction between 1000..1999 */
   else if ((nAction >= 1900) && (nAction <= 1999))
   {
      /* feature exists */
      bFeatureExist = IFX_TRUE;

      i=nAction%100;

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Feature DTMF code: *%d - Set PCM coder type.\n", nAction));

      /* Change codec */
      ret = PCM_CodecSet(i);

      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                 ("Err, Unable to set PCM codec, invalid number given. %d. "
                  "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* Codec recognized */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
              ("PCM codec set to: %s\n",
               Common_Enum2Name(i,TD_rgEnumPCM_CodecName)));

   } /* if nAction between 1900..1999 */
   else if ((nAction >= 2000) && (nAction <= 2099))
   {
      switch (nAction)
      {
         case 2000:
            {
               /* feature exists */
               bFeatureExist = IFX_TRUE;
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                     ("Feature DTMF code: *%d - Set speech range.\n", nAction));
#if defined(EASY336) || defined(XT16)
               /* check if single connection and Voip ID is assigned */
               if ((pPhone->nVoipIDs <= 0) ||
                   (NO_CONFERENCE != pPhone->nConfIdx) ||
                   (0 >= pPhone->nConnCnt))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Phone No %d: Configure NarrowBand can be only done "
                         "during single call(no conference), codec must be used. "
                         "(File: %s, line: %d)\n",
                         pPhone->nPhoneNumber, __FILE__, __LINE__));
                  return IFX_ERROR;
               }
#endif /* defined(EASY336) || defined(XT16) */
               /* Turn this channel to narrowband, coder is set to G.711 */
               ret = FEATURE_SetSpeechRange(pPhone, nDataCh_FD,
                                            IFX_TAPI_LINE_TYPE_FXS_NB,
                                            IFX_TAPI_COD_TYPE_MLAW, IFX_FALSE);

               if (ret != IFX_SUCCESS)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err, to configure NarrowBand. "
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  return IFX_ERROR;
               }
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Switch one party to NARROWBAND\n"));

#if defined(HAVE_DRV_VMMC_HEADERS) || defined(EASY3201) || defined(EASY3201_EVS)
/* play tone to indicate change of setting */
#ifdef TAPI_VERSION4
               if (pBoard->fSingleFD)
               {
                  Common_TonePlay(pCtrl, pPhone, TD_ITN_SET_BAND_NARROW);
               } /* if (pBoard->fSingleFD) */
               else
#endif /* TAPI_VERSION4 */
               {
                  TD_IOCTL(nDataCh_FD, IFX_TAPI_TONE_LOCAL_PLAY,
                           TD_ITN_SET_BAND_NARROW,
                           nDevTmp, pPhone->nSeqConnId);
               } /* if (pBoard->fSingleFD) */
#endif /* defined(HAVE_DRV_VMMC_HEADERS) || defined(EASY3201) */

               pPhone->fWideBand_Cfg = IFX_FALSE;
            }
            break;
         case 2001:
            {
               /* feature exists */
               bFeatureExist = IFX_TRUE;
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                     ("Feature DTMF code: *%d - Set speech range.\n", nAction));
#if defined(EASY336) || defined(XT16)
               /* check if single connection and Voip ID is assigned */
               if ((pPhone->nVoipIDs <= 0) ||
                   (NO_CONFERENCE != pPhone->nConfIdx) ||
                   (0 >= pPhone->nConnCnt))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Phone No %d: Configure WideBand can be only done "
                         "during call that uses codecs. "
                         "(File: %s, line: %d)\n",
                         pPhone->nPhoneNumber, __FILE__, __LINE__));
                  return IFX_ERROR;
               }
#endif /* defined(EASY336) || defined(XT16) */
               /* Turn this channel to wideband, coder is set to G.722 (64k)*/
               ret = FEATURE_SetSpeechRange(pPhone, nDataCh_FD,
                                            IFX_TAPI_LINE_TYPE_FXS_WB,
                                            IFX_TAPI_COD_TYPE_G722_64, IFX_TRUE);
               if (ret != IFX_SUCCESS)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err to configure WideBand. "
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  return IFX_ERROR;
               }
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Switch one party to WIDEBAND\n"));

#if defined(HAVE_DRV_VMMC_HEADERS) || defined(EASY3201) || defined(EASY3201_EVS)
/* play tone to indicate change of setting */
#ifdef TAPI_VERSION4
               if (pBoard->fSingleFD)
               {
                  Common_TonePlay(pCtrl, pPhone, TD_ITN_SET_BAND_WIDE);
               } /* if (pBoard->fSingleFD) */
               else
#endif /* TAPI_VERSION4 */
               {
                  TD_IOCTL(nDataCh_FD, IFX_TAPI_TONE_LOCAL_PLAY,
                           TD_ITN_SET_BAND_WIDE,
                           nDevTmp, pPhone->nSeqConnId);
               } /* if (pBoard->fSingleFD) */
#endif /* defined(HAVE_DRV_VMMC_HEADERS) || defined(EASY3201) */

               pPhone->fWideBand_Cfg = IFX_TRUE;
            }
            break;
         case 2010:
            {
               /* feature exists */
               bFeatureExist = IFX_TRUE;
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                     ("Feature DTMF code: *%d - Disable AGC.\n", nAction));
               /* check if single call */
               if ((NO_CONFERENCE == pPhone->nConfIdx) &&
                   (0 < pPhone->nConnCnt))
               {
                  pConn = &pPhone->rgoConn[0];
                  /* check if call uses codecs */
                  if ((EXTERN_VOIP_CALL == pConn->nType) ||
                      ((LOCAL_CALL == pConn->nType) &&
                       (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)))
                  {
                     /* everything alright - enable AGC */
                  }
                  else
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Phone No %d: Disabling Automatic Gain Control can "
                            "be only done during single call that uses codecs. "
                            "(File: %s, line: %d)\n",
                            pPhone->nPhoneNumber, __FILE__, __LINE__));
                     return IFX_ERROR;
                  }
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Phone No %d: Disabling Automatic Gain Control can "
                         "be only done during single call that uses codecs. "
                         "(File: %s, line: %d)\n",
                         pPhone->nPhoneNumber, __FILE__, __LINE__));
                  return IFX_ERROR;
               }

               /* Disable Automatic Gain Control (AGC) */
               ret = FEATURE_AGC_Enable(nDataCh_FD, pPhone, IFX_FALSE);

               if (ret != IFX_SUCCESS)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err with disable AGC. "
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  return IFX_ERROR;
               }

#ifndef TAPI_VERSION4
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                     ("Automatic Gain Control (AGC) was disabled on data ch %d\n",
                      (int) pPhone->nDataCh));
#else/* TAPI_VERSION4 */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                     ("Automatic Gain Control (AGC) was disabled on phone "
                      "dev %d, channel %d\n",
                      pPhone->nDev,
                      pPhone->nPhoneCh));
#endif /* TAPI_VERSION4 */

               pPhone->fAGC_Cfg = IFX_FALSE;
            }
            break;
         case 2011:
            {
               /* feature exists */
               bFeatureExist = IFX_TRUE;
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                     ("Feature DTMF code: *%d - Enable AGC.\n", nAction));
               /* check if single call */
               if ((NO_CONFERENCE == pPhone->nConfIdx) &&
                   (0 < pPhone->nConnCnt))
               {
                  pConn = &pPhone->rgoConn[0];
                  /* check if call uses codecs */
                  if ((EXTERN_VOIP_CALL == pConn->nType) ||
                      ((LOCAL_CALL == pConn->nType) &&
                       (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)))
                  {
                     /* everything alright - enable AGC */
                  }
                  else
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Phone No %d: Enabling Automatic Gain Control can "
                            "be only done during single call that uses codecs. "
                            "(File: %s, line: %d)\n",
                            pPhone->nPhoneNumber, __FILE__, __LINE__));
                     return IFX_ERROR;
                  }
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Phone No %d: Enabling Automatic Gain Control can "
                         "be only done during single call that uses codecs. "
                         "(File: %s, line: %d)\n",
                         pPhone->nPhoneNumber, __FILE__, __LINE__));
                  return IFX_ERROR;
               }
               /* Enable Automatic Gain Control (AGC) */
               ret = FEATURE_AGC_Cfg(nDataCh_FD, pPhone,
                                     -30,
                                     10,
                                     -20,
                                     -25);

               if (ret != IFX_SUCCESS)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err with configure AGC. "
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  return IFX_ERROR;
               }

               ret = FEATURE_AGC_Enable(nDataCh_FD, pPhone, IFX_TRUE);
               if (ret != IFX_SUCCESS)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err, with enable AGC. "
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  return IFX_ERROR;
               }
#ifndef EASY336
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                     ("Automatic Gain Control (AGC) was enabled on data ch %d\n",
                      (int) pPhone->nDataCh));
#else /* EASY336 */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                     ("Automatic Gain Control (AGC) was enabled on phone "
                      "dev %d, channel %d\n",
                      pPhone->nDev,
                      pPhone->nPhoneCh));
#endif /* EASY336 */

               pPhone->fAGC_Cfg = IFX_TRUE;
            }
            break;
         case 2020:
         /** No voice activity detection. */
         case 2021:
         /** Voice activity detection on, in this case also comfort noise and
            spectral information (nicer noise) is switched on.*/
         case 2022:
         /** Voice activity detection on with comfort noise generation without
            spectral information. */
         case 2023:
         /** Voice activity detection on with comfort noise generation without
            silence compression */
         case 2024:
         /** Voice activity detection on with silence compression without
            comfort noise generation. */
            /* feature exists */
            bFeatureExist = IFX_TRUE;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                  ("Feature DTMF code: *%d - Change VAD setting.\n",
                   nAction));
            {
               ret = Common_SetVad(pPhone, pBoard, nAction % 10);
            }
         case 2025:
         case 2026:
         case 2027:
         case 2028:
         case 2029:
            break;
#ifdef TD_DECT
         case 2030:
            bFeatureExist = IFX_TRUE;
            /* Disable echo suppressor for DECT channels.
               Does not apply to the actived DECT channels.
               Apply only to all newly activated DECT channels */
            pPhone->pBoard->pCtrl->pProgramArg->nDectEcCfg = IFX_TAPI_EC_TYPE_OFF;

            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Feature DTMF code: Disable echo suppressor for DECT channels\n"
                   "                   (apply only to new DECT connections)."));
            break;
         case 2031:
            bFeatureExist = IFX_TRUE;
            /* Enable echo suppressor for DECT channels.
               Does not apply to the actived DECT channels.
               Apply only to all newly activated DECT channels */
            pPhone->pBoard->pCtrl->pProgramArg->nDectEcCfg = IFX_TAPI_EC_TYPE_ES;

            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Feature DTMF code: Enable echo suppressor for DECT channels\n"
                   "                   (apply only to new DECT connections)."));
#endif /* TD_DECT */
      case 2050:
#ifdef FXO
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         if (IFX_NULL != pPhone->pFXO)
         {
            FXO_FlashHook(pPhone->pFXO);
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: unable to call FXO_FlashHook(),"
                   " no fxo connected.\n",
                   pPhone->nPhoneNumber));
         }
#else /* FXO */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, FXO support must be on to use feature number *%04d.\n",
                nAction));
#endif /* FXO */
         break;
         default:
            ret = IFX_ERROR;
            break;
      }
   } /* else if ((nAction >= 2000) && (nAction <= 2099)) */
   else if ((nAction >= 2100) && (nAction <= 2199))
   {
      /*   actions needed by ITM for of call progress tone detection
           during modular test:
        2101 - stop playing tone
        2101 - play dialtone
        2102 - play busy tone
        2103 - play ringback tone
        if number of action changes,
        then it should be also changed in test scripts.
       */
      if (nAction == 2100)
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Feature DTMF code: *%d - Stop playing tone.\n",
                nAction));
         /* stop playing tone */
         ret = Common_ToneStop(pCtrl, pPhone);
         if (IFX_SUCCESS == ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Phone No %d: Stop playing tone.\n",
                   (int) pPhone->nPhoneNumber));
         }
      }
      else if (nAction == 2101)
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Feature DTMF code: *%d - Play dialtone.\n",
                nAction));
         Common_ToneStop(pCtrl, pPhone);
         /* play dialtone */
         TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, DIALTONE_IDX, "Dialtone");
      }
      else if (nAction == 2102)
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Feature DTMF code: *%d - Play busy tone.\n",
                nAction));
         Common_ToneStop(pCtrl, pPhone);
         /* play busy tone */
         TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, BUSY_TONE_IDX, "Busy tone");
      }
      else if (nAction == 2103)
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Feature DTMF code: *%d - Play ringback tone.\n",
                nAction));
         Common_ToneStop(pCtrl, pPhone);
         /* play ringback tone */
         TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, RINGBACK_TONE_IDX,
                            "Ringback tone");
      } /* if (nAction == 2101) */
   } /* else if ((nAction >= 2100) && (nAction <= 2199)) */
   else if ((nAction >= 3000) && (nAction <= 3999))
   {
   }
   else if ( (nAction >= 4000) && (nAction <= 4999) )
   {
      if (IFX_TRUE != g_pITM_Ctrl->fITM_Enabled)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("DTMF codes from range <4000:4999> are available only during "
                "Internal Test Mode (ITM).\n"));
      }
      else if (nAction == 4001)
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         TD_DEV_PrintConnections(pBoard);
      }
      else if (nAction == 4002)
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         TD_DEV_PrintMappedDataCh(pBoard);
      }
      /* temporary code for changing country settings */
      else if (nAction == 4020)
      {

      }
      else if (nAction == 4100)
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         /* use default CPTs */
         g_pITM_Ctrl->nUseCustomCpt = IFX_FALSE;
      }
      else if (nAction == 4101)
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         /* use custom CPTs */
         g_pITM_Ctrl->nUseCustomCpt = IFX_TRUE;
      }
      else if (nAction == 4102)
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         COM_CPT_SET(pCtrl, IFX_NULL);
      }
      else if ( (nAction >= 4110) && (nAction <= 4119) )
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         i = nAction % 10;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Play custom CPT %d using ioctl(IFX_TAPI_TONE_LOCAL_PLAY).\n",
                (CPT_INDEX + i)));
         TD_IOCTL(pPhone->nDataCh_FD, IFX_TAPI_TONE_LOCAL_PLAY,
               (IFX_int32_t) (CPT_INDEX + i), nDevTmp, pPhone->nSeqConnId);
      }
      else if ( (nAction >= 4120) && (nAction <= 4129) )
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         i = nAction % 10;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Play CPT %d using Common_TonePlay().\n", i));
         Common_TonePlay(pCtrl, pPhone, i);
      }
      else if ( (nAction >= 4200) && (nAction <= 4399) )
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
#ifdef SVIP
         RM_SVIP_RU_t RU;
         IFX_TAPI_TONE_PLAY_t aTone;
         RESOURCES_t resources;
#else /* SVIP */
#ifndef TAPI_VERSION4
         IFX_TAPI_TONE_IDX_t nTone;
#endif /* TAPI_VERSION4 */
#endif /* SVIP */

         /* Given tone number */
         i = nAction % 200;
#ifdef SVIP
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Play CPT %d using ioctl (IFX_TAPI_TONE_PLAY).\n", i));
         memset (&aTone, 0, sizeof (IFX_TAPI_TONE_PLAY_t));
         ret = SVIP_RM_LineIdRUSigGenGet(pPhone->lineID, &RU);
         if (ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_RM_LineIdRUSigGenGet returned %d (File: %s, line: %d)\n",
               ret, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         /* siggen not allocated ? */
         if (RU.devType == IFX_TAPI_DEV_TYPE_NONE)
         {
            /* Y */
            resources.nType = RES_GEN;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            if (Common_ResourcesGet(pPhone, &resources) != IFX_SUCCESS)
               return IFX_ERROR;
            ret = SVIP_RM_LineIdRUSigGenGet(pPhone->lineID, &RU);
         }

         if (ret != SVIP_RM_Success)
         {
            return IFX_ERROR;
         }

         pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
         if (pBoard == IFX_NULL)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Invalid device type (File: %s, line: %d)\n",
                __FILE__, __LINE__));
            return IFX_ERROR;
         }
         aTone.dev = RU.nDev;
         aTone.module = RU.module;
         aTone.ch = RU.nCh;
         aTone.index = i;
         aTone.external = 1;
         aTone.internal = 1;
         /* Run ioctl */
         TD_IOCTL(pPhone->nDataCh_FD, IFX_TAPI_TONE_PLAY, &aTone,
                  nDevTmp, pPhone->nSeqConnId);
#else /* SVIP */
#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Play CPT %d using ioctl (IFX_TAPI_TONE_LOCAL_PLAY).\n", i));
         nTone = i;
         TD_IOCTL(pPhone->nDataCh_FD, IFX_TAPI_TONE_LOCAL_PLAY, nTone,
                  nDevTmp, pPhone->nSeqConnId);
#endif /* TAPI_VERSION4 */
#endif /* SVIP */
      }
#ifdef TD_DECT
      else if (nAction == 4400)
      {
         bFeatureExist = IFX_TRUE;
         /* Start registration of DECT phone, this functionality can
            be used instead of pressing page key on board */
         TD_DECT_StartRegistration();
      }
#endif
      else if ( (nAction >= 4410) && (nAction <= 4420) )
      {
         bFeatureExist = IFX_TRUE;
         /* Set CID standard. */
         COM_CID_STANDART_SET(pCtrl, nAction - 4410, ret);
      }
   }
   else if ((nAction >= 8000) && (nAction <= 8999))
   {
      if ((nAction >= 8000) && (nAction < 8010))
      {
         /* get new debug level */
         i = nAction%10;
         /* check if valid value */
         if ((i > DBG_LEVEL_OFF) || (i < DBG_LEVEL_LOW))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Wrong parameter of change debug level DTMF command "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
         }
         else
         {
            /* feature exists */
            bFeatureExist = IFX_TRUE;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                  ("Feature DTMF code: *%d - Change Debug Trace Level.\n",
                   nAction));
            /* change trace level */
            TD_OS_PRN_USR_LEVEL_SET(TAPIDEMO, i);
#ifdef EASY336
            SVIP_RM_SetTraceLevel(i);
#endif
         }

      }
#ifdef TD_DECT
      else if (nAction == 8010)
      {
         bFeatureExist = IFX_TRUE;
         /* Start registration of DECT phone, this functionality can
            be used instead of pressing page key on board.
            Created to simplify DECT handset registration.
            Should be used only for development. Still for end tests page key
            must be used. */
         TD_DECT_StartRegistration();
      }
#endif /* TD_DECT */
#ifdef EASY336
      else if ((nAction >= 8020) && (nAction < 8030))
      {
         /* get new RM debug level */
         i = nAction%10;
         /* check if valid value */
         if ((i > DBG_LEVEL_OFF) || (i < DBG_LEVEL_LOW))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Wrong parameter of change RM debug level DTMF command "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
         }
         else
         {
            /* feature exists */
            bFeatureExist = IFX_TRUE;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                  ("Feature DTMF code: *%d - Change RM Debug Trace Level.\n",
                   nAction));
            SVIP_RM_SetTraceLevel(i);
         }

      }
#endif /* EASY336 */
#ifdef TD_IPV6_SUPPORT
      else if(nAction == 8060)
      {
         /* print phone book */
         bFeatureExist = IFX_TRUE;
         Common_PhonebookShow(pCtrl->rgoPhonebook);
      }
      else if(nAction == 8061)
      {
         /* add addresses from file */
         bFeatureExist = IFX_TRUE;
         Common_PhonebookGetFromFile(pCtrl);
      }
      else if(nAction == 8062)
      {
         /* send multicast and broadcast */
         bFeatureExist = IFX_TRUE;
         Common_PhonebookSendEntry(pCtrl, IE_PHONEBOOK_UPDATE, IFX_NULL);
      }
#endif /* TD_IPV6_SUPPORT */
      else if ((nAction >= 8100) && (nAction <= 8299))
      {
         /* feature exists */
         bFeatureExist = IFX_TRUE;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Feature DTMF code: *%d - %s JB statistics.\n",
                 nAction, (8200 > nAction) ? "Get" : "Reset"));
         i = nAction % 100;
         if (pBoard->nMaxCoderCh < i)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("There in no data channel %d. (File: %s, line: %d)\n",
                   i, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         if (8200 > nAction)
         {
            Common_JBStatistic(i, pPhone->nDev, IFX_FALSE, IFX_TRUE, pBoard);
         }
         else
         {
            Common_JBStatistic(i, pPhone->nDev, IFX_TRUE, IFX_FALSE, pBoard);
         }
      }
   }
   else if ((nAction >= 9000) && (nAction <= 9999))
   {
#if defined(EASY336) || defined(XT16)
      /* check if single connection and Voip ID is assigned */
      if ((pPhone->nVoipIDs <= 0) ||
          (NO_CONFERENCE != pPhone->nConfIdx) ||
          (0 >= pPhone->nConnCnt))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d: Configure NarrowBand and WideBand "
                "can be only done during call that uses codecs. "
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         return IFX_ERROR;
      }
#endif /* defined(EASY336) || defined(XT16) */
      /* Go through all conections of this phone and send them
      to all the parties to set this feature also. */
      for (i = 0; i < pPhone->nConnCnt; i++)
      {
         if (i < MAX_PEERS_IN_CONF)
         {
            pConn = &pPhone->rgoConn[i];
            if ((pConn != IFX_NULL) &&
                (CALL_DIRECTION_TX == pPhone->nCallDirection))
            {
               pConn->nFeatureID = nAction;
               ret = TAPIDEMO_SetAction(pCtrl, pPhone, pConn,
                                        IE_CONFIG_PEER, pPhone->nSeqConnId);
            }
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Invalid number of connections - %d. "
                   "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
      switch (nAction)
      {
         case 9000:
            {
               /* feature exists */
               bFeatureExist = IFX_TRUE;
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                     ("Feature DTMF code: *%d - "
                      "Set speech range for both peers.\n", nAction));
               /* Turn this channel to narrowband, coder is set to G.711 */
               ret = FEATURE_SetSpeechRange(pPhone, nDataCh_FD,
                                            IFX_TAPI_LINE_TYPE_FXS_NB,
                                            IFX_TAPI_COD_TYPE_MLAW,
                                            IFX_FALSE);

               if (ret != IFX_SUCCESS)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err, configure NarrowBand. "
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  return IFX_ERROR;
               }
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Switch all parties to NARROWBAND\n"));

#if defined(HAVE_DRV_VMMC_HEADERS) || defined(EASY3201) || defined(EASY3201_EVS)
/* play tone to indicate change of setting */
#ifdef TAPI_VERSION4
               if (pBoard->fSingleFD)
               {
                  Common_TonePlay(pCtrl, pPhone, TD_ITN_SET_BAND_NARROW);
               } /* if (pBoard->fSingleFD) */
               else
#endif /* TAPI_VERSION4 */
               {
                  TD_IOCTL(nDataCh_FD, IFX_TAPI_TONE_LOCAL_PLAY,
                           TD_ITN_SET_BAND_NARROW,
                           nDevTmp, pPhone->nSeqConnId);
               } /* if (pBoard->fSingleFD) */
#endif /* defined(HAVE_DRV_VMMC_HEADERS) || defined(EASY3201) */

               pPhone->fWideBand_Cfg = IFX_FALSE;
            }
            break;
         case 9001:
            {
               /* feature exists */
               bFeatureExist = IFX_TRUE;
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                     ("Feature DTMF code: *%d - "
                      "Set speech range for both peers.\n", nAction));
               /* Turn this channel to wideband,
                  coder is set to G.722 (64k)*/
               ret = FEATURE_SetSpeechRange(pPhone, nDataCh_FD,
                                            IFX_TAPI_LINE_TYPE_FXS_WB,
                                            IFX_TAPI_COD_TYPE_G722_64,
                                            IFX_TRUE);

               if (ret != IFX_SUCCESS)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err, configure WideBand. "
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  return IFX_ERROR;
               }
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Switch all parties to WIDEBAND\n"));

#if defined(HAVE_DRV_VMMC_HEADERS) || defined(EASY3201) || defined(EASY3201_EVS)
/* play tone to indicate change of setting */
#ifdef TAPI_VERSION4
               if (pBoard->fSingleFD)
               {
                  Common_TonePlay(pCtrl, pPhone, TD_ITN_SET_BAND_WIDE);
               } /* if (pBoard->fSingleFD) */
               else
#endif /* TAPI_VERSION4 */
               {
                  TD_IOCTL(nDataCh_FD, IFX_TAPI_TONE_LOCAL_PLAY,
                           TD_ITN_SET_BAND_WIDE,
                           nDevTmp, pPhone->nSeqConnId);
               } /* if (pBoard->fSingleFD) */
#endif /* defined(HAVE_DRV_VMMC_HEADERS) || defined(EASY3201) */

               pPhone->fWideBand_Cfg = IFX_TRUE;
            }
            break;
         case 9010:
         case 9011:
            /* feature exists */
            bFeatureExist = IFX_TRUE;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Feature DTMF code: *%d - "
                "Set PCM speech range for both peers.\n", nAction));
            /* must be Duslic-XT */
            if (pPhone->pBoard->nType == TYPE_DUSLIC_XT)
            {
               /* must be local PCM call */
               if (pPhone->rgoConn[0].nType == LOCAL_PCM_CALL)
               {
                  if (9010 == nAction)
                  {
                     /* Set NarrowBand */
                      ret = FEATURE_SetPCM_SpeechRange(pPhone, IFX_FALSE);
                  } /* else if (9010 == nAction) */
                  else if (9011 == nAction)
                  {
                     /* Set WideBand */
                     ret = FEATURE_SetPCM_SpeechRange(pPhone, IFX_TRUE);
                  } /* else if (9011 == nAction) */
                  else
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err, Common_ConfigureTAPI(): there is no option %d. "
                         "(File: %s, line: %d)\n",
                         nAction, __FILE__, __LINE__));
                     return IFX_ERROR;
                  } /* else */
               } /* if local PCM call */
            } /* if Duslic-XT */
            break;
         default:
            ret = IFX_ERROR;
            break;
      }
   }
   else
   {
      ret = IFX_ERROR;
   }
   /* check if selected feature exists */
   if (IFX_FALSE == bFeatureExist)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("No feature for DTMF code: *%d.\n", nAction));
   }

   if (IFX_TRUE == pCtrl->oITM_Ctrl.fITM_Enabled)
   {
      COM_EXECUTE_DTMF_COMMAND(pBoard, nAction, ret, nSeqConnId);
   }

   return ret;
} /* Common_ConfigureTAPI() */

/**
   Make ip address from phone number.

   \param pCtrl   - handle to control structure
   \param pPhone  - pointer to PHONE
   \param pConn    - pointer to phone connection
   \param nIP_Addr - ip address
   \param fIsPCM_Num - IFX_TRUE we have phone number of PCM phone,
                       otherwise normal phone number.

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)

   \remark This address will be used to call the external phone.
*/
IFX_return_t Common_FillAddrByNumber(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn,
                                     IFX_uint32_t nIP_Addr,
                                     IFX_boolean_t fIsPCM_Num)
{
#if (defined(VXWORKS) || defined(LINUX))
   IFX_int32_t ip = 0;
   struct in_addr tmp_ip_addr = {0}, ip_addr = {0};

   /* check input parameter */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Makes addres between network address (xxx.xxx.xxx.0)
      and local host address (0.0.0.xxx). In VxWorks we must remove last
      number, otherwise will not make address. */

   tmp_ip_addr = inet_makeaddr((pCtrl->nMy_IPv4_Addr >> 8), 0);

   /* Calling boards from different network */
   if (pCtrl->pProgramArg->oArgFlags.nDiffNet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Calling board on different network.\n",
             pPhone->nPhoneNumber));
      tmp_ip_addr.s_addr = pCtrl->pProgramArg->oDiffNetIP.s_addr;
   }

   /* set ip */
   if (nIP_Addr <= 0xff)
   {
      /* set low part of IP address */
      ip = nIP_Addr;
   }
   else
   {
      /* set low part of IP address - only last bit is used */
      ip = TD_IP_LOWEST_BYTE_MASK & nIP_Addr;
      /* set high part of IP address - IP address was specified
         with dialing sequence */
      tmp_ip_addr = inet_makeaddr((nIP_Addr >> 8), 0);
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: IP low part = %u, netmask %s\n",
          pPhone->nPhoneNumber, nIP_Addr, inet_ntoa(tmp_ip_addr)));


   /* Check if ip is valid */
   if ((0 > ip) || (255 < ip))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid IP-number %d. (File: %s, line: %d)\n",
             (int)ip, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (IFX_TRUE == fIsPCM_Num)
   {
      memset(&pConn->oConnPeer.oPCM.oToAddr, 0,
             sizeof(pConn->oConnPeer.oPCM.oToAddr));
      TD_SOCK_FamilySet(&pConn->oConnPeer.oPCM.oToAddr, AF_INET);
      TD_SOCK_PortSet(&pConn->oConnPeer.oPCM.oToAddr, ADMIN_UDP_PORT);

#if defined(VXWORKS)
       /* Only for VxWorks inet_makeaddr
           1st Parameter is Network Part, Ex: 10.1.1.0 should be used
           0x000A0101
           2nd Parameter is Host Part, depend on how many bits have been used
              by Network Part
           In Our Case, we always regard the mask is 255.255.255.0, so
              shift right 8 bits
       */
      ip_addr = inet_makeaddr((ntohl(tmp_ip_addr.s_addr) >> 8), ip);
      TD_SOCK_AddrIPv4Set(&pConn->oConnPeer.oPCM.oToAddr, ip_addr.s_addr);
#else /* VXWORKS */
      ip_addr = inet_makeaddr((ntohl(tmp_ip_addr.s_addr) >> 8), ip);
      TD_SOCK_AddrIPv4Set(&pConn->oConnPeer.oPCM.oToAddr, ip_addr.s_addr);
#endif /* VXWORKS */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Peer address [%s]:%d\n",
             pPhone->nPhoneNumber,
             TD_GetStringIP(&pConn->oConnPeer.oPCM.oToAddr),
             TD_SOCK_PortGet(&pConn->oConnPeer.oPCM.oToAddr)));
   }
   else
   {
      memset(&pConn->oConnPeer.oRemote.oToAddr, 0,
             sizeof(pConn->oConnPeer.oRemote.oToAddr));
      TD_SOCK_FamilySet(&pConn->oConnPeer.oRemote.oToAddr, AF_INET);

#if defined(VXWORKS)
          /* Only for VxWorks inet_makeaddr
              1st Parameter is Network Part, Ex: 10.1.1.0 should be used
              0x000A0101
              2nd Parameter is Host Part, depend on how mane bits have been used
                 by Network Part
              In Our Case, we always regard the mask is 255.255.255.0, so
                 shift right 8 bits
          */
      ip_addr = inet_makeaddr((ntohl(tmp_ip_addr.s_addr) >> 8), ip);
      TD_SOCK_AddrIPv4Set(&pConn->oConnPeer.oRemote.oToAddr, ip_addr.s_addr);
#else /* VXWORKS */
      ip_addr = inet_makeaddr((ntohl(tmp_ip_addr.s_addr) >> 8), ip);
      TD_SOCK_AddrIPv4Set(&pConn->oConnPeer.oRemote.oToAddr, ip_addr.s_addr);
#endif /* VXWORKS */

      if (pCtrl->pProgramArg->oArgFlags.nQos)
      {
         /* #warning QoS support - alos done later in state transition
            - after some tests should be removed from here,
            left here because of urgent delivery */
         QOS_InitializePairStruct(pCtrl, pConn, pPhone->pBoard->nBoard_IDX);
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Peer address = %s\n", pPhone->nPhoneNumber,
             TD_GetStringIP(&pConn->oConnPeer.oRemote.oToAddr)));
   }

   return IFX_SUCCESS;
#else
   /* Function to set default IP address must be specific for every system.
      Currently, Tapidemo supports only Linux and WxWorks. In case of use
      diffrent system, please add implementation */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Err, Unsupported function TAPIDEMO_SetDefaultAddr()\n"));
   return IFX_ERROR;
#endif /* #if (defined(VXWORKS) || defined(LINUX)) */
} /* Common_FillAddrByNumber() */


/**
   Converts digits to number, changes digits to chars

   \param prgnDialNum - pointer to array of digits
   \param nFirstDigit - first digit to make number from
   \param nDialNrCnt  - number of digits
   \param nSeqConnId  - Seq Conn ID

   \return Dialed number (if 0 error)
*/
IFX_int32_t Common_DigitsToNum(IFX_char_t* prgnDialNum, IFX_int32_t nFirstDigit,
                               IFX_int32_t nDialNrCnt, IFX_uint32_t nSeqConnId)
{
   const IFX_int32_t digit2number[32] = {
       -1,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1,  0, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

   const IFX_char_t digit2char[32] = {
      ' ', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', '*', '0', '#', ' ', ' ', ' ',
      ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
      ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

   IFX_int32_t i = 0;
   IFX_int32_t dialed_number = 0;

   /* if converting more than 9 chars to number then number
      could be too big for IFX_int32_t */
   TD_PTR_CHECK(prgnDialNum, dialed_number);
   TD_PARAMETER_CHECK((0 > nDialNrCnt), nDialNrCnt, dialed_number);
   TD_PARAMETER_CHECK((9 < (nDialNrCnt - nFirstDigit)),
                      (nDialNrCnt - nFirstDigit), dialed_number);
   TD_PARAMETER_CHECK((MAX_DIALED_NUM <= nDialNrCnt), nDialNrCnt, dialed_number);

   for (i = 0; i < nDialNrCnt; i++)
   {
      if ((0 > (IFX_int8_t) prgnDialNum[i]) ||
          (32 < (IFX_int8_t) prgnDialNum[i]))
      {
         /* Error, our digit outside conversion table */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, Digit %d outside transform table. (File: %s, line: %d)\n",
                prgnDialNum[i], __FILE__, __LINE__));
         return dialed_number;
      }
      /* check if first digit of number occured */
      if (i >= nFirstDigit)
      {
         /* get number */
         dialed_number = dialed_number * 10 +
            digit2number[(int) prgnDialNum[i]];
      }
      /* change digit number to char */
      prgnDialNum[i] = digit2char[(int) prgnDialNum[i]];
   }

   prgnDialNum[nDialNrCnt] = 0;
   return dialed_number;
} /* Common_DigitsToNum() */

/**
   Check dialed number for IP address if dialing started with "0*" sequence.

   \param prgnDialNum - array of dialed digits
   \param nDialNrCnt  - number of digits
   \param nIpAddr     - ip address, used with sockets
   \param nSeqConnId  - Seq Conn ID

   \return UNKNOWN_CALL_TYPE if wrong number or EXTERN_VOIP_CALL if dialing
           is completed.
*/
CALL_TYPE_t Common_CheckDialedNumForFull_IP(IFX_char_t* prgnDialNum,
                                            IFX_int32_t nDialNrCnt,
                                            IFX_uint32_t* nIpAddr,
                                            IFX_uint32_t nSeqConnId)
{
   IFX_int32_t i, j = 0, nNumber;
   IFX_int32_t prgnStarLocation[NUM_OF_STARS_IN_IP];
   IFX_int32_t prgnIP_address[NUM_OF_STARS_IN_IP];

   /* array to convert dialed digits to numbers */
   const IFX_int32_t digit2number[32] = {
       -1,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1,  0, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

   /* check all dialed digits */
   for (i=0; i<nDialNrCnt; i++)
   {
      /* check if dialed digit is a star digit - '*' */
      if (DIGIT_STAR == prgnDialNum[i])
      {
         /* check if not too many star digits found */
         if (NUM_OF_STARS_IN_IP > j)
         {
            /* save star digit location */
            prgnStarLocation[j] = i;
         }
         /* increas star digit count */
         j++;
      }
   }
   /* clear table */
   memset(prgnIP_address, 0, NUM_OF_STARS_IN_IP * sizeof(IFX_int32_t));
   /* check if right amount of star digits was found and if phone number has
      right length */
   if ((NUM_OF_STARS_IN_IP == j) &&
       ((prgnStarLocation[NUM_OF_STARS_IN_IP - 1] + 1) ==
        nDialNrCnt))
   {
      /* set star digit counter */
      j = 0;
      for (i=2; i<nDialNrCnt; i++)
      {
         if (32 <= (IFX_int32_t) prgnDialNum[i])
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, Invalid digit number %d. (File: %s, line: %d)\n",
                   i, __FILE__, __LINE__));
            return UNKNOWN_CALL_TYPE;
         }
         /* get number of one dialed digit */
         nNumber = digit2number[(IFX_int32_t)prgnDialNum[i]];
         /* TAPIDEMO_DigitsToNum() should return -1 for star digit */
         if (-1 == nNumber)
         {
            /* check if star digit was detected in this loaction */
            if (NUM_OF_STARS_IN_IP > (j+1))
            {
               if ((j >= NUM_OF_STARS_IN_IP) ||
                   (i != prgnStarLocation[j+1]))
               {
                  /* change dialed digits table to printable string */
                  Common_DigitsToNum(prgnDialNum, nDialNrCnt, nDialNrCnt,
                                     nSeqConnId);
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                        ("Err, Dialed number \"%s\" is invalid."
                         " (File: %s, line: %d)\n",
                         prgnDialNum, __FILE__, __LINE__));
                  return UNKNOWN_CALL_TYPE;
               }
               /* increase star digit counter */
               j++;
            }
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                     ("Err, Invalid index of array 'prgnStarLocation'. "
                      "(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
         } /* if (-1 == nNumber) */
         else
         {
            /* get IP address, last element of table is phone number */
            prgnIP_address[j] = prgnIP_address[j] * 10 + nNumber;
         } /* if (-1 == nNumber) */
      } /* for (i=2; i<nDialNrCnt; i++) */
   } /* if valid number */
   else
   {
      return UNKNOWN_CALL_TYPE;
   }
   /* reset IP address */
   *nIpAddr = 0;
   /* get IP address */
   for (j=0; j<4; j++)
   {
      /* check if valid part of IP address */
      if (0xFF < prgnIP_address[j])
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, Dialed number \"%s\" is invalid - wrong IP address!"
                " (File: %s, line: %d)\n",
                prgnDialNum, __FILE__, __LINE__));
         return UNKNOWN_CALL_TYPE;
      }
      /* add next part of ip address */
      *nIpAddr = *nIpAddr * 0x100 + prgnIP_address[j];
   }
   /* change dialed digits table to printable string */
   Common_DigitsToNum(prgnDialNum, nDialNrCnt, nDialNrCnt, nSeqConnId);

   /* making extern voip call */
   return EXTERN_VOIP_CALL;
} /* Common_CheckDialedNumForFull_IP() */

/**
   Check dialed number.

   \param pPhone        - pointer to phone
   \param nDialedNum    - dialed number
   \param nIpAddr       - ip address, used with sockets
   \param pCalledIP     - called board's IP address (as a string)
   \param pCtrl         - handle to control status structure

   \return UNKNOWN_CALL_TYPE if wrong number or call type and dialed number
           and also channel number if phone is called.
   \remarks nDevNum is used only for TAPI_VERSION4
*/
#define MAX_NUMBER_TO_DIAL_L 24
#define MAX_NUMBER_TO_DIAL_H 26
CALL_TYPE_t Common_CheckDialedNum(PHONE_t* pPhone,
                                  IFX_int32_t* nDialedNum,
                                  IFX_uint32_t* nIpAddr,
                                  IFX_char_t* pCalledIP,
                                  CTRL_STATUS_t* pCtrl)
{
   TD_PTR_CHECK(pPhone, UNKNOWN_CALL_TYPE);
   TD_PTR_CHECK(nDialedNum, UNKNOWN_CALL_TYPE);
   TD_PTR_CHECK(nIpAddr, UNKNOWN_CALL_TYPE);
   TD_PTR_CHECK(pCtrl, UNKNOWN_CALL_TYPE);
   TD_PARAMETER_CHECK((0 > pPhone->nDialNrCnt), pPhone->nDialNrCnt,
                      UNKNOWN_CALL_TYPE);

#if (!defined(EASY336) && !defined(XT16))
   /* 0x0b = '0' => DIGIT_ZERO */
   /* LOCAL CALL */
   if (( MAX_NUMBER_TO_DIAL_L<= pPhone->nDialNrCnt) && (MAX_NUMBER_TO_DIAL_H >= pPhone->nDialNrCnt) /*&&
       (DIGIT_ZERO != pPhone->pDialedNum[0])*/ &&
       (DIGIT_STAR != pPhone->pDialedNum[0]))
   {
      /* Number <xx>; but first must not be 0. Phone channel counting
         from 0, but we count from 1. */
      /* If the first digit is a star - DTMF command is requested*/
      *nDialedNum = Common_DigitsToNum(pPhone->pDialedNum, 0,
                       pPhone->nDialNrCnt, pPhone->nSeqConnId);
      if (pCtrl->nSumPhones < *nDialedNum)
      {
         /* Calling unknown phone channel */
         /* If it's not a test of digits... */
         if ( COM_MOD_DIGIT_IS_NOT_ON )
         {
            /* ...raport error */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Invalid number entered (%d), "
                   "phone chanel does not exists.\n",
                   (int) *nDialedNum));
         }
         return UNKNOWN_CALL_TYPE;
      }
      else
      {
         return LOCAL_CALL;
      }
   }
#else /* (!defined(EASY336) && !defined(XT16)) */
   /* LOCAL CALL */
   if ((pPhone->nDialNrCnt == 3) &&
       (DIGIT_ZERO != pPhone->pDialedNum[0]) &&
       (DIGIT_STAR != pPhone->pDialedNum[0]))
   {
      /* Number <xxx>; but first must not be 0. Device and phone channel
       * counting from 0, but we count from 1. */
      /* If the first digit is a star - DTMF command is requested*/
      *nDialedNum = Common_DigitsToNum(pPhone->pDialedNum, 0,
                       pPhone->nDialNrCnt, pPhone->nSeqConnId);

      /* If it's a test of digits... */
      COM_MOD_DIGIT_RETURN_UNKNOWN_CALL_TYPE_DURING_TEST(0);

      return LOCAL_CALL;
   }
#endif /* (!defined(EASY336) && !defined(XT16)) */

   /* EXTERN_VOIP_CALL */
   if ( (4 == pPhone->nDialNrCnt) &&
        (DIGIT_ZERO == pPhone->pDialedNum[0]) &&
        ( (DIGIT_ZERO == pPhone->pDialedNum[1]) ||
          (DIGIT_ONE == pPhone->pDialedNum[1]) ||
          (DIGIT_TWO == pPhone->pDialedNum[1]) ) )
   {
      *nDialedNum = Common_DigitsToNum(pPhone->pDialedNum, 1,
                       pPhone->nDialNrCnt, pPhone->nSeqConnId);
      *nIpAddr = *nDialedNum;

      return EXTERN_VOIP_CALL;
   }

#ifdef TD_IPV6_SUPPORT
   /* EXTERN_VOIP_CALL using phonebook*/
   if ( (4 == pPhone->nDialNrCnt) &&
        (DIGIT_ZERO == pPhone->pDialedNum[0]) &&
        (DIGIT_SIX == pPhone->pDialedNum[1]))
   {

      *nDialedNum = Common_DigitsToNum(pPhone->pDialedNum, 0,
                       pPhone->nDialNrCnt, pPhone->nSeqConnId);

      if (IFX_SUCCESS !=
          Common_PhonebookGetIpAddr(pCtrl, pPhone->pDialedNum, pCalledIP))
      {
         /* not available in phone book */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Unable to get address for phonebook index %02d.\n",
                ((int) *nDialedNum) % TD_PHONEBOOK_SIZE));
         strcpy(pCalledIP, TD_NO_NAME);
      }
      return EXTERN_VOIP_CALL;
   }
#endif /* TD_IPV6_SUPPORT */

   /* EXTERN_VOIP_CALL with whole ip address */
   if ((DIGIT_ZERO == pPhone->pDialedNum[0]) &&
       (DIGIT_STAR == pPhone->pDialedNum[1]))
   {
      /* Dial 0*<aaa>*<bbb>*<ccc>*<ddd>*xxx  to
         call ip address aaa.bbb.ccc.ddd, phone number xxx */
      if (EXTERN_VOIP_CALL == Common_CheckDialedNumForFull_IP(
         pPhone->pDialedNum, pPhone->nDialNrCnt, nIpAddr, pPhone->nSeqConnId))
      {
         return EXTERN_VOIP_CALL;
      }
   }

   /* check if PCM CALL is supported */
   if ((2 == pPhone->nDialNrCnt) &&
       (DIGIT_ZERO == pPhone->pDialedNum[0]) &&
       (DIGIT_NINE == pPhone->pDialedNum[1]))
   {
      if (pPhone->pBoard->nMaxPCM_Ch <= 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: %s: PCM not supported.\n",
                pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));
         return NOT_SUPPORTED;
      }
   }

#if (!defined(EASY336) && !defined(XT16))
   /* PCM CALL */
   if ((5 == pPhone->nDialNrCnt) &&
       (DIGIT_ZERO == pPhone->pDialedNum[0]) &&
       (DIGIT_NINE == pPhone->pDialedNum[1]))
   {
      if ((pCtrl->pProgramArg->oArgFlags.nPCM_Master) ||
          (pCtrl->pProgramArg->oArgFlags.nPCM_Slave))
      {
         /* Number 09<xxx><yy>; xxx - 0..255 (ip addr), yy - pcm channel.
            Phone channel counting from 0, but we type from 1. */
         *nDialedNum = Common_DigitsToNum(pPhone->pDialedNum, 2,
                          pPhone->nDialNrCnt, pPhone->nSeqConnId);
         *nIpAddr = *nDialedNum;

         return PCM_CALL;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: %s: Unable to start PCM call.\n",
                pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));
      }
   }
   /* FXO CALL */
   if ((3 == pPhone->nDialNrCnt) &&
       (DIGIT_ZERO == pPhone->pDialedNum[0]) &&
       (DIGIT_FIVE == pPhone->pDialedNum[1]))
   {
      /* FXO call. */
      /* Number 05<x>; x - 1..9 - number of FXO. */
      *nDialedNum = Common_DigitsToNum(pPhone->pDialedNum, 2,
                       pPhone->nDialNrCnt, pPhone->nSeqConnId);

      return FXO_CALL;
   }
#else /* (!defined(EASY336) && !defined(XT16)) */
   /* PCM CALL */
   if ((pPhone->nDialNrCnt == 5) &&
       (DIGIT_ZERO == pPhone->pDialedNum[0]) &&
       (DIGIT_NINE == pPhone->pDialedNum[1]))
   {
      if ((pCtrl->pProgramArg->oArgFlags.nPCM_Master) ||
          (pCtrl->pProgramArg->oArgFlags.nPCM_Slave))
      {
        /* Number 09<xxx><yzz>; xxx - 0..255 (ip addr), y - device number,
            zz - phone channel number. Device and
            phone channel counting from 0, but we type from 1.
            Example.
            09<112><104>
            IP = xxx.xxx.xxx.112
            Device number = 0
            Channel number = 3 */
         *nDialedNum = Common_DigitsToNum(pPhone->pDialedNum, 2,
                          pPhone->nDialNrCnt, pPhone->nSeqConnId);

         *nIpAddr = *nDialedNum;

         return PCM_CALL;
      }
   }
#endif /* (!defined(EASY336) && !defined(XT16)) */
   /* DTMF COMMAND */
   if ((5 == pPhone->nDialNrCnt) &&
       (DIGIT_STAR == pPhone->pDialedNum[0]))
   {
       /* If the first digit is a star and 4 digits were pressed
          - DTMF command is requested*/
       *nDialedNum = Common_DigitsToNum(pPhone->pDialedNum, 1,
                        pPhone->nDialNrCnt, pPhone->nSeqConnId);
       return DTMF_COMMAND;
   }
   return UNKNOWN_CALL_TYPE;
} /* Common_CheckDialedNum() */

/**
   Print call data, like start and end of connection and call type.

   \param current_state   - current state of phone
   \param new_event  - new internal event of phone
   \param pPhone    - pointer to phone structure
   \param pConn    - pointer to phone connection
   \param nUseCodersForLocal - IFX_TRUE if codecs are used for local connection

   \return none
*/
IFX_void_t Common_PrintCallProgress(STATE_MACHINE_STATES_t current_state,
                                    INTERNAL_EVENTS_t new_event,
                                    CALL_TYPE_t nOldCallType,
                                    PHONE_t* pPhone, CONNECTION_t* pConn,
                                    IFX_int32_t nUseCodersForLocal)
{
   IFX_char_t call_string[PCP_MAX_STR_LEN];
   IFX_char_t phone_string[PCP_MAX_STR_LEN];

   CALL_TYPE_t nCallType;
   IFX_return_t ret = IFX_SUCCESS;

   /* set action according to state transition */
   switch (current_state)
   {
      case S_IN_RINGING:
         if (IE_HOOKOFF == new_event)
         {
            strcpy(call_string,
                   "Phone No %d: Established %s with calling %s\n");
            break;
         }
         ret = IFX_ERROR;
         break;
      case S_ACTIVE:
         if (IE_END_CONNECTION == new_event ||
             IE_HOOKON == new_event)
         {
            strcpy(call_string,
                   "Phone No %d: Disconnected %s with %s\n");
            break;
         }
         ret = IFX_ERROR;
      case S_OUT_RINGBACK:
         if (IE_ACTIVE == new_event)
         {
            strcpy(call_string,
                   "Phone No %d: Established %s with called %s\n");
            break;
         }
         ret = IFX_ERROR;
         break;
      default :
         ret = IFX_ERROR;
         break;
   }

   /* if none of supported state transitions then leave function */
   if (IFX_ERROR == ret)
   {
      return;
   }
   /* check if call type is set */
   if ((UNKNOWN_CALL_TYPE == pConn->nType) && (pConn->nType != nOldCallType))
   {
      /* if pConn->nType was resetted on connection end,
         use remembered call type */
      nCallType = nOldCallType;
   }
   else
   {
      nCallType = pConn->nType;
   }

   /* set phones data depending on call type */
   switch (nCallType)
   {
      case EXTERN_VOIP_CALL:
         sprintf(phone_string, "PhoneNo %d\n%son %s",
                 pConn->oConnPeer.nPhoneNum,
                 pPhone->pBoard->pIndentPhone,
                 TD_GetStringIP(&pConn->oConnPeer.oRemote.oToAddr)
                 );
         break;
      case LOCAL_CALL:
      case LOCAL_BOARD_CALL:
         if (IFX_FALSE == nUseCodersForLocal)
         {
            sprintf(phone_string, "PhoneNo %d, PhoneCh %d",
                    pConn->oConnPeer.nPhoneNum,
                    pConn->oConnPeer.oLocal.pPhone->nPhoneCh
                    );
         }
         else
         {
            sprintf(phone_string, "PhoneNo %d, DataCh %d",
                    pConn->oConnPeer.nPhoneNum,
                    pConn->oConnPeer.oLocal.pPhone->nDataCh
                    );
         }
         break;
      case PCM_CALL:
         sprintf(phone_string, "PhoneNo %d, on %s",
                 pConn->oConnPeer.nPhoneNum,
                 TD_GetStringIP(&pConn->oConnPeer.oPCM.oToAddr)
                 );
         break;
      case LOCAL_PCM_CALL:
      case LOCAL_BOARD_PCM_CALL:
         sprintf(phone_string, "PhoneNo %d DataCh %d",
                 pConn->oConnPeer.nPhoneNum,
                 pConn->oConnPeer.oLocal.pPhone->nDataCh
                 );
         break;
      default:
         ret = IFX_ERROR;
         break;
   }

   /* if none of supported call types then leave function */
   if (IFX_ERROR == ret)
   {
      return;
   }
   /* print connection call progress data */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
         (call_string, pPhone->nPhoneNumber,
          Common_Enum2Name(nCallType, TD_rgCallTypeName),
          phone_string));
   return;

} /* Common_PrintCallProgres() */

/**
   Print encoder type and packets length.

   \param pCtrl - pointer to cotrol structure
   \param pPhone - pointer to phone structure
   \param pConn - pointer to connection structure

   \return none
*/
IFX_void_t Common_PrintEncoderType(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn)
{

   IFX_TAPI_ENC_TYPE_t nCodecType = IFX_TAPI_ENC_TYPE_MAX;
   IFX_int32_t nFrameLen = 0;
#ifdef TAPI_VERSION4
#ifdef EASY336
   VOIP_DATA_CH_t *pOurCodec;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
   BOARD_t *pUsedboard;

   /* check input */
   TD_PTR_CHECK(pCtrl,);
   TD_PTR_CHECK(pPhone,);
   TD_PTR_CHECK(pConn,);

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      /* get codec data structure */
      pOurCodec = Common_GetCodec(pCtrl, pConn->voipID, pPhone->nSeqConnId);
      if (pOurCodec != IFX_NULL)
      {
         /* set codec type and packets length */
         nCodecType = pOurCodec->nCodecType;
         nFrameLen = pOurCodec->nFrameLen;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Failed to get codec. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
#endif /* EASY336 */
   }
   else
#endif /* TAPI_VERSION4 */
   {
#ifdef EASY3111
      if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
      {
         pUsedboard = TD_GET_MAIN_BOARD(pCtrl);
      }
      else
#endif /* EASY3111 */
      {
         pUsedboard = pPhone->pBoard;
      }
      /* set codec type and packets length */
      nCodecType = pUsedboard->pDataChStat[pConn->nUsedCh].nCodecType;
      nFrameLen = pUsedboard->pDataChStat[pConn->nUsedCh].nFrameLen;
   } /* if (pPhone->pBoard->fSingleFD) */
   /* check if codec data obtained */
   if (IFX_TAPI_ENC_TYPE_MAX > nCodecType)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Voice encoding: %s, %s\n",
             pPhone->nPhoneNumber, pCodecName[nCodecType],
             oFrameLen[nFrameLen]));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to get codec name. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
   }

   return;
} /* Common_PrintEncoderType() */

/**
   Function to show and reset jitter buffer statistics.

   \param nDataCh - data channel number
   \param nDev    - device number
   \param fReset  - if not IFX_FALSE function resets JB statistics
   \param fShow   - if not IFX_FALSE function prints JB statistics
   \param pBoard  - pointer to board structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
   \remarks nDev is used only for TAPI_VERSION4
*/
IFX_return_t Common_JBStatistic(IFX_int32_t nDataCh,
                                IFX_int32_t nDev,
                                IFX_boolean_t fReset,
                                IFX_boolean_t fShow,
                                BOARD_t* pBoard)
{
   IFX_int32_t ret;
   IFX_int32_t fd;
   IFX_TAPI_JB_STATISTICS_t jbStatistic;
#ifdef TAPI_VERSION4
   IFX_TAPI_JB_STATISTICS_RESET_t jbStatisticReset;
#else
   PHONE_t* pPhone;
   CONNECTION_t* pConn;
#endif /* TAPI_VERSION4 */
   IFX_char_t aDataChannelInfo[TD_MAX_NAME];
   IFX_int32_t param = 0;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   fd = Common_GetFD_OfCh(pBoard, nDataCh);
   if (IFX_ERROR == fd)
   {
      /* Couldn't get file descriptor */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
           ("Err, Couldn't get file descriptor of data channel %d."
            "(File: %s, line: %d)\n",
            nDataCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#ifndef TAPI_VERSION4
   pPhone = ABSTRACT_GetPHONE_OfDataCh(nDataCh, &pConn, pBoard);
   if (IFX_NULL == pPhone)
   {
      /* Couldn't get phone of channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
           ("Err, Data channel %d is not mapped to phone channel. "
            "(File: %s, line: %d)\n",
            nDataCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* TAPI_VERSION4 */
   /* get info string */
#ifndef TAPI_VERSION4
   snprintf(aDataChannelInfo, TD_MAX_NAME, "Phone No %d, data channel %d",
            pPhone->nPhoneNumber, nDataCh);
#else
   snprintf(aDataChannelInfo, TD_MAX_NAME, "data channel %d, device %d",
            nDataCh, nDev);
#endif /* TAPI_VERSION4 */
   if (IFX_FALSE != fShow)
   {

      memset(&jbStatistic, 0, sizeof(IFX_TAPI_JB_STATISTICS_t));
#ifdef TAPI_VERSION4
      /* set device and channel */
      jbStatistic.ch = nDataCh;
      jbStatistic.dev = nDev;
      nDevTmp = jbStatistic.dev;
#endif /* TAPI_VERSION4 */
      ret = TD_IOCTL(fd, IFX_TAPI_JB_STATISTICS_GET, (IFX_int32_t)&jbStatistic,
                     nDevTmp, TD_CONN_ID_DBG);

      if (IFX_SUCCESS == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("Jitter buffer statistics for %s:\n",
                aDataChannelInfo));

         /* jitter buffer type*/
         switch (jbStatistic.nType)
         {
            case 1:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
                     ("  - jitter buffer type fixed,\n"));
               break;
            case 2:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
                     ("  - jitter buffer type adaptive,\n"));
               break;
            default:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
                     ("  - jitter buffer type unknown,\n"));
               break;
         }
         /* current jitter buffer size */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - current jitter buffer size 0x%hx(%uus),\n",
                jbStatistic.nBufSize,
                (jbStatistic.nBufSize * 125)));

         /* minimum and maximum estimated buffer size */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - estimated jitter buffer size minimum 0x%hx(%uus),\n"
                "    maximum 0x%hx(%uus),\n",
                jbStatistic.nMinBufSize,
                (jbStatistic.nMinBufSize * 125),
                jbStatistic.nMaxBufSize,
                (jbStatistic.nMaxBufSize * 125)));

         /* playout delay */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - playout delay %hu,\n",
                jbStatistic.nPODelay));

         /* minimum and maximum playout delay */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - playout delay minimum %hu, maximum %hu,\n",
                jbStatistic.nMinDelay,
                jbStatistic.nMaxPODelay));

         /* received packet number */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - received packet number "TD_JB_DATA_FORMAT",\n",
                jbStatistic.nPackets));

         /*  invalid packet number */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - invalid packet number %hu,\n",
                jbStatistic.nInvalid));

         /*  late packets number */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - late packets number %hu,\n",
                jbStatistic.nLate));

         /*  early packets number */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - early packet number %hu,\n",
                jbStatistic.nEarly));

         /*  resynchronizations number */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - resynchronizations number %hu,\n",
                jbStatistic.nResync));

         /* nIsUnderflow - total number of injected samples since the beginning
            of the connection or since the last statistic reset
            due to jitter buffer underflows.*/

         /* nIsNoUnderflow total number of injected samples since the beginning
            of the connection or since the last statistic reset in case
             of normal jitter buffer operation,
            which means when there is not a jitter buffer underflow. */

         /* nIsIncrement total number of injected samples since the beginning
            of the connection or
            since the last statistic reset in case of jitter buffer
            increments.*/
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - nIsUnderflow "TD_JB_DATA_FORMAT","
                   " nIsNoUnderflow "TD_JB_DATA_FORMAT","
                   " nIsIncrement "TD_JB_DATA_FORMAT"\n",
                jbStatistic.nIsUnderflow,
                jbStatistic.nIsNoUnderflow,
                jbStatistic.nIsIncrement));

         /* nSkDecrement total number of skipped lost samples since
            the beginning of the connection or since the last statistic
            reset in case of jitter buffer decrements. */

         /* nDsDecrement total number of dropped samples since the beginning
            of the connection or since the last statistic reset in case
            of jitter buffer decrements.*/

         /* nDsOverflow total number of dropped samples since the beginning
            of the connection  or since the last statistic reset in case
            of jitter buffer overflows.*/
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - nSkDecrement "TD_JB_DATA_FORMAT","
                   " nDsDecrement "TD_JB_DATA_FORMAT","
                   " nDsOverflow "TD_JB_DATA_FORMAT"\n",
                jbStatistic.nSkDecrement,
                jbStatistic.nDsDecrement,
                jbStatistic.nDsOverflow));

         /* nSid total number of comfort noise samples since the beginning
            of the connection or since the last statistic reset. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - total number of comfort noise samples "TD_JB_DATA_FORMAT",\n",
                jbStatistic.nSid));

         /* nRecBytesH number of received bytes high part
            including event packets. */
         /* nRecBytesL number of received bytes low part
            including event packets. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - number of received bytes including event packets:\n"
                "    high part "TD_JB_DATA_FORMAT", low part "TD_JB_DATA_FORMAT".\n",
                jbStatistic.nRecBytesH,
                jbStatistic.nRecBytesL));

      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
               ("Err, Failed to get jitter buffer statistics for %s.\n",
                aDataChannelInfo));
         return IFX_ERROR;
      }
   }
   if (IFX_FALSE != fReset)
   {
#ifdef TAPI_VERSION4
      param = (IFX_int32_t)&jbStatisticReset;
      jbStatisticReset.ch = nDataCh;
      jbStatisticReset.dev = nDev;
      nDevTmp = jbStatisticReset.dev;
#endif

      ret = TD_IOCTL(fd, IFX_TAPI_JB_STATISTICS_RESET, param,
                     nDevTmp, TD_CONN_ID_DBG);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
               ("Err, Failed to reset jitter buffer statistics %s.\n",
                aDataChannelInfo));
         return IFX_ERROR;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("Jitter buffer statistics for %s are reseted.\n",
                aDataChannelInfo));
      } /* if (IFX_ERROR == ret) */

   } /* if (IFX_FALSE != fReset) */

   return IFX_SUCCESS;
} /* Common_JBStatistic() */

/**
   Function to show and reset RTCP statistics

   \param fd      - data channel file descriptor
   \param fReset  - if not IFX_FALSE function resets RTCP statistics
   \param fShow   - if not IFX_FALSE function prints RTCP statistics

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t Common_RTCPStatistic(IFX_int32_t fd,
                                  IFX_boolean_t fReset,
                                  IFX_boolean_t fShow)
{
   IFX_int32_t ret;
   IFX_TAPI_PKT_RTCP_STATISTICS_t rtcpStatistic;

   if (IFX_FALSE != fShow)
   {

      memset(&rtcpStatistic, 0, sizeof(IFX_TAPI_PKT_RTCP_STATISTICS_t));

      ret = TD_IOCTL(fd, IFX_TAPI_PKT_RTCP_STATISTICS_GET,
                     (IFX_int32_t)&rtcpStatistic,
                     TD_DEV_NOT_SET, TD_CONN_ID_DBG);

      if (IFX_SUCCESS == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("RTCP statistic for fd %d:\n",fd));


         /* Sender generating this report. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - sender generating this report "TD_RTCP_DATA_FORMAT",\n",
                rtcpStatistic.ssrc));

         /* RTP time stamp. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
                  ("  - RTP time stamp "TD_RTCP_DATA_FORMAT",\n",
                   rtcpStatistic.rtp_ts));

         /* Sent packet count. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - sent packet count "TD_RTCP_DATA_FORMAT",\n",
                rtcpStatistic.psent));

         /* Sent octets count. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - sent octets count "TD_RTCP_DATA_FORMAT",\n",
                rtcpStatistic.osent));

         /* Data source. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - data source "TD_RTCP_DATA_FORMAT",\n",
                rtcpStatistic.rssrc));

         /* Receivers fraction loss. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - receivers fraction loss %u,\n",
                rtcpStatistic.fraction));

         /* Receivers packet loss. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - receivers packet loss %d,\n",
                rtcpStatistic.lost));
         /*TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
                     ("  - receivers packet loss %d,\n",
                      ((rtcpStatistic.lost&&0x00FFFFFF)));*/

         /* Extended last seq nr. received.*/
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - extended last seq nr. received "TD_RTCP_DATA_FORMAT",\n",
                rtcpStatistic.last_seq));

         /* Receives interarrival jitter. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("  - receives interarrival jitter "TD_RTCP_DATA_FORMAT"\n",
                rtcpStatistic.jitter));

      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
               ("Failed to get RTCP statistics.\n"));
         return IFX_ERROR;
      }
   }
   if (IFX_FALSE != fReset)
   {

      ret = TD_IOCTL(fd, IFX_TAPI_PKT_RTCP_STATISTICS_RESET, 0,
                     TD_DEV_NOT_SET, TD_CONN_ID_DBG);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
             ("Err, Failed to reset RTCP statistics.\n"));
         return IFX_ERROR;
      }
      else
      {
          TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
             ("RTCP statistics for fd %d are reseted.\n", fd));
      }

   }
   return IFX_SUCCESS;
} /* Common_RTCPStatistic */

#ifdef TAPI_VERSION4
#ifdef EASY336
/**
   Set PCM configuration.

   \param pBoard - pointer to board
   \param pcm_cfg - pointer to PCM configuration.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
   \remark
      Memory for member "pDevs..." of pcm_cfg allocated by procedure and
      should be freed by caller afterwards.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t Common_PCM_ConfigSet(BOARD_t* pBoard,
                                  RM_SVIP_PCM_CFG_t* pcm_cfg)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   switch (pBoard->nType)
   {
#ifdef EASY336
      case TYPE_SVIP:
         return BOARD_Easy336_PCM_ConfigSet(pBoard, pcm_cfg);
#ifdef WITH_VXT
      case TYPE_XT16:
         return BOARD_XT_16_PCM_ConfigSet(pBoard, pcm_cfg);
#endif /* WITH_VXT */
#endif /* EASY336 */
      default:
         return IFX_ERROR;
   }
} /* Common_PCM_ConfigSet */

/**
   Get resources (signal generator, signal detector, coder etc.) from resource
   manager.
   \param pPhone pointer to PHONE.
          pResources Pointer to resources structure to get.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_ResourcesGet(PHONE_t* pPhone, const RESOURCES_t* pResources)
{
   RM_SVIP_RU_t RU, RU_Tone;
   SVIP_RM_Status_t ret;
   IFX_int32_t voipID, connID, i;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("Phone No %d: (%s): getting\n",
       pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));

   if (pResources->nType & RES_VOIP_ID)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tVoipID\n"));

   if (pResources->nType & RES_CONN_ID)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tConnID\n"));
   else if (pResources->nConnID_Nr != SVIP_RM_UNUSED)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tConnecting to connID %d\n", pResources->nConnID_Nr));

   if (pResources->nType & RES_CODEC)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tCodec\n"));
   if (pResources->nType & RES_GEN)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tSignal generator\n"));
   if (pResources->nType & RES_DET)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tSignal detector\n"));
   if (pResources->nType & RES_LEC)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tLEC\n"));

   if (pResources->nType & RES_CONN_ID && pPhone->connID != SVIP_RM_UNUSED)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, connID already assigned to phone. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get connID */
   if (pResources->nType & RES_CONN_ID)
   {
      /* get connID, assign lineID and voipID to it */
      ret = SVIP_RM_ConnIdAlloc(&connID);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_RM_ConnIdAlloc returned %d (File: %s, line: %d)\n",
                ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
#ifdef TD_RM_DEBUG
      pPhone->oRM_Debug.nConnIdCnt++;
#endif /* TD_RM_DEBUG */

      ret = SVIP_RM_LineIdAssign(connID, pPhone->lineID);

      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, SVIP_RM_LineIdAssign returned %d (File: %s, line: %d)\n",
               ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      pPhone->connID = connID;
      /* count LineIDs */
      if (IFX_ERROR == Common_ConnID_LineID_CntAdd(pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Common_ConnID_LineID_CntAdd failed. "
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
   }
   else
   {
      if (pResources->nConnID_Nr != SVIP_RM_UNUSED)
      {
         ret = SVIP_RM_LineIdAssign(pResources->nConnID_Nr, pPhone->lineID);
         if (ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, SVIP_RM_LineIdAssign returned %d "
                   "(File: %s, line: %d)\n",
                   ret, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         pPhone->connID = pResources->nConnID_Nr;
         /* count LineIDs */
         if (IFX_ERROR == Common_ConnID_LineID_CntAdd(pPhone))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Common_ConnID_LineID_CntAdd failed. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
         }
      }
   }

   /* get voipID */
   if (pResources->nType & RES_VOIP_ID)
   {
      ret = SVIP_RM_VoipIdAlloc(&voipID);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, SVIP_RM_VoipIdAlloc returned %d (File: %s, line: %d)\n",
               ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      Common_VoipID_Add(pPhone, voipID);
#ifdef TD_RM_DEBUG
      pPhone->oRM_Debug.nVoipIdCnt++;
#endif /* TD_RM_DEBUG */
   }

   /* if connID allocation done in this step, assign all voipIDs to connID */
   if (pResources->nType & RES_CONN_ID)
   for (i = 0; i < pPhone->nVoipIDs; i++)
   {
      ret = SVIP_RM_VoipIdAssign(pPhone->connID,
                                 pPhone->pVoipIDs[i]);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_VoipIdAssign returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   /* if voipID allocation done in this call, assign connID to it */
   if (pResources->nType & RES_VOIP_ID && pPhone->connID != SVIP_RM_UNUSED)
   {
      ret = SVIP_RM_VoipIdAssign(pPhone->connID,
         pPhone->pVoipIDs[pPhone->nVoipIDs - 1]);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_VoipIdAssign returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   /* get codec */
   if (pResources->nType & RES_CODEC)
   {
      ret = SVIP_RM_RUCodAlloc(pPhone->pVoipIDs[pPhone->nVoipIDs - 1],
         eCurrEncType, pPhone->nDev, &RU);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_RUCodAlloc returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
         Common_ErrorHandle (pPhone->pBoard->nDevCtrl_FD, pPhone->pBoard,
            pPhone->nDev);
         return IFX_ERROR;
      }
#ifdef TD_RM_DEBUG
      pPhone->oRM_Debug.nCodecCnt++;
#endif /* TD_RM_DEBUG */
   }

   RU.module = IFX_TAPI_MODULE_TYPE_ALM;
   RU.nDev = pPhone->nDev;
   RU.nCh = pPhone->nPhoneCh;
   if (pPhone->pBoard->nType == TYPE_SVIP)
      RU.devType = IFX_TAPI_DEV_TYPE_VIN_SUPERVIP;
   else if (pPhone->pBoard->nType == TYPE_XT16)
      RU.devType = IFX_TAPI_DEV_TYPE_VIN_XT16;

   /* get signal detector */
   if (pResources->nType & RES_DET)
   {
      ret = SVIP_RM_RUSigDetAlloc(&RU, &RU_Tone);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_RUSigDetAlloc returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
#ifdef TD_RM_DEBUG
      pPhone->oRM_Debug.nDetCnt++;
#endif /* TD_RM_DEBUG */
   }

   /* get singal generator */
   if (pResources->nType & RES_GEN)
   {
      ret = SVIP_RM_RUSigGenAlloc(&RU, &RU_Tone);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_RUSigGenAlloc returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
#ifdef TD_RM_DEBUG
      pPhone->oRM_Debug.nGenCnt++;
#endif /* TD_RM_DEBUG */
   }

   /* get LEC */
   if (pResources->nType & RES_LEC)
   {
      ret = SVIP_RM_RULecAlloc(pPhone->lineID, &RU_Tone);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_RM_RULecAlloc returned %d (File: %s, line: %d)\n",
                ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
#ifdef TD_RM_DEBUG
      pPhone->oRM_Debug.nLecCnt++;
#endif /* TD_RM_DEBUG */
   }
   return IFX_SUCCESS;
}


/**
   Return resources to resource manager.
   \param pPhone pointer to PHONE.
          pResources Pointer to resources structure to release.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_ResourcesRelease(PHONE_t* pPhone,
                                     const RESOURCES_t* pResources)
{
   RM_SVIP_RU_t RU;
   SVIP_RM_Status_t ret;
   IFX_return_t funcRet = IFX_SUCCESS;
   IFX_int32_t i;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("Phone No %d: (%s): releasing\n",
       pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));

   if (pResources->nType & RES_VOIP_ID)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tVoipID\n"));
   if ((pResources->nType & RES_CONN_ID))
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tConnID (%d)\n", pPhone->connID));
   if (pResources->nType & RES_GEN)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tSignal generator\n"));
   if (pResources->nType & RES_DET)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tSignal detector\n"));
   if (pResources->nType & RES_LEC)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tLEC\n"));
   if (pResources->nType & RES_CODEC)
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tCodec\n"));

   if ((pResources->nType & RES_VOIP_ID) && (pPhone->nVoipIDs == 0))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, voipID not assigned to phone. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      funcRet = IFX_ERROR;
   }
   if ((pResources->nType & RES_CONN_ID) && (pPhone->connID == SVIP_RM_UNUSED))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Err, connID not assigned to phone. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      funcRet = IFX_ERROR;
   }

   if (pResources->nType & RES_GEN)
   {
   /* release signal generator */
   ret = SVIP_RM_LineIdRUSigGenGet(pPhone->lineID, &RU);
   if (ret == SVIP_RM_Success)
   {
      ret = SVIP_RM_RUSigGenRelease(&RU);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_RUSigGenRelease returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
         funcRet = IFX_ERROR;
      }
#ifdef TD_RM_DEBUG
      else
      {
         pPhone->oRM_Debug.nGenCnt--;
      }
#endif /* TD_RM_DEBUG */
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, SVIP_RM_LineIdRUSigGenGet returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      funcRet = IFX_ERROR;
   }
   }

   if (pResources->nType & RES_DET)
   {
   /* release signal detector */
   ret = SVIP_RM_LineIdRUSigDetGet(pPhone->lineID, &RU);
   if (ret == SVIP_RM_Success)
   {
      ret = SVIP_RM_RUSigDetRelease(&RU);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_RUSigDetRelease returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
         funcRet = IFX_ERROR;
      }
#ifdef TD_RM_DEBUG
      else
      {
         pPhone->oRM_Debug.nDetCnt--;
      }
#endif /* TD_RM_DEBUG */
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, SVIP_RM_LineIdRUSigDetGet returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      funcRet = IFX_ERROR;
   }
   }

   if (pResources->nType & RES_LEC)
   {
      /* release LEC */
      ret = SVIP_RM_RULecRelease(pPhone->lineID);
   if (ret != SVIP_RM_Success)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, SVIP_RM_RULecRelease returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      funcRet = IFX_ERROR;
   }
#ifdef TD_RM_DEBUG
   else
   {
      pPhone->oRM_Debug.nLecCnt--;
   }
#endif /* TD_RM_DEBUG */
   }

   if (pResources->nType & RES_CODEC)
   {
      /* release codec */
      if (pResources->nVoipID_Nr != SVIP_RM_UNUSED)
      {
         ret = SVIP_RM_RUCodRelease(pResources->nVoipID_Nr);
         if (ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_RM_RUCodRelease returned %d (File: %s, line: %d)\n",
                ret, __FILE__, __LINE__));
            Common_ErrorHandle (pPhone->pBoard->nDevCtrl_FD, pPhone->pBoard,
                                pPhone->nDev);
            funcRet = IFX_ERROR;
         }
#ifdef TD_RM_DEBUG
         else
         {
            pPhone->oRM_Debug.nCodecCnt--;
         }
#endif /* TD_RM_DEBUG */
      }
      else
      /* release all codecs allocated for this phone */
      for (i = pPhone->nVoipIDs - 1; i >= 0; i--)
      {
         ret = SVIP_RM_RUCodRelease(pPhone->pVoipIDs[i]);
         if (ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_RM_RUCodRelease returned %d (File: %s, line: %d)\n",
                ret, __FILE__, __LINE__));
            funcRet = IFX_ERROR;
         }
#ifdef TD_RM_DEBUG
         else
         {
            pPhone->oRM_Debug.nCodecCnt--;
         }
#endif /* TD_RM_DEBUG */
      }
   }

   /* deassign voipID form connID, release voipID */
   if (pResources->nType & RES_VOIP_ID)
   {
      if (pPhone->connID != SVIP_RM_UNUSED)
      {
         if (pResources->nVoipID_Nr != SVIP_RM_UNUSED)
         {
            ret = SVIP_RM_VoipIdDeassign(pPhone->connID,
               pResources->nVoipID_Nr);
            if (ret != SVIP_RM_Success)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, SVIP_RM_VoipIdDeassign returned %d "
                   "(File: %s, line: %d)\n",
                   ret, __FILE__, __LINE__));
               funcRet = IFX_ERROR;
            }
         }
         else
         {
            for (i = pPhone->nVoipIDs - 1; i >= 0; i--)
            {
               ret = SVIP_RM_VoipIdDeassign(pPhone->connID,
                  pPhone->pVoipIDs[i]);
               if (ret != SVIP_RM_Success)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, SVIP_RM_VoipIdDeassign returned %d "
                      "(File: %s, line: %d)\n",
                      ret, __FILE__, __LINE__));
                  funcRet = IFX_ERROR;
               }
            }
         }
      }
      /* release voipID */
      if (pResources->nVoipID_Nr != SVIP_RM_UNUSED)
      {
         ret = SVIP_RM_VoipIdRelease(pResources->nVoipID_Nr);
         if (ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_RM_VoipIdRelease returned %d "
                "(File: %s, line: %d)\n",
                ret, __FILE__, __LINE__));
            funcRet = IFX_ERROR;
         }
#ifdef TD_RM_DEBUG
         else
         {
            pPhone->oRM_Debug.nVoipIdCnt--;
         }
#endif /* TD_RM_DEBUG */
         Common_VoipID_Remove(pPhone, pResources->nVoipID_Nr);
      }
      else
      {
         for (i = pPhone->nVoipIDs - 1; i >= 0; i--)
         {
            ret = SVIP_RM_VoipIdRelease(pPhone->pVoipIDs[i]);
            if (ret != SVIP_RM_Success)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, SVIP_RM_VoipIdRelease returned %d "
                   "(File: %s, line: %d)\n",
                   ret, __FILE__, __LINE__));
               funcRet = IFX_ERROR;
            }
#ifdef TD_RM_DEBUG
            else
            {
               pPhone->oRM_Debug.nVoipIdCnt--;
            }
#endif /* TD_RM_DEBUG */
            Common_VoipID_Remove(pPhone, pPhone->pVoipIDs[i]);
         }
      }
   }

   /* deassign lineID and voipIDs form connID, release connID */
   if (pResources->nType & RES_CONN_ID)
   {
      for (i = 0; i < pPhone->nVoipIDs; i++)
      {
         ret = SVIP_RM_VoipIdDeassign(pPhone->connID,
            pPhone->pVoipIDs[i]);
         if (ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_RM_VoipIdDeassign returned %d "
                "(File: %s, line: %d)\n",
                ret, __FILE__, __LINE__));
            funcRet = IFX_ERROR;
         }
      }

      ret = SVIP_RM_LineIdDeassign(pPhone->connID, pPhone->lineID);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_LineIdDeassign returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
         funcRet = IFX_ERROR;
      }
      /* count LineIDs */
      if (IFX_ERROR == Common_ConnID_LineID_CntRemove(pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Common_ConnID_LineID_CntRemove failed. "
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
      /* if no more LineIDs are assigned then ConnID can be freed */
      if (0 == Common_ConnID_LineID_CntCheck(pPhone))
      {
         /* release connID */
         ret = SVIP_RM_ConnIdRelease(pPhone->connID);

         if (SVIP_RM_LineID_Assigned == ret)
         {
         /* SVIP_RM_ConnIdRelease() should be done only for the last phone
            that disconects and uses SVIP_RM_LineIdDeassign() with its
            lineID, to do so additional phone counting mechanisms would be
            needed, this is a simpler way to do it (checking return value) */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                  ("Phone No %d: Line ID still asigned - "
                   "Other phone will free ConnID (%d).\n",
                   pPhone->nPhoneNumber, pPhone->connID));
         }
         else if (ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, SVIP_RM_ConnIdRelease returned %d "
                   "(File: %s, line: %d)\n",
                   ret, __FILE__, __LINE__));
            funcRet = IFX_ERROR;
         }
#ifdef TD_RM_DEBUG
         else
         {
            pPhone->oRM_Debug.nConnIdCnt--;
         }
#endif /* TD_RM_DEBUG */
      }
      pPhone->connID = SVIP_RM_UNUSED;
   }

   return funcRet;
} /* Common_ResourcesRelease() */

/**
   Get pointer to codec status structure from voipID.

   \param pCtrl - handle to connection control structure
   \param voipID - voipID of codec of interest.
   \param nSeqConnId  - Seq Conn ID

   \return pointer to structure on success, otherwise IFX_NULL
*/
VOIP_DATA_CH_t* Common_GetCodec(CTRL_STATUS_t* pCtrl, IFX_int32_t voipID,
                                IFX_uint32_t nSeqConnId)
{
   IFX_int32_t i;
   RM_SVIP_RU_t RU;
   SVIP_RM_Status_t ret;
   BOARD_t *pBoard;

   if (voipID == SVIP_RM_UNUSED)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("Err, invalid voipID. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_NULL;
   }
   ret = SVIP_RM_VoipIdRUCodGet(voipID, &RU);
   if (ret == SVIP_RM_Success)
   {
      /* codec not allocated ? */
      if (RU.devType == IFX_TAPI_DEV_TYPE_NONE)
         /* Y */
         return IFX_NULL;

      pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
      if (pBoard == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Invalid device type (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_NULL;
      }
      if ((0 < pBoard->nMaxCoderCh) && (IFX_NULL == pBoard->pDataChStat))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, %s: No data channel statistic. (File: %s, line: %d)\n",
                pBoard->pszBoardName, __FILE__, __LINE__));
         return IFX_NULL;
      }

      for (i = 0; i < pBoard->nMaxCoderCh; i++)
         if (pBoard->pDataChStat[i].nDev == RU.nDev &&
            pBoard->pDataChStat[i].nCh == RU.nCh)
            return &pBoard->pDataChStat[i];
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
         ("Err, SVIP_RM_VoipIdRUCodGet returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      return IFX_NULL;
   }

   return IFX_NULL;
} /* Common_GetCodec() */
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

/**
   Configure indication tones.

   \param pCtrl - handle to control structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_void_t Common_ToneIndicationCfg(CTRL_STATUS_t* pCtrl)
{
   IFX_TAPI_TONE_t tone;
   BOARD_t *pBoard = TD_GET_MAIN_BOARD(pCtrl);

   /* this is global for all tapi drivers, setting for one dev is sufficient */

   memset (&tone, 0, sizeof(tone));

   tone.simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
   tone.simple.index  = TD_ITN_SET_BAND_WIDE;
   tone.simple.levelA = -1;
   tone.simple.levelB = -1;
   tone.simple.levelC = -1;
   tone.simple.levelD = -1;
   tone.simple.loop   = 1;
   tone.simple.freqA  = 396;
   tone.simple.freqB  = 440;
   tone.simple.freqC  = 495;
   tone.simple.freqD  = 528;
   tone.simple.pause = 10;
   tone.simple.cadence[0]     = 100;
   tone.simple.frequencies[0] = IFX_TAPI_TONE_FREQA;
   tone.simple.modulation[0]  = 0;
   tone.simple.cadence[1]     = 100;
   tone.simple.frequencies[1] = IFX_TAPI_TONE_FREQB;
   tone.simple.modulation[1]  = 0;
   tone.simple.cadence[2]     = 100;
   tone.simple.frequencies[2] = IFX_TAPI_TONE_FREQC;
   tone.simple.modulation[2]  = 0;
   tone.simple.cadence[3]     = 100;
   tone.simple.frequencies[3] = IFX_TAPI_TONE_FREQD;
   tone.simple.modulation[3]  = 0;

   TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_TONE_TABLE_CFG_SET, &tone,
      TD_DEV_NOT_SET, TD_CONN_ID_INIT);

   tone.simple.index  = TD_ITN_SET_BAND_NARROW;
   tone.simple.frequencies[0] = IFX_TAPI_TONE_FREQD;
   tone.simple.frequencies[1] = IFX_TAPI_TONE_FREQC;
   tone.simple.frequencies[2] = IFX_TAPI_TONE_FREQB;
   tone.simple.frequencies[3] = IFX_TAPI_TONE_FREQA;

   TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_TONE_TABLE_CFG_SET, &tone,
      TD_DEV_NOT_SET, TD_CONN_ID_INIT);

   memset (&tone, 0, sizeof(tone));

   tone.simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
   tone.simple.index  = TD_ITN_ERROR;
   tone.simple.levelA = -1;
   tone.simple.levelB = -1;
   tone.simple.loop   = 1;
   tone.simple.freqA  = 600;
   tone.simple.freqB  = 1300;
   tone.simple.pause = 0;
   tone.simple.cadence[0]     = 125;
   tone.simple.frequencies[0] = IFX_TAPI_TONE_FREQA;
   tone.simple.modulation[0]  = 0;
   tone.simple.cadence[1]     = 250;
   tone.simple.frequencies[1] = IFX_TAPI_TONE_FREQB;
   tone.simple.modulation[1]  = 0;
   tone.simple.cadence[2]     = 125;
   tone.simple.frequencies[2] = IFX_TAPI_TONE_FREQA;
   tone.simple.modulation[2]  = 0;
   tone.simple.cadence[3]     = 250;
   tone.simple.frequencies[3] = IFX_TAPI_TONE_FREQB;
   tone.simple.modulation[3]  = 0;

   TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_TONE_TABLE_CFG_SET, &tone,
      TD_DEV_NOT_SET, TD_CONN_ID_INIT);
}

 /**
   Start playing tone on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \param nTone Tone to play.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_TonePlay(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                             IFX_int32_t nTone)
{
#ifdef TAPI_VERSION4
   IFX_TAPI_TONE_PLAY_t tone;
#ifdef EASY336
   SVIP_RM_Status_t ret;
   RM_SVIP_RU_t RU;
   BOARD_t *pBoard;
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
   IFX_int32_t nToneArg = NO_PARAM, nFd;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_int32_t nRet;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nTone), nTone, IFX_ERROR);

   COM_CHANGE_TONE_IF_CUSTOM_CPT(nTone);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* play tone on DECT channel */
      return TD_DECT_TonePlay(pPhone, nTone);
   }
#endif /* TD_DECT */

#ifdef TAPI_VERSION4
   if (!pPhone->pBoard->fSingleFD)
#endif /* TAPI_VERSION4 */
   {
      nToneArg = nTone;
      /* check if data channel available */
      if (0 < pPhone->pBoard->nMaxCoderCh)
      {
         nFd = pPhone->nDataCh_FD;

      }
      else
      {
         /* use phone channel if no data channel avaible on board */
         nFd = pPhone->nPhoneCh_FD;
      }
      nRet = TD_IOCTL(nFd, IFX_TAPI_TONE_LOCAL_PLAY, nToneArg,
                      nDevTmp, pPhone->nSeqConnId);
   }
#ifdef TAPI_VERSION4
   else
   {
      memset (&tone, 0, sizeof (IFX_TAPI_TONE_PLAY_t));
#ifdef EASY336
      ret = SVIP_RM_LineIdRUSigGenGet(pPhone->lineID, &RU);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_LineIdRUSigGenGet returned %d (File: %s, line: %d)\n",
            ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* siggen not allocated ? */
      if (RU.devType == IFX_TAPI_DEV_TYPE_NONE)
      {
         /* Y */
         resources.nType = RES_GEN;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         if (Common_ResourcesGet(pPhone, &resources) != IFX_SUCCESS)
            return IFX_ERROR;
         ret = SVIP_RM_LineIdRUSigGenGet(pPhone->lineID, &RU);
      }

      if (ret != SVIP_RM_Success)
      {
         return IFX_ERROR;
      }

      pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
      if (pBoard == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid device type (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_ERROR;
      }

      tone.module = RU.module;
      tone.dev = RU.nDev;
      tone.ch = RU.nCh;
      nFd = pBoard->nDevCtrl_FD;
#else /* EASY336 */
      tone.dev = pPhone->nDev;
      tone.ch = pPhone->nPhoneCh;
      tone.module = IFX_TAPI_MODULE_TYPE_ALM;
      nFd = pPhone->pBoard->nDevCtrl_FD;
#endif /* EASY336 */

      tone.index = nTone;
      tone.external = 1;
      tone.internal = 0;
      tone.reserved = 0;
      nDevTmp = tone.dev;
      nToneArg = (IFX_int32_t) &tone;

      nRet = TD_IOCTL(nFd, IFX_TAPI_TONE_PLAY, nToneArg,
                      nDevTmp, pPhone->nSeqConnId);
   }
#endif /* TAPI_VERSION4 */
   return nRet;
}

/**
   Stop playing tone on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_ToneStop(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
#ifdef TAPI_VERSION4
   IFX_TAPI_TONE_PLAY_t tone;
#ifdef EASY336
   SVIP_RM_Status_t ret;
   RM_SVIP_RU_t RU;
   BOARD_t *pBoard;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
   IFX_int32_t nToneArg = NO_PARAM, nFd;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_int32_t nRet;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* stop tone on DECT channel */
      return TD_DECT_ToneStop(pPhone);
   }
#endif /* TD_DECT */

#ifdef TAPI_VERSION4
   if (!pPhone->pBoard->fSingleFD)
#endif /* TAPI_VERSION4 */
   {
      /* check if data channel available */
      if (0 < pPhone->pBoard->nMaxCoderCh)
      {
         nFd = pPhone->nDataCh_FD;

      }
      else
      {
         /* use phone channel if no data channel avaible on board */
         nFd = pPhone->nPhoneCh_FD;
      }
   }
#ifdef TAPI_VERSION4
   else
   {
      memset (&tone, 0, sizeof (IFX_TAPI_TONE_PLAY_t));
#ifdef EASY336
      /* if DTMF digit test then end function
         because resource manager is deinitialized */
      COM_MOD_DIGIT_DTMF_SVIP_RETURN_FROM_FUNCTION(IFX_SUCCESS);

      ret = SVIP_RM_LineIdRUSigGenGet(pPhone->lineID, &RU);
      if (ret != SVIP_RM_Success)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_LineIdRUSigGenGet returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
      if (pBoard == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid device type (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_ERROR;
      }

      tone.module = RU.module;
      tone.dev = RU.nDev;
      tone.ch = RU.nCh;
      nFd = pBoard->nDevCtrl_FD;
#else /* EASY336 */
      tone.module = IFX_TAPI_MODULE_TYPE_ALM;
      tone.dev = pPhone->nDev;
      tone.ch = pPhone->nPhoneCh;
      nFd = pPhone->pBoard->nDevCtrl_FD;
#endif /* EASY336 */

      /* common for SVIP and VXT */
      tone.external = 1;
      tone.internal = 0;
      tone.reserved = 0;
      nDevTmp = tone.dev;
      nToneArg = (IFX_int32_t)&tone;
   }
#endif /* TAPI_VERSION4 */
   /* stop playing tone */
   nRet = TD_IOCTL(nFd, IFX_TAPI_TONE_STOP, nToneArg,
                      nDevTmp, pPhone->nSeqConnId);

   return nRet;
} /* Common_ToneStop() */

/**
   Start ringing on phone.

   \param pPhone - pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_RingingStart(PHONE_t* pPhone)
{
   IFX_return_t nRet = IFX_ERROR;
#ifdef TAPI_VERSION4
   IFX_TAPI_RING_t ring;
#endif /* TAPI_VERSION4 */
   IFX_int32_t nRing = NO_PARAM, nFd;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   TD_PTR_CHECK(pPhone, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* start ringing on DECT handset */
      nRet = TD_DECT_Ring(pPhone);
   }
   else
#endif /* TD_DECT */
   {
#ifdef TAPI_VERSION4
      if (pPhone->pBoard->fSingleFD)
      {
         memset (&ring, 0, sizeof (IFX_TAPI_RING_t));
         ring.ch = pPhone->nPhoneCh;
         ring.dev = pPhone->nDev;
         nFd = pPhone->pBoard->nDevCtrl_FD;
         nDevTmp = ring.dev;
         nRing = (IFX_int32_t)&ring;
      }
      else
#endif /* TAPI_VERSION4 */
      {
         nFd = pPhone->nPhoneCh_FD;
      }
      /* start ringing */
      nRet = TD_IOCTL(nFd, IFX_TAPI_RING_START, nRing,
                      nDevTmp, pPhone->nSeqConnId);
   }

   if (IFX_SUCCESS == nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Start Ringing.\n",
             (IFX_int32_t) pPhone->nPhoneNumber));
   }

   return nRet;
} /* Common_RingingStart() */

/**
   Stop ringing on phone.

   \param pPhone - pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_RingingStop(PHONE_t* pPhone)
{
   IFX_return_t nRet = IFX_ERROR;
#ifdef TAPI_VERSION4
   IFX_TAPI_RING_t ring;
#endif /* TAPI_VERSION4 */
   IFX_int32_t nRing = NO_PARAM, nFd;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   TD_PTR_CHECK(pPhone, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* stop ringing is done differently for missed call
         and established call */
      return IFX_SUCCESS;
   }
#endif /* TD_DECT */
#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
      memset (&ring, 0, sizeof (IFX_TAPI_RING_t));
      ring.ch = pPhone->nPhoneCh;
      ring.dev = pPhone->nDev;
      nFd = pPhone->pBoard->nDevCtrl_FD;
      nDevTmp = ring.dev;
      nRing = (IFX_int32_t)&ring;
   } /* if (pPhone->pBoard->fSingleFD) */
   else
#endif
   {
      nFd = pPhone->nPhoneCh_FD;
   } /* if (pPhone->pBoard->fSingleFD) */
   /* stop phone ringing */
   nRet = TD_IOCTL(nFd, IFX_TAPI_RING_STOP, nRing,
                      nDevTmp, pPhone->nSeqConnId);
   /* check if ringing stopped */
   if (nRet == IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Ringing stop.\n",
             pPhone->nPhoneNumber));
   }
   return nRet;
} /* Common_RingingStopt() */

#ifdef TAPI_VERSION4
/**
   Enable DTMF detection on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_DTMF_Enable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
   IFX_TAPI_EVENT_t tapiEvent;
   IFX_return_t ioctlRet;
#ifdef EASY336
   SVIP_RM_Status_t ret = SVIP_RM_Success;
   RM_SVIP_RU_t RU;
   BOARD_t *pBoard;
   RESOURCES_t resources;

   ret = SVIP_RM_LineIdRUSigDetGet(pPhone->lineID, &RU);
   if (ret == SVIP_RM_Success)
   {
      /* sigdet not allocated ? */
      if (RU.devType == IFX_TAPI_DEV_TYPE_NONE)
      {
         /* Y */
         resources.nType = RES_DET;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         if (Common_ResourcesGet(pPhone, &resources) !=
            IFX_SUCCESS)
            return IFX_ERROR;
         ret = SVIP_RM_LineIdRUSigDetGet(pPhone->lineID, &RU);
     }

      if (ret == SVIP_RM_Success)
      {
         pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
         if (pBoard == IFX_NULL)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Invalid device type (File: %s, line: %d)\n",
                __FILE__, __LINE__));
            return IFX_ERROR;
         }

         memset((void*)&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.dev = RU.nDev;
         tapiEvent.ch = RU.nCh;
         tapiEvent.module = RU.module;
         /* DTMF tone detection done from the analog line side. */
         tapiEvent.id = IFX_TAPI_EVENT_DTMF_DIGIT;
#ifdef EASY336
         tapiEvent.data.dtmf.external = 1;
         tapiEvent.data.dtmf.internal = 0;
#else
         tapiEvent.data.dtmf.local = 1;
         tapiEvent.data.dtmf.network = 0;
#endif
         ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_ENABLE,
                             (IFX_int32_t) &tapiEvent,
                             tapiEvent.dev, pPhone->nSeqConnId);

         TD_RETURN_IF_ERROR(ioctlRet);

         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: Tone detection started on %s dev %d, ch %d \n",
                pPhone->nPhoneNumber,
                pBoard->pszBoardName, tapiEvent.dev, tapiEvent.ch));
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, SVIP_RM_LineIdRUSigDetGet returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#else /* EASY336 */
   memset((void*)&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
   tapiEvent.dev = pPhone->nDev;
   tapiEvent.ch = pPhone->nPhoneCh;
   tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
   /* DTMF tone detection done from the analog line side. */
   tapiEvent.id = IFX_TAPI_EVENT_DTMF_DIGIT;
#if (defined(EASY336) || defined(XT16))
         tapiEvent.data.dtmf.external = 1;
         tapiEvent.data.dtmf.internal = 0;
#else
         tapiEvent.data.dtmf.local = 1;
         tapiEvent.data.dtmf.network = 0;
#endif
   ioctlRet = TD_IOCTL(pPhone->pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_ENABLE,
                       (IFX_int32_t) &tapiEvent,
                       tapiEvent.dev, pPhone->nSeqConnId);

   TD_RETURN_IF_ERROR(ioctlRet);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Tone detection started on %s dev %d, ch %d \n",
          pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName,
          tapiEvent.dev, tapiEvent.ch));
#endif /* EASY336 */
   return IFX_SUCCESS;
}

/**
   Disable DTMF detection on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_DTMF_Disable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
   IFX_TAPI_EVENT_t tapiEvent;
   IFX_return_t ioctlRet;
#ifdef EASY336
   SVIP_RM_Status_t ret = SVIP_RM_Success;
   RM_SVIP_RU_t RU;
   BOARD_t *pBoard;

   ret = SVIP_RM_LineIdRUSigDetGet(pPhone->lineID, &RU);
   if (ret == SVIP_RM_Success)
   {
      pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
      if (pBoard == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid device type (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_ERROR;
      }

      memset((void*)&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
      tapiEvent.dev = RU.nDev;
      tapiEvent.ch = RU.nCh;
      tapiEvent.module = RU.module;
      /* DTMF tone detection done from the analog line side. */
      tapiEvent.id = IFX_TAPI_EVENT_DTMF_DIGIT;
#ifdef EASY336
      tapiEvent.data.dtmf.external = 1;
      tapiEvent.data.dtmf.internal = 0;
#else
      tapiEvent.data.dtmf.local = 1;
      tapiEvent.data.dtmf.network = 0;
#endif
      ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_DISABLE,
                    (IFX_int32_t) &tapiEvent,
                    tapiEvent.dev, pPhone->nSeqConnId);

      TD_RETURN_IF_ERROR(ioctlRet);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
           ("Phone No %d: Tone detection stopped on %s dev %d, channel %d \n",
            pPhone->nPhoneNumber, pBoard->pszBoardName,
            tapiEvent.dev, tapiEvent.ch));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, SVIP_RM_LineIdRUSigDetGet returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#else /* EASY336 */
   memset((void*)&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
   tapiEvent.dev = pPhone->nDev;
   tapiEvent.ch = pPhone->nPhoneCh;
   tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
   /* DTMF tone detection done from the analog line side. */
   tapiEvent.id = IFX_TAPI_EVENT_DTMF_DIGIT;
#if (defined(EASY336) || defined(XT16))
   tapiEvent.data.dtmf.external = 1;
   tapiEvent.data.dtmf.internal = 0;
#else
   tapiEvent.data.dtmf.local = 1;
   tapiEvent.data.dtmf.network = 0;
#endif
   ioctlRet = TD_IOCTL(pPhone->pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_DISABLE,
                       (IFX_int32_t) &tapiEvent,
                       tapiEvent.dev, pPhone->nSeqConnId);

   TD_RETURN_IF_ERROR(ioctlRet);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("Phone No %d: Tone detection stopped on %s dev %d, ch %d \n",
       pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName,
       tapiEvent.dev, tapiEvent.ch));
#endif /* EASY336 */
   return IFX_SUCCESS;
}

#ifdef EASY336
/**
   Enable LEC on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_LEC_Enable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
   SVIP_RM_Status_t ret;
   IFX_TAPI_WLEC_CFG_t wlecConf;
   RM_SVIP_RU_t RU;
   BOARD_t *pBoard;
   IFX_return_t ioctlRet;

   /* check if lec should be used */
   if (pCtrl->pProgramArg->oArgFlags.nNoLEC)
      return IFX_SUCCESS;

   /* start LEC on callee */
   ret = SVIP_RM_LineIdRULecGet(pPhone->lineID, &RU);
   if (ret == SVIP_RM_Success)
   {
      pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
      if (pBoard == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid device type (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
      memset(&wlecConf, 0, sizeof(IFX_TAPI_WLEC_CFG_t));

      wlecConf.nType = pCtrl->pProgramArg->nLecCfg;
      wlecConf.bNlp = IFX_TAPI_WLEC_NLP_DEFAULT;
      wlecConf.nNBFEwindow = IFX_TAPI_WLEN_WSIZE_8;
      wlecConf.nNBNEwindow = IFX_TAPI_WLEN_WSIZE_8;
      /* wlecConf.nWBNEwindow = IFX_TAPI_WLEN_WSIZE_8; */
      wlecConf.dev = RU.nDev;
      wlecConf.ch = RU.nCh;

      /* change default settings for LEC test */
      COM_MOD_LEC_SET(pPhone, &wlecConf);

      if (RU.module == IFX_TAPI_MODULE_TYPE_ALM)
      {
         ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_WLEC_PHONE_CFG_SET,
                             (IFX_int32_t) &wlecConf,
                             wlecConf.dev, pPhone->nSeqConnId);
      }
      else
      {
         ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_WLEC_PCM_CFG_SET,
                             (IFX_int32_t) &wlecConf,
                             wlecConf.dev, pPhone->nSeqConnId);
      }
      TD_RETURN_IF_ERROR(ioctlRet);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: LEC started on %s dev %d, ch %d\n",
          pPhone->nPhoneNumber, pBoard->pszBoardName,
          wlecConf.dev, wlecConf.ch));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, SVIP_RM_LineIdRULecGet returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Disable LEC on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_LEC_Disable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
   SVIP_RM_Status_t ret;
   IFX_TAPI_WLEC_CFG_t wlecConf;
   RM_SVIP_RU_t RU;
   BOARD_t *pBoard;
   IFX_return_t ioctlRet;

   /* check if lec should be used */
   if (pCtrl->pProgramArg->oArgFlags.nNoLEC)
      return IFX_SUCCESS;

   ret = SVIP_RM_LineIdRULecGet(pPhone->lineID, &RU);
   if (ret == SVIP_RM_Success)
   {
      pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
      if (pBoard == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid device type (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_ERROR;
      }

      memset(&wlecConf, 0, sizeof(IFX_TAPI_WLEC_CFG_t));

      wlecConf.nType = IFX_TAPI_WLEC_TYPE_OFF;
      wlecConf.bNlp = IFX_TAPI_WLEC_NLP_OFF;
      wlecConf.dev = RU.nDev;
      wlecConf.ch = RU.nCh;
      if (RU.module == IFX_TAPI_MODULE_TYPE_ALM)
      {
         ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_WLEC_PHONE_CFG_SET,
                             (IFX_int32_t) &wlecConf,
                             wlecConf.dev, pPhone->nSeqConnId);
      }
      else
      {
         ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_WLEC_PCM_CFG_SET,
                             (IFX_int32_t) &wlecConf,
                             wlecConf.dev, pPhone->nSeqConnId);
      }
      TD_RETURN_IF_ERROR(ioctlRet);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: LEC stopped on %s dev %d, ch %d\n",
          pPhone->nPhoneNumber, pBoard->pszBoardName,
          wlecConf.dev, wlecConf.ch));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, SVIP_RM_LineIdRULecGet returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}
#endif /* EASY336 */
#else /* TAPI_VERSION4 */

/**
   Enable LEC on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_LEC_Enable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
#if (!defined EASY3201 && !defined EASY3201_EVS && !defined XT16)
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_WLEC_CFG_t wlecConf = {0};
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* no LEC for DECT phone */
      return IFX_SUCCESS;
   }
#endif /* TD_DECT */

#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* no LEC enable/disable for DuSLIC-xT phones, currently not supported */
      return IFX_SUCCESS;
   }
#endif /* EASY3111 */

   /* check if lec should be used */
   if (pCtrl->pProgramArg->oArgFlags.nNoLEC)
      return IFX_SUCCESS;

   /* Activate WLEC for Phone */
   memset(&wlecConf, 0, sizeof(IFX_TAPI_WLEC_CFG_t));

   wlecConf.nType = pCtrl->pProgramArg->nLecCfg;
   wlecConf.bNlp = IFX_TAPI_WLEC_NLP_ON;
   wlecConf.nNBNEwindow = 0 /* IFX_TAPI_WLEN_WSIZE_8 */;
   wlecConf.nNBFEwindow = 0 /* IFX_TAPI_WLEN_WSIZE_8 */;
   wlecConf.nWBNEwindow = 0 /* IFX_TAPI_WLEN_WSIZE_8 */;

#ifdef TAPI_VERSION4
   nDevTmp = wlecConf.dev;
#endif /* TAPI_VERSION4 */

   /* change default settings for LEC test */
   COM_MOD_LEC_SET(pPhone, &wlecConf);

   ret = TD_IOCTL(pPhone->nPhoneCh_FD, IFX_TAPI_WLEC_PHONE_CFG_SET,
                  (IFX_int32_t) &wlecConf, nDevTmp, pPhone->nSeqConnId);
   TD_RETURN_IF_ERROR(ret);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: LEC started on %s, PhoneCh %d\n",
          pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName, pPhone->nPhoneCh));
#endif /* !EASY3201 && !XT16 */

   return IFX_SUCCESS;
}

/**
   Disable LEC on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_LEC_Disable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
#if (!defined EASY3201 && !defined EASY3201_EVS && !defined XT16)
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_WLEC_CFG_t wlecConf = {0};
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* no LEC for DECT phone */
      return IFX_SUCCESS;
   }
#endif /* TD_DECT */

#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* no LEC enable/disable for DuSLIC-xT phones, currently not supported */
      return IFX_SUCCESS;
   }
#endif /* EASY3111 */

   /* check if lec should be used */
   if (pCtrl->pProgramArg->oArgFlags.nNoLEC)
      return IFX_SUCCESS;

   /* Activate WLEC for Phone */
   memset(&wlecConf, 0, sizeof(IFX_TAPI_WLEC_CFG_t));

   wlecConf.nType = IFX_TAPI_WLEC_TYPE_OFF;
   wlecConf.bNlp = IFX_TAPI_WLEC_NLP_OFF;
   wlecConf.nNBNEwindow = 0 /* IFX_TAPI_WLEN_WSIZE_8 */;
   wlecConf.nNBFEwindow = 0 /* IFX_TAPI_WLEN_WSIZE_8 */;
   wlecConf.nWBNEwindow = 0 /* IFX_TAPI_WLEN_WSIZE_8 */;

#ifdef TAPI_VERSION4
   nDevTmp = wlecConf.dev;
#endif /* TAPI_VERSION4 */

   ret = TD_IOCTL(pPhone->nPhoneCh_FD, IFX_TAPI_WLEC_PHONE_CFG_SET,
                  (IFX_int32_t) &wlecConf, nDevTmp, pPhone->nSeqConnId);
   TD_RETURN_IF_ERROR(ret);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: LEC stopped on %s, phone ch %d\n",
          pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName, pPhone->nPhoneCh));

#endif /* !EASY3201 && !XT16 */
   return IFX_SUCCESS;
}
#endif /* TAPI_VERSION4 */

/**
   Enable LEC on phone.

   \param pFXO - pointer to fxo
   \param nPCM_Ch - PCM channel number
   \param pEnable - if IFX_ENABLE then enable LEC on PCM, if else disable LEC
   \param pBoard  - poiter to board

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_LEC_On_PCM_Ch(FXO_t* pFXO, IFX_int32_t nPCM_Ch,
                                  IFX_enDis_t nEnable, BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_WLEC_CFG_t wlecConf = {0};
   IFX_int32_t nPCM_Fd;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nPCM_Ch), nPCM_Ch, IFX_ERROR);

   /* check if lec should be used */
   if (pBoard->pCtrl->pProgramArg->oArgFlags.nNoLEC)
      return IFX_SUCCESS;

   /* get channel file descriptor */
   nPCM_Fd = PCM_GetFD_OfCh(nPCM_Ch, pBoard);
   if (NO_DEVICE_FD == nPCM_Fd)
   {
      /* PCM channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Failed to get FD of PCM ch %d, board %s. "
             "(File: %s, line: %d)\n",
             nPCM_Ch, pBoard->pszBoardName, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Prepare structure */
   memset(&wlecConf, 0, sizeof(IFX_TAPI_WLEC_CFG_t));

   if (IFX_ENABLE == nEnable)
   {
      /* enable settings */
      wlecConf.nType = pBoard->pCtrl->pProgramArg->nLecCfg;
      wlecConf.bNlp = IFX_TAPI_WLEC_NLP_ON;
      wlecConf.nNBNEwindow = IFX_TAPI_WLEN_WSIZE_16;
      wlecConf.nNBFEwindow = 0;
      wlecConf.nWBNEwindow = 0;
   }
   else
   {
      /* disable settings */
      wlecConf.nType = IFX_TAPI_WLEC_TYPE_OFF;
      wlecConf.bNlp = IFX_TAPI_WLEC_NLP_OFF;
      wlecConf.nNBNEwindow = 0;
      wlecConf.nNBFEwindow = 0;
      wlecConf.nWBNEwindow = 0;
   }
#ifdef TAPI_VERSION4
   wlecConf.ch = nPCM_Ch;
   wlecConf.dev = 0;
   nDevTmp = wlecConf.dev;
#endif /*  TAPI_VERSION4 */
   /* activate LEC */
   ret = TD_IOCTL(nPCM_Fd, IFX_TAPI_WLEC_PCM_CFG_SET, (IFX_int32_t) &wlecConf,
                   nDevTmp, pFXO->nSeqConnId);
   if (ret != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, PCM ch No %d, Board %s: LEC failed to %s on PCM channel "
             "(File: %s, line: %d)\n",
             nPCM_Ch, pBoard->pszBoardName,
             IFX_ENABLE == nEnable ? "start" : "stop", __FILE__, __LINE__));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
            ("PCM ch No %d, Board %s: LEC %s on PCM channel\n",
             nPCM_Ch, pBoard->pszBoardName,
             IFX_ENABLE == nEnable ? "started" : "stopped"));
   }
   return ret;
}

#ifdef TAPI_VERSION4
/**
   Enable pulse detection on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_PulseEnable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
#ifdef EASY336
   SVIP_RM_Status_t ret = SVIP_RM_Success;
   RM_SVIP_RU_t RU;
#endif
   IFX_TAPI_EVENT_t tapiEvent;
   IFX_return_t ioctlRet;
   BOARD_t *pBoard;

#ifdef EASY336
   ret = SVIP_RM_LineIdRUSigDetGet(pPhone->lineID, &RU);
   if (ret == SVIP_RM_Success)
#endif
   {
#ifdef XT16
      pBoard = ABSTRACT_GetBoard(pCtrl, IFX_TAPI_DEV_TYPE_VIN_XT16);
#elif EASY336
      pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
#else
      pBoard = pPhone->pBoard;
#endif
      if (pBoard == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid device type (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_ERROR;
      }

      memset((void*)&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
#ifdef EASY336
      tapiEvent.dev = RU.nDev;
      tapiEvent.ch = RU.nCh;
      tapiEvent.module = RU.module;
#else
      tapiEvent.dev = pPhone->nDev;
      tapiEvent.ch = pPhone->nPhoneCh;
      tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
#endif
      tapiEvent.id = IFX_TAPI_EVENT_PULSE_DIGIT;
      ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_ENABLE,
                           (IFX_int32_t) &tapiEvent,
                           tapiEvent.dev, pPhone->nSeqConnId);
      if (ioctlRet != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, IFX_TAPI_EVENT_ENABLE failed (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
#ifdef EASY336
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, SVIP_RM_LineIdRUSigDetGet returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif
   return IFX_SUCCESS;
}

/**
   Disable pulse detection on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_PulseDisable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
#ifdef EASY336
   SVIP_RM_Status_t ret = SVIP_RM_Success;
   RM_SVIP_RU_t RU;
#endif
   IFX_TAPI_EVENT_t tapiEvent;
   IFX_return_t ioctlRet;
   BOARD_t *pBoard;

#ifdef EASY336
   ret = SVIP_RM_LineIdRUSigDetGet(pPhone->lineID, &RU);
   if (ret == SVIP_RM_Success)
#endif
   {
#ifdef EASY336
      pBoard = ABSTRACT_GetBoard(pCtrl, RU.devType);
#elif XT16
      pBoard = ABSTRACT_GetBoard(pCtrl, IFX_TAPI_DEV_TYPE_VIN_XT16);
#else
      pBoard = pPhone->pBoard;
#endif
      if (pBoard == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Invalid device type (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_ERROR;
      }

      memset((void*)&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
#ifdef EASY336
      tapiEvent.dev = RU.nDev;
      tapiEvent.ch = RU.nCh;
      tapiEvent.module = RU.module;
#else
      tapiEvent.dev = pPhone->nDev;
      tapiEvent.ch = pPhone->nDataCh;
      tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
#endif
      /* DTMF tone detection done from the analog line side. */
      tapiEvent.id = IFX_TAPI_EVENT_PULSE_DIGIT;
      ioctlRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_DISABLE,
                           (IFX_int32_t) &tapiEvent,
                           tapiEvent.dev, pPhone->nSeqConnId);
      if (ioctlRet != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, IFX_TAPI_EVENT_DISABLE failed (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
#ifdef EASY336
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, SVIP_RM_LineIdRUSigDetGet returned %d (File: %s, line: %d)\n",
          ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif
   return IFX_SUCCESS;
}

/**
   Enable CID detection on phone.

   \param pPhone Pointer to PHONE.
   \param bCID_AS Boolean switch to request CID with acknowledge signal
                  detection
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_CID_Enable(PHONE_t* pPhone, IFX_boolean_t bCID_AS)
{
#ifdef EASY336
   SVIP_RM_Status_t ret = SVIP_RM_Success;
   RM_SVIP_RU_t RU;
   if (IFX_TRUE == pPhone->fCID_Enabled)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: CID already enabled. (File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* request CID from resource manager */
   ret = SVIP_RM_CidAlloc (pPhone->lineID, bCID_AS, &RU);
   if (ret != SVIP_RM_Success)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_CidAlloc returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pPhone->fCID_Enabled = IFX_TRUE;
#endif

   COM_ITM_TRACES(pPhone->pBoard->pCtrl, pPhone, IFX_NULL,
                  IFX_SUCCESS, TTP_CID_ENABLE, pPhone->nSeqConnId);

   return IFX_SUCCESS;
}

/**
   Disable CID detection on phone.

   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_CID_Disable(PHONE_t* pPhone)
{
#ifdef EASY336
   SVIP_RM_Status_t ret = SVIP_RM_Success;
   if (IFX_FALSE == pPhone->fCID_Enabled)
   {
      /* CID already disabled on this phone */
      return IFX_SUCCESS;
   }

   ret = SVIP_RM_CidRelease(pPhone->lineID);
   if (ret != SVIP_RM_Success)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_RM_CidRelease returned %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pPhone->fCID_Enabled = IFX_FALSE;
#endif

   COM_ITM_TRACES(pPhone->pBoard->pCtrl, pPhone, IFX_NULL,
                  IFX_SUCCESS, TTP_CID_DISABLE, pPhone->nSeqConnId);

   return IFX_SUCCESS;
}
#endif /* TAPI_VERSION4 */

#ifndef TAPI_VERSION4
/**
   Enable DTMF signal detection on phone.

   \param pCtrl   - handle to connection control structure
   \param pPhone  - pointer to PHONE

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_DTMF_SIG_Enable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_SIG_DETECTION_t dtmfDetection;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* no DTMF detection for DECT */
      return IFX_SUCCESS;
   }
#endif /* TD_DECT */
   /* do not use if data channel is not available */
   if (0 <= pPhone->nDataCh)
   {
      memset(&dtmfDetection, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));

      /* Start detection of DTMF tones from local interface */
      dtmfDetection.sig = IFX_TAPI_SIG_DTMFTX;
      ret = TD_IOCTL(pPhone->nDataCh_FD, IFX_TAPI_SIG_DETECT_ENABLE,
                     (IFX_int32_t) &dtmfDetection, TD_DEV_NOT_SET,
                     pPhone->nSeqConnId);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: data ch %d, fd %d "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->nDataCh, pPhone->nDataCh_FD,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
           ("Phone No %d: DTMF detection started on %s, DataCh %d, fd %d\n",
            pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName,
            pPhone->nDataCh, pPhone->nDataCh_FD));
   }

   return IFX_SUCCESS;
}

/**
   Disable DTMF signal detection on phone.

   \param pCtrl - handle to connection control structure
   \param pPhone Pointer to PHONE.
   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_DTMF_SIG_Disable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_SIG_DETECTION_t dtmfDetection;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* no DTMF detection for DECT */
      return IFX_SUCCESS;
   }
#endif /* TD_DECT */
   /* do not use if data channel is not available */
   if (0 <= pPhone->nDataCh)
   {
      memset(&dtmfDetection, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));

      /* Stop detection of DTMF tones from local interface */
      dtmfDetection.sig = IFX_TAPI_SIG_DTMFTX;
      ret = TD_IOCTL(pPhone->nDataCh_FD, IFX_TAPI_SIG_DETECT_DISABLE,
                     (IFX_int32_t) &dtmfDetection, TD_DEV_NOT_SET,
                     pPhone->nSeqConnId);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: data ch %d, fd %d "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->nDataCh, pPhone->nDataCh_FD,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
           ("Phone No %d: DTMF detection stopped on %s, data ch %d, fd %d \n",
            pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName,
            pPhone->nDataCh, pPhone->nDataCh_FD));
   }
   return IFX_SUCCESS;

}
#endif /* TAPI_VERSION4 */

/**
   Check start/end markers and length of message.

   \param pMsg - pointer to message structure

   return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_CommMsgSet(TD_COMM_MSG_t* pMsg, IFX_char_t* pBuff,
                               IFX_int32_t nBuffLen)
{
   IFX_int32_t nMsgLenMax;
   /* check input parameter */
   TD_PTR_CHECK(pMsg, IFX_ERROR);
   TD_PTR_CHECK(pBuff, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= nBuffLen), nBuffLen, IFX_ERROR);
   TD_PARAMETER_CHECK((TD_MAX_COMM_MSG_SIZE < nBuffLen), nBuffLen, IFX_ERROR);

   /* number of bytes thet will be copied to structure cannot be biger than
      structure size or buffer length */
   nMsgLenMax = (sizeof(TD_COMM_MSG_t) < nBuffLen) ?
      sizeof(TD_COMM_MSG_t) : nBuffLen;
   memcpy (pMsg, pBuff, nMsgLenMax);

   /* check message and buffer lengths */
   if (pMsg->nMsgLength != nBuffLen)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_MSG,
            ("Err, Recevied message has incorrect size: %d\n"
             "(File: %s, line: %d)\n", nBuffLen, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* get end marker */
   pMsg->nMarkEnd =
      *((IFX_uint32_t*) &pBuff[pMsg->nMsgLength - sizeof(IFX_uint32_t)]);

   /* check start/end markers */
   if (COMM_MSG_START_FLAG != pMsg->nMarkStart ||
       COMM_MSG_END_FLAG != pMsg->nMarkEnd)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_MSG,
            ("Err, Invalid stat(%d)/end(%d) markers . (File: %s, line: %d)\n",
             pMsg->nMarkStart, pMsg->nMarkEnd, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* compare message versions */
   if (TD_COMM_MSG_CURRENT_VERSION != pMsg->nMsgVersion)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_MSG,
         ("Difference in MSG versions used for communication:\n"
          "   received 0x%02X, current 0x%02X.\n"
          "   MSG functionality added since version 0x%02X "
          "will not be supported.\n",
          pMsg->nMsgVersion, TD_COMM_MSG_CURRENT_VERSION,
          (pMsg->nMsgVersion < TD_COMM_MSG_CURRENT_VERSION) ?
          pMsg->nMsgVersion : TD_COMM_MSG_CURRENT_VERSION));
   }
   return IFX_SUCCESS;
}

#ifdef TAPI_VERSION4
#ifdef EASY336
/**
   Get array of phones with the specified connID.

   \param pCtrl - handle to connection control structure
   \param connID - connID
   \param pPhones - pointer to array of pointers to phones (array allocated
   and filled by function)

   \return Number of phones with specified connID
*/
IFX_int32_t Common_ConnID_PhonesGet(CTRL_STATUS_t* pCtrl,
                                    IFX_int32_t connID, PHONE_t** pPhones[])
{
   IFX_int32_t i, j, k = 0, nPhones = 0;

   for (i = 0; i < pCtrl->nBoardCnt; i++)
      for (j = 0; j < pCtrl->rgoBoards[i].nMaxPhones; j++)
         if (pCtrl->rgoBoards[i].rgoPhones[j].connID == connID)
            nPhones++;

   if (nPhones > 0)
   {
      *pPhones = TD_OS_MemAlloc(sizeof(PHONE_t*) * nPhones);
      for (i = 0; i < pCtrl->nBoardCnt; i++)
         for (j = 0; j < pCtrl->rgoBoards[i].nMaxPhones; j++)
            if (pCtrl->rgoBoards[i].rgoPhones[j].connID == connID)
               (*pPhones)[k++] = &pCtrl->rgoBoards[i].rgoPhones[j];
   }
   else
      *pPhones = IFX_NULL;

   return nPhones;
}
#endif /* EASY336 */

#ifdef LINUX
/**
   Writes a byte to the specified memory address.

   \param nAddr - address.
   \param nValue - value.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_WriteByte(IFX_uint32_t nAddr, IFX_uint8_t nValue)
{
   IFX_int32_t mfd;
   IFX_void_t *pRealIO;
   off_t real_addr, offset;

   real_addr = nAddr & ~4095;
   if (real_addr == 0xfffff000)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
             ("Err, cannot map the top 4K page (File: %s, line: %d)\n",
              __FILE__, __LINE__));
      return IFX_ERROR;
   }
   offset = nAddr - real_addr;
   if (real_addr + 0x1000 < real_addr)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
         ("Err, Aligned addr+len exceeds top of address space "
          "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   mfd = open("/dev/mem", O_RDWR);
   if (0 > mfd)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
         ("Err, /dev/mem open failed (File: %s, line: %d)\n",
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pRealIO = mmap(IFX_NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, mfd,
      real_addr);
   if (pRealIO == (void *)(-1))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, mmap failed (File: %s, line: %d)\n", __FILE__, __LINE__));
      close (mfd);
      return IFX_ERROR;
   }
   *(IFX_uint8_t *)((IFX_uint8_t *)pRealIO + offset) = nValue;
   munmap(pRealIO, 0x1000);
   close (mfd);
   return IFX_SUCCESS;
}
#endif

/**
   Reads chip count and sets board field "nChipCnt" of passed BOARD_t structure
   pointer.

   \param pBoard - pointer to board structure.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_SetChipCount(BOARD_t* pBoard)
{
   IFX_TAPI_CAP_t CapList;
   IFX_int32_t nIO_Ret;

   memset(&CapList, 0, sizeof(IFX_TAPI_CAP_t));
   switch (pBoard->nType)
   {
      case TYPE_VINETIC:
         CapList.cap = IFX_TAPI_DEV_TYPE_VINETIC;
         break;
      case TYPE_DANUBE:
         CapList.cap = IFX_TAPI_DEV_TYPE_INCA_IPP;
         break;
      case TYPE_DUSLIC_XT:
         CapList.cap = IFX_TAPI_DEV_TYPE_DUSLIC_XT;
         break;
      case TYPE_DUSLIC:
         CapList.cap = IFX_TAPI_DEV_TYPE_DUSLIC;
         break;
      case TYPE_SVIP:
         CapList.cap = IFX_TAPI_DEV_TYPE_VIN_SUPERVIP;
         break;
      case TYPE_XT16:
         CapList.cap = IFX_TAPI_DEV_TYPE_VIN_XT16;
         break;
      default:
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Invalid board type (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   CapList.captype = IFX_TAPI_CAP_TYPE_DEVTYPE;
   do
   {
      CapList.dev = (IFX_uint16_t) pBoard->nChipCnt;
      if (0 == CapList.dev)
      {
         nIO_Ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_CHECK,
                      (IFX_int32_t) &CapList, CapList.dev, TD_CONN_ID_INIT);
      }
      else
      {
         nIO_Ret = TD_IOCTL_NO_ERR(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_CHECK,
                      (IFX_int32_t) &CapList, CapList.dev, TD_CONN_ID_INIT);
      }
      if (nIO_Ret == 1)
         pBoard->nChipCnt++;
   }
   while (nIO_Ret == 1);

   return IFX_SUCCESS;
}
#endif /* TAPI_VERSION4 */

/**
   Sets line feed mode.

   \param pPhone - pointer to phone structure.
   \param mode - desired line feed mode.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t Common_LineFeedSet(PHONE_t* pPhone, TD_LINE_MODE_t mode)
{
   IFX_return_t ret = IFX_ERROR;
#ifdef TAPI_VERSION4
   IFX_TAPI_LINE_FEED_t lineFeed;
#endif /* TAPI_VERSION4 */
   IFX_int32_t nMode;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   TD_PTR_CHECK(pPhone, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* no line feed for DECT */
      return IFX_SUCCESS;
   }
#endif /* TD_DECT */

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
      lineFeed.dev = pPhone->nDev;
      lineFeed.ch = pPhone->nPhoneCh;
      lineFeed.lineMode = mode;
      nMode = (IFX_int32_t)&lineFeed;
      nDevTmp = lineFeed.dev;
   }
   else
#endif /* TAPI_VERSION4 */
   {
      nMode = mode;
   }
   if (0 <= pPhone->nPhoneCh_FD)
   {
      ret = TD_IOCTL(pPhone->nPhoneCh_FD, IFX_TAPI_LINE_FEED_SET, nMode,
                     nDevTmp, pPhone->nSeqConnId);
      if (IFX_SUCCESS != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d: Line feed mode set to %s failed "
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber,
                Common_Enum2Name(mode, TD_rgLineModeName),
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Line mode changed to %s \n",
             pPhone->nPhoneNumber,
             Common_Enum2Name(mode, TD_rgLineModeName)));
   }
   return ret;
}

/**
   Moves string pointer to the filename part if passed string.

   \param sPath - string.

   \return none
*/
IFX_void_t Common_StripPath (IFX_char_t** sFileName)
{
   IFX_char_t* sBegin = *sFileName;

   if (*sFileName == IFX_NULL || strlen(*sFileName) == 0)
      return;

   /* move to the last character */
   *sFileName += strlen(*sFileName) - 1;

   /* only name of file without path */
   while (*sFileName > sBegin && *(*sFileName - 1) != 0x5C &&
      *(*sFileName - 1) != '/')
   {
      *sFileName -= 1;
   }
}

/**
   Return pointer to the CPU device structure from given board.

   \param pDevice - pointer to the device list
   \return pointer to the CPU device structure or IFX_NULL in case of error
*/
TAPIDEMO_DEVICE_t * Common_GetDevice_CPU(TAPIDEMO_DEVICE_t* pDevice)
{
   TAPIDEMO_DEVICE_t* pDev = pDevice;

   TD_PTR_CHECK(pDevice, IFX_NULL);

   do
   {
      if (pDev->nType == TAPIDEMO_DEV_CPU)
      {
         /* CPU device found. Return pointer to structure. */
         return pDev;
      }
      pDev = pDev->pNext;
   } while(pDev != IFX_NULL);

   return IFX_NULL;
}

/**
   Add device to the board.

   \param pBoard - pointer to the board
   \param pDevice - pointer to the device to add
   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t Common_AddDevice(BOARD_t* pBoard, TAPIDEMO_DEVICE_t* pDevice)
{
   TAPIDEMO_DEVICE_t* pDev = IFX_NULL;

   /* Check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pDevice, IFX_ERROR);

   if (pBoard->pDevice == IFX_NULL)
   {
      pBoard->pDevice = pDevice;
      return IFX_SUCCESS;
   }
   pDev = pBoard->pDevice;

   /* look for last element in the list */
   while (pDev->pNext != IFX_NULL)
   {
      pDev = pDev->pNext;
   }

   /* add device to the end of list */
   pDev->pNext = pDevice;
   return IFX_SUCCESS;
}

/**
   Remove device list from the board.

   \param pBoard - pointer to the board
   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t Common_RemoveDeviceList(BOARD_t* pBoard)
{
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_t *pNextDevice = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pDevice = pBoard->pDevice;
   while (pDevice != IFX_NULL)
   {
      pNextDevice = pDevice->pNext;
#ifdef FXO
      if (TAPIDEMO_DEV_FXO == pDevice->nType)
      {
         FXO_CleanUpFxoDevice(&pDevice->uDevice.stFxo);
      }
#endif /* FXO */
      TD_OS_MemFree(pDevice);
      pDevice = pNextDevice;
   }
   return IFX_SUCCESS;
}

/**
   Set Voice Activity Detection (VAD).

   \param pPhone - pointer to phone
   \param pBoard - pointer to the board
   \param nVadCfg - vad configuration to set,
        if RESTORE_DEFAULT then restore default settings

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t Common_SetVad(PHONE_t* pPhone, BOARD_t* pBoard,
                           IFX_int32_t nVadCfg)
{
   IFX_return_t ret = IFX_SUCCESS;
   VOIP_DATA_CH_t* pDataChStat = IFX_NULL;
#ifdef TAPI_VERSION4
   IFX_TAPI_ENC_VAD_CFG_t oVadCfg;
#endif /* TAPI_VERSION4 */
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_int32_t nFd;
   IFX_int32_t nVadCfgTmp;

   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      if (TD_NOT_SET != pPhone->nDataCh && RESTORE_DEFAULT != nVadCfg)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Phone No %d: Vad setting not used for DECT phone\n",
               pPhone->nPhoneNumber));
      }
      return IFX_SUCCESS;
   }
#endif /* TD_DECT */
#ifdef TAPI_VERSION4
#ifdef EASY336
   /* get codec data */
   if (pPhone->nVoipIDs > 0)
   {
      pDataChStat = Common_GetCodec(pBoard->pCtrl,
                       pPhone->pVoipIDs[pPhone->nVoipIDs - 1],
                       pPhone->nSeqConnId);
   }
   else if (RESTORE_DEFAULT != nVadCfg)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: No Voip ID - unable to change VAD settings  "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else
   {
      /* we are trying to restore default settings but no codec available,
         settings should be restored before releasing voip id */
      return IFX_SUCCESS;
   }
#endif /* EASY336 */
#else /* TAPI_VERSION4 */
   if (0 > pPhone->nDataCh || 0 >= pPhone->pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to get channel number of phone No %d "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* get used data channel statistics */
   pDataChStat = &pBoard->pDataChStat[pPhone->nDataCh];
#endif /* TAPI_VERSION4 */

   if (IFX_NULL == pDataChStat)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Failed to get codec. "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (RESTORE_DEFAULT == nVadCfg)
   {
      if (IFX_TRUE == pDataChStat->fDefaultVad)
      {
         return IFX_SUCCESS;
      }
      nVadCfg = pBoard->pCtrl->pProgramArg->nVadCfg;
   } /* if (IFX_FALSE == nVadCfg) */
   else
   {
      switch (nVadCfg)
      {
         case IFX_TAPI_ENC_VAD_NOVAD:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Phone No %d: Set VAD: %d - IFX_TAPI_ENC_VAD_NOVAD\n"
                   "%s(no voice activity detection).\n",
                   pPhone->nPhoneNumber, nVadCfg, pBoard->pIndentPhone));
            break;
         case IFX_TAPI_ENC_VAD_ON:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Phone No %d: Set VAD: %d - IFX_TAPI_ENC_VAD_ON\n"
                   "%s(voice activity detection on).\n",
                   pPhone->nPhoneNumber, nVadCfg, pBoard->pIndentPhone));
            break;
         case IFX_TAPI_ENC_VAD_G711:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Phone No %d: Set VAD: %d - IFX_TAPI_ENC_VAD_G711\n"
                   "%s(voice activity detection on without "
                   "spectral information).\n",
                   pPhone->nPhoneNumber, nVadCfg, pBoard->pIndentPhone));
            break;
         case IFX_TAPI_ENC_VAD_SC_ONLY:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Phone No %d: Set VAD: %d - IFX_TAPI_ENC_VAD_SC_ONLY\n"
                   "%s(voice activity detection on without "
                   "comfort noise generation).\n",
                   pPhone->nPhoneNumber, nVadCfg, pBoard->pIndentPhone));
            break;
         default:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: Invalid vad configuration - %d.\n",
                   pPhone->nPhoneNumber, nVadCfg));
            return IFX_ERROR;
            break;
      } /* switch (nVadCfg) */
   } /* if (IFX_FALSE == nVadCfg) */

#ifdef TAPI_VERSION4
   if (pBoard->fSingleFD)
   {
      BOARD_t *pSvipBoard = pBoard;
      /* Voice Activity Detection (VAD) configuration */
      oVadCfg.dev = pDataChStat->nDev;
      oVadCfg.ch = pDataChStat->nCh;
      oVadCfg.vadMode = nVadCfg;
      nDevTmp = pDataChStat->nDev;

      if (TYPE_XT16 == pBoard->nType)
      {
         pSvipBoard = ABSTRACT_GetBoard(pBoard->pCtrl,
                                        IFX_TAPI_DEV_TYPE_VIN_SUPERVIP);
         if (IFX_NULL == pSvipBoard)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Failed to get svip board structure. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
      nFd = pSvipBoard->nDevCtrl_FD;
      nVadCfgTmp = (IFX_int32_t) &oVadCfg;
   }
   else
#endif /* TAPI_VERSION4 */
   {
      nFd = pPhone->nDataCh_FD;
      nVadCfgTmp = nVadCfg;
   }
   /* Voice Activity Detection (VAD) configuration */
   ret =  TD_IOCTL(nFd, IFX_TAPI_ENC_VAD_CFG_SET, nVadCfg,
                      nDevTmp, pPhone->nSeqConnId);
   TD_RETURN_IF_ERROR(ret);

   /* set vad configuration flag to restore vad settings */
   if (nVadCfg == pBoard->pCtrl->pProgramArg->nVadCfg)
   {
      pDataChStat->fDefaultVad = IFX_TRUE;
   }
   else
   {
      pDataChStat->fDefaultVad = IFX_FALSE;
   }

   return IFX_SUCCESS;
}

/**
    function to return the name assigned to a enum value

   \param   nEnum  -  enum value (id)
   \param   pEnumName -  pointer to array with enum name

   \return  pointer to enum name. If enum value is not found or in case of
            error, pointer to string with error description is returned.
*/
IFX_char_t* Common_Enum2Name(IFX_int32_t nEnum, TD_ENUM_2_NAME_t *pEnumName)
{
   IFX_char_t *pName = IFX_NULL;
   IFX_int32_t nDebugLevel = DBG_LEVEL_HIGH;

   /* check pointer, on error return pointer to error string */
   TD_PTR_CHECK(pEnumName, TD_rgEnum2NameErrorCode[ENUM_NULL_POINTER]);

   /* search for nEnum value */
   do
   {
      /* check if value is the same */
      if (pEnumName->nID == nEnum)
      {
         /* set enum name string */
         pName = pEnumName->rgName;
      }
      pEnumName++;
   } while ((pName == IFX_NULL) && (pEnumName->nID != TD_MAX_ENUM_ID));

   if (g_pITM_Ctrl->fITM_Enabled)
   {
      /* printing high level traces cause test failure,
         lack of enum name string shouldn't cause test failure,
         because of that this workaround is used,
         this is usefull e.g. when new cappabilities are added and new string
         isn't added */
      nDebugLevel = DBG_LEVEL_NORMAL;
   }

   if (pName == IFX_NULL)
   {
      /* error - enum name was not found */
      TD_TRACE(TAPIDEMO, nDebugLevel, TD_CONN_ID_DBG,
            ("Warning, Name for enum value (0x%x) type %s not found in the table.\n",
             nEnum, pEnumName->rgName));
      /* if pName is IFX_NULL, it means that pEnumName points to last
         element in the table. Assign string with error info. */
      pName = TD_rgEnum2NameErrorCode[ENUM_VALUE_NOT_FOUND];
   }
   return pName;
}

/**
   Function to return the number of enum assigned to a enum name.

   \param   pString  -  name of enum to be found
   \param   pEnumName -  pointer to array with enum name
   \param   bPrintError -  if IFX_TRUE error message is printed
                           if name not found

   \return  number of enum from pEnumName table or
            IFX_ERROR if enum number not found.
*/
IFX_int32_t Common_Name2Enum(IFX_char_t* pString, TD_ENUM_2_NAME_t *pEnumName,
                             IFX_boolean_t bPrintError)
{
   /* check pointer, on error return pointer to error string */
   TD_PTR_CHECK(pEnumName, IFX_ERROR);
   TD_PTR_CHECK(pString, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= strlen(pString)), strlen(pString), IFX_ERROR);

   /* search for nEnum value */
   do
   {
      /* check if value is the same */
      if (0 == strcmp(pString, pEnumName->rgName))
      {
         /* return enum number */
         return pEnumName->nID;
      }
      pEnumName++;
   } while (pEnumName->nID != TD_MAX_ENUM_ID);

   if (IFX_TRUE == bPrintError)
   {
      /* error - enum number was not found */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Number of enum %s for string %s was not found.\n",
             pEnumName->rgName, pString));
   }

   return IFX_ERROR;
}

/**
   Function to map phones for local call. It accespts analog phones,
   DECT phones and phones on extension board (vmmc on main board,
   DuSLIC-xT on extension).

   \param   pSrcPhone   -  pointer to phone structure that will be mapped to
   \param   pDstPhone   -  pointer to phone structure that will be mapped
   \param   nMapFlag    -  TD_MAP_ADD if phones should be mapped,
                           TD_MAP_REMOVE if phones should be unmapped

   \return   IFX_SUCCESS if ok, otherwise IFX_ERROR.
*/
IFX_return_t Common_MapLocalPhones(PHONE_t* pSrcPhone, PHONE_t* pDstPhone,
                                   TD_MAP_ADD_REMOVE_t nMapFlag)
{
   BOARD_t *pUsedBoard = IFX_NULL;
   IFX_int32_t i;
   IFX_int32_t nPhoneA = TD_NOT_SET, nPhoneB = TD_NOT_SET;
   IFX_int32_t nChannelA = TD_NOT_SET, nChannelB = TD_NOT_SET;
   PHONE_t *pPhoneA = IFX_NULL, *pPhoneB = IFX_NULL;
   TD_LOCAL_MAPPING_TYPE_t nMappingType = TD_MAP_UNKNOWN_2_UNKNOWN;
   IFX_return_t nRet = IFX_ERROR;
   /* static variables */
   static IFX_boolean_t bMappingTableInitialized = IFX_FALSE;
   static TD_LOCAL_MAPPING_t oLocalMappingArray[TD_MAX_LOCAL_MAPPINGS];
   static IFX_int32_t nMappingCnt = 0;

   /* check input parameters */
   TD_PTR_CHECK(pSrcPhone, IFX_ERROR);
   TD_PTR_CHECK(pDstPhone, IFX_ERROR);
   TD_PARAMETER_CHECK(((TD_MAP_REMOVE != nMapFlag) && (TD_MAP_ADD != nMapFlag)),
                      nMapFlag, IFX_ERROR);

   /* check if mapping table initialized */
   if (IFX_TRUE != bMappingTableInitialized)
   {
      /* for all elements */
      for (i=0; i<TD_MAX_LOCAL_MAPPINGS; i++)
      {
         /* reset values */
         oLocalMappingArray[i].nPhoneA = TD_NOT_SET;
         oLocalMappingArray[i].nPhoneB = TD_NOT_SET;
         oLocalMappingArray[i].nType = TD_MAP_UNKNOWN_2_UNKNOWN;
      } /* for all elements */
      /* set table initialized flag */
      bMappingTableInitialized = IFX_TRUE;
   } /* check if mapping table initialized */


   /* check if unmapping and remove mapping from table
      also get mapping type and channel numbers*/
   if (TD_MAP_REMOVE == nMapFlag)
   {
      /* for all elements */
      for (i=0; i<TD_MAX_LOCAL_MAPPINGS; i++)
      {
         /* check if for this element phone pair is set */
         if (TD_NOT_SET != oLocalMappingArray[i].nPhoneA)
         {
            /* check if this is the element we used to save mapping info */
            if ( (pSrcPhone->nPhoneNumber == oLocalMappingArray[i].nPhoneA &&
                  pDstPhone->nPhoneNumber == oLocalMappingArray[i].nPhoneB) ||
                 (pSrcPhone->nPhoneNumber == oLocalMappingArray[i].nPhoneB &&
                  pDstPhone->nPhoneNumber == oLocalMappingArray[i].nPhoneA) )
            {
               /* get mapping data for unmapping */
               nPhoneA = oLocalMappingArray[i].nPhoneA;
               nChannelA = oLocalMappingArray[i].nChannelA;
               nPhoneB = oLocalMappingArray[i].nPhoneB;
               nChannelB = oLocalMappingArray[i].nChannelB;
               nMappingType = oLocalMappingArray[i].nType;
               pUsedBoard = oLocalMappingArray[i].pBoard;
               /* reset data */
               oLocalMappingArray[i].nPhoneA = TD_NOT_SET;
               oLocalMappingArray[i].nPhoneB = TD_NOT_SET;
               oLocalMappingArray[i].nType = TD_MAP_UNKNOWN_2_UNKNOWN;
               /* check if valid mapping count */
               if (0 < nMappingCnt)
               {
                  /* decrease mapping count */
                  nMappingCnt--;
               } /* check if valid mapping count */
               else
               {
                  /* this shoul not occur */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pSrcPhone->nSeqConnId,
                        ("Err, invalid mapping number count."
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
               } /* check if valid mapping count */
               /* leave the loop */
               break;
            } /* check if this is the element we used to save mapping info */
         } /* check if for this element phone pair is set */
      } /* for all elements */
      /* check if mapping was found */
      if (i == TD_MAX_LOCAL_MAPPINGS)
      {
         /* do not return error if unmapping was already done */
         return IFX_SUCCESS;
      }
   } /* if (TD_MAP_REMOVE == nMapFlag) */
   else if (TD_MAP_ADD == nMapFlag)
   {
      /* set board - mapped channels must be on the same board */
      pUsedBoard = pSrcPhone->pBoard;
      /* check if max number of mapping is reached */
      if (nMappingCnt >= TD_MAX_LOCAL_MAPPINGS)
      {
         /* cannot make more than TD_MAX_LOCAL_MAPPINGS mappings */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pSrcPhone->nSeqConnId,
               ("Err, Unable to do local mapping for "
                "Phone No %d and Phone No %d.\n"
                "     Max number of mappings(%d) reached"
                "(File: %s, line: %d)\n",
                pSrcPhone->nPhoneNumber, pDstPhone->nPhoneNumber,
                nMappingCnt, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* do while() is used to get similar functionality like goto */
      do
      {
#ifdef TD_DECT
         /* check if there are any DECT phones */
         if (PHONE_TYPE_DECT == pSrcPhone->nType)
         {
            pPhoneA = pSrcPhone;
         }
         if (PHONE_TYPE_DECT == pDstPhone->nType)
         {
            if (IFX_NULL == pPhoneA)
            {
               pPhoneA = pDstPhone;
            }
            else if (IFX_NULL == pPhoneB)
            {
               pPhoneB = pDstPhone;
            }
         }
#endif /* TD_DECT */
#ifdef EASY3111
         /* checki if phones on extension board,
         assumption is made that on DuSLIC-xT board there are no DECT phones */
         if (TYPE_DUSLIC_XT == pSrcPhone->pBoard->nType)
         {
            pUsedBoard = TD_GET_MAIN_BOARD(pSrcPhone->pBoard->pCtrl);
            if (IFX_NULL == pPhoneA)
            {
               pPhoneA = pSrcPhone;
            }
            else if (IFX_NULL == pPhoneB)
            {
               pPhoneB = pSrcPhone;
            }
         }
         else if (TYPE_DUSLIC_XT == pDstPhone->pBoard->nType)
         {
            if (IFX_NULL == pPhoneA)
            {
               pPhoneA = pDstPhone;
            }
            else if (IFX_NULL == pPhoneB)
            {
               pPhoneB = pDstPhone;
            }
         } /* if (TD_NOT_SET != nPCM_Ch) */
#endif /* EASY3111 */
         /* now analog phones are set */
         if (IFX_NULL == pPhoneA)
         {
            if (pPhoneB != pSrcPhone)
            {
               pPhoneA = pSrcPhone;
            }
            else if (pPhoneB != pDstPhone)
            {
               pPhoneA = pDstPhone;
            }
         } /* if (IFX_NULL == pPhoneA) */
         if (IFX_NULL == pPhoneB)
         {
            if (pPhoneA != pSrcPhone)
            {
               pPhoneB = pSrcPhone;
            }
            else if (pPhoneA != pDstPhone)
            {
               pPhoneB = pDstPhone;
            }
         } /* if (IFX_NULL == pPhoneB) */
         /* check if phones are set */
         if (IFX_NULL == pPhoneA || IFX_NULL == pPhoneB)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pSrcPhone->nSeqConnId,
                  ("Err, Invalid phones (%d, %d).\n"
                   "(File: %s, line: %d)\n",
                   (IFX_int32_t) pPhoneA, (IFX_int32_t) pPhoneB,
                   __FILE__, __LINE__));
            break;
         }
         /* get phone numbers */
         nPhoneA = pPhoneA->nPhoneNumber;
         nPhoneB = pPhoneB->nPhoneNumber;
#ifdef TD_DECT
         /* get mapping type if at least one phone is DECT */
         if (PHONE_TYPE_DECT == pPhoneA->nType)
         {
            nChannelA = pPhoneA->nDectCh;
            /* check second phone type */
            if (PHONE_TYPE_DECT == pPhoneB->nType)
            {
               nChannelB = pPhoneB->nDectCh;
               nMappingType = TD_MAP_DECT_2_DECT;
            }
#ifdef EASY3111
            else if (TYPE_DUSLIC_XT == pPhoneB->pBoard->nType)
            {
               nChannelB = pPhoneB->oEasy3111Specific.nOnMainPCM_Ch;
               nMappingType = TD_MAP_DECT_2_PCM;
            }
#endif /* EASY3111 */
            else
            {
               nChannelB = pPhoneB->nPhoneCh;
               nMappingType = TD_MAP_DECT_2_PHONE;
            }
         } /* if (PHONE_TYPE_DECT == pPhoneA->nType) */
         else
#endif /* TD_DECT */
#ifdef EASY3111
         /* check if at least one phone is on DxT */
         if (TYPE_DUSLIC_XT == pPhoneA->pBoard->nType)
         {
            nChannelA = pPhoneA->oEasy3111Specific.nOnMainPCM_Ch;
            /* check second phone type */
            if (TYPE_DUSLIC_XT == pPhoneB->pBoard->nType)
            {
               nChannelB = pPhoneB->oEasy3111Specific.nOnMainPCM_Ch;
               nMappingType = TD_MAP_PCM_2_PCM;
            }
            else
            {
               nChannelB = pPhoneB->nPhoneCh;
               nMappingType = TD_MAP_PCM_2_PHONE;
            }
         } /* if (TYPE_DUSLIC_XT == pPhoneA->pBoard->nType) */
         else
#endif /* EASY3111 */
         {
            nChannelA = pPhoneA->nPhoneCh;
            nChannelB = pPhoneB->nPhoneCh;
            nMappingType = TD_MAP_PHONE_2_PHONE;
         }
      }
      while (0);
   } /* else if (TD_MAP_ADD == nMapFlag) */
   /* handle mapping type */
   switch (nMappingType)
   {
#ifdef TD_DECT
      case TD_MAP_DECT_2_DECT:
         nRet = TD_DECT_MapToDect(nChannelA, nChannelB, nMapFlag, pUsedBoard,
                   pSrcPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(nRet, "TD_DECT_MapToDect", pSrcPhone->nSeqConnId);
         break;
#ifdef EASY3111
      case TD_MAP_DECT_2_PCM:
         nRet = TD_DECT_MapToPCM(nChannelA, nChannelB, nMapFlag, pUsedBoard,
                                 pSrcPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(nRet, "TD_DECT_MapToPCM", pSrcPhone->nSeqConnId);
         break;
#endif /* EASY3111 */
      case TD_MAP_DECT_2_PHONE:
         nRet = TD_DECT_MapToAnalog(nChannelA, nChannelB, nMapFlag, pUsedBoard,
                   pSrcPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(nRet, "TD_DECT_MapToAnalog", pSrcPhone->nSeqConnId);
         break;
#endif /* TD_DECT */
#ifdef EASY3111
      case TD_MAP_PCM_2_PCM:
         nRet = PCM_MapToPCM(nChannelA, nChannelB,
                   ((nMapFlag == TD_MAP_ADD) ? IFX_TRUE : IFX_FALSE),
                   pUsedBoard, pSrcPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(nRet, "PCM_MapToPCM", pSrcPhone->nSeqConnId);
         break;
      case TD_MAP_PCM_2_PHONE:
         nRet = PCM_MapToPhone(nChannelA, nChannelB,
                   ((nMapFlag == TD_MAP_ADD) ? IFX_TRUE : IFX_FALSE),
                   pUsedBoard, pSrcPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(nRet, "PCM_MapToPhone", pSrcPhone->nSeqConnId);
         break;
#endif /* EASY3111 */
      case TD_MAP_PHONE_2_PHONE:
         nRet = ALM_MapToPhone(nChannelA, nChannelB, nMapFlag, pUsedBoard,
                               pSrcPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(nRet, "ALM_MapToPhone", pSrcPhone->nSeqConnId);
         break;
      case TD_MAP_UNKNOWN_2_UNKNOWN:
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pSrcPhone->nSeqConnId,
               ("Err, unhandled mapping type %d.\n"
                "(File: %s, line: %d)\n",
                nMappingType, __FILE__, __LINE__));
         break;
   } /* handle mapping type */
   /* check if successfull mapping was done */
   if (IFX_SUCCESS != nRet)
   {
      /* at least one of mappings should be done already,
         if not, then error occured */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pSrcPhone->nSeqConnId,
            ("Err, Unable to do local %smapping for "
             "Phone No %d and Phone No %d.\n"
             "(File: %s, line: %d)\n",
             (nMapFlag != TD_MAP_ADD) ? "un" : "",
             pSrcPhone->nPhoneNumber, pDstPhone->nPhoneNumber,
             __FILE__, __LINE__));
      return IFX_ERROR;
   } /* if (IFX_SUCCESS != nRet) */
   /* add mapping to mapping table */
   else if (nMapFlag == TD_MAP_ADD)
   {
      /* look for unused mapping info element */
      for (i=0; i<TD_MAX_LOCAL_MAPPINGS; i++)
      {
         /* phone number is set to TD_NOT_SET if element is not used */
         if (TD_NOT_SET == oLocalMappingArray[i].nPhoneA)
         {
            /* save mapped phones numbers */
            oLocalMappingArray[i].nPhoneA = nPhoneA;
            oLocalMappingArray[i].nChannelA = nChannelA;
            oLocalMappingArray[i].nPhoneB = nPhoneB;
            oLocalMappingArray[i].nChannelB = nChannelB;
            oLocalMappingArray[i].nType = nMappingType;
            oLocalMappingArray[i].pBoard = pUsedBoard;
            nMappingCnt++;
            break;
         } /* phone number is set to TD_NOT_SET if element is not used */
      } /* look for unused mapping info element */
   } /* if (IFX_SUCCESS != nRet) */

   return nRet;
}

/**
   Check if value is a number.

   \param pValue - value to check

   \return true or false
*/
IFX_boolean_t Common_IsNumber(IFX_char_t* pValue)
{
   IFX_int32_t i;
   IFX_char_t c;

   /* check input parameters */
   TD_PTR_CHECK(pValue, IFX_FALSE);

   if(0 >= strlen(pValue))
   {
      return IFX_FALSE;
   }

   /* Check every element of array */
   for (i=0; i < strlen(pValue); i++)
   {
      c = pValue[i];
      if( !isdigit(c) )
      {
         return IFX_FALSE;
      }
   } /* for */

   return IFX_TRUE;
} /* IsNumber */

/**
   Set local call type depending on position (main/extension board, board type)
   of phones.

   \param pValue - value to check

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t Common_SetLocalCallType(PHONE_t *pPhoneSrc, PHONE_t *pPhoneDst,
                                     CONNECTION_t *pConn)
{
   /* check input parameters */
   TD_PTR_CHECK(pPhoneSrc, IFX_ERROR);
   TD_PTR_CHECK(pPhoneDst, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

#ifndef EASY3111
#ifndef EASY336
   /* with EASY336 xT-16 boards are considered to be on a
    * different board (abstraction). Here we should ignore this. */
   if (pPhoneDst->pBoard != pPhoneSrc->pBoard)
   {
      /* Local calls, but between two boards. */
      if ((pPhoneSrc->pBoard->nType == TYPE_DUSLIC_XT) ||
          (pPhoneDst->pBoard->nType == TYPE_DUSLIC_XT))
      {
         /* DXT supports only PCM */
         pConn->nType = LOCAL_BOARD_PCM_CALL;
      }
      else
      {
         /* Can use coders for connection */
         pConn->nType = LOCAL_BOARD_CALL;
      }
   } /* if (pPhoneDst->pBoard != pPhoneSrc->pBoard) */
   else
#endif /* EASY336 */
   {
      /* Making connection on same board */
      if (pPhoneSrc->pBoard->nType == TYPE_DUSLIC_XT
#ifdef XT16
          /* XT16 is defined only when compiled tapidemo for XT16 board,
            XT16 is not defined when compiled tapidemo for EASY336(SVIP)
            with XT16 extension board however XT16 board is used used */
          || pPhoneSrc->pBoard->nType == TYPE_XT16
#endif /* XT16 */
          )
      {
         /* Can use only PCM channels */
         pConn->nType = LOCAL_PCM_CALL;

      }
      else
      {
         /* Can use coder channels, but actually we will perform
            only phone ch mapping. When enabling local QOS,
            coders will be used. */
         pConn->nType = LOCAL_CALL;
      }
   } /* if (pDstPhone->pBoard != pPhone->pBoard) */
#else /* EASY3111 */
   /* special call type for vinetic used as extension board */
   if ( ( (pPhoneSrc->pBoard->nType == TYPE_VINETIC) ||
          (pPhoneDst->pBoard->nType == TYPE_VINETIC) ) &&
        (pPhoneDst->pBoard->nType != pPhoneSrc->pBoard->nType) )
   {
      /* Can use coders for connection */
      pConn->nType = LOCAL_BOARD_CALL;
   }
   else
   {
/* when EASY 3111 is used as extension board for vmmc boards (e.g. EASY 50812),
   then for all scenarios above it is a "plain" LOCAL_CALL,
   for some of those scenarios PCM channles are also used for LOCAL_CALL,
   this is done to simpilfy code */
      pConn->nType = LOCAL_CALL;
   }
#endif  /* EASY3111 */

   return IFX_SUCCESS;
} /* IsNumber */

/**
   Initialize synchronization mechanism and start thread.

   \param pThreadCtrl     - Pointer to thread control structure.
   \param pFun            - Specifies the user entry function of the thread.
   \param pThreadName     - Specifies thread name.
   \param pSynchPipe      - Pointer to synch pipe.
   \param pLockThreadStop - Pointer to lock thread stop.

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t Common_StartThread(TD_OS_ThreadCtrl_t* pThreadCtrl,
                                IFX_int32_t (*pFun)(TD_OS_ThreadParams_t*),
                                IFX_char_t* pThreadName,
                                TD_PIPE_t* pSynchPipe,
                                TD_OS_lock_t* pLockThreadStop)
{

#ifdef VXWORKS
   IFX_uint32_t   nStackSize = 20000;
   IFX_uint32_t   nPriority = 70;
#else
   IFX_uint32_t   nStackSize = 0;
   IFX_uint32_t   nPriority = 0;
#endif

   TD_PTR_CHECK(pThreadCtrl, IFX_ERROR);
   TD_PTR_CHECK(pFun, IFX_ERROR);
   TD_PTR_CHECK(pThreadName, IFX_ERROR);

   if (IFX_NULL != pSynchPipe)
   {
      /*******************************************************************
            Prepare pipe for thread synch
       *******************************************************************/
      sprintf(pSynchPipe->rgName, "%s", pThreadName);
      if (IFX_SUCCESS != Common_PreparePipe(pSynchPipe))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Failed to prepare pipe for %s.\n"
                "(File: %s, line: %d)\n", pThreadName, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   if (IFX_NULL != pLockThreadStop)
   {
      /********************************************************
            Init lock which will be waked-up then thread stops working
       ********************************************************/
      if (IFX_SUCCESS != TD_OS_LockInit(pLockThreadStop))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, TD_OS_LockInit for %s failed.\n"
                "(File: %s, line: %d)\n", pThreadName, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   /********************************************************
         Start thread
    ********************************************************/
   if (IFX_SUCCESS != TD_OS_ThreadInit (pThreadCtrl, pThreadName, pFun,
                                         nStackSize, nPriority, 0, 0))
   {
       TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
             ("Err, TD_OS_ThreadInit for %s failed.\n"
              "(File: %s, line: %d)\n", pThreadName, __FILE__, __LINE__));
       return IFX_ERROR;
   }
   return IFX_SUCCESS;
} /* Common_StartThread */

/**
   Clean synchronization mechanism and stop thread using library IFX OS.

   \param pThreadCtrl     - Pointer to thread control structure.
   \param pSynchPipe      - Pointer to synch pipe.
   \param pLockThreadStop - Pointer to lock thread stop.

   \return
*/
IFX_void_t Common_StopThread(TD_OS_ThreadCtrl_t* pThreadCtrl,
                               TD_PIPE_t* pSynchPipe,
                               TD_OS_lock_t* pLockThreadStop)
{
   IFX_char_t pThreadName[MAX_THREAD_NAME];

   TD_PTR_CHECK(pThreadCtrl,);

   strcpy(pThreadName, pThreadCtrl->thrParams.pName);

   if ((IFX_NULL != pSynchPipe) && (IFX_NULL != pLockThreadStop))
   {
      /* Send message to stop thread */
      TD_OS_PipePrintf(pSynchPipe->rgFp[TD_PIPE_IN], "%d",
                       TD_SYNCH_THREAD_MSG_STOP);

      /* Wait until thread is stopped */
      if (IFX_SUCCESS != TD_OS_LockGet(pLockThreadStop))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, TD_OS_LockGet for %s failed.\n"
               "(File: %s, line: %d)\n", pThreadName, __FILE__, __LINE__));
      }
   }
   /* Delete thread */
   if (IFX_SUCCESS != TD_OS_ThreadDelete(pThreadCtrl, THREAD_SELF_SHUTDOWN_TIME))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, TD_OS_ThreadDelete for %s failed.\n"
            "(File: %s, line: %d)\n", pThreadName, __FILE__, __LINE__));
   }
   /* Delecte lock */
   if (IFX_NULL != pLockThreadStop)
   {
      /* Delete lock */
      if (IFX_SUCCESS != TD_OS_LockRelease(pLockThreadStop))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, TD_OS_LockRelease for %s failed.\n"
               "(File: %s, line: %d)\n", pThreadName, __FILE__, __LINE__));
      }

      if (IFX_SUCCESS != TD_OS_LockDelete(pLockThreadStop))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, TD_OS_EventDelete for %s failed.\n"
               "(File: %s, line: %d)\n", pThreadName, __FILE__, __LINE__));
      }
   }
   /* Delecte pipe */
   if (IFX_NULL != pSynchPipe)
   {
      /* Close pipe */
      if (IFX_SUCCESS != TD_OS_PipeClose(pSynchPipe->rgFp[TD_PIPE_IN]))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, TD_OS_PipeClose IN for %s failed.\n"
               "(File: %s, line: %d)\n", pThreadName, __FILE__, __LINE__));
      }
      if (IFX_SUCCESS != TD_OS_PipeClose(pSynchPipe->rgFp[TD_PIPE_OUT]))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, TD_OS_PipeClose OUT for %s failed.\n"
               "(File: %s, line: %d)\n",
               pThreadName, __FILE__, __LINE__));
      }
#ifdef LINUX
      /* Remove pipe */
      remove(pSynchPipe->rgName);
#elif VXWORKS
      /* Remove pipe */
      pipeDevDelete(pSynchPipe->rgName, IFX_FALSE);
#endif /* LINUX */
   }
} /* Common_StopThread */

/**
   Prepare pipe using IFX OS.

   Create pipe, open pipe for read and write, disable buffering for pipe.

   \param pPipe - pipe file pointer.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t Common_PreparePipe(TD_PIPE_t* pPipe)
{
   if (IFX_SUCCESS != TD_OS_PipeCreate(pPipe->rgName))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to create pipe.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   pPipe->rgFp[TD_PIPE_OUT] =
      TD_OS_PipeOpen(pPipe->rgName, IFX_TRUE, IFX_FALSE);
   if (IFX_NULL == pPipe->rgFp[TD_PIPE_OUT])
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to open pipe: reading.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Map stream pointer to file descriptor  */
   pPipe->rgFd[TD_PIPE_OUT] =
      fileno(pPipe->rgFp[TD_PIPE_OUT]);
   if (NO_PIPE == pPipe->rgFd[TD_PIPE_OUT])
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to get fd: reading.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   pPipe->rgFp[TD_PIPE_IN] =
      TD_OS_PipeOpen(pPipe->rgName, IFX_FALSE, IFX_TRUE);
   if (IFX_NULL == pPipe->rgFp[TD_PIPE_IN])
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to open pipe writing\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Map stream pointer to file descriptor  */
   pPipe->rgFd[TD_PIPE_IN] =
      fileno(pPipe->rgFp[TD_PIPE_IN]);
   if (NO_PIPE == pPipe->rgFd[TD_PIPE_IN])
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to get fd: writing.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Disable buffering for the stream, which becomes an unbuffered stream. */
   setbuf(pPipe->rgFp[TD_PIPE_IN], IFX_NULL);

   return IFX_SUCCESS;
} /* Common_PreparePipe */


#ifdef LINUX
/**
    Function check if trace is not too long.

   \param   pBuffer2  -  trace

   \return
*/
IFX_void_t Common_CheckTraceLen(IFX_char_t* pBuffer2)
{
   IFX_char_t* pPresentCh;
   IFX_char_t* pPreviusCh;

   /* If there isn't IFX_char_t '\n' at the end of string, some data can be missed */
   IFX_char_t pBuffer[strlen(pBuffer2+1)];
   sprintf(pBuffer, "%s\n", pBuffer2);

   /* Locate first occurrence of character '\n' */
   pPresentCh = strchr(pBuffer, TD_EOL);

   pPreviusCh = pBuffer;

   while (pPresentCh != NULL)
   {
      if (pPresentCh-pPreviusCh+1 > MAX_TRACE_LENGTH)
      {
         TD_PRINTF("Above trace is too long: %d\n", pPresentCh-pPreviusCh+1);
      }

      pPreviusCh = pPresentCh+1;
      /* Locate next occurrence of character '\n' */
      pPresentCh = strchr(pPreviusCh, TD_EOL);
   } /* while */
}
#endif /* LINUX */

#ifdef DANUBE
/**
   Open GPIO port.

   \param pGpioPortName - GPIO port name

   \return Handle to the GPIO port or NO_GPIO_FD if unable to get FD
*/
IFX_int32_t Common_GPIO_OpenPort(IFX_char_t* pGpioPortName)
{
   IFX_int32_t nGPIO_PortFd = NO_GPIO_FD;

   TD_PTR_CHECK(pGpioPortName, NO_GPIO_FD);

   nGPIO_PortFd = Common_Open(pGpioPortName, IFX_NULL);
   if (nGPIO_PortFd < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Open GPIO port (%s): %s.\n(File: %s, line: %d)\n",
             pGpioPortName, strerror(errno), __FILE__, __LINE__));
      return NO_GPIO_FD;
   }
   return nGPIO_PortFd;
} /* Common_GPIO_OpenPort */

/**
   Close GPIO port.

   \param pGpioPortName - GPIO port name
   \param nGPIO_PortFd - Handle to the GPIO port

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t Common_GPIO_ClosePort(IFX_char_t* pGpioPortName,
                                   IFX_int32_t nGPIO_PortFd)
{
   TD_PTR_CHECK(pGpioPortName, IFX_ERROR);
   TD_PARAMETER_CHECK((NO_GPIO_FD >= nGPIO_PortFd), nGPIO_PortFd, IFX_ERROR);

   if (IFX_SUCCESS != TD_OS_DeviceClose(nGPIO_PortFd))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Close GPIO port(%s): %s.\n(File: %s, line: %d)\n",
             pGpioPortName, strerror(errno),  __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
} /* Common_GPIO_ClosePort */

/*
   Reserve GPIO pin.

   \param nFd - GPIO port file descriptor.
   \param nPort - GPIO port.
   \param nPin - GPIO pin.
   \param nModule - GPIO module ID.

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR

   \remark It's not used for old bsp
 */
IFX_return_t Common_GPIO_ReservePin(IFX_int32_t nFd, IFX_int32_t nPort,
                                    IFX_int32_t nPin, IFX_int32_t nModule)
{
#ifndef OLD_BSP
   IFX_return_t nRet;
   struct ifx_gpio_ioctl_pin_reserve reserve = {0};

   TD_PARAMETER_CHECK((NO_GPIO_FD >= nFd), nFd, IFX_ERROR);

   memset(&reserve, 0, sizeof (reserve));
   reserve.pin = IFX_GPIO_PIN_ID(nPort, nPin);
   reserve.module_id = nModule;
   nRet = TD_IOCTL(nFd, IFX_GPIO_IOC_PIN_RESERVE, &reserve,
                      TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, ioctl IFX_GPIO_IOC_PIN_RESERVE, (port %d, pin %d) failed.\n",
             nPort, nPin));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
#else
   return IFX_SUCCESS;
#endif /* OLD_BSP */
}

/*
   Free GPIO pin.

   \param nFd - GPIO port file descriptor.
   \param nPort - GPIO port.
   \param nPin - GPIO pin.
   \param nModule - GPIO module ID.

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR

   \remark It's not used for old bsp
*/
IFX_return_t Common_GPIO_FreePin(IFX_int32_t nFd, IFX_int32_t nPort,
                                 IFX_int32_t nPin, IFX_int32_t nModule)
{
#ifndef OLD_BSP
   IFX_return_t nRet;
   struct ifx_gpio_ioctl_pin_reserve reserve = {0};

   TD_PARAMETER_CHECK((NO_GPIO_FD >= nFd), nFd, IFX_ERROR);

   memset(&reserve, 0, sizeof (reserve));
   reserve.pin = IFX_GPIO_PIN_ID(nPort, nPin);
   reserve.module_id = nModule;
   nRet = TD_IOCTL(nFd, IFX_GPIO_IOC_PIN_FREE, &reserve,
                    TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, ioctl IFX_GPIO_IOC_PIN_FREE, (port %d, pin %d)\n",
             nPort, nPin));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
#else
   return IFX_SUCCESS;
#endif /* OLD_BSP */
}
#endif /* DANUBE */

/*
   Allocates a block of memory.

   \param memBlockNum - Number of elements to be allocated.
   \param memSize - Size of elements.

   \return A pointer to the memory block allocated by the function.
           If the function failed to allocate the requested block of memory,
           a IFX_NULL pointer is returned.
*/
IFX_void_t* Common_MemCalloc(IFX_size_t memBlockNum, IFX_size_t memSize)
{
   IFX_void_t *pMemBlock = IFX_NULL;

   pMemBlock = TD_OS_MemAlloc(memSize * memBlockNum);
   if(pMemBlock == IFX_NULL)
   {
      return IFX_NULL;
   }
   memset(pMemBlock, 0, memSize * memBlockNum);

   return pMemBlock;
} /* Common_MemCalloc */

#ifdef VXWORKS
size_t strnlen (__const char *__string, size_t __maxlen)
{
   return (strlen(__string) < __maxlen) ? strlen(__string) : __maxlen;
}
#endif

#ifdef LINUX
/**
   This function creates a FIFO with a given name.

   \param pcName - Pointer to the name of the FIFO.

   \return Descriptor to the FIFO.
*/
IFX_int32_t Common_CreateFifo(IFX_char_t * pcName)
{
   IFX_int32_t iRet = 0;
   /* Check for the existence of FIFO */
   if (access((pcName), F_OK) == -1)
   {
      iRet = mkfifo((pcName), 0666);
   }/*end of if*/
   return iRet;
}

/**
   This function opens a FIFO with a given name.
   If Fifo doesnt exist, it is created first.

   \param pcName - Pointer to the name of the FIFO.
   \param iFlags - Flags

   \return Descriptor to the FIFO.
*/
IFX_int32_t Common_OpenFifo(IFX_char_t * pcName, IFX_int32_t iFlags)
{
   IFX_int32_t iRet = 0;
   /* Check for the existence of FIFO */
   if (access((pcName), F_OK) == -1)
   {
      iRet = mkfifo((pcName), 0666);
      if (iRet < 0)
      {
         return iRet;
      }
   }/*end of if*/
   iRet = open((pcName), iFlags);
   return iRet;
}
#endif

#ifdef TD_IPV6_SUPPORT
/**
   Verify phonebook IP address and set family value.

   \param pWord         - word to check
   \param pAddrFamily   - return family name

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
   \remarks
*/
IFX_return_t Common_VerifyAddrIP(IFX_char_t *pWord, IFX_int32_t *pAddrFamily)
{
   IFX_int32_t nRet;
   struct in_addr sin_addr;
   struct in6_addr sin6_addr;

   nRet = inet_pton(AF_INET, pWord, &(sin_addr));
   if(0 < nRet)
   {
      *pAddrFamily = AF_INET;
      return IFX_SUCCESS;
   }

   nRet = inet_pton(AF_INET6, pWord, &(sin6_addr));
   if(0 < nRet)
   {
      *pAddrFamily = AF_INET6;
      return IFX_SUCCESS;
   }

   return IFX_ERROR;
} /* Common_VerifyAddrIP */

/**
   Get phonebook from file and update current phonebook.

   \param pCtrl - handle to connection control structure

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS

   \remark Phonebook syntax: <ip address> <boardname>
           <ip address> - mandatory, either IPv4 or IPv6 format
           <boardname> - optional, max len: 100
           e.g.: 192.168.1.1 EASY50712
*/
IFX_return_t Common_PhonebookGetFromFile(CTRL_STATUS_t* pCtrl)
{
   IFX_return_t nRet;
   TD_OS_File_t* pFp;
   IFX_int32_t cChar;
   /* Building work */
   IFX_char_t rgWord[MAX_WORD_LEN];
   /* Index of building work */
   IFX_int32_t nWordIndex = 0;
   /* True if word is building */
   IFX_boolean_t bWordBuilding = IFX_FALSE;
   /* True if error occurs */
   IFX_boolean_t bErrorIndicated = IFX_FALSE;
   /* Address family - we don't use it here */
   IFX_int32_t nAddrFamily;
   IFX_char_t rgAddressIP[TD_ADDRSTRLEN];
   IFX_char_t rgBoardname[TD_PHONEBOOK_BOARDNAME_LEN];
   IFX_boolean_t nUnusedBoolean = IFX_FALSE;

   if (!pCtrl->pProgramArg->oArgFlags.nUseCustomPhonebookPath)
   {
      /* Use default phone book */
      sprintf(pCtrl->pProgramArg->sPathToPhoneBook,
              "%s/%s", TD_PHONEBOOK_PATH, TD_PHONEBOOK_FILE);
   }

   /* Open file */
   pFp = TD_OS_FOpen(pCtrl->pProgramArg->sPathToPhoneBook, "r");
   if (IFX_NULL == pFp)
   {
      /* Failed to open file */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_PHONE_BOOK,
            ("Phonebook file \"%s\" is not available.\n",
             pCtrl->pProgramArg->sPathToPhoneBook));
      return IFX_ERROR;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_PHONE_BOOK,
            ("Using phonebook file: \"%s\".\n",
             pCtrl->pProgramArg->sPathToPhoneBook));
   }

   /* Parse phonebook */
   memset(rgWord, 0, sizeof(rgWord));
   memset(rgAddressIP, 0, sizeof(rgAddressIP));
   memset(rgBoardname, 0, sizeof(rgBoardname));
   strcpy(rgBoardname, TD_NO_NAME);

   while(1)
   {
      /* Get char from file */
      cChar = fgetc(pFp);
      if (cChar == ' ')
      {
         if(bWordBuilding)
         {
            if((strlen(rgAddressIP) > 0) && (strlen(rgBoardname) > 0))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
                     ("Err, Phonebook: incorrect syntax detected,\n"
                      "third word detected after %s %s. (File: %s, line: %d)\n",
                      rgAddressIP, rgWord, __FILE__, __LINE__));
               bErrorIndicated = IFX_TRUE;
               break;
            }

            /* First word in the line must be IP address */
            nRet = Common_VerifyAddrIP(rgWord, &nAddrFamily);
            if (IFX_SUCCESS != nRet)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
                     ("Err, Phonebook: incorrect syntax detected.\n"
                      "Expected IP address, but found: %s.\n"
                      "(File: %s, line: %d)\n",  rgWord, __FILE__, __LINE__));
               bErrorIndicated = IFX_TRUE;
               break;
            }
            strncpy(rgAddressIP, rgWord, TD_ADDRSTRLEN - 1);

            /* Reset building word */
            bWordBuilding = IFX_FALSE;
            nWordIndex = 0;
            memset(rgWord, 0, sizeof(rgWord));
         } /* if(bWordBuilding) */
      }
      else if ((cChar == TD_EOL) || cChar == EOF)
      {
         if(bWordBuilding)
         {
            /* In one line, there can be either only IP adddress or
               IP address and boardname */

            if(strlen(rgWord) == 0)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
                     ("Err, Phonebook: detect incorrect syntax.\n"
                      "(File: %s, line: %d)\n",  __FILE__, __LINE__));
               bErrorIndicated = IFX_TRUE;
               break;
            }

            /* Check if IP address is already ready */
            /* If true, then it means that there is a second word - boardname */
            /* If false, then it means that there is only one word in the line
               and it's IP address */
            if(strlen(rgAddressIP) == 0)
            {
               /* If there is no IP address, it means it's IP address */
               nRet = Common_VerifyAddrIP(rgWord, &nAddrFamily);
               if (IFX_SUCCESS != nRet)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH,  TD_CONN_ID_PHONE_BOOK,
                        ("Err, Phonebook: detect incorrect syntax.\n"
                         "Expect IP address, but found: %s.\n"
                         "(File: %s, line: %d)\n",  rgWord, __FILE__, __LINE__));
                  bErrorIndicated = IFX_TRUE;
                  break;
               }
               strncpy(rgAddressIP, rgWord, TD_ADDRSTRLEN - 1);
            }
            else
            {
               strncpy(rgBoardname, rgWord, TD_PHONEBOOK_BOARDNAME_LEN - 1);
            }

            /* Reset building word */
            bWordBuilding = IFX_FALSE;
            nWordIndex = 0;
            memset(rgWord, 0, sizeof(rgWord));
         } /* if(bWordBuilding) */

         /* EOL means that IP address and boardname must be already ready */
         if((strlen(rgAddressIP) > 0) && (strlen(rgBoardname) > 0))
         {
            Common_PhonebookAddEntry(pCtrl, rgAddressIP, rgBoardname,
                                     &nUnusedBoolean);
            memset(rgAddressIP, 0, sizeof(rgAddressIP));
            memset(rgBoardname, 0, sizeof(rgBoardname));
            strcpy(rgBoardname, TD_NO_NAME);
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
                  ("Err, Phonebook: detected incorrect syntax.\n"
                   "Detect EOL but IP address and boardname aren't ready\n"
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            bErrorIndicated = IFX_TRUE;
            break;
         }

         if (cChar == EOF)
         {
            /* End of file */
            break;
         }

      }
      else
      {
         /* Building word */
         rgWord[nWordIndex] = cChar;
         nWordIndex++;
         bWordBuilding = IFX_TRUE;
         if(nWordIndex >= MAX_WORD_LEN)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
                  ("Err, Phonebook: building word is too long. Max len is %d\n"
                   "(File: %s, line: %d)\n",  MAX_WORD_LEN, __FILE__, __LINE__));
            break;
         }
      }
   } /* while(1) */

   TD_OS_FClose(pFp);
   if (IFX_TRUE == bErrorIndicated)
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
} /* Common_PhonebookGetFromFile */

/**
   Handle phonebook events.

   \param pCtrl      - pointer to status control structure
   \param pSockAddr  - pointer to struct sockaddr_storage
   \param nEvent     - event number: either IE_PHONEBOOK_UPDATE
                       (receive brodcast/multicast message from new board)
                       or IE_PHONEBOOK_UPDATE_RESPONSE (receive answer on sent
                       brodcast/multicast message)
   \param pBoardname - boardname

   \return IFX_SUCCESS message received and handled otherwise IFX_ERROR
*/
IFX_return_t Common_PhonebookHandleEvents(CTRL_STATUS_t* pCtrl,
                                          TD_OS_sockAddr_t* pSockAddr,
                                          IFX_int32_t nEvent,
                                          IFX_char_t* pBoardname)
{
   IFX_char_t rgRecvIpAddr[TD_ADDRSTRLEN];
   IFX_boolean_t bIsBoardIP;
   IFX_boolean_t bNewEntryAdded = IFX_FALSE;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pSockAddr, IFX_ERROR);
   TD_PTR_CHECK(pBoardname, IFX_ERROR);

   if (nEvent == IE_PHONEBOOK_UPDATE &&
       nEvent == IE_PHONEBOOK_UPDATE_RESPONSE)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
            ("Err, Common_PhonebookHandleEvents: unsupported event: %s.\n"
             "(File: %s, line: %d)\n",
             Common_Enum2Name(nEvent, TD_rgIE_EventsName), __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (AF_INET6 == TD_SOCK_FamilyGet(pSockAddr))
   {
      bIsBoardIP = TD_SOCK_AddrCompare(pSockAddr,
                    &pCtrl->oIPv6_Ctrl.oAddrIPv6, IFX_FALSE);
   }
   else if (AF_INET == TD_SOCK_FamilyGet(pSockAddr))
   {
      bIsBoardIP = TD_SOCK_AddrCompare(pSockAddr,
                    &pCtrl->oTapidemo_IP_Addr, IFX_FALSE);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
            ("Err, unhandled addr family %d.\n"
             "(File: %s, line: %d)\n",
             TD_SOCK_FamilyGet(pSockAddr), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* if addresses match do not add this address */
   if (IFX_TRUE == bIsBoardIP)
   {
      return IFX_SUCCESS;
   }

   /* get string representation */
   strcpy(rgRecvIpAddr, TD_GetStringIP(pSockAddr));

   if (nEvent == IE_PHONEBOOK_UPDATE)
   {
      Common_PhonebookAddEntry(pCtrl, rgRecvIpAddr, pBoardname,
                               &bNewEntryAdded);

      if(pCtrl->bPhonebookShow &&
         IFX_TRUE == bNewEntryAdded)
      {
         Common_PhonebookShow(pCtrl->rgoPhonebook);
      }

      if (IFX_SUCCESS != Common_PhonebookSendEntry(pCtrl,
                                                   IE_PHONEBOOK_UPDATE_RESPONSE,
                                                   pSockAddr))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
               ("Err, Common_PhonebookSendEntry() (File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
   }/* IE_PHONEBOOK_UPDATE */
   else if (nEvent == IE_PHONEBOOK_UPDATE_RESPONSE)
   {
      Common_PhonebookAddEntry(pCtrl, rgRecvIpAddr, pBoardname,
                               &bNewEntryAdded);

      if(pCtrl->bPhonebookShow &&
         IFX_TRUE == bNewEntryAdded)
      {
         Common_PhonebookShow(pCtrl->rgoPhonebook);
      }
   }/* IE_PHONEBOOK_UPDATE_RESPONSE */

   return IFX_SUCCESS;

} /* Common_PhonebookHandleEvents */


/**
   Send phonebook entry to other board(s).

   \param pCtrl   - pointer to status control structure
   \param nAction - which action to send to other phone
   \param pDstAddr - pointer to destination address storage

   \return       IFX_SUCCESS - no error, otherwise IFX_ERROR

   \remark Function can send phonebook entry either to specific board or via
           broadcast/multicast.
*/
IFX_return_t Common_PhonebookSendEntry(CTRL_STATUS_t* pCtrl, IFX_int32_t nAction,
                                       TD_OS_sockAddr_t *pDstAddr)
{
   IFX_int32_t ret = IFX_SUCCESS;
   TD_COMM_MSG_t msg = {0};
   TD_OS_socket_t nUsedSock;

   /* check input parameter */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* initialize msg structure */
   msg.nMarkStart = COMM_MSG_START_FLAG;
   msg.nMarkEnd = COMM_MSG_END_FLAG;
   msg.nMsgLength = sizeof(msg);
   msg.nMsgVersion = TD_COMM_MSG_CURRENT_VERSION;
   msg.nSenderPhoneNum = 0;
   msg.nReceiverPhoneNum = 0;
   msg.nSenderPort = 0;
   msg.fPCM = CALL_FLAG_UNKNOWN;
   msg.nTimeslot_RX = 0;
   msg.nTimeslot_TX = 0;
   msg.nBoard_IDX = NO_BOARD_IDX;
   msg.nFeatureID = 0;
   memcpy(msg.MAC, pCtrl->oTapidemo_MAC_Addr, ETH_ALEN);
   msg.nAction = nAction;

   /* copy name */
   strncpy(msg.oData1.aReserved,  pCtrl->rgoBoards[0].pszBoardName,
           TD_MAX_COMM_MSG_SIZE_DATA_1);

   /* Send phonebook entry message via broadcast/multicast */
   if (nAction == IE_PHONEBOOK_UPDATE)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_PHONE_BOOK,
            ("Sending BroadCast [%s]:%d.\n",
             TD_GetStringIP((TD_OS_sockAddr_t*)
                            pCtrl->rgoCast[TD_CAST_BROAD].aiAddrInfo->ai_addr),
             TD_SOCK_PortGet((TD_OS_sockAddr_t*)
                             pCtrl->rgoCast[TD_CAST_BROAD].aiAddrInfo->ai_addr)));
      /* send broadcast */
      ret = sendto(pCtrl->rgoCast[TD_CAST_BROAD].nSocket,
                   (IFX_char_t *) &msg, sizeof(msg), 0,
                   pCtrl->rgoCast[TD_CAST_BROAD].aiAddrInfo->ai_addr,
                   pCtrl->rgoCast[TD_CAST_BROAD].aiAddrInfo->ai_addrlen);

      if (sizeof(msg) != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
               ("Err, sending BroadCast (command %d byte(s)), %s %d. "
                "(File: %s, line: %d)\n",
                sizeof(msg), strerror(errno),
                pCtrl->rgoCast[TD_CAST_BROAD].nSocket, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      if (pCtrl->pProgramArg->oArgFlags.nUseIPv6)
      {
         /* #warning uncomment for debuging
            Common_PrintAddrInfo(pCtrl->rgoCast[TD_CAST_MULTI].aiAddrInfo); */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_PHONE_BOOK,
               ("Sending MultiCast [%s]:%d.\n",
                TD_GetStringIP((TD_OS_sockAddr_t*)
                               pCtrl->rgoCast[TD_CAST_MULTI].aiAddrInfo->ai_addr),
                TD_SOCK_PortGet((TD_OS_sockAddr_t*)
                                pCtrl->rgoCast[TD_CAST_MULTI].aiAddrInfo->ai_addr)));
         /** send multicast */
         ret = sendto(pCtrl->rgoCast[TD_CAST_MULTI].nSocket,
                      (IFX_char_t *) &msg, sizeof(msg), 0,
                      pCtrl->rgoCast[TD_CAST_MULTI].aiAddrInfo->ai_addr,
                      pCtrl->rgoCast[TD_CAST_MULTI].aiAddrInfo->ai_addrlen);

         if (sizeof(msg) != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
                  ("Err, sending MultiCast (command %d byte(s)), %s %d. "
                   "(File: %s, line: %d)\n",
                   sizeof(msg), strerror(errno),
                   pCtrl->rgoCast[TD_CAST_MULTI].nSocket, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
   }/* IE_PHONEBOOK_UPDATE */
   /* Send phonebook entry message to specific board using admin socket */
   else if (nAction == IE_PHONEBOOK_UPDATE_RESPONSE)
   {
      /* check input parameters */
      TD_PTR_CHECK(pDstAddr, IFX_ERROR);

      if (AF_INET6 == TD_SOCK_FamilyGet(pDstAddr))
      {
         nUsedSock = pCtrl->oIPv6_Ctrl.nSocketFd;
      }
      else if (AF_INET == TD_SOCK_FamilyGet(pDstAddr))
      {
         nUsedSock = pCtrl->nAdminSocket;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
               ("Err, Invalid family %d. (File: %s, line: %d)\n",
                TD_SOCK_FamilyGet(pDstAddr), __FILE__, __LINE__));
         return IFX_ERROR;
      }
      TD_SOCK_PortSet(pDstAddr, ADMIN_UDP_PORT);
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_PHONE_BOOK,
            ("Phonebook: sending response to [%s]:%d\n",
             TD_GetStringIP(pDstAddr), TD_SOCK_PortGet(pDstAddr)));
      ret = TD_OS_SocketSendTo(nUsedSock, (IFX_char_t *) &msg, sizeof(msg),
                               pDstAddr);
      if (sizeof(msg) != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
               ("Err, sending data (command %d byte(s)), %s. "
                "(File: %s, line: %d)\n",
                sizeof(msg), strerror(errno), __FILE__, __LINE__));
         return IFX_ERROR;
     }
   }/* IE_PHONEBOOK_UPDATE_RESPONSE */
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
            ("Err, Unsupported action: %s\n(File: %s, line: %d)\n",
             Common_Enum2Name(nAction, TD_rgIE_EventsName),
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* Common_PhonebookSendEntry */

/**
   Add new entry to phonebook.

   \param pCtrl      - handle to connection control structure
   \param pAddrIP    - IP address
   \param pBoardName - board's name
   \param bEntryAdded   - output value - set TRUE if entry was added,
                          otherwise set to IFX_FALSE

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
IFX_return_t Common_PhonebookAddEntry(CTRL_STATUS_t* pCtrl,
                                      IFX_char_t* pAddrIP,
                                      const IFX_char_t* pBoardName,
                                      IFX_boolean_t *bEntryAdded)
{
   IFX_return_t nRet;
   IFX_int32_t nAddrFamily;
   IFX_char_t rgTmpBuf[TD_PHONEBOOK_PREFIX_LEN] = {0};

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pAddrIP, IFX_ERROR);
   TD_PTR_CHECK(pBoardName, IFX_ERROR);
   TD_PTR_CHECK(bEntryAdded, IFX_ERROR);
   TD_PARAMETER_CHECK((0 == strlen(pAddrIP)), strlen(pAddrIP), IFX_ERROR);
   TD_PARAMETER_CHECK((0 == strlen(pBoardName)), strlen(pBoardName), IFX_ERROR);

   /* initialize in value */
   *bEntryAdded = IFX_FALSE;
   if (pCtrl->nPhonebookIndex >= TD_PHONEBOOK_SIZE)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
            ("Err, Phonebook: Reached max number of entries, "
             "can't add new entry."
             "(File: %s, line: %d)\n",  __FILE__, __LINE__));
      return IFX_ERROR;
   }

   nRet = Common_VerifyAddrIP(pAddrIP, &nAddrFamily);
   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
            ("Err, Phonebook: incorrect IP address format: %s.\n"
             "(File: %s, line: %d)\n", pAddrIP, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (Common_PhonebookIsDuplicate(pCtrl->rgoPhonebook, pCtrl->nPhonebookIndex,
                                   pAddrIP))
   {
      /* Entry is duplicated. Ignore */
      return IFX_SUCCESS;
   }

   /* Prepare prefix */
   if (snprintf(rgTmpBuf, TD_PHONEBOOK_PREFIX_LEN, "%02d", pCtrl->nPhonebookIndex)
       >= TD_PHONEBOOK_PREFIX_LEN)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_PHONE_BOOK,
            ("Err, Phonebook: prefix is too long.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   strncpy(pCtrl->rgoPhonebook[pCtrl->nPhonebookIndex].rgAddressIP,
           pAddrIP, TD_ADDRSTRLEN - 1);
   strncpy(pCtrl->rgoPhonebook[pCtrl->nPhonebookIndex].rgPrefix,
           rgTmpBuf, TD_PHONEBOOK_PREFIX_LEN - 1);
   strncpy(pCtrl->rgoPhonebook[pCtrl->nPhonebookIndex].rgBoardname,
           pBoardName, TD_PHONEBOOK_BOARDNAME_LEN - 1);

   pCtrl->nPhonebookIndex++;
   /* entry was successfully added */
   *bEntryAdded = IFX_TRUE;

   return IFX_SUCCESS;
} /* Common_PhonebookAddEntry */

/**
   Display phonebook.

   \param pPhoneBook - pointer to the TD_PHONEBOOK_t structure

 * \return
   \remarks
*/
IFX_void_t Common_PhonebookShow(TD_PHONEBOOK_t* pPhoneBook)
{
   IFX_int32_t i;
   IFX_char_t rgRow[200];
   IFX_char_t* sTableSeparate =
      "-----------------------------------------------------------------------------";

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_PHONE_BOOK,
         ("Phonebook:\n"));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_PHONE_BOOK,
         (" %s \n", sTableSeparate));

   /* "%-15s" adjust string to left with '-' */
   sprintf(rgRow, "| %-5s | %-17s | %-47s |\n", "Index", "Board", "IP address");
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_PHONE_BOOK,
         ("%s", rgRow));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_PHONE_BOOK,
         ("|%s|\n", sTableSeparate));

   for(i = 0; i < TD_PHONEBOOK_SIZE; i++)
   {
      if(!strcmp(pPhoneBook[i].rgPrefix,""))
      {
         break;
      }
      sprintf(rgRow, "|  %2s   | %-17s | %-47s |\n", pPhoneBook[i].rgPrefix,
              pPhoneBook[i].rgBoardname, pPhoneBook[i].rgAddressIP);
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_PHONE_BOOK,
            ("%s", rgRow));
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_PHONE_BOOK,
         (" %s \n", sTableSeparate));

} /* Common_PhonebookShow */

/**
   Check for duplicate in phonebook.

   \param pPhoneBook - pointer to the TD_PHONEBOOK_t structure
   \param nPhonebookSize - phonebook size
   \pAddrIP          - IP address to check

   \return IFX_TRUE if IP address is in phonebook, otherwise IFX_FALSE
*/
IFX_boolean_t Common_PhonebookIsDuplicate(TD_PHONEBOOK_t* pPhoneBook,
                                          IFX_int32_t nPhonebookSize,
                                          IFX_char_t* pAddrIP)
{
   IFX_int32_t i;

   /* Look for prefix in the phonebook */
   for(i = 0; i < nPhonebookSize; i++)
   {
      if(!strcmp(pPhoneBook[i].rgAddressIP, pAddrIP))
      {
         return IFX_TRUE;
      }
   }
   return IFX_FALSE;
} /* Common_PhonebookIsDuplicate */

/**
   Get IP address from phonebook.

   \param pCtrl      - handle to connection control structure
   \param pDialedNum - array of dialed digits
   \param pCalledIP  - called board's IP adress (as a string)

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
IFX_return_t Common_PhonebookGetIpAddr(CTRL_STATUS_t* pCtrl,
                                       IFX_char_t* pDialedNum,
                                       IFX_char_t* pCalledIP)
{
   IFX_int32_t i;
   IFX_char_t rgCalledPrefix[TD_PHONEBOOK_PREFIX_LEN];

   /* Build looking prefix: 3rd + 4th digits (06xx where x is prefix) */
   snprintf(rgCalledPrefix, TD_PHONEBOOK_PREFIX_LEN, "%c%c",
            pDialedNum[2], pDialedNum[3]);

   /* Look for prefix in the phonebook */
   for(i = 0; i < TD_PHONEBOOK_SIZE; i++)
   {
      if(0 == strcmp(pCtrl->rgoPhonebook[i].rgPrefix, rgCalledPrefix))
      {
         strcpy(pCalledIP, pCtrl->rgoPhonebook[i].rgAddressIP);
         return IFX_SUCCESS;
      }
   }

   /* failed to get ip addr */
   return IFX_ERROR;
} /* Common_PhonebookGetIpAddr */

/**
   Print structure struct addrinfo.

   \param addr - pointer to structure struct addrinfo
   \param nSeqConnId  - Seq Conn ID

   \return sockaddr_in or sockaddr_in6
*/
IFX_return_t Common_PrintAddrInfo(struct addrinfo *ai, IFX_uint32_t nSeqConnId)
{
   IFX_char_t rgHostName[NI_MAXHOST];
   IFX_char_t rgServiceName[NI_MAXSERV];
   IFX_int32_t nRet;

   TD_PTR_CHECK(ai, IFX_ERROR);
   TD_PTR_CHECK(ai->ai_addr, IFX_ERROR);

   nRet = getnameinfo(ai->ai_addr, ai->ai_addrlen,
                      rgHostName, sizeof(rgHostName),
                      rgServiceName, sizeof(rgServiceName), 0);
   if (nRet != 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, mutlicast getaddrinfo()(): %s\n"
             "(File: %s, line: %d)\n", gai_strerror(nRet),
             __FILE__, __LINE__));
     return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("address info:\n"
          "      host         = %s\n"
          "      service      = %s\n"
          "      ai_flags     = 0x%02X\n"
          "      ai_family    = %d (PF_INET = %d, PF_INET6 = %d)\n"
          "      ai_socktype  = %d (SOCK_STREAM = %d, SOCK_DGRAM = %d)\n"
          "      ai_protocol  = %d (IPPROTO_TCP = %d, IPPROTO_UDP = %d)\n"
          "      ai_addrlen   = %d (sockaddr_in = %d, sockaddr_in6 = %d)\n"
          "      ai_next      = %d\n",
          rgHostName,
          rgServiceName,
          ai->ai_flags,
          ai->ai_family, PF_INET, PF_INET6,
          ai->ai_socktype, SOCK_STREAM, SOCK_DGRAM,
          ai->ai_protocol, IPPROTO_TCP, IPPROTO_UDP,
          ai->ai_addrlen, sizeof(struct sockaddr_in), sizeof(struct sockaddr_in6),
          (IFX_int32_t)ai->ai_next));

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("      ip           = %s\n"
          "      port         = %d\n"
          "      ai_family    = %d\n",
          TD_GetStringIP((TD_OS_sockAddr_t*) ai->ai_addr),
          TD_SOCK_PortGet((TD_OS_sockAddr_t*) ai->ai_addr),
          TD_SOCK_FamilyGet((TD_OS_sockAddr_t*) ai->ai_addr)));

   return IFX_SUCCESS;
} /* Common_PrintAddrInfo */

/**
   Get address info.

   \param pHost            - host name or address IP
   \param pService         - service name or port number
   \param nAddrFamily      - address family
   \param nSocketType      - socket type
   \param nAiFlag          - additional hint flgs
   \param pInterfaceName   - interface name

   \return structure struct addrinfo
*/
IFX_void_t* Common_GetAddrInfo(IFX_char_t *pHost, IFX_char_t *pService,
                               IFX_int32_t nAddrFamily, IFX_int32_t nSocketType,
                               IFX_int32_t nAiFlag, IFX_char_t* pInterfaceName)
{
   struct addrinfo hints;
   struct addrinfo *ai, *aiHead;
   IFX_int32_t nRet;
   IFX_uint32_t scopeId;

   scopeId = if_nametoindex(pInterfaceName);

   memset(&hints, 0, sizeof hints);
   /* set to 0 to AF_UNSPEC, AF_INET to force IPv4 or AF_INET6 to force IPv6 */
   hints.ai_family = nAddrFamily;
   /* set to 0 to any, SOCK_DGRAM to UPD or SOCK_STREAM to TCP */
   hints.ai_socktype = nSocketType;
   /* set to AI_PASSIVE to use my IP */
   hints.ai_flags = nAiFlag;

   nRet = getaddrinfo(pHost, pService, &hints, &aiHead);
   if (nRet != 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, mutlicast getaddrinfo()(): %s\n"
             "(File: %s, line: %d)\n", gai_strerror(nRet), __FILE__, __LINE__));
      return IFX_NULL;
   }

   for ( ai = aiHead; ai != IFX_NULL; ai = ai->ai_next )
   {
      if ( ai->ai_family == PF_INET6 )
      {
         if (IFX_NULL != ai->ai_addr)
         {
            /* Check if scope ID is set correctly. */
            /* Sth getaddrinfo() doesn't set this value. */
            if ( ((struct sockaddr_in6*) ai->ai_addr)->sin6_scope_id == 0 )
            {
               /* Set correct scope ID, based on interface name */
               ((struct sockaddr_in6*) ai->ai_addr)->sin6_scope_id = scopeId;
            }
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, getaddrinfo(): ai_addr can't be NULL.\n"
                   "(File: %s, line: %d)\n",  __FILE__, __LINE__));
            freeaddrinfo(aiHead);
            return IFX_NULL;
         }
      }
   } /* for */
   /* #warning uncomment for debuging
      Common_PrintAddrInfo(aiHead); */
   return aiHead;
} /* Common_GetAddrInfo */

/**
   Get broadcast address for specified network interface name.

   \param pIfName    - network interface name
   \param pBroadCast - broadcast IP

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t Common_BroadcastAddrGet(IFX_char_t* pIfName,
                                     IFX_char_t* pBroadcastIP)
{
   struct ifreq oIfr;
   struct sockaddr_in *pSockAddr;
   TD_OS_socket_t nSocketFD;

   TD_PTR_CHECK(pIfName, IFX_ERROR);
   TD_PTR_CHECK(pBroadcastIP, IFX_ERROR);

   memset(&oIfr, 0, sizeof(struct ifreq));

   /* open socket */
   nSocketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
   if (-1 == nSocketFD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, broadcast socket(): %s.\n"
             "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* copy interface name */
   strcpy(oIfr.ifr_name, pIfName);
   /* Get the broadcast address */
   if(0 > ioctl(nSocketFD, SIOCGIFBRDADDR, &oIfr))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, broadcast ioctl(SIOCGIFBRDADDR): %s.\n"
             "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
      close(nSocketFD);
      return IFX_ERROR;
   }

   /* socket no longer needed */
   close(nSocketFD);

   pSockAddr = (struct sockaddr_in*) &oIfr.ifr_broadaddr;
   inet_ntop(AF_INET, &(pSockAddr->sin_addr), pBroadcastIP, INET_ADDRSTRLEN);

   return IFX_SUCCESS;
} /* Common_BroadcastAddrGet */

/**
   Open broadcast socket.

   \param pIfName          - network interface name
   \param pBroadcastPort   - broadcast port number
   \param pBroadcast       - pointer to structure TD_BROAD_MULTI_CAST_t

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t Common_BroadcastSocketOpen(IFX_char_t* pIfName,
                                        IFX_char_t* pBroadcastPort,
                                        TD_BROAD_MULTI_CAST_t* pBroadcast)
{
   IFX_int32_t nRet;
   IFX_char_t rgBroadcastIP[INET_ADDRSTRLEN];
   IFX_int32_t nOn = 1;

   TD_PTR_CHECK(pIfName, IFX_ERROR);
   TD_PTR_CHECK(pBroadcastPort, IFX_ERROR);
   TD_PTR_CHECK(pBroadcast, IFX_ERROR);

   /* Check if socket is already created */
   if (NO_SOCKET != pBroadcast->nSocket)
   {
      close(pBroadcast->nSocket);
      pBroadcast->nSocket = NO_SOCKET;
   }

    /* Get broadcast adddress */
   if (IFX_SUCCESS != Common_BroadcastAddrGet(pIfName, rgBroadcastIP))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Can't get broadcast adddress.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Get address info */
   pBroadcast->aiAddrInfo = Common_GetAddrInfo(rgBroadcastIP, pBroadcastPort,
                                               0, SOCK_DGRAM, 0, pIfName);
   if (IFX_NULL == pBroadcast->aiAddrInfo)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, broadcast Common_GetAddrInfo()\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
     return IFX_ERROR;
   }

   /* Open socket */
   pBroadcast->nSocket = socket(pBroadcast->aiAddrInfo->ai_family,
                                pBroadcast->aiAddrInfo->ai_socktype,
                                pBroadcast->aiAddrInfo->ai_protocol);

   if (0 > pBroadcast->nSocket)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, broadcast socket(): %s.\n"
             "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
      pBroadcast->nSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   /* Enable broadcast */
   if (setsockopt(pBroadcast->nSocket, SOL_SOCKET, SO_BROADCAST,
                  &nOn, sizeof(nOn)) < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, broadcast setsockopt(SOL_SOCKET, SO_BROADCAST): %s\n"
             "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
      close(pBroadcast->nSocket);
      pBroadcast->nSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   /* Bind socket */
   nRet = bind(pBroadcast->nSocket, pBroadcast->aiAddrInfo->ai_addr,
               pBroadcast->aiAddrInfo->ai_addrlen);
   if (0 > nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, broadcast bind(): %s.\n"
             "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
      close(pBroadcast->nSocket);
      pBroadcast->nSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   /* make the socket non blocking */
   fcntl(pBroadcast->nSocket, F_SETFL, O_NONBLOCK);

   return IFX_SUCCESS;
} /* Common_BroadcastSocketOpen */

/**
   Check if given address is multicast address.

   \param addr - pointer to structure struct sockaddr

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t Common_MulticastIsAddr(const struct sockaddr *addr)
{
   if (addr->sa_family == AF_INET)
   {
      if(IN_MULTICAST(ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr)))
         return IFX_SUCCESS;
   }
   if (addr->sa_family == AF_INET6)
   {
      if(IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6 *)addr)->sin6_addr))
         return IFX_SUCCESS;
   }
   return IFX_ERROR;
} /* Common_MulticastIsAddr */

/**
   Join multicast group.

   \param nSocket  - multicast socket
   \param addr - pointer to structure struct sockaddr

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t Common_MulticastJoinGroup(TD_OS_socket_t nSocketFd,
                                       struct sockaddr *addr)
{
   if (addr->sa_family == AF_INET)
   {
      struct ip_mreq   mreq;

      memcpy(&mreq.imr_multiaddr,
             &(((struct sockaddr_in *)addr)->sin_addr),
             sizeof(struct in_addr));

      /* Accept multicast from any interface */
      mreq.imr_interface.s_addr = htonl(INADDR_ANY);

      if (setsockopt(nSocketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     &mreq, sizeof(mreq)) < 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, multicast setsockopt(IPPROTO_IP, IP_ADD_MEMBERSHIP): %s\n"
                "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
         return IFX_ERROR;
      }
      return IFX_SUCCESS;
   } /* AF_INET */

   if (addr->sa_family == AF_INET6)
   {
      struct ipv6_mreq mreq6;
      memcpy(&mreq6.ipv6mr_multiaddr,
             &(((struct sockaddr_in6 *)addr)->sin6_addr),
             sizeof(struct in6_addr));

      mreq6.ipv6mr_interface = ((struct sockaddr_in6 *)addr)->sin6_scope_id;

      if (setsockopt(nSocketFd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                     &mreq6, sizeof(mreq6)) < 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, multicast setsockopt(IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP): %s\n"
                "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
         return IFX_ERROR;
      }
      return IFX_SUCCESS;
   } /* AF_INET6 */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, multicast unsupported family address.\n"
          "(File: %s, line: %d)\n", __FILE__, __LINE__));

   return IFX_ERROR;
} /* Common_MulticastJoinGroup */

/**
   Open multicast socket.

   \param pIfName          - network interface name
   \param pMulticastPort   - multicast IP address
   \param pMulticastPort   - multicast port number
   \param pMulticast       - pointer to structure TD_BROAD_MULTI_CAST_t

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t Common_MulticastSocketOpen(IFX_char_t* pIfName,
                                        IFX_char_t* pMulticastIP,
                                        IFX_char_t* pMulticastPort,
                                        TD_BROAD_MULTI_CAST_t* pMulticast)
{
   IFX_int32_t nRet;

   TD_PTR_CHECK(pIfName, IFX_ERROR);
   TD_PTR_CHECK(pMulticastIP, IFX_ERROR);
   TD_PTR_CHECK(pMulticastPort, IFX_ERROR);
   TD_PTR_CHECK(pMulticast, IFX_ERROR);

   /* Check if socket is already created */
   if (NO_SOCKET != pMulticast->nSocket)
   {
      close(pMulticast->nSocket);
      pMulticast->nSocket = NO_SOCKET;
   }

   /* Get address info */
   pMulticast->aiAddrInfo = Common_GetAddrInfo(pMulticastIP, pMulticastPort,
                                               0, SOCK_DGRAM, 0,
                                               pIfName);
   if (IFX_NULL == pMulticast->aiAddrInfo)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Common_GetAddrInfo()\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
     return IFX_ERROR;
   }

   /* Open socket */
   pMulticast->nSocket = socket(pMulticast->aiAddrInfo->ai_family,
                                pMulticast->aiAddrInfo->ai_socktype,
                                pMulticast->aiAddrInfo->ai_protocol);
   if (pMulticast->nSocket < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, mutlicast socket(): %s\n"
             "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
      pMulticast->nSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   /* Bind socket */
   nRet = bind(pMulticast->nSocket, pMulticast->aiAddrInfo->ai_addr,
               pMulticast->aiAddrInfo->ai_addrlen);
   if (0 > nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, mutlicast bind(): %s\n"
             "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
      close(pMulticast->nSocket);
      pMulticast->nSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   /* Check if used address is correct multicast address */
   nRet = Common_MulticastIsAddr(pMulticast->aiAddrInfo->ai_addr);
   if (nRet < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, used multicast address is incorrect.\n"
             "(File: %s, line: %d)\n",  __FILE__, __LINE__));
      close(pMulticast->nSocket);
      pMulticast->nSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   /* Join multicast group */
   nRet = Common_MulticastJoinGroup(pMulticast->nSocket,
                                    pMulticast->aiAddrInfo->ai_addr);
   if (nRet < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, can not joint to multicast group.\n"
             "(File: %s, line: %d)\n",  __FILE__, __LINE__));
      close(pMulticast->nSocket);
      pMulticast->nSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   /* make the socket non blocking */
   fcntl(pMulticast->nSocket, F_SETFL, O_NONBLOCK);

   return IFX_SUCCESS;
} /* Common_MulticastSocketOpen */

/**
   Init phonebook.

   \param pCtrl      - handle to connection control structure

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
IFX_return_t Common_PhonebookInit(CTRL_STATUS_t* pCtrl)
{
   IFX_char_t rgMyIpAddr[TD_ADDRSTRLEN];
   IFX_boolean_t nUnusedBoolean = IFX_FALSE;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   pCtrl->rgoCast[TD_CAST_BROAD].nSocket = NO_SOCKET;
   pCtrl->rgoCast[TD_CAST_MULTI].nSocket = NO_SOCKET;

   if (IFX_SUCCESS != Common_BroadcastSocketOpen(
                         pCtrl->pProgramArg->aNetInterfaceName,
                         TD_BROAD_CAST_PORT,
                         &pCtrl->rgoCast[TD_CAST_BROAD]))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Common_BroadcastSocketOpen() (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Add own board to phonebook */
   inet_ntop(AF_INET, TD_SOCK_GetAddrIn(&pCtrl->oTapidemo_IP_Addr),
             rgMyIpAddr, sizeof rgMyIpAddr);

   if (IFX_SUCCESS != Common_PhonebookAddEntry(pCtrl, rgMyIpAddr,
                         pCtrl->rgoBoards[0].pszBoardName, &nUnusedBoolean))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Common_PhonebookAddEntry() (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (pCtrl->pProgramArg->oArgFlags.nUseIPv6)
   {
      if (IFX_SUCCESS != Common_MulticastSocketOpen(
                            pCtrl->pProgramArg->aNetInterfaceName,
                            TD_MULTICAST_ADDR,
                            TD_MULTI_CAST_PORT,
                            &pCtrl->rgoCast[TD_CAST_MULTI]))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Common_MulticastSocketOpen() (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Add own board to phonebook */
      if (IFX_SUCCESS != Common_PhonebookAddEntry(pCtrl,
                            TD_GetStringIP(&pCtrl->oIPv6_Ctrl.oAddrIPv6),
                            pCtrl->rgoBoards[0].pszBoardName, &nUnusedBoolean))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Common_PhonebookAddEntry() (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   if (IFX_SUCCESS != Common_PhonebookSendEntry(pCtrl, IE_PHONEBOOK_UPDATE,
                                                IFX_NULL))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Common_PhonebookSendEntry() (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Start timeout - Tapidemo doesn't print phonebook
      until the first timeout occurs */
   TD_TIMEOUT_START(pCtrl->nUseTimeoutSocket);
   pCtrl->bPhonebookShow = IFX_FALSE;

   Common_PhonebookGetFromFile(pCtrl);

   return IFX_SUCCESS;
} /* Common_PhonebookInit */

/**
   Allocate memory for IPv6 sockets.

   \param pBoard - pointer to board structure.

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
   \remarks
*/
IFX_return_t Common_SocketAllocateIPv6(BOARD_t *pBoard)
{
   IFX_int32_t i;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

#ifdef TAPI_VERSION4
   /* do nothing if sockets are not used */
   if (IFX_TRUE != pBoard->fUseSockets)
   {
      return IFX_SUCCESS;
   }
#endif /* TAPI_VERSION4 */

   /* if there are no coder channels then sockets will not be used */
   if (pBoard->nMaxCoderCh <= 0)
   {
      return IFX_SUCCESS;
   }
   /* allocate memory */
   pBoard->rgnSocketsIPv6 = TD_OS_MemCalloc(pBoard->nMaxCoderCh,
                                            sizeof(IFX_int32_t));

   /* check if successfully allocated memory */
   if (pBoard->rgnSocketsIPv6 == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Allocate memory failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* initialize memory */
   for (i=0; i < pBoard->nMaxCoderCh; i++)
   {
      pBoard->rgnSocketsIPv6[i] = TD_NOT_SET;
   }
   return IFX_SUCCESS;
}
#endif /* TD_IPV6_SUPPORT */

#ifdef TD_PPD
/**
   Sets the FXS Phone Plug Detection configuration.

   \param pBoard - pointer to board structure.
   \param nCh    - channel number.
   \param nSeqConnId  - Seq Conn ID

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t Common_SetPpdCfg(BOARD_t *pBoard, IFX_uint32_t nCh,
                              IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_LINE_PHONE_DETECT_CFG_t stPpdConf;

   if (pBoard->pCtrl->pProgramArg->oArgFlags.nDisablePpd)
   {
      /* Tapidemo started with PPD disabled. No need to set the PPD
         configuration. This is not error. */
      return IFX_SUCCESS;
   }

   if (pBoard->pCtrl->pProgramArg->nPpdFlag == 0)
   {
      /* There is no user request to change the PPD configuration. */
      return IFX_SUCCESS;
   }

   memset(&stPpdConf, 0, sizeof(IFX_TAPI_LINE_PHONE_DETECT_CFG_t));
   /* Read current configuration. */
   if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[nCh],
                         IFX_TAPI_LINE_PHONE_DETECT_CFG_GET,
                         (IFX_int32_t)&stPpdConf, TD_DEV_NOT_SET, nSeqConnId))
   {
      return IFX_ERROR;
   }

   /* Use user configuration. */
   if (pBoard->pCtrl->pProgramArg->nPpdFlag & TD_PPD_FLAG_T1)
   {
      stPpdConf.nLostPeriod = pBoard->pCtrl->pProgramArg->nTimeoutT1;
   }
   if (pBoard->pCtrl->pProgramArg->nPpdFlag & TD_PPD_FLAG_T2)
   {
      stPpdConf.nFindPeriod = pBoard->pCtrl->pProgramArg->nTimeoutT2;
   }
   if (pBoard->pCtrl->pProgramArg->nPpdFlag & TD_PPD_FLAG_T3)
   {
      stPpdConf.nOffHookTime = pBoard->pCtrl->pProgramArg->nTimeoutT3;
   }
   if (pBoard->pCtrl->pProgramArg->nPpdFlag & TD_PPD_FLAG_CAP)
   {
      stPpdConf.nCapacitance = pBoard->pCtrl->pProgramArg->nCapacitance;
   }

   /* Apply changes. */
   if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[nCh],
                         IFX_TAPI_LINE_PHONE_DETECT_CFG_SET,
                         (IFX_int32_t)&stPpdConf,
                         TD_DEV_NOT_SET, nSeqConnId))
   {
      return IFX_ERROR;
   }
   /* Print current configuration. */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
           ("PPD Conf, ch %d: cap=%d[nF], T1=%d[s], T2=%d[ms], T3=%d[ms].\n",
            nCh, stPpdConf.nCapacitance, stPpdConf.nLostPeriod,
            stPpdConf.nFindPeriod, stPpdConf.nOffHookTime));
   return IFX_SUCCESS;
}
#endif /* TD_PPD */

/**
   Send trace to Event Logger.

   \param pMsg - buffer with trace

   \return none
*/
IFX_void_t Common_SendMsgToEL(IFX_char_t *pMsg)
{
#ifdef EVENT_LOGGER_DEBUG
   IFX_char_t *pIn = pMsg;
   IFX_char_t *pOut = g_buf_el;
   IFX_uint32_t nSize = 0;

   if (pMsg == IFX_NULL)
   {
      TD_PRINTF("error, invalid pointer "
                "(File: %s, line: %d):\n",
                __FILE__, __LINE__);
      return;
   }

   while ('\n' == *pIn)
     pIn++;

   do
   {
      /* validate length of string */
      if (nSize >= (TD_COM_BUF_SIZE - 1))
      {
         TD_PRINTF("warning: trace too long (File: %s, line: %d):\n",
                   __FILE__, __LINE__);
         break;
      }
      *pOut = *pIn;
      if ('\0' == *pIn)
      {
         if (nSize > 0)
         {
            EL_USR_LOG_EVENT_USER_STR(TD_EventLoggerFd, 
                                      IFX_TAPI_DEV_TYPE_VOICESUB_GW, 
                                      0, 0, g_buf_el);
         }
         break;
      }
      nSize++;
      pOut++;
      pIn++;

      if ('\n' == *pIn)
      {
         *pOut = '\0';
         EL_USR_LOG_EVENT_USER_STR(TD_EventLoggerFd, IFX_TAPI_DEV_TYPE_VOICESUB_GW, 0, 0, g_buf_el);
         nSize = 0;
         pIn++;
         while ('\n' == *pIn)
            pIn++;
         pOut = g_buf_el;
      }
   } while ( 1 );
#endif /* EVENT_LOGGER_DEBUG */
}

