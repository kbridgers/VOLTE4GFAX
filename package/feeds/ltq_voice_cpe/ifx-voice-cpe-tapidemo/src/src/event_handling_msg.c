

/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file event_handling_msg.c
   \date 2006-03-28
   \brief This file contains the implementations handling events by using
          messages.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "common.h"
#include "event_handling.h"
#include "cid.h"
#include "voip.h"
#include "abstract.h"
#include "state_trans.h"
#ifdef LINUX
#include "tapidemo_config.h"
#endif

/* Used structures for event handling, like IFX_TAPI_EVENT_t */
#ifdef HAVE_DRV_TAPI_HEADERS
    #include "drv_tapi_event_io.h"
#else
    #error "drv_tapi is missing"
#endif

#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
   #include "common_fxo.h"
   #include "conference.h"
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */

#ifdef EASY336
   #include "board_easy336.h"
#endif /* EASY336 */

#include "faxstruct.h"




/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** representation of digits */
static IFX_char_t* pDigitsRepresentation[] =
{
   /* digits number in event data starts from 1,
      for '0' event data is set to 11 */
   "? - unknown digit",
   "1",
   "2",
   "3",
   "4",
   "5",
   "6",
   "7",
   "8",
   "9",
   /* '*' should not occur for pulse digits */
   "*",
   "0",
   /* '#' should not occur for pulse digits */
   "#"
};

/** table with association enum value -> name for
    IFX_TAPI_EVENT_ID_t. */
TD_ENUM_2_NAME_t TD_rgTapiEventName[] =
{
   {IFX_TAPI_EVENT_NONE, "IFX_TAPI_EVENT_NONE"},
   {IFX_TAPI_EVENT_IO_GENERAL_NONE, "IFX_TAPI_EVENT_IO_GENERAL_NONE"},
   {IFX_TAPI_EVENT_IO_INTERRUPT_NONE, "IFX_TAPI_EVENT_IO_INTERRUPT_NONE"},
   {IFX_TAPI_EVENT_FXS_NONE, "IFX_TAPI_EVENT_FXS_NONE"},
   {IFX_TAPI_EVENT_FXS_RING, "IFX_TAPI_EVENT_FXS_RING"},
   {IFX_TAPI_EVENT_FXS_RINGBURST_END, "IFX_TAPI_EVENT_FXS_RINGBURST_END"},
   {IFX_TAPI_EVENT_FXS_RINGING_END, "IFX_TAPI_EVENT_FXS_RINGING_END"},
   {IFX_TAPI_EVENT_FXS_ONHOOK, "IFX_TAPI_EVENT_FXS_ONHOOK"},
   {IFX_TAPI_EVENT_FXS_OFFHOOK, "IFX_TAPI_EVENT_FXS_OFFHOOK"},
   {IFX_TAPI_EVENT_FXS_FLASH, "IFX_TAPI_EVENT_FXS_FLASH"},
   {IFX_TAPI_EVENT_FXS_ONHOOK_INT, "IFX_TAPI_EVENT_FXS_ONHOOK_INT"},
   {IFX_TAPI_EVENT_FXS_OFFHOOK_INT, "IFX_TAPI_EVENT_FXS_OFFHOOK_INT"},
   {IFX_TAPI_EVENT_FXO_NONE, "IFX_TAPI_EVENT_FXO_NONE"},
   {IFX_TAPI_EVENT_FXO_BAT_FEEDED, "IFX_TAPI_EVENT_FXO_BAT_FEEDED"},
   {IFX_TAPI_EVENT_FXO_BAT_DROPPED, "IFX_TAPI_EVENT_FXO_BAT_DROPPED"},
   {IFX_TAPI_EVENT_FXO_POLARITY, "IFX_TAPI_EVENT_FXO_POLARITY"},
   {IFX_TAPI_EVENT_FXO_RING_START, "IFX_TAPI_EVENT_FXO_RING_START"},
   {IFX_TAPI_EVENT_FXO_RING_STOP, "IFX_TAPI_EVENT_FXO_RING_STOP"},
   {IFX_TAPI_EVENT_FXO_OSI, "IFX_TAPI_EVENT_FXO_OSI"},
   {IFX_TAPI_EVENT_FXO_APOH, "IFX_TAPI_EVENT_FXO_APOH"},
   {IFX_TAPI_EVENT_FXO_NOPOH, "IFX_TAPI_EVENT_FXO_NOPOH"},
   {IFX_TAPI_EVENT_LT_GR909_RDY, "IFX_TAPI_EVENT_LT_GR909_RDY"},
   {IFX_TAPI_EVENT_PULSE_NONE, "IFX_TAPI_EVENT_PULSE_NONE"},
   {IFX_TAPI_EVENT_PULSE_DIGIT, "IFX_TAPI_EVENT_PULSE_DIGIT"},
   {IFX_TAPI_EVENT_PULSE_START, "IFX_TAPI_EVENT_PULSE_START"},
   {IFX_TAPI_EVENT_DTMF_NONE, "IFX_TAPI_EVENT_DTMF_NONE"},
   {IFX_TAPI_EVENT_DTMF_DIGIT, "IFX_TAPI_EVENT_DTMF_DIGIT"},
   {IFX_TAPI_EVENT_DTMF_END, "IFX_TAPI_EVENT_DTMF_END"},
   {IFX_TAPI_EVENT_CALIBRATION_NONE, "IFX_TAPI_EVENT_CALIBRATION_NONE"},
   {IFX_TAPI_EVENT_CALIBRATION_END, "IFX_TAPI_EVENT_CALIBRATION_END"},
   {IFX_TAPI_EVENT_CALIBRATION_END_INT, "IFX_TAPI_EVENT_CALIBRATION_END_INT"},
   {IFX_TAPI_EVENT_CID_TX_NONE, "IFX_TAPI_EVENT_CID_TX_NONE"},
   {IFX_TAPI_EVENT_CID_TX_SEQ_START, "IFX_TAPI_EVENT_CID_TX_SEQ_START"},
   {IFX_TAPI_EVENT_CID_TX_SEQ_END, "IFX_TAPI_EVENT_CID_TX_SEQ_END"},
   {IFX_TAPI_EVENT_CID_TX_INFO_START, "IFX_TAPI_EVENT_CID_TX_INFO_START"},
   {IFX_TAPI_EVENT_CID_TX_INFO_END, "IFX_TAPI_EVENT_CID_TX_INFO_END"},
   {IFX_TAPI_EVENT_CID_TX_NOACK_ERR, "IFX_TAPI_EVENT_CID_TX_NOACK_ERR"},
   {IFX_TAPI_EVENT_CID_TX_RINGCAD_ERR, "IFX_TAPI_EVENT_CID_TX_RINGCAD_ERR"},
   {IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR, "IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR"},
   {IFX_TAPI_EVENT_CID_TX_NOACK2_ERR, "IFX_TAPI_EVENT_CID_TX_NOACK2_ERR"},
   {IFX_TAPI_EVENT_CID_RX_NONE, "IFX_TAPI_EVENT_CID_RX_NONE"},
   {IFX_TAPI_EVENT_CID_RX_CAS, "IFX_TAPI_EVENT_CID_RX_CAS"},
   {IFX_TAPI_EVENT_CID_RX_END, "IFX_TAPI_EVENT_CID_RX_END"},
   {IFX_TAPI_EVENT_CID_RX_CD, "IFX_TAPI_EVENT_CID_RX_CD"},
   {IFX_TAPI_EVENT_CID_RX_ERROR_READ, "IFX_TAPI_EVENT_CID_RX_ERROR_READ"},
   {IFX_TAPI_EVENT_CID_RX_ERROR1, "IFX_TAPI_EVENT_CID_RX_ERROR1"},
   {IFX_TAPI_EVENT_CID_RX_ERROR2, "IFX_TAPI_EVENT_CID_RX_ERROR2"},
   {IFX_TAPI_EVENT_TONE_GEN_NONE, "IFX_TAPI_EVENT_TONE_GEN_NONE"},
   {IFX_TAPI_EVENT_TONE_GEN_BUSY, "IFX_TAPI_EVENT_TONE_GEN_BUSY"},
   {IFX_TAPI_EVENT_TONE_GEN_END, "IFX_TAPI_EVENT_TONE_GEN_END"},
   {IFX_TAPI_EVENT_TONE_GEN_END_RAW, "IFX_TAPI_EVENT_TONE_GEN_END_RAW"},
   {IFX_TAPI_EVENT_TONE_DET_NONE, "IFX_TAPI_EVENT_TONE_DET_NONE"},
   {IFX_TAPI_EVENT_TONE_DET_RECEIVE, "IFX_TAPI_EVENT_TONE_DET_RECEIVE"},
   {IFX_TAPI_EVENT_TONE_DET_TRANSMIT, "IFX_TAPI_EVENT_TONE_DET_TRANSMIT"},
   {IFX_TAPI_EVENT_TONE_DET_CPT, "IFX_TAPI_EVENT_TONE_DET_CPT"},
   {IFX_TAPI_EVENT_FAXMODEM_NONE, "IFX_TAPI_EVENT_FAXMODEM_NONE"},
   {IFX_TAPI_EVENT_FAXMODEM_DIS, "IFX_TAPI_EVENT_FAXMODEM_DIS"},
   {IFX_TAPI_EVENT_FAXMODEM_CED, "IFX_TAPI_EVENT_FAXMODEM_CED"},
   {IFX_TAPI_EVENT_FAXMODEM_PR, "IFX_TAPI_EVENT_FAXMODEM_PR"},
   {IFX_TAPI_EVENT_FAXMODEM_AM, "IFX_TAPI_EVENT_FAXMODEM_AM"},
   {IFX_TAPI_EVENT_FAXMODEM_CNGFAX, "IFX_TAPI_EVENT_FAXMODEM_CNGFAX"},
   {IFX_TAPI_EVENT_FAXMODEM_CNGMOD, "IFX_TAPI_EVENT_FAXMODEM_CNGMOD"},
   {IFX_TAPI_EVENT_FAXMODEM_V21L, "IFX_TAPI_EVENT_FAXMODEM_V21L"},
   {IFX_TAPI_EVENT_FAXMODEM_V18A, "IFX_TAPI_EVENT_FAXMODEM_V18A"},
   {IFX_TAPI_EVENT_FAXMODEM_V27, "IFX_TAPI_EVENT_FAXMODEM_V27"},
   {IFX_TAPI_EVENT_FAXMODEM_BELL, "IFX_TAPI_EVENT_FAXMODEM_BELL"},
   {IFX_TAPI_EVENT_FAXMODEM_V22, "IFX_TAPI_EVENT_FAXMODEM_V22"},
   {IFX_TAPI_EVENT_FAXMODEM_V22ORBELL, "IFX_TAPI_EVENT_FAXMODEM_V22ORBELL"},
   {IFX_TAPI_EVENT_FAXMODEM_V32AC, "IFX_TAPI_EVENT_FAXMODEM_V32AC"},
   {IFX_TAPI_EVENT_FAXMODEM_V8BIS, "IFX_TAPI_EVENT_FAXMODEM_V8BIS"},
   {IFX_TAPI_EVENT_FAXMODEM_HOLDEND, "IFX_TAPI_EVENT_FAXMODEM_HOLDEND"},
   {IFX_TAPI_EVENT_FAXMODEM_CEDEND, "IFX_TAPI_EVENT_FAXMODEM_CEDEND"},
   {IFX_TAPI_EVENT_FAXMODEM_CAS_BELL, "IFX_TAPI_EVENT_FAXMODEM_CAS_BELL"},
   {IFX_TAPI_EVENT_FAXMODEM_V21H, "IFX_TAPI_EVENT_FAXMODEM_V21H"},
   {IFX_TAPI_EVENT_FAXMODEM_VMD, "IFX_TAPI_EVENT_FAXMODEM_VMD"},
   {IFX_TAPI_EVENT_LIN_NONE, "IFX_TAPI_EVENT_LIN_NONE"},
   {IFX_TAPI_EVENT_LIN_UNDERFLOW, "IFX_TAPI_EVENT_LIN_UNDERFLOW"},
   {IFX_TAPI_EVENT_COD_NONE, "IFX_TAPI_EVENT_COD_NONE"},
   {IFX_TAPI_EVENT_COD_DEC_CHG, "IFX_TAPI_EVENT_COD_DEC_CHG"},
   {IFX_TAPI_EVENT_COD_ROOM_NOISE, "IFX_TAPI_EVENT_COD_ROOM_NOISE"},
   {IFX_TAPI_EVENT_COD_ROOM_SILENCE, "IFX_TAPI_EVENT_COD_ROOM_SILENCE"},
   {IFX_TAPI_EVENT_RTP_NONE, "IFX_TAPI_EVENT_RTP_NONE"},
#ifdef TAPI_VERSION4
   {IFX_TAPI_EVENT_RTP_FIRST, "IFX_TAPI_EVENT_RTP_FIRST"},
   {IFX_TAPI_EVENT_RTP_EXT_BROKEN, "IFX_TAPI_EVENT_RTP_EXT_BROKEN"},
#endif /* TAPI_VERSION4 */
   {IFX_TAPI_EVENT_AAL_NONE, "IFX_TAPI_EVENT_AAL_NONE"},
   {IFX_TAPI_EVENT_RFC2833_NONE, "IFX_TAPI_EVENT_RFC2833_NONE"},
   {IFX_TAPI_EVENT_RFC2833_EVENT, "IFX_TAPI_EVENT_RFC2833_EVENT"},
   {IFX_TAPI_EVENT_KPI_NONE, "IFX_TAPI_EVENT_KPI_NONE"},
   {IFX_TAPI_EVENT_KPI_INGRESS_FIFO_FULL,
      "IFX_TAPI_EVENT_KPI_INGRESS_FIFO_FULL"},
#ifdef QOS_SUPPORT
   {IFX_TAPI_EVENT_KPI_SOCKET_FAILURE, "IFX_TAPI_EVENT_KPI_SOCKET_FAILURE"},
#endif /* QOS_SUPPORT */
   {IFX_TAPI_EVENT_T38_NONE, "IFX_TAPI_EVENT_T38_NONE"},
   {IFX_TAPI_EVENT_T38_ERROR_GEN, "IFX_TAPI_EVENT_T38_ERROR_GEN"},
   {IFX_TAPI_EVENT_T38_ERROR_OVLD, "IFX_TAPI_EVENT_T38_ERROR_OVLD"},
   {IFX_TAPI_EVENT_T38_ERROR_READ, "IFX_TAPI_EVENT_T38_ERROR_READ"},
   {IFX_TAPI_EVENT_T38_ERROR_WRITE, "IFX_TAPI_EVENT_T38_ERROR_WRITE"},
   {IFX_TAPI_EVENT_T38_ERROR_DATA, "IFX_TAPI_EVENT_T38_ERROR_DATA"},
   {IFX_TAPI_EVENT_T38_ERROR_SETUP, "IFX_TAPI_EVENT_T38_ERROR_SETUP"},
   {IFX_TAPI_EVENT_T38_FDP_REQ, "IFX_TAPI_EVENT_T38_FDP_REQ"},
   {IFX_TAPI_EVENT_T38_STATE_CHANGE, "IFX_TAPI_EVENT_T38_STATE_CHANGE"},
   {IFX_TAPI_EVENT_JB_NONE, "IFX_TAPI_EVENT_JB_NONE"},
   {IFX_TAPI_EVENT_DOWNLOAD_NONE, "IFX_TAPI_EVENT_DOWNLOAD_NONE"},
   {IFX_TAPI_EVENT_INFO_NONE, "IFX_TAPI_EVENT_INFO_NONE"},
   {IFX_TAPI_EVENT_INFO_MBX_CONGESTION, "IFX_TAPI_EVENT_INFO_MBX_CONGESTION"},
   {IFX_TAPI_EVENT_DEBUG_NONE, "IFX_TAPI_EVENT_DEBUG_NONE"},
   {IFX_TAPI_EVENT_DEBUG_CERR, "IFX_TAPI_EVENT_DEBUG_CERR"},
   {IFX_TAPI_EVENT_LL_DRIVER_NONE, "IFX_TAPI_EVENT_LL_DRIVER_NONE"},
   {IFX_TAPI_EVENT_LL_DRIVER_WD_FAIL, "IFX_TAPI_EVENT_LL_DRIVER_WD_FAIL"},
   {IFX_TAPI_EVENT_FAULT_GENERAL_NONE, "IFX_TAPI_EVENT_FAULT_GENERAL_NONE"},
   {IFX_TAPI_EVENT_FAULT_GENERAL, "IFX_TAPI_EVENT_FAULT_GENERAL"},
   {IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO, "IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO"},
   {IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO,
      "IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO"},
   {IFX_TAPI_EVENT_FAULT_LINE_NONE, "IFX_TAPI_EVENT_FAULT_LINE_NONE"},
   {IFX_TAPI_EVENT_FAULT_LINE_GK_POS, "IFX_TAPI_EVENT_FAULT_LINE_GK_POS"},
   {IFX_TAPI_EVENT_FAULT_LINE_GK_NEG, "IFX_TAPI_EVENT_FAULT_LINE_GK_NEG"},
   {IFX_TAPI_EVENT_FAULT_LINE_GK_LOW, "IFX_TAPI_EVENT_FAULT_LINE_GK_LOW"},
   {IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH, "IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH"},
   {IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP, "IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP"},
   {IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT,
      "IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT"},
   {IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END,
      "IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END"},
   {IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END,
      "IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END"},
   {IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END,
      "IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END"},
#ifndef TAPI_VERSION4
   {IFX_TAPI_EVENT_FAULT_HDLC_NONE, "IFX_TAPI_EVENT_FAULT_HDLC_NONE"},
   {IFX_TAPI_EVENT_FAULT_HDLC_FRAME_LENGTH,
      "IFX_TAPI_EVENT_FAULT_HDLC_FRAME_LENGTH"},
   {IFX_TAPI_EVENT_FAULT_HDLC_NO_KPI_PATH,
      "IFX_TAPI_EVENT_FAULT_HDLC_NO_KPI_PATH"},
   {IFX_TAPI_EVENT_FAULT_HDLC_TX_OVERFLOW,
      "IFX_TAPI_EVENT_FAULT_HDLC_TX_OVERFLOW"},
   {IFX_TAPI_EVENT_FAULT_HDLC_DISABLED, "IFX_TAPI_EVENT_FAULT_HDLC_DISABLED"},
#ifdef TD_PPD
   {IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY,
      "IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY"},
#endif /* TD_PPD */
#endif /* TAPI_VERSION4 */
   {IFX_TAPI_EVENT_FAULT_HW_NONE, "IFX_TAPI_EVENT_FAULT_HW_NONE"},
   {IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS, "IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS"},
   {IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL, "IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL"},
   {IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END,
      "IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END"},
   {IFX_TAPI_EVENT_FAULT_HW_FAULT, "IFX_TAPI_EVENT_FAULT_HW_FAULT"},
   {IFX_TAPI_EVENT_FAULT_HW_SYNC, "IFX_TAPI_EVENT_FAULT_HW_SYNC"},
   {IFX_TAPI_EVENT_FAULT_HW_RESET, "IFX_TAPI_EVENT_FAULT_HW_RESET"},
   {IFX_TAPI_EVENT_FAULT_HW_SSI_ERR, "IFX_TAPI_EVENT_FAULT_HW_SSI_ERR"},
   {IFX_TAPI_EVENT_FAULT_FW_NONE, "IFX_TAPI_EVENT_FAULT_FW_NONE"},
   {IFX_TAPI_EVENT_FAULT_FW_EBO_UF, "IFX_TAPI_EVENT_FAULT_FW_EBO_UF"},
   {IFX_TAPI_EVENT_FAULT_FW_EBO_OF, "IFX_TAPI_EVENT_FAULT_FW_EBO_OF"},
   {IFX_TAPI_EVENT_FAULT_FW_CBO_UF, "IFX_TAPI_EVENT_FAULT_FW_CBO_UF"},
   {IFX_TAPI_EVENT_FAULT_FW_CBO_OF, "IFX_TAPI_EVENT_FAULT_FW_CBO_OF"},
   {IFX_TAPI_EVENT_FAULT_FW_CBI_OF, "IFX_TAPI_EVENT_FAULT_FW_CBI_OF"},
   {IFX_TAPI_EVENT_FAULT_FW_WATCHDOG, "IFX_TAPI_EVENT_FAULT_FW_WATCHDOG"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_EVENT_ID_t"}
};

/** table with association enum value -> name for
       IFX_TAPI_EVENT_T38_STATE_t. */
TD_ENUM_2_NAME_t TD_rgT38StateName[] =
{
   {IFX_TAPI_EVENT_T38_INTERNAL0, "IFX_TAPI_EVENT_T38_INTERNAL0"},
   {IFX_TAPI_EVENT_T38_INTERNAL1, "IFX_TAPI_EVENT_T38_INTERNAL1"},
   {IFX_TAPI_EVENT_T38_INTERNAL2, "IFX_TAPI_EVENT_T38_INTERNAL2"},
   {IFX_TAPI_EVENT_T38_NEG, "IFX_TAPI_EVENT_T38_NEG"},
   {IFX_TAPI_EVENT_MOD, "IFX_TAPI_EVENT_MOD"},
   {IFX_TAPI_EVENT_DEM, "IFX_TAPI_EVENT_DEM"},
   {IFX_TAPI_EVENT_TRANS, "IFX_TAPI_EVENT_TRANS"},
   {IFX_TAPI_EVENT_PP, "IFX_TAPI_EVENT_PP"},
   {IFX_TAPI_EVENT_INT, "IFX_TAPI_EVENT_INT"},
   {IFX_TAPI_EVENT_DCN, "IFX_TAPI_EVENT_DCN"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_EVENT_T38_STATE_t"}
};

/** table with association enum value -> name for IFX_TAPI_MODULE_TYPE_t. */
TD_ENUM_2_NAME_t TD_rgTapiEventModuleName[] =
{
   {IFX_TAPI_MODULE_TYPE_NONE, "not set"},
   {IFX_TAPI_MODULE_TYPE_ALM, "alm"},
   {IFX_TAPI_MODULE_TYPE_PCM, "pcm"},
   {IFX_TAPI_MODULE_TYPE_COD, "coder"},
   {IFX_TAPI_MODULE_TYPE_CONF, "conference"},
   {IFX_TAPI_MODULE_TYPE_DECT, "dect"},
   {IFX_TAPI_MODULE_TYPE_ALL, "all"},
   {TD_MAX_ENUM_ID, "IFX_TAPI_MODULE_TYPE_t"}
};

#ifdef TAPI_VERSION4
/** table with list of callibration events */
IFX_int32_t aEventsCalibration[] ={
   IFX_TAPI_EVENT_CALIBRATION_END,
   IFX_TAPI_EVENT_NONE};

/** events that are handled without fax, DTMF and pulse event
    - those events are enabled separately */
IFX_int32_t aEvents[] ={
   /* FXS */
   IFX_TAPI_EVENT_FXS_ONHOOK,
   IFX_TAPI_EVENT_FXS_OFFHOOK,
   IFX_TAPI_EVENT_FXS_FLASH,
   /* PULSE */
   IFX_TAPI_EVENT_PULSE_DIGIT,
   /* CID */
   IFX_TAPI_EVENT_CID_TX_SEQ_START,
   IFX_TAPI_EVENT_CID_TX_SEQ_END,
   IFX_TAPI_EVENT_CID_TX_INFO_START,
   IFX_TAPI_EVENT_CID_TX_INFO_END,
   IFX_TAPI_EVENT_CID_TX_NOACK_ERR,
   IFX_TAPI_EVENT_CID_TX_RINGCAD_ERR,
   /* not handled by tapi drv
   IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR, */
   IFX_TAPI_EVENT_CID_TX_NOACK2_ERR,
   /* COD */
   IFX_TAPI_EVENT_COD_DEC_CHG,
   /* RFC 2833 */
   IFX_TAPI_EVENT_RFC2833_EVENT,
#if defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)
   /* T.38 */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_T38_ERROR_GEN, */
   IFX_TAPI_EVENT_T38_ERROR_OVLD,
   IFX_TAPI_EVENT_T38_ERROR_READ,
   IFX_TAPI_EVENT_T38_ERROR_WRITE,
   IFX_TAPI_EVENT_T38_ERROR_DATA,
   IFX_TAPI_EVENT_T38_ERROR_SETUP,
   /* not handled by tapi drv
   IFX_TAPI_EVENT_T38_FDP_REQ, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_T38_STATE_CHANGE, */
#endif /* defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW) */
   IFX_TAPI_EVENT_INFO_MBX_CONGESTION,
   /* FAULT_GENERAL */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_GENERAL_EVT_FIFO_OVERFLOW, */
   /* FAULT_LINE */
   IFX_TAPI_EVENT_FAULT_LINE_GK_POS,
   IFX_TAPI_EVENT_FAULT_LINE_GK_NEG,
   IFX_TAPI_EVENT_FAULT_LINE_GK_LOW,
   IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH,
   IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP,
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT, */
   IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END,
   IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END,
   IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END,
   /* FAULT_HW */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS, */
   IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL,
   IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END,
   IFX_TAPI_EVENT_FAULT_HW_FAULT,
   IFX_TAPI_EVENT_FAULT_HW_SYNC,
   IFX_TAPI_EVENT_FAULT_HW_RESET,
   IFX_TAPI_EVENT_FAULT_HW_SSI_ERR,
   /* FAULT_FW */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_FW_EBO_UF, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_FW_EBO_OF, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_FW_CBO_UF, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_FW_CBO_OF, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_FW_CBI_OF, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_FW_WATCHDOG, */
   /** HDLC errors */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_HDLC_FRAME_LENGTH, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_HDLC_NO_KPI_PATH, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_HDLC_TX_OVERFLOW, */
   /* not handled by tapi drv
   IFX_TAPI_EVENT_FAULT_HDLC_DISABLED, */
   /* end */
   IFX_TAPI_EVENT_NONE
};
#endif /* TAPI_VERSION4 */

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */
extern int incoming_call_flag;
/* ============================= */
/* Global function definition    */
/* ============================= */

#ifdef EVENT_COUNTER
void EventSeqNoCheck(BOARD_t *pBoard, IFX_TAPI_EVENT_t *pEvent)
{
   IFX_uint32_t *pEventSeqNo;

   if (pEvent->ch == IFX_TAPI_EVENT_ALL_CHANNELS)
      pEventSeqNo = &pBoard->evDbg[pEvent->dev].nEventDevSeqNo;
   else
      pEventSeqNo = &pBoard->evDbg[pEvent->dev].pEventChSeqNo[pEvent->ch];

   if (pEvent->nEventSeqNo != *pEventSeqNo)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
         ("%s: Event on channel %2d, dev %2d module %s(%d)\n"
          "ID: 0x%0X (%s) has incorrect number.\n"
          "Expected %d, arrived %d\n",
          pBoard->pszBoardName,
          (int)pEvent->ch, (int)pEvent->dev,
          Common_Enum2Name(pEvent->module, TD_rgTapiEventModuleName),
          pEvent->module, pEvent->id,
          Common_Enum2Name(pEvent->id, TD_rgTapiEventName),
          *pEventSeqNo, pEvent->nEventSeqNo));
   }
   (*pEventSeqNo)++;
}
#endif

/**
   Check for event status on device.

   \param nDevCtrl_FD - file descriptor for control device
   \param pBoard - pointer to board

   \return IFX_SUCCESS if some events exists, otherwise IFX_ERROR
   \remarks nDev is used only for TAPI_VERSION4
*/
IFX_return_t EVENT_Check(IFX_int32_t nDevCtrl_FD, BOARD_t* pBoard)
{
   IFX_TAPI_EVENT_t* pEvent = IFX_NULL;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i;
   TAPIDEMO_DEVICE_t* pDevice;
   IFX_char_t aDevName[MAX_DEV_NAME_LEN_WITH_NUMBER];
   IFX_char_t* pTraceIndent;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_boolean_t bCleanEvtTer_Workaround = IFX_FALSE;
#if FAX_MANAGER_ENABLED
   FAXEvent_t FaxEvent;
   IFX_int32_t fd_ch0;
	IFX_TAPI_SIG_DETECTION_t startSig;
	IFX_TAPI_SIG_DETECTION_t stopSig;
   fd_ch0 = Common_GetFD_OfCh(pBoard, 0);
#endif
   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nDevCtrl_FD), nDevCtrl_FD, IFX_ERROR);

   pDevice = ABSTRACT_GetDeviceOfDevFd(nDevCtrl_FD, pBoard->pCtrl);
   TD_PTR_CHECK(pDevice, IFX_ERROR);
   /* check event source/type */
   if (TAPIDEMO_DEV_CPU == pDevice->nType)
   {
      pEvent = &pBoard->tapiEvent;
      snprintf(aDevName, MAX_DEV_NAME_LEN_WITH_NUMBER, "%s",
               pBoard->pszBoardName);
      pTraceIndent = pBoard->pIndentBoard;
   }
   else if (TAPIDEMO_DEV_FXO == pDevice->nType)
   {
      pEvent = &pDevice->uDevice.stFxo.oTapiEventOnFxo;
      snprintf(aDevName, MAX_DEV_NAME_LEN_WITH_NUMBER, "%s No %d",
               pDevice->uDevice.stFxo.pFxoTypeName,
               pDevice->uDevice.stFxo.nDevNum);
      pTraceIndent = pDevice->uDevice.stFxo.pIndent;
      bCleanEvtTer_Workaround = IFX_TRUE;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Invalid device type %d for FD %d. (File: %s, line: %d)\n",
             pDevice->nType, nDevCtrl_FD, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check chip count number */
   if (0 >= pBoard->nChipCnt)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_EVT,
            ("%s, %d: Invalid chip count %d.\n",
             pBoard->pszBoardName, pBoard->nBoard_IDX, pBoard->nChipCnt));
   }
   /* for all devices */
   for (i = 0; i < pBoard->nChipCnt; i++)
   {
      memset(pEvent, 0, sizeof(IFX_TAPI_EVENT_t));
#ifdef TAPI_VERSION4
      pEvent->dev = i;
      nDevTmp = pEvent->dev;
#endif /* TAPI_VERSION4 */
      /* Check events on all channels */
      pEvent->ch = IFX_TAPI_EVENT_ALL_CHANNELS;

      /* Get the status of all data channel(s) */
      ret = TD_IOCTL(nDevCtrl_FD, IFX_TAPI_EVENT_GET, (IFX_int32_t) pEvent,
               nDevTmp, TD_CONN_ID_EVT);
      /*Gettting the eventdata info from the channel*/
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
               ("Err, %s -> Getting event status failed (fd %d). "
                "(File: %s, line: %d)\n",
                aDevName, nDevCtrl_FD,
                __FILE__, __LINE__));
         return IFX_ERROR;
      } /* if (IFX_ERROR == ret) */
      else
      {
         if (IFX_TRUE == bCleanEvtTer_Workaround)
         {
            /* drv_ter before version 2.7.3lq7 set unused event structure
               with random data, because of that some clean up is needed
               \todo remove this workaround when new drv_ter version
                     will be used for all currently developed branches */
            pEvent->dev = 0;
            pEvent->module = IFX_TAPI_MODULE_TYPE_ALM;
         }
         /* check event id */
         if (pEvent->id != IFX_TAPI_EVENT_NONE)
         {
#ifdef TAPI_VERSION4
#ifdef EVENT_COUNTER
            EventSeqNoCheck(pBoard, pEvent);
#endif /* EVENT_COUNTER */
            if (pEvent->module == IFX_TAPI_MODULE_TYPE_NONE &&
                pEvent->id != IFX_TAPI_EVENT_DEBUG_CERR)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                     ("Err, Invalid module type of event. "
                      "(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
#endif /* TAPI_VERSION4 */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_EVT,
               ("%s: Detected event on ch %2d, dev %2d, module %s(%d):\n"
                "%s0x%0X (%s).\n",
                aDevName, (int) pEvent->ch, (int) pEvent->dev,
                Common_Enum2Name(pEvent->module, TD_rgTapiEventModuleName),
                pEvent->module,
                pTraceIndent, pEvent->id,
                Common_Enum2Name(pEvent->id, TD_rgTapiEventName)));
#if FAX_MANAGER_ENABLED
            	printf("Event Detected %d,%s \n",pEvent->id,Common_Enum2Name(pEvent->id, TD_rgTapiEventName));
            	if(pEvent->id==IFX_TAPI_EVENT_FAXMODEM_CNGFAX)
            	{
            		   FaxEvent.FAXEventType=eFAX_FAX_CALL_DETECTED;
            		   FaxEvent.FAXEventData=0;					//Assigning the Event ata
            		   sendEventToPipe(FaxEvent);						//let us see wat will happen

            			memset(&startSig, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));
            			memset(&stopSig, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));
            			   //IFX_TAPI_SIG_CNGFAXTX- For detecting the out FAX CNG tone (OUTGoing)
            			   //IFX_TAPI_SIG_CEDRX -	For detecting the response of the remote FAX machine (OutGoing)
            			   //IFX_TAPI_SIG_CNGFAXRX-	For detecting the CNG tone from remote FAX machine(Incoming Call)
            			   //IFX_TAPI_SIG_CEDTX  -	For detecting the Called tone from the host FAX Machine(Incoming Call)
            			/* Example: start detection of Fax DIS and CNG for Fax and Modem */
            			startSig.sig = IFX_TAPI_SIG_CED|IFX_TAPI_SIG_CNGFAXTX |IFX_TAPI_SIG_CNGMODRX|IFX_TAPI_SIG_DISRX|IFX_TAPI_SIG_CEDRX|IFX_TAPI_SIG_CNGFAXRX |IFX_TAPI_SIG_CEDTX;
            			if (ioctl(fd_ch0, IFX_TAPI_SIG_DETECT_DISABLE, (IFX_int32_t) &startSig) != IFX_SUCCESS)
            				return IFX_ERROR;





            	}
            	else if(pEvent->id==IFX_TAPI_EVENT_FXS_ONHOOK)
            	{
           			memset(&startSig, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));
					memset(&stopSig, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));
					   //IFX_TAPI_SIG_CNGFAXTX- For detecting the out FAX CNG tone (OUTGoing)
					   //IFX_TAPI_SIG_CEDRX -	For detecting the response of the remote FAX machine (OutGoing)
					   //IFX_TAPI_SIG_CNGFAXRX-	For detecting the CNG tone from remote FAX machine(Incoming Call)
					   //IFX_TAPI_SIG_CEDTX  -	For detecting the Called tone from the host FAX Machine(Incoming Call)
					/* Example: start detection of Fax DIS and CNG for Fax and Modem */
					startSig.sig = IFX_TAPI_SIG_CED|IFX_TAPI_SIG_CNGFAXTX |IFX_TAPI_SIG_CNGMODRX|IFX_TAPI_SIG_DISRX|IFX_TAPI_SIG_CEDRX|IFX_TAPI_SIG_CNGFAXRX |IFX_TAPI_SIG_CEDTX;
					if (ioctl(fd_ch0, IFX_TAPI_SIG_DETECT_ENABLE, (IFX_int32_t) &startSig) != IFX_SUCCESS)
						return IFX_ERROR;
            	}
#endif
            /* Valid event was retrieved */
            return IFX_SUCCESS;
         } /* if (pEvent->id != IFX_TAPI_EVENT_NONE) */
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_EVT,
                  ("EVENT on %s - NO EVENT DETECTED.\n", aDevName));
         }
      } /* if (IFX_ERROR == ret) */
   } /* for cycle on devices end */
   /* The ioctl command executed with success, but none event was read. */
   return IFX_ERROR;
} /* EVENT_Check() */


/**
   Handles pulse digit event

   \param  pPhone  - pointer to PHONE
   \param  pBoard - pointer to board

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_HandlePulse(PHONE_t* pPhone, BOARD_t* pBoard)
{
   IFX_char_t nDigit;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pBoard->pCtrl, IFX_ERROR);

   COM_MOD_HOOKFLASH_IGNORE_PULS_DIGIT(pPhone);

   if (pPhone->nConfStarter && (pBoard->tapiEvent.ch != pPhone->nDataCh))
   {
      /* phone mapped to data channel different than in pPhone structure
         propably conference in progress - do nothing or
         else digit will be counted twice */
   }
   else
   {
      nDigit = pBoard->tapiEvent.data.pulse.digit & 0x1f;
#if (defined(XT16) || defined(EASY336))
      if (nDigit == 0)
      {
         nDigit = DIGIT_ZERO;
      }
#endif /* (defined(XT16) || defined(EASY336)) */
      /* to avoid writing outside table check number of dialed digits */
      if ((MAX_DIALED_NUM - 1) > pPhone->nDialNrCnt)
      {
         pPhone->pDialedNum[pPhone->nDialNrCnt] = nDigit;
         pPhone->nDialNrCnt++;
      }
      else
      {
         pPhone->pDialedNum[pPhone->nDialNrCnt] = nDigit;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                ("Err, Phone No %d: Dialed DTMF digits count "
                 "exceded MAX_DIALED_NUM\n",
                 (int) pPhone->nPhoneNumber));
      }
      if((sizeof(pDigitsRepresentation)/sizeof(IFX_char_t*)) > nDigit)
      {
#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: Pulse Digit: %s \n",
                (int) pPhone->nPhoneNumber,
                pDigitsRepresentation[(int) nDigit]));
#else /* TAPI_VERSION4 */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Device %d ch %d -> Pulse Digit: %s \n",
                pBoard->tapiEvent.dev, pBoard->tapiEvent.ch,
                pDigitsRepresentation[(int) nDigit]));
#endif /* TAPI_VERSION4 */
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Invalid index of array 'pDigitsRepresentation'."
                " (File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
      if (nDigit == DIGIT_HASH) /* This one is basically not possible
                                   with pulse dialing */
      {
         if (pBoard->pCtrl->pProgramArg->oArgFlags.nConference)
         {
            /* If in the middle of dialing and # is pressed
            cancel dialing and start again AND already in
            connection with one or more peers. */
            pPhone->nDialNrCnt = 0;

            pPhone->nIntEvent_FXS = IE_CONFERENCE;
         }
         else
         {
            pPhone->nIntEvent_FXS = IE_DIALING;
         }
      }
      else if (nDigit == DIGIT_STAR)
      {
         /* check if making external call with full ip number */
         if (((S_OUT_DIALING == pPhone->rgStateMachine[FXS_SM].nState) ||
              (S_CF_DIALING == pPhone->rgStateMachine[FXS_SM].nState)) &&
             (pPhone->pDialedNum[0] == DIGIT_ZERO))
         {
            pPhone->nIntEvent_FXS = IE_DIALING;
         }
         else
         {
            pPhone->nIntEvent_Config = IE_CONFIG;
         }
      }
      else
      {
         if (pPhone->rgStateMachine[CONFIG_SM].nState != S_READY)
         {
            pPhone->nIntEvent_Config = IE_CONFIG_DIALING;
         }
         else
         {
            pPhone->nIntEvent_FXS = IE_DIALING;
         }
      }
      COM_MOD_DIGIT_PULSE_HANDLE(pPhone, nDigit);
   }
   return IFX_SUCCESS;
}

/**
   Handles DTMF digit event

   \param  pPhone  - pointer to PHONE
   \param  pBoard - pointer to board

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_HandleDTMF(PHONE_t* pPhone, BOARD_t* pBoard)
{
   IFX_char_t nDigit;
#if FAX_MANAGER_ENABLED
   FAXEvent_t FaxEvent;
#endif
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pBoard->pCtrl, IFX_ERROR);

   /*  MASK = 0x00000001 : Do not react on DTMF Exeptions.
       There is another function to read digit stored in
       the digit buffer */
   nDigit = pBoard->tapiEvent.data.dtmf.digit & 0x1f;
#if FAX_MANAGER_ENABLED
   FaxEvent.FAXEventType=eFAX_EVENT_DTMF_DETECTED;
   FaxEvent.FAXEventData=nDigit;					//Assigning the Event ata
   sendEventToPipe(FaxEvent);						//let us see wat will happen
#endif


#ifndef EASY336
   if ((pPhone->nConfStarter) &&
       (TD_NOT_SET != pPhone->nDataCh) &&
       (pBoard->tapiEvent.ch != pPhone->nDataCh))
   {
      /* phone mapped to data channel different than in pPhone structure
         propably conference in progress - do nothing, or
         else digit will be counted twice */
   }
   else
#endif /* EASY336 */
   {
      /* to avoid writing outside table check number of dialed digits */
      if ((MAX_DIALED_NUM-1) > pPhone->nDialNrCnt)
      {
         pPhone->pDialedNum[pPhone->nDialNrCnt] = nDigit;
         pPhone->nDialNrCnt++;
      }
      else
      {
         pPhone->pDialedNum[pPhone->nDialNrCnt] = nDigit;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                ("Err, Phone No %d: Dialed DTMF digits count "
                 "exceded MAX_DIALED_NUM\n",
                 pPhone->nPhoneNumber));
      }
      if (0 != pBoard->tapiEvent.data.dtmf.ascii)
      {
#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: DTMF Digit: %c\n",
                pPhone->nPhoneNumber,
                pBoard->tapiEvent.data.dtmf.ascii));
#else /* TAPI_VERSION4 */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Device %d channel %d -> DTMF Digit: %c \n",
                pBoard->tapiEvent.dev, pBoard->tapiEvent.ch,
                pBoard->tapiEvent.data.dtmf.ascii));
#endif /* TAPI_VERSION4 */
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                ("Err, Phone No %d: Failed to get DTMF digit in "
                 "ASCII representation\n",
                 pPhone->nPhoneNumber));
      }

      if (nDigit == DIGIT_HASH)
      {
         if (pBoard->pCtrl->pProgramArg->oArgFlags.nConference)
         {
            /* If in the middle of dialing and # is pressed
            cancel dialing and start again AND already in
            connection with one or more peers. */
            pPhone->nDialNrCnt = 0;

            pPhone->nIntEvent_FXS = IE_CONFERENCE;
         }
         else
         {
            pPhone->nIntEvent_FXS = IE_DIALING;
         }
      }
      else if (nDigit == DIGIT_STAR)
      {
         /* check if making external call with full ip number */
         if (((S_OUT_DIALING == pPhone->rgStateMachine[FXS_SM].nState) ||
              (S_CF_DIALING == pPhone->rgStateMachine[FXS_SM].nState)) &&
             (pPhone->pDialedNum[0] == DIGIT_ZERO))
         {
            pPhone->nIntEvent_FXS = IE_DIALING;
         }
         else
         {
            pPhone->nIntEvent_Config = IE_CONFIG;
         }
      }
      else
      {
#if FAX_MANAGER_ENABLED
    	  printf("Executing the Else sate in diallig\n");
#endif

         if (pPhone->rgStateMachine[CONFIG_SM].nState !=  S_READY)
         {
            pPhone->nIntEvent_Config = IE_CONFIG_DIALING;
         }
         else
         {
            pPhone->nIntEvent_FXS = IE_DIALING;
         }
      }
      COM_MOD_DIGIT_DTMF_HANDLE(pPhone, nDigit);
   }
   return IFX_SUCCESS;
}

/**
   Handles channel exceptions, events

   \param  pPhone  - pointer to PHONE
   \param  pCtrl   - pointer to status control structure
   \param  pBoard - pointer to board

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_Handle(PHONE_t* pPhone, CTRL_STATUS_t* pCtrl,
                          BOARD_t* pBoard)
{
#if FAX_MANAGER_ENABLED
   FAXEvent_t FaxEvent;
#endif
	IFX_uint32_t nSeqConnId = TD_CONN_ID_EVT;
   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

    /* use conn id from phone if set */
   if (TD_CONN_ID_INIT != pPhone->nSeqConnId)
   {
      nSeqConnId = pPhone->nSeqConnId;
   }

#ifndef TAPI_VERSION4
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
      ("Phone No %d: Event on DevIDX %d, PhoneCh %d, DataCh %d:\n"
       "%s0x%0X (%s)\n",
       (int) pPhone->nPhoneNumber, (int) pBoard->nBoard_IDX,
       (int) pPhone->nPhoneCh, (int) pPhone->nDataCh,
       pBoard->pIndentPhone, pBoard->tapiEvent.id,
       Common_Enum2Name(pBoard->tapiEvent.id, TD_rgTapiEventName)));
#else
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
      ("Phone No %d: Event on DevIDX %d, PhoneCh %d, dev %d, DataCh %d:\n"
       "%s0x%0X (%s)\n",
       (int) pPhone->nPhoneNumber, (int) pBoard->nBoard_IDX,
       (int) pPhone->nPhoneCh, (int) pPhone->nDev, (int) pPhone->nDataCh,
       pBoard->pIndentPhone, pBoard->tapiEvent.id,
       Common_Enum2Name(pBoard->tapiEvent.id, TD_rgTapiEventName)));
#endif /* TAPI_VERSION4 */
   switch (pBoard->tapiEvent.id)
   {
      /* ON HOOK state */
      case IFX_TAPI_EVENT_FXS_ONHOOK:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Phone No %d: On-Hook detected.\n",
             (IFX_int32_t) pPhone->nPhoneNumber));
         /* Hook on - set phone event */
         pPhone->nIntEvent_FXS = IE_HOOKON;
         if ( pPhone->rgStateMachine[CONFIG_SM].nState != S_READY)
         {
            pPhone->nIntEvent_Config = IE_CONFIG_HOOKON;
         }

#if FAX_MANAGER_ENABLED				/*sanju*/
         FaxEvent.FAXEventType=eFAX_EVENT_ON_HOOK;
         FaxEvent.FAXEventData=0xff;					//Assigning the Event ata
         sendEventToPipe(FaxEvent);						//let us see wat will happen
#endif
         COM_MOD_EVENT_HOOK_ON(pPhone);
         break;
      }

      /* OFF HOOK state */
      case IFX_TAPI_EVENT_FXS_OFFHOOK:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("Phone No %d: Off-Hook detected.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         /* Hook off set phone event*/
         pPhone->nIntEvent_FXS = IE_HOOKOFF;

#if FAX_MANAGER_ENABLED
         FaxEvent.FAXEventType=eFAX_EVENT_OFF_HOOK;
         FaxEvent.FAXEventData=0xff;					//Assigning the Event ata
         sendEventToPipe(FaxEvent);						//let us see wat will happen
#endif
         COM_MOD_EVENT_HOOK_OFF(pPhone);
         break;
      }

      /* Flash hook detected */
      case IFX_TAPI_EVENT_FXS_FLASH:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Phone No %d: Flashhook detected.\n",
             (int) pPhone->nPhoneNumber));
         COM_MOD_HOOKFLASH_HANDLE(pPhone);
         pPhone->nIntEvent_FXS = IE_FLASH_HOOK;
         break;
      }
      case IFX_TAPI_EVENT_PULSE_DIGIT:
      {
         /* handle pulse digit event */
         EVENT_HandlePulse(pPhone, pBoard);
         break;
      }
      case IFX_TAPI_EVENT_DTMF_DIGIT:
      {
         /* handle dtmf digit event */
         EVENT_HandleDTMF(pPhone, pBoard);
         break;
      }

      case IFX_TAPI_EVENT_INFO_MBX_CONGESTION:
      {
#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Phone No %d: Got event IFX_TAPI_EVENT_INFO_MBX_CONGESTION,\n"
             "%smailbox congestion in downstream, packet dropped.\n",
             (IFX_int32_t) pPhone->nPhoneNumber,
             pBoard->pIndentPhone));
#else /* TAPI_VERSION4 */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Device %d, phone ch %d, got event IFX_TAPI_EVENT_INFO_MBX_CONGESTION,\n"
             "%smailbox congestion in downstream, packet dropped.\n",
             pPhone->nDev, (int) pPhone->nPhoneCh,
             pBoard->pIndentPhone));
#endif /* TAPI_VERSION4 */
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_CNGFAX:
      {
         pPhone->nIntEvent_FXS = IE_CNG_FAX;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_CNGMOD:
      {
         pPhone->nIntEvent_FXS = IE_CNG_MOD;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_CED:
      {
         pPhone->nIntEvent_FXS = IE_CED;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_CEDEND:
      {
         pPhone->nIntEvent_FXS = IE_CED_END;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_DIS:
      {
         pPhone->nIntEvent_FXS = IE_DIS;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_AM:
      {
         pPhone->nIntEvent_FXS = IE_AM;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_PR:
      {
         pPhone->nIntEvent_FXS = IE_PR;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_V8BIS:
      {
         pPhone->nIntEvent_FXS = IE_V8BIS;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_V21L:
      {
         pPhone->nIntEvent_FXS = IE_V21L;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_V18A:
      {
         pPhone->nIntEvent_FXS = IE_V18A;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_V27:
      {
         pPhone->nIntEvent_FXS = IE_V27;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_BELL:
      {
         pPhone->nIntEvent_FXS = IE_BELL;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_V22:
      {
         pPhone->nIntEvent_FXS = IE_V22;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_V22ORBELL:
      {
         pPhone->nIntEvent_FXS = IE_V22ORBELL;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_V32AC:
      {
         pPhone->nIntEvent_FXS = IE_V32AC;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_CAS_BELL:
      {
         pPhone->nIntEvent_FXS = IE_CAS_BELL;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_V21H:
      {
         pPhone->nIntEvent_FXS = IE_V21H;
         break;
      }
      case IFX_TAPI_EVENT_FAXMODEM_HOLDEND:
      {
         pPhone->nIntEvent_FXS = IE_END_DATA_TRANSMISSION;
         break;
      }
#ifdef QOS_SUPPORT
      /* IFX_TAPI_EVENT_KPI_SOCKET_FAILURE is defined for tapi version 4.8.0.0
         and next versions.
         To make backward compatibility easier, usage of this enum can be
         commented out with enablig/disabling QoS support. */
      case IFX_TAPI_EVENT_KPI_SOCKET_FAILURE:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Phone No %d: Error occured during socket operation for ch %d.\n",
             pPhone->nPhoneNumber, pBoard->tapiEvent.ch));
         pPhone->nIntEvent_FXS = IE_KPI_SOCKET_FAILURE;
         break;
      }
#endif /* QOS_SUPPORT */
      case IFX_TAPI_EVENT_T38_ERROR_GEN:
      case IFX_TAPI_EVENT_T38_ERROR_OVLD:
      case IFX_TAPI_EVENT_T38_ERROR_READ:
      case IFX_TAPI_EVENT_T38_ERROR_WRITE:
      case IFX_TAPI_EVENT_T38_ERROR_DATA:
      case IFX_TAPI_EVENT_T38_ERROR_SETUP:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("Phone No %d: Error, %s received. \n",
                pPhone->nPhoneNumber,
                Common_Enum2Name(pBoard->tapiEvent.id, TD_rgTapiEventName)));
         pPhone->nIntEvent_FXS = IE_T38_ERROR;
         break;
      }
      case IFX_TAPI_EVENT_T38_FDP_REQ:
         break;
      case IFX_TAPI_EVENT_T38_STATE_CHANGE:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("Phone No %d: IFX_TAPI_EVENT_T38_STATE_CHANGE received, "
                "data = %d (%s)). \n",
                pPhone->nPhoneNumber,
                pBoard->tapiEvent.data.t38,
                Common_Enum2Name(pBoard->tapiEvent.data.t38,
                   TD_rgT38StateName)));
         if (pBoard->tapiEvent.data.t38 == IFX_TAPI_EVENT_DCN)
         {
            /* Facsimile session finishes with DCN. */
            pPhone->nIntEvent_FXS = IE_END_DATA_TRANSMISSION;
         }
         break;
      }
      case IFX_TAPI_EVENT_RFC2833_EVENT:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("Phone No %d: IFX_TAPI_EVENT_RFC2833_EVENT received, "
                "event code = %d ). \n",
                pPhone->nPhoneNumber,
                pBoard->tapiEvent.data.rfc2833.event));
         break;
      }
      /* decoder change detected */
      case IFX_TAPI_EVENT_COD_DEC_CHG:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("Phone No %d: Voice decoding: %s, %s\n",
                pPhone->nPhoneNumber,
                pCodecName[pBoard->tapiEvent.data.dec_chg.dec_type],
                oFrameLen[pBoard->tapiEvent.data.dec_chg.dec_framelength]));
      }
         break;
      /* BEGINNING of TAPI GENERAL FAULT EVENTS */
      /* FAULT_GENERAL - IFX_TAPI_EVENT_TYPE_FAULT_GENERAL */
      case IFX_TAPI_EVENT_FAULT_GENERAL_NONE:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_GENERAL_NONE "
                "- Generic fault, no event(reserved).\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_GENERAL:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_GENERAL "
                "- General system fault (reserved).\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO "
                "- General channel fault (reserved).\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO "
                "- General device fault (reserved).\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      /* FAULT_LINE - IFX_TAPI_EVENT_TYPE_FAULT_LINE */
      case IFX_TAPI_EVENT_FAULT_LINE_NONE:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_NONE "
                "- Reserved. Line fault, no event.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_LINE_GK_POS:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_GK_POS "
                "- Ground Key, positive polarity detected.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_LINE_GK_NEG:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_GK_NEG "
                "- Ground Key, negative polarity detected.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW:
         if (pCtrl->pProgramArg->oArgFlags.nGroundStart)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
                  ("Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_GK_LOW detected.\n",
                   (IFX_int32_t) pPhone->nPhoneNumber));
            pPhone->nIntEvent_FXS = IE_FAULT_LINE_GK_LOW;
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_GK_LOW "
                "- Ground key low detected.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         }
         break;

      case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END:
         if (pCtrl->pProgramArg->oArgFlags.nGroundStart)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
                  ("Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END detected.\n",
                   (IFX_int32_t) pPhone->nPhoneNumber));
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END "
                "- Ground key low detected.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         }

      case IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH "
                "- Ground key high detected.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;

      case IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP "
                "- Overtemperature.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT "
                "- Overcurrent.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      /* IFX_TAPI_EVENT_TYPE_FAULT_HW */
      case IFX_TAPI_EVENT_FAULT_HW_NONE:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_HW_NONE "
                "- Reserved.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS "
                "- SPI access error.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL "
                "- Clock failure.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END "
                "- Clock failure end.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_HW_FAULT:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_HW_FAULT "
                "- Hardware failure.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      /* FAULT_FW - IFX_TAPI_EVENT_TYPE_FAULT_FW */
      case IFX_TAPI_EVENT_FAULT_FW_NONE:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_FW_NONE "
                "- Reserved.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_FW_EBO_UF:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_FW_EBO_UF "
                "- Event mailbox out underflow.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_FW_EBO_OF:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_FW_EBO_OF "
                "- Event mailbox out overflow.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_FW_CBO_UF:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_FW_CBO_UF "
                "- Command mailbox out underflow.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_FW_CBO_OF:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_FW_CBO_OF "
                "- Command mailbox out overflow.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      case IFX_TAPI_EVENT_FAULT_FW_CBI_OF:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_FW_CBI_OF "
                "- Command mailbox in overflow.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      /* FAULT_SW - IFX_TAPI_EVENT_TYPE_FAULT_SW */
      case IFX_TAPI_EVENT_FAULT_SW_NONE:
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FAULT: Phone No %d: IFX_TAPI_EVENT_FAULT_SW_NONE "
                "- Reserved.\n",
                (IFX_int32_t) pPhone->nPhoneNumber));
         break;
      /* END of TAPI GENERAL FAULT EVENTS */
      default:
         /* There are plenty more, but not handled at the moment, CID events
            are handled in the CID section. */
         /* TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("TAPI: Phone ch%d -> Unknown event, not handled at the moment\n",
               (int) pPhone->nPhoneCh));*/
         break;
   } /* switch */
   /* ITM only part */
   if (IFX_TRUE == pCtrl->oITM_Ctrl.fITM_Enabled)
   {
      /* check event ID */
      switch (pBoard->tapiEvent.id)
      {
         /*case IFX_TAPI_EVENT_FXS_ONHOOK:
         case IFX_TAPI_EVENT_FXS_OFFHOOK:
         case IFX_TAPI_EVENT_FXS_FLASH:
         case IFX_TAPI_EVENT_PULSE_DIGIT:
         case IFX_TAPI_EVENT_DTMF_DIGIT:
         case IFX_TAPI_EVENT_INFO_MBX_CONGESTION:
            break;*/
         case IFX_TAPI_EVENT_FAXMODEM_DIS:
         case IFX_TAPI_EVENT_FAXMODEM_CNGFAX:
         case IFX_TAPI_EVENT_FAXMODEM_CNGMOD:
         case IFX_TAPI_EVENT_FAXMODEM_CED:
         case IFX_TAPI_EVENT_FAXMODEM_CEDEND:
         case IFX_TAPI_EVENT_FAXMODEM_AM:
         case IFX_TAPI_EVENT_FAXMODEM_PR:
         case IFX_TAPI_EVENT_FAXMODEM_V8BIS:
         case IFX_TAPI_EVENT_FAXMODEM_V21L:
         case IFX_TAPI_EVENT_FAXMODEM_V18A:
         case IFX_TAPI_EVENT_FAXMODEM_V27:
         case IFX_TAPI_EVENT_FAXMODEM_BELL:
         case IFX_TAPI_EVENT_FAXMODEM_V22:
         case IFX_TAPI_EVENT_FAXMODEM_V22ORBELL:
         case IFX_TAPI_EVENT_FAXMODEM_V32AC:
         case IFX_TAPI_EVENT_FAXMODEM_CAS_BELL:
         case IFX_TAPI_EVENT_FAXMODEM_V21H:
            if (pPhone->rgoConn[0].nUsedCh == pBoard->tapiEvent.ch)
            {
               COM_ITM_TRACES(pCtrl, pPhone, &pPhone->rgoConn[0],
                              IFX_NULL, TTP_FAX_MODEM_SIGNAL,
                              nSeqConnId);
               /* do actions for LEC test also resets event if LEC test */
               COM_MOD_LEC_CHECK_FOR_RESET(pPhone, pBoard->tapiEvent.id);
            }
            break;
         case IFX_TAPI_EVENT_FAXMODEM_HOLDEND:
            if (pPhone->rgoConn[0].nUsedCh == pBoard->tapiEvent.ch)
            {
               COM_ITM_TRACES(pCtrl, pPhone, &pPhone->rgoConn[0],
                              IFX_NULL, TTP_HOLDEND_DETECTED,
                              nSeqConnId);
            }
            break;
         /*case IFX_TAPI_EVENT_T38_ERROR_GEN:
         case IFX_TAPI_EVENT_T38_ERROR_OVLD:
         case IFX_TAPI_EVENT_T38_ERROR_READ:
         case IFX_TAPI_EVENT_T38_ERROR_WRITE:
         case IFX_TAPI_EVENT_T38_ERROR_DATA:
         case IFX_TAPI_EVENT_T38_ERROR_SETUP:
         case IFX_TAPI_EVENT_T38_FDP_REQ:
            break;*/
         case IFX_TAPI_EVENT_T38_STATE_CHANGE:
         {
            if (pBoard->tapiEvent.data.t38 == IFX_TAPI_EVENT_DCN)
            {
               if (pPhone->rgoConn[0].nUsedCh == pBoard->tapiEvent.ch)
               {
                  COM_ITM_TRACES(pCtrl, pPhone, &pPhone->rgoConn[0],
                                 IFX_NULL, TTP_DCN_DETECTED,
                                 nSeqConnId);
               }
            }
            break;
         }
         /*case IFX_TAPI_EVENT_RFC2833_EVENT:*/
      case IFX_TAPI_EVENT_COD_DEC_CHG:
         /* send iformation about used codec (decoder) */
         COM_MOD_SEND_INFO_USED_CODEC(pPhone, pBoard->tapiEvent.ch,
                                      pBoard->tapiEvent.data.dec_chg.dec_type,
                                      pBoard->tapiEvent.data.dec_chg.dec_framelength,
                                      IFX_TRUE);
         break;

#ifdef DXT
         /* CID event shifted to this part only for DxT ITM tests */
         /* case IFX_TAPI_EVENT_CID_TX_SEQ_END: */
         case IFX_TAPI_EVENT_CID_TX_SEQ_END:
            if (TYPE_DUSLIC_XT == pBoard->nType)
            {
               /* Used for DxT CID modular test */
               COM_ITM_TRACES(IFX_NULL, pPhone, IFX_NULL,
                              IFX_NULL, TTP_CID_EVENT_CID_TX_SEQ_END,
                              nSeqConnId);
               COM_ITM_TRACES(IFX_NULL, pPhone, IFX_NULL,
                              IFX_SUCCESS, TTP_CID_DISABLE, nSeqConnId);
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                     ("CID: Transmission sequence ended.\n"));
            }
            break;
#endif /* DXT */
         /*case IFX_TAPI_EVENT_FAULT_GENERAL_NONE:
         case IFX_TAPI_EVENT_FAULT_GENERAL:
         case IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO:
         case IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO:
         case IFX_TAPI_EVENT_FAULT_LINE_NONE:
         case IFX_TAPI_EVENT_FAULT_LINE_GK_POS:
         case IFX_TAPI_EVENT_FAULT_LINE_GK_NEG:
         case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW:
         case IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH:
         case IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP:
         case IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT:
         case IFX_TAPI_EVENT_FAULT_HW_NONE:
         case IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS:
         case IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL:
         case IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END:
         case IFX_TAPI_EVENT_FAULT_HW_FAULT:
         case IFX_TAPI_EVENT_FAULT_FW_NONE:
         case IFX_TAPI_EVENT_FAULT_FW_EBO_UF:
         case IFX_TAPI_EVENT_FAULT_FW_EBO_OF:
         case IFX_TAPI_EVENT_FAULT_FW_CBO_UF:
         case IFX_TAPI_EVENT_FAULT_FW_CBO_OF:
         case IFX_TAPI_EVENT_FAULT_FW_CBI_OF:
         case IFX_TAPI_EVENT_FAULT_SW_NONE:
            break*/
         default:
            /* There are plenty more, but not handled at the moment, CID events
               are handled in the CID section. */
            break;
      } /* switch */
   } /* ITM only part */
   /* for EASY 3201 (DxT) CID is detected on ALM so handling it on data channel
      is not correct  */
#if !defined EASY3201 && !defined EASY3201_EVS
   if (pCtrl->pProgramArg->oArgFlags.nCID)
   {
      IFX_int32_t nEventType;
      nEventType = pBoard->tapiEvent.id & IFX_TAPI_EVENT_TYPE_MASK;
      /* check if event type handled by EVENT_Handle_DataCH() */
      if ((IFX_TAPI_EVENT_TYPE_CID == nEventType) ||
          (IFX_TAPI_EVENT_TYPE_TONE_DET == nEventType))
      {
         /* The use of phone ch is valid. If the event occurred on data
          * channel, that is connected to a phone channel, function will be
          * called for this phone channel, otherwise (which is impossible) the
          * event will be discarded */
         /* for DxT no data channel number is set.
            In such CID occured for phone channel. */
         if (TD_NOT_SET != pPhone->nDataCh)
         {
            EVENT_Handle_DataCH(pPhone->nDataCh, pPhone, pBoard, IFX_NULL);
         }
      }
   } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */
#endif /* EASY3201 */

   /* Cleanup event field */
   pBoard->tapiEvent.id = IFX_TAPI_EVENT_NONE;

   return IFX_SUCCESS;
} /* EVENT_Handle() */

/**
   Handle IFX_TAPI_EVENT_CID_RX_END.

   \param nDataCh - data channel
   \param pBoard  - pointer to board
   \param pFXO    - pointer to FXO (for CID detected its valid only if CID RX
                    FSK is used, otherwise its IFX_NULL)

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
IFX_return_t EVENT_Handle_DataCH_CID_RX_END(IFX_int32_t nDataCh,
                                            BOARD_t* pBoard,
                                            FXO_t* pFXO)
{
   IFX_return_t nRet = IFX_SUCCESS;
#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
   IFX_int32_t i, j;
   PHONE_t *pPhone;
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0>nDataCh), nDataCh, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_EVT,
         ("IFX_TAPI_EVENT_CID_RX_END occurred on data ch %d "
          "and %d data received\n",
          (int) nDataCh, pBoard->tapiEvent.data.cid_rx_end.number));

#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
   if (pFXO != IFX_NULL)
   {

      /* We received CID over FXO */
      if ((pFXO->fCID_FSK_Started) && (S_IN_RINGING == pFXO->nState))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
               ("CID was received, stop CID RX FSK.\n"));

         pFXO->fCID_FSK_Started = IFX_FALSE;

         nRet = CID_StopFSK(pFXO, VOIP_GetFD_OfCh(nDataCh, pBoard), nDataCh);
         /* stop DTMF detection */
         if (IFX_FALSE != pFXO->oCID_DTMF.nDTMF_DetectionEnabled)
         {
            FXO_DTMF_DetectionStop(pFXO, pBoard);
         }
         if (nRet != IFX_ERROR)
         {
            nRet = CID_ReadData(pFXO, nDataCh, VOIP_GetFD_OfCh(nDataCh, pBoard),
                                &pFXO->oCID_Msg,
                                pBoard->tapiEvent.data.cid_rx_end.number);
         }
         if (nRet != IFX_ERROR)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
                  ("FXO No %d: Received CID.\n",
                   pFXO->nFXO_Number));
         } /* if (nRet != IFX_ERROR) */
         else
         {
            return nRet;
         } /* if (nRet != IFX_ERROR) */

      } /* if ((pFXO->fCID_FSK_Started) && (S_IN_RINGING == pFXO->nState)) */
      else
      {
         return nRet;
      } /* if ((pFXO->fCID_FSK_Started) && (S_IN_RINGING == pFXO->nState)) */
   } /* if (pFXO != IFX_NULL) */
   else
   {
      return nRet;
   } /* if (pFXO != IFX_NULL) */
/* !!!                strat ringing with CID on all phones                !!! */
   /* for all boards */
   for (j=0; j<pBoard->pCtrl->nBoardCnt; j++)
   {
       /* if main board or EASY 3111 */
      if ((j==0) || (TYPE_DUSLIC_XT == pBoard->pCtrl->rgoBoards[j].nType))
      {
         /* for all phones */
         for (i = 0; i < pBoard->pCtrl->rgoBoards[j].nMaxPhones; i++)
         {
            pPhone = &pBoard->pCtrl->rgoBoards[j].rgoPhones[i];
            /* check phone state */
            if (S_READY == pPhone->rgStateMachine[FXS_SM].nState)
            {
               /* change phone state to S_IN_RINGING */
               pPhone->rgoConn[0].nType = FXO_CALL;
               pPhone->nIntEvent_FXS = IE_RINGING;
               pPhone->pFXO = pFXO;
               pPhone->fFXO_Call = IFX_TRUE;
               pBoard->pCtrl->bInternalEventGenerated = IFX_TRUE;
               ABSTRACT_SeqConnID_Set(pPhone, IFX_NULL, pFXO->nSeqConnId);
               /* if phone cannot play CID (e.g. DECT handset) then ringing
                  is started during state transition */
               if (0 < pBoard->pCtrl->rgoBoards[j].nMaxCoderCh)
               {
                  nRet = CID_Send(pPhone->nDataCh_FD, pPhone,
                                  IFX_TAPI_CID_HM_ONHOOK,
                                  &pFXO->oCID_Msg, IFX_NULL);
               }
               else
               {
                  nRet = CID_Send(pPhone->nPhoneCh_FD, pPhone,
                                  IFX_TAPI_CID_HM_ONHOOK,
                                  &pFXO->oCID_Msg, IFX_NULL);

               } /* if (0 < pBoard->nMaxCoderCh) */
/* #warning temporary workaround, sometimes CID_Send() causes problem on SPI
line if one is done right after another, workaround should stay until problem
is solved in drivers - Jira issue VOICECPE_SW-333 */
TD_OS_MSecSleep(10);
            } /* check phone state */
         } /* for all phones */
      } /* if main board or EASY 3111 */
   } /* for all boards */
   /* release transparent message data */
   CID_Release(&pFXO->oCID_Msg);
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */
   return nRet;
}
/**
   Cid transmission handling, uses control device and channels fds.
   Detecting CPTD on FXO data channels.
   BUT detection of signalling must be ENABLED.

   \param nDataCh - data channel
   \param pPhone  - pointer to phone
   \param pBoard  - pointer to board
   \param pFXO    - pointer to FXO (for CID detected its valid only if CID RX
                    FSK is used, otherwise its IFX_NULL)

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR

   \remark TAPI signalling must be enabled.
               for TAPI3 nDataCh is used,
               for TAPI4 pPhone is used,
*/
IFX_return_t EVENT_Handle_DataCH(IFX_int32_t nDataCh,
                                 PHONE_t *pPhone,
                                 BOARD_t* pBoard,
                                 FXO_t* pFXO)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_uint32_t nSeqConnId = TD_CONN_ID_EVT;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
#ifndef TAPI_VERSION4
   TD_PARAMETER_CHECK((0 > nDataCh), nDataCh, IFX_ERROR);
#else /* TAPI_VERSION4 */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
#endif /* TAPI_VERSION4 */

#ifndef TAPI_VERSION4
   if (IFX_NULL != pPhone)
   {
      /* use conn id from phone if set */
      if (TD_CONN_ID_INIT != pPhone->nSeqConnId)
      {
         nSeqConnId = pPhone->nSeqConnId;
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Phone No %d: Event on DataCh %d on board %s:\n"
             "%s0x%0X (%s)\n",
             pPhone->nPhoneNumber, nDataCh, pBoard->pszBoardName,
             pBoard->pIndentPhone, pBoard->tapiEvent.id,
             Common_Enum2Name(pBoard->tapiEvent.id, TD_rgTapiEventName)));
   }
   else if (IFX_NULL != pFXO)
   {
      /* use conn id from fxo if set */
      if (TD_CONN_ID_INIT != pFXO->nSeqConnId)
      {
         nSeqConnId = pFXO->nSeqConnId;
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Fxo No %d: Event on DataCh %d on board %s:\n"
             "           0x%0X (%s)\n",
             pFXO->nFXO_Number, nDataCh, pBoard->pszBoardName,
             pBoard->tapiEvent.id,
             Common_Enum2Name(pBoard->tapiEvent.id, TD_rgTapiEventName)));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("DataCh No %d (%s): Event: 0x%0X (%s)\n",
             nDataCh, pBoard->pszBoardName, pBoard->tapiEvent.id,
             Common_Enum2Name(pBoard->tapiEvent.id, TD_rgTapiEventName)));

   }
#else /* TAPI_VERSION4 */
   /* use conn id from phone if set */
   if (TD_CONN_ID_INIT != pPhone->nSeqConnId)
   {
      nSeqConnId = pPhone->nSeqConnId;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Phone No %d: Event on %s device %d, DataCh %d:\n"
          "%s0x%0X (%s)\n",
          pPhone->nPhoneNumber, pBoard->pszBoardName, pPhone->nDev,
          pPhone->nDataCh,
          pBoard->pIndentPhone, pBoard->tapiEvent.id,
          Common_Enum2Name(pBoard->tapiEvent.id, TD_rgTapiEventName)));
#endif /* TAPI_VERSION4 */
   switch (pBoard->tapiEvent.id)
   {
      /* Check cid signals */
      case IFX_TAPI_EVENT_CID_TX_SEQ_END:
         COM_ITM_TRACES(IFX_NULL, pPhone, IFX_NULL,
                        IFX_NULL, TTP_CID_EVENT_CID_TX_SEQ_END,
                        nSeqConnId);
#ifdef TAPI_VERSION4
#if (defined(EASY336) || defined(XT16))
         if (pPhone->pBoard->nType != TYPE_XT16 ||
             (pPhone->rgStateMachine[FXS_SM].nState != S_ACTIVE &&
              pPhone->nCallDirection != CALL_DIRECTION_RX))
         {
            ret = Common_CID_Disable(pPhone);
            if (ret == IFX_ERROR)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                     ("Common_CID_Disable failed. (File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               return IFX_ERROR;
            }
         }
#endif
#else /* TAPI_VERSION4 */
         COM_ITM_TRACES(IFX_NULL, pPhone, IFX_NULL,
                        IFX_SUCCESS, TTP_CID_DISABLE, nSeqConnId);
#endif /* TAPI_VERSION4 */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("CID: Data ch %d, transmission sequence ended.\n",
               (int) nDataCh));
         break;
      case IFX_TAPI_EVENT_CID_TX_INFO_END:
      {

#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("CID: Data ch %d, tx ended succesfully\n",
               (int) nDataCh));
#endif /* TAPI_VERSION4 */
         break;
      }

      /* Check cid errors */
      case IFX_TAPI_EVENT_CID_TX_RINGCAD_ERR:
      {
#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("CID: Data Ch %d, ring cadence settings error! "
               "(File: %s, line: %d)\n",
               (int) nDataCh, __FILE__, __LINE__));
#endif /* TAPI_VERSION4 */
         ret = IFX_ERROR;
         break;
      }

      case IFX_TAPI_EVENT_CID_TX_NOACK_ERR:
      {
#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("CID: Data ch %d, no acknowledge during CID sequence! "
               "(File: %s, line: %d)\n",
               (int) nDataCh, __FILE__, __LINE__));
#endif /* TAPI_VERSION4 */
         ret = IFX_ERROR;
         break;
      }

      case IFX_TAPI_EVENT_CID_TX_NONE:
      {
#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_TX_NONE occurred on data ch %d\n",
               (int) nDataCh));
#endif /* TAPI_VERSION4 */
         break;
      }

#ifndef TAPI_VERSION4
      case IFX_TAPI_EVENT_CID_TX_SEQ_START:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_TX_SEQ_START occurred on data ch %d\n",
               (int) nDataCh));
         break;
      }

      case IFX_TAPI_EVENT_CID_TX_INFO_START:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_TX_INFO_START occurred on data ch %d\n",
               (int) nDataCh));
         break;
      }

      case IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR occurred on data ch %d\n",
               (int) nDataCh));
         ret = IFX_ERROR;
         break;
      }

      case IFX_TAPI_EVENT_CID_TX_NOACK2_ERR:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_TX_NOACK2_ERR occurred on data ch %d\n",
               (int) nDataCh));
         ret = IFX_ERROR;
         break;
      }

      case IFX_TAPI_EVENT_CID_RX_NONE:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_RX_NONE occurred on data ch %d\n",
               (int) nDataCh));
         break;
      }

      case IFX_TAPI_EVENT_CID_RX_CAS:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_RX_CAS occurred on data ch %d\n",
               (int) nDataCh));
         break;
      }

      case IFX_TAPI_EVENT_CID_RX_END:
      {

         if (IFX_NULL != pFXO)
         {
            if (nDataCh == pFXO->nDataCh)
            {
               COM_ITM_TRACES(IFX_NULL, IFX_NULL, IFX_NULL,
                              pFXO, TTP_CID_EVENT_CID_RX_END, nSeqConnId);
            }
         }
         /* Handle event */
         ret = EVENT_Handle_DataCH_CID_RX_END(nDataCh, pBoard, pFXO);
         break;
      }

      case IFX_TAPI_EVENT_CID_RX_CD:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_RX_CD occurred on data ch %d\n",
               (int) nDataCh));
         break;
      }

      case IFX_TAPI_EVENT_CID_RX_ERROR_READ:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_RX_ERROR_READ occurred on data ch %d\n",
               (int) nDataCh));
         ret = IFX_ERROR;
         break;
      }

      case IFX_TAPI_EVENT_CID_RX_ERROR1:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_RX_ERROR1 occurred on data ch %d\n",
               (int) nDataCh));
         ret = IFX_ERROR;
         break;
      }

      case IFX_TAPI_EVENT_CID_RX_ERROR2:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("IFX_TAPI_EVENT_CID_RX_ERROR2 occurred on data ch %d\n",
               (int) nDataCh));
         ret = IFX_ERROR;
         break;
      }
   case IFX_TAPI_EVENT_TONE_DET_CPT:
       {
          IFX_int32_t nToneIdx;

#ifdef TD_3_CPTD_SUPPORT_DISABLED
          /* if no 3 CPTD then only one tone can be detected and tone index
            is not available in event data */
          nToneIdx = TD_DEFAULT_CPT_TO_DETECT_ON_FXO;
#else /* TD_3_CPTD_SUPPORT_DISABLED */
          nToneIdx = pBoard->tapiEvent.data.tone_det.index;
#endif /* TD_3_CPTD_SUPPORT_DISABLED */

          if (IFX_NULL != pFXO)
          {
             TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                   ("FXO No %d: Detected %s on data ch %d \n",
                    pFXO->nFXO_Number,
                    Common_Enum2Name(nToneIdx, TD_rgEnumToneIdx),
                    (int) nDataCh));
#ifdef FXO
             if (nToneIdx == BUSY_TONE_IDX)
             {
                if (IFX_SUCCESS != CONFERENCE_PeerFXO_RemoveBusy(pBoard->pCtrl,
                                                                 pFXO))
                {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                        ("Err, FXO No %d: Unable to handle busy tone for "
                         "data ch %d. (File: %s, line: %d)\n",
                         pFXO->nFXO_Number, (int) nDataCh, __FILE__, __LINE__));
                }
                COM_MOD_FXO(nSeqConnId,
                            ("FXO_TEST:BUSY_TONE::%d\n",(int) nDataCh));
             }
#endif /* FXO */
          }
          else
          {
             /* by default CPTD detection is used only for FXO */
             TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                   ("Detected %s on data ch %d "
                    "(warning no coresponding FXO)\n",
                    Common_Enum2Name(nToneIdx, TD_rgEnumToneIdx),
                    (int) nDataCh));
          }
      }
#endif /* TAPI_VERSION4 */
      break;
      case IFX_TAPI_EVENT_DTMF_DIGIT:
         if (IFX_NULL != pFXO)
         {
            /* check if ASCII  */
            if (0 != pBoard->tapiEvent.data.dtmf.ascii)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                     ("Fxo No %d: DTMF Digit: %c\n",
                      pFXO->nFXO_Number,
                      pBoard->tapiEvent.data.dtmf.ascii));
               /* collect digits for S_IN_RINGING state if CID enabled */
               if (pBoard->pCtrl->pProgramArg->oArgFlags.nCID &&
                   S_IN_RINGING == pFXO->nState)
               {
                     /** check number of dialed digits */
                     if (MAX_DIALED_NUM > pFXO->oCID_DTMF.nCnt)
                     {
                        pFXO->oCID_DTMF.acDialed[pFXO->oCID_DTMF.nCnt] =
                           pBoard->tapiEvent.data.dtmf.ascii;
                        pFXO->oCID_DTMF.nCnt++;
                     }
                     else
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                               ("Err, Fxo No %d: "
                                "More than %d digits collected\n",
                                pFXO->nFXO_Number, MAX_DIALED_NUM));
                     }
               }
            }
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                      ("Err, Fxo No %d: Failed to get DTMF digit in "
                       "ASCII representation\n",
                       pFXO->nFXO_Number));
            }
         } /* if (IFX_NULL != pFXO) */
         break;
      default:
         break;
   } /* switch */

   /* Cleanup event field */
   pBoard->tapiEvent.id = IFX_TAPI_EVENT_NONE;

   return ret;
} /* EVENT_Handle_DataCH() */

#ifndef TAPI_VERSION4
#ifdef EASY3111
/**
   If EASY 3111 is used as extension board, then for external call,
   data channels from main board are used.

   \param pCtrl      - pointer to control stucture
   \param ppPhone    - pointer to phone structure pointer
   \param ppConn     - pointer to connection structure pointer
   \param pBoard     - pointer to board structure where event was detected
   \param nDataCh    - number of data channel

   \return IFX_SUCCESS no error occured,
           otherwise return IFX_ERROR.
 */
IFX_return_t EVENT_GetOwnerEasy3111(CTRL_STATUS_t* pCtrl, PHONE_t** ppPhone,
                                    CONNECTION_t** ppConn, BOARD_t* pBoard,
                                    IFX_int32_t nDataCh)
{
   IFX_int32_t nPhone, nBoard, nConn;
   PHONE_t* pPhone;
   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(ppPhone, IFX_ERROR);
   TD_PTR_CHECK(ppConn, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* for all boards */
   for (nBoard=0; nBoard < pCtrl->nBoardCnt; nBoard++)
   {
      /* must be Duslic-xT */
      if (TYPE_DUSLIC_XT == pCtrl->rgoBoards[nBoard].nType)
      {
         /* for all phones on board */
         for (nPhone=0; nPhone < pCtrl->rgoBoards[nBoard].nMaxPhones; nPhone++)
         {
            /* set phone pointer */
            pPhone = &pCtrl->rgoBoards[nBoard].rgoPhones[nPhone];
            /* for all connections */
            for (nConn=0; nConn < pPhone->nConnCnt; nConn++)
            {
               /* must be external call */
               if ((EXTERN_VOIP_CALL == pPhone->rgoConn[nConn].nType) &&
                   (nDataCh == pPhone->rgoConn[nConn].nUsedCh))
               {
                  /* phone and connection is found */
                  *ppPhone = pPhone;
                  *ppConn = &pPhone->rgoConn[nConn];
                  return IFX_SUCCESS;
               } /* must be external call */
            } /* for all connections */
         } /* for all phones on board */
      } /* must be Duslic-xT */
   } /* for all boards */

   return IFX_SUCCESS;
}
#endif /* EASY3111 */

/**
   Function implementation for TAPIv3.
   Find phone and connection for event detected on board device control,
   also if no phone can be found then check if event on fxo.

   \param pBoard     - pointer to board structure where event was detected.
   \param nDevCtrl_FD - file descriptor for control device
   \param ppPhone    - pointer to phone structure pointer
   \param ppConn     - pointer to connection structure pointer
   \param pbIsFxo    - pointer to FXO event flag - set if event for FXO
                       is detected
   \param ppFXO      - pointer to fxo structure pointer

   \return IFX_SUCCESS if phone or fxo is found for event,
           otherwise return IFX_ERROR.
 */
IFX_return_t EVENT_GetOwnerOfEvent(BOARD_t* pBoard, IFX_int32_t nDevCtrl_FD,
                                   PHONE_t** ppPhone, CONNECTION_t** ppConn,
                                   IFX_boolean_t* pbIsFxo, FXO_t** ppFXO)
{
   IFX_int32_t i;
#ifndef EASY3201
   PHONE_t* pTmpPhone;
   CONNECTION_t* pTmpConn;
   IFX_int32_t j;
#endif /* EASY3201 */
   IFX_TAPI_EVENT_t* pTapiEvent;
   TAPIDEMO_DEVICE_t* pDevice;
#ifdef FXO
#ifdef SLIC121_FXO
   TAPIDEMO_DEVICE_t* pTempDevice;
   IFX_int32_t nCh;
   TAPIDEMO_DEVICE_FXO_t   *pFxoDevice;
#endif /* SLIC121_FXO */
#endif /* FXO */

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(ppPhone, IFX_ERROR);
   TD_PTR_CHECK(ppConn, IFX_ERROR);
   TD_PTR_CHECK(pbIsFxo, IFX_ERROR);
   TD_PTR_CHECK(ppFXO, IFX_ERROR);

   pDevice = ABSTRACT_GetDeviceOfDevFd(nDevCtrl_FD, pBoard->pCtrl);
   TD_PTR_CHECK(pDevice, IFX_ERROR);
   /* check event source/type */
   if (TAPIDEMO_DEV_CPU == pDevice->nType)
   {
      pTapiEvent = &pBoard->tapiEvent;
   }
   else if (TAPIDEMO_DEV_FXO == pDevice->nType)
   {
      pTapiEvent = &pDevice->uDevice.stFxo.oTapiEventOnFxo;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Invalid device type %d for FD %d. (File: %s, line: %d)\n",
             pDevice->nType, nDevCtrl_FD, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if(incoming_call_flag==1)
   {/*Patch Added by sanju to make an incoming call*/
	   pTapiEvent->ch=0;
	   pTapiEvent->id=IFX_TAPI_EVENT_TYPE_FXS;
   }
   /* check if channel number is ok */
   if (IFX_TAPI_EVENT_ALL_CHANNELS == pTapiEvent->ch)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Unable to get channel for event id 0x%0X (%s),"
             " channel is set to IFX_TAPI_EVENT_ALL_CHANNELS. "
             "(File: %s, line: %d)\n", pTapiEvent->id,
             Common_Enum2Name(pTapiEvent->id, TD_rgTapiEventName),
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if event number is ok */
   if (pTapiEvent->id == IFX_TAPI_EVENT_NONE)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Board %s: Event IFX_TAPI_EVENT_NONE for ch %d."
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, pTapiEvent->ch, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* reset structures */
   *ppPhone = IFX_NULL;
   *ppConn = IFX_NULL;
   *ppFXO = IFX_NULL;
   *pbIsFxo = IFX_FALSE;

   /* check event id group */
   switch (pTapiEvent->id & IFX_TAPI_EVENT_TYPE_MASK)
   {
      case IFX_TAPI_EVENT_TYPE_NONE:
         break;
         /* data channel events */
      case IFX_TAPI_EVENT_TYPE_DTMF:
      case IFX_TAPI_EVENT_TYPE_CID:
      case IFX_TAPI_EVENT_TYPE_COD:
      case IFX_TAPI_EVENT_TYPE_TONE_DET:
      case IFX_TAPI_EVENT_TYPE_TONE_GEN:
      case IFX_TAPI_EVENT_TYPE_FAXMODEM_SIGNAL:
      case IFX_TAPI_EVENT_TYPE_RFC2833:
      case IFX_TAPI_EVENT_TYPE_KPI:
      case IFX_TAPI_EVENT_TYPE_T38:
      /* if any of this events occured for Duslic-xT then it was detected on
         phone channel */
      if (TYPE_DUSLIC_XT !=pBoard->nType)
      {
         /* DXT (EASY3201) has no data chennels */
#ifndef EASY3201
         /* TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_EVT,
               ("Event on data channel\n")); */
         /* search for phone connected to this data channel */
         for (i=0; i<pBoard->nMaxPhones; i++)
         {
            /* get phone to check */
            pTmpPhone = &pBoard->rgoPhones[i];
            if (IFX_TAPI_EVENT_TONE_GEN_END == pTapiEvent->id &&
                IFX_TAPI_MODULE_TYPE_DECT == pTapiEvent->module)
            {
               /* currently (for tapi 4.8.1.0) for tapi V3 module field is set
                  only for IFX_TAPI_EVENT_TONE_GEN_END,
                  here is an example of how it should be handled */
#ifdef TD_DECT
               if (PHONE_TYPE_DECT == pTmpPhone->nType &&
                   pTmpPhone->nDectCh == pTapiEvent->ch)
               {
                  /* set DECT phone and first connection
                     (all connections use this DECT channel) */
                  *ppPhone = pTmpPhone;
                  *ppConn = &(*ppPhone)->rgoConn[0];
                  break;
               }
               else
#endif /* TD_DECT */
               {
                  /* this event cannot be assigned to this resource  */
                  continue;
               }
            }
            /* for all connections of phone */
            for (j=0; j < pTmpPhone->nConnCnt; j++)
            {
               /* get connection to check */
               pTmpConn = &pTmpPhone->rgoConn[j];
               /* only for EXTERN VOIP call data channel that is not default
                  can be used */
               if ((EXTERN_VOIP_CALL == pTmpConn->nType) &&
                   (pTapiEvent->ch == pTmpConn->nUsedCh))
               {
                  /* set phone and connection */
                  *ppPhone = pTmpPhone;
                  *ppConn = pTmpConn;
                  break;
               } /* check call type */
            } /* for all connections of phone */
            /* check if default data channel of phone */
            if ((IFX_NULL == *ppPhone) && (IFX_NULL == *ppConn) &&
                (pBoard->rgoPhones[i].nDataCh == pTapiEvent->ch))
            {
               /* set phone and connection */
               *ppPhone = &pBoard->rgoPhones[i];
               /* data channel is not used by any connection - use first one,
                  event handling functions will take care of rest */
               *ppConn = &(*ppPhone)->rgoConn[0];
            }
            /* check if connection and phone are set */
            if ((IFX_NULL != *ppPhone) && (IFX_NULL != *ppConn))
            {
               /* connection and phone were found break from for loop */
               break;
            }
         } /* for all phones on board */
#ifdef EASY3111
         /* No phone and connection found for data channel,
            this data channel can be used by extension board. */
         if ( ( (IFX_NULL == *ppPhone) || (IFX_NULL == *ppConn) ) &&
              (TYPE_DUSLIC_XT != pBoard->nType) )
         {
            EVENT_GetOwnerEasy3111(pBoard->pCtrl, ppPhone, ppConn, pBoard,
                                   pTapiEvent->ch);
         }
#endif /* EASY3111 */
#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
         /* check if using FXO */
         if (pBoard->pCtrl->pProgramArg->oArgFlags.nFXO)
         {
            /* No phone and connection found for FXS.
               Data channel can be used for FXO. */
            if ((IFX_NULL == *ppPhone) || (IFX_NULL == *ppConn))
            {
               /* get FXO */
               *ppFXO = FXO_GetFxoOfDataCh(pTapiEvent->ch, pBoard->pCtrl);
            } /* if phone and connection not found */
         } /* fxo is used */
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */
         break;
#endif /* EASY3201 */
      } /* if (TYPE_DUSLIC_XT !=pBoard->nType) */
         /* phone channels events */
      case IFX_TAPI_EVENT_TYPE_FXS:
      case IFX_TAPI_EVENT_TYPE_PULSE:
      case IFX_TAPI_EVENT_TYPE_LT:
      case IFX_TAPI_EVENT_TYPE_FAULT_LINE:
         /* phone channel event detected */
         /* TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_EVT,
               ("Event on phone channel\n")); */
         /* for all phones on board */
         for (i=0; i<pBoard->nMaxAnalogCh; i++)
         {
            /* if phone connected to phone channel */
            if (pBoard->rgoPhones[i].nPhoneCh == pTapiEvent->ch)
            {
               /* set phone and connection */
               *ppPhone = &pBoard->rgoPhones[i];
               /* can not specify connection - use first one */
               *ppConn = &(*ppPhone)->rgoConn[0];
               break;
            }
         } /* for all phones on board */
         break;
         /* FXO events */
      case IFX_TAPI_EVENT_TYPE_FXO:
         /* ALM FXO */
#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
         /* check if using FXO */
         if (pBoard->pCtrl->pProgramArg->oArgFlags.nFXO)
         {
            if ((TAPIDEMO_DEV_FXO == pDevice->nType) &&
                (pTapiEvent->ch < pDevice->uDevice.stFxo.nNumOfCh))
            {
               /* TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_EVT,
                     ("Event on FXO channel\n")); */
               *ppFXO = &pDevice->uDevice.stFxo.rgoFXO[pTapiEvent->ch];
            }
            else if (TAPIDEMO_DEV_CPU == pDevice->nType)
            {
#ifdef SLIC121_FXO
               /* Events on the fxo in SLIC121 are reported on CPU device. */
               pTempDevice = pDevice->pNext;
               while (pTempDevice != IFX_NULL)
               {
                  if ((pTempDevice->nType == TAPIDEMO_DEV_FXO) &&
                      (pTempDevice->uDevice.stFxo.nFxoType == FXO_TYPE_SLIC121))
                  {
                     pFxoDevice = &pTempDevice->uDevice.stFxo;
                     for (nCh=0; nCh < pFxoDevice->nNumOfCh; nCh++)
                     {
                        if (pFxoDevice->rgoFXO[nCh].nFxoCh == pTapiEvent->ch)
                        {
                           *ppFXO = &pFxoDevice->rgoFXO[nCh];
                           /* It is necessary to copy event to the fxo structure
                              because fuction EVENT_FXO_Handle() expects
                              events there. */
                           memcpy((void *) &pFxoDevice->oTapiEventOnFxo,
                                  (void*) pTapiEvent,
                                  sizeof(IFX_TAPI_EVENT_t));
                           break;
                        }
                     }
                  }
                  pTempDevice = pTempDevice->pNext;
               } /* while */
#endif /* SLIC121_FXO */
            }
         } /* fxo is used */
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */
         break;
      case IFX_TAPI_EVENT_TYPE_IO_GENERAL:
      case IFX_TAPI_EVENT_TYPE_IO_INTERRUPT:
      case IFX_TAPI_EVENT_TYPE_CALIBRATION:
      case IFX_TAPI_EVENT_TYPE_LIN:
      case IFX_TAPI_EVENT_TYPE_RTP:
      case IFX_TAPI_EVENT_TYPE_AAL:
      case IFX_TAPI_EVENT_TYPE_DEBUG:
         /* this events are not handled by tapidemo for now */
         break;
      default:
         /* unknown resource type */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
               ("Err, Unable to get resource type for event 0x%0X (%s),"
                " channel (%d), module %s(%d). (File: %s, line: %d)\n",
                pTapiEvent->id,
                Common_Enum2Name(pTapiEvent->id, TD_rgTapiEventName),
                pTapiEvent->ch,
                Common_Enum2Name(pTapiEvent->module, TD_rgTapiEventModuleName),
                pTapiEvent->module,
                __FILE__, __LINE__));
         break;
   } /* switch (pTapiEvent->id & IFX_TAPI_EVENT_TYPE_MASK) */

   /* check if FXO was found */
   if (IFX_NULL != *ppFXO)
   {
      *pbIsFxo = IFX_TRUE;
      return IFX_SUCCESS;
   }
   /* check if phone and connection were found */
   if ((IFX_NULL != *ppPhone) && (IFX_NULL != *ppConn))
   {
      return IFX_SUCCESS;
   }
   /* no phone/fxo found for this event */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_EVT,
      ("Board %s: channel %d, event id 0x%0X (%s), module %s(%3d) no owner found.\n",
       pBoard->pszBoardName, pTapiEvent->ch, pTapiEvent->id,
       Common_Enum2Name(pTapiEvent->id, TD_rgTapiEventName),
       Common_Enum2Name(pTapiEvent->module, TD_rgTapiEventModuleName),
       pTapiEvent->module));
   return IFX_ERROR;
}
#else /*  TAPI_VERSION4 */
/**
   Function implementation for TAPIv4.
   Acording to event module type(IFX_TAPI_MODULE_TYPE_COD,
   IFX_TAPI_MODULE_TYPE_ALM..) function searches for phone on witch detected
   event occured.

   \param pBoard     - pointer to board structure where event was detected.
   \param nDevCtrl_FD - file descriptor for control device
   \param ppPhone    - pointer to phone structure pointer
   \param ppConn     - pointer to connection structure pointer
   \param pbIsFxo    - pointer to FXO event flag - set if event for FXO
                       is detected
   \param ppFXO      - pointer to fxo structure pointer

   \return IFX_SUCCESS if phone or fxo is found for detected event,
           otherwise return IFX_ERROR.
 */
IFX_return_t EVENT_GetOwnerOfEvent(BOARD_t* pBoard, IFX_int32_t nDevCtrl_FD,
                                   PHONE_t** ppPhone, CONNECTION_t** ppConn,
                                   IFX_boolean_t* pbIsFxo, FXO_t** ppFXO)
{
   IFX_int32_t i;
#ifndef XT16
   IFX_int32_t j;
#endif /* XT16 */
   IFX_TAPI_EVENT_t* pTapiEvent;
   BOARD_t* pTmpBoard = IFX_NULL;
   IFX_int32_t m = 0;
#ifdef EASY336
   IFX_int32_t k;
   RM_SVIP_RU_t svipRU;
   IFX_TAPI_DEV_TYPE_t devType;
#endif /* EASY336 */
   TAPIDEMO_DEVICE_t* pDevice;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(ppPhone, IFX_ERROR);
   TD_PTR_CHECK(ppConn, IFX_ERROR);
   TD_PTR_CHECK(pbIsFxo, IFX_ERROR);
   TD_PTR_CHECK(ppFXO, IFX_ERROR);

   /* get event structure */
   pDevice = ABSTRACT_GetDeviceOfDevFd(nDevCtrl_FD, pBoard->pCtrl);
   TD_PTR_CHECK(pDevice, IFX_ERROR);

   /* check event source/type */
   if (TAPIDEMO_DEV_CPU == pDevice->nType)
   {
      pTapiEvent = &pBoard->tapiEvent;
   }
   else if (TAPIDEMO_DEV_FXO == pDevice->nType)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Invalid input argument, FXO not implemented for board %s."
             " (File: %s, line: %d)\n", pBoard->pszBoardName,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Invalid device type %d for FD %d. (File: %s, line: %d)\n",
             pDevice->nType, nDevCtrl_FD, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* check if channel number is ok */
   if (IFX_TAPI_EVENT_ALL_CHANNELS == pTapiEvent->ch)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Unable to get channel for event  id 0x%08X, (%s)"
             " channel is set to IFX_TAPI_EVENT_ALL_CHANNELS. "
             "(File: %s, line: %d)\n", pTapiEvent->id,
             Common_Enum2Name(pTapiEvent->id, TD_rgTapiEventName),
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if device number is ok */
   if (IFX_TAPI_EVENT_ALL_DEVICES == pTapiEvent->dev)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Unable to get device for event  id 0x%08X (%s),"
             " device is set to IFX_TAPI_EVENT_ALL_DEVICES. "
             "(File: %s, line: %d)\n", pTapiEvent->id,
             Common_Enum2Name(pTapiEvent->id, TD_rgTapiEventName),
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if event number is ok */
   if (pTapiEvent->id == IFX_TAPI_EVENT_NONE)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Board %s: Event IFX_TAPI_EVENT_NONE for ch %d, dev %d,"
             " module %d. (File: %s, line: %d)\n",
             pBoard->pszBoardName, pTapiEvent->ch, pTapiEvent->dev,
             pTapiEvent->module, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* reset structures */
   *ppPhone = IFX_NULL;
   *ppConn = IFX_NULL;
   *ppFXO = IFX_NULL;
   *pbIsFxo = IFX_FALSE;

   /* set board to check for phone */
   pTmpBoard = pBoard;
   /* phones on more than one board must be checked */
   while(1)
   {
      /* for all phones on board */
      for (i=0; i<pTmpBoard->nMaxPhones; i++)
      {
         /* set phone and connection */
         *ppPhone = &pTmpBoard->rgoPhones[i];
         *ppConn = &(*ppPhone)->rgoConn[0];
         /* chek if event detected for this phone */
         if (pTapiEvent->dev == (*ppPhone)->nDev &&
             pTapiEvent->ch == (*ppPhone)->nPhoneCh &&
             pTapiEvent->module == IFX_TAPI_MODULE_TYPE_ALM)
         {
            /* phone is found */
            return IFX_SUCCESS;
         } /* if (pTapiEvent->module == ...) */
#ifdef EASY336
         else if (pTapiEvent->module == IFX_TAPI_MODULE_TYPE_COD)
         {
            /* for all voip ids */
            for (j = 0; j < (*ppPhone)->nVoipIDs; j++)
            {
               /* chec if event for this voip id */
               if (SVIP_RM_Success ==
                   SVIP_RM_VoipIdRUCodGet((*ppPhone)->pVoipIDs[j], &svipRU) &&
                   svipRU.module != IFX_TAPI_MODULE_TYPE_NONE &&
                   pTapiEvent->dev == svipRU.nDev &&
                   pTapiEvent->ch == svipRU.nCh)
               {
                  /* for all connections */
                  for (k=0; k<(*ppPhone)->nConnCnt; k++)
                  {
                     /* check connection id */
                     if ((*ppPhone)->pVoipIDs[j] ==
                        (*ppPhone)->rgoConn[k].voipID)
                     {
                        /* set connection */
                        *ppConn = &(*ppPhone)->rgoConn[k];
                     } /* check if this connection */
                  } /* for all connections */
                  /* phone is found */
                  return IFX_SUCCESS;
               } /* chec if event for this voip id */
            } /* for all voip ids */
         } /* if (pTapiEvent->module == ...) */
         else if (pTapiEvent->module == IFX_TAPI_MODULE_TYPE_PCM)
         {
            /* get device type */
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
            if (SVIP_RM_Success ==
                SVIP_RM_LineIdRUSigDetGet((*ppPhone)->lineID, &svipRU) &&
                svipRU.module == IFX_TAPI_MODULE_TYPE_PCM &&
                svipRU.devType == devType &&
                pTapiEvent->dev == svipRU.nDev &&
                pTapiEvent->ch == svipRU.nCh)
            {
               /* phone is found */
               return IFX_SUCCESS;
            }
         } /* if (pTapiEvent->module == ...) */
#endif /* EASY336 */
      } /* for all phones */
      /* event didn't belong to any phone from this board,
         check phones on next board */
      m++;
      /* events for xT-16 can occure on SVIP */
      if ((m < pBoard->pCtrl->nBoardCnt)  && (pBoard->nType == TYPE_SVIP) &&
          (pBoard->pCtrl->rgoBoards[m].nType == TYPE_XT16))
      {
         /* search for phone on this board */
         pTmpBoard = &pBoard->pCtrl->rgoBoards[m];
      }
      else
      {
         /* failed to get phone for this event */
         break;
      }
   } /* while(1) */
   if ((pTapiEvent->id & IFX_TAPI_EVENT_TYPE_MASK) != IFX_TAPI_EVENT_TYPE_DEBUG)
   {
      /* no phone/fxo found for this event */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
            ("Err, Board %s: Unknown phone for event: channel %d, dev %d,"
             " module %s(%d), event id 0x%08X (%s). (File: %s, line: %d)\n",
             pBoard->pszBoardName, pTapiEvent->ch, pTapiEvent->dev,
             Common_Enum2Name(pTapiEvent->module, TD_rgTapiEventModuleName),
             pTapiEvent->module,
             pTapiEvent->id, Common_Enum2Name(pTapiEvent->id, TD_rgTapiEventName),
             __FILE__, __LINE__));
   }
   return IFX_ERROR;
}
#endif /*  TAPI_VERSION4 */

/**
   FXO calls and local calls can set phone internal event.
   When such event is generated also pCtrl->bInternalEventGenerated flag
   must be set. This function calls ST_HandleState_FXS() for phones with
   internal event set to handle them.

   \param  pCtrl   - pointer to status control structure

   \return no return value
 */
IFX_return_t EVENT_InternalEventHandle(CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t nBoard, nPhone;
   CONNECTION_t* pConn = IFX_NULL;
   PHONE_t* pPhone = IFX_NULL;

   /* check input parameter */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* do while IFX_TRUE == pCtrl->bInternalEventGenerated */
   do
   {
      /* reset internal event flag */
      pCtrl->bInternalEventGenerated = IFX_FALSE;
      /* EVENT_FXO_Handle can generate internal events for phones - handle
       * them. */
      for (nBoard=0; nBoard < pCtrl->nBoardCnt; nBoard++)
      {
         for (nPhone=0; nPhone<pCtrl->rgoBoards[nBoard].nMaxPhones; nPhone++)
         {
            pPhone = &pCtrl->rgoBoards[nBoard].rgoPhones[nPhone];
            /* if internal event is set */
            if (IE_NONE != pPhone->nIntEvent_FXS)
            {
               /* check if local connection is specified */
               if (IFX_NULL != pPhone->pConnFromLocal)
               {
                  /* set connection - propably FXO call */
                  pConn = pPhone->pConnFromLocal;
                  pPhone->pConnFromLocal = IFX_NULL;
               }
               else
               {
                  /* use first connection - propably FXO call */
                  pConn = &pPhone->rgoConn[0];
               }

               while (pPhone->nIntEvent_FXS != IE_NONE)
               {
                  /* Handle internal event */
                  ST_HandleState_FXS(pCtrl, pPhone, pConn);
               }
            } /* if internal event is set */
         } /* for all phones */
      } /* for all boards */
   /* check if internal event was generated */
   } while (IFX_TRUE == pCtrl->bInternalEventGenerated);

   return IFX_SUCCESS;
} /* EVENT_InternalEventHandle() */



#if (!defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) \
&& !defined(EASY336) && !defined(XT16))
/**
   Handles fxo ring start event.

   \param  pFXO  - pointer to fxo structure
   \param  pCtrl   - pointer to status control structure
   \param  pBoard - pointer to board

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_FXO_Handle_RingStart(FXO_t* pFXO, CTRL_STATUS_t* pCtrl,
                                        BOARD_t* pBoard)
{
   IFX_return_t ret;
   IFX_int32_t i = 0, j;
   IFX_int32_t nReadyPhonesCount = 0, nRingingPhonesCount = 0, nMaxPhones = 0;
   PHONE_t *pPhone;
   IFX_uint32_t nSeqConnId = TD_CONN_ID_EVT;

   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   if (TD_CONN_ID_INIT != pFXO->nSeqConnId)
   {
      nSeqConnId = pFXO->nSeqConnId;
   }

   /* send message for config detection test */
   COM_SEND_IDENTIFICATION(IFX_NULL, pFXO);

   /* if FXO is already in active state then ignore detected event,
      it was observed that ring start event was detected after FXO call
      was established */
   if (S_ACTIVE == pFXO->nState)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("FXO No %d: %s No %d (ch %d): ring start detected "
             "during S_ACTIVE state.\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh));
      return IFX_SUCCESS;
   }
   if (0 != pBoard->nBoard_IDX)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("FXO No %d: %s No %d (ch %d): Invalid board %s(%d),\n"
             "          only main board can be used for FXO call.\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh,
             pBoard->pszBoardName, pBoard->nBoard_IDX));
      return IFX_ERROR;
   }
   /* set new timeout */
   FXO_RefreshTimeout(pFXO);
   /* count available phones */
   for (j=0; j<pCtrl->nBoardCnt; j++)
   {
      /* for incomin FXO call phones on main board can be used,
         also phones from EASY 3111 (with DxT) can be used */
      if ((0 == j) || (TYPE_DUSLIC_XT == pCtrl->rgoBoards[j].nType))
      {
         nMaxPhones += pCtrl->rgoBoards[j].nMaxPhones;
      } /* if main board or EASY 3111 */
   } /* for all boards */
   /* check number of analog channels */
   if (nMaxPhones <= 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("FXO No %d: %s No %d (ch %d): Board %s(%d) has no analog channels,\n"
             "          Unable to make FXO call (File: %s, line: %d).\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh,
             pBoard->pszBoardName, pBoard->nBoard_IDX,
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* for all boards */
   for (j=0; j<pCtrl->nBoardCnt; j++)
   {
      /* for incomin FXO call phones on main board can be used,
         also phones from EASY 3111 (with DxT) can be used */
      if ((0 == j) || (TYPE_DUSLIC_XT == pCtrl->rgoBoards[j].nType))
      {
         /* check if phones are avaible */
         for (i=0; i < pCtrl->rgoBoards[j].nMaxPhones; i++)
         {
            pPhone = &pCtrl->rgoBoards[j].rgoPhones[i];
            /* check if phone in ready state */
            if (pPhone->rgStateMachine[FXS_SM].nState == S_READY)
            {
               nReadyPhonesCount++;
            }
            /* check if phone in ringing state conected with this fxo */
            else if ((pPhone->rgStateMachine[FXS_SM].nState ==  S_IN_RINGING) &&
                     (pPhone->fFXO_Call = IFX_TRUE) &&
                     (pPhone->pFXO == pFXO))
            {
               nRingingPhonesCount++;
            }
         } /* for (i=0; i < pRingBoard->nMaxAnalogCh; i++) */
      } /* if main board or EASY 3111 */
   } /* for all boards */
   /* check if new FXO connection can be started */
   if (S_READY == pFXO->nState)
   {
      /* check if ringing on phone can be started */
      if (0 != nReadyPhonesCount)
      {
         if (IFX_SUCCESS != FXO_PrepareConnection(pCtrl->pProgramArg,
                                                  pFXO, pBoard))
         {
            /* Could not prepare fxo connection */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, FXO No %d: %s No %d (ch %d): "
                   "Could not prepare FXO connection. (File: %s, line: %d)\n",
                   pFXO->nFXO_Number,  pFXO->pFxoDevice->pFxoTypeName,
                   pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh,
                   __FILE__, __LINE__));
            return IFX_ERROR;
         }
         /* update conn ID */
         if (TD_CONN_ID_INIT != pFXO->nSeqConnId)
         {
            nSeqConnId = pFXO->nSeqConnId;
         }
         COM_ITM_TRACES(pCtrl, IFX_NULL, IFX_NULL,
                        pFXO, TTP_CID_EVENT_FXO_RING_START, nSeqConnId);
      }
      else
      {
         /* Could not prepare fxo connection, no phones are available */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d): "
                "Could not prepare FXO connection, "
                "no phones in S_READY state.\n",
                pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh));

         /* If all phones are busy, we need to inform caller about it */
         /* We need busy tone for caller
            - so we hook-off and hook-on on FXO fd. */
         ret = TD_IOCTL(pFXO->nFxoCh_FD, IFX_TAPI_FXO_HOOK_SET,
                        IFX_TAPI_FXO_HOOK_OFFHOOK, TD_DEV_NOT_SET, nSeqConnId);
         if (IFX_ERROR == ret)
         {
             TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                   ("Err, FXO No %d: %s No %d (ch %d): failed to set off-hook. "
                    "(File: %s, line: %d)\n",
                    pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                    pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
                    __FILE__, __LINE__));
         }
         ret = TD_IOCTL(pFXO->nFxoCh_FD, IFX_TAPI_FXO_HOOK_SET,
                        IFX_TAPI_FXO_HOOK_ONHOOK, TD_DEV_NOT_SET, nSeqConnId);
         if (IFX_ERROR == ret)
         {
             TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                   ("Err, FXO No %d: %s No %d (ch %d): failed to set on-hook."
                    "(File: %s, line: %d)\n",
                    pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                    pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
                    __FILE__, __LINE__));
         }

         return IFX_SUCCESS;
      }
   }
   /* check if cid enabled */
   if (pCtrl->pProgramArg->oArgFlags.nCID)
   {
      if (IFX_TRUE != pFXO->fFirstStopRingEventOccured)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d): CID RX FSK will be started "
                "on first stop ring event,\n"
                "          ringing will start on next fxo ring start event.\n",
                pFXO->nFXO_Number,  pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh));
         /* must wait for next ring start to start ringing on phones */
         return IFX_SUCCESS;
      } /* if (IFX_FALSE == pFXO->fFirstStopRingEventOccured) */
      /* check if CID receiving is still on */
      if (IFX_TRUE == pFXO->fCID_FSK_Started)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d): "
                "Second ring event detected, stop CID RX FSK.\n",
                pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh));

         /* Stop FSK and change fxo CID flag state */
         CID_StopFSK(pFXO, VOIP_GetFD_OfCh(pFXO->nDataCh, pBoard),
                     pFXO->nDataCh);
         pFXO->fCID_FSK_Started = IFX_FALSE;
      } /* if (IFX_TRUE == pFXO->fCID_FSK_Started) */
      /* stop DTMF detection */
      if (IFX_FALSE != pFXO->oCID_DTMF.nDTMF_DetectionEnabled)
      {
         FXO_DTMF_DetectionStop(pFXO, pBoard);
      }
   } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */

   /* check if this fxo has any ringing phones */
   if (0 == nRingingPhonesCount)
   {
      /* check if any phone is avaible */
      if (0 == nReadyPhonesCount)
      {
         /* Could not start ringing, no phones are avaible */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, FXO No %d: %s No %d (ch %d): "
                "Could not start ringing, no phones in S_READY state.\n",
                pFXO->nFXO_Number,  pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nNumOfCh,  pFXO->nFxoCh));
         /* cancel fxo call preparation */
         return FXO_EndConnection(pCtrl->pProgramArg, pFXO, IFX_NULL,
                                  IFX_NULL, pBoard);
      } /* if (0 == nReadyPhonesCount) */
      else
      {
         /* check if DTMF CID was received */
         if (pBoard->pCtrl->pProgramArg->oArgFlags.nCID &&
             0 != pFXO->oCID_DTMF.nCnt)
         {
            /* try to send CID */
            if (IFX_SUCCESS != FXO_ReceivedCID_DTMF_Send(pFXO, pBoard))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                     ("Err, FXO No %d: FXO_CID_DTMF_Send() failed."
                      "(File: %s, line: %d)\n",
                      pFXO->nFXO_Number, __FILE__, __LINE__));            }
            else
            {
               return IFX_SUCCESS;
            }
         }
         /* CID was not enabled or not received, use this structure as flag,
            so state machine will know if normal ringing must start or
            if ringing is started with CID send (CID send will set
            pFXO->oCID_Msg.txMode = IFX_TAPI_CID_HM_ONHOOK) */
         pFXO->oCID_Msg.txMode = IFX_TAPI_CID_HM_OFFHOOK;

         /* for all boards */
         for (j=0; j<pCtrl->nBoardCnt; j++)
         {
            /* for incomin FXO call phones on main board can be used,
               also phones from EASY 3111 (with DxT) can be used */
            if ((0 == j) || (TYPE_DUSLIC_XT == pCtrl->rgoBoards[j].nType))
            {
               /* Start ringing on all phones, but only first time */
               for (i = 0; i < pCtrl->rgoBoards[j].nMaxPhones; i++)
               {
                  pPhone = &pCtrl->rgoBoards[j].rgoPhones[i];
                  /* check if phone in ready state */
                  if (pPhone->rgStateMachine[FXS_SM].nState == S_READY)
                  {
                     pPhone->rgoConn[0].nType = FXO_CALL;
                     pPhone->nIntEvent_FXS = IE_RINGING;
                     pPhone->pFXO = pFXO;
                     pPhone->fFXO_Call = IFX_TRUE;
                     pCtrl->bInternalEventGenerated = IFX_TRUE;
                  } /* check if phone in ready state */
               } /* for all phones on board */
            } /* if main board or EASY 3111 */
         } /* for all boards */
      } /* if (0 == nReadyPhonesCount) */
   } /* if (0 == nRingingPhonesCount) */

   return IFX_SUCCESS;
}

/**
   Handles fxo ring stop event.

   \param  pFXO  - pointer to fxo structure
   \param  pCtrl   - pointer to status control structure
   \param  pBoard - pointer to main board

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_FXO_Handle_RingStop(FXO_t* pFXO, CTRL_STATUS_t* pCtrl,
                                       BOARD_t* pBoard)
{
   IFX_uint32_t nSeqConnId = TD_CONN_ID_EVT;

   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* if conn id is not set then this is all about event */
   if (TD_CONN_ID_INIT != pFXO->nSeqConnId)
   {
      nSeqConnId = pFXO->nSeqConnId;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("FXO No %d: %s No %d (ch %d): End of ring burst.\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh));
   /* fxo ring stop event is handled only
      if pFXO->nState is set to S_IN_RINGING */
   if (S_IN_RINGING != pFXO->nState)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("FXO No %d: %s No %d (ch %d): ring stop detected during %s state.\n",
             pFXO->nFXO_Number,  pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh,
             Common_Enum2Name(pFXO->nState, TD_rgStateName)));
      return IFX_SUCCESS;
   }

   FXO_RefreshTimeout(pFXO);
   /* if CID is used */
   if (pCtrl->pProgramArg->oArgFlags.nCID)
   {
      /* check if this is first fxo ring stop event */
      if (IFX_FALSE == pFXO->fFirstStopRingEventOccured)
      {
         COM_ITM_TRACES(pCtrl, IFX_NULL, IFX_NULL,
                        pFXO, TTP_CID_EVENT_FXO_RING_STOP, nSeqConnId);

         /* get ring board, on this board phones will be ringing */
         if (0 != pBoard->nBoard_IDX)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                  ("FXO No %d: %s No %d (ch %d): Invalid board %s(%d),\n"
                   "          only main board can be used for FXO call.\n",
                   pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                   pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh,
                   pBoard->pszBoardName, pBoard->nBoard_IDX));
            return IFX_ERROR;
         }
         if ((pBoard->nMaxPhones == 0) && (pBoard->nBoard_IDX > 0))
         {
            /* This is the extension board without phone channels.
               Start ringing on the evaluation board. */
            pBoard = &pBoard->pCtrl->rgoBoards[0];
         }
         /* Afterthat start FSK for CID detection */
         if (IFX_SUCCESS != CID_StartFSK(pFXO,
                               VOIP_GetFD_OfCh(pFXO->nDataCh, pBoard),
                               IFX_TAPI_CID_HM_ONHOOK))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, FXO No %d: %s No %d (ch %d): "
                   "Err, Could not start CID FSK. (File: %s, line: %d)\n",
                   pFXO->nFXO_Number,  pFXO->pFxoDevice->pFxoTypeName,
                   pFXO->pFxoDevice->nNumOfCh, pFXO->nFxoCh,
                   __FILE__, __LINE__));
         }
         else
         {
            pFXO->fCID_FSK_Started = IFX_TRUE;

            COM_ITM_TRACES(pCtrl, IFX_NULL, IFX_NULL,
                           pFXO, TTP_CID_FSK_RX_START, nSeqConnId);
         }
         /* start DTMF detection for CID */
         if (IFX_SUCCESS != FXO_DTMF_DetectionStart(pFXO, pBoard))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, FXO No %d: FXO_DTMF_DetectionStart() failed. "
                   "(File: %s, line: %d)\n",
                   pFXO->nFXO_Number, __FILE__, __LINE__));
         }
      }
   } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */

   pFXO->fFirstStopRingEventOccured = IFX_TRUE;

   return IFX_SUCCESS;
}

/**
   Handles channel exceptions, events

   \param  pFXO  - pointer to fxo structure
   \param  pCtrl   - pointer to status control structure
   \param  pBoard - pointer to board

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_FXO_Handle(FXO_t* pFXO, CTRL_STATUS_t* pCtrl,
                              BOARD_t* pBoard)
{
   IFX_TAPI_EVENT_t* pEvent = IFX_NULL;
   IFX_uint32_t nSeqConnId = TD_CONN_ID_EVT;

   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* set event structure */
   pEvent = &pFXO->pFxoDevice->oTapiEventOnFxo;
   /* if conn id is not set then this is all about event */
   if (TD_CONN_ID_INIT != pFXO->nSeqConnId)
   {
      nSeqConnId = pFXO->nSeqConnId;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("FXO No %d: %s No %d(ch %d): Event ID: 0x%0X,\n"
          "          Event data: 0x%0X (%s)\n",
          (int) pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, (int) pFXO->nFxoCh,
          pEvent->id, pEvent->data.value,
          Common_Enum2Name(pEvent->id, TD_rgTapiEventName)));

   switch (pEvent->id)
   {
      case IFX_TAPI_EVENT_FXO_BAT_FEEDED:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d): "
                "Battery - line is feeded from FXO.\n",
                (int) pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nDevNum, (int) pFXO->nFxoCh));
         break;
      }

      case IFX_TAPI_EVENT_FXO_BAT_DROPPED:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d): "
                "Battery - line is dropped on FXO.\n",
                (int) pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nDevNum, (int) pFXO->nFxoCh));
         break;
      }

      case IFX_TAPI_EVENT_FXO_POLARITY:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d): "
                "Polarity changed.\n",
                (int) pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nDevNum, (int) pFXO->nFxoCh));
         break;
      }

      case IFX_TAPI_EVENT_FXO_RING_START:
      {
         /* Handle event */
         EVENT_FXO_Handle_RingStart(pFXO, pCtrl, pBoard);
         break;
      }
      case IFX_TAPI_EVENT_FXO_RING_STOP:
      {
         /* Handle event */
         EVENT_FXO_Handle_RingStop(pFXO, pCtrl, pBoard);
         break;
      }

      case IFX_TAPI_EVENT_FXO_OSI:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d): "
                "OSI signal (short drop of DC voltage, less than 300 ms), "
                "indicating the start of a CID transmission.\n",
                (int) pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nDevNum, (int) pFXO->nFxoCh));
         break;
      }

      case IFX_TAPI_EVENT_FXO_APOH:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d): "
                "APOH (another phone off-hook).\n",
                (int) pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nDevNum, (int) pFXO->nFxoCh));
         break;
      }

      case IFX_TAPI_EVENT_FXO_NOPOH:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d): "
                "NOPOH (no other phone off-hook).\n",
                (int) pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nDevNum, (int) pFXO->nFxoCh));
         break;
      }

      default:
         /* There are plenty more, but not handled at the moment */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
               ("FXO No %d: %s No %d (ch %d):  Unknown event detected 0x%0X (%s).\n",
                (int) pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
                pFXO->pFxoDevice->nDevNum, (int) pFXO->nFxoCh, pEvent->id,
                Common_Enum2Name(pEvent->id, TD_rgTapiEventName)));
         break;
   } /* switch */

   /* Cleanup event field */
   pEvent->id = IFX_TAPI_EVENT_NONE;

   return IFX_SUCCESS;

} /* EVENT_FXO_Handle() */
#endif /* #if (!defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) \
&& !defined(EASY336) && !defined(XT16)) */

#ifdef TAPI_VERSION4
/**
   Disable/Enable detection of table of events

   \param  pBoard - pointer to board
   \param  nDevFd - device FD on which enable/disable events
   \param  pEvents - pointer to table of events
   \param  nEnabled - if IFX_ENABLE then enable event detection, otherwise
                      disable event detection
   \param nSeqConnId  - Seq Conn ID

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_DetectionMultiple(BOARD_t *pBoard, IFX_int32_t nDevFd,
                                     IFX_int32_t *pEvents,
                                     IFX_enDis_t nEnabled,
                                     IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_EVENT_t oEvent;
   IFX_boolean_t nNoErr = IFX_TRUE;
   IFX_int32_t j=0;
   IFX_int32_t nRet;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pEvents, IFX_ERROR);

   /* for all events in table */
   while (IFX_TAPI_EVENT_NONE != pEvents[j])
   {
      /* prepare event strructure */
      memset(&oEvent, 0, sizeof(oEvent));
      /* set for all channel and devices */
      oEvent.dev = IFX_TAPI_EVENT_ALL_DEVICES;
      oEvent.ch = IFX_TAPI_EVENT_ALL_CHANNELS;
      /* for all modules */
      oEvent.module = IFX_TAPI_MODULE_TYPE_ALL;
      /* set event */
      oEvent.id = pEvents[j];

      /*#warning debug trace uncomment if needed
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("TD_DBG: EVENT %s, (j=%d, fd %d, ch %d, dev %d).\n",
             Common_Enum2Name(oEvent.id,TD_rgTapiEventName),
             j, nDevFd, oEvent.ch, oEvent.dev)); */
      /* check if enabling events */
      if (IFX_ENABLE == nEnabled)
      {
         nRet = TD_IOCTL(nDevFd, IFX_TAPI_EVENT_ENABLE, &oEvent,
                   oEvent.dev, nSeqConnId);
      }
      else if (IFX_DISABLE == nEnabled)
      {
         nRet = TD_IOCTL(nDevFd, IFX_TAPI_EVENT_DISABLE, &oEvent,
                   oEvent.dev, nSeqConnId);
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, Invalid value of nEnabled (%d). (File: %s, line: %d)\n",
                nEnabled, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      if (IFX_SUCCESS != nRet)
      {
         /* handle error */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, %s for %s failed. (File: %s, line: %d)\n",
                (IFX_ENABLE == nEnabled) ?
                "IFX_TAPI_EVENT_ENABLE" : "IFX_TAPI_EVENT_DISABLE",
                Common_Enum2Name(oEvent.id,TD_rgTapiEventName),
                __FILE__, __LINE__));
         nNoErr = IFX_FALSE;
      }
      /* go to next element */
      j++;
   } /* for all events in table */
   /* if any IOCTL returned error then return error */
   if (IFX_TRUE != nNoErr)
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Enable detection of calibration events

   \param  pBoard - pointer to board
   \param nSeqConnId  - Seq Conn ID

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_CalibrationEnable(BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* disable events */
   if (IFX_SUCCESS != EVENT_DetectionMultiple(pBoard, pBoard->nDevCtrl_FD,
                         aEventsCalibration, IFX_ENABLE, nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s EVENT_DetectionMultiple() for calibration failed. "
             "(File: %s, line: %d)\n",
             pBoard->pszBoardName, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Enable events to handle

   \param  pCtrl - pointer to control structure

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_DetectionInit(CTRL_STATUS_t* pCtrl)
{
   IFX_boolean_t nNoErr = IFX_TRUE;
   IFX_int32_t i = 0;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* for all boards */
   for (i=0; i<pCtrl->nBoardCnt; i++)
   {
      /* workaround for XT-16 extension board when it is not connected */
      if (TYPE_XT16 != pCtrl->rgoBoards[i].nType ||
          0 != pCtrl->rgoBoards[i].nChipCnt)
      {
         /* enable events */
         if (IFX_SUCCESS != EVENT_DetectionMultiple(&pCtrl->rgoBoards[i],
                               pCtrl->rgoBoards[i].nDevCtrl_FD,
                               aEvents, IFX_ENABLE, TD_CONN_ID_INIT))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, %s EVENT_DetectionMultiple() for init detection failed. "
                   "(File: %s, line: %d)\n",
                   pCtrl->rgoBoards[i].pszBoardName, __FILE__, __LINE__));
            nNoErr = IFX_FALSE;
         }
      }
   } /* for all boards */
   /* if failed for any board then return error */
   if (IFX_TRUE != nNoErr)
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Disable all enabled events

   \param  pCtrl - pointer to control structure

   \return IFX_ERROR or IFX_SUCCESS
*/
IFX_return_t EVENT_DetectionDeInit(CTRL_STATUS_t* pCtrl)
{
   IFX_boolean_t nNoErr = IFX_TRUE;
   IFX_int32_t i=0;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* for all boards */
   for (i=0; i<pCtrl->nBoardCnt; i++)
   {
      /* workaround for XT-16 extension board when it is not connected */
      if (TYPE_XT16 != pCtrl->rgoBoards[i].nType ||
          0 != pCtrl->rgoBoards[i].nChipCnt)
      {
         /* disable events */
         if (IFX_SUCCESS != EVENT_DetectionMultiple(&pCtrl->rgoBoards[i],
                               pCtrl->rgoBoards[i].nDevCtrl_FD,
                               aEvents, IFX_DISABLE, TD_CONN_ID_INIT))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, %s EVENT_DetectionMultiple() for DeInit detection failed. "
                   "(File: %s, line: %d)\n",
                   pCtrl->rgoBoards[i].pszBoardName, __FILE__, __LINE__));
            nNoErr = IFX_FALSE;
         }
         /* disable calibration events */
         if (IFX_SUCCESS != EVENT_DetectionMultiple(&pCtrl->rgoBoards[i],
                               pCtrl->rgoBoards[i].nDevCtrl_FD,
                               aEventsCalibration, IFX_DISABLE, TD_CONN_ID_INIT))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, %s disable calibration events detection failed. "
                   "(File: %s, line: %d)\n",
                   pCtrl->rgoBoards[i].pszBoardName, __FILE__, __LINE__));
            nNoErr = IFX_FALSE;
         }
      }
   } /* for all boards */
   /* if failed for any board then return error */
   if (IFX_TRUE != nNoErr)
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

#endif /* TAPI_VERSION4 */

