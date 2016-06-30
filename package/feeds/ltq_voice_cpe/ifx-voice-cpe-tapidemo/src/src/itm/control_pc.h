#ifndef _CONTROL_PC_H
#define _CONTROL_PC_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : control_pc.h
   Date        : 2007-08-20
   Description : This file contains definitions of communication functions.
 ****************************************************************************
   \file control_pc.h

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"

/* Include board specific commands, structures
   used in COM_BBD_ChangeCountrySettings */

#if defined(EASY3332)
   #include "board_easy3332.h"
#endif /* EASY3332, VIN_2CPE ,HAVE_DRV_VINETIC_HEADERS  */

#if defined(EASY50510)
   #include "board_easy50510.h"
#endif

#ifdef DANUBE
   #include "board_easy50712.h"
#endif /* DANUBE */

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

#if defined (EASY3201) || defined (EASY3201_EVS)
   #include "board_easy3201.h"
#endif /* EASY3201 */

#ifdef EASY336
   #include "device_svip.h"
#endif /* EASY336 */

#if (defined(XT16) || defined(WITH_VXT))
   #include "device_vxt.h"
#endif /* (defined(XT16) || defined(WITH_VXT)) */

/* ============================= */
/* Global Defines                */
/* ============================= */

/** index of start of user defined CPT table */
#define CPT_INDEX 100

#ifdef LINUX

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/** if default key/value Length is set to this value, then command doesn't
    have default key/value size */
#define NO_DEFAULT_KEY_VALUE_LENGTH    0xFF

/* The length of an empty command frame received from Control PC

+====+============+=========+=========+=========+=========+=====+
| id | repetition | key_len | val_len | Nth_key | Nth_val | EOM |
+====+============+=========+=========+=========+=========+=====+
| 1B |     2B     |   1B    |   1B    | key_len | val_len | 1B  |
+----+------------+---------+---------+---------+---------+-----+
*/

/** timeout to activate comunication socket [ms] */
#define ITM_COMUNICATION_TIMEOUT 500

/** Length of ITM message header in bytes */
#define ITM_MSG_HEADER_LEN 5

/** data that is send in EOM (End Of Message) field of ITM messages */
#define ITM_EOM 0xFF

/** size of ITM_EOM in bytes */
#define ITM_EOM_LEN 1

/** max. size of received ITM configuration message */
#define MAX_SIZE_OF_ITM_MESSAGE     2048

/** max. size of ITM test message */
#define MAX_ITM_MESSAGE_TO_SEND     700

/** max. number of chars needed to hold representation of IFX_int32_t,
    e.g. -1234567890 with trailing '\0' */
#define ITM_MAX_INT32_NUMBER_LENGTH 12

/** Max. allowed tone frequency */
#define MAX_TONE_FREQ 4000

/** Max. cadence length in ms */
#define MAX_CADENCE 32000

/** Max. length of BBD file name */
#define MAX_FILE_NAME_LEN 25

/** number of bits in one byte. */
#define ONE_BYTE 8

/** mask used during conversion from IFX_char_t to IFX_int16_t */
#define BYTE_TO_INT16_MASK 0x00FF

/** String used as an ending sequence in messages sent to Control PC*/
#define COM_MESSAGE_ENDING_SEQUENCE "\n\007\n"

/** run command set response key value */
#define RUN_COMMAND_SET_RESPONSE    0x00

/** run command set program name key value */
#define RUN_COMMAND_PROGRAM_NAME    0x01

/** run command send response before program execution */
#define RUN_COMMAND_RESONSE_BEFORE_EXECUTION    0x00

/** run command send response after program execution */
#define RUN_COMMAND_RESONSE_AFTER_EXECUTION     0x01

/** run command send response after program execution */
#define RUN_COMMAND_MAX_ARGUMENTS   0x08

#define MAX_CMD_LEN 1024

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/** Data part. */
typedef enum _ITM_MSG_DATA_TYPE_t
{
   /** Get Key value. */
   ITM_MSG_DATA_TYPE_KEY = 0x01,
   /** Get value value. */
   ITM_MSG_DATA_TYPE_VALUE = 0x02
} ITM_MSG_DATA_TYPE_t;

/** Enums to check if test should be started or stoped. */
typedef enum _COM_TEST_START_STOP_t
{
   /** Test in progress - function should check
      if action should be taken. */
   COM_TEST_IN_PROGRESS = 0x00,
   /** Start test. */
   COM_TEST_START = 0x01,
   /** Stop test. */
   COM_TEST_STOP = 0x02
} COM_TEST_START_STOP_t;

/** Enums key values of ANALOG_TO_IP_CALL_CTRL message. */
typedef enum _COM_ANALOG_TO_IP_CALL_CTRL_t
{
   /** Number of value field in ITM message with number of tested phone. */
   ANALOG_TO_IP_CALL_CTRL_CALLED_NUMBER = 0x00,
   /** Number of value field in ITM message with ip of
       testing equipment card. */
   ANALOG_TO_IP_CALL_CTRL_IP = 0x01,
   /** Number of value field in ITM message with testing equipment
       receiving port. */
   ANALOG_TO_IP_CALL_CTRL_PORT = 0x02,
   /** Number of value field in ITM message
      with testing equipment card MAC address. */
   ANALOG_TO_IP_CALL_CTRL_MAC = 0x03,
   /** Number of value field in ITM message
      with start/stop test argument. */
   ANALOG_TO_IP_CALL_CTRL_START_STOP = 0x04
} COM_ANALOG_TO_IP_CALL_CTRL_t;

/** ITM message Data Header Position - positions of data fields in message */
typedef enum _ITM_HEADER_DATA_POSITION_t
{
   /** Number of byte holding id number in ITM message header. */
   HDP_ID = 0,
   /** Number of first byte holding repetition value
       in ITM message header. */
   HDP_REPETITION_H_BYTE = 1,
   /** Number of second byte holding repetition value
       in ITM message header. */
   HDP_REPETITION_L_BYTE = 2,
   /** Number of byte holding repetition key length
       in ITM message header. */
   HDP_KEY_LEN = 3,
   /** Number of byte holding repetition value length
       in ITM message header. */
   HDP_VALUE_LEN = 4,
   /** Number of bytes in ITM message header. */
   HDP_END = 5
}ITM_HEADER_DATA_POSITION_t;

/** call progress tones parameters */
typedef enum _CPT_PARAM_t
{
   CPT_NAME,
   FREQ_ONE,
   FREQ_TWO,
   CADENCE_ONE,
   CADENCE_TWO,
   CADENCE_THREE,
   CADENCE_FOUR,
   MAX_PARAM
}CPT_PARAM_t;

/** Structure to hold default values of key len and value len. */
typedef struct _KEY_UINT_VALUE_UINT_t
{
   /** specify witch test parameter holds nValue */
   IFX_uint32_t nKey;
   /** value for test parameter specified by nKey */
   IFX_uint32_t nValue;
}KEY_UINT_VALUE_UINT_t;

/** Structure holds default values of key len and value len fields. */
typedef struct _KEY_LEN_VALUE_LEN_t
{
   /** default value of key length */
   IFX_uint32_t nKeyLen;
   /** default value of value length */
   IFX_uint32_t nValueLen;
}KEY_LEN_VALUE_LEN_t;

/** values of key field in CID test setup message */
typedef enum _CID_TEST_KEY_t
{
   /** number that will be transmitted with CID is send */
   CID_TEST_NUMBER = 0x01,
   /** number that will be transmitted with CID is send */
   CID_TEST_NAME = 0x02,
   /** number of phone that will transmit custom CID */
   CID_TEST_PHONE_NUM = 0xF0
}CID_TEST_KEY_t;

/** values of key field in LEC test setup message */
typedef enum _LEC_TEST_KEY_t
{
   /** key value for LEC setting */
   LEC_TEST_LEC = 0x01,
   /** key value for NLP setting */
   LEC_TEST_NLP = 0x02
}LEC_TEST_KEY_t;

/** values of key field in LEC test setup message */
typedef enum _LEC_CHECK_KEY_t
{
   /** key value for board no */
   LEC_CHECK_BOARD_NO = 0x01,
   /** key value for device no */
   LEC_CHECK_DEVICE_NO = 0x02,
   /** key value for LEC setting */
   LEC_CHECK_SETTING = 0x03
}LEC_CHECK_KEY_t;

/** values of key field in set communication ports message */
typedef enum _ITM_SET_PORT_NUMBER_KEY_t
{
   /** set port number for incomming messages */
   ITM_SET_PORT_NUMBER_IN = 0x01,
   /** set port number for outgoing messages */
   ITM_SET_PORT_NUMBER_OUT = 0x02
}ITM_SET_PORT_NUMBER_KEY_t;

/** Structure to hold ITM message data when key is IFX_uint8_t type
   and value is string with file name. */
typedef struct _KEY_UINT_VALUE_POINTER_TO_STRING_t
{
   /** specify witch test parameter holds pValue */
   IFX_uint32_t nKey;
   /* pointer to string */
   IFX_char_t* pValue;
}KEY_UINT_VALUE_POINTER_TO_STRING_t;


/** Structure to hold ITM message header data. */
typedef struct _ITM_MSG_HEADER_t
{
   /** ITM message ID. */
   IFX_uint8_t nId;
   /** number of Key and Value pairs in message */
   IFX_uint16_t nRepetition;
   /** Length of Key fields. */
   IFX_uint8_t nKeyLen;
   /** Length of Value fields. */
   IFX_uint8_t nValueLen;
   /** sum of size of value and key */
   IFX_uint16_t nDataLen;
   /** size of ITM message calculated from ITM message data */
   IFX_uint32_t nMsgLen;
   /** message data */
   IFX_char_t* pData;
} ITM_MSG_HEADER_t;

/** Structure to hold CPT prameters. */
typedef struct _CPT_ParamsTable_t
{
   /** Array of CPT params values. */
   IFX_int32_t rgoParams[MAX_PARAM];
}CPT_ParamsTable_t;

/** type of modular test */
typedef enum _TTP_TYPE_t
{
   TTP_TYPE_NONE         = 0x00000000,
   TTP_TYPE_FAX_MODEM    = 0x00010000,
   TTP_TYPE_CID          = 0x00020000
}TTP_TYPE_t;

/** enum to distinguish place where COM_ITM_Traces() was called */
typedef enum _TEST_TRACE_PLACE_t
{
   /** changing phone state from S_IN_RINGING to S_ACTIVE */
   TTP_RINGING_ACTIVE                 = TTP_TYPE_FAX_MODEM | 0x0001,
   /** changing phone state from S_OUT_RINGBACK to S_ACTIVE */
   TTP_RINGBACK_ACTIVE                = TTP_TYPE_FAX_MODEM | 0x0002,
   /** fax/modem signal detected */
   TTP_FAX_MODEM_SIGNAL               = TTP_TYPE_FAX_MODEM | 0x0003,
   /** first fax modem signal detected, changed state to S_FAX_MODEM state */
   TTP_SET_S_FAX_MODEM                = TTP_TYPE_FAX_MODEM | 0x0004,
   /** stopped codec to prepare data channel for T.38 */
   TTP_T_38_CODEC_STOP                = TTP_TYPE_FAX_MODEM | 0x0005,
   /** started codec after end of T.38 transmission */
   TTP_T_38_CODEC_START               = TTP_TYPE_FAX_MODEM | 0x0006,
   /** disable LEC to prepare data channel for T.38 */
   TTP_LEC_DISABLED                   = TTP_TYPE_FAX_MODEM | 0x0007,
   /** enable LEC at end of fax/modem transmission */
   TTP_LEC_ENABLE                     = TTP_TYPE_FAX_MODEM | 0x0008,
   /** fax T.38 transmission started */
   TTP_START_FAXT38_TRANSMISSION      = TTP_TYPE_FAX_MODEM | 0x0009,
   /** fax T.38 transmission stopped */
   TTP_STOP_FAXT38_TRANSMISSION       = TTP_TYPE_FAX_MODEM | 0x000A,
   /** changed state to S_FAXT38 state */
   TTP_SET_S_FAXT38                   = TTP_TYPE_FAX_MODEM | 0x000B,
   /** clear channel function was called */
   TTP_CLEAR_CHANNEL                  = TTP_TYPE_FAX_MODEM | 0x000C,
   /** disable NLP function was called */
   TTP_DISABLE_NLP                    = TTP_TYPE_FAX_MODEM | 0x000D,
   /** restore channel function was called */
   TTP_RESTORE_CHANNEL                = TTP_TYPE_FAX_MODEM | 0x000E,
   /** fax/modem transmission ended - IFX_TAPI_EVENT_FAXMODEM_HOLDEND detected */
   TTP_HOLDEND_DETECTED               = TTP_TYPE_FAX_MODEM | 0x000F,
   /** T.38 state change detected - IFX_TAPI_EVENT_DCN detected */
   TTP_DCN_DETECTED                   = TTP_TYPE_FAX_MODEM | 0x0010,
   /** RING STOP called in CID_Send() */
   TTP_CID_RING_STOP                  = TTP_TYPE_CID | 0x0001,
   /** CID message to send is set, send data to test script */
   TTP_CID_MSG_TO_SEND                = TTP_TYPE_CID | 0x0002,
   /** CID is enabled */
   TTP_CID_ENABLE                     = TTP_TYPE_CID | 0x0003,
   /** transmission sequence is started */
   TTP_CID_TX_SEQ_START               = TTP_TYPE_CID | 0x0004,
   /** transmission sequence end event detected */
   TTP_CID_EVENT_CID_TX_SEQ_END       = TTP_TYPE_CID | 0x0005,
   /** CID is disabled */
   TTP_CID_DISABLE                    = TTP_TYPE_CID | 0x0006,
   /** first ring start on FXO event detected */
   TTP_CID_EVENT_FXO_RING_START       = TTP_TYPE_CID | 0x0007,
   /** first ring stop on FXO event detected */
   TTP_CID_EVENT_FXO_RING_STOP        = TTP_TYPE_CID | 0x0008,
   /** receiving CID FSK started */
   TTP_CID_FSK_RX_START               = TTP_TYPE_CID | 0x0009,
   /** DTMF detection for CID DTMF standard receiving started */
   TTP_CID_DTMF_DETECTION_START       = TTP_TYPE_CID | 0x000A,
   /** CID receiving end event detected */
   TTP_CID_EVENT_CID_RX_END           = TTP_TYPE_CID | 0x000B,
   /** receiving CID stopped */
   TTP_CID_FSK_RX_STOP                = TTP_TYPE_CID | 0x000C,
   /** DTMF detection for CID DTMF standard receiving stopped */
   TTP_CID_DTMF_DETECTION_STOP        = TTP_TYPE_CID | 0x000D,
   /** got CID receiving status */
   TTP_CID_FSK_RX_STATUS_GET          = TTP_TYPE_CID | 0x000E,
   /** received CID data is read */
   TTP_CID_FSK_RX_DATA_GET            = TTP_TYPE_CID | 0x000F,
   /** FSK CID message is received, send data to test script */
   TTP_CID_MSG_RECEIVED_FSK           = TTP_TYPE_CID | 0x0010,
   /** DTMF CID message is received, send data to test script */
   TTP_CID_MSG_RECEIVED_DTMF          = TTP_TYPE_CID | 0x0011
} TEST_TRACE_PLACE_t;

/** program features */
typedef enum _TD_ENABLED_DISABLED_t
{
   /** enable feature */
   TD_DISABLED = -1,
   /** disable feature */
   TD_ENABLED = 0
} TD_ENABLED_DISABLED_t;

/** program features */
typedef enum _TAPIDEMO_OPTIONS_t
{
   TD_OPT_PATH_TO_DOWNLOAD_FILES = 0,
   TD_OPT_USE_FILESYSTEM,
   TD_OPT_BOARD_COMBINATION,
   TD_OPT_DEBUG_LEVEL,
   TD_OPT_ITM_DEBUG_LEVEL,
   TD_OPT_CALLER_ID,
   TD_OPT_CALLER_ID_SETTING,
   TD_OPT_VAD_SETTING,
   TD_OPT_CONFERENCE,
   TD_OPT_QOS_SUPPORT,
   TD_OPT_QOS,
   TD_OPT_QOS_LOCAL_UDP,
   TD_OPT_FXO_SUPPORT,
   TD_OPT_FXO,
   TD_OPT_USE_CODERS_FOR_LOCAL,
   TD_OPT_NO_LEC,
   TD_OPT_ENCODER,
   TD_OPT_PACKETISATION,
   TD_OPT_DEAMON,
   TD_OPT_TRACE_REDIRECTION,
   TD_OPT_PCM_MASTER,
   TD_OPT_PCM_SLAVE,
   TD_OPT_PCM_LOOP,
   TD_OPT_PCM_WIDEBAND,
   TD_OPT_FAX_MODEM,
   TD_OPT_FAX_T38_SUPPORT,
   TD_OPT_FAX_T38,
   TD_OPT_CH_INIT,
   TD_OPT_QOS_ON_SOCKET,
   TD_OPT_IPV6_SUPPORT,
   TD_OPT_IPV6,
   /** no more features - used with for() loop */
   TD_OPT_MAX
} TAPIDEMO_OPTIONS_t;
#endif /* LINUX */

/** Type of file e.g. BBD, FW */
typedef enum _ITM_RCV_FILE_TYPE_t
{
   /** */
   ITM_RCV_FILE_NONE = 0,
   /** */
   ITM_RCV_FILE_BBD = 1,
   /** */
   ITM_RCV_FILE_BBD_FXS = 2,
   /** */
   ITM_RCV_FILE_BBD_FXO = 3,
   /** */
   ITM_RCV_FILE_FW = 4,
   /** */
   ITM_RCV_FILE_FW_DRAM = 5,
   /** */
   ITM_RCV_FILE_MAX
} ITM_RCV_FILE_TYPE_t;

#ifdef LINUX

#define COM_EMPTY_STR ""

/** format string for ITM message with program options */
#define COM_PROG_OPT_MSG_FORMAT_STR_VAL      \
   "PROG_OPT:{{NAME {%s}} "                  \
   "{DFLT {%s}} {DFLT_TXT {%s}} "            \
   "{DESC {%s}}}%s"


/** prepare program option message

    \param  nRet  - return value of snprintf() to check
    \param  pName - option name
    \param  pDfltValue   - default option value, string with value
    \param  pDfltValueName  - string representation of nDfltValue
    \param  pDesc  - aditional description of default value
  */
#define COM_OPT_MSG_STR_VAL(nRet, pName,                       \
                            pDfltValue, pDfltValueName,        \
                            pDesc)                             \
   nRet = snprintf(pszITM_Message, MAX_ITM_MESSAGE_TO_SEND,  \
                   COM_PROG_OPT_MSG_FORMAT_STR_VAL,            \
                   pName, pDfltValue, pDfltValueName,             \
                   pDesc, COM_MESSAGE_ENDING_SEQUENCE);

/* ============================= */
/* Global Macros                 */
/* ============================= */

/** set new tone number if custom CPTs are used
 *  \param m_nTone - oryginal tone number that will be changed
 *                   if custom CPTs are used */
#define COM_CHANGE_TONE_IF_CUSTOM_CPT(m_nTone) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (IFX_TRUE == g_pITM_Ctrl->nUseCustomCpt)) \
      { \
         /* get CPT index*/ \
         m_nTone = COM_GetCustomCPT(m_nTone); \
      } \
   } while (0)

/** save FW version number
 *  \param m_vinVers - structure with FW version number */
#define COM_GET_FW_VERSION_VCPE(m_vinVers)   \
   do \
   { \
      snprintf(g_pITM_Ctrl->rgoFW_Version, TD_MAX_NAME, \
               "%d.%d", m_vinVers.nEdspVers, m_vinVers.nEdspIntern); \
   } while (0)

#ifndef TD_3_CPTD_SUPPORT_DISABLED
/** save FW version number
 *  \param m_vinVers - structure with FW version number */
#define COM_GET_FW_VERSION_VMMC(m_vinVers)   \
   do \
   { \
      snprintf(g_pITM_Ctrl->rgoFW_Version, TD_MAX_NAME, \
               "%d.%d.%d", \
               m_vinVers.nEdspVers, m_vinVers.nEdspIntern, m_vinVers.nEDSPHotFix); \
   } while (0)
#else /* TD_3_CPTD_SUPPORT_DISABLED */
/**
   Here TD_3_CPTD_SUPPORT_DISABLED flag has second purpose as nEDSPHotFix
   was implemented not so long before multiple CPTD on one channel feature.
  */
#define COM_GET_FW_VERSION_VMMC COM_GET_FW_VERSION_VCPE
#endif /* TD_3_CPTD_SUPPORT_DISABLED */

/** save FW version number for tapi v4 (svip and vxt)
 *  \param m_vinVers - structure with FW version number */
#define COM_GET_FW_VERSION_TAPI_V4(m_acFW_Ver)   \
   do \
   { \
      if ('\0' == g_pITM_Ctrl->rgoFW_Version[0]) \
      { \
         strncpy(g_pITM_Ctrl->rgoFW_Version, m_acFW_Ver, TD_MAX_NAME - 1); \
      } \
   } while (0)

/** set cid message structure pointer for CID test
 *  \param m_pCID_Msg - pointer to cid msg structure that can be changed
 *  \param m_pPhone   - pointer to phone structure that plays CID msg */
#define COM_MOD_CID_DATA_SET(m_pCID_Msg, m_pPhone) \
   do \
   { \
      /* check if custom CID for test purpose is set */ \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (0 != g_pITM_Ctrl->oCID_Test.nPhoneNumber)) \
      { \
         m_pCID_Msg = COM_CID_DataSet(m_pCID_Msg, m_pPhone); \
      } \
   } while (0)

/** reset/disable LEC for phone
 *  \param m_pPhone  - pointer phone
 *  \param m_nEventType - detected event */
#define COM_MOD_LEC_CHECK_FOR_RESET(m_pPhone, m_nEventType) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_LEC)) \
      { \
         if (IFX_SUCCESS != COM_LEC_Disable(m_pPhone, m_nEventType)) \
         { \
            /* Wrong input arguments */ \
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_pPhone->nSeqConnId, \
               ("Err, ITM: Failed to reset LEC. (File: %s, line: %d)\n", \
                __FILE__, __LINE__)); \
         } \
      } \
   } while(0)

/** overwrite LEC settings
 *  \param m_pPhone  - pointer phone
 *  \param m_pLEC_Settings - pointer to LEC configuration */
#define COM_MOD_LEC_SET(m_pPhone, m_pLEC_Settings) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_LEC)) \
      { \
         if (IFX_SUCCESS != COM_LEC_DataSet(m_pPhone, m_pLEC_Settings)) \
         { \
            /* Wrong input arguments */ \
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_pPhone->nSeqConnId, \
               ("Err, ITM: Failed to set LEC. (File: %s, line: %d)\n", \
                __FILE__, __LINE__)); \
         } \
      } \
   } while(0)

/** ignore pulse digit in EVENT_Handle during modular test of hook flash event
 *  \param m_pPhone   - pointer to phone where event was detected */
#define COM_MOD_HOOKFLASH_IGNORE_PULS_DIGIT(m_pPhone) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) &&  \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_HOOKFLASH))   \
      {  \
         /* if modular test of hook flash in progress then ignore pulse */ \
         /* digit, hook flash shorter than defined time is not detected */ \
         /* or even detected as pulse digit */  \
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, m_pPhone->nSeqConnId, \
               ("Phone No %d: Pulse digit ignored due to hook-flash test\n", \
                m_pPhone->nPhoneNumber));   \
         return IFX_SUCCESS;   \
      } \
   } while (0)

/** send msg for hook-flash modular test
 *  \param m_pPhone   - pointer to phone where event was detected */
#define COM_MOD_HOOKFLASH_HANDLE(m_pPhone) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_HOOKFLASH)) \
      { \
         TAPIDEMO_PRINTF(TD_CONN_ID_ITM, ("HOOK_EVENT:HOOK_FLASH::%d\n", \
                          (int) m_pPhone->nPhoneNumber)); \
      } \
   } while (0)

/** send msg about hook-off event depending on modular test type
 *  \param m_pPhone   - pointer to phone where event was detected */
#define COM_MOD_EVENT_HOOK_OFF(m_pPhone) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST)) \
      { \
         if (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_HOOKSTATE) \
         { \
            TAPIDEMO_PRINTF(TD_CONN_ID_ITM, ("HOOK_EVENT:HOOK_OFF::%d\n", \
                             (IFX_int32_t) m_pPhone->nPhoneNumber)); \
         } \
         else if (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_CONFERENCE) \
         { \
            TAPIDEMO_PRINTF(TD_CONN_ID_ITM, ("CONFERENCE:HOOK_OFF::%d\n", \
                             (IFX_int32_t) m_pPhone->nPhoneNumber)); \
         } \
      } \
   } while (0)

/** send msg about hook-on event depending on modular test type
 *  \param m_pPhone   - pointer to phone where event was detected */
#define COM_MOD_EVENT_HOOK_ON(m_pPhone) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST)) \
      { \
         if (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_HOOKSTATE) \
         { \
            TAPIDEMO_PRINTF(TD_CONN_ID_ITM, ("HOOK_EVENT:HOOK_ON::%d\n", \
                             (IFX_int32_t) m_pPhone->nPhoneNumber)); \
         } \
         else if (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_CONFERENCE) \
         { \
            TAPIDEMO_PRINTF(TD_CONN_ID_ITM, ("CONFERENCE:HOOK_ON::%d\n", \
                             (IFX_int32_t) m_pPhone->nPhoneNumber)); \
         } \
      } \
   } while (0)

/** send msg for DTMF digit modular test and reset phone event data
 *  \param m_pPhone   - pointer to phone where event was detected
 *  \param m_nDigit   - digit number */
#define COM_MOD_DIGIT_DTMF_HANDLE(m_pPhone, m_nDigit) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_DTMFDIGIT)) \
      { \
         TAPIDEMO_PRINTF(TD_CONN_ID_ITM, \
                         ("DTMF_DIGIT_EVENT:DIALED_DIGIT_%d::%d\n", \
                          m_nDigit, m_pPhone->nPhoneNumber)); \
         m_pPhone->nIntEvent_FXS = IE_NONE; \
      } \
   } while (0)

/** send msg for pulse digit modular test and reset phone event data
 *  \param m_pPhone   - pointer to phone where event was detected
 *  \param m_nDigit   - digit number */
#define COM_MOD_DIGIT_PULSE_HANDLE(m_pPhone, m_nDigit) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_PULSEDIGIT)) \
      { \
         TAPIDEMO_PRINTF(TD_CONN_ID_ITM, \
                         ("PULSE_DIGIT_EVENT:DIALED_DIGIT_%d::%d\n", \
                          m_nDigit, m_pPhone->nPhoneNumber)); \
         m_pPhone->nIntEvent_FXS = IE_NONE; \
      } \
   } while (0)

#ifdef EASY336
/** return from function in case modular test DTMF digit is in progress
 *  \param m_ret - return value */
#define COM_MOD_DIGIT_DTMF_SVIP_RETURN_FROM_FUNCTION(m_ret) \
   do \
   { \
      /* if DTMF digit test then end function */ \
      /* because resource manager is deinitialized */ \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_DTMFDIGIT)) \
      { \
         /* end function */ \
         return m_ret; \
      } \
   } while (0)

/** during modular DTMF digit test enable tone detection
 *  \param m_pPhone - pointer to phone */
#define COM_MOD_DIGIT_DTMF_SVIP_TONE_DET_ENABLE_IF(m_pPhone) \
   /* If it's a test of DTMF digits... */ \
   if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
       (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
       (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_DTMFDIGIT)) \
   { \
      /* turn on DTMF detection and play dialtone */ \
      ret = COM_SvipToneDetection(IFX_TRUE, pPhone->nSeqConnId); \
   }

/** during modular DTMF digit test disable tone detection
 *  and return from function
 *  \param m_pPhone - used phone
 *  \param m_ret - return value */
#define COM_MOD_DIGIT_DTMF_SVIP_TONE_DET_DISABLE_RETURN(m_pPhone, m_ret) \
   do \
   { \
      /* If it's a test of DTMF digits... */ \
      if ((g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_DTMFDIGIT)) \
      { \
         /* turn off tone detection */ \
         m_ret = COM_SvipToneDetection(IFX_FALSE, pPhone->nSeqConnId); \
         /* Set line in standby */ \
         Common_LineFeedSet(m_pPhone, IFX_TAPI_LINE_FEED_STANDBY); \
         return m_ret; \
      } \
   } while (0)
#endif /* EASY336 */

/** return UNKNOWN_CALL_TYPE during test to avoid phone state change
 *  \param m_zero - unused argument */
#define COM_MOD_DIGIT_RETURN_UNKNOWN_CALL_TYPE_DURING_TEST(m_zero) \
   do \
   { \
      /* If it's a test of digits... */ \
      if ( ( IFX_TRUE == g_pITM_Ctrl->fITM_Enabled ) && \
           ( g_pITM_Ctrl->nTestType == COM_MODULAR_TEST ) && \
           ( g_pITM_Ctrl->nSubTestType & \
             (COM_SUBTEST_PULSEDIGIT | COM_SUBTEST_DTMFDIGIT) ) ) \
      { \
         return UNKNOWN_CALL_TYPE; \
      } \
   } while (0)

/** true when modular test of DTMF or pules digits is on */
#define COM_MOD_DIGIT_IS_NOT_ON \
   ( \
      /* check if ITM enabled */ \
      ( IFX_TRUE != g_pITM_Ctrl->fITM_Enabled ) || \
      /* not a modular test of digits */ \
      !( \
         ( g_pITM_Ctrl->nTestType == COM_MODULAR_TEST ) && \
         ( \
            g_pITM_Ctrl->nSubTestType & \
            ( COM_SUBTEST_PULSEDIGIT | COM_SUBTEST_DTMFDIGIT ) \
         ) \
      ) \
   )


/** prepare for modular test of verify system initialization
 *  \param m_pCtrl   - pointer control structure */
#define COM_MOD_VERIFY_SYSTEM_INIT_PREPARE(m_pCtrl) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          g_pITM_Ctrl->oVerifySytemInit.fEnabled) \
      { \
         if (IFX_ERROR == COM_PrepareVerifySystemInit(m_pCtrl)) \
         { \
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM, \
               ("Err, COM_PrepareVerifySystemInit() failed. " \
                "(File: %s, line: %d)\n", \
                __FILE__, __LINE__)); \
            return IFX_ERROR; \
         } \
      } \
   }while (0)

/**  send end message for verify system initialization test
 *  \param m_zero - unused argument */
#define COM_MOD_VERIFY_SYSTEM_INIT_SEND_DONE(m_zero) \
   do \
   { \
      /* check if system initialization test*/ \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          g_pITM_Ctrl->oVerifySytemInit.fEnabled) \
      { \
         /* inform control PC that all boards are initialiazed */ \
         if (IFX_ERROR == COM_SendSysInitDone()) \
         { \
            /* failed to send message to control PC */ \
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM, \
                  ("Err, failled to send message to control PC " \
                   "'all boards initialized'. " \
                   "(File: %s, line: %d)\n", \
                   __FILE__, __LINE__)); \
         }  \
         else \
         { \
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM, \
                  ("Message SYSINIT_DONE sent to control PC\n")); \
         } \
      } /* if (nVerifySystemInitializtion) */ \
   }while (0)

/** send message to control PC that busy tone is played during conference test
 *  \param m_pPhone - used phone
 *  \param m_nTone - CPT number */
#define COM_MOD_CONFERENCE_BUSY_TRACE(m_pPhone, m_nTone) \
   do \
   { \
      /* If it's a conference test... */ \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled ) && \
          (BUSY_TONE_IDX == m_nTone)) \
      { \
         if ((COM_MODULAR_TEST == g_pITM_Ctrl->nTestType ) && \
             (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_CONFERENCE)) \
         { \
            TAPIDEMO_PRINTF(m_pPhone->nSeqConnId, \
                ("CONFERENCE:BUSYTONE::%d\n", \
                 (IFX_int32_t) m_pPhone->nPhoneNumber)); \
         } \
      } \
   } while(0)

/** print and send msg for fxo modular test
 *  \param m_ConnID - sequential conn ID
 *  \param m_message  - msg to use */
#define COM_MOD_FXO(m_ConnID, m_message) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_FXO)) \
      { \
         TAPIDEMO_PRINTF(m_ConnID, m_message); \
      } \
   } while(0)


/** print and send msg for fxo modular test
 *  \param m_pCtrl  - pointer to control structure
 *  \param m_pPhone - pointer to phone structure
 *  \param m_pConn  - pointer to connection structure
 *  \param m_oData  - various data
 *  \param m_nCallPlace - where function is called
 *  \param m_ConnID - sequential conn ID */
#define COM_ITM_TRACES(m_pCtrl, m_pPhone, m_pConn, m_oData, m_nCallPlace, m_ConnID) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (COM_MODULAR_TEST == g_pITM_Ctrl->nTestType)) \
      { \
         COM_ITM_Traces(m_pCtrl, m_pPhone, m_pConn, \
                        m_oData, m_nCallPlace, m_ConnID); \
      } \
   } while(0)

/** change peer IP address for analog to IP test
 *  \param m_pAddrFrom  - current address to change */
#define COM_ITM_SET_IP(m_pAddrFrom) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (COM_MODULAR_TEST == g_pITM_Ctrl->nTestType)) \
      { \
         /* change peer address if needed */ \
         if (IFX_ERROR == COM_SetGetPort(m_pAddrFrom, \
                                         COM_TEST_IN_PROGRESS)) \
         { \
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM, \
               ("Err, COM_SetGetPort() failed. (File: %s, line: %d)\n", \
                __FILE__, __LINE__)); \
            return IFX_ERROR; \
         } \
      } \
   } while(0)

#ifdef TAPI_VERSION4
/** get channel number (TAPI_V4)
 *  \param m_pPhone  - used phone
 *  \param m_pConn  - used connection */
#define COM_MOD_SEND_PHONE_MAPPING_GET_CHANNEL(m_pPhone, m_pConn) \
   (m_pPhone->nPhoneNumber % 100 - 1)
#else /* TAPI_VERSION4 */
/** get channel number (TAPI_V3)
 *  \param m_pPhone  - used phone
 *  \param m_pConn  - used connection */
#define COM_MOD_SEND_PHONE_MAPPING_GET_CHANNEL(m_pPhone, m_pConn) \
    ((IFX_int32_t) m_pConn->nUsedCh)
#endif /* TAPI_VERSION4 */

/** send used UDP port number for phone
 *  \param m_msg  - msg to use
 *  \param m_pPhone  - used phone
 *  \param m_pConn  - used connection
 *  \param m_ConnID - sequential conn ID */
#define COM_MOD_SEND_PHONE_MAPPING(m_msg, m_pPhone, m_pConn, m_ConnID) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST) && \
          (g_pITM_Ctrl->nSubTestType == COM_SUBTEST_NONE || \
           g_pITM_Ctrl->nSubTestType & COM_SUBTEST_LEC)) \
      { \
         if (IE_ACTIVE == m_msg.nAction) \
         { \
            IFX_char_t m_buf[100]; \
            sprintf(m_buf, \
                    "UDP_PORT_2_PHONE_MAPPING:" \
                    "{CHANNEL %d PHONE %d UDP_PORT %d}%s", \
                    COM_MOD_SEND_PHONE_MAPPING_GET_CHANNEL(m_pPhone, m_pConn), \
                    (IFX_int32_t) m_msg.nSenderPhoneNum, \
                    (IFX_int32_t) m_msg.nSenderPort, \
                    COM_MESSAGE_ENDING_SEQUENCE); \
            COM_SendMessage(g_pITM_Ctrl->nComOutSocket, m_buf, m_ConnID); \
         } \
      } \
   } while(0)

/** Send iformation about used codec after checkcking
 *  if communication with control PC is established and modular test in progress
 *  \param m_pPhone  - pointer to phone
 *  \param m_nDataCh - data channel
 *  \param m_nCodec  - codec number
 *  \param m_nPacketization   - packetization number
 *  \param m_bDecoder   - IFX_TRUE if DECODER info,
 *                        otherwise ENCODER info is send */
#define COM_MOD_SEND_INFO_USED_CODEC(m_pPhone, m_nDataCh, m_nCodec, \
                                     m_nPacketization, m_bDecoder) \
   do \
   { \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (g_pITM_Ctrl->nComOutSocket != NO_SOCKET) && \
          (g_pITM_Ctrl->nTestType == COM_MODULAR_TEST)) \
      { \
         PHONE_t *m_pPhoneTmp = (m_pPhone); \
         if (IFX_SUCCESS != COM_SendInfoUsedCodec(m_pPhoneTmp, m_nDataCh, \
                               m_nCodec, m_nPacketization, m_bDecoder)) \
         { \
            /* Wrong input arguments */ \
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, \
               (IFX_NULL != m_pPhoneTmp) ? m_pPhoneTmp->nSeqConnId : TD_CONN_ID_ERR, \
               ("Err, Phone No %d: COM_SendInfoUsedCodec() failed." \
                " (File: %s, line: %d)\n", \
                (IFX_NULL != m_pPhoneTmp) ? m_pPhoneTmp->nPhoneNumber : 0, \
                __FILE__, __LINE__)); \
         } \
      } \
   } while(0)

/** send first idetification message (used when first test is started)
 *  \param m_zero - unused argument */
#define COM_SEND_IDENTIFICATION(m_pPhone, m_pFXO) \
   do \
   { \
      /* if there is a connection with control PC first HOOKOFF \
         means that SUT has to send its identification */ \
      if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
          (COM_SUBTEST_CONFIG_DETECTION & g_pITM_Ctrl->nSubTestType) && \
          (NO_SOCKET != g_pITM_Ctrl->nComOutSocket)) \
      { \
          COM_SendIdentification(m_pPhone, m_pFXO); \
      } \
   } while(0)

/** send first idetification message (used when first test is started)
 *  \param m_aIfName - name of network interface
 *  \param m_oMy_IP_Addr - structure with new address
 *  \param m_ConnID - sequential conn ID */
#define COM_COMMUNICATION_INIT(m_aIfName, m_oMy_IP_Addr, m_ConnID) \
   do \
   { \
      if (g_pITM_Ctrl->fITM_Enabled) \
      { \
         /* Create communication socket for receiving commands */ \
         /* from control PC*/ \
         g_pITM_Ctrl->nComInSocket = \
            COM_InitCommunication(TD_SOCK_AddrIPv4Get(&m_oMy_IP_Addr), \
                                  COM_DIRECTION_IN); \
         if (IFX_SUCCESS != COM_BrCastAddrSet(m_aIfName)) \
         { \
             /* Print error message */ \
             TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_ConnID, \
                   ("Err, COM_BrCastAddrSet(%s) failed." \
                    " (File: %s, line: %d)\n", \
                    m_aIfName, __FILE__, __LINE__)); \
         } \
         /* Open broadcast socket */ \
         else if (IFX_SUCCESS != COM_BrCastSocketOpen()) \
         { \
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_ConnID, \
                  ("Err, ITM: COM_BrCastSocketOpen() failed. " \
                   "(File: %s, line: %d)\n", __FILE__, __LINE__)); \
         } \
      } \
   } while(0)

/** if needed wait for files download
 *  \param m_pCtrl - control structure */
#define COM_RCV_FILES_ON_STARTUP(m_pCtrl) \
   do \
   { \
      if (g_pITM_Ctrl->fITM_Enabled && \
          IFX_TRUE == g_pITM_Ctrl->oVerifySytemInit.fEnabled && \
          IFX_TRUE == g_pITM_Ctrl->oVerifySytemInit.fFileDwnld) \
      { \
          if (IFX_ERROR == COM_RcvFileOnStartUp(m_pCtrl)) \
          { \
             /* Wrong input arguments */ \
             TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM, \
                   ("Err, COM_RcvFileOnStartUp." \
                    " (File: %s, line: %d)\n", __FILE__, __LINE__)); \
             return IFX_ERROR; \
          } \
      } \
   } while(0)

/** set data pointer if specific file already downloaded
 *  \param m_nChipType  - chip type (e.g. danube, ar9...)
 *  \param m_nFileType  - file type (BBD, BBD_FXS, FW...)
 *  \param m_ppData     - pointer to data pointer
 *  \param m_pnDataLen  - pointer to length of data */
#define COM_RCV_FILES_GET(m_nChipType, m_nFileType, m_ppData, m_pnDataLen) \
   do \
   { \
      if (g_pITM_Ctrl->fITM_Enabled) \
      { \
         COM_RcvFileGet(m_nChipType, m_nFileType, \
                        (m_ppData), (m_pnDataLen)); \
      } \
   } while(0)

/** check if pointer allocated for dowloaded file
 *  \param m_pData - binary data */
#define COM_RCV_FILES_CHECK_IF_FREE_MEM(m_pData) \
   (IFX_FALSE == COM_RcvFileCheckIfRelease(m_pData))

/** reset communication sockets
 *  \param m_pCtrl - control structure
 *  \param m_rfds - set of read FDs */
#define COM_IF_COMMUNICATION_SOCKETS_RESET(m_pCtrl, m_rfds) \
   do \
   { \
      if (g_pITM_Ctrl->fITM_Enabled && \
          IFX_TRUE == g_pITM_Ctrl->bCommunicationReset) \
      { \
          if (IFX_ERROR == COM_ResetCommunicationSockets(m_pCtrl, &m_rfds)) \
          { \
             /* Wrong input arguments */ \
             TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM, \
                   ("Err, Failed to reset communication sockets." \
                    " (File: %s, line: %d)\n", __FILE__, __LINE__)); \
          } \
      } \
   } while(0)

/** clean up after ITM
 *  \param m_zero - unused argument */
#define COM_ITM_CLEAN_UP(m_zero) \
   do \
   { \
      if (IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) \
      { \
         /* clean up after test mode */ \
         COM_CleanUp(); \
      } \
   } while(0)

/** add ITM communication sockets FDs to set of FDs used by select()
 *  \param m_pRdFds - set of read FDs
 *  \param m_rwd - highest number of FD in set */
#define COM_SOCKET_FD_SET(m_pRdFds, m_rwd) \
   do \
   { \
      /* Add communication socket for handling PC's requests */ \
      if(IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) \
      { \
         if (NO_SOCKET != g_pITM_Ctrl->nComInSocket) \
         { \
            TD_SOC_FD_SET(g_pITM_Ctrl->nComInSocket, m_pRdFds, m_rwd); \
         } \
         if (NO_SOCKET != g_pITM_Ctrl->oBroadCast.nSocket) \
         { \
            TD_SOC_FD_SET(g_pITM_Ctrl->oBroadCast.nSocket, m_pRdFds, m_rwd); \
         } \
      } \
   } while(0)

/** verify system initialization
 *  \param pBoard - pointer to board
 *  \param pInit - pointer to TAPI initialization structure */
#define COM_VERIFY_SYSTEM_INIT(pBoard, pInit) \
   do \
   { \
      COM_VerifySystemInit(pBoard, &pInit);\
   } while (0)

/** receive message from communication socket
 *  \param m_pCtrl  - pointer to control structure
 *  \param m_bBrCast - return status
 *  \param m_ret  - return status  */
#define COM_RECEIVE_COMMAND(m_pCtrl, m_bBrCast, m_ret) \
   do \
   { \
      if( IFX_SUCCESS != COM_ReceiveCommand(m_pCtrl, m_bBrCast))\
      { \
         m_ret =  IFX_ERROR;\
      }\
   } while (0)

/** send trace message through communication socket
 *  \param level    - message debug level
 *  \param msg      - pointer to the message string  */
#define COM_SEND_TRACE_MESSAGE(level, msg) \
   do \
   { \
      COM_SendTraceMessage(level, msg);\
   } while (0)


/** send DTMF command response, also executes ITM specific functions
 *  \param m_pBoard - pointer to board structure
 *  \param m_nDialedNum - number of dtmf command
 *  \param m_ret - default command response
 *  \param m_nSeqConnId - conn ID */
#define COM_EXECUTE_DTMF_COMMAND(m_pBoard, m_nAction, m_ret, m_nSeqConnId) \
   do \
   { \
      ret = COM_ExecuteDTMFCommand(m_pBoard, m_nAction, m_ret, m_nSeqConnId);\
   } while (0)

/** function to set CPT parameters according to CPT_ParamsTable_t
 *  \param pCtrl  - pointer to control structure
 *  \param pCPT_New - pointer to table with new CPT params */
#define COM_CPT_SET(pCtrl, pCPT_New) \
   do \
   { \
      COM_CPT_Set(pCtrl, pCPT_New);\
   } while (0)

/** set CID standard
 *  \param pCtrl     - pointer to control structure
 *  \param nCidStandard  - number of CID standard
 *  \param ret - return status */
#define COM_CID_STANDART_SET(pCtrl, nCidStandard, ret) \
   do \
   { \
      ret = COM_CID_StandardSet(pCtrl, nCidStandard);\
   } while (0)

/** set default values for ITM control structure
 *  \param pCtrl      - ITM control structure
 *  \param nArgc      - number of program arguments
 *  \param pArgv      - array of pointers with program arguments
 *  \param pTD_Version   - TAPIDEMO version name
 *  \param pITM_Version  - ITM version name */
#define COM_SET_DEFAULT(pITM_Ctrl, Argc, pArgv, pTD_Version, pITM_Version) \
   do \
   { \
      COM_SetDefault(pITM_Ctrl, Argc, pArgv, pTD_Version, pITM_Version);\
   } while (0)

/**
 * Do ITM action for IE_DIALING event received
 * during S_ACIVE or S_CF_ACTIVE state
 * \param m_pPhone - pointer to phone structure */
#define COM_ST_FXS_ACTIVE_DIALING_HANDLE(m_pPhone) \
   do \
   { \
      if (g_pITM_Ctrl->fITM_Enabled && \
          COM_MODULAR_TEST == g_pITM_Ctrl->nTestType && \
          (g_pITM_Ctrl->nSubTestType & COM_SUBTEST_CONFERENCE)) \
      { \
         COM_ST_FXS_Active_Dialing_Handle_Conference(m_pPhone); \
      } \
   } while (0)


#else /* LINUX */

/** set new tone number if custom CPTs are used
 *  \param m_nTone - oryginal tone number that will be changed
 *                   if custom CPTs are used */
#define COM_CHANGE_TONE_IF_CUSTOM_CPT(m_nTone) \
   do \
   { \
   } while (0)

/** save FW version number
 *  \param m_vinVers - structure with FW version number */
#define COM_GET_FW_VERSION_VMMC(m_vinVers) \
   do \
   { \
   } while (0)

/** save FW version number
 *  \param m_vinVers - structure with FW version number */
#define COM_GET_FW_VERSION_VCPE(m_vinVers) \
   do \
   { \
   } while (0)

/** save FW version number for tapi v4 (svip and vxt)
 *  \param m_vinVers - structure with FW version number */
#define COM_GET_FW_VERSION_TAPI_V4(m_acFW_Ver)   \
   do \
   { \
   } while (0)

/** set cid message structure pointer for CID test
 *  \param m_pCID_Msg - pointer to cid msg structure that can be changed
 *  \param m_pPhone   - pointer to phone structure that plays CID msg */
#define COM_MOD_CID_DATA_SET(m_pCID_Msg, m_pPhone) \
   do \
   { \
   } while (0)

/** reset/disable LEC for phone
 *  \param m_pPhone  - pointer phone
 *  \param m_nEventType - detected event */
#define COM_MOD_LEC_CHECK_FOR_RESET(m_pPhone, m_nEventType) \
   do \
   { \
   } while(0)

/** overwrite LEC settings
 *  \param m_pPhone  - pointer phone
 *  \param m_pLEC_Settings - pointer to LEC configuration */
#define COM_MOD_LEC_SET(m_pPhone, m_pLEC_Settings) \
   do \
   { \
   } while(0)

/** ignore pulse digit in EVENT_Handle during modular test of hook flash event
 *  \param m_pPhone   - pointer to phone where event was detected */
#define COM_MOD_HOOKFLASH_IGNORE_PULS_DIGIT(m_pPhone) \
   do \
   { \
   } while (0)

/** send msg for hook-flash modular test
 *  \param m_pPhone   - pointer to phone where event was detected */
#define COM_MOD_HOOKFLASH_HANDLE(m_pPhone) \
   do \
   { \
   } while (0)

/** send msg about hook-off event depending on modular test type
 *  \param m_pPhone   - pointer to phone where event was detected */
#define COM_MOD_EVENT_HOOK_OFF(m_pPhone) \
   do \
   { \
   } while (0)

/** send msg about hook-on event depending on modular test type
 *  \param m_pPhone   - pointer to phone where event was detected */
#define COM_MOD_EVENT_HOOK_ON(m_pPhone) \
   do \
   { \
   } while (0)

/** send msg for DTMF digit modular test and reset phone event data
 *  \param m_pPhone   - pointer to phone where event was detected
 *  \param m_nDigit   - digit number */
#define COM_MOD_DIGIT_DTMF_HANDLE(m_pPhone, m_nDigit) \
   do \
   { \
   } while (0)

/** send msg for pulse digit modular test and reset phone event data
 *  \param m_pPhone   - pointer to phone where event was detected
 *  \param m_nDigit   - digit number */
#define COM_MOD_DIGIT_PULSE_HANDLE(m_pPhone, m_nDigit) \
   do \
   { \
   } while (0)

#ifdef EASY336
/** return from function in case modular test DTMF digit is in progress
 *  \param m_ret - return value */
#define COM_MOD_DIGIT_DTMF_SVIP_RETURN_FROM_FUNCTION(m_ret) \
   do \
   { \
   } while (0)

/** during modular DTMF digit test enable tone detection
 *  \param m_pPhone - pointer to phone */
#define COM_MOD_DIGIT_DTMF_SVIP_TONE_DET_ENABLE_IF(m_pPhone) \
   if (0) \
   { \
   }

/** during modular DTMF digit test disable tone detection
 *  and return from function
 *  \param m_pPhone - used phone
 *  \param m_ret - return value */
#define COM_MOD_DIGIT_DTMF_SVIP_TONE_DET_DISABLE_RETURN(m_pPhone, m_ret) \
   do \
   { \
   } while (0)
#endif /* EASY336 */

/** return UNKNOWN_CALL_TYPE during test to avoid phone state change
 *  \param m_zero - unused argument */
#define COM_MOD_DIGIT_RETURN_UNKNOWN_CALL_TYPE_DURING_TEST(m_zero) \
   do \
   { \
   } while (0)

/** true when modular test of DTMF or pules digits is on */
#define COM_MOD_DIGIT_IS_NOT_ON 1

/** prepare for modular test of verify system initialization
 *  \param m_pCtrl   - pointer control structure */
#define COM_MOD_VERIFY_SYSTEM_INIT_PREPARE(m_pCtrl) \
   do \
   { \
   }while (0)

/**  send end message for verify system initialization test
 *  \param m_zero - unused argument */
#define COM_MOD_VERIFY_SYSTEM_INIT_SEND_DONE(m_zero) \
   do \
   { \
   }while (0)

/** send message to control PC that busy tone is played during conference test
 *  \param m_pPhone - used phone
 *  \param m_nTone - CPT number */
#define COM_MOD_CONFERENCE_BUSY_TRACE(m_pPhone, m_nTone) \
   do \
   { \
   } while(0)

/** print and send msg for fxo modular test
 *  \param m_ConnID - sequential conn ID
 *  \param m_message  - msg to use */
#define COM_MOD_FXO(m_ConnID, m_message) \
   do \
   { \
   } while(0)

/** print and send msg for fxo modular test
 *  \param m_pCtrl  - pointer to control structure
 *  \param m_pPhone - pointer to phone structure
 *  \param m_pConn  - pointer to connection structure
 *  \param m_oData  - various data
 *  \param m_nCallPlace - where function is called
 *  \param m_ConnID - sequential conn ID */
#define COM_ITM_TRACES(m_pCtrl, m_pPhone, m_pConn, m_oData, m_nCallPlace, m_ConnID) \
   do \
   { \
   } while(0)

/** change peer IP address for analog to IP test
 *  \param m_pAddrFrom  - current address to change */
#define COM_ITM_SET_IP(m_pAddrFrom) \
   do \
   { \
   } while(0)

/** send used UDP port number for phone
 *  \param m_msg  - msg to use
 *  \param m_pPhone  - used phone
 *  \param m_pConn  - used connection
 *  \param m_ConnID - sequential conn ID */
#define COM_MOD_SEND_PHONE_MAPPING(m_msg, m_pPhone, m_pConn, m_ConnID) \
   do \
   { \
   } while(0)

/** Send iformation about used codec after checkcking
 *  if communication with control PC is established and modular test in progress
 *  \param m_pPhone  - pointer to phone
 *  \param m_nDataCh - data channel
 *  \param m_nCodec  - codec number
 *  \param m_nPacketization   - packetization number
 *  \param m_bDecoder   - IFX_TRUE if DECODER info,
 *                        otherwise ENCODER info is send */
#define COM_MOD_SEND_INFO_USED_CODEC(m_pPhone, m_nDataCh, m_nCodec, \
                                     m_nPacketization, m_bDecoder) \
   do \
   { \
   } while(0)

/** send first idetification message (used when first test is started)
 *  \param m_zero - unused argument */
#define COM_SEND_IDENTIFICATION(m_pPhone, m_pFXO) \
   do \
   { \
   } while(0)

/** send first idetification message (used when first test is started)
 *  \param m_aIfName - name of network interface
 *  \param m_oMy_IP_Addr - structure with new address
 *  \param m_ConnID - sequential conn ID */
#define COM_COMMUNICATION_INIT(m_aIfName, m_oMy_IP_Addr, m_ConnID) \
   do \
   { \
   } while(0)

/** if needed wait for files download
 *  \param m_pCtrl - control structure */
#define COM_RCV_FILES_ON_STARTUP(m_pCtrl) \
   do \
   { \
   } while(0)

/** set data pointer if specific file already downloaded
 *  \param m_nChipType  - chip type (e.g. danube, ar9...)
 *  \param m_nFileType  - file type (BBD, BBD_FXS, FW...)
 *  \param m_ppData     - pointer to data pointer
 *  \param m_pnDataLen  - pointer to length of data */
#define COM_RCV_FILES_GET(m_nChipType, m_nFileType, m_ppData, m_pnDataLen) \
   do \
   { \
   } while(0)

/** check if pointer allocated for dowloaded file
 *  \param m_pData - binary data */
#define COM_RCV_FILES_CHECK_IF_FREE_MEM(m_pData) \
   (IFX_NULL != m_pData)

/** reset communication sockets
 *  \param m_pCtrl - control structure
 *  \param m_rfds - set of read FDs */
#define COM_IF_COMMUNICATION_SOCKETS_RESET(m_pCtrl, m_rfds) \
   do \
   { \
   } while(0)

/** clean up after ITM
 *  \param m_zero - unused argument */
#define COM_ITM_CLEAN_UP(m_zero) \
   do \
   { \
   } while(0)

/** add ITM communication socket FD to set of FDs used by select()
 *  \param m_pRdFds - set of read FDs
 *  \param m_rwd - highest number of FD in set */
#define COM_SOCKET_FD_SET(m_pRdFds, m_rwd) \
   do \
   { \
   } while(0)

/** verify system initialization
 *  \param pBoard - pointer to board
 *  \param pInit - pointer to TAPI initialization structure */
#define COM_VERIFY_SYSTEM_INIT(pBoard, pInit) \
   do \
   { \
   } while (0)

/** receive message from communication socket
 *  \param m_pCtrl  - pointer to control structure
 *  \param m_ret  - return status
 *  \param m_bBrCast - return status */
#define COM_RECEIVE_COMMAND(m_pCtrl, m_bBrCast, m_ret) \
   do \
   { \
   } while (0)

/** send trace message through communication socket
 *  \param level    - message debug level
 *  \param msg      - pointer to the message string  */
#define COM_SEND_TRACE_MESSAGE(level, msg) \
   do \
   { \
   } while (0)


/** send DTMF command response, also executes ITM specific functions
 *  \param m_pBoard - pointer to board structure
 *  \param m_nDialedNum - number of dtmf command
 *  \param m_ret - default command response
 *  \param m_nSeqConnId - conn ID */
#define COM_EXECUTE_DTMF_COMMAND(m_pBoard, m_nAction, m_ret, m_nSeqConnId) \
   do \
   { \
   } while (0)

/** function to set CPT parameters according to CPT_ParamsTable_t
 *  \param pCtrl  - pointer to control structure
 *  \param pCPT_New - pointer to table with new CPT params */
#define COM_CPT_SET(pCtrl, pCPT_New) \
   do \
   { \
   } while (0)

/** set CID standard
 *  \param pCtrl     - pointer to control structure
 *  \param nCidStandard  - number of CID standard
 *  \param ret - return status */
#define COM_CID_STANDART_SET(pCtrl, nCidStandard, ret) \
   do \
   { \
   } while (0)

/** set default values for ITM control structure
 *  \param pCtrl      - ITM control structure
 *  \param nArgc      - number of program arguments
 *  \param pArgv      - array of pointers with program arguments
 *  \param pTD_Version   - TAPIDEMO version name
 *  \param pITM_Version  - ITM version name */
#define COM_SET_DEFAULT(pITM_Ctrl, Argc, pArgv, pTD_Version, pITM_Version) \
   do \
   { \
      COM_SetDefault(pITM_Ctrl, Argc, pArgv, pTD_Version, pITM_Version);\
   } while (0)

/**
 * Do ITM action for IE_DIALING event received
 * during S_ACIVE or S_CF_ACTIVE state
 * \param m_pPhone - pointer to phone structure */
#define COM_ST_FXS_ACTIVE_DIALING_HANDLE(m_pPhone) \
   do \
   { \
   } while (0)

#endif /* LINUX */

/** Set "failed to use custom path" flag
 *  \param m_zero - unused argument */
#define COM_FAILED_TO_USE_CUSTOM_PATH(m_zero) \
   do \
   { \
      g_pITM_Ctrl->bNoProblemWithDwldPath = IFX_FALSE; \
   } while(0)

/* ============================= */
/* Global Variable declaration   */
/* ============================= */

extern TD_ENUM_2_NAME_t TD_rgBoardTypeName[];

/* ============================= */
/* Global function declaration   */
/* ============================= */
#ifdef LINUX
IFX_return_t COM_VerifySystemInit(BOARD_t* pBoard,
                                  IFX_TAPI_CH_INIT_t* pInit);
IFX_return_t COM_PrepareVerifySystemInit(CTRL_STATUS_t* pCtrl);
IFX_return_t COM_ReceiveCommand(CTRL_STATUS_t* pctrl, IFX_boolean_t bBrCast);
extern IFX_return_t COM_SendTraceMessage(IFX_int32_t, IFX_char_t*);
extern IFX_return_t COM_SendIdentification(PHONE_t *pPhone, FXO_t *pFXO);
IFX_return_t COM_ExecuteDTMFCommand(BOARD_t* pBoard, IFX_int32_t nDialedNum,
                                    IFX_int32_t nStatus,
                                    IFX_uint32_t nSeqConnId);
extern IFX_return_t COM_CPT_Set(CTRL_STATUS_t* pCtrl,
                                CPT_ParamsTable_t* pCPT_New);
#ifdef EASY336
IFX_return_t COM_SvipToneDetection(IFX_boolean_t fEnable,
                                   IFX_uint32_t nSeqConnId);
#endif /* EASY336 */
extern IFX_return_t COM_SetGetPort(TD_OS_sockAddr_t* addr_from,
                                   IFX_int32_t nAction);
IFX_return_t COM_CID_StandardSet(CTRL_STATUS_t* pCtrl,
                                 IFX_int32_t nCidStandard);
IFX_TAPI_CID_MSG_t * COM_CID_DataSet(IFX_TAPI_CID_MSG_t *pCID_Msg,
                                     PHONE_t* pPhone);
IFX_return_t COM_CID_DataGet(CTRL_STATUS_t* pCtrl,
                             ITM_MSG_HEADER_t* pITM_MsgHeader);
IFX_return_t COM_LEC_Disable(PHONE_t* pPhone,
                             IFX_TAPI_EVENT_ID_t nEvent);
IFX_return_t COM_LEC_DataSet(PHONE_t* pPhone,
                             IFX_TAPI_WLEC_CFG_t* pLEC_Settings);
IFX_return_t COM_LEC_DataGet(CTRL_STATUS_t* pCtrl,
                             ITM_MSG_HEADER_t* pITM_MsgHead);

IFX_boolean_t COM_RcvFileGet(BOARD_TYPE_t const nChipType,
                            ITM_RCV_FILE_TYPE_t const nFileType,
                            IFX_uint8_t **ppData,
                            IFX_size_t *pnDataLen);
IFX_boolean_t COM_RcvFileCheckIfRelease(IFX_uint8_t *pData);
IFX_return_t COM_RcvFileOnStartUp(CTRL_STATUS_t* pCtrl);

IFX_return_t COM_SendInfoUsedCodec(PHONE_t* pPhone, IFX_int32_t nDataCh,
                                   IFX_TAPI_ENC_TYPE_t nCodec,
                                   IFX_TAPI_COD_LENGTH_t nPacketization,
                                   IFX_boolean_t bDecoder);
IFX_return_t COM_GetCustomCPT(IFX_int32_t nTone);
IFX_return_t COM_ITM_Traces(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                            CONNECTION_t* pConn, IFX_void_t* oData,
                            TEST_TRACE_PLACE_t nCallPlace,
                            IFX_uint32_t nSeqConnId);
IFX_return_t COM_SendCapabilityData(IFX_int32_t nBoardIdx, IFX_int32_t nDev,
                                    IFX_TAPI_CAP_t* pCapData);
IFX_return_t COM_SendSysInitDone();
IFX_return_t COM_ST_FXS_Active_Dialing_Handle_Conference(PHONE_t *pPhone);
IFX_return_t COM_ExecuteShellCommand(IFX_char_t* pCmd, IFX_char_t* pCmdOutput);
IFX_void_t COM_CleanUp(IFX_void_t);
#endif /* LINUX */
IFX_void_t COM_SetDefault(TD_ITM_CONTROL_t* pITM_Ctrl,
                          IFX_int32_t nArgc, IFX_char_t **pArgv,
                          const IFX_char_t* const pTD_Version,
                          IFX_char_t* pITM_Version);
#endif /* _CONTROL_PC_H */

