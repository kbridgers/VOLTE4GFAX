
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : parse_cmd_arg.c
   Description : Parses command line arguments
*******************************************************************************/

/**
   \file parse_cmd_arg.c

   This file contains implementation of parsing command line arguments.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef VXWORKS
#include <ctype.h>
#endif

#include "ifx_types.h"
/*#include "sys_drv_debug.h"*/
#include "common.h"
#include "tapidemo.h"
#include "parse_cmd_arg.h"
#include "cid.h"
#include "pcm.h"

#ifdef FXO
#include "common_fxo.h"
#endif /* FXO */

#ifdef DXT
#include "device_duslic_xt.h"
#endif /* DXT */

/* ============================= */
/* Debug interface               */
/* ============================= */

/* ============================= */
/* Global Structures             */
/* ============================= */

extern IFX_boolean_t g_bPrintIoctl;
extern IFX_boolean_t g_bAddPrefix;

/** Command line parameter delimiter */
const IFX_char_t* ARG_DELIMITER = " ";

/** Minimum valid port number. */
enum { MIN_PORT_NUM = 0 };

/** Default port for syslog. */
enum { DEFAULT_SYSLOG_PORT = 514 };

/** Max length of each argument */
enum { MAX_ARGUMENT_LEN = 256 };

/** Delimiter between ip addrres and port number. */
const IFX_char_t IP_ADDR_PORT_DELIM = ':';

/** Name of running application */
const IFX_char_t* PROGRAM_NAME = "tapidemo";

#ifndef LINUX
/** Structure holding description of each argument. */
struct option
{
   const char* name;
   int has_arg;
   int* flag;
   int val;
};
#endif /* LINUX */

enum
{
#ifdef TD_RM_DEBUG
   /** option value for additional RM debug (resource counting). */
   TAPIDEMO_OPTION_NUMBER_RM_DBG_RES = 298,
#endif /* TD_RM_DEBUG */
#ifdef TD_DECT
   /** option value for DECT debug level set. */
   TAPIDEMO_OPTION_NUMBER_DECT_DBG = 299,
#endif /* TD_DECT */
   /** option value for VAD. */
   TAPIDEMO_OPTION_NUMBER_VAD = 300,
   /** option value for lec. */
   TAPIDEMO_OPTION_NUMBER_LEC = 301,
   /** option value for no lec. */
   TAPIDEMO_OPTION_NUMBER_NO_LEC = 302,
   /** option value for ground start. */
   TAPIDEMO_OPTION_NUMBER_GROUND_START = 303,
   /** print Tapidemo version */
   TAPIDEMO_OPTION_PRINT_VERSION = 304,
#ifdef LINUX
   /** print Tapidemo version */
   TAPIDEMO_OPTION_PRINT_CONFIGURE = 305,
#endif
   TAPIDEMO_OPTION_NUMBER_NO_PPD = 306,
   TAPIDEMO_OPTION_NUMBER_T1 = 307,
   TAPIDEMO_OPTION_NUMBER_T2 = 308,
   TAPIDEMO_OPTION_NUMBER_T3 = 309,
   TAPIDEMO_OPTION_NUMBER_CAP    = 310,
#ifdef TD_DECT
   /** option value for lec on DECT */
   TAPIDEMO_OPTION_NUMBER_DECT_EC = 311,
#endif /* TD_DECT */
   TAPIDEMO_OPTION_NUMBER_IPV6 = 312,
   TAPIDEMO_OPTION_NUMBER_TAPI_DBG = 313,
   TAPIDEMO_OPTION_NUMBER_PHONEBOOK = 314,
   TAPIDEMO_OPTION_FILE_FW = 315,
   TAPIDEMO_OPTION_FILE_DRAM = 316,
   TAPIDEMO_OPTION_FILE_BBD = 317,
   /** option value for seting RM debug level */
   TAPIDEMO_OPTION_NUMBER_RM_DBG = 318,
   TAPIDEMO_OPTION_NUMBER_DXT_ROM_FW = 319,
   TAPIDEMO_OPTION_NUMBER_NO_BBD_DWLD = 320,
   TAPIDEMO_OPTION_NUMBER_EL = 321,
   TAPIDEMO_OPTION_NUMBER_OOB_TX = 322,
   TAPIDEMO_OPTION_NUMBER_OOB_RX = 323,
   TAPIDEMO_OPTION_NUMBER_END
};

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** names of debug levels */
TD_ENUM_2_NAME_t TD_rgDebugLevelName[] =
{
   {DBG_LEVEL_LOW,      "LOW"},
   {DBG_LEVEL_NORMAL,   "NORMAL"},
   {DBG_LEVEL_HIGH,     "HIGH"},
   {DBG_LEVEL_OFF,      "OFF"},
   {TD_MAX_ENUM_ID,     "DEBUG_LEVEL_t"}
};

/** Command line arguments */
static struct option PROGRAM_OPTIONS[] =
{
   /*    name, has_arg, *flag, val */
   {"help"                , 0, 0, 'h'},
   {"version"             , 0, 0, TAPIDEMO_OPTION_PRINT_VERSION},
#ifdef LINUX
   {"configure"           , 0, 0, TAPIDEMO_OPTION_PRINT_CONFIGURE},
   {"daemon_logfile"      , 2, 0, 'r'},
   {"daemon_syslog"       , 2, 0, 's'},
#endif /* LINUX */
   {"debug"               , 1, 0, 'd'},
   {"ip-address"          , 1, 0, 'i'},
   {"wait"                , 0, 0, 'w'},
#ifdef VXWORKS
   {"qos"                 , 1, 0, 'q'},
#else
   {"qos"                 , 2, 0, 'q'},
#endif /* VXWORKS */
   {"cid"                 , 1, 0, 'c'},
   {"conferencing"        , 1, 0, 'k'},
   {"pcm"                 , 1, 0, 'p'},
   {"pcm_coder_type"      , 1, 0, 'u'},
   {"encoder_type"        , 1, 0, 'e'},
   {"packetization_time"  , 1, 0, 'f'},
   {"vad"                 , 1, 0, TAPIDEMO_OPTION_NUMBER_VAD},
   {"oob-tx"              , 1, 0, TAPIDEMO_OPTION_NUMBER_OOB_TX},
   {"oob-rx"              , 1, 0, TAPIDEMO_OPTION_NUMBER_OOB_RX},
   {"lec"                 , 1, 0, TAPIDEMO_OPTION_NUMBER_LEC},
   {"nolec"               , 0, 0, TAPIDEMO_OPTION_NUMBER_NO_LEC},
#ifdef TD_DECT
   {"es_dect"             , 1, 0, TAPIDEMO_OPTION_NUMBER_DECT_EC},
#endif /* TD_DECT */
   {"different_network"   , 1, 0, 'n'},
   {"board_combination"   , 1, 0, 'b'},
#ifdef LINUX
   {"FXO"                 , 2, 0, 'x'},
   {"path_to_FW_files"    , 1, 0, 'l'},
#endif /* LINUX */
#ifdef USE_FILESYSTEM
   /* FW filename */
   {"file_fw"             , 1, 0, TAPIDEMO_OPTION_FILE_FW},
   /* DRAM filename */
   {"file_dram"           , 1, 0, TAPIDEMO_OPTION_FILE_DRAM},
   /* BBD filename */
   {"file_bbd"            , 1, 0, TAPIDEMO_OPTION_FILE_BBD},
#endif /* USE_FILESYSTEM */
#ifdef DXT
   /* use polling mode in LL driver - now only for DxT */
   {"polling_mode"         , 0, 0, 'P'},
#endif /* DXT */
   /* ground start */
   {"gs"                  , 0, 0, TAPIDEMO_OPTION_NUMBER_GROUND_START},
   /* dect debug flag */
   {"dect"                , 0, 0, 't'},
   /* Phone Plug Detection functionality */
   {"no_ppd"              , 0, 0, TAPIDEMO_OPTION_NUMBER_NO_PPD},
   /* Timeout for T1 Timer from Phone Plug Detection State Machine */
   {"t1"                 , 1, 0, TAPIDEMO_OPTION_NUMBER_T1},
   /* Timeout for T2 Timer from Phone Plug Detection State Machine */
   {"t2"                 , 1, 0, TAPIDEMO_OPTION_NUMBER_T2},
   /* Timeout for T3 Timer from Phone Plug Detection State Machine */
   {"t3"                 , 1, 0, TAPIDEMO_OPTION_NUMBER_T3},
   /* Timeout for T4 Timer from Phone Plug Detection State Machine */
   {"cap"                , 1, 0, TAPIDEMO_OPTION_NUMBER_CAP},
#ifdef EASY336
   /* rm debug level argument */
   {"rm_dbg"              , 1, 0, TAPIDEMO_OPTION_NUMBER_RM_DBG},
#endif /* EASY336 */
   /* TAPI debug argument */
   {"tapi_dbg"            , 1, 0, TAPIDEMO_OPTION_NUMBER_TAPI_DBG},
#ifdef DXT
   {"dxt_rom_fw"          , 0, 0, TAPIDEMO_OPTION_NUMBER_DXT_ROM_FW},
#endif /* DXT */
   {"no_bbd_download"     , 0, 0, TAPIDEMO_OPTION_NUMBER_NO_BBD_DWLD},
   {"trace_ioctl"         , 0, 0, 'T'},
   {"disable_conn_id"     , 0, 0, 'C'},
   /* Event Logger support argument */
   {"el"                  , 0, 0, TAPIDEMO_OPTION_NUMBER_EL},
#ifdef TD_IPV6_SUPPORT
   {"ipv6"               , 0, 0, TAPIDEMO_OPTION_NUMBER_IPV6},
   {"file_phonebook"     , 1, 0, TAPIDEMO_OPTION_NUMBER_PHONEBOOK},
#endif /* TD_IPV6_SUPPORT */
   /* from here additional arguments start - not listed in help */
#ifdef TD_DECT
   /* dect debug argument */
   {"dect_dbg"            , 1, 0, TAPIDEMO_OPTION_NUMBER_DECT_DBG},
#endif /* TD_DECT */
#ifdef TD_RM_DEBUG
   /* rm debug argument */
   {"rm_dbg_resource"   , 0, 0, TAPIDEMO_OPTION_NUMBER_RM_DBG_RES},
#endif /* TD_RM_DEBUG */
   /* start with ITM enabled (Internal Test Module) */
   {"itm"                 , 0, 0, 'm'},
   /* ITM modular test case - Verify System Initialization.
      When used program waits with initialization for control PC connection. */
   {"initialize_system"   , 0, 0, 'v'},
   {0                     , 0, 0, 0 }
};

/** string with short command line arguments */
const IFX_char_t OPTION_PARAMETERS[] =
#ifdef LINUX
   /* LINUX specific flags */
                                       "r::s::x::l:q::"
#else
   /* VxWorks specific flags
      VxWorks doesn't have filesystem so remove arguments working with file. */
                                       "q:"
#endif /* LINUX */
#ifdef DXT
                                       "P"
#endif /* DXT */
   /* common flags */
                                       "hd:i:wc:k:p:u:e:f:n:b:tTCmv";

/** Strings describing command line parameters.
   {""} is delimiter between help message for one argument and its flag
   for end of table. */
static const IFX_char_t helpItems[][200] =
{
   {"Help screen"},
   {""},
   {"Print version"},
   {""},
#ifdef LINUX
   {"Print compilation arguments"},
   {""},
   {"Run as a daemon with trace redirection to file (default is OFF - 0)"},
      {"Set path to log file (by default is /opt/work/ifx/bin/tapidemo.log) <-r/path/name.log>"},
   {""},
   {"Run as a daemon with trace redirection to syslog (default is OFF - 0)"},
      {"Specify network IP address and port of remote syslog server "},
      {"(by default local syslog is used).<-sxxx.xxx.xxx.xxx:xxxx>"},
#endif /* LINUX */
   {""},
   {"Set debug level:"},
      {"4 - OFF"},
      {"3 - HIGH (default)"},
      {"2 - NORMAL"},
      {"1 - LOW"},
   {""},
   {"Specify network IP address of the board for connections, by default "},
      {"retrieve it automatically. Syntax is <xxx.xxx.xxx.xxx>"},
   {""},
   {"Wait after each state machine step (default is OFF - 0)"},
   {""},
   {"Qos support : packets redirected via udp redirector (default is OFF - 0)"},
   {""},
   {"CID support (default ON): showing CID on phone LCD:"},
      {" 0 - TELCORDIA, Belcore, USA"},
      {" 1 - ETSI FSK, Europe (default)"},
      {" 2 - ETSI DTMF, Europe"},
      {" 3 - SIN BT, Great Britain"},
      {" 4 - NTT, Japan"},
#ifndef TAPI_VERSION4
      {" 5 - KPN DTMF"},
      {" 6 - KPN DTMF+FSK"},
#endif /* TAPI_VERSION4 */
      {"10 - turn OFF CID support"},
   {""},
   {"Conferencing support (default is ON - 1): more than two phone connection."},
      {"HASH - '#' is used to start another call."},
   {""},
   {"PCM (default is OFF): using two boards connected over PCM."},
      {"Start/stop uses ethernet, but voice stream uses PCM:"},
      {" - One board is master <-p m>,"},
      {" - The other one is slave <-p s>"},
      {" - Using PCM on one board (will be master with PCM local loop) <-p l>"},
   {""},
   {"PCM codec type (default is "TD_STR_TAPI_PCM_NB_RESOLUTION_DEFAULT"):"},
      {" 0 - IFX_TAPI_PCM_RES_NB_ALAW_8BIT [G.711 A-law, 8 bits, NB] "},
      {" 1 - IFX_TAPI_PCM_RES_NB_ULAW_8BIT [G.711 u-law, 8 bits, NB] "},
      {" 2 - IFX_TAPI_PCM_RES_NB_LINEAR_16BIT [Linear 16 bits, NB] "},
      {" 3 - IFX_TAPI_PCM_RES_WB_ALAW_8BIT [G.711 A-law, 8 bits, WB] "},
      {" 4 - IFX_TAPI_PCM_RES_WB_ULAW_8BIT [G.711 u-law, 8 bits, WB] "},
      {" 5 - IFX_TAPI_PCM_RES_WB_LINEAR_16BIT [Linear 16 bits, WB] "},
      {" 6 - IFX_TAPI_PCM_RES_WB_G722 [G.722 16 bits, WB] "},
      {" 7 - IFX_TAPI_PCM_RES_NB_G726_16 [G.726 16 kbit/s, NB] "},
      {" 8 - IFX_TAPI_PCM_RES_NB_G726_24 [G.726 24 kbit/s, NB] "},
      {" 9 - IFX_TAPI_PCM_RES_NB_G726_32 [G.726 32 kbit/s, NB] "},
      {" 10 - IFX_TAPI_PCM_RES_NB_G726_40 [G.726 40 kbit/s, NB] "},
      {" 11 - IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT [Linear 16 bits, split time slots, WB] "},
   {""},
   {"Encoder type (default is IFX_TAPI_ENC_TYPE_MLAW):"},
      {" 1 - IFX_TAPI_ENC_TYPE_G723_63 [G723, 6.3 kBit/s]"},
      {" 2 - IFX_TAPI_ENC_TYPE_G723_53 [G723, 5.3 kBit/s]"},

      {" 7 - IFX_TAPI_ENC_TYPE_G729_AB [G729 A and B (silence compression),"
             " 8 kBit/s]"},
      {" 8 - IFX_TAPI_ENC_TYPE_MLAW [G711 u-law, 64 kBit/s]"},
      {" 9 - IFX_TAPI_ENC_TYPE_ALAW [G711 A-law, 64 kBit/s]"},
      {"12 - IFX_TAPI_ENC_TYPE_G726_16 [G726, 16 kBit/s]"},
      {"13 - IFX_TAPI_ENC_TYPE_G726_24 [G726, 24 kBit/s]"},
      {"14 - IFX_TAPI_ENC_TYPE_G726_32 [G726, 32 kBit/s]"},
      {"15 - IFX_TAPI_ENC_TYPE_G726_40 [G726, 40 kBit/s]"},
      {"16 - IFX_TAPI_ENC_TYPE_G729_E [G729 E, 11.8 kBit/s]"},
      {"17 - IFX_TAPI_ENC_TYPE_ILBC_133 [iLBC, 13.3 kBit/s]"},
      {"18 - IFX_TAPI_ENC_TYPE_ILBC_152 [iLBC, 15.2 kBit/s]"},
      {"21 - IFX_TAPI_COD_TYPE_AMR_4_75 [AMR, 4.75 kBit/s]"},
      {"22 - IFX_TAPI_COD_TYPE_AMR_5_15 [AMR, 5.15 kBit/s]"},
      {"23 - IFX_TAPI_COD_TYPE_AMR_5_9 [AMR, 5.9 kBit/s]"},
      {"24 - IFX_TAPI_COD_TYPE_AMR_6_7 [AMR, 6.7 kBit/s]"},
      {"25 - IFX_TAPI_COD_TYPE_AMR_7_4 [AMR, 7.4 kBit/s]"},
      {"26 - IFX_TAPI_COD_TYPE_AMR_7_95 [AMR, 7.95 kBit/s]"},
      {"27 - IFX_TAPI_COD_TYPE_AMR_10_2 [AMR, 10.2 kBit/s]"},
      {"28 - IFX_TAPI_COD_TYPE_AMR_12_2 [AMR, 12.2 kBit/s]"},
   {""},
   {"Packetisation time (default is 10 ms): Length of frames in milliseconds"},
      {" 1 - 2.5 ms"},
      {" 2 - 5 ms"},
      {" 3 - 5.5 ms"},
      {" 4 - 10 ms"},
      {" 5 - 11 ms"},
      {" 6 - 20 ms"},
      {" 7 - 30 ms"},
      {" 8 - 40 ms"},
      {" 9 - 50 ms"},
      {"10 - 60 ms"},
   {""},
   {"Voice Activity Detection (VAD) configuration:"},
      {" 0 - IFX_TAPI_ENC_VAD_NOVAD - no voice activity detection (default)"},
      {" 1 - IFX_TAPI_ENC_VAD_ON - voice activity detection on"},
      {" 2 - IFX_TAPI_ENC_VAD_G711 - voice activity detection on without "
       "spectral information"},
      {" 4 - IFX_TAPI_ENC_VAD_SC_ONLY - voice activity detection on without "
       "comfort noise generation"},
  {""},
  {"Out-Of-Band generation of RTP packets from local tones configuration:"},
      {" 1 - IFX_TAPI_PKT_EV_OOB_NO    - "TD_STR_TAPI_PKT_EV_OOB_NO" (default)"},
      {" 2 - IFX_TAPI_PKT_EV_OOB_ONLY  - "TD_STR_TAPI_PKT_EV_OOB_ONLY},
      {" 3 - IFX_TAPI_PKT_EV_OOB_ALL   - "TD_STR_TAPI_PKT_EV_OOB_ALL},
      {" 4 - IFX_TAPI_PKT_EV_OOB_BLOCK - "TD_STR_TAPI_PKT_EV_OOB_BLOCK},
  {""},
  {"Out-Of-Band received RTP packets playout configuration:"},
      {" 1 - IFX_TAPI_PKT_EV_OOBPLAY_PLAY - "TD_STR_TAPI_PKT_EV_OOBPLAY_PLAY" (default)"},
      {" 2 - IFX_TAPI_PKT_EV_OOBPLAY_MUTE - "TD_STR_TAPI_PKT_EV_OOBPLAY_MUTE},
  {""},
  {"LEC Type configuration:"},
         {" 0 - FX_TAPI_WLEC_TYPE_OFF - LEC and Echo Suppressor turned off."},
         {" 1 - IFX_TAPI_WLEC_TYPE_NE - LEC using fixed window. No Echo Suppressor "
          "(default)."},
         {" 2 - IFX_TAPI_WLEC_TYPE_NFE - LEC using fixed and moving window.  "
          "No Echo Suppressor."},
         {" 3 - IFX_TAPI_WLEC_TYPE_NE_ES - LEC using fixed window + Echo Suppressor."},
         {" 4 - IFX_TAPI_WLEC_TYPE_NFE_ES - LEC using fixed and moving window + "
          "Echo Suppressor."},
         {" 5 - IFX_TAPI_WLEC_TYPE_ES - Echo Suppressor."},
   {""},
   {"LEC is not enabled (by default LEC is enabled during call) - obsolete"},
   {""},
#ifdef TD_DECT
   {"Echo Suppressor for DECT handset:"},
         {" 0 - IFX_TAPI_EC_TYPE_OFF - Echo Suppressor turned off."},
         {" 1 - IFX_TAPI_EC_TYPE_ES - Echo Suppressor turned on (default)."},
   {""},
#endif
   {"Different network calls: (default is same network address),"},
      {"otherwise first 3 numbers of ip adrress are input <xxx.xxx.xxx>."},
   {""},
   {"Board combination to use (main board is the board for which TAPIDEMO"},
      {" was compiled, extension board is autodetected):"},
      {" 0 - no autodetection of the extension board (1 board)"},
      {" 1 - reserved - obsolete"},
      {" 2 - one EASY50712 (1 board) - obsolete"},
      {" 3 - one EASY50712 and one EASY50510 (2 boards) - obsolete"},
      {" 4 - one EASY3201 (1 board) - obsolete"},
      {" 5 - one EASY3201 with one EASY3111 (2 boards) - obsolete"},
      {" 6 - one EASY80800 (1 board) - obsolete"},
      {" 7 - one EASY508xx (1 board) - obsolete"},
      {" 8 - one EASY336 (1 board) - obsolete"},
      {" 9 - one EASY336 and one XT16 (2 boards) - obsolete"},
      {"10 - one XT16 (1 board) - obsolete"},
      {"11 - one EASY80910 (1 board) - obsolete"},
#ifdef LINUX
   {""},
   {"FXO support:"},
      {" no value - all FXO devices are automatically detected"},
      {" t - only Teridian FXO is used"},
      {" s - only FXO connected to SLIC121 is used"},
      {" All values can be used together e.g. -xst. Sequence not important."},
      {"  NOTE: this is the optional parameter and space between parameter"},
      {"        and value is not allowed. Correct example: -xs or -xts etc."},
   {""},
   {"Path to FW, BBD files."},
#endif /* LINUX */
#ifdef USE_FILESYSTEM
   {""},
   {"Use custom FW filename for main board."},
   {""},
   {"Use custom DRAM filename for main board."},
   {""},
   {"Use custom BBD filename for main board."},
#endif /* USE_FILESYSTEM */
#ifdef DXT
   {""},
   {"Enable polling mode for DxT device."},
#endif /* DXT */
   {""},
   {"Ground Start support."},
   {""},
   {"Enable DECT support."},
   {""},
   {"Disable the FXS Phone Plug Detection support."},
   {""},
   {"Timeout for T1 Timer - Phone Plug Detection functionality, [s]."},
   {""},
   {"Timeout for T2 Timer - Phone Plug Detection functionality, [ms]."},
   {""},
   {"Timeout for T3 Timer - Phone Plug Detection functionality, [ms]."},
   {""},
   {"Telephone capacitance threshold - Phone Plug Detection functionality, [nF]."},
#ifdef EASY336
   {""},
   {"Set RM (Resource Manager Module) debug level:"},
      {"4 - OFF"},
      {"3 - HIGH (default)"},
      {"2 - NORMAL"},
      {"1 - LOW"},
#endif /* EASY336 */
   {""},
   {"TAPI driver debug level."},
#ifdef DXT
   {""},
   {"Do not download FW for DxT - ROM FW."},
#endif /* DXT */
   {""},
   {"Do not download BBD."},
   {""},
   {"For all ioctls print ioctl's name when used."},
   {""},
   {"Disable tracing prefix with connection ID."},
   {""},
   {"Send traces to Event Logger."},
#ifdef TD_IPV6_SUPPORT
   {""},
   {"Enable IPv6 support."},
   {""},
   {"Name with path of phonebook file."},
#endif /* TD_IPV6_SUPPORT */
   /* data below two empty strings will not be printed */
   {""},
   {""},
   {"Modular test case - Verify System Initialization."},
   {""},
};

/** Max. number of possible command line arguments that follow application. */
#define TD_CMD_MAX_ARGS  32

/** List of all arguments. */
static IFX_char_t* pCmdArgs[TD_CMD_MAX_ARGS] = {0};


/* ============================= */
/* Local function declaration    */
/* ============================= */
IFX_return_t ReadOptions(IFX_int32_t nArgCnt,
                         IFX_char_t* pArgv[],
                         PROGRAM_ARG_t* pProgramArg);
IFX_int32_t GetArgCnt(IFX_char_t* pParam,
                      IFX_char_t* pCmdArgs[]);

#ifndef LINUX
static int getopt_long(int argc, char* const* argv, const char* shortopts,
                       const struct option* longopts, int* longindex);
#endif /* LINUX */

/* ============================= */
/* Local function definition     */
/* ============================= */

/** Used when reading command line arguments, to store argument value. */
static IFX_char_t* ifx_optarg = IFX_NULL;

#ifndef LINUX

/** Option index */
static IFX_int32_t optidx = 0;

/**
   Parse array of strings holding command line arguments.

   \param argc - number of arguments (basically number of words in string)
   \param argv - array of arguments (first one is the name of started program and
                        then parameters are following)
   \param shortopts - Mask which parameters are handled
   \param longopts - Command line arguments
   \param (int *) longindex)
*/
static int getopt_long(int argc, char* const* argv, const char* shortopts,
                       const struct option* longopts, int* longindex)
{
   IFX_int32_t j = 0;
   IFX_char_t* token = IFX_NULL;
   IFX_char_t short_arg = 0;
   IFX_char_t* long_arg = IFX_NULL;
   IFX_boolean_t valid_arg = IFX_FALSE;
   IFX_boolean_t need_val = IFX_FALSE;
   IFX_char_t* arg_location = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(argv, -1);
   TD_PTR_CHECK(shortopts, -1);
   TD_PTR_CHECK(longopts, -1);
   TD_PARAMETER_CHECK((argc <= 0), argc, -1);

   if ((optidx < 0) || (optidx >= argc))
   {
      optidx = 0;
      /* Index outside of argument list, but could also end correct, if
         we parse all the arguments and came to an end. */
      /*printf("Argument index outside argument list or at the end of argument "
             "list. (File: %s, line: %d)\n", __FILE__, __LINE__);*/
      return -1;
   }

   if (optidx == 0)
   {
      /* Skip program name */
      optidx++;
   }

   token = argv[optidx];
   if (token == IFX_NULL)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("In argument list NULL value. (File: %s, line: %d)\n",
          __FILE__, __LINE__));
      return -1;
   }

   /* Valid argument must have '-' or '--' infront and optional next token
      has value. */
   if ((strlen(token) == 2) && (token[0] == '-') && (token[1] != '-'))
   {
      /* Have short argument, only one char {-<n>} */
      short_arg = token[1];
      /* Check if exists */
      arg_location = strchr(shortopts, short_arg);
      if (arg_location != IFX_NULL)
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("Valid short arg %c\n", short_arg));
         valid_arg = IFX_TRUE;
         /* set flag if value follows and we are not outside of string */
         if (((shortopts + strlen(shortopts)) > arg_location)
             && (*(arg_location + 1) == ':'))
         {
            need_val = IFX_TRUE;
         }
      }
   }
   else if ((strlen(token) > 2) && (token[0] == '-') && (token[1] == '-'))
   {
      /* Long argument, string {--<nnnnnn>}, using longopts */
      long_arg = &token[2];

      j = 0;
      while ((longopts[j].name != IFX_NULL) && (strlen(longopts[j].name) != 0))
      {
         if (!strncmp(long_arg, longopts[j].name, strlen(longopts[j].name)))
         {
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("Valid long arg %s\n", long_arg));
            valid_arg = IFX_TRUE;
            short_arg = longopts[j].val;
            *longindex = (int) &longopts[j];
            if (longopts[j].has_arg == 1)
            {
               need_val = IFX_TRUE;
            }
            break;
         }
      }
   }
   else
   {
      /* Wrong syntax of argument */
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("Wrong argument syntax. (File: %s, line: %d)\n",
          __FILE__, __LINE__));
      return -1;
   }

   /* Jump to next argument or value */
   optidx++;

   /* Check if value is needed for this argument and retrieve it. */
   if (need_val)
   {
      if (optidx > argc)
      {
         /* Error, value is missing */
         ifx_optarg = IFX_NULL;
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("Value for arg is missing. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         return -1;
      }
      else
      {
         ifx_optarg = argv[optidx];
      }
      /* Move to next argument */
      optidx++;
   }

   return short_arg;
}
#endif /* LINUX */

/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   Return array of arguments.

   \return 0 - ERROR or no words, 1 - only program name, >1 counted words
*/
IFX_char_t** GetCmdArgsArr(IFX_void_t)
{
   return &pCmdArgs[0];
}


/**
   Count all words separated with delimiter and returns number of them.

   \param pParam - string containing all words
   \param pCmdArgs - array of strings with first one as application name

   \return 0 - ERROR or no words, 1 - only program name, >1 counted words
*/
IFX_int32_t GetArgCnt(IFX_char_t* pParam, IFX_char_t* pCmdArgs[])
{
   IFX_int32_t cnt = 0;
   IFX_char_t* token = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCmdArgs, 0);
   TD_PTR_CHECK(pParam, 0);

   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("GetArgCnt(): Get word count in command line arguments list %s.\n",
       pParam));

   /* Add program name to first place */
   pCmdArgs[cnt] = (IFX_char_t *) &PROGRAM_NAME[0];
   cnt++;

   /* Convert ordinary string into array of strings */
   token = (char *) strtok(pParam, ARG_DELIMITER);
   while (token != IFX_NULL)
   {
      pCmdArgs[cnt] = token;
      cnt++;
      token = (char *) strtok(IFX_NULL, ARG_DELIMITER);
   }

   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("GetArgCnt(): Got %d words count in command line arguments list.\n",
       (int) cnt));

   return cnt;
}

/**
   Reads syslog argument.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_SUCCESS argument is ok, otherwise IFX_ERROR
*/
IFX_return_t ReadOptions_Syslog(PROGRAM_ARG_t* pProgramArg)
{
   /* Formated command line options */
   IFX_char_t psCmdOption[MAX_ARGUMENT_LEN];
   IFX_char_t* port_num = IFX_NULL;
   IFX_int32_t arg_value = 0;
   IFX_uint32_t nIpAddrrLen;
   IFX_char_t psIpAddress[TD_IP_ADDRESS_LEN];

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   pProgramArg->oArgFlags.nDaemon= 1;
   if (ifx_optarg != IFX_NULL)
   {
      if(strnlen(ifx_optarg, MAX_ARGUMENT_LEN) == MAX_ARGUMENT_LEN)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                ("Program argument is incorrect. It is either too long "
                 "or it is non null terminated string\n (File: %s, line: %d)\n",
                 __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* Format command line option to string */
      snprintf(psCmdOption, MAX_ARGUMENT_LEN, "%s", ifx_optarg);

      if (strlen(psCmdOption) > TD_IP_ADDRESS_LEN + PORT_NUM_LEN + 1)

      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("Too long ip address with port number %s "
             "right syntax is <xxx.xxx.xxx.xxx:nnnnn>>.\n",
             psCmdOption));
         return IFX_ERROR;
      }

      /* Set port */
      port_num = strchr(psCmdOption, IP_ADDR_PORT_DELIM);
      TD_SOCK_FamilySet(&oTraceRedirection.oSyslog_IP_Addr, AF_INET);
      if ((port_num == IFX_NULL) || (strlen(port_num) == 1))
      {
         /* Use default port number */
         arg_value = DEFAULT_SYSLOG_PORT;
         TD_SOCK_PortSet(&oTraceRedirection.oSyslog_IP_Addr, arg_value);
         nIpAddrrLen = strlen(psCmdOption);
      }
      else
      {
         arg_value = atoi((port_num + 1));
         if ((arg_value > MAX_PORT_NUM) || (arg_value < MIN_PORT_NUM))
         {
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Port number outside boundaries [%d..%d].\n",
                MIN_PORT_NUM, MAX_PORT_NUM));
            return IFX_ERROR;
         }
         else
         {
            TD_SOCK_PortSet(&oTraceRedirection.oSyslog_IP_Addr, arg_value);
         }
         /* get address length */
         nIpAddrrLen = strlen(psCmdOption) - strlen(port_num);
      }

      /* Set ip address */
      if (TD_IP_ADDRESS_LEN < nIpAddrrLen+1)
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("Too long ip address."
                          "Right syntax is <xxx.xxx.xxx.xxx>.\n"));
         return IFX_ERROR;
      }

      strncpy(psIpAddress, psCmdOption, nIpAddrrLen);
      psIpAddress[TD_IP_ADDRESS_LEN-1]='\0';

      /* In Linux returns 0 if address is invalid. */
      if (IFX_SUCCESS != TD_OS_SocketAton(psIpAddress,
             TD_OS_IFXOS_ADDR_MAP &oTraceRedirection.oSyslog_IP_Addr))
      {
          TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
             ("Wrong format of IP address %s, it should be <xxx.xxx.xxx.xxx>.\n",
              psIpAddress));
          return IFX_ERROR;
      }

      pProgramArg->nTraceRedirection = TRACE_REDIRECTION_SYSLOG_REMOTE;
   } /* if (ifx_optarg != IFX_NULL) */
   else
   {
      pProgramArg->nTraceRedirection = TRACE_REDIRECTION_SYSLOG_LOCAL;
   }

   return IFX_SUCCESS;
}

/**
   Specify network IP address of the board for connections.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_SUCCESS argument is ok, otherwise IFX_ERROR
*/
IFX_return_t ReadOptions_IP_Addr(PROGRAM_ARG_t* pProgramArg)
{
   /* Formated command line options */
   IFX_char_t psCmdOption[MAX_ARGUMENT_LEN];
#ifdef TD_IPV6_SUPPORT
   IFX_int32_t nRet;
#endif /* TD_IPV6_SUPPORT */

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);


   if (ifx_optarg == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
             ("Can't set different network.\n (File: %s, line: %d)\n",
               __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if(strnlen(ifx_optarg, MAX_ARGUMENT_LEN) == MAX_ARGUMENT_LEN)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
             ("Program argument is incorrect. It is either too long or "
              "it is non null terminated string\n (File: %s, line: %d)\n",
              __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Format command line option to string */
   snprintf(psCmdOption, MAX_ARGUMENT_LEN, "%s", ifx_optarg);

   if (strlen(psCmdOption) > TD_ADDRSTRLEN)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("Too long ip address %s right syntax is <xxx.xxx.xxx.xxx>.\n",
          psCmdOption));
      return IFX_ERROR;
   }
#ifndef TD_IPV6_SUPPORT
   if (IFX_SUCCESS != TD_OS_SocketAton(psCmdOption,
                         TD_OS_IFXOS_ADDR_MAP &pProgramArg->oMy_IP_Addr))
   {
       TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
          ("Wrong format of IP address %s, it should be <xxx.xxx.xxx.xxx>.\n",
           psCmdOption));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
#else  /* TD_IPV6_SUPPORT */
   /* set family */
   TD_SOCK_FamilySet(&pProgramArg->oMy_IP_Addr, AF_INET);
   TD_SOCK_FamilySet(&pProgramArg->oMy_IPv6_Addr, AF_INET6);

   nRet = inet_pton(AF_INET, psCmdOption,
             TD_SOCK_GetAddrIn(&pProgramArg->oMy_IP_Addr));
   if(0 < nRet)
   {
      return IFX_SUCCESS;
   }
   errno = 0;

   nRet = inet_pton(AF_INET6, psCmdOption,
              TD_SOCK_GetAddrIn(&pProgramArg->oMy_IPv6_Addr));
   if(0 < nRet)
   {
      pProgramArg->oArgFlags.nAddrIsSetIPv6 = 1;
      return IFX_SUCCESS;
   }
   errno = 0;
#endif /* TD_IPV6_SUPPORT */

   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("Incorrect IP address: %s\n", psCmdOption));
   return IFX_ERROR;


}

/**
   Get different network calls option argument.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_SUCCESS argument is ok, otherwise IFX_ERROR
*/
IFX_return_t ReadOptions_Network(PROGRAM_ARG_t* pProgramArg)
{
   /* Formated command line options */
   IFX_char_t psCmdOption[MAX_ARGUMENT_LEN] = {0};
   IFX_char_t tmp_ip_addr[DIFF_NET_IP_LEN + 2] = {0};
   TD_OS_sockAddr_t sockAddr = {0};

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   /* Set ip address of different network */
   if (ifx_optarg == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
             ("Can't set ip address .\n (File: %s, line: %d)\n",
               __FILE__, __LINE__));

      return IFX_ERROR;
   }

   pProgramArg->oArgFlags.nDiffNet = 0;
   /* Format command line option to string */
   snprintf(psCmdOption, MAX_ARGUMENT_LEN, "%s", ifx_optarg);
   if (DIFF_NET_IP_LEN <= strlen(psCmdOption))
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("IP address %s of different network is wrong, "
          "right format is xxx.xxx.xxx\n", psCmdOption));
      return IFX_ERROR;
   }
   strncpy(tmp_ip_addr, psCmdOption, DIFF_NET_IP_LEN);
   tmp_ip_addr[DIFF_NET_IP_LEN] = '\0';

   /* Add last ip number, must not be 0 or 255 */
   if((DIFF_NET_IP_LEN + 2) > (strlen(tmp_ip_addr) + 2))
   {
      strcat(tmp_ip_addr, ".1");
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Invalid index of array 'tmp_ip_addr'."
            " (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Check if correct ip address */
   if (IFX_SUCCESS != TD_OS_SocketAton(tmp_ip_addr,
                                       TD_OS_IFXOS_ADDR_MAP &sockAddr))
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("IP address %s of different network is wrong, "
          "right format is xxx.xxx.xxx\n", psCmdOption));
      return IFX_ERROR;
   }

   pProgramArg->oDiffNetIP.s_addr = TD_SOCK_AddrIPv4Get(&sockAddr);
   /* set different network flag */
   pProgramArg->oArgFlags.nDiffNet = 1;

   return IFX_SUCCESS;
}

/**
   Get FXO option argument.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_SUCCESS argument is ok, otherwise IFX_ERROR
*/
IFX_return_t ReadOptions_Fxo(PROGRAM_ARG_t* pProgramArg)
{
   IFX_int32_t i;

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   if (IFX_NULL != ifx_optarg)
   {
      /* Check every element of array */
      for (i=0; i < strlen(ifx_optarg); i++)
      {
         if (0 == strncmp(&ifx_optarg[i], TD_CMD_ARG_FXO_TERIDIAN, 1))
         {
#ifdef TERIDIAN_FXO
            pProgramArg->oArgFlags.nFXO = 1;
            pProgramArg->nFxoMask |= TD_FXO_TER;
#else
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Err, Tapidemo compiled without Teridian support "
                "(drv_ter1x66 missing).\n"
                "Option '-x%s' can be used only when Tapidemo is "
                "compiled with Teridian support. ",
                TD_CMD_ARG_FXO_TERIDIAN));
            return IFX_ERROR;
#endif
         } else if (0 == strncmp(&ifx_optarg[i], TD_CMD_ARG_FXO_DUSLIC, 1))
         {
#ifdef DUSLIC_FXO
            pProgramArg->oArgFlags.nFXO = 1;
            pProgramArg->nFxoMask |= TD_FXO_DUS;
#else
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Err, Tapidemo compiled without Duslic support.\n"
                "(driver missing).\n"
                "Option '-x%s' can be used only when Tapidemo is "
                "compiled with Duslic support. ",
                TD_CMD_ARG_FXO_DUSLIC));
            return IFX_ERROR;
#endif
         } else if (0 == strncmp(&ifx_optarg[i], TD_CMD_ARG_FXO_SLIC121, 1))
         {
#ifdef SLIC121_FXO
            pProgramArg->oArgFlags.nFXO = 1;
            pProgramArg->nFxoMask |= TD_FXO_SLIC121;
#else
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Err, Tapidemo does not support the SLIC 121 FXO on this board.\n "
                "Option '-x%s' can't be used. \n",
                TD_CMD_ARG_FXO_SLIC121));
            return IFX_ERROR;
#endif
         }
         else
         {
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Err, not supported argument '-x%s'. \n", ifx_optarg));
            return IFX_ERROR;
         }
      } /* for */
   }
   else
   {
      /* '-x' is used without additional parameters. Set all supported
         FXO devices. */
      pProgramArg->oArgFlags.nFXO = 1;
#ifdef DUSLIC_FXO
      pProgramArg->nFxoMask |= TD_FXO_DUS;
#endif
#ifdef TERIDIAN_FXO
      pProgramArg->nFxoMask |= TD_FXO_TER;
#endif
#ifdef SLIC121_FXO
      pProgramArg->nFxoMask |= TD_FXO_SLIC121;
#endif
   }
   return IFX_SUCCESS;
}

/**
   Get QoS option argument.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_SUCCESS argument is ok, otherwise IFX_ERROR
*/
IFX_return_t ReadOptions_QOS(PROGRAM_ARG_t* pProgramArg)
{
   const IFX_char_t* const UDP_FLAG_LOCAL = "l";
   /* o stands for Older implementation */
   const IFX_char_t* const UDP_FLAG_USING_FIO_QOS_START = "o";
#ifdef VXWORKS
   const IFX_char_t* const UDP_FLAG_EXTERNAL = "e";
#endif /* VXWORKS */

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   /* Set default settings */
   pProgramArg->oArgFlags.nQOS_Local = 0;
   pProgramArg->oArgFlags.nQos = 1;
#ifdef FIO_QOS_ON_SOCKET_START
   pProgramArg->oArgFlags.nQosSocketStart = 1;
#else
   pProgramArg->oArgFlags.nQosSocketStart = 0;
#endif /* FIO_QOS_ON_SOCKET_START */
   /* QOS */
   if (IFX_NULL != ifx_optarg)
   {
      if (0 == strcmp(UDP_FLAG_LOCAL, ifx_optarg))
      {
         /* Use codec for local call */
         pProgramArg->oArgFlags.nUseCodersForLocal = 1;
#ifndef TAPI_VERSION4
         /* QoS is supported for TAPI 3 */
         pProgramArg->oArgFlags.nQOS_Local = 1;
#else
         /* QoS is not supported for TAPI 4 */
         pProgramArg->oArgFlags.nQos = 0;
         pProgramArg->oArgFlags.nQOS_Local = 0;
#endif /* TAPI_VERSION4 */
      } /* if (0 == strcmp(UDP_FLAG_LOCAL, ifx_optarg)) */
      else if (0 == strcmp(UDP_FLAG_USING_FIO_QOS_START, ifx_optarg))
      {
         /* QoS is supported for TAPI 3 */
         pProgramArg->oArgFlags.nQosSocketStart = 0;
      }
#ifndef VXWORKS
      else
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("Wrong parameter for setting UDP (QOS), "
             "should be %s - local or %s use FIO_QOS_START.\n",
             UDP_FLAG_LOCAL, UDP_FLAG_USING_FIO_QOS_START));
         return IFX_ERROR;
      }
#else
      else if (0 == strcmp(UDP_FLAG_EXTERNAL, ifx_optarg))
      {
         pProgramArg->oArgFlags.nQOS_Local = 0;
      } /* else if (0 == strcmp(UDP_FLAG_EXTERNAL, ifx_optarg)) */
      else
      {
         pProgramArg->oArgFlags.nQos = 0;
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("Wrong parameter for setting UDP (QOS), "
             "should be %s - local or %s - external.\n",
             UDP_FLAG_LOCAL, UDP_FLAG_EXTERNAL));
         return IFX_ERROR;
      }
#endif /* VXWORKS */
      return IFX_SUCCESS;
   } /* if (ifx_optarg != IFX_NULL) */

   return IFX_SUCCESS;
}

/**
   Get LEC option argument.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_SUCCESS argument is ok, otherwise IFX_ERROR
*/
IFX_return_t ReadOptions_LEC(PROGRAM_ARG_t* pProgramArg)
{
   IFX_int32_t arg_value = 0;

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   /* LEC configuration */
   if (IFX_NULL != ifx_optarg)
   {
      if (Common_IsNumber(ifx_optarg))
      {
         arg_value = atoi(ifx_optarg);
         if ((IFX_TAPI_WLEC_TYPE_OFF <= arg_value) &&
             (IFX_TAPI_WLEC_TYPE_ES >= arg_value))
         {
            switch (arg_value)
            {
               case IFX_TAPI_WLEC_TYPE_OFF:
               case IFX_TAPI_WLEC_TYPE_NE:
               case IFX_TAPI_WLEC_TYPE_NFE:
               case IFX_TAPI_WLEC_TYPE_NE_ES:
               case IFX_TAPI_WLEC_TYPE_NFE_ES:
               case IFX_TAPI_WLEC_TYPE_ES:
                  pProgramArg->nLecCfg = arg_value;
                  break;
               default:
                  return IFX_ERROR;
            } /* switch (arg_value) */
            return IFX_SUCCESS;
         } /* if */
      } /* if */
   } /* if */
   return IFX_ERROR;
 } /* ReadOptions_LEC */

#ifdef TD_DECT
/**
   Get Echo Canceller settings for DECT.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_SUCCESS argument is ok, otherwise IFX_ERROR
*/
IFX_return_t ReadOptions_DECT_EC(PROGRAM_ARG_t* pProgramArg)
{
   IFX_int32_t arg_value = 0;

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   /* EC configuration */
   if (IFX_NULL != ifx_optarg)
   {
      if (Common_IsNumber(ifx_optarg))
      {
         arg_value = atoi(ifx_optarg);
         switch (arg_value)
         {
            case IFX_TAPI_EC_TYPE_OFF:
            case IFX_TAPI_EC_TYPE_ES:
               pProgramArg->nDectEcCfg = arg_value;
               break;
            default:
               return IFX_ERROR;
         } /* switch (arg_value) */
         return IFX_SUCCESS;
      } /* if */
   } /* if */
   return IFX_ERROR;
 } /* ReadOptions_LEC */
#endif /* TD_DECT */


/**
   Reads program arguments.

   \param nArgCnt     - number of arguments (basically number of words in string)
   \param pArgv       - array of arguments (first one is the name of started program and
                        then parameters are following)
   \param pProgramArg - pointer to program arguments structure

   \return IFX_SUCCESS arguments ok, otherwise IFX_ERROR
*/
IFX_return_t ReadOptions(IFX_int32_t nArgCnt,
                         IFX_char_t* pArgv[],
                         PROGRAM_ARG_t* pProgramArg)
{
   const IFX_char_t* const PCM_FLAG_MASTER = "m";
   const IFX_char_t* const PCM_FLAG_SLAVE = "s";
   const IFX_char_t* const PCM_FLAG_LOOP = "l";

   IFX_int32_t option_index = 0, option = 0;
   IFX_int32_t arg_value = 0;
   IFX_int32_t nPCM_Counter = 0;
#ifdef EASY336
   IFX_boolean_t fIsRmDbgSet = IFX_FALSE;
#endif /* EASY336 */

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);
   TD_PTR_CHECK(pArgv, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
      ("Read program arguments.\n"));

   if (1 >= nArgCnt)
   {
      /* None arguments passed to program */
      ProgramHelp();
      return IFX_ERROR;
   }

   while ((option = getopt_long(nArgCnt, pArgv,
                                OPTION_PARAMETERS, PROGRAM_OPTIONS,
                                (int *) option_index)) != -1)
   {
#ifdef LINUX
      ifx_optarg = optarg;
#endif
      switch (option)
      {
         case 'h':
            pProgramArg->oArgFlags.nHelp = 1;
            break;
#ifdef LINUX
         case 'r':
            pProgramArg->oArgFlags.nDaemon= 1;
            pProgramArg->nTraceRedirection = TRACE_REDIRECTION_FILE;
            if (IFX_NULL != ifx_optarg)
            {
               strncpy(oTraceRedirection.sPathToLogFiles, ifx_optarg, MAX_PATH_LEN - 1);
            }
            else
            {
               strncpy(oTraceRedirection.sPathToLogFiles,
                       "/opt/lantiq/bin/tapidemo.log", MAX_PATH_LEN - 1);
            }
            break;
         case 's':
            if (IFX_SUCCESS != ReadOptions_Syslog(pProgramArg))
            {
               return IFX_ERROR;
            }
            break;
#endif /* LINUX */
         case 'd':
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  if ((arg_value > DBG_LEVEL_OFF) || (arg_value < DBG_LEVEL_LOW))
                  {
                     arg_value = DBG_LEVEL_HIGH;
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                            ("Can't set debug level. Use default level.\n"
                             "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  }
                  pProgramArg->nDbgLevel = arg_value;
                  g_pITM_Ctrl->nComDbgLevel = arg_value;
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set debug level.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
#ifdef EASY336
         case TAPIDEMO_OPTION_NUMBER_RM_DBG:
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  if ((arg_value > DBG_LEVEL_OFF) || (arg_value < DBG_LEVEL_LOW))
                  {
                     arg_value = DBG_LEVEL_HIGH;
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                            ("Can't set RM debug level. Use default level.\n"
                             "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  }
                  pProgramArg->nRmDbgLevel = arg_value;
                  fIsRmDbgSet = IFX_TRUE;
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set RM debug level.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
#endif /* EASY336 */
         case 'i':
            if (IFX_SUCCESS != ReadOptions_IP_Addr(pProgramArg))
            {
               return IFX_ERROR;
            }
            break;
         case 'w':
            pProgramArg->oArgFlags.nWait = 1;
            break;
         case 'q':
            if (IFX_SUCCESS != ReadOptions_QOS(pProgramArg))
            {
               return IFX_ERROR;
            }
            break;
         case 'c':
            /* CID */
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  /* get CID standard */
                  pProgramArg->nCIDStandard = atoi(ifx_optarg);
                  /* if arguemt is equal TD_TURN_OFF_CID then CID should not be used */
                  if (TD_TURN_OFF_CID == pProgramArg->nCIDStandard)
                  {
                     /* CID will not be used */
                     pProgramArg->oArgFlags.nCID = 0;
                  }
                  else
                  {
                     /* CID will not be used */
                     pProgramArg->oArgFlags.nCID = 1;
                  }
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set CID.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
         case 'k':
            /* Conferencing */
            if (ifx_optarg != IFX_NULL)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  if (0 == arg_value)
                  {
                     pProgramArg->oArgFlags.nConference = 0;
                     pProgramArg->oArgFlags.nUseCodersForLocal = 1;
                  }
                  else
                  {
                     pProgramArg->oArgFlags.nConference = 1;
                  }
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                         ("Can't set conference.\n (File: %s, line: %d)\n",
                           __FILE__, __LINE__));
                  return IFX_ERROR;
               }
            } /* if (ifx_optarg != IFX_NULL) */
            else
            {
               pProgramArg->oArgFlags.nConference = 1;
            } /* if (ifx_optarg != IFX_NULL) */
            break;
         case 'p':
            /* Using PCM */
            if (IFX_NULL != ifx_optarg)
            {
               if (0 == strcmp(PCM_FLAG_MASTER, ifx_optarg))
               {
                  pProgramArg->oArgFlags.nPCM_Master = 1;
                  pProgramArg->oArgFlags.nPCM_Slave = 0;
                  pProgramArg->oArgFlags.nPCM_Loop = 0;
               }
               else if (0 == strcmp(PCM_FLAG_SLAVE, ifx_optarg))
               {
                  pProgramArg->oArgFlags.nPCM_Master = 0;
                  pProgramArg->oArgFlags.nPCM_Slave = 1;
                  pProgramArg->oArgFlags.nPCM_Loop = 0;
               }
               else if (0 == strcmp(PCM_FLAG_LOOP, ifx_optarg))
               {
                  pProgramArg->oArgFlags.nPCM_Master = 1;
                  pProgramArg->oArgFlags.nPCM_Slave = 0;
                  pProgramArg->oArgFlags.nPCM_Loop = 1;
               }
               else
               {
                  TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                     ("Wrong parameter for setting PCM, should be %s - "
                      "master or %s - slave.\n",
                      PCM_FLAG_MASTER, PCM_FLAG_SLAVE));
                  return IFX_ERROR;
               }
               break;
            } /* if (ifx_optarg != IFX_NULL) */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Can't set PCM.\n (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
         case 'u':
            /* Set PCM codec */
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  pProgramArg->oArgFlags.nPCM_TypeDef = 1;
                  /* First parameter, used as default */
                  if (0 == nPCM_Counter)
                  {
                     if(IFX_SUCCESS != PCM_CodecSet(atoi(ifx_optarg)))
                     {
                        TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                          ("Err, can't recognize codec no. %d.\n",
                           atoi(ifx_optarg)));
                        return IFX_ERROR;
                     }
                     /* Waiting for next option */
                     nPCM_Counter++;
                     break;
                  }
                  else if (1 == nPCM_Counter)
                  {
                     /* Second parameter */
                     if (TD_WIDEBAND == g_oPCM_Data.nBand)
                     {
                        if(TD_WIDEBAND == PCM_CodecBand(atoi(ifx_optarg)))
                        {
                           TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                                ("Err, Wideband coder type given twice.\n"));
                           return IFX_ERROR;
                        }
                        else
                        {
                           g_oPCM_Data.ePCM_NB_Resolution = atoi(ifx_optarg);
                        }
                     }
                     else if (TD_NARROWBAND == g_oPCM_Data.nBand)
                     {
                        if(TD_WIDEBAND == PCM_CodecBand(atoi(ifx_optarg)))
                        {
                           g_oPCM_Data.ePCM_WB_Resolution = atoi(ifx_optarg);
                        }
                        else
                        {
                           TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                                ("Err, Narrowband coder type given twice.\n"));
                           return IFX_ERROR;
                        }
                     }
                     break;
                  }
               }
            } /* if (ifx_optarg != IFX_NULL) */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Err, Can't set PCM codec.\n (File: %s, line: %d)\n",
                    __FILE__, __LINE__));
            return IFX_ERROR;
         case 'e':
            /* Set encoder type */
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  pProgramArg->oArgFlags.nEncTypeDef = 1;
                  pProgramArg->nEnCoderType = atoi(ifx_optarg);
                  break;
               }
            } /* if (ifx_optarg != IFX_NULL) */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set encoder type.\n (File: %s, line: %d)\n",
                    __FILE__, __LINE__));
            return IFX_ERROR;
         case 'f':
            /* Set packetisation time */
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  pProgramArg->oArgFlags.nFrameLen = 1;
                  pProgramArg->nPacketisationTime = atoi(ifx_optarg);
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set packetisation time.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
         case TAPIDEMO_OPTION_NUMBER_VAD:
            /* voice activity detection (VAD) configuration */
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  if ((IFX_TAPI_ENC_VAD_NOVAD <= arg_value) &&
                      (IFX_TAPI_ENC_VAD_SC_ONLY >= arg_value))
                  {
                     pProgramArg->nVadCfg = arg_value;
                     switch (arg_value)
                     {
                        case IFX_TAPI_ENC_VAD_NOVAD:
                           /* TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                              ("Set VAD: %d - IFX_TAPI_ENC_VAD_NOVAD - "
                               "no voice activity detection.\n", arg_value)); */
                           break;
                        case IFX_TAPI_ENC_VAD_ON:
                           /* TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                              ("Set VAD: %d - IFX_TAPI_ENC_VAD_ON - "
                               "voice activity detection on.\n", arg_value)); */
                           break;
                        case IFX_TAPI_ENC_VAD_G711:
                           /* TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                              ("Set VAD: %d - IFX_TAPI_ENC_VAD_G711 - "
                               "voice activity detection on without "
                               "spectral information.\n", arg_value)); */
                           break;
                        case IFX_TAPI_ENC_VAD_SC_ONLY:
                           /* TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                              ("Set VAD: %d - IFX_TAPI_ENC_VAD_SC_ONLY - "
                               "voice activity detection on without "
                               "comfort noise generation.\n", arg_value)); */
                           break;
                        default:
                           TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                              ("Invalid vad argument %d, using default.\n",
                               arg_value));
                           pProgramArg->nVadCfg = DEFAULT_VAD_CFG;
                           break;
                     } /* switch (arg_value) */
                  }
                  else
                  {
                     TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                        ("Invalid vad argument %d, using default.\n",
                         arg_value));
                  }
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set voice activity detection.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
         case TAPIDEMO_OPTION_NUMBER_OOB_TX:
            if (IFX_NULL != ifx_optarg)
            {
               if (!Common_IsNumber(ifx_optarg))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Err, Invalid OOB TX argument - %s.\n (File: %s, line: %d)\n",
                      ifx_optarg, __FILE__, __LINE__));
                  break;
               }
               oVoipCfg.oRtpConf.bPrintSettings = IFX_TRUE;
               arg_value = atoi(ifx_optarg);
               switch (arg_value)
               {
                  case 0:
                     oVoipCfg.oRtpConf.nOobEvents = IFX_TAPI_PKT_EV_OOB_DEFAULT;
                     break;
                  case 1:
                     oVoipCfg.oRtpConf.nOobEvents = IFX_TAPI_PKT_EV_OOB_NO;
                     break;
                  case 2:
                     oVoipCfg.oRtpConf.nOobEvents = IFX_TAPI_PKT_EV_OOB_ONLY;
                     break;
                  case 3:
                     oVoipCfg.oRtpConf.nOobEvents = IFX_TAPI_PKT_EV_OOB_ALL;
                     break;
                  case 4:
                     oVoipCfg.oRtpConf.nOobEvents = IFX_TAPI_PKT_EV_OOB_BLOCK;
                     break;
                  default:
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                        ("Warning, Setting OOB TX to unknown value %d.\n",
                         arg_value));
                     oVoipCfg.oRtpConf.nOobEvents = arg_value;
                     break;
               }
               break;
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Err, Can't set OOB TX setting.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
            break;
         case TAPIDEMO_OPTION_NUMBER_OOB_RX:
            if (IFX_NULL != ifx_optarg)
            {
               if (!Common_IsNumber(ifx_optarg))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Err, Invalid OOB RX argument - %s.\n (File: %s, line: %d)\n",
                      ifx_optarg, __FILE__, __LINE__));
                  break;
               }
               oVoipCfg.oRtpConf.bPrintSettings = IFX_TRUE;
               arg_value = atoi(ifx_optarg);
               switch (arg_value)
               {
                  case 0:
                     oVoipCfg.oRtpConf.nOobPlayEvents = IFX_TAPI_PKT_EV_OOBPLAY_DEFAULT;
                     break;
                  case 1:
                     oVoipCfg.oRtpConf.nOobPlayEvents = IFX_TAPI_PKT_EV_OOBPLAY_PLAY;
                     break;
                  case 2:
                     oVoipCfg.oRtpConf.nOobPlayEvents = IFX_TAPI_PKT_EV_OOBPLAY_MUTE;
                     break;
                  default:
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                        ("Warning, Setting OOB RX to unknown value %d.\n",
                         arg_value));
                     oVoipCfg.oRtpConf.nOobPlayEvents = arg_value;
                     break;
               }
               break;
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Err, Can't set OOB RX setting.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
            break;
         case TAPIDEMO_OPTION_NUMBER_LEC:
            if (IFX_SUCCESS != ReadOptions_LEC(pProgramArg))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                      ("Can't set LEC. Use default LEC setting.\n"));
               pProgramArg->nLecCfg = DEFAULT_LEC_CFG;
            }
            break;
         case TAPIDEMO_OPTION_NUMBER_NO_LEC:
            /* do not enable LEC - obsolete */
            pProgramArg->oArgFlags.nNoLEC = 1;
            break;
#ifdef TD_DECT
         case TAPIDEMO_OPTION_NUMBER_DECT_EC:
            if (IFX_SUCCESS != ReadOptions_DECT_EC(pProgramArg))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                      ("Can't set ES for DECT. "
                       "Use default ES settings for DECT channels.\n"));
               pProgramArg->nDectEcCfg = DEFAULT_DECT_EC_CFG;
            }
            break;
#endif /* TD_DECT */
         case 'n':
            if (IFX_SUCCESS != ReadOptions_Network(pProgramArg))
            {
               return IFX_ERROR;
            }
            break;
         case 'b':
            /* Set board combination */
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  if ((0 > arg_value) || (UNSUPPORTED_COMB <= arg_value))
                  {
                     /* invalid board combination, don't change it */
                     TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                        ("Unknown board combination selected %d, ignoring.\n",
                         (int) arg_value));
                  }
                  else
                  {
                     /* set board combination */
                     pProgramArg->nBoardComb = arg_value;
                     pProgramArg->oArgFlags.nUseCombinationBoard = 1;
                  }
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Can't set board combination.\n (File: %s, line: %d)\n",
                    __FILE__, __LINE__));
            return IFX_ERROR;
         case 'v':
            /* enable initialization test */
            g_pITM_Ctrl->oVerifySytemInit.fEnabled = IFX_TRUE;
            /* enable waiting for file */
            g_pITM_Ctrl->oVerifySytemInit.fFileDwnld = IFX_TRUE;
            break;
         case 'm':
            /* enable ITM */
#ifdef LINUX
            g_pITM_Ctrl->fITM_Enabled = IFX_TRUE;
#else /* LINUX */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("ITM is not currently supported for VxWorks.\n"));
            g_pITM_Ctrl->fITM_Enabled = IFX_FALSE;
#endif /* LINUX */
            break;
#ifdef LINUX
         case 'x':
            /* FXO */
            if (IFX_SUCCESS != ReadOptions_Fxo(pProgramArg))
            {
               return IFX_ERROR;
            }
            break;
         case 'l':
            /* Path to download files */
            if (IFX_NULL == ifx_optarg)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                      ("Can't set path to download files.\n "
                       "(File: %s, line: %d)\n",
                        __FILE__, __LINE__));
               return IFX_ERROR;
            }

            strncpy(pProgramArg->sPathToDwnldFiles, ifx_optarg, MAX_PATH_LEN - 1);
            pProgramArg->oArgFlags.nUseCustomDownloadsPath = IFX_TRUE;
            break;
#endif /* LINUX */

#ifdef DXT
         case 'P':
            /* use polling mode in LL driver - now only for DxT */
            pProgramArg->oArgFlags.nPollingMode = 1;
            break;
#endif /* DXT */

         case TAPIDEMO_OPTION_NUMBER_GROUND_START:
            /* enable Ground Start support */
            pProgramArg->oArgFlags.nGroundStart = 1;
            break;

         case 't':
#ifndef TD_DECT
            /* DECT argument used without DECT support */
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Invalid command line argument - compiled without DECT support.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
#else /* TD_DECT */
            /* enable DECT */
            pProgramArg->oArgFlags.nNoDect = 0;
            /* enable QoS - without it external calls with DECT will not work */
            pProgramArg->oArgFlags.nQos = 1;
#ifdef FIO_QOS_ON_SOCKET_START
            /* by default use FIO_QOS_ON_SOCKET_START if available */
            pProgramArg->oArgFlags.nQosSocketStart = 1;
#else
            pProgramArg->oArgFlags.nQosSocketStart = 0;
#endif /* FIO_QOS_ON_SOCKET_START */
#endif /* TD_DECT */
            break;
#ifdef TD_RM_DEBUG
         case TAPIDEMO_OPTION_NUMBER_RM_DBG_RES:
            /* enable RM debug (resource counting) */
            pProgramArg->oArgFlags.nRM_Count = 1;
            break;
#endif /* TD_RM_DEBUG */
#ifdef TD_DECT
         case TAPIDEMO_OPTION_NUMBER_DECT_DBG:
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  if ((arg_value > DBG_LEVEL_OFF) || (arg_value < DBG_LEVEL_LOW))
                  {
                     arg_value = DBG_LEVEL_OFF;
                  }
                  pProgramArg->nDectDbgLevel = arg_value;
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set dect debug level.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
#endif /* TD_DECT */
        case TAPIDEMO_OPTION_PRINT_VERSION:
            pProgramArg->oArgFlags.nVersion = 1;
            break;
#ifdef LINUX
         case TAPIDEMO_OPTION_PRINT_CONFIGURE:
            pProgramArg->oArgFlags.nConfigure = 1;
            break;
#endif /* LINUX */

         case TAPIDEMO_OPTION_NUMBER_NO_PPD:
#ifndef TD_PPD
            /* DECT argument used without PPD support */
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Invalid command line argument - compiled without PPD support.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
#else
            /* enable Phone Plug Detection. */
            pProgramArg->oArgFlags.nDisablePpd = 1;
#endif
            break;

         case TAPIDEMO_OPTION_NUMBER_T1:
#ifndef TD_PPD
            /* DECT argument used without PPD support */
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Invalid command line argument - compiled without PPD support.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
#else
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  pProgramArg->nTimeoutT1 = arg_value;
                  pProgramArg->nPpdFlag |= TD_PPD_FLAG_T1;
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set T1 timeout.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
#endif
            break;

         case TAPIDEMO_OPTION_NUMBER_T2:
#ifndef TD_PPD
            /* DECT argument used without PPD support */
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Invalid command line argument - compiled without PPD support.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
#else
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  pProgramArg->nTimeoutT2 = arg_value;
                  pProgramArg->nPpdFlag |= TD_PPD_FLAG_T2;
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Can't set T2 timeout.\n (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
#endif
            break;

         case TAPIDEMO_OPTION_NUMBER_T3:
#ifndef TD_PPD
            /* DECT argument used without PPD support */
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Invalid command line argument - compiled without PPD support.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
#else
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  pProgramArg->nTimeoutT3 = arg_value;
                  pProgramArg->nPpdFlag |= TD_PPD_FLAG_T3;
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set T3 timeout.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
#endif
            break;

         case TAPIDEMO_OPTION_NUMBER_CAP:
#ifndef TD_PPD
            /* DECT argument used without PPD support */
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Invalid command line argument - compiled without PPD support.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
#else
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  pProgramArg->nCapacitance = arg_value;
                  pProgramArg->nPpdFlag |= TD_PPD_FLAG_CAP;
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, Can't set telephone capacitance threshold.\n"
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
#endif
            break;

         case TAPIDEMO_OPTION_NUMBER_TAPI_DBG:
            if (IFX_NULL != ifx_optarg)
            {
               if (Common_IsNumber(ifx_optarg))
               {
                  arg_value = atoi(ifx_optarg);
                  if ((arg_value > IFX_TAPI_DEBUG_REPORT_SET_HIGH) ||
                      (arg_value < IFX_TAPI_DEBUG_REPORT_SET_OFF))
                  {
                     arg_value = IFX_TAPI_DEBUG_REPORT_SET_HIGH;
                  }
                  pProgramArg->nTapiDbgLevel = arg_value;
                  break;
               }
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                   ("Can't set tapi debug level.\n (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
            return IFX_ERROR;
#ifdef DXT
         case TAPIDEMO_OPTION_NUMBER_DXT_ROM_FW:
            /* FW is located in ROM for DxT device */
            bDxT_RomFW = IFX_TRUE;
            break;
#endif /* DXT */
         case TAPIDEMO_OPTION_NUMBER_NO_BBD_DWLD:
            /* do not download BBD */
            g_bBBD_Dwld = IFX_FALSE;
            break;
         case 'T':
            /* enable printing ioctl data */
            g_bPrintIoctl = IFX_TRUE;
            break;
         case 'C':
            /* disable printing prefix before each trace */
            g_bAddPrefix = IFX_FALSE;
            break;
         case TAPIDEMO_OPTION_NUMBER_EL:
            /* send traces to Event Logger */
#ifndef EVENT_LOGGER_DEBUG
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Invalid command line argument - compiled without "
                "Event Logger support.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
#else
            g_bUseEL = IFX_TRUE;
#endif
            break;

#ifdef TD_IPV6_SUPPORT
         case TAPIDEMO_OPTION_NUMBER_IPV6:
#if defined(EASY336) && !defined(CONFIG_LTQ_SVIP_NAT_PT)
         /* CONFIG_LTQ_SVIP_NAT_PT must be available for SVIP to support IPv6 */
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Err, compiled withou CONFIG_LTQ_SVIP_NAT_PT, "
                "no IPv6 support available.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
            pProgramArg->oArgFlags.nUseIPv6 = 0;
#else /* defined(EASY336) && !defined(CONFIG_LTQ_SVIP_NAT_PT) */
            /* enable IPv6 */
            pProgramArg->oArgFlags.nUseIPv6 = 1;
#endif /* defined(EASY336) && !defined(CONFIG_LTQ_SVIP_NAT_PT) */
            break;
         case TAPIDEMO_OPTION_NUMBER_PHONEBOOK:
            /* Path to download files */
            if (IFX_NULL == ifx_optarg)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                      ("Can't set path to phonebook.\n "
                       "(File: %s, line: %d)\n",
                        __FILE__, __LINE__));
               return IFX_ERROR;
            }

            strncpy(pProgramArg->sPathToPhoneBook, ifx_optarg, MAX_PATH_LEN - 1);
            pProgramArg->oArgFlags.nUseCustomPhonebookPath = IFX_TRUE;
            break;
#endif /* TD_IPV6_SUPPORT */

#ifdef USE_FILESYSTEM
         case TAPIDEMO_OPTION_FILE_FW:
            /* FW filename */
            if ((IFX_NULL == ifx_optarg) || (MAX_PATH_LEN <= strlen(ifx_optarg)))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                      ("Can't set FW filename.\n (File: %s, line: %d)\n",
                        __FILE__, __LINE__));
               return IFX_ERROR;
            }

            strncpy(pProgramArg->sFwFileName, ifx_optarg, MAX_PATH_LEN - 1);
            break;
         case TAPIDEMO_OPTION_FILE_DRAM:
            /* DRAM filename */
            if ((IFX_NULL == ifx_optarg) || (MAX_PATH_LEN <= strlen(ifx_optarg)))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                      ("Can't set DRAM filename.\n (File: %s, line: %d)\n",
                        __FILE__, __LINE__));
               return IFX_ERROR;
            }

            strncpy(pProgramArg->sDramFileName, ifx_optarg, MAX_PATH_LEN - 1);
            break;
         case TAPIDEMO_OPTION_FILE_BBD:
            /* BBD filename */
            if ((IFX_NULL == ifx_optarg) || (MAX_PATH_LEN <= strlen(ifx_optarg)))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                      ("Can't set BBD filename.\n (File: %s, line: %d)\n",
                        __FILE__, __LINE__));
               return IFX_ERROR;
            }

            strncpy(pProgramArg->sBbdFileName, ifx_optarg, MAX_PATH_LEN - 1);
            break;
#endif /* USE_FILESYSTEM */
         default:
            /* Unknown argument */
            /* Go out if unknown arg */
#ifdef LINUX
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                ("Unknown or incomplete arguments: %c. (File: %s, line: %d)\n",
                 optopt, __FILE__, __LINE__));
#else
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Unknown or incomplete arguments. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
#endif /* LINUX */
            return IFX_ERROR;
      } /* switch */
   } /* while */

#ifdef LINUX
   /* Check if all arguments were parsed */
   /* Print unknown/incorrect one */
   if (optind < nArgCnt)
   {
       TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("Unknown or incorrect arguments: "));
       while (optind < nArgCnt)
          TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("%s ", pArgv[optind++]));
       TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("\n"));

       return IFX_ERROR;
   }
#endif /* LINUX */
   /* verify system initialization can only be used when test mode is on */
   if (g_pITM_Ctrl->oVerifySytemInit.fEnabled && !g_pITM_Ctrl->fITM_Enabled)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("Warning, verify system initialization cannot be used "
          "when test mode isn't turned on\n"));
      g_pITM_Ctrl->oVerifySytemInit.fEnabled = IFX_FALSE;
   }
   return IFX_SUCCESS;
} /* ReadOptions() */

/**
   Displays program usage with arguments.

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t ProgramHelp(IFX_void_t)
{
   IFX_int32_t i = 0;
   IFX_int32_t j = 0;

   TAPIDEMO_PRINTF(TD_CONN_ID_HELP, ("Program usage.\n"));

   TAPIDEMO_PRINTF(TD_CONN_ID_HELP, ("Usage: <%s> [options]\r\n",
                                     (IFX_char_t *) PROGRAM_NAME));
   TAPIDEMO_PRINTF(TD_CONN_ID_HELP, ("Following options defined:\r\n\n"));
   i = 0;
   j = 0;
   while ((PROGRAM_OPTIONS[i].name) && (0 != strlen(helpItems[j])))
   {
      /* print option number and long name */
      TAPIDEMO_PRINTF(TD_CONN_ID_HELP,
         ("(%d) '--%s'", (int)i + 1, PROGRAM_OPTIONS[i].name));
      /* print option short name if avaible */
      if ((0 < PROGRAM_OPTIONS[i].val) && ((255 > PROGRAM_OPTIONS[i].val)))
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_HELP, (" OR '-%c'", PROGRAM_OPTIONS[i].val));
      }
      /* print option help first line */
      TAPIDEMO_PRINTF(TD_CONN_ID_HELP, (" : %s\n", helpItems[j]));
      j++;
      while (0 != strlen(helpItems[j]))
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_HELP, ("\t%s\n", helpItems[j]));
         j++;
      }
      /* Jump over delimiter for help of next argument. */
      j++;

      i++;
   }

   return IFX_SUCCESS;
} /* ProgramHelp() */

/**
   Print used program arguments.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_SUCCESS arguments ok, otherwise IFX_ERROR
*/
IFX_return_t PrintUsedOptions(PROGRAM_ARG_t* pProgramArg)
{
   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

#ifdef TD_DECT
   if (!pProgramArg->oArgFlags.nNoDect)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- DECT support.\n"));
   }
#endif /* TD_DECT */

#ifdef TD_PPD
   if (pProgramArg->oArgFlags.nDisablePpd)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- disabled FXS Phone Plug Detection support.\n"));
   }
#endif /* TD_PPD */

   if (pProgramArg->oArgFlags.nFXO)
   {
#ifdef FXO
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- FXO support on.\n"));
#else
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- FXO support unavailable.\n"
          "  Please rebuild with all required header files.\n"));
#endif /* FXO */
   } /* FXO */

   if (!pProgramArg->oArgFlags.nConference)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- Conference support disabled.\n"));
   } /* Conference */
   /* CID */
   if (pProgramArg->oArgFlags.nCID)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- CID support: %s (standard %d).\n",
          Common_Enum2Name(CID_GetStandard(), TD_rgCidStandardName),
          CID_GetStandard()));
   } /* CID */
   else
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- CID support: disabled.\n"));
   }

   switch (pProgramArg->nVadCfg)
   {
      case IFX_TAPI_ENC_VAD_NOVAD:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- VAD support: IFX_TAPI_ENC_VAD_NOVAD - "
             "no voice activity detection.\n"));
         break;
      case IFX_TAPI_ENC_VAD_ON:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- VAD support: IFX_TAPI_ENC_VAD_ON - "
             "voice activity detection on.\n"));
         break;
      case IFX_TAPI_ENC_VAD_G711:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- VAD support: IFX_TAPI_ENC_VAD_G711 - "
             "voice activity detection on without spectral information.\n"));
         break;
      case IFX_TAPI_ENC_VAD_SC_ONLY:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- VAD support: IFX_TAPI_ENC_VAD_SC_ONLY - "
             "voice activity detection on without comfort noise generation.\n"));
         break;
      default:
         break;
   } /* VAD */

   switch (pProgramArg->nLecCfg)
   {
      case IFX_TAPI_WLEC_TYPE_OFF:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- LEC support: IFX_TAPI_WLEC_TYPE_OFF - \n"
             "               LEC and Echo Suppressor turned off.\n"));
         break;
      case IFX_TAPI_WLEC_TYPE_NE:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- LEC support: IFX_TAPI_WLEC_TYPE_NE - \n"
             "               LEC using fixed window. No Echo Suppressor.\n"));
         break;
      case IFX_TAPI_WLEC_TYPE_NFE:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- LEC support: IFX_TAPI_WLEC_TYPE_NFE - \n"
             "               LEC using fixed and moving window. No Echo Suppressor.\n"));
         break;
      case IFX_TAPI_WLEC_TYPE_NE_ES:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- LEC support: IFX_TAPI_WLEC_TYPE_NE_ES - \n"
             "               LEC using fixed window + Echo Suppressor.\n"));
         break;
      case IFX_TAPI_WLEC_TYPE_NFE_ES:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- LEC support: IFX_TAPI_WLEC_TYPE_NFE_ES - \n"
             "               LEC using fixed and moving window + Echo Suppressor.\n"));
         break;
      case IFX_TAPI_WLEC_TYPE_ES:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- LEC support: IFX_TAPI_WLEC_TYPE_ES - Echo Suppressor.\n"));
         break;
   }

   if (IFX_TRUE == oVoipCfg.oRtpConf.bPrintSettings)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Out-Of-band settings for RTP packets:\n"));
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("   - %s,\n",
          Common_Enum2Name(oVoipCfg.oRtpConf.nOobEvents,
                           TD_rgTapiOOB_EventDesc)));
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("   - %s.\n",
          Common_Enum2Name(oVoipCfg.oRtpConf.nOobPlayEvents,
                           TD_rgTapiOOB_EventPlayDesc)));
   }


   if (pProgramArg->oArgFlags.nNoLEC)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- LEC support disabled.\n"));
   } /* LEC */

#ifdef TD_DECT
   switch (pProgramArg->nDectEcCfg)
   {
      case IFX_TAPI_EC_TYPE_OFF:
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- Echo Suppressor for DECT channels turned off.\n"));
         break;
      case IFX_TAPI_EC_TYPE_ES:
          TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- Echo Suppressor for DECT channels turned on.\n"));
         break;
   }
#endif

   if (pProgramArg->oArgFlags.nPCM_Slave)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- PCM slave mode.\n"));
   }

   if (pProgramArg->oArgFlags.nPCM_Master)
   {
      if (pProgramArg->oArgFlags.nPCM_Loop)
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- PCM master mode with local loop.\n"));
      }
      else
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- PCM master mode.\n"));
      }
   } /* PCM */

   /* If using custom PCM codec */
   if (pProgramArg->oArgFlags.nPCM_TypeDef)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- PCM NB codec used - %s. %s\n"
          "- PCM WB codec used - %s. %s\n",
          Common_Enum2Name(g_oPCM_Data.ePCM_NB_Resolution,
                           TD_rgEnumPCM_CodecName),
          (TD_NARROWBAND == g_oPCM_Data.nBand)?"(default)":"",
          Common_Enum2Name(g_oPCM_Data.ePCM_WB_Resolution,
                           TD_rgEnumPCM_CodecName),
          (TD_WIDEBAND == g_oPCM_Data.nBand)?"(default)":""));
   }

   /* if using coders for local calls */
   if (pProgramArg->oArgFlags.nUseCodersForLocal)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- Coders for local connections.\n"));
   }

      /* if QoS flags are set */
   if (pProgramArg->oArgFlags.nQOS_Local)
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- Local UDP redirection.\n"));
   } /* Local UDP */

   if (pProgramArg->oArgFlags.nQos)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- External UDP redirection.\n"));
   } /* External UDP */

   if (pProgramArg->oArgFlags.nDiffNet)
   {
      IFX_char_t sNetworkAddr[TD_IP_ADDRESS_LEN + 1] = {0};
      IFX_int32_t i, j;

      /* copy string representation of address */
      strncpy(sNetworkAddr, inet_ntoa(pProgramArg->oDiffNetIP),
              TD_IP_ADDRESS_LEN);
      sNetworkAddr[TD_IP_ADDRESS_LEN] = '\0';
      /* remove last part of IP address */
      for (i=0, j=0; i<TD_IP_ADDRESS_LEN; i++)
      {
         if ('.' == sNetworkAddr[i])
         {
            /* increase counter */
            j++;
            /* only 3 first bytes are network address */
            if (3 == j)
            {
               /* set end of string */
               sNetworkAddr[i] = '\0';
               break;
            } /* if (3 == j) */
         } /* if ('.' == sNetworkAddr[i]) */
         else if (!isdigit(sNetworkAddr[i]))
         {
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Err, Invalid calling network address: %s."
                "(File: %s, line: %d)\n",
                inet_ntoa(pProgramArg->oDiffNetIP),
                __FILE__, __LINE__));
            break;
         }
      } /* for (i=0, j=0; i<MAX_ARGUMENT_LEN; i++) */
      /* check for null termination of string */
      if (sNetworkAddr[i] == '\0')
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("- Calling network address: %s.\n", sNetworkAddr));
      }
      else
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("Err, Invalid calling network address: %s.\n",
             inet_ntoa(pProgramArg->oDiffNetIP)));

      }
   } /* Calling network address */

   if (pProgramArg->oArgFlags.nWait)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Waiting for user input in state machine.\n"));
   } /* Wait */

   if (pProgramArg->oArgFlags.nDaemon)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- Daemon mode with "));
   } /* Daemon */
   if (pProgramArg->nTraceRedirection == TRACE_REDIRECTION_SYSLOG_LOCAL)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("redirecting logs to local syslog.\n"));
   } /* Log redirection */
   else if (pProgramArg->nTraceRedirection == TRACE_REDIRECTION_SYSLOG_REMOTE)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("redirecting logs to remote syslog.\n"));
   } /* Log redirection */
   else if (pProgramArg->nTraceRedirection == TRACE_REDIRECTION_FILE)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("redirecting logs to file.\n"));
   } /* Log redirection */

   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- Debug level: %s.\n",
                    Common_Enum2Name(pProgramArg->nDbgLevel,
                                     TD_rgDebugLevelName)));

#ifdef EASY336
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- RM Debug level: %s.\n",
          Common_Enum2Name(pProgramArg->nRmDbgLevel, TD_rgDebugLevelName)));
#endif /* EASY336 */
   if (IFX_FALSE == g_bBBD_Dwld)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- No BBD download.\n"));
   }

#ifdef TD_FAX_T38
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("- Transparent fax/modem and T.38 transmission support.\n"));
#else
   #ifdef TD_FAX_MODEM
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Transparent fax/modem transmission support.\n"));
   #endif /* TD_FAX_MODEM */
#endif /* TD_FAX_T38 */

   if (pProgramArg->oArgFlags.nGroundStart)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Ground Start support enabled.\n"));
   }
   else
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Ground Start support disabled.\n"));
   }

#ifdef TD_RM_DEBUG
   /* enable RM debug (resource counting) */
   if (pProgramArg->oArgFlags.nRM_Count)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- RM debug (resource counting) on.\n"));
   }
#endif /* TD_RM_DEBUG */
   /* enable printing of ioctls' names and arguments */
   if (IFX_TRUE == g_bPrintIoctl)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Printing used ioctls.\n"));
   }
   /* enable printing of ioctls' names and arguments */
   if (IFX_TRUE != g_bAddPrefix)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Printing prefix with conn id disabled.\n"));
   }
   /* enable ITM Internal Test Mode */
   if (g_pITM_Ctrl->fITM_Enabled)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Internal Test Mode (ITM) enabled.\n"));
   }

#ifdef TD_IPV6_SUPPORT
   if (pProgramArg->oArgFlags.nUseIPv6)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Using IPv6.\n"));
   }
#endif /* TD_IPV6_SUPPORT */

#ifdef EVENT_LOGGER_DEBUG
   if (g_bUseEL == IFX_TRUE)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Redirection to Event Logger.\n"));
   }
#endif /* EVENT_LOGGER_DEBUG */

#ifdef DXT
   if (pProgramArg->oArgFlags.nPollingMode)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("- Polling mode for DxT enabled.\n"));
   }
#endif /* DXT */

   return IFX_SUCCESS;
} /* PrintUsedOptions */

