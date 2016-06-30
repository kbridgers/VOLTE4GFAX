/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file control_pc.c
   \date 2007-08-20
   \brief Implementation of communication module for application.

*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "com_client.h"
#include "control_pc.h"
#include "common.h"
#include "abstract.h"
#include "voip.h"
#include "pcm.h"
#include "tapidemo.h"
#include "state_trans.h"
#include "analog.h"
#include "cid.h"
#include "qos.h"
#include "common_fxo.h"

#ifdef DXT
/* #warning!! temporary solution - this struct should be included. */
typedef struct
{
   /** block based download buffer,
       big-endian aligned */
   IFX_uint8_t *buf;
   /** size of buffer in bytes */
   IFX_uint32_t size;
} bbd_format_t;
#endif

#ifdef LINUX

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Local Structures             */
/* ============================= */


/* Command Ids received from Control PC*/
typedef enum _COM_COMMAND_ID_t
{
   COM_ID_UNKNOWN                               = 0x00,
   COM_ID_SET_TEST_TYPE                         = 0x01,
   COM_ID_SET_TRACE_LEVEL                       = 0x02,
   COM_ID_SET_PHONES                            = 0x03,
   COM_ID_GET_PHONES                            = 0x04,
   COM_ID_SET_SUT_COUNTRY_SETTINGS              = 0x05,
   COM_ID_GET_SUT_COUNTRY_SETTINGS              = 0x06,
   COM_ID_BROADCAST_MSG                         = 0x07,
   COM_ID_COM_RESET                             = 0x08,
   COM_ID_CONFIG_CHANNELS                       = 0x09,
   COM_ID_SET_CH_INIT_VOICE                     = 0x0a,
   COM_ID_SET_LINE_FEED_SET_STANDBY             = 0x0b,
   COM_ID_SET_LINE_FEED_SET_ACTIVE              = 0x0c,
   COM_ID_COM_CHANGE_CODEC                      = 0x0d,
   COM_ID_GENERATE_TONES                        = 0x0e,
   COM_ID_SET_INTERNAL_SUT_CHANNEL_CONNECTION   = 0x0f,
   COM_ID_SET_MODULAR_TEST_TYPE                 = 0x10,
   COM_ID_RUN_PROGRAM                           = 0x11,
   COM_ID_ANALOG_TO_IP_CALL_CTRL                = 0x12,
   COM_ID_SET_SUT_CPT_COUNTRY_SETTINGS          = 0x13,
   COM_ID_REPEAT_SYSINIT_TEST                   = 0x14,
   COM_ID_TAPI_RESTART                          = 0x15,
   COM_ID_GET_SYSINIT_TYPE                      = 0x16,
   COM_ID_ADD_ARP_ENTRY                         = 0x17,
   COM_ID_PRINT_TXT                             = 0x18,
   COM_ID_SET_CID                               = 0x19,
   COM_ID_SET_CID_DATA                          = 0x1a,
   COM_ID_GET_INFO                              = 0x1b,
   COM_ID_SET_LEC                               = 0x1c,
   COM_ID_CHECK_LEC                             = 0x1d,
   COM_ID_PING                                  = 0x1e,
   COM_ID_SET_PORT_NUMBER                       = 0x1f,
   COM_ID_RCV_FILE                              = 0x20,
   COM_ID_CTRL_PROG_INIT                        = 0x21,
   COM_ID_OOB_CFG                               = 0x22,
   COM_ID_MAX
} COM_COMMAND_ID_t;

/** names of Call types */
TD_ENUM_2_NAME_t TD_rgMsgIdName[] =
{
   {COM_ID_UNKNOWN, "COM_ID_UNKNOWN"},
   {COM_ID_SET_TEST_TYPE, "COM_ID_SET_TEST_TYPE"},
   {COM_ID_SET_TRACE_LEVEL, "COM_ID_SET_TRACE_LEVEL"},
   {COM_ID_SET_PHONES, "COM_ID_SET_PHONES"},
   {COM_ID_GET_PHONES, "COM_ID_GET_PHONES"},
   {COM_ID_SET_SUT_COUNTRY_SETTINGS, "COM_ID_SET_SUT_COUNTRY_SETTINGS"},
   {COM_ID_GET_SUT_COUNTRY_SETTINGS, "COM_ID_GET_SUT_COUNTRY_SETTINGS"},
   {COM_ID_BROADCAST_MSG, "COM_ID_BROADCAST_MSG"},
   {COM_ID_COM_RESET, "COM_ID_COM_RESET"},
   {COM_ID_CONFIG_CHANNELS, "COM_ID_CONFIG_CHANNELS"},
   {COM_ID_SET_CH_INIT_VOICE, "COM_ID_SET_CH_INIT_VOICE"},
   {COM_ID_SET_LINE_FEED_SET_STANDBY, "COM_ID_SET_LINE_FEED_SET_STANDBY"},
   {COM_ID_SET_LINE_FEED_SET_ACTIVE, "COM_ID_SET_LINE_FEED_SET_ACTIVE"},
   {COM_ID_COM_CHANGE_CODEC, "COM_ID_COM_CHANGE_CODEC"},
   {COM_ID_GENERATE_TONES, "COM_ID_GENERATE_TONES"},
   {COM_ID_SET_INTERNAL_SUT_CHANNEL_CONNECTION,
      "COM_ID_SET_INTERNAL_SUT_CHANNEL_CONNECTION"},
   {COM_ID_SET_MODULAR_TEST_TYPE, "COM_ID_SET_MODULAR_TEST_TYPE"},
   {COM_ID_RUN_PROGRAM, "COM_ID_RUN_PROGRAM"},
   {COM_ID_ANALOG_TO_IP_CALL_CTRL, "COM_ID_ANALOG_TO_IP_CALL_CTRL"},
   {COM_ID_SET_SUT_CPT_COUNTRY_SETTINGS, "COM_ID_SET_SUT_CPT_COUNTRY_SETTINGS"},
   {COM_ID_REPEAT_SYSINIT_TEST, "COM_ID_REPEAT_SYSINIT_TEST"},
   {COM_ID_TAPI_RESTART, "COM_ID_TAPI_RESTART"},
   {COM_ID_GET_SYSINIT_TYPE, "COM_ID_GET_SYSINIT_TYPE"},
   {COM_ID_ADD_ARP_ENTRY, "COM_ID_ADD_ARP_ENTRY"},
   {COM_ID_PRINT_TXT, "COM_ID_PRINT_TXT"},
   {COM_ID_MAX, "COM_ID_MAX"},
   {COM_ID_SET_CID, "COM_ID_SET_CID"},
   {COM_ID_SET_CID_DATA, "COM_ID_SET_CID_DATA"},
   {COM_ID_GET_INFO, "COM_ID_GET_INFO"},
   {COM_ID_SET_LEC, "COM_ID_SET_LEC"},
   {COM_ID_CHECK_LEC, "COM_ID_CHECK_LEC"},
   {COM_ID_PING, "COM_ID_PING"},
   {COM_ID_SET_PORT_NUMBER, "COM_ID_SET_PORT_NUMBER"},
   {COM_ID_RCV_FILE, "COM_ID_RCV_FILE"},
   {COM_ID_CTRL_PROG_INIT, "COM_ID_CTRL_PROG_INIT"},
   {COM_ID_OOB_CFG, "COM_ID_OOB_CFG"},
   {TD_MAX_ENUM_ID, "COM_COMMAND_ID_t"}
};

/** Sizes of Keys in corresponding command frames */
typedef enum _COM_COMMAND_KEY_SIZE_t
{
   KEY_SIZE_COM_UNKNOWN_COMMAND = 0x00,
   KEY_SIZE_SET_TEST_TYPE = 0x00,
   KEY_SIZE_SET_TRACE_LEVEL = 0x00,
   KEY_SIZE_SET_PHONES = 0x08,
   KEY_SIZE_GET_PHONES = 0x00,
   KEY_SIZE_SET_SUT_COUNTRY_SETTINGS = 0x01,
   KEY_SIZE_GET_SUT_COUNTRY_SETTINGS = 0x00,
   KEY_SIZE_BROADCAST_MSG = 0x00,
   KEY_SIZE_COM_RESET = 0x00,
   KEY_SIZE_CONFIG_CHANNELS = 0x00,
   KEY_SIZE_SET_CH_INIT_VOICE = 0x04,
   KEY_SIZE_SET_LINE_FEED_SET_STANDBY = 0x04,
   KEY_SIZE_SET_LINE_FEED_SET_ACTIVE = 0x04,
   KEY_SIZE_COM_CHANGE_CODEC = 0x00,
   KEY_SIZE_GENERATE_TONES = 0x04,
   KEY_SIZE_SET_INTERNAL_SUT_CHANNEL_CONNECTION = 0x04,
   KEY_SIZE_SET_MODULAR_TEST_TYPE = 0x01,
   KEY_SIZE_SET_RUN_PROGRAM = NO_DEFAULT_KEY_VALUE_LENGTH,
   KEY_SIZE_SET_ANALOG_TO_IP_CALL_CTRL = 0x01,
   KEY_SIZE_SET_SUT_CPT_COUNTRY_SETTINGS = NO_DEFAULT_KEY_VALUE_LENGTH,
   KEY_SIZE_REPEAT_SYSINIT_TEST = 0x00,
   KEY_SIZE_TAPI_RESTART = 0x00,
   KEY_SIZE_GET_SYSINIT_TYPE = 0x00,
   KEY_SIZE_ADD_ARP_ENTRY = 0x04,
   KEY_SIZE_PRINT_TXT = NO_DEFAULT_KEY_VALUE_LENGTH,
   KEY_SIZE_SET_CID = 0x00,
   KEY_SIZE_SET_CID_DATA = 0x01,
   KEY_SIZE_GET_INFO = 0x00,
   KEY_SIZE_SET_LEC = 0x01,
   KEY_SIZE_CHECK_LEC = 0x01,
   KEY_SIZE_PING = 0x00,
   KEY_SIZE_SET_PORT_NUMBER = 0x01,
   KEY_SIZE_RCV_FILE = 0x0D,
   KEY_SIZE_CTRL_PROG_INIT = 0x00,
   KEY_SIZE_OOB_CFG = 0x01,
} COM_COMMAND_KEY_SIZE_t;


/* Sizes of Value in corresponding command frames */
typedef enum _COM_COMMAND_VALUE_SIZE_t
{
   VALUE_SIZE_COM_UNKNOWN_COMMAND = 0x00,
   VALUE_SIZE_SET_TEST_TYPE = 0x01,
   VALUE_SIZE_SET_TRACE_LEVEL = 0x01,
   VALUE_SIZE_SET_PHONES = 0x08,
   VALUE_SIZE_GET_PHONES = 0x00,
   VALUE_SIZE_SET_SUT_COUNTRY_SETTINGS = 0x18,
   VALUE_SIZE_GET_SUT_COUNTRY_SETTINGS = 0x00,
   VALUE_SIZE_BROADCAST_MSG = 0x00,
   VALUE_SIZE_COM_RESET = 0x00,
   VALUE_SIZE_CONFIG_CHANNELS = 0x00,
   VALUE_SIZE_SET_CH_INIT_VOICE = 0x04,
   VALUE_SIZE_SET_LINE_FEED_SET_STANDBY = 0x04,
   VALUE_SIZE_SET_LINE_FEED_SET_ACTIVE = 0x04,
   VALUE_SIZE_COM_CHANGE_CODEC = 0x02,
   VALUE_SIZE_GENERATE_TONES = 0x04,
   VALUE_SIZE_SET_INTERNAL_SUT_CHANNEL_CONNECTION = 0x04,
   VALUE_SIZE_SET_MODULAR_TEST_TYPE = 0x01,
   VALUE_SIZE_SET_RUN_PROGRAM = NO_DEFAULT_KEY_VALUE_LENGTH,
   VALUE_SIZE_SET_ANALOG_TO_IP_CALL_CTRL = 0x06,
   VALUE_SIZE_SET_SUT_CPT_COUNTRY_SETTINGS = NO_DEFAULT_KEY_VALUE_LENGTH,
   VALUE_SIZE_REPEAT_SYSINIT_TEST = 0x01,
   VALUE_SIZE_TAPI_RESTART = 0x00,
   VALUE_SIZE_GET_SYSINIT_TYPE = 0x00,
   VALUE_SIZE_ADD_ARP_ENTRY_TYPE = 0x06,
   VALUE_SIZE_PRINT_TXT = NO_DEFAULT_KEY_VALUE_LENGTH,
   VALUE_SIZE_SET_CID = 0x02,
   VALUE_SIZE_SET_CID_DATA = NO_DEFAULT_KEY_VALUE_LENGTH,
   VALUE_SIZE_GET_INFO = 0x00,
   VALUE_SIZE_SET_LEC = 0x01,
   VALUE_SIZE_CHECK_LEC = 0x02,
   VALUE_SIZE_PING = 0x04,
   VALUE_SIZE_SET_PORT_NUMBER = 0x02,
   VALUE_SIZE_RCV_FILE = 0xFF,
   VALUE_SIZE_CTRL_PROG_INIT = 0x01,
   VALUE_SIZE_OOB_CFG = 0x04,
} COM_COMMAND_VALUE_SIZE_t;

/* Statuses of IFX_TAPI_LINE_FEED_SET tests
   (in verify system initialization test). */
typedef enum _COM_REPEAT_STATUS_t {
   REPEAT_STATUS_INVALID = 0,
   REPEAT_STATUS_END = 1,
   REPEAT_STATUS_REPEAT = 2
} COM_REPEAT_STATUS_t;

/* Statuses of IFX_TAPI_LINE_FEED_SET tests
   (in verify system initialization test). */
typedef enum _COM_OPTION_VALUE_TYPE_t {
   OPT_VALUE_STRING = 0,
   OPT_VALUE_NUMERIC,
} COM_OPTION_VALUE_TYPE_t;

/** values of key field in OOB setup message */
typedef enum
{
   /** key value for board no */
   OOB_CFG_BOARD_NO = 0x00,
   /** key value for device no */
   OOB_CFG_DEVICE_NO = 0x01,
   /** key value for channel no */
   OOB_CFG_DATA_CH_NO = 0x02,
   /** key value for OOB event setting */
   OOB_CFG_EVENT = 0x03,
   /** key value for OOB RFC 2833 packets setting */
   OOB_CFG_RFC = 0x04
}OOB_CFG_t;

/** structure holding mapping table */
typedef struct _ITM_MAP_TABLE_t
{
   /** pointer to board */
   BOARD_t *pBoard;
   /** device */
   IFX_int32_t nDevice;
   /** phone, data and pcm channels mapping */
   STARTUP_MAP_TABLE_t oMapping;
} ITM_MAP_TABLE_t;

#ifdef EASY336
/** table with maping of channels for test of DTMF digits */
ITM_MAP_TABLE_t map_table = { IFX_NULL, -1, {-1, -1, -1}};
#endif /* EASY336 */

/** structure holding driver name and path to file with driver version */
typedef struct _ITM_GET_DRIVER_t
{
   /** version file name with path */
   IFX_char_t* pszVersionFile;
   /** driver name */
   IFX_char_t* pszDriverName;
} ITM_GET_DRIVER_t;

/** key structure used for receiving BBD and FW files */
typedef struct _ITM_RCV_FILE_KEY_t
{
   /** Number of packet */
   IFX_uint32_t nPacketNumber;
   /** Size of file */
   IFX_uint32_t nOverallSize;
   /** Number of data bytes in value */
   IFX_uint8_t nPacketSize;
   /** Type of file e.g. BBD, FW... */
   IFX_uint8_t nFileType;
   /** Chip type e.g. ar9/gr9, danube... */
   IFX_uint8_t nChipType;
   /** File ID number. */
   IFX_uint8_t nFileUID;
   /** Check Sum for packet = (Sum(all data bytes) & 0xFF) */
   IFX_uint8_t nCheckSum;
} ITM_RCV_FILE_KEY_t;

/** structure used for receiving BBD and FW files */
typedef struct _ITM_RCV_FILE_DATA_LIST_t
{
   /** Next packet number */
   IFX_uint32_t nNextPacketNum;
   /** Size of file */
   IFX_uint32_t nOverallSize;
   /** Number of bytes received */
   IFX_uint32_t nReceivedData;
   /** Type of file e.g. BBD, FW... */
   IFX_uint8_t nFileType;
   /** Chip type e.g. ar9/gr9, danube... */
   IFX_uint8_t nChipType;
   /** File ID number. */
   IFX_uint8_t nFileUID;
   /** file binary data */
   IFX_char_t *pData;
   struct _ITM_RCV_FILE_DATA_LIST_t *pNext;
} ITM_RCV_FILE_DATA_LIST_t;

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** names of debug levels */
TD_ENUM_2_NAME_t TD_rgDbgLevelName[] =
{
   {DBG_LEVEL_LOW, "DBG_LEVEL_LOW"},
   {DBG_LEVEL_NORMAL, "DBG_LEVEL_NORMAL"},
   {DBG_LEVEL_HIGH, "DBG_LEVEL_HIGH"},
   {DBG_LEVEL_OFF, "DBG_LEVEL_OFF"},
   {TD_MAX_ENUM_ID, "DEBUG_LEVEL_t"}
};

/** VAD configuration names */
TD_ENUM_2_NAME_t TD_rgVadName[] =
{
   {IFX_TAPI_ENC_VAD_NOVAD, "IFX_TAPI_ENC_VAD_NOVAD"},
   {IFX_TAPI_ENC_VAD_ON, "IFX_TAPI_ENC_VAD_ON"},
   {IFX_TAPI_ENC_VAD_G711, "IFX_TAPI_ENC_VAD_G711"},
   {IFX_TAPI_ENC_VAD_CNG_ONLY, "IFX_TAPI_ENC_VAD_CNG_ONLY"},
   {IFX_TAPI_ENC_VAD_SC_ONLY, "IFX_TAPI_ENC_VAD_SC_ONLY"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_ENC_VAD_t"}
};

/** names of TAPIDEMO options */
TD_ENUM_2_NAME_t TD_rgOptName[] =
{
   {TD_OPT_PATH_TO_DOWNLOAD_FILES, "Path to download files"},
   {TD_OPT_USE_FILESYSTEM, "Use filesystem (FW and BBD read from download path)"},
   {TD_OPT_CH_INIT, "Default initialization mode"},
   {TD_OPT_BOARD_COMBINATION, "Board combination"},
   {TD_OPT_DEBUG_LEVEL, "Tapidemo debug level"},
   {TD_OPT_ITM_DEBUG_LEVEL, "Communication debug level"},
   {TD_OPT_CALLER_ID, "Caller ID (CID)"},
   {TD_OPT_CALLER_ID_SETTING, "Caller ID standard used"},
   {TD_OPT_VAD_SETTING, "VAD setting"},
   {TD_OPT_CONFERENCE, "Conference"},
   {TD_OPT_QOS_SUPPORT, "QoS support"},
   {TD_OPT_QOS, "QoS"},
   {TD_OPT_QOS_ON_SOCKET, "QoS on socket"},
   {TD_OPT_QOS_LOCAL_UDP, "QoS - local UDP redirection"},
   {TD_OPT_IPV6_SUPPORT, "IPv6 support"},
   {TD_OPT_IPV6, "IPv6"},
   {TD_OPT_FXO_SUPPORT, "FXO support"},
   {TD_OPT_FXO, "FXO"},
   {TD_OPT_USE_CODERS_FOR_LOCAL, "Use coders for local"},
   {TD_OPT_NO_LEC, "No LEC"},
   {TD_OPT_ENCODER, "Encoder: Coder"},
   {TD_OPT_PACKETISATION, "Encoder: Packetisation"},
   {TD_OPT_DEAMON, "Deamon (trace redirection)"},
   {TD_OPT_TRACE_REDIRECTION, "Trace redirection setting"},
   {TD_OPT_PCM_MASTER, "PCM Master"},
   {TD_OPT_PCM_SLAVE, "PCM Slave"},
   {TD_OPT_PCM_LOOP, "PCM Loop"},
   {TD_OPT_PCM_WIDEBAND, "PCM WideBand"},
   {TD_OPT_FAX_MODEM, "FAX/MODEM"},
   {TD_OPT_FAX_T38_SUPPORT, "FAX T.38 support"},
   {TD_OPT_FAX_T38, "FAX T.38"},
   {TD_OPT_MAX, "TD_OPT_MAX"},
   {TD_MAX_ENUM_ID, "TAPIDEMO_OPTIONS_t"}
};

/** translate IFX_boolean_t to disabled and enabled string */
TD_ENUM_2_NAME_t TD_rgDisableEnable[] =
{
   {IFX_FALSE, "disabled"},
   {IFX_TRUE, "enabled"},
   {TD_MAX_ENUM_ID, "IFX_boolean_t"}
};

/** names of IFX_TAPI_CID_STD_t */
TD_ENUM_2_NAME_t TD_rgCidName[] =
{
   {IFX_TAPI_CID_STD_TELCORDIA, "IFX_TAPI_CID_STD_TELCORDIA"},
   {IFX_TAPI_CID_STD_ETSI_FSK, "IFX_TAPI_CID_STD_ETSI_FSK"},
   {IFX_TAPI_CID_STD_ETSI_DTMF, "IFX_TAPI_CID_STD_ETSI_DTMF"},
   {IFX_TAPI_CID_STD_SIN, "IFX_TAPI_CID_STD_SIN"},
   {IFX_TAPI_CID_STD_NTT, "IFX_TAPI_CID_STD_NTT"},
#ifndef TAPI_VERSION4
   {IFX_TAPI_CID_STD_KPN_DTMF, "IFX_TAPI_CID_STD_KPN_DTMF"},
   {IFX_TAPI_CID_STD_KPN_DTMF_FSK, "IFX_TAPI_CID_STD_KPN_DTMF_FSK"},
#endif /* TAPI_VERSION4 */
   {TD_MAX_ENUM_ID, "IFX_TAPI_CID_STD_t"}
};

/** names of TRACE_REDIRECTION_t */
TD_ENUM_2_NAME_t TD_rgTraceRedirection[] =
{
   {TRACE_REDIRECTION_NONE, "TRACE_REDIRECTION_NONE"},
   {TRACE_REDIRECTION_FILE, "TRACE_REDIRECTION_FILE"},
   {TRACE_REDIRECTION_SYSLOG_LOCAL, "TRACE_REDIRECTION_SYSLOG_LOCAL"},
   {TRACE_REDIRECTION_SYSLOG_REMOTE, "TRACE_REDIRECTION_SYSLOG_REMOTE"},
   {TD_MAX_ENUM_ID, "TRACE_REDIRECTION_t"}
};

/** names of IFX_TAPI_CAP_TYPE_t */
TD_ENUM_2_NAME_t TD_rgCapType[] =
{
   {IFX_TAPI_CAP_TYPE_VENDOR, "IFX_TAPI_CAP_TYPE_VENDOR"},
   {IFX_TAPI_CAP_TYPE_DEVICE, "IFX_TAPI_CAP_TYPE_DEVICE"},
   {IFX_TAPI_CAP_TYPE_PORT, "IFX_TAPI_CAP_TYPE_PORT"},
   {IFX_TAPI_CAP_TYPE_CODEC, "IFX_TAPI_CAP_TYPE_CODEC"},
   {IFX_TAPI_CAP_TYPE_DSP, "IFX_TAPI_CAP_TYPE_DSP"},
   {IFX_TAPI_CAP_TYPE_PCM, "IFX_TAPI_CAP_TYPE_PCM"},
   {IFX_TAPI_CAP_TYPE_CODECS, "IFX_TAPI_CAP_TYPE_CODECS"},
   {IFX_TAPI_CAP_TYPE_PHONES, "IFX_TAPI_CAP_TYPE_PHONES"},
   {IFX_TAPI_CAP_TYPE_SIGDETECT, "IFX_TAPI_CAP_TYPE_SIGDETECT"},
   {IFX_TAPI_CAP_TYPE_T38, "IFX_TAPI_CAP_TYPE_T38"},
   {IFX_TAPI_CAP_TYPE_DEVVERS, "IFX_TAPI_CAP_TYPE_DEVVERS"},
   {IFX_TAPI_CAP_TYPE_DEVTYPE, "IFX_TAPI_CAP_TYPE_DEVTYPE"},
   {IFX_TAPI_CAP_TYPE_DECT, "IFX_TAPI_CAP_TYPE_DECT"},
#ifdef TAPI_VERSION4
   {IFX_TAPI_CAP_TYPE_SIGGEN, "IFX_TAPI_CAP_TYPE_SIGGEN"},
   {IFX_TAPI_CAP_TYPE_LEC, "IFX_TAPI_CAP_TYPE_LEC"},
   {IFX_TAPI_CAP_TYPE_PEAKD, "IFX_TAPI_CAP_TYPE_PEAKD"},
   {IFX_TAPI_CAP_TYPE_MF_R2, "IFX_TAPI_CAP_TYPE_MF_R2"},
#else
   {IFX_TAPI_CAP_TYPE_FXO, "IFX_TAPI_CAP_TYPE_FXO"},
#endif /* TAPI_VERSION4 */
   {TD_MAX_ENUM_ID, "IFX_TAPI_CAP_TYPE_t"}
};

/** table with paths to drivers version files,
   must end with IFX_NULL */
static ITM_GET_DRIVER_t DriverTable[] = {
   {"/proc/driver/tapi/version", "TAPI"},
   {"/proc/driver/vmmc/version", "VMMC"},
   {"/proc/driver/vinetic-cpe/version", "VINETIC-CPE"},
   {"/proc/driver/svip/version", "SVIP"},
   {"/proc/driver/duslic/version", "DUSLIC"},
   {"/proc/driver/drv_daa/version", "DAA"},
   {"/proc/driver/lq_mps/version", "IFX_MPS"},
   {"/proc/driver/ifx_mps/version", "IFX_MPS"},
   {"/proc/driver/spi/version", "SPI"},
   {"/proc/driver/duslicxt/version", "DXT"},
   {"/proc/driver/vxt/version", "VXT"},
   {IFX_NULL, IFX_NULL}
};

/** Names of board types */
TD_ENUM_2_NAME_t TD_rgBoardTypeName[] =
{
   {TYPE_VINETIC, "VINETIC"},
   {TYPE_DANUBE, "DANUBE"},
   {TYPE_DUSLIC_XT, "DUSLIC_XT"},
   {TYPE_DUSLIC, "DUSLIC"},
   {TYPE_VINAX, "VINAX"},
   {TYPE_AR9, "AR9/GR9"},
   {TYPE_XWAY_XRX300, "XWAY_XRX300"},
   {TYPE_SVIP, "SVIP"},
   {TYPE_XT16, "xT-16"},
   {TYPE_TERIDIAN, "TERIDIAN"},
   {TYPE_VR9, "VR9"},
   {TYPE_NONE, "NONE"},
   {TD_MAX_ENUM_ID, "BOARD_TYPE_t"}
};

/** Names of file types */
TD_ENUM_2_NAME_t TD_rgFileTypeName[] =
{
   {ITM_RCV_FILE_NONE, "NONE"},
   {ITM_RCV_FILE_BBD, "BBD"},
   {ITM_RCV_FILE_BBD_FXS, "BBD_FXS"},
   {ITM_RCV_FILE_BBD_FXO, "BBD_FXO"},
   {ITM_RCV_FILE_FW, "FW"},
   {ITM_RCV_FILE_FW_DRAM, "FW_DRAM"},
   {ITM_RCV_FILE_MAX, "MAX"},
   {TD_MAX_ENUM_ID, "ITM_RCV_FILE_TYPE_t"}
};

/** Names of LEC settings */
TD_ENUM_2_NAME_t TD_rgLEC_Type[] =
{
   {IFX_TAPI_WLEC_TYPE_OFF, "IFX_TAPI_WLEC_TYPE_OFF"},
   {IFX_TAPI_WLEC_TYPE_NE, "IFX_TAPI_WLEC_TYPE_NE"},
   {IFX_TAPI_WLEC_TYPE_NFE, "IFX_TAPI_WLEC_TYPE_NFE"},
   {IFX_TAPI_WLEC_TYPE_NE_ES, "IFX_TAPI_WLEC_TYPE_NE_ES"},
   {IFX_TAPI_WLEC_TYPE_NFE_ES, "IFX_TAPI_WLEC_TYPE_NFE_ES"},
   {IFX_TAPI_WLEC_TYPE_ES, "IFX_TAPI_WLEC_TYPE_ES"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_WLEC_TYPE_t"}
};

/** Names of NLP settings */
TD_ENUM_2_NAME_t TD_rgNLP_Type[] =
{
   {IFX_TAPI_WLEC_NLP_DEFAULT, "IFX_TAPI_WLEC_NLP_DEFAULT"},
   {IFX_TAPI_WLEC_NLP_ON, "IFX_TAPI_WLEC_NLP_ON"},
   {IFX_TAPI_WLEC_NLP_OFF, "IFX_TAPI_WLEC_NLP_OFF"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_WLEC_NLP_t"}
};

/** Holds default CPT params, should be same as in drv_tapi (drv_tapi_tone.c),
    used for testing purposes. If changes were made check if MAX_TONE = 6. */
static const CPT_ParamsTable_t CPT_Default[6] =
{
   /** name, 2*frequency, 4*cadence */
   /* DIAL_TONE */
   {{  DIAL_TONE, 350, 440, 1000, 0, 0, 0 }},
   /* SECOND_DIAL_TONE */
   {{  SECOND_DIAL_TONE, 0, 0, 0, 0, 0, 0 }},
   /* BUSY_TONE */
   {{  BUSY_TONE, 480, 620, 500, 500, 0, 0 }},
   /* RINGBACK_TONE */
   {{  RINGBACK_TONE, 440, 480, 1000, 4000, 0, 0 }},
   /* CONGESTION_TONE */
   {{  CONGESTION_TONE, 0, 0, 0, 0, 0, 0 }},
   /* HOWLER_TONE */
   {{  HOWLER_TONE,  0, 0, 0, 0, 0, 0 }}
};

/* Array storing the sizes of keys and values for corresponding commands.
    Used for checking the consistency of command frames */
static const KEY_LEN_VALUE_LEN_t pDefaultLen[COM_ID_MAX] = {
   {KEY_SIZE_COM_UNKNOWN_COMMAND, VALUE_SIZE_COM_UNKNOWN_COMMAND},
   {KEY_SIZE_SET_TEST_TYPE, VALUE_SIZE_SET_TEST_TYPE},
   {KEY_SIZE_SET_TRACE_LEVEL, VALUE_SIZE_SET_TRACE_LEVEL},
   {KEY_SIZE_SET_PHONES, VALUE_SIZE_SET_PHONES},
   {KEY_SIZE_GET_PHONES, VALUE_SIZE_GET_PHONES},
   {KEY_SIZE_SET_SUT_COUNTRY_SETTINGS, VALUE_SIZE_SET_SUT_COUNTRY_SETTINGS},
   {KEY_SIZE_GET_SUT_COUNTRY_SETTINGS, VALUE_SIZE_GET_SUT_COUNTRY_SETTINGS},
   {KEY_SIZE_BROADCAST_MSG, VALUE_SIZE_BROADCAST_MSG},
   {KEY_SIZE_COM_RESET, VALUE_SIZE_COM_RESET},
   {KEY_SIZE_CONFIG_CHANNELS, VALUE_SIZE_CONFIG_CHANNELS},
   {KEY_SIZE_SET_CH_INIT_VOICE, VALUE_SIZE_SET_CH_INIT_VOICE},
   {KEY_SIZE_SET_LINE_FEED_SET_STANDBY,
      VALUE_SIZE_SET_LINE_FEED_SET_STANDBY},
   {KEY_SIZE_SET_LINE_FEED_SET_ACTIVE,
      VALUE_SIZE_SET_LINE_FEED_SET_ACTIVE},
   {KEY_SIZE_COM_CHANGE_CODEC, VALUE_SIZE_COM_CHANGE_CODEC},
   {KEY_SIZE_GENERATE_TONES, VALUE_SIZE_GENERATE_TONES},
   {KEY_SIZE_SET_INTERNAL_SUT_CHANNEL_CONNECTION,
      VALUE_SIZE_SET_INTERNAL_SUT_CHANNEL_CONNECTION},
   {KEY_SIZE_SET_MODULAR_TEST_TYPE, VALUE_SIZE_SET_MODULAR_TEST_TYPE},
   {KEY_SIZE_SET_RUN_PROGRAM, VALUE_SIZE_SET_RUN_PROGRAM},
   {KEY_SIZE_SET_ANALOG_TO_IP_CALL_CTRL,
      VALUE_SIZE_SET_ANALOG_TO_IP_CALL_CTRL},
   {KEY_SIZE_SET_SUT_CPT_COUNTRY_SETTINGS,
      VALUE_SIZE_SET_SUT_CPT_COUNTRY_SETTINGS},
   {KEY_SIZE_REPEAT_SYSINIT_TEST, VALUE_SIZE_REPEAT_SYSINIT_TEST},
   {KEY_SIZE_TAPI_RESTART, VALUE_SIZE_TAPI_RESTART},
   {KEY_SIZE_GET_SYSINIT_TYPE, VALUE_SIZE_GET_SYSINIT_TYPE},
   {KEY_SIZE_ADD_ARP_ENTRY, VALUE_SIZE_ADD_ARP_ENTRY_TYPE},
   {KEY_SIZE_PRINT_TXT, VALUE_SIZE_PRINT_TXT},
   {KEY_SIZE_SET_CID, VALUE_SIZE_SET_CID},
   {KEY_SIZE_SET_CID_DATA, VALUE_SIZE_SET_CID_DATA},
   {KEY_SIZE_GET_INFO, VALUE_SIZE_GET_INFO},
   {KEY_SIZE_SET_LEC, VALUE_SIZE_SET_LEC},
   {KEY_SIZE_CHECK_LEC, VALUE_SIZE_CHECK_LEC},
   {KEY_SIZE_PING, VALUE_SIZE_PING},
   {KEY_SIZE_SET_PORT_NUMBER, VALUE_SIZE_SET_PORT_NUMBER},
   {KEY_SIZE_RCV_FILE, VALUE_SIZE_RCV_FILE},
   {KEY_SIZE_CTRL_PROG_INIT, VALUE_SIZE_CTRL_PROG_INIT},
   {KEY_SIZE_OOB_CFG, VALUE_SIZE_OOB_CFG},
};

/*                                                                            */
/* START    CALLER_ID test defines and macros                           START */
/*                                                                            */
/** Type of CID transparent message is set on 1st byte */
#define TD_CID_TRANSP_OFFSET_TYPE       0
/** Length of data is set on 2nd byte */
#define TD_CID_TRANSP_OFFSET_LENGTH     1
/** Date is started from 3rd byte,
    type must be set to TD_CID_TRANSP_TYPE_DATE_AND_NUMBER */
#define TD_CID_TRANSP_OFFSET_DATE       2
/** If number is present in CID message then it starts from 11th byte,
    type must be set to TD_CID_TRANSP_TYPE_DATE_AND_NUMBER */
#define TD_CID_TRANSP_OFFSET_NUMBER     10

/** numbers of bytes used for date -  month(2 bytes) + day(2 bytes) +
    hour (2 bytes) + minute (2 bytes) - ascii is used to write date from range
    '0' - '9' ('\0x30'- '\0x39') */
#define TD_CID_TRANSP_DATE_SIZE         8

/** Number of non data bytes in transparent CID message.
    In transparent message fisrt byte is type, second is length of data, from
    third byte starts data, last byte is CRC byte. CRC, type and length bytes
    are not counted as data length. */
#define TD_CID_TRANSP_NON_DATA_BYTES_NUMBER      3

/** Type of CID message that is received on FXO port from testing hardware. */
#define TD_CID_TRANSP_TYPE_DATE_AND_NUMBER    0x04

/** Used for OOB_CFG ITM message to mark all elements */
#define TD_COM_OOB_CFG_ALL 0xFFFFFFFF

/** Used for OOB_CFG ITM message to mark no action */
#define TD_COM_OOB_CFG_DO_NOT_CHANGE 0xFFFFFFFE

/** Used for OOB_CFG_ITM message to mark default action */
#define TD_COM_OOB_CFG_DEFAULT 0xFFFFFFFF

/** Check if date parameter is in range
    \nParam  - parameter to check (unsigned)
    \nCnt    - number structure from which parameter is used
    \m_ConnID  - sequential conn ID */
#define TD_COM_CID_DATE_CHECK(nParam, nCnt, m_ConnID) \
   do \
   { \
      if (100 <= nParam) \
      { \
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_ConnID, \
               ("Err, ITM: Invalid value "#nParam" = %u (%d). " \
                "(File: %s, line: %d)\n", \
                nParam, nCnt, __FILE__, __LINE__)); \
         break; \
      } \
   } while (0)

/** Validate CID data, check if length of array is valid (for number or name)
    \m_nRet  - return value set to IFX_SUCCESS if data is valid
    \m_nLen  - length of array
    \m_pMsgStr - pointer to data string
    \m_nCnt  - number structure from which parameter is used
    \m_ConnID  - sequential conn ID */
#define TD_COM_CID_STRING_LEN_CHECK(m_nRet, m_nLen, m_pMsgStr, m_nCnt, m_ConnID) \
   do \
   { \
      m_nRet = IFX_ERROR; \
      if (IFX_TAPI_CID_MSG_LEN_MAX <= m_nLen || 0 >= m_nLen) \
      { \
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_ConnID, \
               ("Err, ITM: Invalid value "#m_nLen" = %d (%d). " \
                "(File: %s, line: %d)\n", \
                m_nLen, m_nCnt, __FILE__, __LINE__)); \
         break; \
      } \
      if (m_nLen != strlen((IFX_char_t*)m_pMsgStr)) \
      { \
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_ConnID, \
               ("Err, ITM: Not maching "#m_nLen" = %d and " \
                "strlen("#m_pMsgStr") = %d for %d. " \
                "(File: %s, line: %d)\n", \
                m_nLen, strlen((IFX_char_t*)m_pMsgStr), m_nCnt, \
                __FILE__, __LINE__)); \
         break; \
      } \
      m_nRet = IFX_SUCCESS; \
   } while (0)
/*                                                                            */
/* END      CALLER_ID test defines and macros                             END */
/*                                                                            */

/** Format string for receive data ITM message */
#define ITM_RCV_FILE_RESPONSE_STRING \
   "SUTS_RESPONSE:{{COMMAND_NAME BIN_CHUNK_RCV} {COMMAND_RESPONSE %s} }%s"

/** List of used files e.g.: BBD, FW... */
ITM_RCV_FILE_DATA_LIST_t *pFileList = IFX_NULL;

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */

IFX_return_t COM_ITM_SetAction(CTRL_STATUS_t* pCtrl,
                               IFX_uint32_t nCalledPhoneNumber,
                               IFX_uint8_t* pTestMac, IFX_uint32_t nPort,
                               IFX_int32_t nAction);
IFX_return_t COM_BBD_ChangeCountrySettings(CTRL_STATUS_t* pCtrl,
                                           ITM_MSG_HEADER_t* pITM_MsgHead);
IFX_return_t COM_MessageWait(CTRL_STATUS_t* pCtrl);
IFX_void_t COM_TestEnd(CTRL_STATUS_t* pCtrl);
IFX_return_t COM_SendSutInfo(IFX_void_t);
IFX_return_t COM_SendAppVersion(CTRL_STATUS_t* pCtrl);
IFX_return_t COM_SendSutSW_VersionAndCappabilities(CTRL_STATUS_t* pCtrl);
IFX_return_t COM_SendSutSettings(IFX_void_t);
IFX_return_t COM_GetMsgData(ITM_MSG_HEADER_t* pITM_MsgHead, IFX_uint32_t* pUnsInt,
                            IFX_uint32_t nRep, ITM_MSG_DATA_TYPE_t nType);
IFX_char_t* COM_GetMsgDataPointer(ITM_MSG_HEADER_t* pITM_MsgHead,
                                  IFX_uint32_t nRep,
                                  ITM_MSG_DATA_TYPE_t nType);
IFX_return_t COM_RunCommand(ITM_MSG_HEADER_t* pITM_MsgHead, IFX_char_t* buf);
IFX_return_t COM_SendSutArgv(IFX_void_t);
IFX_return_t COM_SendSutSettingMessage(TAPIDEMO_OPTIONS_t      nTD_option,
                                       COM_OPTION_VALUE_TYPE_t nNumericValue,
                                       const IFX_char_t*       pszDesc,
                                       IFX_int32_t             nDfltValue,
                                       const IFX_char_t*       pszDfltValueTxt);
IFX_return_t COM_SendSysInitProceed(IFX_int32_t nBoardNo);
IFX_return_t COM_ExtractDevAndChannelFromMsg(ITM_MSG_HEADER_t* pITM_MsgHead);
IFX_return_t COM_SendSysInitHalt(IFX_int32_t nBoardNo);

IFX_int32_t COM_GetCapabilityValue(BOARD_t* pBoard,
                                   IFX_int32_t nDev,
                                   IFX_TAPI_CAP_TYPE_t nCapType);
IFX_return_t COM_OOB_Configure(CTRL_STATUS_t* pCtrl,
                               ITM_MSG_HEADER_t* pITM_MsgHead);
IFX_return_t COM_OOB_Configure_Execute(BOARD_t* pBoard,
                               IFX_uint32_t nBoardNo,
                               IFX_uint32_t nDevTmp,
                               IFX_uint32_t nDataCh,
                               IFX_TAPI_PKT_EV_OOB_DTMF_t* eventParam,
                               IFX_TAPI_PKT_RTP_CFG_t* rfcParam);
IFX_int32_t COM_GetChDataFromITM_Msg(ITM_MSG_HEADER_t* pITM_MsgHead,
                                     IFX_int32_t nKey,
                                     IFX_uint32_t* pBoardNo,
                                     IFX_uint32_t* pDevNo,
                                     IFX_uint32_t* pPhoneChNo,
                                     IFX_uint32_t* pValue);
IFX_return_t COM_PrintTxt(CTRL_STATUS_t* pCtrl,
                          ITM_MSG_HEADER_t* pITM_MsgHead);
IFX_return_t COM_InfoGet(CTRL_STATUS_t* pCtrl);
IFX_return_t COM_TapiRestart(CTRL_STATUS_t* pCtrl, BOARD_t* pBoard);
#ifdef TAPI_VERSION4
IFX_return_t COM_DevStop(BOARD_t* pBoard);
#endif /* TAPI_VERSION4 */
IFX_return_t COM_ExecuteShellCommand(IFX_char_t* pCmd, IFX_char_t* pCmdOutput);
IFX_return_t COM_AddArpEntry(CTRL_STATUS_t* pCtrl, ITM_MSG_HEADER_t* pITM_MsgHead,
                            IFX_char_t* buf);
IFX_void_t COM_ReportErrorDwlPath(IFX_void_t);

/* ============================= */
/* Local function definition     */
/* ============================= */

/* mapping channels doesn't work when resource manager is used. */
#ifdef EASY336
/**
   Map phone channel and data channel using custom map table.

   \param p_custom_map_table - pointer table of mapping structure
   \param fDoMapping - if IFX_TRUE - map channels,
                       if IFX_FALSE - only unmap channels

   \return IFX_SUCCESS everything ok, otherwise IFX_ERROR

*/
static IFX_return_t COM_SvipCustomMapping(ITM_MAP_TABLE_t custom_map_table,
                                          IFX_boolean_t fDoMapping)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
   IFX_TAPI_MAP_DATA_t map_data;
#endif /* TAPI_VERSION4 */

   /* check input parameters */
   TD_PTR_CHECK(custom_map_table.pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((custom_map_table.oMapping.nPhoneCh
                        >= custom_map_table.pBoard->nMaxAnalogCh),
                      custom_map_table.oMapping.nPhoneCh, IFX_ERROR);
   TD_PARAMETER_CHECK((custom_map_table.oMapping.nPhoneCh < 0),
                      custom_map_table.oMapping.nPhoneCh, IFX_ERROR);
   TD_PARAMETER_CHECK((custom_map_table.oMapping.nDataCh
                        >= custom_map_table.pBoard->nMaxCoderCh),
                      custom_map_table.oMapping.nDataCh, IFX_ERROR);
   TD_PARAMETER_CHECK((custom_map_table.oMapping.nDataCh < 0),
                      custom_map_table.oMapping.nDataCh, IFX_ERROR);

   if ((NO_MAPPING != map_table.oMapping.nPhoneCh)
         && (NO_MAPPING != map_table.oMapping.nDataCh))
   {
#ifdef TAPI_VERSION4
      memset(&map_data, 0, sizeof (IFX_TAPI_MAP_DATA_t));
      /* set device */
      map_data.dev = (IFX_uint16_t) map_table.nDevice;
      /* set data channel */
      map_data.ch = map_table.oMapping.nDataCh;
      /* Do not modify the status of the encoder or the decoder */
      map_data.nPlayStart = IFX_TAPI_MAP_DATA_UNCHANGED;
      map_data.nRecStart = IFX_TAPI_MAP_DATA_UNCHANGED;

      /* Remove the Coder from analog line module */
      map_data.nChType = IFX_TAPI_MAP_TYPE_PHONE;
      map_data.nDstCh = map_table.oMapping.nPhoneCh;
      /* Remove connection between phone channel and data channel */
      ret = TD_IOCTL(custom_map_table.pBoard->nDevCtrl_FD,
               IFX_TAPI_MAP_DATA_REMOVE, (IFX_int32_t) &map_data,
               map_data.dev, TD_CONN_ID_ITM);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Error unmapping phone ch %d and data ch %d,"
                " board %d dev %d."
                " (File: %s, line: %d)\n",
                map_table.oMapping.nPhoneCh,
                map_table.oMapping.nDataCh,
                map_table.pBoard->nBoard_IDX,
                map_table.nDevice,
                __FILE__, __LINE__));
         return ret;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("ITM: unmapping data ch %d and phone ch %d,"
                " board %d dev %d\n",
                map_data.ch, map_data.nDstCh,
                custom_map_table.pBoard->nBoard_IDX, map_data.dev));
      }
#else
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Function only implemented for TAPI_VERSION4 ."
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
#endif /* TAPI_VERSION4 */
   }

   if (IFX_TRUE == fDoMapping)
   {
#ifdef TAPI_VERSION4
      memset(&map_data, 0, sizeof (IFX_TAPI_MAP_DATA_t));
      /* set device */
      map_data.dev = (IFX_uint16_t) custom_map_table.nDevice;
      /* set data channel */
      map_data.ch = (IFX_uint16_t) custom_map_table.oMapping.nDataCh;
      /* Do not modify the status of the encoder or the decoder */
      map_data.nPlayStart = IFX_TAPI_MAP_DATA_UNCHANGED;
      map_data.nRecStart = IFX_TAPI_MAP_DATA_UNCHANGED;

      /* Remove the Coder from analog line module */
      map_data.nChType = IFX_TAPI_MAP_TYPE_PHONE;
      map_data.nDstCh = (IFX_uint8_t) custom_map_table.oMapping.nPhoneCh;
      /* Map phone channel and data channel */
      ret = TD_IOCTL(custom_map_table.pBoard->nDevCtrl_FD,
               IFX_TAPI_MAP_DATA_ADD, (IFX_int32_t) &map_data,
               map_data.dev, TD_CONN_ID_ITM);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: mapping phone ch %d to data ch %d,"
                " board %d dev %d failed."
                "(File: %s, line: %d)\n",
                custom_map_table.oMapping.nPhoneCh,
                custom_map_table.oMapping.nDataCh,
                custom_map_table.pBoard->nBoard_IDX,
                custom_map_table.nDevice,
                __FILE__, __LINE__));
         return ret;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("ITM: mapping data ch %d and phone ch %d,"
                " board %d dev %d\n",
                map_data.ch, map_data.nDstCh,
                custom_map_table.pBoard->nBoard_IDX, map_data.dev));
      }
#else
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
             ("Err, ITM: Function only implemented for TAPI_VERSION4 ."
              "(File: %s, line: %d)\n",
              __FILE__, __LINE__));
#endif /* TAPI_VERSION4 */
      map_table = custom_map_table;
   }
   else
   {
      /* reset mapping table */
      map_table.oMapping.nDataCh = NO_MAPPING;
      map_table.oMapping.nPhoneCh = NO_MAPPING;
      map_table.oMapping.nPCM_Ch = NO_MAPPING;
   }

   return ret;
} /* COM_SvipCustomMapping() */

/**
   Turn on tone detector on mapped phone channel and play dialtone
   or turn off tone detector on mapped phone channel and stop playing tone.

   \param fEnable - if IFX_TRUE - enable DTMF detection
                    if IFX_FALSE - disable DTMF detection
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS everything ok, otherwise IFX_ERROR

*/
IFX_return_t COM_SvipToneDetection(IFX_boolean_t fEnable,
                                   IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
   IFX_TAPI_EVENT_t tapiEvent = {0};
   IFX_TAPI_TONE_PLAY_t tone = {0};

   /* set event structure */
   tapiEvent.dev = (IFX_uint16_t) map_table.nDevice;
   tapiEvent.ch = map_table.oMapping.nPhoneCh;
   tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
   /* DTMF tone detection done from the analog line side. */
   tapiEvent.id = IFX_TAPI_EVENT_DTMF_DIGIT;

   tapiEvent.data.dtmf.external = 1;
   tapiEvent.data.dtmf.internal = 0;
   /* set tone parameters */
   tone.dev = (IFX_uint16_t) map_table.nDevice;
   tone.ch = map_table.oMapping.nPhoneCh;
   tone.external = 1;
   tone.internal = 0;
   tone.module = IFX_TAPI_MODULE_TYPE_ALM;
   /* Dialtone */
   tone.index = DIALTONE_IDX;

   if (IFX_TRUE == fEnable)
   {
      /* play Dialtone - it is needed modular test of DTMF digits */
      ret = TD_IOCTL(map_table.pBoard->nDevCtrl_FD, IFX_TAPI_TONE_PLAY,
                     (IFX_int32_t) &tone, tone.dev, nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, ITM: Phone Ch %d: Failed to play dialtone, "
                "board %d, dev %d."
                "(File: %s, line: %d)\n",
                map_table.oMapping.nPhoneCh,
                map_table.pBoard->nBoard_IDX,
                map_table.nDevice,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* enable tone detection */
      ret = TD_IOCTL(map_table.pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_ENABLE,
               (IFX_int32_t) &tapiEvent, tapiEvent.dev, nSeqConnId);
   }
   else
   {
      /* stop playing tone on channel */
      TD_IOCTL(map_table.pBoard->nDevCtrl_FD, IFX_TAPI_TONE_STOP,
         (IFX_int32_t) &tone, tone.dev, nSeqConnId);
      /* disable tone detection */
      ret = TD_IOCTL(map_table.pBoard->nDevCtrl_FD, IFX_TAPI_EVENT_DISABLE,
               (IFX_int32_t) &tapiEvent, tapiEvent.dev, nSeqConnId);
   }

   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, ITM: Phone Ch %d: Failed to %s tone detection, "
             "board %d, dev %d. "
             "(File: %s, line: %d)\n",
             map_table.oMapping.nPhoneCh,
             IFX_TRUE == fEnable ? "enable" : "disable",
             map_table.pBoard->nBoard_IDX,
             map_table.nDevice,
             __FILE__, __LINE__));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("ITM: Phone Ch %d: %sd tone detection "
             "(board %d, dev %d).\n",
             tapiEvent.ch,
             IFX_TRUE == fEnable ? "enable" : "disable",
             map_table.pBoard->nBoard_IDX,
             map_table.nDevice));
   }

#endif /* TAPI_VERSION4 */
   return ret;
} /* COM_SvipToneDetection() */
#endif

/**
   Function to set ITM_MSG_HEADER_t structure and check message parmeters
   (length, End of Message and default values of key and value length).

   \param pITM_MsgHead - pointer to ITM_MSG_HEADER_t structure
                          that should be filed with data.
   \param pMsg          - pointer to ITM message.
   \param nSize         - length of received message

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t COM_SetITM_MsgHeader(ITM_MSG_HEADER_t* pITM_MsgHead,
                                  IFX_char_t* pMsg, IFX_int32_t nSize)
{
   IFX_char_t* pIdName;
   IFX_int16_t nRepetitionLPart, nRepetitionHPart;
   /* check input parameters */
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);
   TD_PTR_CHECK(pMsg, IFX_ERROR);

   /* Check whether size of received message fits in size declared in
          its content
   +====+============+=========+=========+=========+=========+=====+
   | id | repetition | key_len | val_len | Nth_key | Nth_val | EOM |
   +====+============+=========+=========+=========+=========+=====+
   | 1B |     2B     |   1B    |   1B    | key_len | val_len | 1B  |
   +----+------------+---------+---------+---------+---------+-----+
      */

   memset(pITM_MsgHead, 0, sizeof(ITM_MSG_HEADER_t));
   /* Write data to structure */
   /* get ID number and name pointer */
   pITM_MsgHead->nId = (IFX_uint8_t) pMsg[HDP_ID];
   pIdName = Common_Enum2Name(pITM_MsgHead->nId, TD_rgMsgIdName);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
         ("Command ID Id 0x%X - %s\n", pITM_MsgHead->nId, pIdName));
   /* check if valid ID */
   if ((COM_ID_UNKNOWN >= pITM_MsgHead->nId) ||
       (COM_ID_MAX <= pITM_MsgHead->nId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, Invalid command ID (0x%02X). (File: %s, line: %d)\n",
             pITM_MsgHead->nId, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* number of data fields - repetition */
   nRepetitionHPart = (pMsg[HDP_REPETITION_H_BYTE] << ONE_BYTE);
   nRepetitionLPart = (BYTE_TO_INT16_MASK & pMsg[HDP_REPETITION_L_BYTE]);
   pITM_MsgHead->nRepetition = nRepetitionHPart + nRepetitionLPart;

   /* key field length */
   pITM_MsgHead->nKeyLen = (IFX_uint8_t)pMsg[HDP_KEY_LEN];
   /* value field length */
   pITM_MsgHead->nValueLen = (IFX_uint8_t)pMsg[HDP_VALUE_LEN];

   /* frame with command to run program can have diffrent key length
      and diffrent value length */
   if ((pDefaultLen[pITM_MsgHead->nId].nKeyLen != NO_DEFAULT_KEY_VALUE_LENGTH) &&
       (pDefaultLen[pITM_MsgHead->nId].nValueLen != NO_DEFAULT_KEY_VALUE_LENGTH))
   {
      /* if the key length differs from expected value indicate error */
      if (pDefaultLen[pITM_MsgHead->nId].nKeyLen != pITM_MsgHead->nKeyLen)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Invalid key length (0x%02X) for command ID %s (0x%02X)."
                "(File: %s, line: %d)\n", pITM_MsgHead->nKeyLen, pIdName,
                pITM_MsgHead->nId, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* if the value length differs from expected value indicate error*/
      if (pDefaultLen[pITM_MsgHead->nId].nValueLen != pITM_MsgHead->nValueLen)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Invalid value length (0x%02X) for command ID %s (0x%02X)."
                "(File: %s, line: %d)\n", pITM_MsgHead->nValueLen,
                pIdName, pITM_MsgHead->nId, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   /* length of pair key and value fields */
   pITM_MsgHead->nDataLen = pITM_MsgHead->nKeyLen + pITM_MsgHead->nValueLen;

   /* get message length from ITM_MSG_HEADER_t data */
   pITM_MsgHead->nMsgLen = ITM_MSG_HEADER_LEN +
      (pITM_MsgHead->nDataLen * pITM_MsgHead->nRepetition) + ITM_EOM_LEN;

   /* set pointer to data */
   pITM_MsgHead->pData = &pMsg[HDP_END];
   /* check message length */
   if (nSize != pITM_MsgHead->nMsgLen)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Received wrong data length (%d, expected %d)."
             " (File: %s, line: %d)\n",
             pITM_MsgHead->nMsgLen, nSize, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* check if proper end of message */
   if (ITM_EOM == (IFX_uint8_t) pMsg[pITM_MsgHead->nMsgLen - 1])
   {
#if 0
      /* print header main info */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("\tmsg id 0x%02X: msg len %d, key len %d, value len %d,"
             " rep %d, data %d\n",
             pITM_MsgHead->nId,
             pITM_MsgHead->nMsgLen,
             pITM_MsgHead->nKeyLen,
             pITM_MsgHead->nValueLen,
             pITM_MsgHead->nRepetition,
             pITM_MsgHead->nDataLen));
#endif
      return IFX_SUCCESS;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Invalid End of Message. "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));

   return IFX_ERROR;
} /* COM_SetITM_MsgHeader() */

/**
   Function to send message to chosen phone. Used to start analog to ip calls.

   \param pCtrl  - pointer to control structure.
   \param nCalledPhoneNumber  - number of phone to which we are sending message.
   \param pTestMac  - pointer to array with MAC address.
   \param nPort  - number of used communication port.
   \param nAction - which action to send to other phone.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t COM_ITM_SetAction(CTRL_STATUS_t* pCtrl,
                               IFX_uint32_t nCalledPhoneNumber,
                               IFX_uint8_t* pTestMac, IFX_uint32_t nPort,
                               IFX_int32_t nAction)
{
   TD_OS_sockAddr_t to_addr;
   TD_COMM_MSG_t oMsg;
   /* This is a random value, it shouldn't
      matter in analog to IP modular test. */
#ifdef TAPI_VERSION4
   IFX_int32_t nTestPhoneNumber = 777;
#else /* TAPI_VERSION4 */
   IFX_int32_t nTestPhoneNumber = 7;
#endif /* TAPI_VERSION4 */

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pTestMac, IFX_ERROR);
   TD_PARAMETER_CHECK((MAX_PORT_NUM < nPort), nPort, IFX_ERROR);

   memset(&oMsg, 0, sizeof(TD_COMM_MSG_t));

   /* set voip call message */
   oMsg.fPCM = CALL_FLAG_VOIP;
   memcpy(oMsg.MAC, pTestMac, ETH_ALEN);
   oMsg.nAction = nAction;
   /* Caller board IDX */
   oMsg.nBoard_IDX = 0;
   /* set phone numbers */
   oMsg.nReceiverPhoneNum = nCalledPhoneNumber;
   oMsg.nSenderPhoneNum = nTestPhoneNumber;
   /* Set receiving port */
   oMsg.nSenderPort = nPort;
   /* oMsg.nFeatureID = 0; */
   /* Mark start and end of message with flags */
   oMsg.nMarkStart = COMM_MSG_START_FLAG;
   oMsg.nMarkEnd = COMM_MSG_END_FLAG;
   /* set size and version */
   oMsg.nMsgLength = sizeof(oMsg);
   /* set message version */
   oMsg.nMsgVersion = TD_COMM_MSG_CURRENT_VERSION;

   /* no CID sending */
   oMsg.oData1.oCID.nType = TD_COMM_MSG_DATA_1_NONE;

   if (IE_RINGING == nAction)
   {
      oMsg.oData2.oSeqConnId.nType = TD_COMM_MSG_DATA_1_CONN_ID;
      oMsg.oData2.oSeqConnId.nSeqConnID =
         ABSTRACT_SeqConnID_Get(IFX_NULL, IFX_NULL, pCtrl);
   }

   /* Set action to phone, for example tell him that we are calling,
      using board IP address */
   memset(&to_addr, 0, sizeof(TD_OS_sockAddr_t));
   memcpy(&to_addr, &pCtrl->pProgramArg->oMy_IP_Addr, sizeof(to_addr));
   TD_SOCK_PortSet(&to_addr, ADMIN_UDP_PORT);

   /* send message */
   return TD_OS_SocketSendTo(pCtrl->nAdminSocket, (IFX_char_t *) &oMsg,
                             sizeof(oMsg), &to_addr);
} /* COM_ITM_SetAction() */

/**
   Function to change ip address of received message.
   For analog to ip test - fakes peer address.

   \param addr_from  - pointer to address structure
   \param nAction - which action should be taken,
                   if ITM_TEST_START or COM_TEST_STOP save new address,
                   if COM_TEST_IN_PROGRESS change address.

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t COM_SetGetPort(TD_OS_sockAddr_t* addr_from,
                            IFX_int32_t nAction)
{
   static TD_OS_sockAddr_t oTestAdd;
   static IFX_int32_t nCounter = 0;

   /* check input parameters */
   TD_PTR_CHECK(addr_from, IFX_ERROR);

   switch (nAction)
   {
      case COM_TEST_START:
      case COM_TEST_STOP:
         /* Save tested address */
         memcpy(&oTestAdd, addr_from, sizeof(oTestAdd));
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("ITM: Analog2IP %d, start %d, stop %d.\n",
                nAction, COM_TEST_START, COM_TEST_STOP));
         /* one more message will be received */
         nCounter++;
         break;
      case COM_TEST_IN_PROGRESS:
         if (0 < nCounter)
         {
            /* set tested address */
            memcpy(addr_from, &oTestAdd, sizeof(oTestAdd));

            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                  ("ITM: Analog2IP set addr [%s]:%d - test in progress.\n",
                   TD_GetStringIP(addr_from), TD_SOCK_PortGet(addr_from)));
            /* message was received */
            nCounter--;
         }
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid action(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
         break;
   }
   return IFX_SUCCESS;
} /* COM_SetGetPort */

/**
   Function to initialize board with new BBD file,
   used when changing BBD file during test.

   \param pBoard  - pointer to board structure.
   \param pCpuDevice - board main device structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remarks
      Init.nCountry is currently not used
      but if that changes it should be added to function.
      Before function can be used
      firmware pointers(pBoard->pDevDependStuff) should be set.
*/
IFX_return_t COM_BBD_InitBoardChannels(BOARD_t* pBoard,
                                       TAPIDEMO_DEVICE_CPU_t* pCpuDevice)
{
   IFX_int32_t i;
   IFX_TAPI_CH_INIT_t Init;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
         ("BBD_InitBoardChannels\n"));

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice, IFX_ERROR);
   TD_PTR_CHECK(pCpuDevice->pDevDependStuff, IFX_ERROR);

   Init.nMode = IFX_TAPI_INIT_MODE_DEFAULT;
   Init.pProc = (IFX_void_t*) pCpuDevice->pDevDependStuff;

   /* for all data channels */
   for (i = 0; i < pBoard->nMaxCoderCh; i++)
   {

      /* Initialize all system channels */
      if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i], IFX_TAPI_CH_INIT,
                            (IFX_int32_t) &Init, TD_DEV_NOT_SET, TD_CONN_ID_ITM))
      {
         return IFX_ERROR;
      }

   }/* for (i = 0; i < pBoard->nMaxCoderCh; i++) */
   return IFX_SUCCESS;
} /* BBD_InitBoardChannels() */

/**
   Function to change BBD file acording to message data,
   used in change country settings test.

   \param pCtrl  - pointer to control structure
   \param pITM_MsgHead - structure holds message data

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t COM_BBD_ChangeCountrySettings(CTRL_STATUS_t* pCtrl,
                                           ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_int32_t i, len;
   IFX_int32_t ret = IFX_ERROR;
   BOARD_t* pBoard;
   KEY_UINT_VALUE_POINTER_TO_STRING_t oData;
   IFX_char_t* pTemp;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t  *pCpuDevice = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
         ("BBD_ChangeCountrySettings\n"));

   for (i=0; i<pITM_MsgHead->nRepetition; i++)
   {
      /* get board type from key field */
      if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &oData.nKey, i,
                                      ITM_MSG_DATA_TYPE_KEY))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get key value."
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get name of BBD file */
      oData.pValue = COM_GetMsgDataPointer(pITM_MsgHead, i,
                                           ITM_MSG_DATA_TYPE_VALUE);
      if (IFX_NULL == oData.pValue)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value pointer."
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* set pointer to i-th board structure */
      pBoard = &pCtrl->rgoBoards[i];
      /* check if board type same as in ITM message */
      if (pBoard->nType != oData.nKey)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Wrong board type in TCL message. "
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         break;
      }

      /* check Value Length  */
      if (MAX_FILE_NAME_LEN < pITM_MsgHead->nValueLen)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Value Length %d too big, more than %d. "
                "(File: %s, line: %d)\n",
                pITM_MsgHead->nValueLen, MAX_FILE_NAME_LEN,
                __FILE__, __LINE__));
         break;
      }

      /* get length of file name */
      len = strlen(oData.pValue);
      if ((0>=len) && (MAX_FILE_NAME_LEN < len))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Wrong file name in TCL message. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         break;
      }

      if ((pDevice = Common_GetDevice_CPU(pBoard->pDevice)) == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, CPU device not found. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         break;
      }
      pCpuDevice = &pDevice->uDevice.stCpu;
      /* remember pointer to previous BBD file name */
      pTemp = pCpuDevice->pszBBD_CRAM_File[0];
      pCpuDevice->pszBBD_CRAM_File[0] = (IFX_char_t*) TD_OS_MemAlloc(len+1);
      strncpy(pCpuDevice->pszBBD_CRAM_File[0], oData.pValue, len+1);

      ret = pCpuDevice->FillinFwPtr(pBoard->nType, pCpuDevice->pDevDependStuff,
                                    pCpuDevice,
                                    pCtrl->pProgramArg->sPathToDwnldFiles);

      if (IFX_SUCCESS == ret)
      {
         ret = COM_BBD_InitBoardChannels(pBoard, pCpuDevice);
      }
      pCpuDevice->ReleaseFwPtr(pCpuDevice->pDevDependStuff);

      if (IFX_SUCCESS != ret)
      {
         /* take back changes and free memory */
         TD_OS_MemFree(pCpuDevice->pszBBD_CRAM_File[0]);
         pCpuDevice->pszBBD_CRAM_File[0] = pTemp;
      }
      else if (IFX_TRUE == pBoard->fBBD_Changed)
      {
         /* free memory if previously allocated */
         TD_OS_MemFree(pTemp);
      }
      else
      {
         /* set BBD changed flag, needed to free allocated memory */
         pBoard->fBBD_Changed = IFX_TRUE;
      }

   } /* for (i=0; i<pITM_MsgHead->nRepetition; i++) */

   /* read data from bbd file with country settings */

   return IFX_SUCCESS;
} /* BBD_ChangeCountrySettings */


/**
   Function to set CPT parameters according to CPT_ParamsTable_t.

   \param pCtrl  - pointer to control structure
   \param pCPT_New - pointer to table with new CPT params,
                     if pCPT_New == IFX_NULL then set default values

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t COM_CPT_Set(CTRL_STATUS_t* pCtrl, CPT_ParamsTable_t* pCPT_New)
{
   CPT_PARAM_t nParamCount;
   CPT_NAME_t nToneCount;
   IFX_return_t ret = IFX_ERROR;
   IFX_int32_t j;
   IFX_TAPI_TONE_t tone[MAX_TONE];
   CPT_ParamsTable_t* pCPT_Params;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   if (IFX_NULL == pCPT_New)
   {
      /* set default values */
      pCPT_Params = (CPT_ParamsTable_t*) &CPT_Default[0];
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
            ("CPT_Set(): Default CPT settings\n"));
   }
   else
   {
      /* set custom values */
      pCPT_Params = pCPT_New;
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM, ("CPT_Set()\n"));
   }

   memset(tone, 0, MAX_TONE * sizeof(IFX_TAPI_TONE_t));

   /* for all Call Progress Tones */
   for (nToneCount=0; nToneCount<MAX_TONE; nToneCount++)
   {
      /* check tone name */
      if (nToneCount != pCPT_Params[nToneCount].rgoParams[CPT_NAME])
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Wrong tone name %d, should be %d. "
                   "(File: %s, line: %d)\n",
                   pCPT_Params[nToneCount].rgoParams[CPT_NAME],
                   nToneCount, __FILE__, __LINE__));
            return IFX_ERROR;

      }
      else
      {
         /* set tone index */
         tone[nToneCount].simple.index = nToneCount + CPT_INDEX;
         /* set tone type */
         tone[nToneCount].simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
      }

      /* for all cadences */
      for (nParamCount=CADENCE_ONE; nParamCount<=CADENCE_FOUR; nParamCount++)
      {
         /* check cadence */
         if ( (0 <= pCPT_Params[nToneCount].rgoParams[nParamCount])  &&
              (MAX_CADENCE >= pCPT_Params[nToneCount].rgoParams[nParamCount]) )
         {
            tone[nToneCount].simple.cadence[nParamCount - CADENCE_ONE] =
               pCPT_Params[nToneCount].rgoParams[nParamCount];
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Wrong cadence value %d for CPT %d. "
                   "(File: %s, line: %d)\n",
                   pCPT_Params[nToneCount].rgoParams[nParamCount],
                   nToneCount, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
      /* check frequencies, first frequency must be set, second one can be 0 */
      if ( ( (MAX_TONE_FREQ > pCPT_Params[nToneCount].rgoParams[FREQ_ONE]) ||
             (0 < pCPT_Params[nToneCount].rgoParams[FREQ_ONE]) ) &&
           ( (MAX_TONE_FREQ > pCPT_Params[nToneCount].rgoParams[FREQ_TWO]) ||
             (0 <= pCPT_Params[nToneCount].rgoParams[FREQ_TWO]) ) )
      {
         tone[nToneCount].simple.freqA =
            pCPT_Params[nToneCount].rgoParams[FREQ_ONE];
         tone[nToneCount].simple.freqB =
            pCPT_Params[nToneCount].rgoParams[FREQ_TWO];

         if (0 == pCPT_Params[nToneCount].rgoParams[FREQ_TWO])
         {
            /* play only freqA */
            tone[nToneCount].simple.frequencies [0] = IFX_TAPI_TONE_FREQA;
            /* -9 dB = -90 x 0.1 dB for the higher frequency */
            tone[nToneCount].simple.levelA   = -90;
            tone[nToneCount].simple.levelB   = 0;
         }
         else
         {
            /* Play both frequencies as dtmf  */
            tone[nToneCount].simple.frequencies[0] =
               (IFX_TAPI_TONE_FREQA | IFX_TAPI_TONE_FREQB);
            /* -11 dB = -110 x 0.1 dB for the lower frequency ,
               -9  dB = -90  * 0.1 dB for the higher frequency
                        (to attenuate the higher tx loss) */
            tone[nToneCount].simple.levelA   = -110;
            tone[nToneCount].simple.levelB   = -90;
         }
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Wrong frequency value for CPT %d. "
                "(File: %s, line: %d)\n",
                nToneCount, __FILE__, __LINE__));

         return IFX_ERROR;
      }/* check frequencies */

   } /* for (nToneCount=0; nToneCount<MAX_TONE; nToneCount++) */

   /* for all tones */
   for (nToneCount=0; nToneCount<MAX_TONE; nToneCount++)
   {
      /* for all boards */
      for (j=0; j<pCtrl->nBoardCnt; j++)
      {
         /* check if tone defined */
         if (0 == tone[nToneCount].simple.cadence[0])
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                  ("CPT %d not defined\n", nToneCount));
            /* skip this one */
            continue;
         }

         /* set new CPT params */
         ret = TD_IOCTL(pCtrl->rgoBoards[j].nDevCtrl_FD,
                  IFX_TAPI_TONE_TABLE_CFG_SET, &(tone[nToneCount]),
                  TD_DEV_NOT_SET, TD_CONN_ID_ITM);
         if (IFX_ERROR == ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Failed to set CPT %d for board %d. "
                   "(File: %s, line: %d)\n",
                   nToneCount, j, __FILE__, __LINE__));
            return ret;
         }
      }/* for (j=0; j<pCtrl->nBoardCnt; j++) */

   } /* for (nToneCount=0; nToneCount<MAX_TONE; nToneCount++) */
   return ret;
}

/**
   Change CPT according to TCL message.

   \param pCtrl   - pointer to control structure
   \param pITM_MsgHead - pointer to ITM msg header data structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t COM_CPT_ChangeCountrySettings(CTRL_STATUS_t* pCtrl,
                                           ITM_MSG_HEADER_t* pITM_MsgHead)
{
   CPT_PARAM_t nParamCount, nParamType;
   CPT_NAME_t nToneCount, nToneType;
   CPT_ParamsTable_t rgoCPT[MAX_TONE];
   KEY_UINT_VALUE_UINT_t oData;
   /* check first key/value pair */
   IFX_int32_t i = 0;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   /* reset table */
   memset (rgoCPT, 0, sizeof(CPT_ParamsTable_t) * MAX_TONE );

   /* check if valid number of repetitions */
   if ((MAX_TONE * MAX_PARAM) != pITM_MsgHead->nRepetition)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Wrong wrong number of repetions %d should be %d."
             "(File: %s, line: %d)\n", pITM_MsgHead->nRepetition,
             (MAX_TONE * MAX_PARAM), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* for all tones (CPT) */
   for (nToneCount=0; nToneCount < MAX_TONE; nToneCount++)
   {
      /* for all tone params */
      for (nParamCount=0; nParamCount<MAX_PARAM; nParamCount++)
      {
         /* get tone number from key field */
         if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &oData.nKey, i,
                                         ITM_MSG_DATA_TYPE_KEY))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Failed to get key value."
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         /* get tone type from key */
         if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &oData.nValue, i,
                                         ITM_MSG_DATA_TYPE_VALUE))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Failed to get value value."
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         /* get tone and param type */
         nToneType = (IFX_int32_t) (oData.nKey / MAX_PARAM);
         nParamType = (IFX_int32_t) (oData.nKey % MAX_PARAM);

         /* check if right tone and param */
         if ((nToneCount != nToneType) || (nParamCount != nParamType))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Wrong wrong key for tone %d, param %d."
                   "(File: %s, line: %d)\n",
                   nToneCount, nParamCount, __FILE__, __LINE__));
            return IFX_ERROR;
         }

         /* copy value to CPT params table */
         rgoCPT[nToneCount].rgoParams[nParamCount] = oData.nValue;
         /* go to next pair */
         i++;
      } /* for all tone params */
   } /* for all tones (CPT) */

   /* change CPT */
   return COM_CPT_Set(pCtrl, rgoCPT);
} /* COM_CPT_ChangeCountrySettings */

/**
   Function to handle ITM message to start Analog to IP Call test.

   \param pCtrl  - pointer to control structure
   \param pITM_MsgHead - pointer to structure with common ITM message data.

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
static IFX_return_t COM_AnalogToIP_Call(CTRL_STATUS_t* pCtrl,
                                        ITM_MSG_HEADER_t* pITM_MsgHead)
{
   TD_OS_sockAddr_t oTestAdd;
   IFX_uint8_t oTestMAC[ETH_ALEN];
   IFX_uint32_t nKey, nValue;
   IFX_uint32_t i, nStartStop = COM_TEST_STOP;
   IFX_char_t* pValue = IFX_NULL;
   IFX_return_t nRet = IFX_SUCCESS;
   STATE_MACHINE_STATES_t nAction = S_TEST_TONE;
   /* set initial values - those values should chenge before using variables */
   IFX_uint32_t nCalledPhoneNumber  = 0xFFFFFFFF;
   IFX_uint32_t nTestPort         = 0xFFFFFFFF;
   IFX_uint32_t nTestIP           = 0xFFFFFFFF;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   memset(&oTestAdd, 0, sizeof(oTestAdd));
   /* for all key/value pairs */
   for (i=0; i<pITM_MsgHead->nRepetition; i++)
   {
      /* get tone number from key field */
      if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nKey, i,
                                      ITM_MSG_DATA_TYPE_KEY))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get key value."
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get phone channel number from value field (tone is played
         on data channel if aviable) */
      if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nValue, i,
                                      ITM_MSG_DATA_TYPE_VALUE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value value."
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check if valid key - should be number of key/value pair */
      if (i != nKey)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid Key value(%d) for ITM command id(%d)"
                "(File: %s, line: %d)\n",
                nKey, pITM_MsgHead->nId, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      switch(nKey)
      {
         case ANALOG_TO_IP_CALL_CTRL_CALLED_NUMBER:
            /* Get called phone number */
            nCalledPhoneNumber = nValue;
            break;
         case ANALOG_TO_IP_CALL_CTRL_IP:
            /* Get testing card IP */
            nTestIP = nValue;
            break;
         case ANALOG_TO_IP_CALL_CTRL_PORT:
            /* get IP address */
            nTestPort = nValue;
            break;
         case ANALOG_TO_IP_CALL_CTRL_MAC:
            pValue = COM_GetMsgDataPointer(pITM_MsgHead, i,
                                           ITM_MSG_DATA_TYPE_VALUE);
            if (IFX_NULL == pValue)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, ITM: Failed to get value pointer."
                      "(File: %s, line: %d)\n", __FILE__, __LINE__));
               return IFX_ERROR;
            }
            /* Get MAC address - used by EASY336 but can be set
               for all boards */
            memcpy(oTestMAC, pValue, ETH_ALEN);
            break;

         case ANALOG_TO_IP_CALL_CTRL_START_STOP:
            /* end or start call */
            nStartStop = nValue;
            if (COM_TEST_START == nStartStop)
            {
               /* Start analog to IP call */
               nAction = IE_RINGING;
            }
            else if (COM_TEST_STOP == nStartStop)
            {
               /* Stop analog to IP call */
               nAction = IE_READY;
            }
            else
            {
               /* invalid data */
               nRet = IFX_ERROR;
            }
            break;
         default:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Invalid Key value(%d) "
                   "for ITM command id(%d)"
                   "(File: %s, line: %d)\n",
                   nKey, pITM_MsgHead->nId, __FILE__, __LINE__));
            return IFX_ERROR;
            break;
      }
      if (IFX_ERROR == nRet)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid Value field(%d) for ITM command id(%d)"
                "(File: %s, line: %d)\n",
                i, pITM_MsgHead->nId, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }/* for all key/value pairs */
   if (IFX_ERROR != nRet)
   {
      /* Send message to phone if no error occured */
      nRet = COM_ITM_SetAction(pCtrl, nCalledPhoneNumber, oTestMAC,
                               nTestPort, nAction);
   }

   if (IFX_ERROR == nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to send message to phone %d"
             "(File: %s, line: %d)\n",
             nCalledPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set Abascus address */
   TD_SOCK_FamilySet(&oTestAdd, AF_INET);
   TD_SOCK_PortSet(&oTestAdd, nTestPort);
   TD_SOCK_AddrIPv4Set(&oTestAdd, htonl(nTestIP));

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
         ("ITM: Analog2IP set addr [%s]:%d - %d\n",
          TD_GetStringIP(&oTestAdd), TD_SOCK_PortGet(&oTestAdd), nStartStop));

   return COM_SetGetPort(&oTestAdd, nStartStop);
} /* COM_AnalogToIP_Call */

/**
   Get board number, device number, phone channels numbers and tone numbers
   from ITM message and set phones to play chosen tone after hook-off.
   Playing predefined tones is done
   when TCL_server sends GENERATE_TONE command
   There can be more than one group of board/device/channel/tone.
   nRepetitions holds nuumber of those groups.
   If one group is invalid(bad channel, bad tone or phone not in S_READY state)
   FAILED message is sent to TCL server.
   If same channel number is in more than one pair of key and value
   only the first pair is used,
   and trace is used to communicate it - this isn't treated as error.
   OK message is sent when there is no invalid pair and
   at least one phone channel is set as S_TEST_TONE,
   by command from TCL server.
   For this algorithm to work new state is added S_TEST_TONE,
   handling of this state was added in tapidemo.c file.
   Tone number is sent through pPhone->pDialedNum[0].
   valid tones numbers are: 1-12, 17, 22, 28-31

   \param pCtrl - pointer to Control Status structure
   \param pITM_MsgHead - pointer to ITM message structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_GenerateTones(CTRL_STATUS_t* pCtrl,
                               ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_uint32_t nPhoneCh, nTone, i;
   IFX_uint32_t nDev, nBoard;
   PHONE_t* pPhone;
   IFX_return_t ret = IFX_ERROR;
   /* number of phones that were in S_READY state */
   IFX_int32_t nInReady = 0;
   /* number of phones that were in S_TEST_TONE state */
   IFX_int32_t nInTestTone = 0;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   /* Generate tone on specified channel. */
   for (i=0; i < pITM_MsgHead->nRepetition; i+=4)
   {
      if ( IFX_SUCCESS != COM_GetChDataFromITM_Msg(pITM_MsgHead, i,
                                                   &nBoard, &nDev,
                                                   &nPhoneCh, &nTone))
      {
         /* error */
         return IFX_ERROR;
      }
      /* *************************************************************
      * configure channel to play tone
      ***************************************************************/
      /* verify values read from message */
      if ( nBoard >= pCtrl->nBoardCnt )
      {
         /* invalid board number */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: wrong board number (%d)."
                " (File: %s, line: %d)\n",
                nBoard, __FILE__, __LINE__));
      }
      if ( nDev >= pCtrl->rgoBoards[nBoard].nChipCnt )
      {
         /* invalid device number */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: wrong device number (%d)."
                " (File: %s, line: %d)\n",
                nDev, __FILE__, __LINE__));
      }

      pPhone = ABSTRACT_GetPHONE_OfPhoneCh(&pCtrl->rgoBoards[nBoard],
                                           nDev, nPhoneCh, TD_CONN_ID_ITM);
      /* check if ABSTRACT_GetPHONE_OfPhoneCh didn't fail */
      if (IFX_NULL == pPhone)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: phone channel not found - "
                " wrong phone channel number %d."
                "(File: %s, line: %d)\n",
                nPhoneCh, __FILE__, __LINE__));
      }
      else if (S_READY == pPhone->rgStateMachine[FXS_SM].nState)
      {
         /* check if valid tone value */
         if(((  0 < nTone ) && ( 13 > nTone )) ||
            (( 27 < nTone ) && ( 32 > nTone )) ||
             ( 17 == nTone) || ( 22 == nTone))
         {
            /* set tone number in phone structure - pDialedNum[0] */
            pPhone->pDialedNum[0] = nTone;
            pPhone->nDialNrCnt = 0;
            /* change phone state */
            pPhone->nIntEvent_FXS = IE_TEST_TONE;
            while (pPhone->nIntEvent_FXS != IE_NONE)
            {
               ST_HandleState_FXS(pCtrl, pPhone, &pPhone->rgoConn[0]);
            }
            /* increase changed phone state count */
            nInReady++;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                  ("ITM: Phone No %d, phone ch %d: "
                   "will generate TONE number(%d).\n",
                   pPhone->nPhoneNumber, nPhoneCh ,(int) nTone));
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("ITM: Phone No %d, phone ch %d: wrong "
                   "TONE number(%d). (File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, nPhoneCh,
                   (int) nTone, __FILE__, __LINE__));
         }
      }
      else if (S_TEST_TONE == pPhone->rgStateMachine[FXS_SM].nState)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Phone No %d, phone ch %d: "
                "status already is S_TEST_TONE\n",
                 pPhone->nPhoneNumber, nPhoneCh));
         /* increase already in S_TEST_TONE state count */
         nInTestTone++;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Change Phone No %d, "
                "phone ch %d status to S_READY\n",
                pPhone->nPhoneNumber, nPhoneCh));
      } /* check phone state */
   }
   /* if all pairs were valid and
   *  if at least one phone state was changed then return IFX_SUCCESS
   *  note: each tone in message has 4 key-value pairs (board no., device.,
   *  channel no. and tone no.)
   */
   if ( ( (nInReady + nInTestTone) == (pITM_MsgHead->nRepetition / 4) ) &&
        (nInReady != 0) )
   {
      ret = IFX_SUCCESS;
   }
   return ret;
} /* COM_GenerateTones() */

/**
   Change channel mapping according to ITM message.

   \param pCtrl - pointer to Control Status structure
   \param pITM_MsgHead - pointer to ITM message structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_ChannelMappingChange(CTRL_STATUS_t* pCtrl,
                                      ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_uint32_t nBoard;
#ifdef EASY336
   ITM_MAP_TABLE_t custom_map_table;
#else
   IFX_int32_t i;
   IFX_uint32_t nPhoneCh, nDataCh;
   IFX_uint32_t nDev;
   /* board number from first key-value pair */
   IFX_uint32_t nBoardFromFirstPair;
   BOARD_t* pBoard;
   IFX_int32_t nMappingCounter;
#endif /* EASY336 */
   IFX_return_t ret = IFX_SUCCESS;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);
   /* Map phone channel to data channel.
      Key: data channel number.
      Value: phone channel number. */

#ifdef EASY336
   /* Mapping channels should be not used when using resource manager. */
   /* If it's a test of DTMF digits... */
   if ((g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) &&
       (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_DTMFDIGIT))
   {
      /* one group (board no., device no., phone channel no., data channel no.)
      *  consist of 4 entries in message */
      if (1 == (pITM_MsgHead->nRepetition / 4) )
      {
         /* get board no., device no., phone and data channel from message */
         ret = COM_GetChDataFromITM_Msg(pITM_MsgHead, 0, &nBoard,
                  (IFX_uint32_t*) &custom_map_table.nDevice,
                  (IFX_uint32_t*) &custom_map_table.oMapping.nPhoneCh,
                  (IFX_uint32_t*) &custom_map_table.oMapping.nDataCh);
         if(IFX_SUCCESS != ret)
         {
            return IFX_ERROR;
         }
         /* PCM channel, not used during mapping operation */
         custom_map_table.oMapping.nPCM_Ch = 0;
         /* board and device*/
         custom_map_table.pBoard = &pCtrl->rgoBoards[nBoard];
         /* map phone and data channel */
         ret = COM_SvipCustomMapping(custom_map_table, IFX_TRUE);
         if (IFX_ERROR == ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: failed to do custom mapping,"
                   " board %d, dev %d. (File: %s, line: %d)\n",
                   nBoard, custom_map_table.nDevice, __FILE__, __LINE__));
         }
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Wrong number of mappings "
                "- for SVIP only one is allowed. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         ret = IFX_ERROR;
      }
   } /* if valid test type */
   else
   {
      /* maping channels cannot be done */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
         ("Err, ITM: Wrong test type or subtype"
          " - for SVIP change of channel mapping is only allowed"
          " in DTMF digits test. (File: %s, line: %d)\n",
          __FILE__, __LINE__));
      ret = IFX_ERROR;
   }
#else /* EASY336 */
   /* board number from first group from message (board no., device no.,*
   *  phone channel no., data channel no.) */
   if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nBoardFromFirstPair, 0,
                                   ITM_MSG_DATA_TYPE_VALUE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get board number from first key-value pair."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get pointer to board */
   pBoard = &pCtrl->rgoBoards[nBoardFromFirstPair];

   /* unmapp current mapping and free memory if needed */
   if (pBoard->pChCustomMapping == IFX_NULL)
   {
      ret = ABSTRACT_UnmapDefaults(pCtrl, pBoard, TD_CONN_ID_ITM);

      /* check unmapping result */
      if (IFX_SUCCESS != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: unmapping of default channel mapping failed."
                " (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         /* restore mapping */
         ABSTRACT_DefaultMapping(pCtrl, pBoard, TD_CONN_ID_ITM);
         return IFX_ERROR;
      }
   }
   else
   {
      ret = ABSTRACT_UnmapCustom(pBoard, TD_CONN_ID_ITM);
      TD_OS_MemFree(pBoard->pChCustomMapping);
      pBoard->pChCustomMapping = IFX_NULL;

      /* check unmapping result */
      if (IFX_SUCCESS != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: unmapping of custom channel mapping failed."
                " (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         /* restore mapping */
         ABSTRACT_DefaultMapping(pCtrl, pBoard, TD_CONN_ID_ITM);
         return IFX_ERROR;
      }
   }


   /* allocate memory */
   pBoard->pChCustomMapping = (STARTUP_MAP_TABLE_t *)
         TD_OS_MemCalloc(pBoard->nMaxAnalogCh, sizeof(STARTUP_MAP_TABLE_t));
   /* check if memory allocated */
   if (pBoard->pChCustomMapping == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
         ("Err, allocating memory."
          "(File: %s, line: %d)\n", __FILE__, __LINE__));
      /* restore mapping */
      ABSTRACT_DefaultMapping(pCtrl, pBoard, TD_CONN_ID_ITM);
      return IFX_ERROR;
   }
   nMappingCounter = 0;
   /* for all key/value pairs */
   for (i = 0; i < pITM_MsgHead->nRepetition; i+=4)
   {
      /* get board no., device no., phone and data channel from message */
      ret = COM_GetChDataFromITM_Msg(pITM_MsgHead, i,
                                     &nBoard, &nDev, &nPhoneCh, &nDataCh);
      if(IFX_SUCCESS != ret)
      {
         return IFX_ERROR;
      }

      /* check board number */
      if (nBoardFromFirstPair != nBoard)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: mapping for different boards in one message "
             "is not allowed. "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
         /* restore mapping */
         ABSTRACT_DefaultMapping(pCtrl, pBoard, TD_CONN_ID_ITM);
         return IFX_ERROR;
      }

      /* Phone channel */
      pBoard->pChCustomMapping[nMappingCounter].nPhoneCh = nPhoneCh;
      /* Data channel */
      pBoard->pChCustomMapping[nMappingCounter].nDataCh = nDataCh;
      /* PCM channel */
      pBoard->pChCustomMapping[nMappingCounter].nPCM_Ch = nMappingCounter;

      ++nMappingCounter;
   } /* for all key/value pairs */
   /* if the given number of mapping sets is less than number of phone
      channels */
   if (nMappingCounter < pBoard->nMaxAnalogCh)
   {
      /* append missing values */
      for (i = nMappingCounter; i < pBoard->nMaxAnalogCh; ++i)
      {
         /* Phone channel */
         pBoard->pChCustomMapping[i].nPhoneCh = -1;
         /* Data channel */
         pBoard->pChCustomMapping[i].nDataCh = -1;
         /* PCM channel */
         pBoard->pChCustomMapping[i].nPCM_Ch = -1;
      }
   }

   /* do mapping */
   ret = ABSTRACT_CustomMapping(pBoard, nMappingCounter, TD_CONN_ID_ITM);
#endif /* EASY336 */
   return ret;
} /* COM_ChannelConnectionChange() */

/**
   Start/stop test, mainly save and restore CID settings.

   \param  pCtrl        - pointer to control structure
   \param  nStartStop   - if test should be started stopped

   \return IFX_SUCCESS or IFX_ERROR
   \remark
*/
IFX_return_t COM_CID_TestInit(CTRL_STATUS_t* pCtrl,
                              COM_TEST_START_STOP_t nStartStop)
{
   static IFX_int32_t nCidStandard = TD_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* check if test is already started */
   if (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_CID)
   {
      /* check if stopping a started test */
      if (COM_TEST_STOP == nStartStop)
      {
         /* cid standard should be set */
         if (TD_NOT_SET == nCidStandard)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: nCidStandard not set."
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         } /* cid standard should be set */

         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
               ("ITM: CID restoring settings.\n"));

         /* restore CID setting */
         if (IFX_SUCCESS != COM_CID_StandardSet(pCtrl, nCidStandard))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: COM_CID_StandardSet() failed."
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         } /* restore CID setting */

         nCidStandard = TD_NOT_SET;
         return IFX_SUCCESS;
      } /* check if stoping a started test */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: invalid value nStartStop %d."
             "(File: %s, line: %d)\n",
             nStartStop, __FILE__, __LINE__));
      return IFX_ERROR;
   } /* check if test already started */
   /* stopped test can be only started */
   if (COM_TEST_START == nStartStop)
   {
      /* cid standard shouldn't be set */
      if (TD_NOT_SET != nCidStandard)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: nCidStandard not set."
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      } /* cid standard should be set */

      /* save standard setting */
      nCidStandard = CID_GetStandard();

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
            ("ITM: CID saving settings.\n"));

      return IFX_SUCCESS;
   } /* stopped test can be only started */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
         ("Err, ITM: invalid value nStartStop %d."
          "(File: %s, line: %d)\n",
          nStartStop, __FILE__, __LINE__));

   return IFX_ERROR;
}

/**
   Set CID standard.

   \param  pCtrl     - pointer to control structure
   \param  nCidStandard  - number of CID standard

   \return IFX_SUCCESS or IFX_ERROR
   \remark
*/
IFX_return_t COM_CID_StandardSet(CTRL_STATUS_t* pCtrl,
                                 IFX_int32_t nCidStandard)
{
   IFX_int32_t nBoard, nPhone;
   IFX_return_t nRet = IFX_SUCCESS;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->pProgramArg, IFX_ERROR);

   /* CID can be turned off */
   if (TD_TURN_OFF_CID == nCidStandard)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("CID support disabled.\n"));
      pCtrl->pProgramArg->oArgFlags.nCID = 0;
      return nRet;
   }
   /* check CID standard */
   switch(nCidStandard)
   {
      /* CID standards */
      case IFX_TAPI_CID_STD_TELCORDIA:
      case IFX_TAPI_CID_STD_ETSI_FSK:
      case IFX_TAPI_CID_STD_ETSI_DTMF:
      case IFX_TAPI_CID_STD_SIN:
      case IFX_TAPI_CID_STD_NTT:
#ifndef TAPI_VERSION4
      case IFX_TAPI_CID_STD_KPN_DTMF:
      case IFX_TAPI_CID_STD_KPN_DTMF_FSK:
#endif /* TAPI_VERSION4 */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("CID standard set to %s (%d).\n",
                Common_Enum2Name(nCidStandard, TD_rgCidName), nCidStandard));
         if (IFX_ERROR == CID_SetStandard(nCidStandard, TD_CONN_ID_ITM))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Failed to set CID standard."
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         /* for all boards */
         for (nBoard=0; nBoard < pCtrl->nBoardCnt; nBoard++)
         {
            /* for all phones */
            for (nPhone = 0; nPhone < pCtrl->rgoBoards[nBoard].nMaxPhones;
                  nPhone++)
            {
               /* set CID data (and allocate memory) if turning on CID support */
               if (!pCtrl->pProgramArg->oArgFlags.nCID)
               {
                  CID_SetupData(pCtrl->rgoBoards[nBoard].rgoPhones[nPhone].nPhoneCh,
                                &pCtrl->rgoBoards[nBoard].rgoPhones[nPhone]);
               }
               /* configure CID driver */
               if (IFX_SUCCESS !=
                   CID_ConfDriver(&pCtrl->rgoBoards[nBoard].rgoPhones[nPhone]))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, ITM: Phone No %d: "
                         "Failed to configure CID driver."
                         "(File: %s, line: %d)\n",
                         pCtrl->rgoBoards[nBoard].rgoPhones[nPhone].nPhoneNumber,
                          __FILE__, __LINE__));
                  nRet = IFX_ERROR;
               }
            } /* for all phones */
         } /* for all boards */
         /* turn on CID support */
         pCtrl->pProgramArg->oArgFlags.nCID = 1;
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid or unhandled CID standard value %d."
                "(File: %s, line: %d)\n", nCidStandard, __FILE__, __LINE__));
         return IFX_ERROR;
         break;
   } /* swich(nCidStandard) */

   return nRet;
}

/**
   Parse CID data and send to TCL script

   \param pPhone  - pointer to phone structure
   \param pCID_Msg  - pointer to message structure
   \param nCallPlace  - where function was called
   \param pGroupName  - TCL script message group name
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_CID_MsgParse(PHONE_t* pPhone,
                              IFX_TAPI_CID_MSG_t* pCID_Msg,
                              TEST_TRACE_PLACE_t nCallPlace,
                              IFX_char_t* pGroupName, IFX_uint32_t nSeqConnId)
{
   IFX_char_t* pszSubGroupPrefix = IFX_NULL;
   IFX_char_t pszDate[TD_CID_TRANSP_DATE_SIZE + 1] = {0};
   IFX_char_t pszName[IFX_TAPI_CID_MSG_LEN_MAX + 1] = {0};
   IFX_char_t pszNumber[IFX_TAPI_CID_MSG_LEN_MAX + 1] = {0};
   IFX_int32_t i, nTranspLen, nTranspNumberLen, nTranspSum = 0;
   IFX_uint8_t* pData;
   IFX_return_t nRet;

   /* check input parameters */
   TD_PTR_CHECK(pCID_Msg, IFX_ERROR);
   TD_PTR_CHECK(pGroupName, IFX_ERROR);
   TD_PTR_CHECK(pCID_Msg->message, IFX_ERROR);

   /* set subgroup name */
   if (TTP_CID_MSG_RECEIVED_FSK == nCallPlace)
   {
      pszSubGroupPrefix = "RECEIVED_";
   }
   else if (TTP_CID_MSG_TO_SEND == nCallPlace)
   {
      pszSubGroupPrefix = "SENDING_";
   }
   else
   {
      /* invalid input value */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, ITM: Invalid TTP %d. (File: %s, line: %d)\n",
             TTP_CID_MSG_TO_SEND, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* check if transparent message */
   if (IFX_TAPI_CID_ST_TRANSPARENT ==
       pCID_Msg->message->transparent.elementType)
   {
      /* get pointer to data array */
      pData = pCID_Msg->message->transparent.data;


      /* check if length is OK */
      if ((IFX_NULL != pData) &&
          ( ( TD_CID_TRANSP_NON_DATA_BYTES_NUMBER +
              pData[TD_CID_TRANSP_OFFSET_LENGTH] ) !=
            pCID_Msg->message->transparent.len ) )
      {
         /* one additional byte is allowed (should be set to 0)
            see JIRA_RND - VOICECPE_SW-337 for more details */
         if ( ( TD_CID_TRANSP_NON_DATA_BYTES_NUMBER +
                pData[TD_CID_TRANSP_OFFSET_LENGTH] + 1 ) ==
              pCID_Msg->message->transparent.len )
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("ITM: INFO: One additional byte in CID RX message detected\n"));
         }
         else
         {
            /* invalid number of bytes detected */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, ITM:  msg len %d != read bytes %d for "
                   "transparent message. (File: %s, line: %d)\n",
                   (TD_CID_TRANSP_NON_DATA_BYTES_NUMBER +
                    pData[TD_CID_TRANSP_OFFSET_LENGTH]),
                   pCID_Msg->message->transparent.len,
                   __FILE__, __LINE__));
         }
         /* if this problem occured then correct len value */
         pCID_Msg->message->transparent.len =
            TD_CID_TRANSP_NON_DATA_BYTES_NUMBER + pData[TD_CID_TRANSP_OFFSET_LENGTH];
      }

      /* data array pointer is set */
      if (IFX_NULL == pData)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, ITM: Invalid data pointer for transparent"
                " message. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
      /* there should be at least 3 bytes */
      else if (TD_CID_TRANSP_NON_DATA_BYTES_NUMBER >
               pCID_Msg->message->transparent.len)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, ITM: Not enough data for transparent message. "
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
      /* check type of message */
      else if (TD_CID_TRANSP_TYPE_DATE_AND_NUMBER !=
               pData[TD_CID_TRANSP_OFFSET_TYPE])
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, ITM: Transparent message type %d is not handled. "
                "(File: %s, line: %d)\n",
                pData[TD_CID_TRANSP_OFFSET_TYPE],
                __FILE__, __LINE__));
      }
      else
      {
         /* get length of message */
         nTranspLen = pCID_Msg->message->transparent.len;
         /* get length of number field */
         nTranspNumberLen = nTranspLen - TD_CID_TRANSP_NON_DATA_BYTES_NUMBER
            - TD_CID_TRANSP_DATE_SIZE;
         /* check if length of number is valid */
         if ((0 >= nTranspNumberLen) ||
             (IFX_TAPI_CID_MSG_LEN_MAX < nTranspNumberLen))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, ITM: Invalid length of number field - %d. "
                   "(File: %s, line: %d)\n",
                   nTranspNumberLen, __FILE__, __LINE__));
         }
         else
         {
            /* check crc */
            for (i=0; i<nTranspLen; i++)
            {
               nTranspSum += pData[i];
            }
            /* to count CRC: crc = ((~(sumDATA & 0xFF)) + 1)
            last byte in transparent msg is CRC number,
            modulo of sum of all data bits and CRC number
            and 0x100 must give 0 */
            if (0 != (nTranspSum % 0x100))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                     ("Err, ITM: CRC check failed for CID "
                      "transparent message. "
                      "(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
            /* check size  */
            if (nTranspLen <= (TD_CID_TRANSP_OFFSET_DATE + TD_CID_TRANSP_DATE_SIZE) ||
                nTranspLen <= (TD_CID_TRANSP_OFFSET_DATE + nTranspNumberLen))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                     ("Err, ITM: invalid msg length %d. "
                      "(File: %s, line: %d)\n",
                      nTranspLen, __FILE__, __LINE__));
            }
            else
            {
               /* copy date and number strings */
               memcpy(pszDate, &pData[TD_CID_TRANSP_OFFSET_DATE],
                      TD_CID_TRANSP_DATE_SIZE);
               pszDate[TD_CID_TRANSP_DATE_SIZE] = '\0';
               memcpy(pszNumber, &pData[TD_CID_TRANSP_OFFSET_NUMBER],
                      nTranspNumberLen);
               pszNumber[IFX_TAPI_CID_MSG_LEN_MAX] = '\0';
               if (0 >= strlen(pszNumber) ||
                   IFX_TAPI_CID_MSG_LEN_MAX <= strlen(pszNumber))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                        ("Err, ITM: invalid number length %d. "
                         "(File: %s, line: %d)\n",
                         strlen(pszNumber), __FILE__, __LINE__));
                  /* clear buffer if invalid length */
                  memset(pszNumber, 0,
                         IFX_TAPI_CID_MSG_LEN_MAX * sizeof(IFX_char_t));
               }
            }
         }
      }

   } /* check if transparent */
   else
   {
      /* for all message fields */
      for (i =0; i < pCID_Msg->nMsgElements; i++)
      {
         /* handle element type */
         switch(pCID_Msg->message[i].string.elementType)
         {
            case IFX_TAPI_CID_ST_DATE:
               /* set date string */
               TD_COM_CID_DATE_CHECK(pCID_Msg->message[i].date.month, i,
                                     nSeqConnId);
               TD_COM_CID_DATE_CHECK(pCID_Msg->message[i].date.day, i,
                                     nSeqConnId);
               TD_COM_CID_DATE_CHECK(pCID_Msg->message[i].date.hour, i,
                                     nSeqConnId);
               TD_COM_CID_DATE_CHECK(pCID_Msg->message[i].date.mn, i,
                                     nSeqConnId);
               sprintf(pszDate, "%02u%02u%02u%02u",
                       pCID_Msg->message[i].date.month % 100,
                       pCID_Msg->message[i].date.day % 100,
                       pCID_Msg->message[i].date.hour % 100,
                       pCID_Msg->message[i].date.mn % 100);
               break;
            case IFX_TAPI_CID_ST_CLI:
               /* get number string */
               TD_COM_CID_STRING_LEN_CHECK(nRet,
                                           pCID_Msg->message[i].string.len,
                                           pCID_Msg->message[i].string.element,
                                           i, nSeqConnId);
               if (IFX_SUCCESS == nRet)
               {
                  memcpy (pszNumber, pCID_Msg->message[i].string.element,
                          pCID_Msg->message[i].string.len);
                  pszNumber[IFX_TAPI_CID_MSG_LEN_MAX] = '\0';
               }
               break;
            case IFX_TAPI_CID_ST_NAME:
               /* get peer name string */
               TD_COM_CID_STRING_LEN_CHECK(nRet,
                                           pCID_Msg->message[i].string.len,
                                           pCID_Msg->message[i].string.element,
                                           i, nSeqConnId);
               if (IFX_SUCCESS == nRet)
               {
                  memcpy (pszName, pCID_Msg->message[i].string.element,
                          pCID_Msg->message[i].string.len);
                  pszName[IFX_TAPI_CID_MSG_LEN_MAX] = '\0';
               }
               break;
            default:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
                     ("ITM: unhandled CID field type %d(%d).\n",
                      pCID_Msg->message[i].string.elementType, i));
               break;
         } /* switch(pCID_Msg->message[i].string.elementType) */
      } /* for (i =0; i < pCID_Msg->nMsgElements; i++) */
   } /* check if transparent */

   /* check call place */
   if (TTP_CID_MSG_RECEIVED_FSK == nCallPlace)
   {
      /* send mode and standard */
      TAPIDEMO_PRINTF(nSeqConnId,
         ("%s:%sSTANDARD::FSK\n", pGroupName, pszSubGroupPrefix));
      TAPIDEMO_PRINTF(nSeqConnId,
         ("%s:%sMODE::%s\n", pGroupName, pszSubGroupPrefix,
          (IFX_TAPI_CID_HM_OFFHOOK == pCID_Msg->txMode) ?
          "OFF_HOOK_TX" :
          (IFX_TAPI_CID_HM_ONHOOK == pCID_Msg->txMode) ?
          "ON_HOOK_TX" : "INVALID"));
      TAPIDEMO_PRINTF(nSeqConnId,
         ("%s:%sDATE:%s:%s\n", pGroupName, pszSubGroupPrefix,
          ('\0' == pszDate[0]) ? "OFF" : "ON", pszDate));
      TAPIDEMO_PRINTF(nSeqConnId,
         ("%s:%sNUMBER:%s:%s\n", pGroupName, pszSubGroupPrefix,
          ('\0' == pszNumber[0]) ? "OFF" : "ON", pszNumber));
      TAPIDEMO_PRINTF(nSeqConnId,
         ("%s:%sNAME:%s:%s\n", pGroupName, pszSubGroupPrefix,
          ('\0' == pszName[0]) ? "OFF" : "ON", pszName));
   }
   else if (TTP_CID_MSG_TO_SEND == nCallPlace)
   {
      /* check if phone pointer is set */
      TD_PTR_CHECK(pPhone, IFX_ERROR);
      /* send mode and standard */
      TAPIDEMO_PRINTF(nSeqConnId,
         ("%s:%sSTANDARD:%d:%d\n", pGroupName, pszSubGroupPrefix,
          CID_GetStandard(), pPhone->nPhoneNumber));
      TAPIDEMO_PRINTF(nSeqConnId,
         ("%s:%sMODE:%s:%d\n", pGroupName, pszSubGroupPrefix,
          (IFX_TAPI_CID_HM_OFFHOOK == pCID_Msg->txMode) ?
          "OFF_HOOK_TX" :
          (IFX_TAPI_CID_HM_ONHOOK == pCID_Msg->txMode) ?
          "ON_HOOK_TX" : "INVALID",  pPhone->nPhoneNumber));
      /* for DTMF CID date isn't send */
      if (IFX_TAPI_CID_STD_ETSI_DTMF != CID_GetStandard())
      {
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:%sDATE:%s:%d\n", pGroupName, pszSubGroupPrefix,
             pszDate, pPhone->nPhoneNumber));
      }
      else
      {
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:%sDATE::%d\n", pGroupName, pszSubGroupPrefix,
             pPhone->nPhoneNumber));

      }
      TAPIDEMO_PRINTF(nSeqConnId,
         ("%s:%sNUMBER:%s:%d\n", pGroupName, pszSubGroupPrefix,
          pszNumber, pPhone->nPhoneNumber));
      /* for DTMF CID name isn't send */
      if (IFX_TAPI_CID_STD_ETSI_DTMF != CID_GetStandard())
      {
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:%sNAME:%s:%d\n", pGroupName, pszSubGroupPrefix,
             pszName, pPhone->nPhoneNumber));
      }
      else
      {
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:%sNAME::%d\n", pGroupName, pszSubGroupPrefix,
             pPhone->nPhoneNumber));
      }
   }
   return IFX_SUCCESS;
}
/**
   Parse CID (DTMF standard) data and send to TCL script

   \param pFXO  - pointer to FXO structure
   \param pGroupName  - TCL script message group name

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_CID_MsgParseDTMF_Rec(FXO_t* pFXO,
                                      IFX_char_t* pGroupName)
{
   IFX_char_t pszNumber[IFX_TAPI_CID_MSG_LEN_MAX + 1] = {0};

   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pGroupName, IFX_ERROR);

   /* set subgroup name */
   if (TD_CID_DTMF_DIGIT_CNT_MIN <= pFXO->oCID_DTMF.nCnt &&
       IFX_TAPI_CID_MSG_LEN_MAX > pFXO->oCID_DTMF.nCnt)
   {
      if (IFX_TAPI_CID_MSG_LEN_MAX > strlen(&pFXO->oCID_DTMF.acDialed[1]) &&
          MAX_DIALED_NUM > pFXO->oCID_DTMF.nCnt)
      {
         memcpy(pszNumber, &pFXO->oCID_DTMF.acDialed[1],
                (pFXO->oCID_DTMF.nCnt - TD_CID_DTMF_START_STOP_DIGIT_CNT));
         pszNumber[IFX_TAPI_CID_MSG_LEN_MAX] = '\0';
      }
   }
   /* send CID data */
   TAPIDEMO_PRINTF(pFXO->nSeqConnId,
      ("%s:RECEIVED_STANDARD::DTMF\n", pGroupName));
   TAPIDEMO_PRINTF(pFXO->nSeqConnId,
      ("%s:RECEIVED_MODE::ON_HOOK_TX\n", pGroupName));
   TAPIDEMO_PRINTF(pFXO->nSeqConnId,
      ("%s:RECEIVED_DATE:OFF:\n", pGroupName));
   TAPIDEMO_PRINTF(pFXO->nSeqConnId,
      ("%s:RECEIVED_NUMBER:%s:%s\n", pGroupName,
       ('\0' == pszNumber[0]) ? "OFF" : "ON", pszNumber));
   TAPIDEMO_PRINTF(pFXO->nSeqConnId,
       ("%s:RECEIVED_NAME:OFF:\n", pGroupName));

   return IFX_SUCCESS;
}

/**
   Set CID data from test script.

   \param pCID_Msg   - pointer to CID message structure
   \param pPhone     - pointer to phone structure

   \return pointer to CID message structure
*/
IFX_TAPI_CID_MSG_t * COM_CID_DataSet(IFX_TAPI_CID_MSG_t *pCID_Msg,
                                     PHONE_t* pPhone)
{
   IFX_TAPI_CID_MSG_t *pCID_MsgITM;
   IFX_int32_t i;

   /* check input parameters */
   TD_PTR_CHECK(pCID_Msg, pCID_Msg);
   TD_PTR_CHECK(pCID_Msg->message, pCID_Msg);
   TD_PTR_CHECK(pPhone, pCID_Msg);

   /* check if tested phone is used */
   if (pPhone->nPhoneNumber != g_pITM_Ctrl->oCID_Test.nPhoneNumber)
   {
      return pCID_Msg;
   }
   /* set pointer */
   pCID_MsgITM = g_pITM_Ctrl->oCID_Test.pCID_Msg;

   /* check if memory allocated */
   if (IFX_NULL == pCID_MsgITM)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Memory for CID not allocated. "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return pCID_Msg;
   }
   /* check if memory allocated */
   if (IFX_NULL == pCID_MsgITM->message)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Memory for CID data not allocated. "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return pCID_Msg;
   }

   /* for all message elements if some field was not set then use the default
      values */
   for (i=0; i<pCID_Msg->nMsgElements; i++)
   {
      /* check if name was set, if not use data from default CID message */
      if (!g_pITM_Ctrl->oCID_Test.bNameIsSet &&
          IFX_TAPI_CID_ST_NAME == pCID_Msg->message[i].string.elementType)
      {
         memcpy (&pCID_MsgITM->message[TD_CID_IDX_NAME].string,
                 &pCID_Msg->message[i].string,
                 sizeof (IFX_TAPI_CID_MSG_STRING_t));
      }
      /* check if number was set, if not use data from default CID message */
      if (!g_pITM_Ctrl->oCID_Test.bNumberIsSet &&
          IFX_TAPI_CID_ST_CLI == pCID_Msg->message[i].string.elementType)
      {
         memcpy (&pCID_MsgITM->message[TD_CID_IDX_CLI_NUM].string,
                 &pCID_Msg->message[i].string,
                 sizeof (IFX_TAPI_CID_MSG_STRING_t));
      }
   }
   /* reset phone number, data from script can be used only once */
   g_pITM_Ctrl->oCID_Test.nPhoneNumber = 0;
   /* return pointer to CID message with tested data */
   return pCID_MsgITM;
}

/**
   Get CID data from test script message

   \param pCtrl - pointer to Control Status structure
   \param pITM_MsgHead - pointer to ITM message structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_CID_DataGet(CTRL_STATUS_t* pCtrl,
                             ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_char_t* pData;
   IFX_TAPI_CID_MSG_t* pCID_Msg;
   IFX_uint32_t nType;
   IFX_uint32_t nLen, i;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   /* get CID pointer */
   pCID_Msg = g_pITM_Ctrl->oCID_Test.pCID_Msg;
   /* check if memory for CID is allocated */
   if (IFX_NULL == pCID_Msg)
   {
      pCID_Msg = TD_OS_MemCalloc(1, sizeof (IFX_TAPI_CID_MSG_t));
      if (IFX_NULL == pCID_Msg)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to allocate memory. "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }
      pCID_Msg->message = TD_OS_MemCalloc(TD_CID_ELEM_COUNT,
                                          sizeof(IFX_TAPI_CID_MSG_ELEMENT_t));
      if (IFX_NULL == pCID_Msg->message)
      {
         TD_OS_MemFree(pCID_Msg);
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to allocate memory. "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }
      pCID_Msg->nMsgElements = TD_CID_ELEM_COUNT;
      g_pITM_Ctrl->oCID_Test.pCID_Msg = pCID_Msg;
   }
   if (IFX_NULL == pCID_Msg->message)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: pCID_Msg->message not set. "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* reset CID data */
   memset(pCID_Msg->message, 0,
          TD_CID_ELEM_COUNT * sizeof(IFX_TAPI_CID_MSG_ELEMENT_t));
   /* date is set right before sending CID */
   pCID_Msg->message[TD_CID_IDX_DATE].string.elementType = IFX_TAPI_CID_ST_DATE;
   pCID_Msg->message[TD_CID_IDX_CLI_NUM].string.elementType =
      IFX_TAPI_CID_ST_CLI;
   pCID_Msg->message[TD_CID_IDX_NAME].string.elementType = IFX_TAPI_CID_ST_NAME;

   g_pITM_Ctrl->oCID_Test.bNameIsSet = IFX_FALSE;
   g_pITM_Ctrl->oCID_Test.bNumberIsSet = IFX_FALSE;
   g_pITM_Ctrl->oCID_Test.nPhoneNumber = 0;

   /* for all repetitions */
   for (i = 0; i < pITM_MsgHead->nRepetition; i++)
   {
      /* get key value */
      if (IFX_SUCCESS !=  COM_GetMsgData(pITM_MsgHead, &nType, i,
                                         ITM_MSG_DATA_TYPE_KEY))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get key value for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get pointer to data */
      pData = COM_GetMsgDataPointer(pITM_MsgHead, i,
                                    ITM_MSG_DATA_TYPE_VALUE);
      if (IFX_NULL == pData)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value pointer for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get length of data string */
      nLen = strlen(pData);
      /* name and number strings can be empty, but phone number must always
         be set */
      if ((0 == nLen) && (CID_TEST_PHONE_NUM != nType))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("ITM: Cid data type %d, not set.\n", nType));
         /* skip this one */
         continue;
      }
      /* check length of data string */
      if (IFX_TAPI_CID_MSG_LEN_MAX <= nLen || 0 == nLen)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid Cid data size %d "
                "for type %d, rep %d. "
                "(File: %s, line: %d)\n",
                nLen, nType, i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check key type */
      switch (nType)
      {
      case CID_TEST_NUMBER:
         /* set string with number */
         strcpy((IFX_char_t*)pCID_Msg->message[TD_CID_IDX_CLI_NUM].string.element,
                pData);
         pCID_Msg->message[TD_CID_IDX_CLI_NUM].string.len = nLen;
         g_pITM_Ctrl->oCID_Test.bNumberIsSet = IFX_TRUE;
         break;
      case CID_TEST_NAME:
         /* set string with name */
         strcpy((IFX_char_t*)pCID_Msg->message[TD_CID_IDX_NAME].string.element,
                pData);
         pCID_Msg->message[TD_CID_IDX_NAME].string.len = nLen;
         g_pITM_Ctrl->oCID_Test.bNameIsSet = IFX_TRUE;
         break;
      case CID_TEST_PHONE_NUM:
         /* get number of tested phone */
         g_pITM_Ctrl->oCID_Test.nPhoneNumber = atoi(pData);
         if (IFX_NULL == ABSTRACT_GetPHONE_OfCalledNum(pCtrl,
                            g_pITM_Ctrl->oCID_Test.nPhoneNumber,
                            TD_CONN_ID_ITM))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Invalid phone number %d "
                   "for type %d, rep %d. "
                   "(File: %s, line: %d)\n",
                   g_pITM_Ctrl->oCID_Test.nPhoneNumber, nType, i,
                   __FILE__, __LINE__));
            g_pITM_Ctrl->oCID_Test.nPhoneNumber = 0;
            return IFX_ERROR;
         }
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid type %d in rep %d. "
                "(File: %s, line: %d)\n",
                nType, i, __FILE__, __LINE__));
         g_pITM_Ctrl->oCID_Test.nPhoneNumber = 0;
         return IFX_ERROR;
         break;

      } /* switch (nType) */
   } /* for (i = 0; i < pITM_MsgHead->nRepetition; i++) */
   /* check if tested phone number was set */
   if (0 == g_pITM_Ctrl->oCID_Test.nPhoneNumber)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Tested phone number not set. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* print info about received data */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
         ("ITM: CID : Tested phone %d (name %s, number %s).\n",
          g_pITM_Ctrl->oCID_Test.nPhoneNumber,
          (IFX_FALSE == g_pITM_Ctrl->oCID_Test.bNameIsSet) ?
          "NOT SET" :
          (IFX_char_t*) pCID_Msg->message[TD_CID_IDX_NAME].string.element,
          (IFX_FALSE == g_pITM_Ctrl->oCID_Test.bNumberIsSet) ?
          "NOT SET" :
          (IFX_char_t*) pCID_Msg->message[TD_CID_IDX_CLI_NUM].string.element));

   /* all went well */
   return IFX_SUCCESS;
}

/**
   Disable LEC and NLP for LEC test.

   \param pPhone - pointer to phone structure
   \param nEvent - event number/id

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_LEC_Disable(PHONE_t* pPhone,
                             IFX_TAPI_EVENT_ID_t nEvent)
{
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);

   /* if we got here then it is LEC test,
      reset event state won't change to FAX state */
   pPhone->nIntEvent_FXS = IE_NONE;

   /* check if handled event */
   switch (nEvent)
   {
   case IFX_TAPI_EVENT_FAXMODEM_CED:
      g_pITM_Ctrl->oLEC_Test.bDetecedEventCED = IFX_TRUE;
      break;
   case IFX_TAPI_EVENT_FAXMODEM_CEDEND:
      g_pITM_Ctrl->oLEC_Test.bDetecedEventPR = IFX_TRUE;
      break;
   case IFX_TAPI_EVENT_FAXMODEM_PR:
      g_pITM_Ctrl->oLEC_Test.bDetecedEventCEDEND = IFX_TRUE;
      break;
   default:
      /* only events above are valid, ignore rest of them */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("ITM: LEC: Phone No %d: Unhandled event %s (%d) detected.\n",
             pPhone->nPhoneNumber,
             Common_Enum2Name(nEvent, TD_rgTapiEventName), nEvent));
      return IFX_SUCCESS;
      break;
   }
   /* check if all events detected - all three are generated after
      CED with Phase Reversal that is used to force LEC disable */
   if (IFX_TRUE == g_pITM_Ctrl->oLEC_Test.bDetecedEventCED &&
       IFX_TRUE == g_pITM_Ctrl->oLEC_Test.bDetecedEventPR &&
       IFX_TRUE == g_pITM_Ctrl->oLEC_Test.bDetecedEventCEDEND &&
       IFX_TRUE != g_pITM_Ctrl->oLEC_Test.bLEC_Disabled)
   {
      /* reset detected event flags */
      g_pITM_Ctrl->oLEC_Test.bDetecedEventCED = IFX_FALSE;
      g_pITM_Ctrl->oLEC_Test.bDetecedEventPR = IFX_FALSE;
      g_pITM_Ctrl->oLEC_Test.bDetecedEventCEDEND = IFX_FALSE;
      /* disable LEC */
#if defined(EASY336) || !defined(TAPI_VERSION4)
      if (IFX_SUCCESS != Common_LEC_Disable(pPhone->pBoard->pCtrl, pPhone))
#endif /* defined(EASY333) || !defined(TAPI_VERSION4) */
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, ITM: LEC: Failed to disable LEC. "
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* send message to control PC */
      TAPIDEMO_PRINTF(pPhone->nSeqConnId,
         ("ECHO_TEST:LEC_AND_NLP:OFF:%d\n", pPhone->nPhoneNumber));
      /* set diabled flag so only one disable LEC will be done during test */
      g_pITM_Ctrl->oLEC_Test.bLEC_Disabled = IFX_TRUE;
   }
   /* all went well */
   return IFX_SUCCESS;
}

/**
   Set LEC and NLP for LEC test.

   \param pPhone - pointer to phone structure
   \param pLEC_Settings - pointer to WLEC configuration structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_LEC_DataSet(PHONE_t* pPhone,
                             IFX_TAPI_WLEC_CFG_t* pLEC_Settings)
{
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pLEC_Settings, IFX_ERROR);

   /* copy settings, copying by memset is not used
      because for TAPI V4 there is also channel and device */
   pLEC_Settings->nType = g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nType;
   pLEC_Settings->bNlp = g_pITM_Ctrl->oLEC_Test.oLEC_Settings.bNlp;
   pLEC_Settings->nNBNEwindow =
      g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nNBNEwindow;
   pLEC_Settings->nNBFEwindow =
      g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nNBFEwindow;
   pLEC_Settings->nWBNEwindow =
      g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nWBNEwindow;

   /* print info about LEC data */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("ITM: Setting LEC for Phone No %d: LEC - %s, NLP - %s.\n",
          pPhone->nPhoneNumber,
          Common_Enum2Name(pLEC_Settings->nType, TD_rgLEC_Type),
          Common_Enum2Name(pLEC_Settings->bNlp, TD_rgNLP_Type)));

   /* reset detected event flags */
   g_pITM_Ctrl->oLEC_Test.bDetecedEventCED = IFX_FALSE;
   g_pITM_Ctrl->oLEC_Test.bDetecedEventPR = IFX_FALSE;
   g_pITM_Ctrl->oLEC_Test.bDetecedEventCEDEND = IFX_FALSE;
   g_pITM_Ctrl->oLEC_Test.bLEC_Disabled = IFX_FALSE;

   /* all went well */
   return IFX_SUCCESS;
}

/**
   Get LEC data from test script message

   \param pCtrl - pointer to Control Status structure
   \param pITM_MsgHead - pointer to ITM message structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_LEC_DataGet(CTRL_STATUS_t* pCtrl,
                             ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_uint32_t nKey, nValue;
   IFX_uint32_t i;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   /* by default disable all */
   g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nType = IFX_TAPI_WLEC_TYPE_OFF;
   g_pITM_Ctrl->oLEC_Test.oLEC_Settings.bNlp = IFX_TAPI_WLEC_NLP_OFF;
   /* when set to 0 defaul value is used */
   g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nNBNEwindow = 0;
   g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nNBFEwindow = 0;
   g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nWBNEwindow = 0;

   /* for all repetitions */
   for (i = 0; i < pITM_MsgHead->nRepetition; i++)
   {
      /* get key value */
      if (IFX_SUCCESS !=  COM_GetMsgData(pITM_MsgHead, &nKey, i,
                                         ITM_MSG_DATA_TYPE_KEY))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get key for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get value value */
      if (IFX_SUCCESS !=  COM_GetMsgData(pITM_MsgHead, &nValue, i,
                                         ITM_MSG_DATA_TYPE_VALUE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check key type */
      switch (nKey)
      {
      case LEC_TEST_LEC:
         /* check value for LEC */
         switch (nValue)
         {
         case 0:
            g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nType =
               IFX_TAPI_WLEC_TYPE_OFF;
            break;
         case 1:
            g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nType =
               IFX_TAPI_WLEC_TYPE_NE;
            break;
         case 2:
            g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nType =
               IFX_TAPI_WLEC_TYPE_NFE;
            break;
         case 3:
            g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nType =
               IFX_TAPI_WLEC_TYPE_NE_ES;
            break;
         case 4:
            g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nType =
               IFX_TAPI_WLEC_TYPE_NFE_ES;
            break;
         case 5:
            g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nType =
               IFX_TAPI_WLEC_TYPE_ES;
            break;
         default:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Invalid value %d for key %d (%d). "
                   "(File: %s, line: %d)\n",
                   nValue, nKey, i, __FILE__, __LINE__));
            break;
         } /* switch (nValue) */
         break;
      case LEC_TEST_NLP:
         /* check value for NLP */
         switch (nValue)
         {
         case 1:
            g_pITM_Ctrl->oLEC_Test.oLEC_Settings.bNlp = IFX_TAPI_WLEC_NLP_ON;
            break;
         case 2:
            g_pITM_Ctrl->oLEC_Test.oLEC_Settings.bNlp = IFX_TAPI_WLEC_NLP_OFF;
            break;
         default:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Invalid value %d for key %d (%d). "
                   "(File: %s, line: %d)\n",
                   nValue, nKey, i, __FILE__, __LINE__));
            return IFX_ERROR;
            break;
         } /* switch (nValue) */
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid key %d in rep %d. (File: %s, line: %d)\n",
                nKey, i, __FILE__, __LINE__));
         return IFX_ERROR;
         break;
      } /* switch (nKey) */
   } /* for (i = 0; i < pITM_MsgHead->nRepetition; i++) */

   /* print info about received data */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
         ("ITM: Received LEC settings : LEC - %s, NLP - %s.\n",
          Common_Enum2Name(g_pITM_Ctrl->oLEC_Test.oLEC_Settings.nType,
                           TD_rgLEC_Type),
          Common_Enum2Name(g_pITM_Ctrl->oLEC_Test.oLEC_Settings.bNlp,
                           TD_rgNLP_Type)));

   /* all went well */
   return IFX_SUCCESS;
}

/**
   Calls ioctls needed to configure OOB setting and sends result message.

   \param pBoard - pointer to board structure
   \param nBoardNo - board number
   \param nDevTmp - device number
   \param nDataCh - number of data channel
   \param eventParam - structure needed for IFX_TAPI_PKT_EV_OOB_DTMF_SET.
   \param rfcParam - structure needed for IFX_TAPI_PKT_RTP_CFG_SET
                     Used for RTP, RFC 2833 event payload types configure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_OOB_Configure_Execute(BOARD_t* pBoard,
                               IFX_uint32_t nBoardNo,
                               IFX_uint32_t nDevTmp,
                               IFX_uint32_t nDataCh,
                               IFX_TAPI_PKT_EV_OOB_DTMF_t* pEventParam,
                               IFX_TAPI_PKT_RTP_CFG_t* pRfcParam)
{
   /* Local variables */
   IFX_char_t buf[MAX_CMD_LEN];
   IFX_int32_t nFD = NO_DEVICE_FD;
   IFX_return_t ioctlDTMFRet = IFX_SUCCESS;
   IFX_return_t ioctlRTPRet = IFX_SUCCESS;
   IFX_uint32_t nEventParam = 0;

   /* Check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if (IFX_NULL != pEventParam)
   {
#ifdef TAPI_VERSION4
      /* Filling the structures */
      pEventParam->dev = nDevTmp;
      pEventParam->ch = nDataCh;
      nEventParam = (IFX_uint32_t) &(*pEventParam);
#else
      nEventParam = (IFX_uint32_t) *pEventParam;
#endif /* TAPI_VERSION4 */
   }
#ifdef TAPI_VERSION4
   if (IFX_NULL != pRfcParam)
   {
      /* Filling the structures */
      pRfcParam->dev = nDevTmp;
      pRfcParam->ch = nDataCh;
   }
#endif /* TAPI_VERSION4 */

   /* Get file descriptor */
   nFD = VOIP_GetFD_OfCh(nDataCh, pBoard);
   if (NO_DEVICE_FD == nFD)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, unable to get file descriptor %d"
               "(File: %s, line: %d)\n",nFD,__FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Warning about no ioctls called */
   if ((IFX_NULL == pEventParam) && (IFX_NULL == pRfcParam))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
          ("Warning, no ioctl is called!\n"));
   }

   /* Run ioctl's */
   if (IFX_NULL != pRfcParam)
   {
      ioctlRTPRet = TD_IOCTL(nFD,  IFX_TAPI_PKT_RTP_CFG_SET,
                             (IFX_int32_t) pRfcParam,
                             nDevTmp, TD_CONN_ID_ITM);
   }
   if (IFX_SUCCESS != ioctlRTPRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, IFX_TAPI_PKT_RTP_CFG_SET ioctl error. "
               "On file descriptor: %d. "
               "IFX_TAPI_PKT_EV_OOB_DTMF_SET ioctl aborted! "
               "(File: %s, line: %d)\n", nFD, __FILE__, __LINE__));
   }
   /* Don't need to run second ioctl, if first failed */
   if ((IFX_SUCCESS == ioctlRTPRet) && (IFX_NULL != pEventParam))
   {
      ioctlDTMFRet = TD_IOCTL(nFD,  IFX_TAPI_PKT_EV_OOB_DTMF_SET,
                              nEventParam,
                              nDevTmp, TD_CONN_ID_ITM);
   }
   /* If any ioctl fails, trace */
   /* If second ioctl fails, it means, the first one was successful */
   if (IFX_SUCCESS != ioctlDTMFRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, IFX_TAPI_PKT_EV_OOB_DTMF_SET ioctl error. "
               "On file descriptor: %d. "
               "(File: %s, line: %d)\n", nFD, __FILE__, __LINE__));
   }
   /* Assemble message */
   sprintf(buf,
           "OOB_CONFIG_RESPONSE:"
           "{"
              " BOARD_NO {%d}"
              " DEV_NO {%d}"
              " CHANNEL_NO {%d}"
              " STATUS {%s}"
           "}%s",
           nBoardNo, nDevTmp, nDataCh,
           ((IFX_SUCCESS == ioctlDTMFRet) && (IFX_SUCCESS == ioctlRTPRet)) ?
           "OK":"FAIL", COM_MESSAGE_ENDING_SEQUENCE);

   /* Send message */
   if (IFX_SUCCESS != COM_SendMessage(g_pITM_Ctrl->nComOutSocket,
                                      buf, TD_CONN_ID_ITM))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, send message error. "
               "(File: %s, line: %d)\n",__FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Everything well */
   if ((IFX_SUCCESS == ioctlDTMFRet) && (IFX_SUCCESS == ioctlRTPRet))
   {
      return IFX_SUCCESS;
   }
   else
   {
      return IFX_ERROR;
   }
}

/**
   Configure OOB settings through ITM. Recognize ITM message and executes
   action with proper arguments.

   \param pCtrl - pointer to Control Status structure
   \param pITM_MsgHead - pointer to ITM message structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_OOB_Configure(CTRL_STATUS_t* pCtrl,
                           ITM_MSG_HEADER_t* pITM_MsgHead)
{
   /* Local variables */
   IFX_uint32_t nKey, nValue;
   IFX_uint32_t i;
   BOARD_t* pBoard = IFX_NULL;
   /* OOB configure */
   IFX_TAPI_PKT_EV_OOB_DTMF_t eventParam = {0};
   IFX_TAPI_PKT_RTP_CFG_t rfcParam = {0};
   IFX_TAPI_PKT_EV_OOB_DTMF_t* pEventParam = &eventParam;
   IFX_TAPI_PKT_RTP_CFG_t* pRfcParam = &rfcParam;
   IFX_uint32_t nEventParam = 0;
   /* By default first board, dev, data ch */
   IFX_uint32_t nBoardNo = 0, nDataCh = 0, nDevTmp = 0;
   /* Flags */
   IFX_boolean_t fOneBoard = IFX_FALSE, fOneDevice = IFX_FALSE;
   IFX_boolean_t fOneDataCh = IFX_FALSE, fReturn = IFX_FALSE;
   /* All variables used in for's */
   /* Initialized with 0 in case of TD_COM_OOB_CFG_ALL usage */
   IFX_uint16_t nBoardMin = 0, nBoardsMax;
   IFX_uint16_t nDevicesMin = 0, nDevicesMax;
   IFX_uint16_t nDataChMin = 0, nDataChMax;

   /* Check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   /* Set structure with default values */
   rfcParam.nSeqNr = TD_DEFAULT_PKT_RTP_CFG_SEQ_NR;
   rfcParam.nSsrc = TD_DEFAULT_PKT_RTP_CFG_SSRC;
   rfcParam.nEventPT = TD_DEFAULT_PKT_RTP_OOB_PAYLOAD;
   rfcParam.nTimestamp = TD_DEFAULT_PKT_RTP_CFG_TIME_STAMP;
   rfcParam.nEventPlayPT = TD_DEFAULT_PKT_RTP_CFG_EVENT_PLAY;

   /* For all repetitions */
   for (i = 0; i < pITM_MsgHead->nRepetition; i++)
   {
      /* Get key value */
      if (IFX_SUCCESS !=  COM_GetMsgData(pITM_MsgHead, &nKey, i,
                                         ITM_MSG_DATA_TYPE_KEY))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get key for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* Get value value */
      if (IFX_SUCCESS !=  COM_GetMsgData(pITM_MsgHead, &nValue, i,
                                         ITM_MSG_DATA_TYPE_VALUE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Check key type */
      switch (nKey)
      {
      case OOB_CFG_BOARD_NO:
         {
            if (TD_COM_OOB_CFG_ALL != nValue)
            {
               /* Set for one board only */
               if (pCtrl->nBoardCnt > nValue)
               {
                  nBoardMin = nValue;
                  fOneBoard = IFX_TRUE;
                  pBoard = &pCtrl->rgoBoards[nBoardNo];
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, board number %d is out of range."
                         "(File: %s, line: %d)\n",
                         nValue,__FILE__, __LINE__));
                  return IFX_ERROR;
               }
            }
            break;
         }
      case OOB_CFG_DEVICE_NO:
         {
            if (TD_COM_OOB_CFG_ALL != nValue)
            {
               /* Set for one device only */
               nDevicesMin = nValue;
               fOneDevice = IFX_TRUE;
            }
            break;
         }
      case OOB_CFG_DATA_CH_NO:
         {
            if (TD_COM_OOB_CFG_ALL != nValue)
            {
               /* Set for one channel only */
               nDataChMin = nValue;
               fOneDataCh = IFX_TRUE;
            }
            break;
         }
      case OOB_CFG_EVENT:
         {
            switch(nValue)
            {
               case 0:
                  nEventParam = IFX_TAPI_PKT_EV_OOB_DEFAULT;
                  break;
               case 1:
                  nEventParam = IFX_TAPI_PKT_EV_OOB_NO;
                  break;
               case 2:
                  nEventParam = IFX_TAPI_PKT_EV_OOB_ONLY;
                  break;
               case 3:
                  nEventParam = IFX_TAPI_PKT_EV_OOB_ALL;
                  break;
               case 4:
                  nEventParam = IFX_TAPI_PKT_EV_OOB_BLOCK;
                  break;
               case TD_COM_OOB_CFG_DEFAULT:
                  nEventParam = oVoipCfg.oRtpConf.nOobEvents;
                  break;
               case TD_COM_OOB_CFG_DO_NOT_CHANGE:
                  pEventParam = IFX_NULL;
                  break;
               default:
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, ITM: Invalid value %d for key %d (%d). "
                      "(File: %s, line: %d)\n",
                      nValue, nKey, i, __FILE__, __LINE__));
                  break;
            }
            break;
         }
      case OOB_CFG_RFC:
         {
            switch(nValue)
            {
               case 0:
                  rfcParam.nPlayEvents = IFX_TAPI_PKT_EV_OOBPLAY_DEFAULT;
                  break;
               case 1:
                  rfcParam.nPlayEvents = IFX_TAPI_PKT_EV_OOBPLAY_PLAY;
                  break;
               case 2:
                  rfcParam.nPlayEvents = IFX_TAPI_PKT_EV_OOBPLAY_MUTE;
                  break;
               case 3:
                  rfcParam.nPlayEvents = IFX_TAPI_PKT_EV_OOBPLAY_APT_PLAY;
                  break;
               case TD_COM_OOB_CFG_DEFAULT:
                  rfcParam.nPlayEvents = oVoipCfg.oRtpConf.nOobPlayEvents;
                  break;
               case TD_COM_OOB_CFG_DO_NOT_CHANGE:
                  pRfcParam = IFX_NULL;
                  break;
               default:
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                       ("Err, ITM: Invalid value %d for key %d (%d). "
                        "(File: %s, line: %d)\n",
                        nValue, nKey, i, __FILE__, __LINE__));
                  break;
            }
            break;
         }
      default:
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid key %d in rep %d. (File: %s, line: %d)\n",
                 nKey, i, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      } /* Switch (nKey) */
   } /* For all repetitions */
   /* Tapi 4 needed */
#ifndef TAPI_VERSION4
   eventParam = nEventParam;
#else
   eventParam.nTransmitMode = nEventParam;
#endif

   /* Set board execution */
   if (IFX_TRUE == fOneBoard)
   {
      /* Set conditions, if one board have to be configured */
      nBoardsMax = nBoardMin + 1;
   }
   else
   {
      /* For all boards */
      nBoardsMax = pCtrl->nBoardCnt;
      pBoard = &pCtrl->rgoBoards[0];
   }

   /* Set configuration for ioctl execution */
   for (nBoardNo = nBoardMin; nBoardNo < nBoardsMax; nBoardNo++, pBoard++)
   {
      if (IFX_TRUE == fOneDevice)
      {
         /* Set one device execution */
         if (nDevicesMin < pBoard->nChipCnt)
         {
            /* If device exist on board, execute */
            nDevicesMax = nDevicesMin + 1;
         }
         else
         {
            /* If passed device number is higher, do nothing */
            nDevicesMax = nDevicesMin;
         }
      }
      else
      {
         /* For all devices */
         nDevicesMax = pBoard->nChipCnt;
      }
      /* For all dev */
      for (nDevTmp = nDevicesMin; nDevTmp < nDevicesMax; nDevTmp++)
      {
         if (IFX_TRUE == fOneDataCh)
         {
            /* Set one data channel execution */
            if (nDataChMin < pBoard->nMaxCoderCh)
            {
               /* If data channel exist, execute */
               nDataChMax = nDataChMin + 1;
            }
            else
            {
               /* If passed channel number is higher, do nothing */
               nDataChMax = nDataChMin;
            }
         }
         else
         {
            /* For all data channels */
#ifndef TAPI_VERSION4
            nDataChMax = pBoard->nMaxCoderCh;
#else
            nDataChMax = pBoard->nDataChOnDev;
#endif
            if (0 == pBoard->nMaxCoderCh)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                       ("Warning, no data channel on board %d.\n", nBoardNo));
            }
         }
         /* For all data ch */
         for (nDataCh = nDataChMin; nDataCh < nDataChMax; nDataCh++)
         {
            /* Print settings */
            /* Only once */
            if (nDataCh == nDataChMin)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                   ("For board %s%d, dev %s%d, ch %s%d\n",
                    ((IFX_TRUE != fOneBoard)?"ALL: ":""),
                    ((IFX_TRUE != fOneBoard)?(-1):nBoardMin),
                    ((IFX_TRUE != fOneDevice)?"ALL: ":""),
                    ((IFX_TRUE != fOneDevice)?(-1):nDevicesMin),
                    ((IFX_TRUE != fOneDataCh)?"ALL: ":""),
                    ((IFX_TRUE != fOneDataCh)?(-1):nDataChMin)));

               if (IFX_NULL != pRfcParam)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                      ("Setting play events to: %s\n",
                      Common_Enum2Name(pRfcParam->nPlayEvents,
                                       TD_rgTapiOOB_EventPlayDesc)));
               }
               if (IFX_NULL != pEventParam)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                      ("Setting transmit mode to: %s\n",
                      Common_Enum2Name(nEventParam, TD_rgTapiOOB_EventDesc)));
               }
            }
            /* Execute ioctls with given configuration */
            if (IFX_SUCCESS != COM_OOB_Configure_Execute(pBoard,
                nBoardNo, nDevTmp, nDataCh,
                pEventParam, pRfcParam))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                       ("Err, configure execution fail.\n"
                        " One or more ioctl might failed.\n"
                        " Board %d, Dev %d, ch %d, (File: %s, line: %d)\n",
                        nBoardNo, nDevTmp, nDataCh, __FILE__, __LINE__));
            }
            else
            {
               /* Execution successfully ended */
               fReturn = IFX_SUCCESS;
            }
         } /* For all data ch */
      }/* For all dev */
   }/* Set configuration for ioctl execution */

   /* If something went wrong */
   if (IFX_ERROR == fReturn)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, configure execution returned error.\n"
               "No configuration was set successfully.\n"
               "(File: %s, line: %d)\n",__FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* Everything went well */
   return IFX_SUCCESS;
}

/**
   Check, if certain LEC configuration is supported

   \param pCtrl - pointer to Control Status structure
   \param pITM_MsgHead - pointer to ITM message structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_LEC_Check(CTRL_STATUS_t* pCtrl,
                           ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_uint32_t nKey, nValue;
   IFX_uint32_t i;
   IFX_return_t ioctlRet;
   IFX_char_t buf[MAX_CMD_LEN];

   /** LEC settings */
   IFX_TAPI_WLEC_CFG_t oLEC_Settings;
   IFX_uint32_t nBoardNo = 0;
   IFX_int32_t nFD;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   memset(&oLEC_Settings, 0, sizeof (IFX_TAPI_WLEC_CFG_t));
   /* by default disable all */
   oLEC_Settings.nType = IFX_TAPI_WLEC_TYPE_OFF;
   oLEC_Settings.bNlp = IFX_TAPI_WLEC_NLP_OFF;
   /* when set to 0 defaul value is used */
   oLEC_Settings.nNBNEwindow = 0;
   oLEC_Settings.nNBFEwindow = 0;
   oLEC_Settings.nWBNEwindow = 0;

   /* for all repetitions */
   for (i = 0; i < pITM_MsgHead->nRepetition; i++)
   {
      /* get key value */
      if (IFX_SUCCESS !=  COM_GetMsgData(pITM_MsgHead, &nKey, i,
                                         ITM_MSG_DATA_TYPE_KEY))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get key for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get value value */
      if (IFX_SUCCESS !=  COM_GetMsgData(pITM_MsgHead, &nValue, i,
                                         ITM_MSG_DATA_TYPE_VALUE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* check key type */
      switch (nKey)
      {
      case LEC_CHECK_BOARD_NO:
         {
            nBoardNo = nValue;
            break;
         }
      case LEC_CHECK_DEVICE_NO:
         {
            nDevTmp = nValue;
            break;
         }
      case LEC_CHECK_SETTING:
         {
            /* check value for LEC */
            switch (nValue)
            {
            case 0:
               oLEC_Settings.nType = IFX_TAPI_WLEC_TYPE_OFF;
               break;
            case 1:
               oLEC_Settings.nType = IFX_TAPI_WLEC_TYPE_NE;
               break;
            case 2:
               oLEC_Settings.nType = IFX_TAPI_WLEC_TYPE_NFE;
               break;
            case 3:
               oLEC_Settings.nType = IFX_TAPI_WLEC_TYPE_NE_ES;
               break;
            case 4:
               oLEC_Settings.nType = IFX_TAPI_WLEC_TYPE_NFE_ES;
               break;
            case 5:
               oLEC_Settings.nType = IFX_TAPI_WLEC_TYPE_ES;
               break;
            default:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, ITM: Invalid value %d for key %d (%d). "
                      "(File: %s, line: %d)\n",
                      nValue, nKey, i, __FILE__, __LINE__));
               break;
            } /* switch (nValue) */
            break;
         }
         default:
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Invalid key %d in rep %d. (File: %s, line: %d)\n",
                   nKey, i, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      } /* switch (nKey) */
   } /* for (i = 0; i < pITM_MsgHead->nRepetition; i++) */

   /* print info about received data */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
         ("ITM: Received LEC: %s to check on board %d, dev %d\n",
          Common_Enum2Name(oLEC_Settings.nType, TD_rgLEC_Type),
          nBoardNo, nDevTmp));

   if (pCtrl->nBoardCnt <= nBoardNo)
   {
      ioctlRet = IFX_ERROR;
   }
   else
   {
#ifdef EASY336
      oLEC_Settings.dev = nDevTmp;
      oLEC_Settings.ch = 0;
      nFD = pCtrl->rgoBoards[nBoardNo].nDevCtrl_FD;
#else
      nFD = pCtrl->rgoBoards[nBoardNo].rgoPhones[0].nPhoneCh_FD;
#endif
      ioctlRet = TD_IOCTL(nFD,  IFX_TAPI_WLEC_PHONE_CFG_SET,
                          (IFX_int32_t) &oLEC_Settings,
                          nDevTmp, TD_CONN_ID_ITM);
   }

   sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME CHECK_LEC_SUPPORT } "
           "{COMMAND_RESPONSE %s} }%s",
           IFX_SUCCESS == ioctlRet ? "OK" : "FAIL",
           COM_MESSAGE_ENDING_SEQUENCE);
   COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);

   /* all went well */
   return IFX_SUCCESS;
} /* COM_LEC_Check */

/**
   Set new communication ports numbers

   \param pCtrl - pointer to Control Status structure
   \param pITM_MsgHead - pointer to ITM message structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_SetPortNumber(CTRL_STATUS_t* pCtrl,
                               ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_uint32_t nKey, nValue;
   IFX_uint32_t i;
   IFX_int32_t nComInPort = 0, nComOutPort = 0;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   /* for all repetitions */
   for (i = 0; i < pITM_MsgHead->nRepetition; i++)
   {
      /* get key value */
      if (IFX_SUCCESS !=  COM_GetMsgData(pITM_MsgHead, &nKey, i,
                                         ITM_MSG_DATA_TYPE_KEY))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get key for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get value value */
      if (IFX_SUCCESS !=  COM_GetMsgData(pITM_MsgHead, &nValue, i,
                                         ITM_MSG_DATA_TYPE_VALUE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check key type */
      switch (nKey)
      {
      case ITM_SET_PORT_NUMBER_IN:
         /* set incoming port number */
         nComInPort = nValue;
         break;
      case ITM_SET_PORT_NUMBER_OUT:
         /* set ougoing port number */
         nComOutPort = nValue;
         break;
      default:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid key %d in rep %d. (File: %s, line: %d)\n",
                nKey, i, __FILE__, __LINE__));
         return IFX_ERROR;
         break;
      } /* switch (nKey) */
   } /* for (i = 0; i < pITM_MsgHead->nRepetition; i++) */

   /* check if values are set */
   if ((0 >= nComInPort) || (MAX_PORT_NUM < nComInPort) ||
       (0 >= nComOutPort) || (MAX_PORT_NUM < nComOutPort))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Set Port: In %d, Out %d, at least one field is nvalid."
             " (File: %s, line: %d)\n",
             nComInPort, nComOutPort, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* save received port numbers */
   g_pITM_Ctrl->nComInPort = nComInPort;
   g_pITM_Ctrl->nComOutPort = nComOutPort;
   /* print info about received data */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
         ("ITM: Setting port numbers: %d IN, %d OUT.\n",
          g_pITM_Ctrl->nComInPort, g_pITM_Ctrl->nComOutPort));
   /* reset communication */
   g_pITM_Ctrl->bCommunicationReset = IFX_TRUE;
   /* all went well */
   return IFX_SUCCESS;
}

/**
   Send list of compiled in devices' support

   \param none

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_RcvFileSendChipList(IFX_void_t)
{
   IFX_char_t buf[TD_COM_SEND_MSG_SIZE];
   IFX_char_t bufTmp[TD_COM_SEND_MSG_SIZE];

   /* set message text */
   sprintf(bufTmp, "SUPPORTED_DEVS:{ ");
   strcpy(buf, bufTmp);

#ifdef HAVE_DRV_VINETIC_HEADERS
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_VINETIC, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* HAVE_DRV_VINETIC_HEADERS */

#ifdef HAVE_DRV_VMMC_HEADERS
#ifdef DANUBE
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_DANUBE, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* DANUBE */
#ifdef AR9
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_AR9, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* AR9 */
#ifdef TD_XWAY_XRX300
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_XWAY_XRX300, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* TD_XWAY_XRX300 */
#ifdef VINAX
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_VINAX, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* VINAX */
#ifdef VR9
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_VR9, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* VR9 */
#endif /* HAVE_DRV_VMMC_HEADERS */

#ifdef HAVE_DRV_DUSLICXT_HEADERS
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_DUSLIC_XT, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* HAVE_DRV_DUSLICXT_HEADERS */

#ifdef HAVE_DRV_SVIP_HEADERS
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_SVIP, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* HAVE_DRV_SVIP_HEADERS */

#ifdef HAVE_DRV_VXT_HEADERS
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_XT16, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* HAVE_DRV_VXT_HEADERS */

#ifdef FXO

#ifdef HAVE_DRV_TERIDIAN_HEADERS
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_TERIDIAN, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* HAVE_DRV_TERIDIAN_HEADERS */

#ifdef DUSLIC_FXO
   sprintf(bufTmp, " %s ", Common_Enum2Name(TYPE_DUSLIC, TD_rgBoardTypeName));
   strcat(buf, bufTmp);
#endif /* DUSLIC_FXO */

#endif /* FXO */

   sprintf(bufTmp, " }%s", COM_MESSAGE_ENDING_SEQUENCE);
   strcat(buf, bufTmp);

   /* send message */
   return COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
}

/**
   Get file structure for given key. Allocate memory if this is new file.

   \param pKey    - file (FW, BBD...) info

   \return pointer to file structure or IFX_NULL if file not available
*/
ITM_RCV_FILE_DATA_LIST_t * COM_RcvFileStructGet(ITM_RCV_FILE_KEY_t *pKey)
{
   ITM_RCV_FILE_DATA_LIST_t *pFile, *pFileListTmp;

   /* check input parameters */
   TD_PTR_CHECK(pKey, IFX_NULL);

   /* get file structure for key */
   pFile = pFileList;
   /* for all elements in list */
   while (IFX_NULL != pFile)
   {
      /* check if data chunk for this structure */
      if (pFile->nChipType == pKey->nChipType &&
          pFile->nFileType == pKey->nFileType)
      {
         /* this is the searched structure */
         return pFile;
      }
      /* check next */
      pFile = pFile->pNext;
   }

   /* structure not found so allocate memory for new structure */
   pFile = TD_OS_MemCalloc(1, sizeof(ITM_RCV_FILE_DATA_LIST_t));
   if (IFX_NULL == pFile)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Allocate memory for file structure failed. "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_NULL;
   }
   /* initialize file structure */
   pFile->nNextPacketNum = 0;
   pFile->nOverallSize = pKey->nOverallSize;
   pFile->nReceivedData = 0;
   pFile->nFileType = pKey->nFileType;
   pFile->nChipType = pKey->nChipType;
   pFile->nFileUID = pKey->nFileUID;
   pFile->pNext = IFX_NULL;

   /* allocate memory for data */
   pFile->pData = TD_OS_MemCalloc(1, sizeof(IFX_char_t) * pFile->nOverallSize);
   if (IFX_NULL == pFile->pData)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Allocate memory for file data failed. "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      TD_OS_MemFree(pFile);
      return IFX_NULL;
   }

   /* save pointer to allocated memory */
   if (IFX_NULL == pFileList)
   {
      pFileList = pFile;
   }
   else
   {
      /* search for last element in list */
      pFileListTmp = pFileList;
      while (IFX_NULL != pFileListTmp->pNext)
      {
         pFileListTmp = pFileListTmp->pNext;
      }
      /* save pointer to newly allocated memory */
      pFileListTmp->pNext = pFile;
   }
   /* return pointer to allocated memory */
   return pFile;
}

/**
   Load BBD for vmmc device.

   \param pBoard  - board where file should be loaded
   \param pCurrentFile - BBD file data

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_RcvFileLoadBbbdVmmc(BOARD_t *pBoard,
                                     ITM_RCV_FILE_DATA_LIST_t *pCurrentFile)
{
#ifdef HAVE_DRV_VMMC_HEADERS
   VMMC_DWLD_t oBBD_Vmmc = {0};
#ifdef SLIC121_FXO
   IFX_int32_t i;
#endif /* SLIC121_FXO */

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCurrentFile, IFX_ERROR);

TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
      ("ITM: DEBUG COM_RcvFileLoadBbbdVmmc\n"));
   /* set BBD file */
   oBBD_Vmmc.buf = (IFX_uint8_t*)pCurrentFile->pData;
   oBBD_Vmmc.size = pCurrentFile->nOverallSize;

   /* load for FXS channels */
   if (pCurrentFile->nFileType == ITM_RCV_FILE_BBD ||
       pCurrentFile->nFileType == ITM_RCV_FILE_BBD_FXS)
   {
TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
      ("ITM: DEBUG BBD FXS\n"));
      /* load BBD file to device */
      if (IFX_SUCCESS != TD_IOCTL(pBoard->nDevCtrl_FD, FIO_BBD_DOWNLOAD,
                            &oBBD_Vmmc, TD_DEV_NOT_SET, TD_CONN_ID_ITM))
      {
         return IFX_ERROR;
      } /* load BBD file to device */
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
               ("ITM: %s loaded BBD for FXS channels.\n",
                 pBoard->pszBoardName));
      } /* load BBD file to device */
   }/* load for FXS channels */
#ifdef SLIC121_FXO
   /* load for FXO channels */
   if (ITM_RCV_FILE_BBD == pCurrentFile->nFileType ||
       ITM_RCV_FILE_BBD_FXO == pCurrentFile->nFileType)
   {
      /* if BBD file is available */
      if (0 < pBoard->nMaxFxo)
      {
         for (i = pBoard->nMaxAnalogCh;
              i < (pBoard->nMaxAnalogCh + pBoard->nMaxFxo);
              i++)
         {
            if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i], FIO_BBD_DOWNLOAD,
                                  &oBBD_Vmmc, TD_DEV_NOT_SET, TD_CONN_ID_ITM))
            {
               return IFX_ERROR;
            }
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                     ("ITM: %s loaded BBD for FXO channel %d.\n",
                      pBoard->pszBoardName, i));
            }
         }
      }
      else if (ITM_RCV_FILE_BBD_FXO == pCurrentFile->nFileType)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
               ("Warning, ITM: %s no FXO channel to load BBD.\n",
                pBoard->pszBoardName));
      }
   }/* load for FXO channels */
#endif /* SLIC121_FXO */
   return IFX_SUCCESS;
#else /* HAVE_DRV_VMMC_HEADERS */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
         ("Err, ITM: Compiled without VMMC device support. "
          "(File: %s, line: %d)\n",
          __FILE__, __LINE__));
   return IFX_ERROR;
#endif /* HAVE_DRV_VMMC_HEADERS */
}

/**
   Load BBD for svip device.

   \param pBoard  - board where file should be loaded
   \param pCurrentFile - BBD file data

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_RcvFileLoadBbbdSvip(BOARD_t *pBoard,
                                     ITM_RCV_FILE_DATA_LIST_t *pCurrentFile)
{
#ifdef HAVE_DRV_SVIP_HEADERS
   SVIP_IO_BBD_t oBBD_Svip = {0};
#ifdef TAPI_VERSION4
   IFX_int32_t i;
#endif /* TAPI_VERSION4 */
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCurrentFile, IFX_ERROR);

   /* load for FXS channels */
   if (pCurrentFile->nFileType != ITM_RCV_FILE_BBD &&
       pCurrentFile->nFileType != ITM_RCV_FILE_BBD_FXS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, Invalid file type %d. (File: %s, line: %d)\n",
             pCurrentFile->nFileType, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set BBD file */
   /* set BBD structure - bBroadcast = IFX_TRUE to download BBD for
      all channels */
   oBBD_Svip.bBroadcast = IFX_TRUE;
   oBBD_Svip.buf = (IFX_uint8_t*)pCurrentFile->pData;
   oBBD_Svip.size = pCurrentFile->nOverallSize;

#ifdef TAPI_VERSION4
   /* reset TAPI for each device */
   for (i = 0; i < pBoard->nChipCnt; ++i)
#endif /* TAPI_VERSION4 */
   {
#ifdef TAPI_VERSION4
      oBBD_Svip.dev = i;
      nDevTmp = oBBD_Svip.dev;
#endif /* TAPI_VERSION4 */
      /* download BBD */
      if (IFX_SUCCESS != TD_IOCTL(pBoard->nDevCtrl_FD, FIO_SVIP_BBD_DOWNLOAD,
                            &oBBD_Svip, nDevTmp, TD_CONN_ID_ITM))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Failed to download BBD. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return IFX_SUCCESS;
#else /* HAVE_DRV_SVIP_HEADERS */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
         ("Err, ITM: Compiled without SVIP device support. "
          "(File: %s, line: %d)\n",
          __FILE__, __LINE__));
   return IFX_ERROR;
#endif /* HAVE_DRV_SVIP_HEADERS */
}

/**
   Load BBD for vxt device.

   \param pBoard  - board where file should be loaded
   \param pCurrentFile - BBD file data

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_RcvFileLoadBbbdVxt(BOARD_t *pBoard,
                                    ITM_RCV_FILE_DATA_LIST_t *pCurrentFile)
{
#ifdef HAVE_DRV_VXT_HEADERS
   VXT_IO_BBD_t oBBD_Vxt = {0};
#ifdef TAPI_VERSION4
   IFX_int32_t i;
#endif /* TAPI_VERSION4 */
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCurrentFile, IFX_ERROR);

   /* load for FXS channels */
   if (pCurrentFile->nFileType != ITM_RCV_FILE_BBD &&
       pCurrentFile->nFileType != ITM_RCV_FILE_BBD_FXS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, Invalid file type %d. (File: %s, line: %d)\n",
             pCurrentFile->nFileType, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set BBD file */
   /* set BBD structure - bBroadcast = IFX_TRUE to download BBD for
      all channels */
   oBBD_Vxt.bBroadcast = IFX_TRUE;
   oBBD_Vxt.buf = (IFX_uint8_t*)pCurrentFile->pData;
   oBBD_Vxt.size = pCurrentFile->nOverallSize;

#ifdef TAPI_VERSION4
   /* reset TAPI for each device */
   for (i = 0; i < pBoard->nChipCnt; ++i)
#endif /* TAPI_VERSION4 */
   {
#ifdef TAPI_VERSION4
      oBBD_Vxt.dev = i;
      nDevTmp = oBBD_Vxt.dev;
#endif /* TAPI_VERSION4 */
      /* download BBD */
      if (IFX_SUCCESS != TD_IOCTL(pBoard->nDevCtrl_FD, FIO_VXT_BBD_DOWNLOAD,
                            &oBBD_Vxt, nDevTmp, TD_CONN_ID_ITM))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Failed to download BBD. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return IFX_SUCCESS;
#else /* HAVE_DRV_VXT_HEADERS */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
         ("Err, ITM: Compiled without xT-16 device support. "
          "(File: %s, line: %d)\n",
          __FILE__, __LINE__));
   return IFX_ERROR;
#endif /* HAVE_DRV_VXT_HEADERS */
}

/**
   Load received file.

   \param pCtrl - pointer to Control Status structure
   \param pCurrentFile - file data

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_RcvFileLoad(CTRL_STATUS_t* pCtrl,
                             ITM_RCV_FILE_DATA_LIST_t *pCurrentFile)
{
   IFX_int32_t nBoard;
   BOARD_t *pBoard;
   IFX_boolean_t nFileLoaded = IFX_FALSE;

   /* Load BBD if not verify system initialization */
   if (IFX_TRUE != g_pITM_Ctrl->oVerifySytemInit.fEnabled)
   {
      /* only BBD file */
      if (pCurrentFile->nFileType == ITM_RCV_FILE_BBD ||
          pCurrentFile->nFileType == ITM_RCV_FILE_BBD_FXS ||
          pCurrentFile->nFileType == ITM_RCV_FILE_BBD_FXO)
      {
         /* for all boards */
         for (nBoard = 0; nBoard < pCtrl->nBoardCnt; nBoard++)
         {
            /* get pointer */
            pBoard = &pCtrl->rgoBoards[nBoard];
            /* if the same chip type */
            if (pBoard->nType == pCurrentFile->nChipType)
            {
               switch (pCurrentFile->nChipType)
               {
               case TYPE_DANUBE:
               case TYPE_VINAX:
               case TYPE_AR9:
               case TYPE_XWAY_XRX300:
               case TYPE_VR9:
                  if (IFX_SUCCESS != COM_RcvFileLoadBbbdVmmc(pBoard,
                                                             pCurrentFile))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                           ("Err, ITM: COM_RcvFileLoadBbbdVmmc failed. "
                            "(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  else
                  {
                     nFileLoaded = IFX_TRUE;
                  }
                  break;
               case TYPE_DUSLIC_XT:
                  break;
               case TYPE_SVIP:
                  if (IFX_SUCCESS != COM_RcvFileLoadBbbdSvip(pBoard,
                                                             pCurrentFile))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                           ("Err, ITM: COM_RcvFileLoadBbbdVmmc failed. "
                            "(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  else
                  {
                     nFileLoaded = IFX_TRUE;
                  }
                  break;
               case TYPE_XT16:
                  if (IFX_SUCCESS != COM_RcvFileLoadBbbdVxt(pBoard,
                                                            pCurrentFile))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                           ("Err, ITM: COM_RcvFileLoadBbbdVmmc failed. "
                            "(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  else
                  {
                     nFileLoaded = IFX_TRUE;
                  }
                  break;
               case TYPE_VINETIC:
                  break;
               case TYPE_TERIDIAN:
                  break;
               case TYPE_DUSLIC:
                  break;
               default:
                  break;
               }
            }
         } /* for all boards */
         if (IFX_TRUE != nFileLoaded)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Warning, ITM: File %s for %s was not loaded. Device not "
                   "present or BBD load procedure not implemented."
                   "(File: %s, line: %d)\n",
                   Common_Enum2Name(pCurrentFile->nFileType, TD_rgFileTypeName),
                   Common_Enum2Name(pCurrentFile->nChipType, TD_rgBoardTypeName),
                   __FILE__, __LINE__));

         }
      } /* only BBD file */
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Warning, ITM: File type %d will not be loaded after init. "
                "(File: %s, line: %d)\n",
                pCurrentFile->nFileType, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }/* Load BBD if not verify system initialization */
   return IFX_SUCCESS;
}

/**
   Get file according to type of chip and purpose (FXS, FXO..).

   \param nChipType  - chip type (dev e.g. danube, vr9...)
   \param nFileType  - FXS, FXO..
   \param ppData     - pointer to file binary data - output
   \param pnDataLen  - pointer to file binary size - output

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_boolean_t COM_RcvFileGet(BOARD_TYPE_t const nChipType,
                            ITM_RCV_FILE_TYPE_t const nFileType,
                            IFX_uint8_t **ppData,
                            IFX_size_t *pnDataLen)
{
   ITM_RCV_FILE_DATA_LIST_t *pFileListTmp = pFileList;

   TD_PTR_CHECK(ppData, IFX_ERROR);
   TD_PTR_CHECK(pnDataLen, IFX_ERROR);

   /* for all files in list */
   while (IFX_NULL != pFileListTmp)
   {
      /* check dev type */
      if (nChipType == pFileListTmp->nChipType)
      {
         /* check if full file received */
         if (pFileListTmp->nOverallSize == pFileListTmp->nReceivedData)
         {
            *ppData = (IFX_uint8_t *) pFileListTmp->pData;
            *pnDataLen = (IFX_size_t)pFileListTmp->nOverallSize;
            /* check file type,
               if BBD without extension, then it is common for FXS and FXO */
            if (ITM_RCV_FILE_BBD == nFileType &&
                (ITM_RCV_FILE_BBD_FXS == pFileListTmp->nFileType ||
                 ITM_RCV_FILE_BBD_FXO == pFileListTmp->nFileType))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                     ("ITM: For %s using downloaded %s file. \n",
                      Common_Enum2Name(pFileListTmp->nFileType, TD_rgFileTypeName),
                      Common_Enum2Name(pFileListTmp->nChipType, TD_rgBoardTypeName)));
               return IFX_SUCCESS;
            }
            else if (nFileType == pFileListTmp->nFileType)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                     ("ITM: For %s using downloaded %s file. \n",
                      Common_Enum2Name(pFileListTmp->nFileType, TD_rgFileTypeName),
                      Common_Enum2Name(pFileListTmp->nChipType, TD_rgBoardTypeName)));
               return IFX_SUCCESS;
            }
         } /* check if full file received */
      } /* check dev type */
      /* go to next */
      pFileListTmp = pFileListTmp->pNext;
   }
   /* reset output */
   *ppData = IFX_NULL;
   *pnDataLen = 0;
   return IFX_SUCCESS;
}

/**
   Check if binary data allocated in list of files.

   \param pData      - pointer to file binary data

   \return IFX_TRUE if data not received from control PC and can be freed,
           otherwise IFX_FALSE
*/
IFX_boolean_t COM_RcvFileCheckIfRelease(IFX_uint8_t *pData)
{
   ITM_RCV_FILE_DATA_LIST_t *pFileListTmp = pFileList;

   TD_PTR_CHECK(pData, IFX_TRUE);

   while (IFX_NULL != pFileListTmp)
   {
      if ((IFX_char_t*)pData == pFileListTmp->pData)
      {
         return IFX_FALSE;
      }
      pFileListTmp = pFileListTmp->pNext;
   }
   return IFX_TRUE;
}

/**
   Prepare board to Verify system initialization test.

   \param pCtrl - pointer to control structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t COM_RcvFileOnStartUp(CTRL_STATUS_t* pCtrl)
{
   /* Informs that function COM_PrepareVerifySystemInit was already called */
   static IFX_boolean_t nPrepareFileDwnldDone = IFX_FALSE;
   /* check if this function was already called (and ended with success) */
   if (IFX_TRUE == nPrepareFileDwnldDone)
   {
      /* function already called (and ended with success) */
      return IFX_SUCCESS;
   }
   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComInSocket),
                      g_pITM_Ctrl->nComInSocket, IFX_ERROR);

   /* Waiting for control PC */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
        ("TAPIDEMO is waiting on control PC\n"));
   /* First message from control PC is a broadcast message. */
   g_pITM_Ctrl->oVerifySytemInit.nExpectedID = COM_ID_BROADCAST_MSG;
   if (IFX_ERROR == COM_MessageWait(pCtrl))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, COM_MessageWait() failed "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
       return IFX_ERROR;
   }
   if(g_pITM_Ctrl->oVerifySytemInit.nExpectedID !=
      g_pITM_Ctrl->oVerifySytemInit.nReceivedID)
   {
       TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, Received command other than expected. "
             "Expected 0x%x, Received 0x%x "
             "(File: %s, line: %d)\n",
             g_pITM_Ctrl->oVerifySytemInit.nExpectedID,
             g_pITM_Ctrl->oVerifySytemInit.nReceivedID,
             __FILE__, __LINE__));
       return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
         ("TAPIDEMO is waiting on message \"COM_ID_COM_RESET\" "
          "from control PC\n"));
   g_pITM_Ctrl->oVerifySytemInit.nExpectedID = COM_ID_COM_RESET;
   do
   {
      if (IFX_ERROR == COM_MessageWait(pCtrl))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, COM_MessageWait() failed "
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } while(g_pITM_Ctrl->oVerifySytemInit.nExpectedID !=
           g_pITM_Ctrl->oVerifySytemInit.nReceivedID);

   /* save information that this function was called */
   nPrepareFileDwnldDone = IFX_TRUE;

   return IFX_SUCCESS;
}

/**
   Handle received file message - binary files receiving.

   \param pCtrl - pointer to Control Status structure
   \param pITM_MsgHead - pointer to ITM message structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_RcvFileHandle(CTRL_STATUS_t* pCtrl,
                               ITM_MSG_HEADER_t* pITM_MsgHead)
{
   ITM_RCV_FILE_DATA_LIST_t *pCurrentFile = IFX_NULL;
   ITM_RCV_FILE_KEY_t *pKey;
   IFX_char_t *pDataChunk, *pDataDst;
   IFX_int32_t i, j, nCheckSum;
   IFX_char_t buf[MAX_CMD_LEN];
   IFX_boolean_t bSkip = IFX_FALSE;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);

   /* for all repetitions */
   for (i = 0; i < pITM_MsgHead->nRepetition; i++)
   {
      /* get key value */
      pKey = (ITM_RCV_FILE_KEY_t*) COM_GetMsgDataPointer(pITM_MsgHead, i,
                                                         ITM_MSG_DATA_TYPE_KEY);
      if (IFX_NULL == pKey)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get key for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* validate chip type */
      if (pKey->nChipType >= TYPE_NONE)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid chip type %d for rep %d. "
                "(File: %s, line: %d)\n",
                pKey->nChipType, i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* validate file type */
      if (pKey->nFileType == ITM_RCV_FILE_NONE ||
          pKey->nFileType >= ITM_RCV_FILE_MAX)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid file type %d for rep %d. "
                "(File: %s, line: %d)\n",
                pKey->nFileType, i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get value's value - binary data */
      pDataChunk =  COM_GetMsgDataPointer(pITM_MsgHead, i,
                                          ITM_MSG_DATA_TYPE_VALUE);
      if (IFX_NULL == pDataChunk)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value for rep = %d. "
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         return IFX_ERROR;
      } /* get value value */
      if (0 == pKey->nPacketNumber)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
               ("ITM: First packet for file %s, type %s, size %d, id %d received.\n",
                Common_Enum2Name(pKey->nChipType, TD_rgBoardTypeName),
                Common_Enum2Name(pKey->nFileType, TD_rgFileTypeName),
                pKey->nOverallSize, pKey->nFileUID));
      }
      /* get file structure for received data */
      pCurrentFile = COM_RcvFileStructGet(pKey);
      /* check if structure was found */
      if (IFX_NULL == pCurrentFile)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get structure for chip %d, type %d, rep %d. "
                "(File: %s, line: %d)\n",
                pKey->nChipType, pKey->nFileType, i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check packet number */
      if (pCurrentFile->nNextPacketNum != pKey->nPacketNumber)
      {
         /* check if sending new data for existing file structure */
         if (0 == pKey->nPacketNumber &&
             0 != pCurrentFile->nReceivedData)
         {
            if (pCurrentFile->nFileUID == pKey->nFileUID)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                     ("ITM: File already downloaded %s, type %s, size %d, uid %d.\n",
                      Common_Enum2Name(pCurrentFile->nChipType, TD_rgBoardTypeName),
                      Common_Enum2Name(pCurrentFile->nFileType, TD_rgFileTypeName),
                      pCurrentFile->nOverallSize, pCurrentFile->nFileUID));
               /* this file is already downloaded */
               bSkip = IFX_TRUE;
               break;
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                  ("ITM: Replacing existing file %s, type %s, size %d.\n",
                   Common_Enum2Name(pCurrentFile->nChipType, TD_rgBoardTypeName),
                   Common_Enum2Name(pCurrentFile->nFileType, TD_rgFileTypeName),
                   pCurrentFile->nOverallSize));
            /* init structure */
            pCurrentFile->nNextPacketNum = 0;
            pCurrentFile->nOverallSize = pKey->nOverallSize;
            pCurrentFile->nReceivedData = 0;
            pCurrentFile->nFileUID = pKey->nFileUID;
            if (IFX_NULL != pCurrentFile->pData)
            {
               TD_OS_MemFree(pCurrentFile->pData);
            }
            pCurrentFile->pData = TD_OS_MemCalloc(1,
                                     sizeof(IFX_char_t) * pKey->nOverallSize);
            if (IFX_NULL == pCurrentFile->pData)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, ITM: Memm alloc failed for file %d, type %d, rep %d."
                      "(File: %s, line: %d)\n",
                      pKey->nChipType, pKey->nFileType, i, __FILE__, __LINE__));
               return IFX_ERROR;
            }
         } /* check if sending new data for existing file structure */
         else
         {
            /* invalid packet number */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Invalid packet number %d "
                   "for file %d, type %d, rep %d. (File: %s, line: %d)\n",
                   pKey->nPacketNumber, pKey->nChipType, pKey->nFileType,
                    i, __FILE__, __LINE__));
            return IFX_ERROR;
         }/* check if sending new data for existing file structure */
      } /* check packet number */
      /* check overall size */
      if (pCurrentFile->nOverallSize != pKey->nOverallSize)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Size doesn't mach for key and stuct %d!=%d, rep %d. "
                "(File: %s, line: %d)\n",
                pCurrentFile->nOverallSize, pKey->nOverallSize, i,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check file ID */
      if (pCurrentFile->nFileUID != pKey->nFileUID)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid file ID %d!=%d, rep %d. "
                "(File: %s, line: %d)\n",
                pCurrentFile->nFileUID, pKey->nFileUID, i,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* do a check sum for this data chunk */
      nCheckSum = 0;
      for (j=0; j < pKey->nPacketSize; j++)
      {
         nCheckSum += pDataChunk[j];
      }
      nCheckSum = nCheckSum & 0xFF;
      if (nCheckSum != pKey->nCheckSum)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Check Sum failed for chip %d, type %d, rep %d. "
                "(File: %s, line: %d)\n",
                pKey->nChipType, pKey->nFileType, i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check data pointer */
      if (IFX_NULL == pCurrentFile->pData)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: No data structure for chip %d, type %d, rep %d. "
                "(File: %s, line: %d)\n",
                pKey->nChipType, pKey->nFileType, i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get pointer to memory where to write this data chunk */
      pDataDst = pCurrentFile->pData;
      pDataDst += pCurrentFile->nReceivedData;
      /* data size after write */
      pCurrentFile->nReceivedData += pKey->nPacketSize;
      if (pCurrentFile->nReceivedData > pCurrentFile->nOverallSize)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Too much data for chip %d, type %d, rep %d. "
                "(File: %s, line: %d)\n",
                pKey->nChipType, pKey->nFileType, i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      memcpy(pDataDst, pDataChunk, pKey->nPacketSize);
      pCurrentFile->nNextPacketNum++;
   } /* for (i = 0; i < pITM_MsgHead->nRepetition; i++) */

   sprintf(buf, ITM_RCV_FILE_RESPONSE_STRING,
           "OK", COM_MESSAGE_ENDING_SEQUENCE);
   COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);

   if (IFX_NULL != pCurrentFile)
   {
      if (IFX_TRUE != bSkip)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("ITM: Receiving binary data for %s, type %s, progress: %d/%d\n",
                Common_Enum2Name(pCurrentFile->nChipType, TD_rgBoardTypeName),
                Common_Enum2Name(pCurrentFile->nFileType, TD_rgFileTypeName),
                pCurrentFile->nReceivedData, pCurrentFile->nOverallSize));
      }
      /* check if all data was send */
      if (pCurrentFile->nOverallSize == pCurrentFile->nReceivedData ||
          IFX_TRUE == bSkip)
      {
         if (IFX_TRUE != bSkip)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                  ("ITM: Received file %s, type %s, size %d.\n",
                   Common_Enum2Name(pCurrentFile->nChipType, TD_rgBoardTypeName),
                   Common_Enum2Name(pCurrentFile->nFileType, TD_rgFileTypeName),
                   pCurrentFile->nOverallSize));
            COM_RcvFileLoad(pCtrl, pCurrentFile);
         }
         sprintf(buf, 
                 "SEND_BIN_DATA_END:{ "
                    " FILE_TYPE {%s} "
                    " CHIP_TYPE {%s} "
                    " FILE_SIZE {%d} "
                    " CHUNK_COUNT {%d} "
                    " FUID {%d} "
                    " STATUS {%s} "
                 "}%s",
                 Common_Enum2Name(pCurrentFile->nFileType, TD_rgFileTypeName),
                 Common_Enum2Name(pCurrentFile->nChipType, TD_rgBoardTypeName),
                 pCurrentFile->nOverallSize, pCurrentFile->nNextPacketNum,
                 pCurrentFile->nFileUID, (IFX_TRUE == bSkip) ? "SKIP" : "DONE" ,
                 COM_MESSAGE_ENDING_SEQUENCE);
         COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
      }
   }

   /* all went well */
   return IFX_SUCCESS;
}

/**
   Clean up received files.

   \param none

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_void_t COM_RcvFileCleanUp(IFX_void_t)
{
   ITM_RCV_FILE_DATA_LIST_t *pFileListTmp;

   while (IFX_NULL != pFileList)
   {
      pFileListTmp = pFileList->pNext;
      if (IFX_NULL != pFileList->pData)
      {
         TD_OS_MemFree(pFileList->pData);
      }
      TD_OS_MemFree(pFileList);
      pFileList = pFileListTmp;
   }
}

/**
   Send info about used codec.

   \param pPhone - phone that uses this codec
   \param nDataCh - data channel number
   \param nCodec - used codec
   \param nPacketization - used codec's packetization
   \param bDecoder - IFX_TRUE if decoderm, IFX_FALSE if encoder

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_SendInfoUsedCodec(PHONE_t* pPhone, IFX_int32_t nDataCh,
                                   IFX_TAPI_ENC_TYPE_t nCodec,
                                   IFX_TAPI_COD_LENGTH_t nPacketization,
                                   IFX_boolean_t bDecoder)
{
   IFX_char_t buf[TD_COM_SEND_MSG_SIZE];

   TD_PTR_CHECK(pPhone, IFX_ERROR);

   /* set message text */
   sprintf(buf,
           "INFO_USED_CODEC:{ "
              " PHONE_NO {%d} "
              " DATA_CH {%d} "
              " DIRECTION {%sCODER} "
              " TYPE_NAME {%s} "
              " TYPE_NO {%d} "
              " PACKETIZATION {%s} "
           "}%s",
           pPhone->nPhoneNumber, nDataCh,
           (IFX_TRUE == bDecoder) ? "DE" : "EN",
           pCodecName[nCodec], nCodec, oFrameLen[nPacketization],
           COM_MESSAGE_ENDING_SEQUENCE);

   /* send message */
   return COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, pPhone->nSeqConnId);
}

/**
   Parse command recieved from communication socket and perform concrete action

   \param pCtrl  - pointer to Control Status structure
   \param pITM_MsgHead - pointer to ITM msg header data structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
static IFX_return_t COM_ParseCommand(CTRL_STATUS_t* pCtrl,
                                     ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_int32_t i, ch;
   IFX_int32_t ret = IFX_ERROR;
   IFX_char_t buf[3072];
   IFX_char_t tbuf[256];
   IFX_uint32_t nKey = 0, nValue = 0;
   IFX_int32_t nDev, nDataChCount, nAnalogChCount;
   IFX_boolean_t bSendResponse = IFX_TRUE;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead->pData, IFX_ERROR);

/* Received command should match the following format:

+====+============+=========+=========+=========+=========+=====+
| id | repetition | key_len | val_len | Nth_key | Nth_val | EOM |
+====+============+=========+=========+=========+=========+=====+
| 1B |     2B     |   1B    |   1B    | key_len | val_len | 1B  |
+----+------------+---------+---------+---------+---------+-----+
*/

   /* Perform action suitable for requested command*/
   switch (pITM_MsgHead->nId)
   {
   /* Set test type to Modular or Bulk.
      Key: not used.
      Value: test type.
   */
   case COM_ID_SET_TEST_TYPE:
      /* get value */
      ret = COM_GetMsgData(pITM_MsgHead, &nValue, 0,
                           ITM_MSG_DATA_TYPE_VALUE);
      if (IFX_SUCCESS == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("Rec. Command Id: SET_TEST_TYPE, "));
         /* check first byte after end of ITM message header */
         switch (nValue)
         {
            /* set modular test type */
            case 0x00:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                     ("Modular\n"));
               g_pITM_Ctrl->nTestType = COM_MODULAR_TEST;
               g_pITM_Ctrl->nSubTestType = COM_SUBTEST_NONE;
               ret = IFX_SUCCESS;
               break;

            /* set bulk test type */
            case 0x01:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                     ("Bulk\n"));
               g_pITM_Ctrl->nTestType = COM_BULK_CALL_TEST;
               g_pITM_Ctrl->nSubTestType = COM_SUBTEST_NONE;
               ret = IFX_SUCCESS;
               break;
            default:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Unknown type!!! - Error\n"));
               ret = IFX_ERROR;
               break;
         }
      }
      sprintf(buf, "SUTS_RESPONSE:{{COMMAND_NAME SET_TEST_TYPE} "
              "{COMMAND_RESPONSE %s} }%s", IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_SET_TRACE_LEVEL:
   /* Set tapidemo trace level.
      Key: not used.
      Value: trace level.
   */
      /* get value */
      ret = COM_GetMsgData(pITM_MsgHead, &nValue, 0,
                           ITM_MSG_DATA_TYPE_VALUE);
      if (IFX_SUCCESS == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("Rec. Command Id: SET_TRACE_LEVEL, "));
         /* check first byte after end of ITM message header */
         switch (nValue)
         {
            case 0x04:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("OFF\n"));
               g_pITM_Ctrl->nComDbgLevel = DBG_LEVEL_OFF;
               ret = IFX_SUCCESS;
               break;
            case 0x03:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("HIGH\n"));
               g_pITM_Ctrl->nComDbgLevel = DBG_LEVEL_HIGH;
               ret = IFX_SUCCESS;
               break;
            case 0x02:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("NORMAL\n"));
               g_pITM_Ctrl->nComDbgLevel = DBG_LEVEL_NORMAL;
               ret = IFX_SUCCESS;
               break;
            case 0x01:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("LOW\n"));
               g_pITM_Ctrl->nComDbgLevel = DBG_LEVEL_LOW;
               ret = IFX_SUCCESS;
               break;
            default:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                    ("Unknown!!! Error\n"));
               ret = IFX_ERROR;
               break;
         }
      }
      sprintf(buf, "SUTS_RESPONSE:{{COMMAND_NAME SET_TRACE_LEVEL} "
              "{COMMAND_RESPONSE %s} }%s", IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_SET_PHONES:
   /* Change phone numbers.
      Key: channel number.
      Value: new phone number for channel from coresponding key value.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: SET_PHONES \n"));
      sprintf(buf, "SUTS_RESPONSE:{{COMMAND_NAME SET_PHONES} "
              "{COMMAND_RESPONSE %s} }%s", IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_GET_PHONES:
   /* Send to tcl script phone numbers for all phone channels.
      Key: not used.
      Value: not used.
   */
      /* get phones configuration */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: GET_PHONES \n"));
      strcpy(buf, "CONFIG_PHONES:{");
      for (i = 0; i < pCtrl->nBoardCnt; ++i)
      {
         if (0 == pCtrl->rgoBoards[i].nChipCnt)
         {
            /* Board not present */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                   ("Board no. %d is not present\n", i));
            /* skip to next board */
            continue;
         }

         sprintf(tbuf,"{{BOARD_NUMBER %d} {", (int)i);
         strcat(buf,tbuf);
         for (ch = 0; ch < pCtrl->rgoBoards[i].nMaxAnalogCh; ++ch)
         {
            sprintf(tbuf, "{{DEV_NO %d} "
                          "{CHANNEL_NO %d} "
                          "{PHONE_NO %d} } ",
                    (int)pCtrl->rgoBoards[i].rgoPhones[ch].nDev,
                    (int)pCtrl->rgoBoards[i].rgoPhones[ch].nPhoneCh,
                    (int)pCtrl->rgoBoards[i].rgoPhones[ch].nPhoneNumber );
            strcat(buf, tbuf);
         }
         strcat(buf," } } ");
      }

      sprintf(tbuf, " }%s", COM_MESSAGE_ENDING_SEQUENCE);
      strcat(buf, tbuf);
      break;
   case COM_ID_SET_SUT_COUNTRY_SETTINGS:
   /* Change BBD file used by board.
      Key: number of board (BoardIDX).
      Value: name of BBD file.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: SET_SUT_COUNTRY_SETTINGS \n"));
#ifdef CHANGE_COUNTRY_SETTING_FINISHED
      ret = COM_BBD_ChangeCountrySettings(pCtrl, pITM_MsgHead);
#else
      ret = IFX_SUCCESS;
#endif /* CHANGE_COUNTRY_SETTING_FINISHED */
      sprintf(buf, "SUTS_RESPONSE:{{COMMAND_NAME SET_SUT_COUNTRY_SETTINGS} "
              "{COMMAND_RESPONSE %s} }%s", IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_SET_SUT_CPT_COUNTRY_SETTINGS:
   /* Change call progress tones parameters.
      Key: number of tone and parameter.
      Value: parameter value.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: SET_SUT_CPT_COUNTRY_SETTINGS \n"));
      /* change country setting */
      if (IFX_SUCCESS == COM_CPT_ChangeCountrySettings(pCtrl, pITM_MsgHead))
      {
         /* use country call progress tone setting  */
         g_pITM_Ctrl->nUseCustomCpt = IFX_TRUE;
         ret = IFX_SUCCESS;
      }
      sprintf(buf, "SUTS_RESPONSE:{{COMMAND_NAME SET_CPT_COUNTRY_SETTINGS} "
              "{COMMAND_RESPONSE %s} }%s", IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_GET_SUT_COUNTRY_SETTINGS:
   /* Change BBD file used by boardGet country settings.
      Key: not used.
      Value: not used.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: GET_SUT_COUNTRY_SETTINGS \n"));
      sprintf(buf, "CONFIG_COUNTRY_SPECIFIC:{{COUNTRY_CODE  USA} }%s",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_COM_RESET:
   /* Disconect with communication server.
      Key: not used.
      Value: not used.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: COM_RESET \n"));
      g_pITM_Ctrl->oVerifySytemInit.nReceivedID = COM_ID_COM_RESET;
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
             ("Closing communication socket.\n"));
      TD_OS_SocketClose(g_pITM_Ctrl->nComOutSocket);
      g_pITM_Ctrl->nComOutSocket = NO_SOCKET;

      if (COM_NO_TEST != g_pITM_Ctrl->nTestType)
      {
         COM_TestEnd(pCtrl);
      }
      return IFX_SUCCESS;
   case COM_ID_CONFIG_CHANNELS:
   /* Get number of analog and data channels.
      Key: not used.
      Value: not used.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: CONFIG_CHANNELS \n"));
      g_pITM_Ctrl->oVerifySytemInit.nReceivedID = COM_ID_CONFIG_CHANNELS;

      strcpy(buf, "CONFIG_CHANNELS:{");

      /* for each board */
      for (i = 0; i < pCtrl->nBoardCnt; ++i)
      {
         if (0 == pCtrl->rgoBoards[i].nChipCnt)
         {
            /* Board not present */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                   ("Board no. %d is not present\n", i));
            /* skip to next board */
            continue;
         }

         /* add board number */
         sprintf(tbuf,"{{BOARD_NUMBER %d} ", (IFX_int32_t)i);
         strcat(buf, tbuf);
         /* add board type */
         sprintf(tbuf,"{BOARD_TYPE %d} ", (IFX_int32_t)pCtrl->rgoBoards[i].nType);
         strcat(buf, tbuf);
         /* add board type name */
         sprintf(tbuf,"{BOARD_TYPE_NAME {%s}} {",
                 Common_Enum2Name(pCtrl->rgoBoards[i].nType, TD_rgBoardTypeName));
         strcat(buf, tbuf);

         /* for each device on board */
         for (nDev = 0; nDev < pCtrl->rgoBoards[i].nChipCnt; ++nDev)
         {
            /* **************************************
            * get number of data channels on board *
            ***************************************/
            nDataChCount = COM_GetCapabilityValue( &(pCtrl->rgoBoards[i]),
                                                   nDev,
                                                   IFX_TAPI_CAP_TYPE_CODECS);

            /* ****************************************
            * get number of analog channels on board *
            *****************************************/
            nAnalogChCount = COM_GetCapabilityValue( &(pCtrl->rgoBoards[i]),
                                                     nDev,
                                                     IFX_TAPI_CAP_TYPE_PHONES);
            sprintf(tbuf,"{DEV_NO %d ANALOG_CH %d DATA_CH %d} ",
                 nDev, nAnalogChCount, nDataChCount);
            strcat(buf, tbuf);

         } /* for (nDev = 0; nDev < pCtrl->rgoBoards[i].nChipCnt; ++nDev) */
         strcat(buf, "}} ");
      } /* for (i = 0; i < pCtrl->nBoardCnt; ++i) */
      sprintf(tbuf, "}%s", COM_MESSAGE_ENDING_SEQUENCE);
      strcat(buf,tbuf);

      break;
   case COM_ID_SET_CH_INIT_VOICE:
   /* Verify initialization message - execute IFX_TAPI_CH_INIT.
      Key: not used.
      Value: number of data channel.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: SET_CH_INIT_VOICE \n"));

      /* TAPIDEMO initializes voice channel */
      g_pITM_Ctrl->oVerifySytemInit.nReceivedID = COM_ID_SET_CH_INIT_VOICE;

      ret = COM_ExtractDevAndChannelFromMsg(pITM_MsgHead);

      if ( IFX_ERROR == ret )
      {
         sprintf(buf,
                 "SUTS_RESPONSE:{{COMMAND_NAME SET_CH_INIT_VOICE} "
                 "{COMMAND_RESPONSE FAIL} }%s",
                 COM_MESSAGE_ENDING_SEQUENCE);
      }

      break;
   case COM_ID_SET_LINE_FEED_SET_STANDBY:
   /* Verify initialization message - execute IFX_TAPI_LINE_FEED_SET with value
      standby.
      Key: not used.
      Value: number of phone channel.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: SET_LINE_FEED_SET_STANDBY \n"));

      /* Set-up Line Feeding Modes in standby */
      g_pITM_Ctrl->oVerifySytemInit.nReceivedID =
         COM_ID_SET_LINE_FEED_SET_STANDBY;

      ret = COM_ExtractDevAndChannelFromMsg(pITM_MsgHead);

      if ( IFX_ERROR == ret )
      {
         sprintf(buf,
                 "SUTS_RESPONSE:"
                 "{{COMMAND_NAME SET_LINE_FEED_SET_STANDBY} "
                 "{COMMAND_RESPONSE FAIL} }%s",
                 COM_MESSAGE_ENDING_SEQUENCE);
      }

      break;
   case COM_ID_SET_LINE_FEED_SET_ACTIVE:
   /* Verify initialization message - execute IFX_TAPI_LINE_FEED_SET with value
      active.
      Key: not used.
      Value: number of phone channel.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: SET_LINE_FEED_SET_ACTIVE \n"));

      /* Set-up Line Feeding Modes in active */
      g_pITM_Ctrl->oVerifySytemInit.nReceivedID =
         COM_ID_SET_LINE_FEED_SET_ACTIVE;

      ret = COM_ExtractDevAndChannelFromMsg(pITM_MsgHead);

      if ( IFX_ERROR == ret )
      {
         sprintf(buf,
                 "SUTS_RESPONSE:"
                 "{{COMMAND_NAME SET_LINE_FEED_SET_ACTIVE} "
                 "{COMMAND_RESPONSE FAIL} }%s",
                 COM_MESSAGE_ENDING_SEQUENCE);
      }

      break;
   case COM_ID_COM_CHANGE_CODEC:
   /* Change codec.
      Key: not used.
      Value: number of codec.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: COM_CHANGE_CODEC \n"));
    /* get key - board index */
/*    if (IFX_SUCCESS != COM_GetMsgData(pITM_MsgHead, &nKey, 0,
                                      ITM_MSG_DATA_TYPE_KEY))
    {
       nKey = 0;
    }*/
      /* get value - number of DTMF code */
      if (IFX_SUCCESS != COM_GetMsgData(pITM_MsgHead, &nValue, 0,
                                        ITM_MSG_DATA_TYPE_VALUE))
      {
         nValue = 0;
      }

      /* nValue is number of codec DTMF code */
      if (nValue >= 1000 && nValue <= 1999)
      {
         /* DTMF code represents codec */
         ret = IFX_SUCCESS;
         /* change codec for all boards */
         for (i = 0; i < pCtrl->nBoardCnt; ++i)
         {
            if (0 == pCtrl->rgoBoards[i].nChipCnt)
            {
               /* Board not present */
               /* skip to next board */
               continue;
            }
            /* check if data channels are available */
            if (0 < pCtrl->rgoBoards[i].nMaxCoderCh)
            {
               if (IFX_SUCCESS != Common_ConfigureTAPI(pCtrl,
                                     &pCtrl->rgoBoards[i],
                                     pCtrl->rgoBoards[i].rgoPhones,
                                     nValue, -1, IFX_TRUE, IFX_FALSE,
                                     TD_CONN_ID_ITM))
               {
                  ret = IFX_ERROR;
                  break;
               }
               else
               {
                  ret = IFX_SUCCESS;
               }
            } /* check if data channels are available */
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
                    ("ITM INFO: Unable to set codec for %s,"
                     " it has no coder channels\n",
                     pCtrl->rgoBoards[i].pszBoardName));
            } /* check if data channels are available */
         }
         return ret;
      }
      else
      {
         /* DTMF code represents feature other than codec
          * Message COM_ID_COM_CHANGE_CODEC should only allow to change codec
          * (not try to active other features) - pass -1 to function
          */
         return Common_ConfigureTAPI(pCtrl, &pCtrl->rgoBoards[0],
                                     pCtrl->rgoBoards[0].rgoPhones,
                                     -1, -1, IFX_FALSE, IFX_FALSE,
                                     TD_CONN_ID_ITM);
      }
      break;
   case COM_ID_GENERATE_TONES:
   /* Generate tone on specified channel.
      Key: number of tone.
      Value: number of tone.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: GENERATE_TONES \n"));
      ret = COM_GenerateTones(pCtrl, pITM_MsgHead);
      sprintf(buf, "SUTS_RESPONSE:{{COMMAND_NAME GENERATE_TONES} "
              "{COMMAND_RESPONSE %s} }%s", IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_SET_INTERNAL_SUT_CHANNEL_CONNECTION:
   /* Map phone channel to data channel.
      Key: data channel number.
      Value: phone channel number.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: SET_INTERNAL_SUT_CHANNEL_CONNECTION \n"));
      ret = COM_ChannelMappingChange(pCtrl, pITM_MsgHead);
      sprintf(buf, "SUTS_RESPONSE:{{COMMAND_NAME "
              "SET_INTERNAL_SUT_CHANNEL_CONNECTION} {COMMAND_RESPONSE %s} }%s",
              IFX_SUCCESS == ret ? "OK" : "FAIL",\
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_SET_MODULAR_TEST_TYPE:
   /* Set modular test type.
      Key: test number.
      Value: start stop test.
   */
      {
         IFX_uint32_t nTestChoice = COM_SUBTEST_NONE;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("Rec. Command Id: SET_MODULAR_TEST_TYPE, "));
         /* get key, for this test number of modular test */
         if (IFX_SUCCESS == COM_GetMsgData(pITM_MsgHead, &nKey, 0,
                                           ITM_MSG_DATA_TYPE_KEY))
         {
            /* get value, for this test start/stop flag */
            if (IFX_SUCCESS != COM_GetMsgData(pITM_MsgHead, &nValue, 0,
                                              ITM_MSG_DATA_TYPE_VALUE))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, ITM: Failed to get value value."
                      " (File: %s, line: %d)\n", __FILE__, __LINE__));
               break;
            }
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Failed to get key value."
                   " (File: %s, line: %d)\n", __FILE__, __LINE__));
            break;
         }
         /* check first byte after end of ITM message header */
         switch (nKey)
         {
            case 0x01:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("HOOKSTATE \n"));
               nTestChoice = COM_SUBTEST_HOOKSTATE;
               ret = IFX_SUCCESS;
               break;
            case 0x02:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("HOOKFLASH \n"));
               nTestChoice = COM_SUBTEST_HOOKFLASH;
               ret = IFX_SUCCESS;
               break;
            case 0x03:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("PULSEDIGIT \n"));
               /* For this test dialtone must be played after hook-off. */
               nTestChoice = COM_SUBTEST_PULSEDIGIT;
               ret = IFX_SUCCESS;
               break;
            case 0x04:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("DTMFDIGIT \n"));
               /* For this test dialtone must be played after hook-off. */
               nTestChoice = COM_SUBTEST_DTMFDIGIT;
               ret = IFX_SUCCESS;
#ifdef EASY336
               if (COM_TEST_START == nValue)
               {
                  /* unmap all channels */
                  ret = SVIP_RM_DeInit(&pcm_cfg);
               }
               else if (COM_TEST_STOP == nValue)
               {
                  /* unmap custom mapping */
                  ret = COM_SvipCustomMapping(map_table, IFX_FALSE);
                  if(IFX_SUCCESS != ret)
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, ITM: failed to do custom unmapping,"
                         " board %d, dev %d. (File: %s, line: %d)\n",
                         (IFX_NULL != map_table.pBoard) ?
                         map_table.pBoard->nBoard_IDX : -1,
                         map_table.nDevice,
                         __FILE__, __LINE__));
                  }

                  ret = SVIP_RM_Init(pCtrl->rgoBoards[0].nDevCtrl_FD,
                                     pCtrl->rgoBoards[0].nChipCnt,
                                     pCtrl->rgoBoards[1].nDevCtrl_FD,
                                     pCtrl->rgoBoards[1].nChipCnt,
                                     &pcm_cfg);
               }
#endif /* EASY336 */
               break;
            case 0x05:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("Conference \n"));
               /* For this test info about HOOK_OFF, HOOK_ON
                  and BUSYTONE is printed. */
               nTestChoice = COM_SUBTEST_CONFERENCE;
               ret = IFX_SUCCESS;
               break;

            case 0x06:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("FXO \n"));
               /* For this test dialtone must be played after hook-off. */
               nTestChoice = COM_SUBTEST_FXO;
#ifdef FXO
               if (pCtrl->pProgramArg->oArgFlags.nFXO)
               {
                  ret = IFX_SUCCESS;
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, program started without FXO option "
                         "(check tapidemo help - ./tapidemo -h)."
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  ret = IFX_ERROR;
               }
#else /* FXO */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, program compiled without FXO support."
                      "(File: %s, line: %d)\n", __FILE__, __LINE__));
               ret = IFX_ERROR;
#endif /* FXO */
               break;
            case 0x07:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("FAX \n"));
#ifdef TD_FAX_MODEM
               /* traces about fax events are printed */
               nTestChoice = COM_SUBTEST_FAX;
               pCtrl->nFaxMode = TD_FAX_MODE_TRANSPARENT;
               ret = IFX_SUCCESS;
#else /* TD_FAX_MODEM */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, program compiled without fax/modem support."
                      "(File: %s, line: %d)\n", __FILE__, __LINE__));
               ret = IFX_ERROR;
#endif /* TD_FAX_MODEM */
               break;
            case 0x08:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("FAX T.38 \n"));
#ifdef HAVE_T38_IN_FW
               /* check if T.38 fax is avaible on this board */
               if (pCtrl->rgoBoards[0].nT38_Support)
               {
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38))
                  /* traces about fax events are printed */
                  nTestChoice = COM_SUBTEST_FAX_T_38;
                  pCtrl->nFaxMode = TD_FAX_MODE_DEAFAULT;
                  ret = IFX_SUCCESS;
#else /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38)) */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, program compiled without fax t.38 support."
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  ret = IFX_ERROR;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38)) */
               }
               else
#endif /* HAVE_T38_IN_FW */
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, board %s doesn't support T.38 fax transmission."
                         "(File: %s, line: %d)\n",
                         pCtrl->rgoBoards[0].pszBoardName, __FILE__, __LINE__));
                  /* FAX T.38 cannot be done for this board */
                  nTestChoice = COM_SUBTEST_NONE;
                  ret = IFX_ERROR;
               }
               break;
            case 0x09:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("MODEM \n"));
#ifdef TD_FAX_MODEM
               /* traces about modem events are printed */
               nTestChoice = COM_SUBTEST_MODEM;
               ret = IFX_SUCCESS;
#else /* TD_FAX_MODEM */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, program compiled without fax/modem support."
                      "(File: %s, line: %d)\n", __FILE__, __LINE__));
               ret = IFX_ERROR;
#endif /* TD_FAX_MODEM */
               break;
            case 0x0A:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                    ("SYSTEM INIT \n"));
               /* check if program is started with '-v' option */
               if (g_pITM_Ctrl->oVerifySytemInit.fEnabled)
               {
                  nTestChoice = COM_SUBTEST_SYSINIT;
                  ret = IFX_SUCCESS;
                  if (COM_TEST_STOP == nValue)
                  {
                     /* reset enable flag */
                     g_pITM_Ctrl->oVerifySytemInit.fEnabled = IFX_FALSE;
                  }
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, ITM: program started without 'initialize system' "
                         "option or initialization test has already ended"
                         "(check tapidemo help - ./tapidemo -h). "
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                  nTestChoice = COM_SUBTEST_NONE;
                  ret = IFX_ERROR;
               } /* if (nVerifySystemInitializtion) */
               break;
            case 0x0B:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                     ("CID \n"));
               /* testing CID (caller ID) */
               nTestChoice = COM_SUBTEST_CID;
               ret = IFX_SUCCESS;
               COM_CID_TestInit(pCtrl, nValue);
               break;
            case 0x0C:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                     ("LEC \n"));
#ifndef TD_FAX_MODEM
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, ITM: LEC test doesn't work without "
                      "fax/modem support. "
                      "(File: %s, line: %d)\n", __FILE__, __LINE__));
               nTestChoice = COM_SUBTEST_NONE;
               ret = IFX_ERROR;
#else /* TD_FAX_MODEM */
               /* testing LEC (Line Echo Canceler) */
               nTestChoice = COM_SUBTEST_LEC;
               ret = IFX_SUCCESS;
#endif /* TD_FAX_MODEM */
               break;
            case 0x0D:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                     ("ABACUS CONFIG DETECTION \n"));
               /* testing LEC (Line Echo Canceler) */
               nTestChoice = COM_SUBTEST_CONFIG_DETECTION;
               ret = IFX_SUCCESS;
               break;
            default:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("- Err, Unknown test type %d. (File: %s, line: %d)\n",
                      nKey, __FILE__, __LINE__));
               ret = IFX_ERROR;
               break;
         } /* switch (nKey) */
         if (COM_TEST_START == nValue)
         {
            if (COM_SUBTEST_NONE != g_pITM_Ctrl->nSubTestType)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Warning, more than one subtest is set (0x%08X)\n",
                      g_pITM_Ctrl->nSubTestType));
            }
            g_pITM_Ctrl->nSubTestType |= nTestChoice;
         }
         else if (COM_TEST_STOP == nValue)
         {
            g_pITM_Ctrl->nSubTestType &= ~nTestChoice;
         }
         else
         {
            ret = IFX_ERROR;
         }
         sprintf(buf, "SUTS_RESPONSE:{{COMMAND_NAME SET_MODULAR_TEST_TYPE} "
                 "{COMMAND_RESPONSE %s} }%s", IFX_SUCCESS == ret ? "OK" : "FAIL",
                 COM_MESSAGE_ENDING_SEQUENCE);
         break;
      }
   case COM_ID_RUN_PROGRAM:
   /* Run program.
      Key: type of value (increases by 1 from 0 for every key/value pair
           in message).
      Value: first value is responsible for when to send response(after or
             before evecution), second value is program name, all aditional
             values are program parameters.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: RUN_PROGRAM\n"));
      COM_RunCommand(pITM_MsgHead, buf);
      /* response was already send or it is not needed */
      bSendResponse = IFX_FALSE;
      break;
   case COM_ID_ANALOG_TO_IP_CALL_CTRL:
   /* Set analog to IP connection paramters.
      Key: type of resource.
      Value: connection parameters (port number, IP address, MAC address,
             channel number and start/stop flagh).
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
             ("Rec. Command Id: ANALOG_TO_IP_CALL_CTRL\n"));
      ret = COM_AnalogToIP_Call(pCtrl, pITM_MsgHead);
      sprintf(buf, "SUTS_RESPONSE:{{COMMAND_NAME ANALOG_TO_IP_CALL_CTRL} "
              "{COMMAND_RESPONSE %s} }%s", IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_REPEAT_SYSINIT_TEST:
   /* Verify initialization message - determine if test should be repeated.
      Key: not used.
      Value: 2 to repeat setting line feed, 1 to stop line feed testing.
             0 is invalid value.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: COM_ID_REPEAT_SYSINIT_TEST \n"));

      g_pITM_Ctrl->oVerifySytemInit.nReceivedID = COM_ID_REPEAT_SYSINIT_TEST;

      ret = IFX_ERROR;
      if (g_pITM_Ctrl->oVerifySytemInit.fEnabled)
      {
         /* get value */
         if (IFX_SUCCESS == COM_GetMsgData(pITM_MsgHead, &nValue, 0,
                                           ITM_MSG_DATA_TYPE_VALUE))
         {
            /* copy phone channel number */
            sprintf(g_pITM_Ctrl->oVerifySytemInit.aParametrs, "%u", nValue);
            ret = IFX_SUCCESS;
         }
      }

      if ( IFX_ERROR == ret )
      {
         sprintf(buf,
                 "SUTS_RESPONSE:"
                 "{{COMMAND_NAME REPEAT_SYSINIT_SCENARIO} "
                 "{COMMAND_RESPONSE FAIL} }%s",
                 COM_MESSAGE_ENDING_SEQUENCE);
      }

      break;
   case COM_ID_TAPI_RESTART:
   /* Verify initialization message - restart TAPI.
      Key: not used.
      Value: not used.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: COM_ID_TAPI_RESTART \n"));

      g_pITM_Ctrl->oVerifySytemInit.nReceivedID = COM_ID_TAPI_RESTART;

      ret = IFX_ERROR;
      if (g_pITM_Ctrl->oVerifySytemInit.fEnabled)
      {
         ret = IFX_SUCCESS;
      }

      if ( IFX_ERROR == ret )
      {
         sprintf(buf,
                 "SUTS_RESPONSE:"
                 "{{COMMAND_NAME TAPI_RESTART} "
                 "{COMMAND_RESPONSE FAIL} }%s",
                 COM_MESSAGE_ENDING_SEQUENCE);
      }

      break;
   case COM_ID_GET_SYSINIT_TYPE:
      /* Send to tcl script type of system initialization.
      Key: not used.
      Value: not used.
      */
      /* get phones configuration */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
           ("Rec. Command Id: GET_SYSINIT_TYPE \n"));

      g_pITM_Ctrl->oVerifySytemInit.nReceivedID = COM_ID_GET_SYSINIT_TYPE;

      sprintf(buf,
                 "SYSINIT_TYPE:"
                 "{{TYPE {"
#ifdef TD_USE_CHANNEL_INIT
                 "CH_INIT"
#else
                 "DEV_START"
#endif /* TD_USE_CHANNEL_INIT */
                 "}}}%s",
                 COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_ADD_ARP_ENTRY:
      /* Add ARP entry.
         Key: IP address.
         Value: MAC address.
      */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("Rec. Command Id: ADD_ARP_ENTRY\n"));
         ret = COM_AddArpEntry(pCtrl, pITM_MsgHead, buf);

         /*Prepare message to send*/
         sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME ADD_ARP_ENTRY} "
                 "{COMMAND_RESPONSE %s} }%s",
                 IFX_SUCCESS == ret ? "OK" : "FAIL",
                 COM_MESSAGE_ENDING_SEQUENCE);

         break;
   case COM_ID_PRINT_TXT:
   /* Print text.
      Key: not used.
      Value: text to print.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: PRINT_TXT\n"));
      ret = COM_PrintTxt(pCtrl, pITM_MsgHead);
      sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME PRINT_TXT} "
               "{COMMAND_RESPONSE %s} }%s",
              IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_SET_CID:
   /* Set CID standard.
      Key: not used.
      Value: CID standard number.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: SET_CID\n"));
      /* get value */
      if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nValue, 0,
                                      ITM_MSG_DATA_TYPE_VALUE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value."
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         ret = IFX_ERROR;
      }
      else
      {
         ret = COM_CID_StandardSet(pCtrl, nValue);
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: COM_CID_Set() failed."
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
         }
      }
      sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME SET_CID} "
               "{COMMAND_RESPONSE %s} }%s",
              IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_SET_CID_DATA:
   /* Set CALLER_ID to send for given phone.
      Key: data type (number, name ..).
      Value: data.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: SET_CID_DATA\n"));
      ret = COM_CID_DataGet(pCtrl, pITM_MsgHead);
      if (IFX_SUCCESS != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: COM_InfoGet() failed."
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
      }
      sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME SET_CID_DATA} "
               "{COMMAND_RESPONSE %s} }%s",
              IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_GET_INFO:
   /* Get information about phones and FXOs.
      Key: not used.
      Value: not used.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: GET_INFO\n"));
      if (IFX_SUCCESS != COM_InfoGet(pCtrl))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: COM_InfoGet failed."
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
      }
      /* response was already send or it is not needed */
      bSendResponse = IFX_FALSE;
      break;
   case COM_ID_SET_LEC:
   /* Change LEC settings.
      Key: 1 - for LEC settings, 2 - for NLP settings.
      Value: LEC or NLP setting.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: SET_LEC\n"));
      ret = IFX_SUCCESS;
      if (IFX_SUCCESS != COM_LEC_DataGet(pCtrl, pITM_MsgHead))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: COM_LEC_DataGet() failed. "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         ret = IFX_ERROR;
      }
      sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME SET_LEC} "
               "{COMMAND_RESPONSE %s} }%s",
              IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_CHECK_LEC:
   /* Check, if certain LEC configuration is supported.
      Key: 1 - for Board no., 2 - for Device no. , 3 - for LEC setting.
      Value: board no, device no, LEC setting.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: CHECK_LEC\n"));
      ret = IFX_SUCCESS;
      if (IFX_SUCCESS != COM_LEC_Check(pCtrl, pITM_MsgHead))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: COM_LEC_Check() failed. "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         ret = IFX_ERROR;

         sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME SET_LEC} "
                  "{COMMAND_RESPONSE %s} }%s",
                 IFX_SUCCESS == ret ? "OK" : "FAIL",
                 COM_MESSAGE_ENDING_SEQUENCE);
      }
      else
      {
         /* response was already send or it is not needed */
         bSendResponse = IFX_FALSE;
      }
      break;
   case COM_ID_SET_PORT_NUMBER:
   /* Change ports used for communication.
      Key: 1 - IN port, 2 - OUT port.
      Value: port number.
   */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Rec. Command Id: SET_PORT_NUMBER\n"));
      ret = IFX_SUCCESS;
      if (IFX_SUCCESS != COM_SetPortNumber(pCtrl, pITM_MsgHead))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: COM_SetPortNumber() failed. "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         ret = IFX_ERROR;
      }
      sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME SET_PORT_NUMBER} "
              "{COMMAND_RESPONSE %s} }%s",
              IFX_SUCCESS == ret ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_PING:
   /* PING from control PC. Control PC checks if TAPIDEMO is still alive.
      Key: none.
      Value: ping number.
   */
      if (IFX_SUCCESS != COM_GetMsgData(pITM_MsgHead, &nValue, 0,
                                        ITM_MSG_DATA_TYPE_VALUE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      sprintf(buf, "PING_REPLY:{ ID {%d} }%s",
              nValue, COM_MESSAGE_ENDING_SEQUENCE);
      break;
   case COM_ID_RCV_FILE:
   /* Receive file from control PC.
      Key: data describing data e.g. file type, check sum...
      Value: up to 255 data bytes of file.
   */
      if (IFX_SUCCESS != COM_RcvFileHandle(pCtrl, pITM_MsgHead))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         sprintf(buf, ITM_RCV_FILE_RESPONSE_STRING,
                 "FAIL", COM_MESSAGE_ENDING_SEQUENCE);
      }
      else
      {
         /* COM_RcvFileHandle takes care of sending response
            if everything is OK */
         bSendResponse = IFX_FALSE;
      }
      break;
   case COM_ID_CTRL_PROG_INIT:
   /* Control verify system initialization.
      Key: none.
      Value: 0 - skip verify test, 1 - do verify test.
   */
      if (IFX_SUCCESS != COM_GetMsgData(pITM_MsgHead, &nValue, 0,
                                        ITM_MSG_DATA_TYPE_VALUE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get value. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      g_pITM_Ctrl->oVerifySytemInit.nReceivedID = COM_ID_CTRL_PROG_INIT;
      sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME CTRL_PROG_INIT} "
              "{COMMAND_RESPONSE %s} }%s",
              g_pITM_Ctrl->oVerifySytemInit.fEnabled ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      /* disable sys verification test */
      if (0 == nValue)
      {
         g_pITM_Ctrl->oVerifySytemInit.fEnabled = IFX_FALSE;
      }
      /* check if valid value */
      else if (1 != nValue)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Invalid value %d. (File: %s, line: %d)\n",
                nValue, __FILE__, __LINE__));
      }
      /* file download finished */
      g_pITM_Ctrl->oVerifySytemInit.fFileDwnld = IFX_FALSE;
      break;
   case COM_ID_OOB_CFG:
     /*
      Configure OOB.
      Key: Size: 1,
      1 -Board, 2 - Device, 3 - Channel, 4 - Event trans. setting,
      5 - RFC 2833 packets playout setting.
      Value: Size: 4,
      */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
              ("Rec. Command Id: OOB_CFG\n"));
      ret = IFX_SUCCESS;
      if (IFX_SUCCESS != COM_OOB_Configure(pCtrl, pITM_MsgHead))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: COM_OOB_Configure() failed. "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         ret = IFX_ERROR;

         sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME OOB_CFG} "
                  "{COMMAND_RESPONSE %s} }%s",
                 IFX_SUCCESS == ret ? "OK" : "FAIL",
                 COM_MESSAGE_ENDING_SEQUENCE);
      }
      else
      {
         /* response was already send or it is not needed */
         bSendResponse = IFX_FALSE;
      }
      break;
   default:
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, Command ID (0x%02X) not supported. (File: %s, line: %d)\n",
             (IFX_int32_t) pITM_MsgHead->nId, __FILE__, __LINE__));
      return IFX_ERROR;
   } /* switch (pITM_MsgHead->nId) */

   /* for system initialization commands no message is sent */
   if ( (pITM_MsgHead->nId == COM_ID_SET_LINE_FEED_SET_STANDBY) ||
        (pITM_MsgHead->nId == COM_ID_SET_CH_INIT_VOICE) ||
        (pITM_MsgHead->nId == COM_ID_SET_LINE_FEED_SET_ACTIVE) ||
        (pITM_MsgHead->nId == COM_ID_REPEAT_SYSINIT_TEST) ||
        (pITM_MsgHead->nId == COM_ID_TAPI_RESTART) )
   {
      if ((g_pITM_Ctrl->oVerifySytemInit.fEnabled) && (IFX_SUCCESS==ret))
      {
         return IFX_SUCCESS;
      }
   }

   /* check if message should be sent, for COM_ID_RUN_PROGRAM message was sent
      in COM_RunCommand(), for COM_ID_GET_INFO is send multiple times
      in separate function */
   if (IFX_TRUE == bSendResponse)
   {
      return COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
   }
   else
   {
      return IFX_SUCCESS;
   }

} /* COM_ParseCommand() */

/**
   Check if nComOutSocket is available

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_ChecknComOutSocket(IFX_void_t)
{
   TD_OS_socFd_set_t rfd, wfd;
   TD_OS_sockAddr_t addr_from;
   static IFX_char_t buf[MAX_SIZE_OF_ITM_MESSAGE];
   IFX_int32_t ret = 0;

   TD_OS_SocFdZero(&rfd);
   TD_OS_SocFdZero(&wfd);
   TD_OS_SocFdSet(g_pITM_Ctrl->nComOutSocket, &rfd);
   TD_OS_SocFdSet(g_pITM_Ctrl->nComOutSocket, &wfd);

   /* wait for message from TCL script */
   TD_OS_SocketSelect(g_pITM_Ctrl->nComOutSocket + 1,
                      &rfd, &wfd, IFX_NULL, TD_OS_SOC_NO_WAIT);
   if (TD_OS_SocFdIsSet(g_pITM_Ctrl->nComOutSocket, &rfd) > 0)
   {
      /* Got data from socket */
      ret = TD_OS_SocketRecvFrom(g_pITM_Ctrl->nComOutSocket, buf, sizeof(buf),
                                 &addr_from);

       if(ret == 0)
       {
          /* No data was read. Can indicate that socket was closed */
           return IFX_ERROR;
       }
       else if (ret < 0)
       {
          /* Something happend to connection,
             further communication could crash tapidemo so socket is closed. */
          TD_OS_SocketClose(g_pITM_Ctrl->nComOutSocket);
          g_pITM_Ctrl->nComOutSocket = NO_SOCKET;
          TAPIDEMO_PRINTF(TD_CONN_ID_ITM,
             ("Err, ITM: recvfrom error %d - %s"
              "(File: %s, line: %d)\n",
              ret, strerror(errno), __FILE__, __LINE__));
          return IFX_ERROR;
       }
       else if (MAX_SIZE_OF_ITM_MESSAGE > ret)
       {
          TAPIDEMO_PRINTF(TD_CONN_ID_ITM,
             ("Err, ITM: Message size is too big - %d "
              "(File: %s, line: %d)\n", ret, __FILE__, __LINE__));
          return IFX_ERROR;
       }
   }

   return IFX_SUCCESS;
} /* COM_ChecknComOutSocket() */

/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   Receive message from communication socket

   \param pCtrl  - pointer to control structure.
   \param bBrCast - IFX_TRUE if message received on broadcast socket.

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_ReceiveCommand(CTRL_STATUS_t* pCtrl, IFX_boolean_t bBrCast)
{
   static IFX_char_t buf[MAX_SIZE_OF_ITM_MESSAGE];
   IFX_int32_t ret = 0;
   TD_OS_sockAddr_t addr_from;
   ITM_MSG_HEADER_t oITM_MsgHead;
   TD_OS_socket_t nUsedSocket;
   IFX_char_t acSendBuf[MAX_CMD_LEN];

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
          ("COM_ReceiveCommand()\n"));

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* get used socket */
   if (IFX_TRUE == bBrCast)
   {
      nUsedSocket = g_pITM_Ctrl->oBroadCast.nSocket;
      TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->oBroadCast.nSocket),
                         g_pITM_Ctrl->oBroadCast.nSocket, IFX_ERROR);
   }
   else
   {
      nUsedSocket = g_pITM_Ctrl->nComInSocket;
      TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComInSocket),
                         g_pITM_Ctrl->nComInSocket, IFX_ERROR);
   }

   /* Got data from socket */
   ret = TD_OS_SocketRecvFrom(nUsedSocket, buf, sizeof(buf), &addr_from);
   if (0 > ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, No data read from communication socket, %s. "
             "(File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else if (0 == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
            ("No data read from communication socket.\n"));
      return IFX_SUCCESS;
   }
   else if (MAX_SIZE_OF_ITM_MESSAGE < ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Received message is too long - %d. "
             "(File: %s, line: %d)\n", ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* get ITM msg header data and check whether size of received message fits in
      size declared in its content

      +====+============+=========+=========+=========+=========+=====+
      | id | repetition | key_len | val_len | Nth_key | Nth_val | EOM |
      +====+============+=========+=========+=========+=========+=====+
      | 1B |     2B     |   1B    |   1B    | key_len | val_len | 1B  |
      +----+------------+---------+---------+---------+---------+-----+
      */

   if (IFX_ERROR == COM_SetITM_MsgHeader(&oITM_MsgHead, buf, ret))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, Received wrong data %s. (File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* First byte determines the Command Id*/
   /* Establish new connection with automation server.
      Key: not used.
      Value: not used.
   */
   if (COM_ID_BROADCAST_MSG == oITM_MsgHead.nId)
   {
      /* check if received on broadcast socket */
      if (IFX_TRUE == bBrCast)
      {
         COM_BrCastResponseSend(pCtrl, &addr_from);
      }
      else
      {
         g_pITM_Ctrl->oVerifySytemInit.nReceivedID = COM_ID_BROADCAST_MSG;

         if (NO_SOCKET != g_pITM_Ctrl->nComOutSocket)
         {
            TD_OS_SocketClose(g_pITM_Ctrl->nComOutSocket);
            g_pITM_Ctrl->nComOutSocket = NO_SOCKET;
         }
         /* The address of broadcaster is needed to establish the connection
            so the Broadcast command has to be perfomed here instead of
            ParseCommand() function*/
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
               ("Creating communication socket with address %s\n",
                TD_GetStringIP(&addr_from)));
         COM_TestEnd(pCtrl);
         g_pITM_Ctrl->nComOutSocket =
            COM_InitCommunication(TD_SOCK_AddrIPv4Get(&addr_from),
                                  COM_DIRECTION_OUT);

         /** before messages can be sent to server timeout must occur */
         TD_OS_MSecSleep(ITM_COMUNICATION_TIMEOUT);

         if (IFX_TRUE == g_pITM_Ctrl->oVerifySytemInit.fFileDwnld)
         {

            sprintf(acSendBuf, "BCAST_REPLY:{ STATUS WAITING_FOR_FILES }%s",
                    COM_MESSAGE_ENDING_SEQUENCE);
            
            if (IFX_SUCCESS != COM_SendMessage(g_pITM_Ctrl->nComOutSocket,
                                               acSendBuf, TD_CONN_ID_ITM))
            {
               return IFX_ERROR;
            }
            COM_SendAppVersion(pCtrl);
            return COM_RcvFileSendChipList();
         }
         else
         {
            sprintf(acSendBuf, "BCAST_REPLY:{ STATUS %s }%s",
                    (IFX_FALSE == g_pITM_Ctrl->oVerifySytemInit.fEnabled) ?
                    "NORMAL" : "WAITING_FOR_SYSINIT",
                    COM_MESSAGE_ENDING_SEQUENCE);
            
            COM_SendMessage(g_pITM_Ctrl->nComOutSocket, acSendBuf,
                            TD_CONN_ID_ITM);
         }
         COM_SendAppVersion(pCtrl);
         COM_SendSutSW_VersionAndCappabilities(pCtrl);
         COM_SendSutSettings();
         COM_SendSutArgv();
         COM_SendSutInfo();
         /* check if correct path is used */
         if (IFX_TRUE != g_pITM_Ctrl->bNoProblemWithDwldPath)
         {
            COM_ReportErrorDwlPath();
         } /* check if correct path is used */
      }
      return IFX_SUCCESS;
   }
   else
   {
      if(g_pITM_Ctrl->nComOutSocket == NO_SOCKET)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, No connection with TCL server.\n"));
         return IFX_ERROR;
      }
   }

   return COM_ParseCommand(pCtrl, &oITM_MsgHead);
}/* COM_ReceiveCommand() */

/**
   Send trace message through communication socket

   \param level    - message debug level
   \param msg      - pointer to the message string

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_SendTraceMessage(IFX_int32_t level, IFX_char_t* msg)
{
   IFX_int32_t size = 0;
   IFX_int32_t ret;
   IFX_char_t buf[MAX_CMD_LEN];

   /* check input parameters */
   TD_PTR_CHECK(msg, IFX_ERROR);
   /* get message size */
   size = strlen(msg);
   TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComOutSocket),
                      g_pITM_Ctrl->nComOutSocket, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > size), size, IFX_ERROR);

   sprintf(buf, "TRACE_MESSAGE:{{%s {%s } } }%s",
           Common_Enum2Name(level, TD_rgDbgLevelName),
           msg, COM_MESSAGE_ENDING_SEQUENCE);

   size = strlen(buf);

   if(COM_ChecknComOutSocket() == IFX_SUCCESS)
   {
      ret = TD_OS_SocketSend(g_pITM_Ctrl->nComOutSocket, buf, size);
      if (ret != size)
      {
         TRACE_LOCAL(TAPIDEMO, DBG_LEVEL_HIGH,
              ("td Err, No data sent through communication socket, "
               "%s. (File: %s, line: %d)\n",
               strerror(errno), __FILE__, __LINE__));
         TD_OS_SocketClose(g_pITM_Ctrl->nComOutSocket);
         g_pITM_Ctrl->nComOutSocket = NO_SOCKET;
         return IFX_ERROR;
      }
   }

   return IFX_SUCCESS;
}/* COM_SendTraceMessage() */

/**
   Send identification message through communication socket

   \param pPhone  - pointer to phone structure
   \param pFXO    - pointer to FXO structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_SendIdentification(PHONE_t *pPhone, FXO_t *pFXO)
{
   IFX_char_t buf[128];
   IFX_uint32_t nSeqConnId = TD_CONN_ID_ERR;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
          ("COM_SendIdentification()\n"));

   if (IFX_NULL != pPhone)
   {
      nSeqConnId = pPhone->nSeqConnId;
      sprintf(buf, "IDENTIFICATION_RESPONSE:{ PHONE_NO %d }%s",
              pPhone->nPhoneNumber, COM_MESSAGE_ENDING_SEQUENCE);
   }
   else if (IFX_NULL != pFXO)
   {
      nSeqConnId = pFXO->nSeqConnId;
      sprintf(buf, "IDENTIFICATION_RESPONSE:{ FXO_NO %d }%s",
              pFXO->nFXO_Number, COM_MESSAGE_ENDING_SEQUENCE);
   }
   else
   {
      /* both fxo and phone sturctures aren't set */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, ITM: both fxo and phone sturctures aren't set."
             " (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }


   if (IFX_SUCCESS == COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf,
                                      nSeqConnId))
   {
      return IFX_SUCCESS;
   }
   return IFX_ERROR;
} /* COM_SendIdentification() */

/**
   Send DTMF command response, also executes ITM specific functions.

   \param pBoard - pointer to board structure
   \param nDialedNum - number of dtmf command
   \param nStatus - default command response
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_ExecuteDTMFCommand(BOARD_t* pBoard, IFX_int32_t nDialedNum,
                                    IFX_int32_t nStatus,
                                    IFX_uint32_t nSeqConnId)
{
   IFX_char_t buf[TD_COM_SEND_MSG_SIZE];
   IFX_char_t *pCodecString;
   IFX_int32_t ret = IFX_SUCCESS;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
          ("COM_ExecuteDTMFCommand()\n"));

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /*Temporal work around */
   if ((1000 <= nDialedNum) && (1999 >= nDialedNum))
   {
      ret = nStatus;
      if ((IFX_NULL != pBoard->pDataChStat) &&
          (pBoard->pDataChStat[0].nCodecType > 0) &&
          (pBoard->pDataChStat[0].nCodecType < IFX_TAPI_ENC_TYPE_MAX))
      {
         /* get codec name */
         pCodecString = pCodecName[pBoard->pDataChStat[0].nCodecType];
      }
      else
      {
         pCodecString = "UNKNOWN_CODEC";
         ret = IFX_ERROR;
      }

      sprintf(buf, "DTMF_CODE_RESPONSE:{{DTMF_CODE %d } "
              "{DTMF_RESPONSE %s }  {{DTMF_MODULE %s } %s } } %s",
              (int)nDialedNum, IFX_ERROR != ret ? "OK" : "FAIL",
              pCodecString, IFX_ERROR != ret ? "{DTMF_MODULE_STATE OK }" : "",
              COM_MESSAGE_ENDING_SEQUENCE);

      return COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, nSeqConnId);
   }
   else if (3010 == nDialedNum)
   {
      /*ret = COM_FaxModemCtrlReset();*/
   }
   else if (3011 == nDialedNum)
   {
      /*ret = COM_FaxModemCtrlReset();*/
   }
   else
   {
      ret = nStatus;
   }

   sprintf(buf,
           "DTMF_CODE_RESPONSE:{{DTMF_CODE %d } {DTMF_RESPONSE %s }   } %s",
           (int) nDialedNum, IFX_ERROR != ret ? "OK" : "FAIL",
           COM_MESSAGE_ENDING_SEQUENCE);


   return COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, nSeqConnId);
} /* COM_ExecuteDTMFCommand() */

/**
   Wait for message from Control PC.

   \param pBoard - pointer to control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t COM_MessageWait(CTRL_STATUS_t* pCtrl)
{
   TD_OS_socFd_set_t rfds, trfds;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t nMaxFd = 0;
   IFX_boolean_t bContinue;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComInSocket),
                      g_pITM_Ctrl->nComInSocket, IFX_ERROR);

   TD_OS_SocFdZero(&rfds);
   COM_SOCKET_FD_SET(&rfds, nMaxFd);

   do
   {
      /* Update the local file descriptor by the copy in the task parameter */
      memcpy((void *) &trfds, (void*) &rfds, sizeof(trfds));

      ret = TD_OS_SocketSelect(nMaxFd + 1, &trfds, IFX_NULL,
                               IFX_NULL, TD_OS_SOC_WAIT_FOREVER);

      bContinue = IFX_FALSE;
      if(ret >= 0)
      {
          /* Check if there is a command pending on communication socket */
         if (TD_OS_SocFdIsSet(g_pITM_Ctrl->nComInSocket, &trfds))
         {
            ret = IFX_SUCCESS;
            /* Receive command */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                  ("MsgWait: Command on communication socket\n"));
            g_pITM_Ctrl->oVerifySytemInit.nReceivedID = -1;
            if( IFX_SUCCESS != COM_ReceiveCommand(pCtrl, IFX_FALSE))
            {
               ret =  IFX_ERROR;
            }
         }
         else if (TD_OS_SocFdIsSet(g_pITM_Ctrl->oBroadCast.nSocket, &trfds))
         {
            ret = IFX_SUCCESS;
            /* Receive command */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                  ("MsgWait: Command on broadcast socket\n"));
            g_pITM_Ctrl->oVerifySytemInit.nReceivedID = -1;
            if( IFX_SUCCESS != COM_ReceiveCommand(pCtrl, IFX_TRUE))
            {
               ret =  IFX_ERROR;
            }
            bContinue = IFX_TRUE;
         }
         else
         {
            /* todo: should we return error? */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, Received message is not for communication socket."
                   " (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            ret = IFX_ERROR;
         }
   
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, in select(). (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         ret = IFX_ERROR;
      }
      /* if command on broadcast socket then wait again */
   } while (IFX_TRUE == bContinue);
   return ret;
}

/**
   Verify system initialization.

   \param pBoard - pointer to board
   \param pInit - pointer to TAPI initialization structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t COM_VerifySystemInit(BOARD_t* pBoard,
                                  IFX_TAPI_CH_INIT_t* pInit)
{
   CTRL_STATUS_t* pCtrl = IFX_NULL;
   IFX_char_t buf[TD_COM_SEND_MSG_SIZE];
   /* Number of analog or data channel. */
   IFX_int32_t nChannel = 0;
   /* Device number */
   IFX_uint16_t nDev = 0;
   IFX_int32_t nDevTmp = 0;
   /* Contains information if execution of command was successful or not. */
   IFX_int32_t nRet = IFX_ERROR;
   /* If true, function will be waiting for new commands from control PC. */
   IFX_boolean_t hWaitOnMsg = IFX_TRUE;
   IFX_uint32_t nRepeatStatus = 0;
   IFX_int32_t i = 0;
   /* Holds result of ioctl commands */
   IFX_int32_t nIoctlCmdRes = IFX_ERROR;
#ifdef TAPI_VERSION4
   IFX_TAPI_LINE_FEED_t oLineFeedParam;
#endif /* TAPI_VERSION4 */
   IFX_int32_t nAnalogChCount;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pBoard->pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pInit, IFX_ERROR);

   pCtrl = pBoard->pCtrl;

   /* check test type */
   if ( (g_pITM_Ctrl->nTestType != COM_MODULAR_TEST) ||
        ( !(g_pITM_Ctrl->nSubTestType & COM_SUBTEST_SYSINIT) ) )
   {
      /* wrong test type / wrong modular test type */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, test type received from control PC doesn't match "
             "'initialize system' program option. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* inform control PC that system initialization can proceed */
   if (IFX_ERROR == COM_SendSysInitProceed(pBoard->nBoard_IDX))
   {
      /* failed to send message to control PC */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, failled to send message to control PC "
             "'proceed with system initialization test'. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Message SYSINIT_PROCEED sent to control PC\n"));
   }

   /* wait on messages from control PC in loop */
   while (hWaitOnMsg)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("TAPIDEMO is waiting on message from control PC\n"));
      sprintf(g_pITM_Ctrl->oVerifySytemInit.aParametrs , "%d", -1);
      /* wait for command from control PC */
      if (IFX_SUCCESS == COM_MessageWait(pCtrl))
      {
         switch (g_pITM_Ctrl->oVerifySytemInit.nReceivedID)
         {
            case COM_ID_SET_CH_INIT_VOICE:
               /* execute IFX_TAPI_CH_INIT */
               nChannel = -1;
               nDev = -1;
               nRet = IFX_SUCCESS;
               sscanf (g_pITM_Ctrl->oVerifySytemInit.aParametrs,
                       "%d;%d", (int*)&nDevTmp, (int*)&nChannel);
               nDev = (IFX_uint16_t)nDevTmp;
               /* check if channel FD is available */
               if (nChannel >= pBoard->nUsedChannels)
               {
                  /* failed to send message to control PC */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, %s invalid ch number %d (%d). "
                         "(File: %s, line: %d)\n",
                         pBoard->pszBoardName, nChannel, pBoard->nUsedChannels,
                         __FILE__, __LINE__));
                  nRet = IFX_ERROR;
               }
               else
               {
                  /* check channel/device number */
                  if(0 <= nChannel)
                  {
                     /* Initialize system channel */

                     nIoctlCmdRes = IFX_ERROR;
   #ifdef TAPI_VERSION4
                     if (pBoard->fSingleFD)
                     {
                        pInit->dev = nDev;
                        /* given channel */
                        pInit->ch = nChannel;
                        nIoctlCmdRes = TD_IOCTL(pBoard->nDevCtrl_FD,
                                          IFX_TAPI_CH_INIT,
                                          (IFX_int32_t) pInit,
                                          nDev, TD_CONN_ID_ITM);
                     }
                     else
   #endif /* TAPI_VERSION4 */
                     {
                        nIoctlCmdRes = TD_IOCTL(pBoard->nCh_FD[nChannel],
                                          IFX_TAPI_CH_INIT,
                                          (IFX_int32_t) pInit,
                                          nDev, TD_CONN_ID_ITM);
                     }


                     if (IFX_SUCCESS != nIoctlCmdRes)
                     {
                        nRet = IFX_ERROR;
                     }
                     else
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                              ("IFX_TAPI_CH_INIT on ch %d\n",nChannel));
                            /* Set DTMF transmission to block all because we
                               are handling the DTMF digits ourselfs. */
   #ifdef HAVE_DRV_VMMC_HEADERS
                        if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[nChannel],
                                              IFX_TAPI_PKT_EV_OOB_DTMF_SET,
                                              IFX_TAPI_PKT_EV_OOB_BLOCK,
                                              nDev, TD_CONN_ID_ITM))
                        {
                           nRet = IFX_ERROR;
                        }
   #endif /* HAVE_DRV_VMMC_HEADERS */
                     }
                  }
                  else
                  {
                     if (0 > nChannel)
                     {
                        /* error, invalid channel no. */
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                              ("Err, Invalid ch for IFX_TAPI_CH_INIT: %d\n",
                               nChannel));
                     }

                     nRet = IFX_ERROR;
                  } /* if((0 <= nChannel) && (0 <= nDev)) */
               } /* if (nChannel >= pBoard->nUsedChannels) */
               sprintf(buf,
                       "SUTS_RESPONSE:{{COMMAND_NAME SET_CH_INIT_VOICE} "
                       "{COMMAND_RESPONSE %s} }%s",
                       IFX_SUCCESS == nRet ? "OK" : "FAIL",
                       COM_MESSAGE_ENDING_SEQUENCE);
               COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
               break; /* case COM_ID_SET_CH_INIT_VOICE */
            case COM_ID_SET_LINE_FEED_SET_STANDBY:
               /* execute IFX_TAPI_LINE_FEED_SET(Standby) */
               nChannel = -1;
               nDev = -1;
               nRet = IFX_SUCCESS;
               sscanf (g_pITM_Ctrl->oVerifySytemInit.aParametrs,
                       "%d;%d", (int*)&nDevTmp, (int*)&nChannel);
               nDev = (IFX_uint16_t)nDevTmp;
               /* check if channel FD is available */
               if (nChannel >= pBoard->nUsedChannels)
               {
                  /* failed to send message to control PC */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, %s invalid chnnale number %d (%d). "
                         "(File: %s, line: %d)\n",
                         pBoard->pszBoardName, nChannel, pBoard->nUsedChannels,
                         __FILE__, __LINE__));
                  nRet = IFX_ERROR;
               }
               else
               /* check channel number */
               if(0 <= nChannel)
               {

                  nIoctlCmdRes = IFX_ERROR;
#ifdef TAPI_VERSION4
                  if (pBoard->fSingleFD)
                  {
                     memset(&oLineFeedParam, 0, sizeof(IFX_TAPI_LINE_FEED_t));

                     /* set line mode to standby */
                     oLineFeedParam.lineMode = IFX_TAPI_LINE_FEED_STANDBY;

                     /* first device */
                     oLineFeedParam.dev = nDev;
                     /* given channel */
                     oLineFeedParam.ch = nChannel;
                     /* Set appropriate feeding on  line channel */
                     nIoctlCmdRes = TD_IOCTL(pBoard->nDevCtrl_FD,
                                       IFX_TAPI_LINE_FEED_SET,
                                       (IFX_int32_t) &oLineFeedParam,
                                       nDev, TD_CONN_ID_ITM);
                  }
                  else
#endif /* TAPI_VERSION4 */
                  {
                     /* Set appropriate feeding on  line channel */
                     nIoctlCmdRes = TD_IOCTL(pBoard->nCh_FD[nChannel],
                                       IFX_TAPI_LINE_FEED_SET,
                                       IFX_TAPI_LINE_FEED_STANDBY,
                                       nDev, TD_CONN_ID_ITM);
                  }


                  if (IFX_SUCCESS != nIoctlCmdRes)
                  {
                     nRet = IFX_ERROR;
                  }
                  else
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                           ("IFX_TAPI_LINE_FEED_STANDBY on line %d\n",
                            nChannel));
                  }
               }
               else
               {
                  if (0 > nChannel)
                  {
                     /* error, invalid channel no. */
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                           ("Err, Invalid channel for IFX_TAPI_LINE_FEED_SET(Standby): %d\n",
                            nChannel));
                  }

                  nRet = IFX_ERROR;
               } /* if((0 <= nChannel) && (0 <= nDev)) */

               sprintf(buf,
                       "SUTS_RESPONSE:"
                       "{{COMMAND_NAME SET_LINE_FEED_SET_STANDBY} "
                       "{COMMAND_RESPONSE %s} }%s",
                       IFX_SUCCESS == nRet ? "OK" : "FAIL",
                       COM_MESSAGE_ENDING_SEQUENCE);
               COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
               break; /* case COM_ID_SET_LINE_FEED_SET_STANDBY */
            case COM_ID_SET_LINE_FEED_SET_ACTIVE:
               /* execute IFX_TAPI_LINE_FEED_SET(Active) */
               nChannel = -1;
               nDev = -1;
               nRet = IFX_SUCCESS;
               sscanf (g_pITM_Ctrl->oVerifySytemInit.aParametrs,
                       "%d;%d", (int*)&nDevTmp, (int*)&nChannel);
               nDev = (IFX_uint16_t)nDevTmp;
               /* check if channel FD is available */
               if (nChannel >= pBoard->nUsedChannels)
               {
                  /* failed to send message to control PC */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, %s invalid chnnale number %d (%d). "
                         "(File: %s, line: %d)\n",
                         pBoard->pszBoardName, nChannel, pBoard->nUsedChannels,
                         __FILE__, __LINE__));
                  nRet = IFX_ERROR;
               }
               else
               {
                  /* check channel number */
                  if(0 <= nChannel)
                  {
                     nIoctlCmdRes = IFX_ERROR;
   #ifdef TAPI_VERSION4
                     if (pBoard->fSingleFD)
                     {
                        memset(&oLineFeedParam, 0, sizeof(IFX_TAPI_LINE_FEED_t));

                        /* set line mode to standby */
                        oLineFeedParam.lineMode = IFX_TAPI_LINE_FEED_ACTIVE;

                        /* first device */
                        oLineFeedParam.dev = nDev;
                        /* given channel */
                        oLineFeedParam.ch = nChannel;
                        /* Set appropriate feeding on  line channel */
                        nIoctlCmdRes = TD_IOCTL(pBoard->nDevCtrl_FD,
                                          IFX_TAPI_LINE_FEED_SET,
                                          (IFX_int32_t) &oLineFeedParam,
                                          nDev, TD_CONN_ID_ITM);
                     }
                     else
   #endif /* TAPI_VERSION4 */
                     {
                        /* Set appropriate feeding on  line channel */
                        nIoctlCmdRes = TD_IOCTL(pBoard->nCh_FD[nChannel],
                                          IFX_TAPI_LINE_FEED_SET,
                                          IFX_TAPI_LINE_FEED_ACTIVE,
                                          nDev, TD_CONN_ID_ITM);
                     }

                     if (IFX_SUCCESS != nIoctlCmdRes)
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                              ("IFX_TAPI_LINE_FEED_SET (Active) "
                               "FAILED (ch: %d)\n",nChannel));
                        nRet = IFX_ERROR;
                     }
                     else
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                              ("IFX_TAPI_LINE_FEED_SET (Active) "
                               "SUCCESS (ch: %d)\n",nChannel));
                     }
                  }
                  else
                  {
                     if (0 > nChannel)
                     {
                        /* error, invalid channel no. */
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                           ("Invalid channel for IFX_TAPI_LINE_FEED_SET(Active): %d\n",
                            nChannel));
                     }
                     nRet = IFX_ERROR;
                  } /* if( (0 <= nChannel) && (0 <= nDev) ) */
               } /* nChannel >= pBoard->nUsedChannels */
               sprintf(buf,
                       "SUTS_RESPONSE:"
                       "{{COMMAND_NAME SET_LINE_FEED_SET_ACTIVE}"
                       " {COMMAND_RESPONSE %s} }%s",
                       IFX_SUCCESS == nRet ? "OK" : "FAIL",
                       COM_MESSAGE_ENDING_SEQUENCE);
               COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);

               break; /* case COM_ID_SET_LINE_FEED_SET_ACTIVE */
            case COM_ID_TAPI_RESTART:
               /* TAPI restart command */
               nRet = COM_TapiRestart(pCtrl, pBoard);
               /* send response */
               sprintf(buf,
                       "SUTS_RESPONSE:"
                       "{{COMMAND_NAME TAPI_RESTART}"
                       " {COMMAND_RESPONSE %s} }%s",
                       IFX_SUCCESS == nRet ? "OK" : "FAIL",
                       COM_MESSAGE_ENDING_SEQUENCE);
               COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
               break; /* COM_ID_TAPI_RESTART */
            case COM_ID_REPEAT_SYSINIT_TEST:
               /* repeat test */
               nRepeatStatus = REPEAT_STATUS_INVALID;
               sscanf (g_pITM_Ctrl->oVerifySytemInit.aParametrs,
                       "%u", &nRepeatStatus);
               switch (nRepeatStatus)
               {
                  case REPEAT_STATUS_END:
                     /* stop test */
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                          ("End of system initialization test.\n"));
                     hWaitOnMsg = IFX_FALSE;
                     nRet = IFX_SUCCESS;
                     break; /* REPEAT_STATUS_END */
                  case REPEAT_STATUS_REPEAT:
                     /* continue test */
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                          ("Repeat system initialization test.\n"));
                     nRet = IFX_SUCCESS;
                     break; /* REPEAT_STATUS_REPEAT */
                  default:
                     /* invalid value in COM_ID_REPEAT_SYSINIT_TEST message */
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                        ("Err, Invalid value (%u) in \"REPEAT_SYSINIT_TEST\" "
                         "message from control PC.\n", nRepeatStatus));
                     nRet = IFX_ERROR;
                     /* error - stop test */
                     hWaitOnMsg = IFX_FALSE;
               } /* switch (nRepeatStatus) */

               /* send response */
               sprintf(buf,
                       "SUTS_RESPONSE:"
                       "{{COMMAND_NAME REPEAT_SYSINIT_SCENARIO}"
                       " {COMMAND_RESPONSE %s} }%s",
                       IFX_SUCCESS == nRet ? "OK" : "FAIL",
                       COM_MESSAGE_ENDING_SEQUENCE);
               COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
               break; /* COM_ID_REPEAT_SYSINIT_TEST */
            case COM_ID_GET_SYSINIT_TYPE:
               /* System initialization type sent to control PC.
                  Nothing to do here.
               */
               break;
            case COM_ID_CONFIG_CHANNELS:
               /* Control PC asks for channel configuration.
                  Nothing to do here.
               */
               break;
            default:
               /* unexpected command from control PC */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                     ("Unexpected command from control PC: %s",
                      Common_Enum2Name(g_pITM_Ctrl->oVerifySytemInit.nReceivedID,
                                       TD_rgMsgIdName)));
               break;

         } /* switch (pCtrl->nReceivedID) */
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, COM_MessageWait() failed "
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
      } /* if (IFX_SUCCESS == COM_MessageWait(pCtrl)) */
   }

   /************************************
   * end of system initialization test *
   ************************************/
   g_pITM_Ctrl->oVerifySytemInit.nExpectedID = COM_ID_UNKNOWN;

   /* for each device on board */
   for (nDev = 0; nDev < pBoard->nChipCnt; ++nDev)
   {
      /*****************************************
      * get number of analog channels on board *
      *****************************************/
      nAnalogChCount = COM_GetCapabilityValue(pBoard,
                                              nDev,
                                              IFX_TAPI_CAP_TYPE_PHONES);
      if (0 > nAnalogChCount)
      {
         /* failed to get number of phone channels */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, to get number of phone channels. "
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /***********************************
      * set line feeds on phone channels *
      ***********************************/
      for(i = 0; i < nAnalogChCount; i++)
      {
         nIoctlCmdRes = IFX_ERROR;
#ifdef TAPI_VERSION4
         if (pBoard->fSingleFD)
         {
            memset(&oLineFeedParam, 0, sizeof(IFX_TAPI_LINE_FEED_t));

            /* set line mode to standby */
            oLineFeedParam.lineMode = IFX_TAPI_LINE_FEED_STANDBY;

            /* first device */
            oLineFeedParam.dev = nDev;
            /* given channel */
            oLineFeedParam.ch = i;
            /* Set appropriate feeding on  line channel */
            nIoctlCmdRes = TD_IOCTL(pBoard->nDevCtrl_FD,
                              IFX_TAPI_LINE_FEED_SET,
                              (IFX_int32_t) &oLineFeedParam,
                              nDev, TD_CONN_ID_ITM);
         }
         else
#endif /* TAPI_VERSION4 */
         {
            /* Set appropriate feeding on  line channel */
            nIoctlCmdRes = TD_IOCTL(pBoard->nCh_FD[i],
                              IFX_TAPI_LINE_FEED_SET,
                              IFX_TAPI_LINE_FEED_STANDBY,
                              nDev, TD_CONN_ID_ITM);
         }

         if (IFX_SUCCESS != nIoctlCmdRes)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, IFX_TAPI_LINE_FEED_SET (Standby) "
                   "FAILED (board: %d, dev: %d, ch: %d)\n",
                   pBoard->nBoard_IDX, nDev, i));
         }
      } /* for(i = 0; i < nAnalogChCount; i++) */
   } /* for (nDev = 0; nDev < pCtrl->rgoBoards[i].nChipCnt; ++nDev) */

   /* inform control PC that system initialization for this board is done */
   if (IFX_ERROR == COM_SendSysInitHalt(pBoard->nBoard_IDX))
   {
      /* failed to send message to control PC */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, failled to send message to control PC "
             "'halt system initialization test'. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Message SYSINIT_HALT sent to control PC\n"));
   }

   return IFX_SUCCESS;
} /* COM_VerifySystemInit() */

/**
   Prepare board to Verify system initialization test.

   \param pCtrl - pointer to control structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t COM_PrepareVerifySystemInit(CTRL_STATUS_t* pCtrl)
{
   /* Informs that function COM_PrepareVerifySystemInit was already called */
   static IFX_int32_t nPrepareVerifySystemInitDone = 0;
   /* check if this function was already called (and ended with success) */
   if ( nPrepareVerifySystemInitDone )
   {
      /* function already called (and ended with success) */
      return IFX_SUCCESS;
   }
   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComInSocket),
                      g_pITM_Ctrl->nComInSocket, IFX_ERROR);

   /* Waiting for control PC */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
        ("TAPIDEMO is waiting on control PC\n"));
   /* First message from control PC is a broadcast message. */
   g_pITM_Ctrl->oVerifySytemInit.nExpectedID = COM_ID_BROADCAST_MSG;
   if (IFX_ERROR == COM_MessageWait(pCtrl))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, COM_MessageWait() failed"
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
       return IFX_ERROR;
   }
   if(g_pITM_Ctrl->oVerifySytemInit.nExpectedID !=
      g_pITM_Ctrl->oVerifySytemInit.nReceivedID)
   {
       TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, Received command other than expected. "
             "Expected 0x%x, Received 0x%x "
             "(File: %s, line: %d)\n",
             g_pITM_Ctrl->oVerifySytemInit.nExpectedID,
             g_pITM_Ctrl->oVerifySytemInit.nReceivedID,
             __FILE__, __LINE__));
       return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
         ("TAPIDEMO is waiting on message \"CONFIG_CHANNELS\" "
          "from control PC\n"));
   g_pITM_Ctrl->oVerifySytemInit.nExpectedID = COM_ID_CONFIG_CHANNELS;
   do
   {
      if (IFX_ERROR == COM_MessageWait(pCtrl))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, COM_MessageWait() failed "
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } while(g_pITM_Ctrl->oVerifySytemInit.nExpectedID !=
           g_pITM_Ctrl->oVerifySytemInit.nReceivedID);

   /* save information that this function was called */
   nPrepareVerifySystemInitDone = 1;

   return IFX_SUCCESS;
}

/**
   Gets custom tone index if defined for this tone.

   \param nTone - default tone number index

   \return if custom tone defined then return custom tone index
           else return input value

   \remark
*/
IFX_return_t COM_GetCustomCPT(IFX_int32_t nTone)
{
   switch (nTone)
   {
      case DIALTONE_IDX:
         nTone = CPT_INDEX + DIAL_TONE;
         break;
      case RINGBACK_TONE_IDX:
         nTone = CPT_INDEX + RINGBACK_TONE;
         break;
      case BUSY_TONE_IDX:
         nTone = CPT_INDEX + BUSY_TONE;
         break;
      default:
         /* no custom tone */
         break;
   }
   return nTone;
} /* COM_GetCustomCPT */

/**
   Function restores default channel mappings, phone default channels and
   frees memory.

   \param pCtrl   - pointer to control structure

   \return

   \remark
*/
IFX_void_t COM_TestEnd(CTRL_STATUS_t* pCtrl)
{
#ifndef EASY336
   BOARD_t* pBoard;
   PHONE_t* pPhone;
   IFX_int32_t i, j;
#endif /* EASY336 */
   /* check input parameters */
   TD_PTR_CHECK(pCtrl,);

   /* notify user that function was called */
   /*TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
      ("ITM: COM_TestEnd()\n"));*/
#ifndef EASY336
   /* for all boards */
   for (i=0; i<pCtrl->nBoardCnt; i++)
   {
      /* if Custom mapping was used */
      if (IFX_NULL != pCtrl->rgoBoards[i].pChCustomMapping)
      {
         /* get board */
         pBoard = &pCtrl->rgoBoards[i];
         /* unmapp test specific mapping and free memory */
         ABSTRACT_UnmapCustom(pBoard, TD_CONN_ID_ITM);
         TD_OS_MemFree(pCtrl->rgoBoards[i].pChCustomMapping);
         pCtrl->rgoBoards[i].pChCustomMapping = IFX_NULL;
         /* use default mapping */
         ABSTRACT_DefaultMapping(pCtrl, pBoard, TD_CONN_ID_ITM);
         /* for all phones */
         for (j=0; j<pBoard->nMaxAnalogCh; j++)
         {
            /* ABSTRACT_DefaultMapping() doesn't set file descriptors  */
            pPhone = &pBoard->rgoPhones[j];
            pPhone->nPhoneCh_FD =  ALM_GetFD_OfCh(pBoard, pPhone->nPhoneCh);
            pPhone->nDataCh_FD =  VOIP_GetFD_OfCh(pPhone->nDataCh, pBoard);
         } /* for all phones */
      } /* if Custom mapping was used */
   } /* for all boards */
#else /* EASY336 */
   if ((g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) &&
       (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_DTMFDIGIT))
   {
      if(IFX_SUCCESS != COM_SvipCustomMapping(map_table, IFX_FALSE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: failed to do custom mapping,"
             " board %d, dev %d. (File: %s, line: %d)\n",
             map_table.pBoard->nBoard_IDX,
             map_table.nDevice,
             __FILE__, __LINE__));
      }
      /* restore default mapping */
      SVIP_RM_Init(pCtrl->rgoBoards[0].nDevCtrl_FD,
                   pCtrl->rgoBoards[0].nChipCnt,
                   pCtrl->rgoBoards[1].nDevCtrl_FD,
                   pCtrl->rgoBoards[1].nChipCnt,
                   &pcm_cfg);
   }
#endif /* EASY336 */
   if (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_CID)
   {
      COM_CID_TestInit(pCtrl, COM_TEST_STOP);
   }
   /* reset tested phone number */
   g_pITM_Ctrl->oCID_Test.nPhoneNumber = 0;
   /* reset test data */
   g_pITM_Ctrl->nTestType = COM_NO_TEST;
   g_pITM_Ctrl->nSubTestType = COM_SUBTEST_NONE;
#ifdef TD_FAX_MODEM
   pCtrl->nFaxMode = TD_FAX_MODE_DEAFAULT;
#endif /* TD_FAX_MODEM */

   return;
} /* COM_TestEnd() */

/**
   Sends ITM messages acording for fax/modem tests.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  nCallPlace  - enum to determine in which place trace function was
                         called
   \param nSeqConnId   - Seq Conn ID

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_ITM_TracesFaxModem(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn,
                                    TEST_TRACE_PLACE_t nCallPlace,
                                    IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_char_t traceGroupName[] = "FAX_MODEM";

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* check where function was called */
   switch (nCallPlace)
   {
      case TTP_RINGING_ACTIVE:
      case TTP_RINGBACK_ACTIVE:
         /* send phone number and its phone channel number */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:PHONE_ACTIVE_PHONE_CH:%d:%d\n",
             traceGroupName, pPhone->nPhoneCh, pPhone->nPhoneNumber));
#ifndef EASY336
         /* phone on extension board can use data channel from main board,
            for such situation pPhone->nDataCh is not set */
         if (TD_NOT_SET != pPhone->nDataCh)
         {
            /* send phone number and its data channel number */
            TAPIDEMO_PRINTF(nSeqConnId,
               ("%s:PHONE_ACTIVE_DATA_CH:%d:%d\n",
                traceGroupName, pPhone->nDataCh, pPhone->nPhoneNumber));
         }
         else
         {
            /* send phone number and used channel number */
            TAPIDEMO_PRINTF(nSeqConnId,
               ("%s:PHONE_ACTIVE_DATA_CH:%d:%d\n",
                traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         }
#else /* EASY336 */
         /* send phone number and its data channel number,
            when RM (resource manager) is used,
            then first free channel is used */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:PHONE_ACTIVE_DATA_CH:%d:%d\n",
             traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
#endif /* EASY336 */
         break;
      case TTP_FAX_MODEM_SIGNAL:
         /* if signal received during  */
         if (S_ACTIVE == pPhone->rgStateMachine[FXS_SM].nState)
         {
            /* first fax/modem signal detected,
               send phone number and data channel */
            TAPIDEMO_PRINTF(nSeqConnId,
               ("%s:SIGNAL_DETECTED:%d:%d\n",
                traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         }
         break;
      case TTP_SET_S_FAX_MODEM:
         /* phone state changed to S_FAX_MODEM - send phone number */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:FAX_MODEM_READY::%d\n",
             traceGroupName, pPhone->nPhoneNumber));
         break;
      case TTP_T_38_CODEC_STOP:
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:CODEC_STOP:%d:%d\n",
             traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         break;
      case TTP_T_38_CODEC_START:
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:CODEC_START:%d:%d\n",
             traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         break;
      case TTP_LEC_DISABLED:
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:LEC_DISABLED:%d:%d\n",
             traceGroupName, pPhone->nPhoneCh, pPhone->nPhoneNumber));
         break;
      case TTP_LEC_ENABLE:
         if (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_FAX_T_38)
         {
            TAPIDEMO_PRINTF(nSeqConnId,
               ("%s:LEC_ENABLED:%d:%d\n",
                traceGroupName, pPhone->nPhoneCh, pPhone->nPhoneNumber));
         }
         break;
      case TTP_START_FAXT38_TRANSMISSION:
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:START_T_38:%d:%d\n",
             traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         break;
      case TTP_STOP_FAXT38_TRANSMISSION:
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:STOP_T_38:%d:%d\n",
             traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         break;
      case TTP_SET_S_FAXT38:
         /* phone state changed to S_FAX_MODEM - send phone number */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:FAX_T_38_READY::%d\n",
             traceGroupName, pPhone->nPhoneNumber));
         break;
      case TTP_CLEAR_CHANNEL:
         /* send phone number and data channel on witch clear channel was made */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:CLEAR_CHANNEL:%d:%d\n",
             traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         break;
      case TTP_DISABLE_NLP:
         /* send phone number and its phone channel number */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:DISABLE_NLP:%d:%d\n",
             traceGroupName, pPhone->nPhoneCh, pPhone->nPhoneNumber));
         break;
      case TTP_RESTORE_CHANNEL:
         /* send phone number and connection used channel number */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:RESTORE_CHANNEL:%d:%d\n",
             traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         break;
      /* fax/modem transmission ended
          IFX_TAPI_EVENT_FAXMODEM_HOLDEND detected */
      case TTP_HOLDEND_DETECTED:
         if (S_FAX_MODEM == pPhone->rgStateMachine[FXS_SM].nState)
         {
            /* first holdend signal detected (still in S_FAX_MODEM state),
               send phone number and data channel */
            TAPIDEMO_PRINTF(nSeqConnId,
               ("%s:HOLDEND_DETECTED:%d:%d\n",
                traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         }
         break;
      /* T.38 state change detected - IFX_TAPI_EVENT_DCN detected */
      case TTP_DCN_DETECTED:
         /* send phone number and connection used channel number */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:T_38_DCN_DETECTED:%d:%d\n",
             traceGroupName, pConn->nUsedCh, pPhone->nPhoneNumber));
         break;
      default:
         /* no action for this call place */
         break;
   } /* switch (nCallPlace) */

   return ret;
}

/**
   Sends ITM messages according for CID test.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  pData  - various data needed to verify test
   \param  nCallPlace  - enum to determine in which place trace function was
                         called
   \param nSeqConnId   - Seq Conn ID

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_ITM_TracesCID(PHONE_t* pPhone, IFX_void_t* pData,
                               TEST_TRACE_PLACE_t nCallPlace,
                               IFX_uint32_t nSeqConnId)
{
   IFX_char_t traceGroupName[] = "CALLER_ID";
   IFX_TAPI_CID_MSG_t* pMsg;
   FXO_t* pFxo = IFX_NULL;

   /* check where function was called */
   switch (nCallPlace)
   {
      case TTP_CID_RING_STOP:
         /* IFX_TAPI_RING_STOP used */
         TD_PTR_CHECK(pPhone, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:RING_STOP:%s:%d\n",
             traceGroupName,
             IFX_SUCCESS == ((IFX_return_t) pData) ? "OK" : "FAIL",
             pPhone->nPhoneNumber));
         break;
      case TTP_CID_MSG_TO_SEND:
      case TTP_CID_MSG_RECEIVED_FSK:
         /* message data is available - send data to test script */
         pMsg = (IFX_TAPI_CID_MSG_t*) pData;
         TD_PTR_CHECK(pMsg, IFX_ERROR);
         if (IFX_SUCCESS != COM_CID_MsgParse(pPhone, pMsg, nCallPlace,
                                             traceGroupName, nSeqConnId))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, COM_CID_MsgParse() failed."
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
         }
         break;
      case TTP_CID_ENABLE:
         /* Common_CID_Enable() used - only TAPI_V4 */
         TD_PTR_CHECK(pPhone, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:ENABLE:%s:%d\n", traceGroupName,
             (IFX_SUCCESS == (IFX_return_t) pData) ? "OK" : "FAIL",
             pPhone->nPhoneNumber));
         break;
      case TTP_CID_DISABLE:
         /* Common_CID_Disable() used - only TAPI_V4 */
         TD_PTR_CHECK(pPhone, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:DISABLE:%s:%d\n", traceGroupName,
             (IFX_SUCCESS == (IFX_return_t) pData) ? "OK" : "FAIL",
             pPhone->nPhoneNumber));
         break;
      case TTP_CID_TX_SEQ_START:
         /* IFX_TAPI_CID_TX_SEQ_START used */
         TD_PTR_CHECK(pPhone, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:TX_SEQ_START:%s:%d\n",
             traceGroupName,
             (IFX_SUCCESS == (IFX_return_t) pData) ? "OK" : "FAIL",
             pPhone->nPhoneNumber));
         break;
      case TTP_CID_EVENT_CID_TX_SEQ_END:
         /* event IFX_TAPI_EVENT_CID_TX_SEQ_END detected */
         TD_PTR_CHECK(pPhone, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:EVENT_CID_TX_SEQ_END::%d\n",
             traceGroupName, pPhone->nPhoneNumber));
         break;
      case TTP_CID_EVENT_FXO_RING_START:
         /* event IFX_TAPI_EVENT_FXO_RING_START detected first time */
         pFxo = (FXO_t*) pData;
         TD_PTR_CHECK(pFxo, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:EVENT_FXO_RING_START::%d\n",
             traceGroupName, pFxo->nFXO_Number));
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:FXO_DATA_CH:SET:%d\n",
             traceGroupName, pFxo->nDataCh));
         break;
      case TTP_CID_EVENT_FXO_RING_STOP:
         /* event IFX_TAPI_EVENT_FXO_RING_STOP detected first time */
         pFxo = (FXO_t*) pData;
         TD_PTR_CHECK(pFxo, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:EVENT_FXO_RING_STOP::%d\n",
             traceGroupName, pFxo->nFXO_Number));
         break;
      case TTP_CID_FSK_RX_START:
         /* successfull use of CID_StartFSK() */
         pFxo = (FXO_t*) pData;
         TD_PTR_CHECK(pFxo, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:FSK_RX_START:OK:%d\n",
             traceGroupName, pFxo->nDataCh));
         break;
      case TTP_CID_DTMF_DETECTION_START:
         /* successfull use of CID_StartFSK() */
         pFxo = (FXO_t*) pData;
         TD_PTR_CHECK(pFxo, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:DTMF_DETECTION_START:OK:%d\n",
             traceGroupName, pFxo->nDataCh));
         break;
      case TTP_CID_EVENT_CID_RX_END:
         /* event IFX_TAPI_EVENT_CID_RX_END detected */
         pFxo = (FXO_t*) pData;
         TD_PTR_CHECK(pFxo, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:EVENT_CID_RX_END::%d\n",
             traceGroupName, pFxo->nDataCh));
         break;
      case TTP_CID_FSK_RX_STOP:
         /* successfull use of CID_StopFSK() */
         /* data channel number in pData */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:FSK_RX_STOP:OK:%d\n",
             traceGroupName, (IFX_int32_t)pData));
         break;
      case TTP_CID_DTMF_DETECTION_STOP:
         /* successfull use of CID_StartFSK() */
         pFxo = (FXO_t*) pData;
         TD_PTR_CHECK(pFxo, IFX_ERROR);
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:DTMF_DETECTION_STOP:OK:%d\n",
             traceGroupName, pFxo->nDataCh));
         break;
      case TTP_CID_FSK_RX_STATUS_GET:
         /* data channel in pData */
         /* successfull use of IFX_TAPI_CID_RX_STATUS_GET */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:FSK_RX_STATUS_GET:OK:%d\n",
             traceGroupName, (IFX_int32_t)pData ));
         break;
      case TTP_CID_FSK_RX_DATA_GET:
         /* data channel in pData */
         /* successfull use of IFX_TAPI_CID_RX_DATA_GET */
         TAPIDEMO_PRINTF(nSeqConnId,
            ("%s:FSK_RX_DATA_GET:OK:%d\n",
             traceGroupName, (IFX_int32_t)pData ));
         break;
      case TTP_CID_MSG_RECEIVED_DTMF:
         /* CID in DTMF standard received */
         pFxo = (FXO_t*) pData;
         TD_PTR_CHECK(pFxo, IFX_ERROR);
         if (IFX_SUCCESS != COM_CID_MsgParseDTMF_Rec(pFxo, traceGroupName))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, COM_CID_MsgParseDTMF_Rec() failed."
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
         }
         break;
      default:
         /* no action for this call place */
         break;
   }
   return IFX_SUCCESS;
}
/**
   Sends ITM messages acording to test type and call place.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  pData  - various data needed to verify test
   \param  nCallPlace  - enum to determine in which place trace function was
                         called

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_ITM_Traces(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                            CONNECTION_t* pConn, IFX_void_t* pData,
                            TEST_TRACE_PLACE_t nCallPlace,
                            IFX_uint32_t nSeqConnId)
{
   /* check if modular test type and subtest is set */
   if ((COM_MODULAR_TEST != g_pITM_Ctrl->nTestType) ||
       (COM_SUBTEST_NONE == g_pITM_Ctrl->nSubTestType))
   {
      /* traces only for modular tests if test type is specified */
      return IFX_SUCCESS;
   }

   /* check test type */
   if ((g_pITM_Ctrl->nSubTestType & COM_SUBTEST_FAX) ||
       (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_MODEM) ||
       (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_FAX_T_38))
   {
      if (nCallPlace & TTP_TYPE_FAX_MODEM)
      {
         /* print fax/modem test trace */
         return COM_ITM_TracesFaxModem(pCtrl, pPhone, pConn, nCallPlace,
                                       nSeqConnId);
      }
   }

   /* check test type */
   if (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_CID)
   {
      if (nCallPlace & TTP_TYPE_CID)
      {
         /* print CID test trace */
         return COM_ITM_TracesCID(pPhone, pData, nCallPlace, nSeqConnId);
      }
   }

   return IFX_SUCCESS;
}

/**
   Send capability data to test script.

   \param nBoardIdx  - board identifier
   \param nDev       - device/chip number
   \param pCapData   - pointer to single capability structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t COM_SendCapabilityData(IFX_int32_t nBoardIdx, IFX_int32_t nDev,
                                    IFX_TAPI_CAP_t* pCapData)
{
   IFX_char_t pszITM_Message[2 * MAX_PATH_LEN];

   /* check input parameters */
   TD_PTR_CHECK(pCapData, IFX_ERROR);
   if (g_pITM_Ctrl->nComOutSocket == NO_SOCKET)
   {
      /* conection with server is not established */
      return IFX_SUCCESS;
   }

   if ((0 == nBoardIdx) && (0 == nDev)) 
   {
      /* Message SYSTEM_CAP is obsolete. It was replaced by DEV_CAP.
         The message is required to keep backward compatibility.
         The message is sent only for the first board and the first device. */
      /* clear string */
      memset(pszITM_Message, 0, 2 * MAX_PATH_LEN);
      /* set message string */
      snprintf(pszITM_Message, 2 * MAX_PATH_LEN,
               "SYSTEM_CAP: "
               "{"
                  "{NAME {%s}} "
                  "{NAME_ID {%d}} "
                  "{DESC {%s}} "
                  "{VALUE {%d}} "
                  "{CAP_NUM {%d}}"
               "}%s",
               Common_Enum2Name(pCapData->captype, TD_rgCapType), pCapData->captype,
               pCapData->desc, pCapData->cap, pCapData->handle,
               COM_MESSAGE_ENDING_SEQUENCE);
      /* send message string */
      COM_SendMessage(g_pITM_Ctrl->nComOutSocket, pszITM_Message,
                      TD_CONN_ID_ITM);
   }

   /* clear string */
   memset(pszITM_Message, 0, 2 * MAX_PATH_LEN);
   /* set message string */
   snprintf(pszITM_Message, 2 * MAX_PATH_LEN,
            "DEV_CAP: "
            "{"
               "BOARD_NO {%d} "
               "DEV_NO {%d} "
               "NAME {%s} "
               "NAME_ID {%d} "
               "DESC {%s} "
               "VALUE {%d} "
               "CAP_NUM {%d}"
            "}%s",
            nBoardIdx, nDev,
            Common_Enum2Name(pCapData->captype, TD_rgCapType), pCapData->captype,
            pCapData->desc, pCapData->cap, pCapData->handle,
            COM_MESSAGE_ENDING_SEQUENCE);
   /* send message string */
   COM_SendMessage(g_pITM_Ctrl->nComOutSocket, pszITM_Message, TD_CONN_ID_ITM);

   return IFX_SUCCESS;
} /* */

/**
   Read and send capability data.

   \param pBoard   - pointer to board structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.

   \remark
*/
IFX_return_t COM_GetCapabilityData(BOARD_t* pBoard)
{
#if 0 /* if not USE_OLD_CAP_LIST - for now new version is disabled */
   IFX_TAPI_CAP_LIST_t capList;
#endif /* USE_OLD_CAP_LIST */
   IFX_TAPI_CAP_t *pCapList = IFX_NULL;
#ifdef TAPI_VERSION4
   IFX_TAPI_CAP_NR_t oCapNum;
#endif /* TAPI_VERSION4 */
   IFX_int32_t nCapNum = 0;
   IFX_int32_t nDev, nCap;
   IFX_return_t nRet;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if (g_pITM_Ctrl->nComOutSocket == NO_SOCKET)
   {
      /* conection with server is not established */
      return IFX_SUCCESS;
   }

   /* for all devices */
   for (nDev = 0; nDev < pBoard->nChipCnt; nDev++)
   {
#ifdef TAPI_VERSION4
      memset(&oCapNum, 0, sizeof(IFX_TAPI_CAP_NR_t));
      oCapNum.dev = nDev;

      /* get number of capabilities */
      nRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_NR, &oCapNum,
                       nDev, TD_CONN_ID_ITM);
      nCapNum = oCapNum.nCap;
#else /* TAPI_VERSION4 */
      /* get number of capabilities */
      nRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_NR, &nCapNum,
                       nDev, TD_CONN_ID_ITM);
#endif /* TAPI_VERSION4 */
      if (IFX_ERROR == nRet)
      {
         return IFX_ERROR;
      }
      if (0 >= nCapNum)
      {
         /* Wrong input arguments */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Invalid number of capabilities (%d). "
                "(File: %s, line: %d)\n",
                nCapNum, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      pCapList = TD_OS_MemCalloc(nCapNum, sizeof(IFX_TAPI_CAP_t));
      if (IFX_NULL == pCapList)
      {
         /* Wrong input arguments */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Failed to allocate memmory (%d, %d). "
                "(File: %s, line: %d)\n",
                nCapNum, sizeof(IFX_TAPI_CAP_t), __FILE__, __LINE__));
         return IFX_ERROR;
      }
#if 0 /* if not USE_OLD_CAP_LIST - for now new version is disabled */

      memset (&capList, 0, sizeof (capList));
#ifdef TAPI_VERSION4
      capList.dev = nDev;
#endif /* TAPI_VERSION4 */
      capList.nCap = nCapNum;
      capList.pList = pCapList;
      /* get number of capabilities */
      nRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_LIST, &capList,
                       nDev, TD_CONN_ID_ITM);
#else /* USE_OLD_CAP_LIST */

#ifdef TAPI_VERSION4
      pCapList->dev = nDev;
#endif /* TAPI_VERSION4 */
      /* get number of capabilities */
      nRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_LIST, pCapList,
                       nDev, TD_CONN_ID_ITM);
#endif /* USE_OLD_CAP_LIST */
      if (IFX_ERROR == nRet)
      {
         return IFX_ERROR;
      }

      for (nCap = 0; nCap < nCapNum; nCap++)
      {
         COM_SendCapabilityData(pBoard->nBoard_IDX, nDev, &pCapList[nCap]);
      }
   } /* for all devices */
   if (IFX_NULL != pCapList)
   {
      TD_OS_MemFree(pCapList);
      pCapList = IFX_NULL;
#if 0 /* if not USE_OLD_CAP_LIST - for now new version is disabled */
      capList.pList = IFX_NULL;
#endif /* USE_OLD_CAP_LIST */
   }
   return IFX_SUCCESS;
}

/**
   Get next line from file descriptor.

   \param pLine   - pointer to char buffer
   \param nMaxLen - size of pLine buffer
   \param nFd     - file handler

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_int32_t COM_GetLine(IFX_char_t* pLine, IFX_uint32_t nMaxLen,
                        TD_OS_File_t* pFd)
{
   IFX_int32_t i = 0;
   IFX_int32_t cChar;

   TD_PTR_CHECK(pLine, IFX_ERROR);
   TD_PARAMETER_CHECK((nMaxLen > MAX_CMD_LEN), nMaxLen, IFX_ERROR);

   memset(pLine, 0, sizeof(IFX_char_t) * nMaxLen);

   while (1)
   {
      /* Get char from file */
      cChar = fgetc(pFd);
      if ((cChar == TD_EOL) || (cChar == EOF))
      {
         return i;
      }
      /* check length */
      if ((MAX_CMD_LEN - 1) < i)
      {
         /* long line */
         return IFX_ERROR;
      }
      /* set char and increase counter */
      pLine[i] = cChar;
      i++;
   }
   return IFX_ERROR;
}

/**
   Send sut info some system info line linux kernel version and uptime.

   \param  none

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_SendSutInfo(IFX_void_t)
{
   IFX_int32_t i=0;
   IFX_char_t rgoCmdOutput[MAX_CMD_LEN];
   IFX_char_t buf[MAX_CMD_LEN];
   TD_OS_File_t* pInfoFileFd;
   IFX_char_t rgoLine[MAX_CMD_LEN];
   IFX_int32_t nRead;
   IFX_char_t *pszInfoFile = "/tmp/sut_info.itm";
   IFX_char_t *pCommands[] =
   {
      "echo \"uptime in seconds\" > /tmp/sut_info.itm",
      "cat /proc/uptime >> /tmp/sut_info.itm",
      "cat /proc/version >> /tmp/sut_info.itm",
      "version.sh >> /tmp/sut_info.itm",
      "cat /tmp/sut_info.itm",
      IFX_NULL
   };

   /* execute all commands */
   for (i=0; IFX_NULL != pCommands[i]; i++)
   {
      COM_ExecuteShellCommand(pCommands[i], rgoCmdOutput);
   }

   /* read from log file */
   pInfoFileFd = TD_OS_FOpen(pszInfoFile, "r");
   if (pInfoFileFd == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
            ("ITM, Failed to read file %s.\n", pszInfoFile));
      return IFX_ERROR;
   }
   /* get text from file, line by line */
   while ((nRead = COM_GetLine(rgoLine, MAX_CMD_LEN / 2, pInfoFileFd)) > 0)
   {
      /* check if line not too long */
      if (2 * nRead <= MAX_CMD_LEN)
      {
         /* send to control pc */
         sprintf(buf, "INFO_SUT:{ %s }%s", rgoLine,
                 COM_MESSAGE_ENDING_SEQUENCE);
         COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_ITM,
               ("ITM, Following line read from %s is too long %d (max=%d):\n"
                "\t%s.\n", pszInfoFile, nRead, (MAX_CMD_LEN / 2), rgoLine));
      }
   } /* get text from file, line by line */

   TD_OS_FClose(pInfoFileFd);

   return IFX_SUCCESS;
}

/**
   Sends tapidemo and ITM module version.

   \param  pCtrl  - pointer to status control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_SendAppVersion(CTRL_STATUS_t* pCtrl)
{
   IFX_char_t pszITM_Message[2 * MAX_PATH_LEN];

   /* check input parameters */
   TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComOutSocket),
                      g_pITM_Ctrl->nComOutSocket, IFX_ERROR);

   /* check if version string is set */
   if ('\0' != g_pITM_Ctrl->rgoTapidemoVersion[0])
   {
      /* clear string */
      memset(pszITM_Message, 0, 2 * MAX_PATH_LEN);
      /* set message string */
      snprintf(pszITM_Message, 2 * MAX_PATH_LEN,
               "SW_VERSION:{{NAME %s} {VERSION {%s}}}%s",
               "TAPIDEMO", g_pITM_Ctrl->rgoTapidemoVersion,
               COM_MESSAGE_ENDING_SEQUENCE);
      /* send message string */
      COM_SendMessage(g_pITM_Ctrl->nComOutSocket, pszITM_Message,
                      TD_CONN_ID_ITM);
   }
   /* check if version string is set */
   if ('\0' != g_pITM_Ctrl->rgoITM_Version[0])
   {
      /* clear string */
      memset(pszITM_Message, 0, 2 * MAX_PATH_LEN);
      /* set message string */
      snprintf(pszITM_Message, 2 * MAX_PATH_LEN,
               "SW_VERSION:{{NAME %s} {VERSION {%s}}}%s",
               "ITM", g_pITM_Ctrl->rgoITM_Version,
               COM_MESSAGE_ENDING_SEQUENCE);
      /* send message string */
      COM_SendMessage(g_pITM_Ctrl->nComOutSocket, pszITM_Message,
                      TD_CONN_ID_ITM);
   }
   return IFX_SUCCESS;
}

/**
   Sends information about software version.

   \param  pCtrl  - pointer to status control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_SendSutSW_VersionAndCappabilities(CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t j, i = 0;
   TD_OS_File_t* fd = 0;
   IFX_char_t pszDriverVersion[MAX_PATH_LEN], pszITM_Message[2 * MAX_PATH_LEN];

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComOutSocket),
                      g_pITM_Ctrl->nComOutSocket, IFX_ERROR);

   /* check if version string is set */
   if ('\0' != g_pITM_Ctrl->rgoFW_Version[0])
   {
      /* clear string */
      memset(pszITM_Message, 0, 2 * MAX_PATH_LEN);
      /* set message string */
      snprintf(pszITM_Message, 2 * MAX_PATH_LEN,
               "FW_VERSION:{{NAME %s} {VERSION {%s}}}%s",
               "FIRMWARE", g_pITM_Ctrl->rgoFW_Version,
               COM_MESSAGE_ENDING_SEQUENCE);
      /* send message string */
      COM_SendMessage(g_pITM_Ctrl->nComOutSocket, pszITM_Message,
                      TD_CONN_ID_ITM);
   }

   /* do untill end of table is reached */
   while (IFX_NULL != DriverTable[i].pszVersionFile)
   {
      /* open file with driver version, assumption was made that version number
         is in first line of the text file */
      fd = TD_OS_FOpen(DriverTable[i].pszVersionFile, "r");
      /* if file was opened */
      if (0 < fd)
      {
         /* reset structures */
         memset(pszDriverVersion, 0, MAX_PATH_LEN);
         memset(pszITM_Message, 0, 2 * MAX_PATH_LEN);
         /* read from file */
         /* TD_OS_FRead returs 0 every time, because it does not
            distinguish between end-of-file and error */
         TD_OS_FRead(pszDriverVersion, MAX_PATH_LEN, 1, fd);
         /* file descriptor no longer needed */
         TD_OS_FClose(fd);
         /* check if read was successfull */
         if (0 < strlen(pszDriverVersion))
         {
            /* for all read chars */
            for (j=0; j<MAX_PATH_LEN; j++)
            {
               /* found end of line or file, only first line of file
                  or first MAX_PATH_LEN chars will be printed. */
               if (('\n' == pszDriverVersion[j]) ||
                   ('\0' == pszDriverVersion[j]))
               {
                  break;
               }
            }
            /* check if value isn't too big */
            if (j >= MAX_PATH_LEN)
            {
               j = MAX_PATH_LEN - 1;
            }
            /* mark end of string */
            pszDriverVersion[j] = '\0';
            /* check if name string isn't too long */
            if ((MAX_PATH_LEN/2) > strlen(DriverTable[i].pszDriverName))
            {
               /* set message string */
               snprintf(pszITM_Message, 2* MAX_PATH_LEN,
                        "SW_VERSION:{{NAME %s} {VERSION {%s}}}%s",
                        DriverTable[i].pszDriverName, pszDriverVersion,
                        COM_MESSAGE_ENDING_SEQUENCE);
               /* send message string */
               COM_SendMessage(g_pITM_Ctrl->nComOutSocket, pszITM_Message,
                               TD_CONN_ID_ITM);
            }
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                     ("Err, String too long %s. (File: %s, line: %d)\n",
                      DriverTable[i].pszDriverName, __FILE__, __LINE__));
            }
         } /* if (0 < nReadBytes) */
      }
      /* go to next driver */
      i++;
   } /* while (IFX_NULL != DriverTable[i].pszVersionFile) */
   /* send cappabilities of all boards */
   for (i=0; i<pCtrl->nBoardCnt; i++)
   {
      COM_GetCapabilityData(&pCtrl->rgoBoards[i]);
   }

   return IFX_SUCCESS;
} /* COM_SendSutSW_VersionAndCappabilities() */


/**
   Sends information about program option.

   \param  nTD_option  - Option number.
   \param  nNumericValue  - Numeric value of option.
   \param  pszDesc  - Option description.
   \param  nDfltValue  - Option default value.
   \param  pszDfltValueTxt  - Option default value as text.

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark Option value/default value may be not used if its usage is disabled
           by other option. Example:
               CID is enabled by default, but user disabled it. In this case
               default value of option CID type contains used CID type. However
               current CID type is not set (CID is disabled). In this situation
               current CID type should be marked as 'value disabled'.
*/
IFX_return_t COM_SendSutSettingMessage(TAPIDEMO_OPTIONS_t      nTD_option,
                                       COM_OPTION_VALUE_TYPE_t nNumericValue,
                                       const IFX_char_t*       pszDesc,
                                       IFX_int32_t             nDfltValue,
                                       const IFX_char_t*       pszDfltValueTxt)
{
   IFX_char_t pszITM_Message[MAX_ITM_MESSAGE_TO_SEND];
   IFX_char_t pszNumberDflt[ITM_MAX_INT32_NUMBER_LENGTH];
   const IFX_char_t* pszEmptyStr = "";
   const IFX_char_t* pszDfltValueAsString = IFX_NULL;
   IFX_int32_t nRet = 0;

   /* clear string */
   memset(pszITM_Message, 0, MAX_ITM_MESSAGE_TO_SEND);

   /* check if numeric value is used */
   switch (nNumericValue)
   {
   case OPT_VALUE_NUMERIC:
      /* convert numeric value to string*/
      nRet = snprintf(pszNumberDflt, ITM_MAX_INT32_NUMBER_LENGTH, "%d", nDfltValue);
      if ((0 >= nRet) || (ITM_MAX_INT32_NUMBER_LENGTH <= nRet))
      {
         /* error or not enough space in string */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, (%d) Failed to prepare message string for option %s(%d),"
               " number to string conversion failed."
               " (File: %s, line: %d)\n",
               nRet, Common_Enum2Name(nTD_option, TD_rgOptName), nTD_option,
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
      pszDfltValueAsString = (const IFX_char_t*) pszNumberDflt;
      break;
   case OPT_VALUE_STRING:
      /* only text value */
      pszDfltValueAsString = pszDfltValueTxt;
      pszDfltValueTxt = pszEmptyStr;
      break;
   default:
      /* error or not enough space in string */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
           ("Invalid option value type %d (File: %s, line: %d)\n",
            nNumericValue, __FILE__, __LINE__));
      return IFX_ERROR;
   } /* switch (nNumericValue) */


   COM_OPT_MSG_STR_VAL(nRet, Common_Enum2Name(nTD_option, TD_rgOptName),
                       pszDfltValueAsString, pszDfltValueTxt, pszDesc);

   /* check if message prepared */
   if ((0 < nRet) && ((MAX_ITM_MESSAGE_TO_SEND) > nRet))
   {
      /* send message string */
      COM_SendMessage(g_pITM_Ctrl->nComOutSocket, pszITM_Message,
                      TD_CONN_ID_ITM);
      return IFX_SUCCESS;
   }
   else
   {
      /* failed to get string */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, (%d) Failed to prepare message string for option %s(%d)."
             " (File: %s, line: %d)\n",
             nRet, Common_Enum2Name(nTD_option, TD_rgOptName), nTD_option,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
} /* COM_SendSutSettingMessage() */

/**
   Sends information about program options.

   \param  pCtrl  - pointer to status control structure

   \return status (IFX_SUCCESS or IFX_ERROR). IFX_ERROR is returned if
           sending at least one option fails
   \remark
*/
IFX_return_t COM_SendSutSettings(IFX_void_t)
{
   TAPIDEMO_OPTIONS_t nTD_option;
   IFX_boolean_t bDfltOption;
   IFX_return_t nSendStatus;
   IFX_return_t nFuncStatus = IFX_SUCCESS;
   PROGRAM_ARG_t* pDefaultProgramArg = IFX_NULL;

   /* check input parameters */
   TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComOutSocket),
                      g_pITM_Ctrl->nComOutSocket, IFX_ERROR);

   /* set pointer */
   pDefaultProgramArg = &g_pITM_Ctrl->oDefaultProgramArg;


   /* for all program option in TAPIDEMO_OPTIONS_t enum */
   for (nTD_option=0; nTD_option < TD_OPT_MAX; nTD_option++)
   {
      nSendStatus = IFX_ERROR;
      /* check witch option data should be now send */
      switch(nTD_option)
      {
      case TD_OPT_PATH_TO_DOWNLOAD_FILES:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_STRING,
                                   COM_EMPTY_STR,
                                   0, pDefaultProgramArg->sPathToDwnldFiles);
         break;
         /*break;*/
      case TD_OPT_BOARD_COMBINATION:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_STRING,
                           COM_EMPTY_STR,
                           0, (IFX_char_t*) BOARD_COMB_NAMES[pDefaultProgramArg->nBoardComb]);
         break;
         /*break;*/
      case TD_OPT_DEBUG_LEVEL:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                   COM_EMPTY_STR,
                                   pDefaultProgramArg->nDbgLevel,
                                   Common_Enum2Name(pDefaultProgramArg->nDbgLevel,
                                                    TD_rgDbgLevelName));
         break;
      case TD_OPT_ITM_DEBUG_LEVEL:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                           COM_EMPTY_STR,
                           TD_ITM_DBG_LEVEL_DEFAULT,
                           Common_Enum2Name(TD_ITM_DBG_LEVEL_DEFAULT,
                                            TD_rgDbgLevelName));
         break;
      case TD_OPT_CALLER_ID:
         /* set message string */
         bDfltOption = pDefaultProgramArg->oArgFlags.nCID != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                              COM_EMPTY_STR,
                              bDfltOption,
                              Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_CALLER_ID_SETTING:
         /* set message string  */
         if (pDefaultProgramArg->oArgFlags.nCID)
         {
            /* CID enabled */
            nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                              COM_EMPTY_STR,
                              pDefaultProgramArg->nCIDStandard,
                              Common_Enum2Name(pDefaultProgramArg->nCIDStandard,
                                               TD_rgCidName));
         }
         else
         {
            /* CID disabled */
            /* nothing to send, set send status to success to prevent errors
               from printing */
            nSendStatus = IFX_SUCCESS;
         }
         break;
      case TD_OPT_VAD_SETTING:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                   COM_EMPTY_STR,
                                   pDefaultProgramArg->nVadCfg,
                                   Common_Enum2Name(pDefaultProgramArg->nVadCfg,
                                         TD_rgVadName));
         break;
      case TD_OPT_CONFERENCE:
         /* set message string */
         bDfltOption = pDefaultProgramArg->oArgFlags.nConference != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                             COM_EMPTY_STR,
                             bDfltOption,
                             Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_QOS_SUPPORT:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_STRING,
                                           COM_EMPTY_STR, 0,
#ifdef QOS_SUPPORT
                                          "compiled with QoS support flag"
#else /* QOS_SUPPORT */
                                          "compiled without QoS support flag"
#endif /* QOS_SUPPORT */
                                          );
         break;
      case TD_OPT_QOS:
         /* set message string */
#ifdef QOS_SUPPORT
         bDfltOption = pDefaultProgramArg->oArgFlags.nQos != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
#else /* QOS_SUPPORT */
         /* nothing to send, set send status to success to prevent errors
            from printing */
         nSendStatus = IFX_SUCCESS;
#endif /* QOS_SUPPORT */
         break;
      case TD_OPT_QOS_ON_SOCKET:
         /* set message string */
#ifdef QOS_SUPPORT
         bDfltOption = pDefaultProgramArg->oArgFlags.nQosSocketStart != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
#else /* QOS_SUPPORT */
         /* nothing to send, set send status to success to prevent errors
            from printing */
         nSendStatus = IFX_SUCCESS;
#endif /* QOS_SUPPORT */
         break;
      case TD_OPT_QOS_LOCAL_UDP:
         /* set message string */
#ifdef QOS_SUPPORT
         if (pDefaultProgramArg->oArgFlags.nQos)
         {
            /* both, current and default, settings have QoS enabled */
            bDfltOption = pDefaultProgramArg->oArgFlags.nQOS_Local != IFX_FALSE;
            nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                      COM_EMPTY_STR,
                                      bDfltOption,
                                      Common_Enum2Name(bDfltOption,
                                                       TD_rgDisableEnable));
         }
         else
         {
            /* else  QoS disabled */
            /* nothing to send, set send status to success to prevent errors
               from printing */
            nSendStatus = IFX_SUCCESS;
         }
#else /* QOS_SUPPORT */
         /* nothing to send, set send status to success to prevent errors
            from printing */
         nSendStatus = IFX_SUCCESS;
#endif /* QOS_SUPPORT */
          break;
      case TD_OPT_IPV6_SUPPORT:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_STRING,
                                           COM_EMPTY_STR, 0,
#ifdef TD_IPV6_SUPPORT
                                          "compiled with IPV6 support"
#else /* TD_IPV6_SUPPORT */
                                          "compiled without IPV6 support"
#endif /* TD_IPV6_SUPPORT */
                                          );
         break;
      case TD_OPT_IPV6:
         /* set message string */
#ifdef TD_IPV6_SUPPORT
         bDfltOption = pDefaultProgramArg->oArgFlags.nUseIPv6 != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
#else /* TD_IPV6_SUPPORT */
         /* nothing to send, set send status to success to prevent errors
            from printing */
         nSendStatus = IFX_SUCCESS;
#endif /* TD_IPV6_SUPPORT */
         break;
      case TD_OPT_FXO_SUPPORT:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_STRING,
                                           "this flag is needed for FXO tests", 0,
#ifdef FXO
                                           "compiled with FXO flag"
#else /* FXO */
                                           "compiled without FXO flag"
#endif /* FXO */
                                           );
         break;
      case TD_OPT_FXO:
         /* set message string */
#ifdef FXO
         bDfltOption = pDefaultProgramArg->oArgFlags.nFXO != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
#else /* FXO */
         /* nothing to send, set send status to success to prevent errors
            from printing */
         nSendStatus = IFX_SUCCESS;
#endif /* FXO */
         break;
      case TD_OPT_USE_CODERS_FOR_LOCAL:
         /* set message string */
         bDfltOption = pDefaultProgramArg->oArgFlags.nUseCodersForLocal != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_NO_LEC:
         /* set message string */
         bDfltOption = pDefaultProgramArg->oArgFlags.nNoLEC != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_ENCODER:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                   COM_EMPTY_STR,
                                   pDefaultProgramArg->nEnCoderType,
                                   pCodecName[pDefaultProgramArg->nEnCoderType]);
         break;
      case TD_OPT_PACKETISATION:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                   COM_EMPTY_STR,
                                   pDefaultProgramArg->nPacketisationTime,
                                   oFrameLen[pDefaultProgramArg->nPacketisationTime]);
         break;
      case TD_OPT_DEAMON:
         /* set message string */
         bDfltOption = pDefaultProgramArg->oArgFlags.nDaemon != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_TRACE_REDIRECTION:
         /* set message string */
         if (pDefaultProgramArg->oArgFlags.nDaemon)
         {
            /* both, current and default, settings have 'execute program
               as deamon' enabled */
            nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                      COM_EMPTY_STR,
                                      pDefaultProgramArg->nTraceRedirection,
                                      Common_Enum2Name(
                                          pDefaultProgramArg->nTraceRedirection,
                                          TD_rgTraceRedirection));
         }
         else
         {
            /* else setting  'execute program as deamon' disabled */
            /* nothing to send, set send status to success to prevent errors
               from printing */
            nSendStatus = IFX_SUCCESS;
         }
         break;
      case TD_OPT_USE_FILESYSTEM:
         /* set message string */
#ifdef USE_FILESYSTEM
         bDfltOption = IFX_TRUE;
#else /* USE_FILESYSTEM */
         bDfltOption = IFX_FALSE;
#endif /* USE_FILESYSTEM */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                  COM_EMPTY_STR, bDfltOption,
                                  Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_CH_INIT:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_STRING,
                                           COM_EMPTY_STR, 0,
#ifdef TD_USE_CHANNEL_INIT
                                          "IFX_TAPI_CH_INIT"
#else /* TD_USE_CHANNEL_INIT */
                                          "IFX_TAPI_DEV_START"
#endif /* TD_USE_CHANNEL_INIT */
                                          );
         break;
      case TD_OPT_PCM_MASTER:
         /* set message string */
         bDfltOption = pDefaultProgramArg->oArgFlags.nPCM_Master != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_PCM_SLAVE:
         /* set message string */
         bDfltOption = pDefaultProgramArg->oArgFlags.nPCM_Slave != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_PCM_LOOP:
         /* set message string */
         bDfltOption = pDefaultProgramArg->oArgFlags.nPCM_Loop != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_PCM_WIDEBAND:
         /* set message string */
         bDfltOption = pDefaultProgramArg->oArgFlags.nUsePCM_WB != IFX_FALSE;
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_NUMERIC,
                                COM_EMPTY_STR,
                                bDfltOption,
                                Common_Enum2Name(bDfltOption, TD_rgDisableEnable));
         break;
      case TD_OPT_FAX_MODEM:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_STRING,
                                     "this flag is needed for fax and modem tests", 0,
#ifdef TD_FAX_MODEM
                                     "compiled with fax/modem flag"
#else /* TD_FAX_MODEM */
                                     "compiled without fax/modem flag"
#endif /* TD_FAX_MODEM */
                                     );
         break;
      case TD_OPT_FAX_T38:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_STRING,
                                     "this flag is needed for fax T.38 tests", 0,
#ifdef TD_FAX_T38
                                     "compiled with fax T.38 flag"
#else /* TD_FAX_T38 */
                                     "compiled without fax T.38 flag"
#endif /* TD_FAX_T38 */
                                     );
         break;
      case TD_OPT_FAX_T38_SUPPORT:
         /* set message string */
         nSendStatus = COM_SendSutSettingMessage(nTD_option, OPT_VALUE_STRING,
                          "this flag is needed for fax T.38 tests", 0,
#ifdef HAVE_T38_IN_FW
                          "tapidemo for this board supports T.38 in firmware"
#else /* HAVE_T38_IN_FW */
                          "tapidemo for this board doesn't support T.38 in firmware"
#endif /* HAVE_T38_IN_FW */
                                     );
         break;
      default:
         /* failed to get string */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Unhandled option '%s' (%d). (File: %s, line: %d)\n",
                Common_Enum2Name(nTD_option, TD_rgOptName), nTD_option,
                __FILE__, __LINE__));
         nSendStatus = IFX_ERROR;
         break;
      } /* switch(nTD_option) */

      if (IFX_ERROR == nSendStatus)
      {
         /* sending message failure */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Failed to send option '%s' (%d)."
                " (File: %s, line: %d)\n",
                Common_Enum2Name(nTD_option, TD_rgOptName), nTD_option,
                __FILE__, __LINE__));
         nFuncStatus = IFX_ERROR;
      } /* if (IFX_ERROR == nSendStatus) */
   } /* for (nTD_option=0; nTD_option < TD_OPT_MAX; ...) */

   return nFuncStatus;
} /* COM_SendSutSettings */

/**
   Sends information about arguments passed to program.

   \param  none

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_SendSutArgv(IFX_void_t)
{
   IFX_int32_t i = 0;
   IFX_char_t pszITM_Message[MAX_ITM_MESSAGE_TO_SEND];
   /*IFX_int32_t nRet = 0;*/

   /* check input parameters */
   TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComOutSocket),
                      g_pITM_Ctrl->nComOutSocket, IFX_ERROR);

   if ( (IFX_NULL == g_pITM_Ctrl->rgsArgv) || (0 >= g_pITM_Ctrl->nArgc) )
   {
      /* nothing to send - error? */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, Program arguments not available"));
      return IFX_SUCCESS;
   }

   /* for each program argument */
   for (i = 0; i < g_pITM_Ctrl->nArgc; ++i)
   {
      /* clear string */
      memset(pszITM_Message, 0, MAX_ITM_MESSAGE_TO_SEND);
      /* set message string */
      snprintf(pszITM_Message, MAX_ITM_MESSAGE_TO_SEND,
               "ARGV:{{NO %d} {STR {%s}}}%s",
               i, g_pITM_Ctrl->rgsArgv[i],
               COM_MESSAGE_ENDING_SEQUENCE);
      /* send message string */
      COM_SendMessage(g_pITM_Ctrl->nComOutSocket, pszITM_Message,
                      TD_CONN_ID_ITM);
   }

   return IFX_SUCCESS;
} /* COM_SendSutArgv() */

/**
   Get data from key/value field from communication message.

   \param  pITM_MsgHead  - pointer to ITM message data
   \param  pUnsInt   - pointer to copy data from message
   \param  nRep      - number of key/value pair
   \param  ntype     - type of field key or value

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_GetMsgData(ITM_MSG_HEADER_t* pITM_MsgHead, IFX_uint32_t* pUnsInt,
                            IFX_uint32_t nRep, ITM_MSG_DATA_TYPE_t nType)
{
   IFX_int32_t nSize, nOffset, i;
   /* check input parameters */
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead->pData, IFX_ERROR);
   TD_PTR_CHECK(pUnsInt, IFX_ERROR);
   TD_PARAMETER_CHECK((nRep >= pITM_MsgHead->nRepetition), nRep, IFX_ERROR);

   /* get offset of key and value pair*/
   nOffset = nRep * pITM_MsgHead->nDataLen;
   if (ITM_MSG_DATA_TYPE_KEY == nType)
   {
      /* offset of last byte of Key field */
      nOffset += pITM_MsgHead->nKeyLen - 1;
      nSize = pITM_MsgHead->nKeyLen;
   }
   else if (ITM_MSG_DATA_TYPE_VALUE == nType)
   {
      /* offset of last byte of Value field */
      nOffset += pITM_MsgHead->nDataLen - 1;
      nSize = pITM_MsgHead->nValueLen;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: message ID: %d, type %d, key %d, val %d, rep %d -"
             " invalid type parameter. (File: %s, line: %d)\n",
             pITM_MsgHead->nId, nType, pITM_MsgHead->nKeyLen,
             pITM_MsgHead->nValueLen, nRep, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* can only read data from less than 4 bytes - sizeof(IFX_uint32_t) */
   if (sizeof(IFX_uint32_t) < nSize)
   {
      nSize = sizeof(IFX_uint32_t);
   }
   else if (0 >= nSize)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: message ID: %d, type %d, key %d, val %d, rep %d -"
             " invalid key/value parameter. (File: %s, line: %d)\n",
             pITM_MsgHead->nId, nType, pITM_MsgHead->nKeyLen,
             pITM_MsgHead->nValueLen, nRep, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* reset data */
   *pUnsInt = 0;
   /* get number from array */
   for (i = 0; i < nSize; i++)
   {
      *pUnsInt += (IFX_uint32_t)
         (((IFX_uint8_t)pITM_MsgHead->pData[nOffset - i]) << (ONE_BYTE * i));
   }

   return IFX_SUCCESS;
}

/**
   Get pointer to key/value field from communication message.

   \param  pITM_MsgHead  - pointer to ITM message data
   \param  nRep      - number of key/value pair
   \param  ntype     - type of field key or value

   \return IFX_NULL if error or pointer to data
   \remark
*/
IFX_char_t* COM_GetMsgDataPointer(ITM_MSG_HEADER_t* pITM_MsgHead,
                                  IFX_uint32_t nRep,
                                  ITM_MSG_DATA_TYPE_t nType)
{
   IFX_int32_t nOffset;

   /* check input parameters */
   TD_PTR_CHECK(pITM_MsgHead, IFX_NULL);
   TD_PTR_CHECK(pITM_MsgHead->pData, IFX_NULL);
   TD_PARAMETER_CHECK((nRep >= pITM_MsgHead->nRepetition), nRep, IFX_NULL);

   /* get offset of key and value pair*/
   nOffset = nRep * pITM_MsgHead->nDataLen;
   if (ITM_MSG_DATA_TYPE_KEY == nType)
   {
      /* offset of first byte of Key field - no aditional offset is needed */
   }
   else if (ITM_MSG_DATA_TYPE_VALUE == nType)
   {
      /* offset of first byte of Value field */
      nOffset += pITM_MsgHead->nKeyLen;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: message ID: %d, type %d, key %d, val %d, rep %d -"
             " invalid type parameter. (File: %s, line: %d)\n",
             pITM_MsgHead->nId, nType, pITM_MsgHead->nKeyLen,
             pITM_MsgHead->nValueLen, nRep, __FILE__, __LINE__));
      return IFX_NULL;
   }
   /* address of key/data field */
   return &pITM_MsgHead->pData[nOffset];
}

/**
   Get pointer to key/value field from communication message.

   \param  pITM_MsgHead  - pointer to ITM message data
   \param  buf       - response message buffer

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_RunCommand(ITM_MSG_HEADER_t* pITM_MsgHead, IFX_char_t* buf)
{
   /*Using in loops*/
   IFX_uint32_t i, j=0;
   IFX_return_t nRet = IFX_SUCCESS;
   /*Check when respond*/
   IFX_uint32_t nRespond = RUN_COMMAND_RESONSE_AFTER_EXECUTION, nKey, nValue;

   /*Array of arguments (using in execvp)*/
   IFX_char_t *paArguments[RUN_COMMAND_MAX_ARGUMENTS] = {0};
   IFX_char_t rgFullCmd[MAX_CMD_LEN] = {0};
   IFX_char_t rgCmdOutput[MAX_CMD_LEN];

   /* check input parameters */
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead->pData, IFX_ERROR);
   TD_PTR_CHECK(buf, IFX_ERROR);

   /* Run program.
      Key: type of value (increases by 1 from 0 for every key/value pair
           in message).
      Value: first value is responsible for when to send response(after or
             before evecution), second value is program name, all aditional
             values are program parameters. */
   for (i=0; i<pITM_MsgHead->nRepetition; i++)
   {
      /* get key */
      if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nKey, i,
                                      ITM_MSG_DATA_TYPE_KEY))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get key value. "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check key value */
      if (nKey != i)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Wrong key value %d, should be %d. "
                "(File: %s, line: %d)\n", nKey, i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* check when to send response */
      if (RUN_COMMAND_SET_RESPONSE == nKey)
      {
         if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nValue, i,
                                         ITM_MSG_DATA_TYPE_VALUE))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Failed to get key value."
                   " (File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         if (0 == (nValue%2))
         {
            nRespond = RUN_COMMAND_RESONSE_BEFORE_EXECUTION;
         }
         else
         {
            nRespond = RUN_COMMAND_RESONSE_AFTER_EXECUTION;
         }
      }
      else
      {
         /* get program name and arguments */
         paArguments[j] = COM_GetMsgDataPointer(pITM_MsgHead, i,
                                                ITM_MSG_DATA_TYPE_VALUE);
         if (IFX_NULL == paArguments[j])
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Failed to get program argument(%d). "
                   "(File: %s, line: %d)\n", j, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         if (pITM_MsgHead->nValueLen <= strlen(paArguments[j]))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Program arguments(%d) too long. "
                   "(File: %s, line: %d)\n", j, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         if (MAX_CMD_LEN < strlen(rgFullCmd) + strlen(paArguments[j]) + 1)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Command line is too long too long. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         strcat(rgFullCmd, paArguments[j]);
         strcat(rgFullCmd, " ");
         j++;
         /* check if valid number of arguments */
         if (j >= RUN_COMMAND_MAX_ARGUMENTS)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Too many program arguments(%d). "
                   "(File: %s, line: %d)\n", j, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         paArguments[j] = IFX_NULL;
      }
   }

   /*Send respond or run program (depend on nRespond value) */
   if (RUN_COMMAND_RESONSE_BEFORE_EXECUTION == nRespond)
   {
      sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME RUN_EXTERNAL_CMD} "
              "{COMMAND_RESPONSE OK} }%s",
              COM_MESSAGE_ENDING_SEQUENCE);
      COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
   }

   if ( IFX_SUCCESS != COM_ExecuteShellCommand(rgFullCmd, rgCmdOutput))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: COM_ExecuteShellCommand().\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if ( 0 != strlen(rgCmdOutput) )
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
             ("Output:\n%s", rgCmdOutput));
   }

   /* check if message should be sent */
   if(RUN_COMMAND_RESONSE_AFTER_EXECUTION == nRespond)
   {
      /*Prepare message to send*/
      sprintf(buf,"SUTS_RESPONSE:{{COMMAND_NAME RUN_EXTERNAL_CMD} "
               "{COMMAND_RESPONSE %s} }%s",
              IFX_SUCCESS == nRet ? "OK" : "FAIL",
              COM_MESSAGE_ENDING_SEQUENCE);
      COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
   }
   else
   {
      /* check if first element of array was initialized */
      if (0 != paArguments[0])
      {
         if( !strcmp(paArguments[0],"ifconfig"))
         {
            /* reset communication sockets - IP address has changed */
            g_pITM_Ctrl->bCommunicationReset = IFX_TRUE;
            g_pITM_Ctrl->bChangeIPAddress = IFX_TRUE;
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                  ("Address of board will be changed\n"));
            /* save new IP address */
            sprintf(g_pITM_Ctrl->oVerifySytemInit.aParametrs,
                    "%s", paArguments[2]);
         }
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Progam name was not set.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }

   }
   return nRet;
}

/**
   Send information to control PC that one of the boards is during initialization.

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_SendSysInitProceed(IFX_int32_t nBoardNo)
{
   IFX_char_t buf[45];

   sprintf(buf, "SYSINIT_PROCEED:{{BOARD_NO %d}}%s",
           nBoardNo, COM_MESSAGE_ENDING_SEQUENCE);

   return COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
} /* COM_SendSysInitProceed() */

/**
   Send information to control PC that one of the boards ended initialization.

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_SendSysInitHalt(IFX_int32_t nBoardNo)
{
   IFX_char_t buf[45];

   sprintf(buf, "SYSINIT_HALT:{{BOARD_NO %d}}%s",
           nBoardNo, COM_MESSAGE_ENDING_SEQUENCE);

   return COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
} /* COM_SendSysInitHalt() */

/**
   Send information to control PC that all boards are initialized.

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_SendSysInitDone()
{
   IFX_char_t buf[24];

   sprintf(buf, "SYSINIT_DONE:{ }%s", COM_MESSAGE_ENDING_SEQUENCE);

   return COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
} /* COM_SendSysInitDone() */

/**
   Handle dialing event for active state (FXS state machine) for conference test,
   basically report detected digit number.

   \param  pPhone - pointer to PHONE

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/

IFX_return_t COM_ST_FXS_Active_Dialing_Handle_Conference(PHONE_t *pPhone)
{
   IFX_char_t acBuf[MAX_CMD_LEN];
   /* check input parameters */
   TD_PARAMETER_CHECK((NO_SOCKET == g_pITM_Ctrl->nComOutSocket),
                      g_pITM_Ctrl->nComOutSocket, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pPhone->nDialNrCnt),
                      pPhone->nDialNrCnt, IFX_ERROR);

   /* clear string */
   memset(acBuf, 0, MAX_CMD_LEN);
   /* set message string */
   snprintf(acBuf, MAX_CMD_LEN,
            "DTMF_DIGIT:{ "
              " PHONE_NO {%d} "
              " DIGIT {%d} "
           "}%s",
            pPhone->nPhoneNumber, pPhone->pDialedNum[pPhone->nDialNrCnt],
            COM_MESSAGE_ENDING_SEQUENCE);

   /* dialed digit was handled - reset buffer */
   pPhone->nDialNrCnt = 0;
   pPhone->pDialedNum[0] = '\0';
   pPhone->pDialedNum[1] = '\0';

   /* send message string */
   return COM_SendMessage(g_pITM_Ctrl->nComOutSocket, acBuf,
                          pPhone->nSeqConnId);
}

/**
   Extract device no. and channel no. from ITM message.
   This function can be used for following message types:
      - COM_ID_SET_CH_INIT_VOICE
      - COM_ID_SET_LINE_FEED_SET_STANDBY
      - COM_ID_SET_LINE_FEED_SET_ACTIVE

   \param pITM_MsgHead - pointer to ITM message structure

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_ExtractDevAndChannelFromMsg(ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_uint32_t nKey = 0, nValue = 0;
   IFX_int32_t ret = IFX_ERROR;

   if ( g_pITM_Ctrl->oVerifySytemInit.fEnabled )
   {
      ret = IFX_SUCCESS;

      /* check if key length is zero (previous version of message,
         without device no.) */
      if (0 == pITM_MsgHead->nKeyLen)
      {
         /* old message version - without key, use device no. 0 */
         nKey = 0;
      }
      else
      {
         /* device no. available, get it */
         if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nKey, 0,
                                         ITM_MSG_DATA_TYPE_KEY))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Failed to get device number from message. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            ret = IFX_ERROR;
         }
      }

      /* get channel no. (if device no. extracted successfully) */
      if ( (IFX_SUCCESS == ret)
           && (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nValue, 0,
                                           ITM_MSG_DATA_TYPE_VALUE)) )
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get channel number from message. "
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         ret = IFX_ERROR;
      }

      /* save device no. and channel no. */
      if (IFX_SUCCESS == ret)
      {
         /* copy device number and data channel number */
         sprintf(g_pITM_Ctrl->oVerifySytemInit.aParametrs,
                 "%u;%u", nKey, nValue);
      }
   } /* if ( nVerifySystemInitializtion ) */

   return ret;
} /* COM_GetDevNoAndChannelNoFromMsg */

/**
   Get capability value.

   \param pBoard  - pointer to board structure.
   \param nDev - device number
   \param nCapType - Capability type, see \ref IFX_TAPI_CAP_TYPE_t.

   \return Capability value. Value -1 is returned if error occured.
*/
IFX_int32_t COM_GetCapabilityValue(BOARD_t* pBoard,
                                   IFX_int32_t nDev,
                                   IFX_TAPI_CAP_TYPE_t nCapType)
{
   IFX_int32_t nCount = -1;
   IFX_TAPI_CAP_t oCapParam;
   IFX_int32_t nRet = IFX_ERROR;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

#ifdef TAPI_VERSION4
   if (pBoard->fSingleFD)
   {
      /* Check device number */
      if ( (0 > nDev) || (pBoard->nChipCnt <= nDev) )
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Invalid device no. %i for board no. %i."
                " Number of devices on board is %i."
                " (File: %s, line: %d)\n",
                nDev, pBoard->nBoard_IDX, pBoard->nChipCnt,
                __FILE__, __LINE__));
         return -1;
      }
   }
   else
#endif
   {
      /* Multiple devices on board not supported. Verify device number. */
      if (0!=nDev)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, No support for multiple devices on single board,"
                " but requested number of coder channels"
                " for device no. %i on board no. %i."
                " (File: %s, line: %d)\n",
                nDev, pBoard->nBoard_IDX, __FILE__, __LINE__));
         return -1;
      }
   }


   memset(&oCapParam, 0, sizeof(IFX_TAPI_CAP_t));
   oCapParam.captype = nCapType;
#ifdef TAPI_VERSION4
   if (pBoard->fSingleFD)
   {
      oCapParam.dev = nDev;
   }
#endif /* TAPI_VERSION4 */

   nRet = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_CAP_CHECK,
                   (IFX_uint32_t) &oCapParam, nDev, TD_CONN_ID_ITM);

   if (0 < nRet)
   {
      /* ok */
      nCount = oCapParam.cap;
   }
   else if(0 == nRet)
   {
      /* capability not supported */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("Capability %s (%d) not supported on board %d device %d\n",
             Common_Enum2Name(oCapParam.captype, TD_rgCapType),
             oCapParam.captype, pBoard->nBoard_IDX, nDev));
      nCount = 0;
   }
   else
   {
      /* error */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ioctl IFX_TAPI_CAP_CHECK "
             "(capability type %s (%d), board %d, device %d). "
             "(File: %s, line: %d)\n",
             Common_Enum2Name(oCapParam.captype, TD_rgCapType),
             oCapParam.captype, pBoard->nBoard_IDX, nDev,
             __FILE__, __LINE__));
   }

   return nCount;
} /* COM_GetCapabilityValue */

/**
   Get data from message from control PC: board no., device no., channel no.
   and value.

   \param pITM_MsgHead
   \param nKey - key of board number (first item to read)
   \param pBoardNo  - pointer to copy board number from message
   \param pDevNo - pointer to copy device number from message
   \param pChannelNo - pointer to copy channel number from message
   \param pValue - pointer to copy value from message

   \return IFX_SUCCESS if all values read from message,
           IFX_ERROR in case of failure
*/
IFX_int32_t COM_GetChDataFromITM_Msg(ITM_MSG_HEADER_t* pITM_MsgHead,
                                     IFX_int32_t nKey,
                                     IFX_uint32_t* pBoardNo,
                                     IFX_uint32_t* pDevNo,
                                     IFX_uint32_t* pPhoneChNo,
                                     IFX_uint32_t* pValue)
{
   IFX_uint32_t nValueId = 0;

   /* check input parameters */
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nKey), nKey, IFX_ERROR);
   TD_PTR_CHECK(pBoardNo, IFX_ERROR);
   TD_PTR_CHECK(pDevNo, IFX_ERROR);
   TD_PTR_CHECK(pPhoneChNo, IFX_ERROR);
   TD_PTR_CHECK(pValue, IFX_ERROR);

   /* reset variables */
   *pBoardNo = 0;
   *pDevNo = 0;
   *pPhoneChNo = 0;
   *pValue = 0;

   /* *************************************************************
   * get board
   ***************************************************************/
   /* check key value */
   if (nKey >= pITM_MsgHead->nRepetition)
   {
      /* key out of range */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: message key (%d) out of range (%d). "
             "(File: %s, line: %d)\n",
             nKey, pITM_MsgHead->nRepetition, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get key - value number, must be the same as variable nKey */
   if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nValueId, nKey,
                                   ITM_MSG_DATA_TYPE_KEY))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get value id. "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* verify value id */
   if (nKey != nValueId)
   {
      /* error, some key-value pairs are missing? */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: value id doesn't match field position in message. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get board number from value field */
   if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, pBoardNo, nKey,
                                   ITM_MSG_DATA_TYPE_VALUE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get board no."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* go to next key-value pair */
   ++nKey;

   /* *************************************************************
   * get device
   ***************************************************************/
   /* check key value */
   if (nKey >= pITM_MsgHead->nRepetition)
   {
      /* key out of range */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: message key (%d) out of range (%d)."
             "  (File: %s, line: %d)\n",
             nKey, pITM_MsgHead->nRepetition, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get key - value number, must be the same as variable nKey */
   if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nValueId, nKey,
                                   ITM_MSG_DATA_TYPE_KEY))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get value id."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* verify value id */
   if (nKey != nValueId)
   {
      /* error, some key-value pairs are missing? */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: value id doesn't match field position "
             "in message. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get device number from value field */
   if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, pDevNo, nKey,
                                   ITM_MSG_DATA_TYPE_VALUE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get device no."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* go to next key-value pair */
   ++nKey;

   /* *************************************************************
   * get phone channel
   ***************************************************************/
   /* check key value */
   if (nKey >= pITM_MsgHead->nRepetition)
   {
      /* key out of range */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: message key (%d) out of range (%d)."
             "  (File: %s, line: %d)\n",
             nKey, pITM_MsgHead->nRepetition, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get key - value number, must be the same as variable nKey */
   if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nValueId, nKey,
                                   ITM_MSG_DATA_TYPE_KEY))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get value id."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* verify value id */
   if (nKey != nValueId)
   {
      /* error, some key-value pairs are missing? */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: value id doesn't match field position "
             "in message. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get phone channel from value field */
   if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, pPhoneChNo, nKey,
                                   ITM_MSG_DATA_TYPE_VALUE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get phone channel no."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* go to next key-value pair */
   ++nKey;

   /* *************************************************************
   * get value
   ***************************************************************/
   /* check key value */
   if (nKey >= pITM_MsgHead->nRepetition)
   {
      /* key out of range */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: message key (%d) out of range (%d)."
             "  (File: %s, line: %d)\n",
             nKey, pITM_MsgHead->nRepetition, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get key - value number, must be the same as variable nKey */
   if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, &nValueId, nKey,
                                   ITM_MSG_DATA_TYPE_KEY))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get value id."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* verify value id */
   if (nKey != nValueId)
   {
      /* error, some key-value pairs are missing? */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: value id doesn't match field position "
             "in message. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get value from value field */
   if (IFX_ERROR == COM_GetMsgData(pITM_MsgHead, pValue, nKey,
                                   ITM_MSG_DATA_TYPE_VALUE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get tone no."
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

/**
   Print text from ITM message.

   \param  pCtrl     - pointer to control structure
   \param  pITM_MsgHead  - pointer to ITM message data

   \return IFX_SUCCESS or IFX_ERROR
   \remark
*/
IFX_return_t COM_PrintTxt(CTRL_STATUS_t* pCtrl, ITM_MSG_HEADER_t* pITM_MsgHead)
{
   IFX_char_t *pTxt;
   IFX_int32_t nRet = IFX_SUCCESS;
   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead->pData, IFX_ERROR);

   /* get text */
   pTxt = COM_GetMsgDataPointer(pITM_MsgHead, 0,
                                ITM_MSG_DATA_TYPE_VALUE);
   if (IFX_NULL != pTxt)
   {
      /* print text */
      TAPIDEMO_PRINTF(TD_CONN_ID_ITM, ("%s\n", pTxt));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed to get text."
             " (File: %s, line: %d)\n", __FILE__, __LINE__));
      nRet = IFX_ERROR;
   }

   return nRet;
}

/**
   Send information about phones and FXOs.

   \param  pCtrl     - pointer to control structure

   \return IFX_SUCCESS or IFX_ERROR
   \remark
*/
IFX_return_t COM_InfoGet(CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t nBoard, nPhone;
   PHONE_t* pPhone;
   IFX_char_t buf[TD_COM_SEND_MSG_SIZE];
#ifndef EASY336
   IFX_int32_t nPhoneTmpUdpPort;
#endif /* EASY336 */
#ifdef FXO
   FXO_t* pFXO;
   IFX_int32_t nFxoNum;
#endif /* FXO */

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* for all boards */
   for (nBoard=0; nBoard < pCtrl->nBoardCnt; nBoard++)
   {
      /* for all phones */
      for (nPhone=0; nPhone < pCtrl->rgoBoards[nBoard].nMaxPhones; nPhone++)
      {
         /* get phone pointer */
         pPhone = &pCtrl->rgoBoards[nBoard].rgoPhones[nPhone];
#ifndef EASY336
         /* check if data channels are available */
         if (0 < pPhone->pBoard->nMaxCoderCh)
         {
            /* get udp port used for external voip calls */
            if (pCtrl->pProgramArg->oArgFlags.nQos)
            {
               nPhoneTmpUdpPort = VOICE_UDP_QOS_PORT + pPhone->nDataCh * 2 +
                  pPhone->pBoard->nBoard_IDX;
            }
            else
            {
               nPhoneTmpUdpPort = VOIP_GetSocketPort(pPhone->nSocket, pCtrl);
            }
         } /* check if data channels are available */
         else
         {
            /* no default UDP port */
            nPhoneTmpUdpPort = TD_NOT_SET;
         } /* check if data channels are available */
#endif /* EASY336 */
         /* set message text */
         sprintf(buf,
                 "INFO_PHONE:{ "
                    " PHONE_NO {%d} "
                    " PHONE_CH_NO {%d} "
                    " DEV_NO {%d} "
                    " BOARD_TYPE {%s} "
                    " DEV_FILE {%s} "
                    " BOARD_NO {%d} "
                    " BOARD_NAME {%s} "
                    " CID_NAME {%s} "
                    " PHONE_STATE {%s} "
                    " DATA_CH {%d} "
                    " UDP_PORT {%d} "
                    " PCM_CH {%d} "
                    " DECT_CH {%d} "
                 "}%s",
                 pPhone->nPhoneNumber,
                 pPhone->nPhoneCh,
                 pPhone->nDev,
                 Common_Enum2Name(pPhone->pBoard->nType,
                                  TD_rgBoardTypeName),
#ifdef TAPI_VERSION4
                 /* for TAPI_VERSION4 pPhone->pBoard->pszChDevName is not set */
                 pPhone->pBoard->pszCtrlDevName,
#else /* TAPI_VERSION4 */
                 pPhone->pBoard->pszChDevName,
#endif /* TAPI_VERSION4 */
                 pPhone->pBoard->nBoard_IDX,
                 pPhone->pBoard->pszBoardName,
                 (IFX_NULL != pPhone->pCID_Msg) ?
                 (IFX_char_t*)
                 pPhone->pCID_Msg->message[TD_CID_IDX_NAME].string.element :
                 "",
                 Common_Enum2Name(pPhone->rgStateMachine[FXS_SM].nState,
                                  TD_rgStateName),
#ifndef EASY336
                 pPhone->nDataCh,
                 nPhoneTmpUdpPort,
                 pPhone->nPCM_Ch,
#else
                 TD_NOT_SET,
                 TD_NOT_SET,
                 TD_NOT_SET,
#endif /* EASY */
#ifdef TD_DECT
                 pPhone->nDectCh,
#else /* TD_DECT */
                 TD_NOT_SET,
#endif /* TD_DECT */
                 COM_MESSAGE_ENDING_SEQUENCE);
         COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
      }
   }

#ifdef FXO
   /* for all fxo channels */
   for (nFxoNum = 1; nFxoNum <= pCtrl->nMaxFxoNumber; nFxoNum++)
   {
      /* get fxo with given number */
      pFXO = ABSTRACT_GetFxo(nFxoNum, pCtrl);
      if (IFX_NULL == pFXO)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get FXO No %d."
                " (File: %s, line: %d)\n", nFxoNum, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* set message text */
      sprintf(buf,
              "INFO_FXO:{ "
                 " FXO_NO {%d} "
                 " FXO_CH_NO {%d} "
                 " FXO_DEV_NO {%d} "
                 " FXO_DEV_NAME {%s} "
                 " FXO_DEV_FILE {%s%s} "
                 " FXO_STATE {%s} "
              "}%s",
              pFXO->nFXO_Number,
              pFXO->nFxoCh,
              pFXO->pFxoDevice->nDevNum,
              pFXO->pFxoDevice->pFxoTypeName,
              TD_DEV_PATH, Common_Enum2Name(pFXO->pFxoDevice->nFxoType,
                                            TD_rgFxoDevName),
              Common_Enum2Name(pFXO->nState, TD_rgStateName),
              COM_MESSAGE_ENDING_SEQUENCE);
      COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, TD_CONN_ID_ITM);
   } /* for (nFxoNum = 1; nFxoNum <= pCtrl->nMaxFxoNumber; nFxoNum++) */
#endif /* FXO */

   return IFX_SUCCESS;
}

#ifdef TAPI_VERSION4
/**
   Stop device.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t COM_DevStop(BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_uint32_t i = 0;
   IFX_TAPI_DEV_START_CFG_t oParamConfig;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* reset TAPI for each device */
   for (i = 0; i < pBoard->nChipCnt; ++i)
   {
      memset(&oParamConfig, 0, sizeof(IFX_TAPI_DEV_START_CFG_t));
      oParamConfig.dev = i;

      ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_DEV_STOP, &oParamConfig,
                      oParamConfig.dev, TD_CONN_ID_ITM);

      if (IFX_SUCCESS != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, Failed to stop, board %d, dev %d."
                " (File: %s, line: %d)\n",
                pBoard->nBoard_IDX, i,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* for (i = 0; i < pBoard->nChipCnt; ++i) */

   return IFX_SUCCESS;
}
#endif /* TAPI_VERSION4 */

/**
   Execute shell command and set output to string.

   \param  pCmd         - shell command
   \param  pCmdOutput  -  output

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark Shell command's output must be set to string, because
           it must be printed via macro TD_TRACE. If it's printed directly on screen,
           it'll be invisible for TCL script.
*/
IFX_return_t COM_ExecuteShellCommand(IFX_char_t* pCmd, IFX_char_t* pCmdOutput)
{
   TD_OS_File_t *nPipeFd;
   IFX_char_t rgInBuffor[100];

   /* Check input parameters */
   TD_PTR_CHECK(pCmd, IFX_ERROR);
   TD_PTR_CHECK(pCmdOutput, IFX_ERROR);

   strcpy(pCmdOutput, "");

   /* The command argument for 'popen' is a pointer to a null-terminated string
      containing a shell command line. This command is passed to /bin/sh
      using the -c flag; interpretation, if any, is performed by the shell.
      The output from shell command line might be read from pipe */
   nPipeFd = popen(pCmd, "r");
   if (!nPipeFd)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Failed command 'popen'.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Read ouput from pipe */
   while (TD_OS_FGets(rgInBuffor, sizeof(rgInBuffor), nPipeFd))
   {
      if (MAX_CMD_LEN <= strlen(rgInBuffor) + strlen(pCmdOutput))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Buffor for cmd '%s' output is too small.\n"
                "(File: %s, line: %d)\n", pCmd, __FILE__, __LINE__));
         pclose(nPipeFd);
         return IFX_ERROR;
      }
      strcat(pCmdOutput, rgInBuffor);
   }
   pclose(nPipeFd);

   return IFX_SUCCESS;
} /* COM_ExecuteShellCommand */

/**
   Add new entry to ARP table.

   \param  pCtrl     - pointer to control structure
   \param  pITM_MsgHead  - pointer to ITM message data
   \param  buf       - response message buffer

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t COM_AddArpEntry(CTRL_STATUS_t* pCtrl, ITM_MSG_HEADER_t* pITM_MsgHead,
                             IFX_char_t* buf)
{
   /* Using in loops */
   IFX_uint32_t i;
   IFX_return_t nRet = IFX_SUCCESS;

   IFX_char_t* pRcvData;
   /* Keep receive IP address */
   IFX_uint8_t oIP[4];
   IFX_char_t rgRcvIP[TD_IP_ADDRESS_LEN];
   /* Keep receive MAC */
   IFX_uint8_t oMAC[ETH_ALEN];
   IFX_char_t rgRcvMAC[MAC_ADDRESS_LEN];

   /* Command name to add ARP entry */
   IFX_char_t* pCmd = "arp -s";
   IFX_char_t rgFullCmd[50];
   IFX_char_t rgCmdOutput[MAX_CMD_LEN];

   /* Check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead, IFX_ERROR);
   TD_PTR_CHECK(pITM_MsgHead->pData, IFX_ERROR);
   TD_PTR_CHECK(buf, IFX_ERROR);

   /* Add ARP entry.
      Key: IP address.
      Value: MAC address. */

   for (i = 0; i < pITM_MsgHead->nRepetition; i++)
   {
      /* Get IP address */
      pRcvData = COM_GetMsgDataPointer(pITM_MsgHead, 0,
                                       ITM_MSG_DATA_TYPE_KEY);
      if(IFX_NULL == pRcvData)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get IP address to ARP entry(%d).\n"
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         nRet = IFX_ERROR;
         break;
      }
      memcpy(oIP, pRcvData, 4);
      snprintf(rgRcvIP, TD_IP_ADDRESS_LEN, "%d.%d.%d.%d",
               oIP[0], oIP[1], oIP[2], oIP[3]);

      /* Get MAC address */
      pRcvData = COM_GetMsgDataPointer(pITM_MsgHead, 0,
                                       ITM_MSG_DATA_TYPE_VALUE);
      if(IFX_NULL == pRcvData)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to get MAC to ARP entry(%d).\n"
                "(File: %s, line: %d)\n", i, __FILE__, __LINE__));
         nRet = IFX_ERROR;
         break;
      }
      memcpy(oMAC, pRcvData, ETH_ALEN);
      snprintf(rgRcvMAC, MAC_ADDRESS_LEN, "%02X:%02X:%02X:%02X:%02X:%02X",
               oMAC[0], oMAC[1], oMAC[2], oMAC[3], oMAC[4], oMAC[5]);

      /* Add 3, because there're spaces between strings */
      if (sizeof(rgFullCmd) <=
          (strlen(pCmd) + strlen(rgRcvIP) + strlen(rgRcvMAC)+3))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Failed to build command for adding ARP entry.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         nRet = IFX_ERROR;
         break;
      }
      sprintf(rgFullCmd,"%s %s %s", pCmd, rgRcvIP, rgRcvMAC);

      /* Add new ARP entry using command 'arp' */
      if ( IFX_SUCCESS != COM_ExecuteShellCommand(rgFullCmd, rgCmdOutput))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: COM_ExecuteShellCommand().\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         nRet = IFX_ERROR;
         break;
      }
      if ( 0 != strlen(rgCmdOutput) )
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                ("Output for cmd '%s':\n%s", rgFullCmd, rgCmdOutput));
      }

      if ( IFX_SUCCESS != COM_ExecuteShellCommand("arp -n", rgCmdOutput))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: COM_ExecuteShellCommand().\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         nRet = IFX_ERROR;
         break;
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
             ("ARP table:\n%s", rgCmdOutput));
      nRet = IFX_SUCCESS;

   } /*  for */

   return nRet;
} /* COM_AddArpEntry */

/**
   Send to control PC message about failed usage of custom download path.

   \param none

   \return IFX_void_t
*/
IFX_void_t COM_ReportErrorDwlPath(IFX_void_t)
{
   IFX_char_t buf[TD_COM_SEND_MSG_SIZE];

   /* set message text */
   sprintf(buf,
           "ERR_INFO:{ "
              " SET_DOWNOLAD_PATH_FAILED {} "
           "}%s",
           COM_MESSAGE_ENDING_SEQUENCE);

   /* send msg, if fails print error trace */
   if (IFX_SUCCESS != COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf,
                                      TD_CONN_ID_ITM))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: COM_SendMessage() failed "
             "to report download path errot.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
   }

   return;
}

/**
   Clean up after ITM

   \param none

   \return IFX_void_t
*/
IFX_void_t COM_CleanUp(IFX_void_t)
{
   COM_CleanUpComSockets();
   /* clean up CID */
   if (IFX_NULL != g_pITM_Ctrl->oCID_Test.pCID_Msg)
   {
      if (IFX_NULL != g_pITM_Ctrl->oCID_Test.pCID_Msg->message)
      {
         TD_OS_MemFree(g_pITM_Ctrl->oCID_Test.pCID_Msg->message);
         g_pITM_Ctrl->oCID_Test.pCID_Msg->message = IFX_NULL;
      }
      TD_OS_MemFree(g_pITM_Ctrl->oCID_Test.pCID_Msg);
      g_pITM_Ctrl->oCID_Test.pCID_Msg = IFX_NULL;
   }
   /* clean up file (BBD, FW...) data */
   COM_RcvFileCleanUp();
   return;
}

/**

   Restarts TAPI for different devices. 

   \param  pCtrl   - pointer to control structure
   \param  pBoard  - pointer to board structure

   \return status (IFX_SUCCESS or IFX_ERROR)

*/
IFX_return_t COM_TapiRestart(CTRL_STATUS_t* pCtrl, BOARD_t* pBoard)
{
   IFX_return_t nRet = IFX_ERROR;
#ifndef TD_USE_CHANNEL_INIT
#ifdef DXT
#if defined(EASY3201) || defined(EASY3201_EVS)
   DXT_BBD_Download_t oBBD_dwnld = {0};
   IFX_int32_t i = 0;
   TAPIDEMO_DEVICE_CPU_t  *pCpuDevice = IFX_NULL;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
#endif /* defined(EASY3201) || defined(EASY3201_EVS) */
#endif /* DXT */
#endif /* TD_USE_CHANNEL_INIT */
   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

#ifdef TD_USE_CHANNEL_INIT
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
           ("Err, Compiled without DevStart support."
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
#elif defined(HAVE_DRV_VMMC_HEADERS)
   nRet = DEVICE_Vmmc_SetupChannelUseDevStart(
             pBoard,
             pCtrl->pProgramArg->sPathToDwnldFiles);
#elif defined (DXT)
#if defined(EASY3201) || defined(EASY3201_EVS)
   /* Basic initialization is needed by device to setup channels */
   /* Implicitly called in tests */

   if ((pDevice = Common_GetDevice_CPU(pBoard->pDevice)) == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, CPU device not found. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pCpuDevice = &pDevice->uDevice.stCpu;

   if(IFX_SUCCESS != BOARD_Easy3201_InitSystem(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, Board_Easy_3201_InitSystem failed. "
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* setup channels */
   nRet = DEVICE_DUSLIC_XT_setupChannel(
            pBoard,
            pCtrl->pProgramArg->sPathToDwnldFiles,
            IFX_FALSE);

   if (IFX_ERROR == nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
              ("Err, DEVICE_DUSLIC_XT_setupChannel failed. "
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Initialize all tapi channels */
   for (i = 0; i < pBoard->nMaxAnalogCh; i++)
   {
      /* Download BBD */
      oBBD_dwnld.buf = ((DXT_IO_Init_t*)pCpuDevice->pDevDependStuff)->pBBDbuf;
      oBBD_dwnld.size = ((DXT_IO_Init_t*)pCpuDevice->pDevDependStuff)->bbd_size;
      if (IFX_NULL != oBBD_dwnld.buf)
      {
         if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[i], FIO_DXT_BBD_DOWNLOAD,
                                     &oBBD_dwnld,
                                     TD_DEV_NOT_SET, TD_CONN_ID_INIT))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, Failed BBD download on ch %d. (File: %s, line: %d)\n",
                   i, __FILE__, __LINE__));
            nRet = IFX_ERROR;
            break;
         }
      }
   }
#endif /* defined(EASY3201) || defined(EASY3201_EVS) */
#elif (defined(EASY336) || defined(XT16))
   switch (pBoard->nType)
   {
#ifdef EASY336
      case TYPE_SVIP:
      {
         nRet = COM_DevStop(pBoard);
         if (IFX_SUCCESS == nRet)
         {
            /* device stopped */
            nRet = DEVICE_SVIP_SetupChannelUseDevStart(
                        pBoard,
                        pCtrl->pProgramArg->sPathToDwnldFiles);
         }
         break;
      }
#endif /* EASY336 */
#if (defined(XT16) || defined(WITH_VXT))
      case TYPE_XT16:
      {
         nRet = COM_DevStop(pBoard);
         if (IFX_SUCCESS == nRet)
         {
            /* device stopped */
            nRet = DEVICE_VXT_SetupChannelUseDevStart(
                        pBoard,
                        pCtrl->pProgramArg->sPathToDwnldFiles);
         }
         break;
      }
#endif /* (defined(XT16) || defined(WITH_VXT)) */
      default:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
         ("Err, unexpected board type %d in system"
         " init (expected"
#ifdef EASY336
         " SVIP,"
#endif /* EASY336 */
#if (defined(XT16) || defined(WITH_VXT))
         " xT16"
#endif /* (defined(XT16) || defined(WITH_VXT)) */
         "). "
         "(File: %s, line: %d)\n",
         pBoard->nType, __FILE__, __LINE__));
         break;
      }
   }
#endif /* TD_USE_CHANNEL_INIT */
   return nRet;
}
#endif /* LINUX */

/**
   Set default values for ITM control structure.

   \param pCtrl      - ITM control structure
   \param nArgc      - number of program arguments
   \param pArgv      - array of pointers with program arguments
   \param pTD_Version   - TAPIDEMO version name
   \param pITM_Version  - ITM version name

   \return IFX_void_t
*/
IFX_void_t COM_SetDefault(TD_ITM_CONTROL_t* pITM_Ctrl,
                          IFX_int32_t nArgc, IFX_char_t **pArgv,
                          const IFX_char_t* const pTD_Version,
                          IFX_char_t* pITM_Version)
{
   /* check input arguments */
   TD_PTR_CHECK(pITM_Ctrl,);
   TD_PTR_CHECK(pTD_Version,);
   TD_PTR_CHECK(pITM_Version,);

   /* set global pointer */
   g_pITM_Ctrl = pITM_Ctrl;
   /* by default test mode is disabled */
   g_pITM_Ctrl->fITM_Enabled = IFX_FALSE;
   g_pITM_Ctrl->oVerifySytemInit.fEnabled = IFX_FALSE;
   /* save information about program arguments */
   g_pITM_Ctrl->rgsArgv = pArgv;
   g_pITM_Ctrl->nArgc = nArgc;
   g_pITM_Ctrl->nComInSocket = NO_SOCKET;
   g_pITM_Ctrl->nComInPort = COM_COMMAND_PORT;
   g_pITM_Ctrl->nComOutSocket = NO_SOCKET;
   g_pITM_Ctrl->nComOutPort = COM_SENDING_PORT;
   g_pITM_Ctrl->oBroadCast.nSocket = NO_SOCKET;
   g_pITM_Ctrl->bCommunicationReset = IFX_FALSE;
   g_pITM_Ctrl->bChangeIPAddress = IFX_FALSE;
   g_pITM_Ctrl->oCID_Test.nPhoneNumber = 0;
   g_pITM_Ctrl->nComDbgLevel = TD_ITM_DBG_LEVEL_DEFAULT;
   g_pITM_Ctrl->nTestType = COM_NO_TEST;
   g_pITM_Ctrl->nSubTestType = COM_SUBTEST_NONE;
   g_pITM_Ctrl->bNoProblemWithDwldPath = IFX_TRUE;

   /* copy version strings to control structure */
   strncpy(g_pITM_Ctrl->rgoTapidemoVersion, pTD_Version, TD_MAX_NAME);
   strncpy(g_pITM_Ctrl->rgoITM_Version, pITM_Version, TD_MAX_NAME);

   /* Reset parameters (default values) */
   memset((IFX_uint8_t *) &g_pITM_Ctrl->oDefaultProgramArg,
          0, sizeof(PROGRAM_ARG_t));

   return;
}

