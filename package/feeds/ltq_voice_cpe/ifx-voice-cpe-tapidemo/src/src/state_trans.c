/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : state_trans.c
   Description :
*******************************************************************************/
/**
   \file state_trans.c
   \date
   \brief Handling events for different states.
*/
/* ============================= */
/* Includes                      */
/* ============================= */

#include "ifx_types.h"
#include "state_trans.h"
#include "abstract.h"
#include "analog.h"
#include "conference.h"
#ifdef FXO
   #include "common_fxo.h"
#endif /* FXO */
#include "cid.h"
#include "event_handling.h"
#include "feature.h"
#include "parse_cmd_arg.h"
#include "pcm.h"
#include "qos.h"
#include "voip.h"
#include "tapidemo.h"
#include "lib_tapi_signal.h"
#include "faxstruct.h"
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   #include "td_t38.h"
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

#ifdef EASY336
   #include "board_easy336.h"
   #include "lib_svip_rm.h"
   #include "lib_svip.h"
#endif

#ifdef TD_DECT
#include "td_dect.h"
#endif /* TD_DECT */

#ifdef EASY3111
#include "board_easy3111.h"
#endif /* EASY3111 */

/* ============================= */
/* Global variable definition     */
/* ============================= */

/** names of internal events */
TD_ENUM_2_NAME_t TD_rgIE_EventsName[] =
{
   {IE_NONE, "IE_NONE"},
   {IE_READY, "IE_READY"},
   {IE_HOOKOFF, "IE_HOOKOFF"},
   {IE_HOOKON, "IE_HOOKON"},
   {IE_FLASH_HOOK, "IE_FLASH_HOOK"},
   {IE_DIALING, "IE_DIALING"},
   {IE_RINGING, "IE_RINGING"},
   {IE_RINGBACK, "IE_RINGBACK"},
   {IE_ACTIVE, "IE_ACTIVE"},
   {IE_CONFERENCE, "IE_CONFERENCE"},
   {IE_END_CONNECTION, "IE_END_CONNECTION"},
   {IE_BUSY, "IE_BUSY"},
   {IE_CNG_FAX, "IE_CNG_FAX"},
   {IE_CNG_MOD, "IE_CNG_MOD"},
   {IE_AM, "IE_AM"},
   {IE_PR, "IE_PR"},
   {IE_CED, "IE_CED"},
   {IE_CED_END, "IE_CED_END"},
   {IE_DIS, "IE_DIS"},
   {IE_V8BIS, "IE_V8BIS"},
   {IE_V21L, "IE_V21L"},
   {IE_V18A, "IE_V18A"},
   {IE_V27, "IE_V27"},
   {IE_BELL, "IE_BELL"},
   {IE_V22, "IE_V22"},
   {IE_V22ORBELL, "IE_V22ORBELL"},
   {IE_V32AC, "IE_V32AC"},
   {IE_CAS_BELL, "IE_CAS_BELL"},
   {IE_V21H, "IE_V21H"},
   {IE_END_DATA_TRANSMISSION, "IE_END_DATA_TRANSMISSION"},
   {IE_CONFIG, "IE_CONFIG"},
   {IE_CONFIG_DIALING, "IE_CONFIG_DIALING"},
   {IE_CONFIG_HOOKON, "IE_CONFIG_HOOKON"},
   {IE_CONFIG_PEER, "IE_CONFIG_PEER"},
   {IE_EXT_CALL_SETUP, "IE_EXT_CALL_SETUP"},
   {IE_EXT_CALL_PROCEEDING, "IE_EXT_CALL_PROCEEDING"},
   {IE_EXT_CALL_ESTABLISHED, "IE_EXT_CALL_ESTABLISHED"},
   {IE_EXT_CALL_WRONG_NUM, "IE_EXT_CALL_WRONG_NUM"},
   {IE_EXT_CALL_NO_ANSWER, "IE_EXT_CALL_NO_ANSWER"},
   {IE_EXT_CALL_WORKAROUND_EASY508xx, "IE_EXT_CALL_WORKAROUND_EASY508xx"},
   {IE_T38_ERROR, "IE_T38_ERROR"},
   {IE_T38_REQ, "IE_T38_REQ"},
   {IE_T38_ACK, "IE_T38_ACK"},
   {IE_T38_END, "IE_T38_END"},
   {IE_DECT_REGISTER, "IE_DECT_REGISTER"},
   {IE_DECT_UNREGISTER, "IE_DECT_UNREGISTER"},
   {IE_TEST_TONE, "IE_TEST_TONE"},
   {IE_PHONEBOOK_UPDATE, "IE_PHONEBOOK_UPDATE"},
   {IE_PHONEBOOK_UPDATE_RESPONSE, "IE_PHONEBOOK_UPDATE_RESPONSE"},
   {IE_FAULT_LINE_GK_LOW, "IE_FAULT_LINE_GK_LOW"},
   {IE_KPI_SOCKET_FAILURE, "IE_KPI_SOCKET_FAILURE"},
   /** this event is never generated,
      it is only used to mark end of EVENT_ACTIONS_t table */
   {IE_END, "IE_END"},
   {TD_MAX_ENUM_ID, "INTERNAL_EVENTS_t"}
};

/** names of states */
TD_ENUM_2_NAME_t TD_rgStateName[] =
{
   {S_READY, "S_READY"},
   {S_OUT_DIALTONE, "S_OUT_DIALTONE"},
   {S_OUT_DIALING, "S_OUT_DIALING"},
   {S_OUT_FXO_CALL, "S_OUT_FXO_CALL"},
   {S_OUT_CALLING, "S_OUT_CALLING"},
   {S_OUT_RINGBACK, "S_OUT_RINGBACK"},
   {S_ACTIVE, "S_ACTIVE"},
   {S_BUSYTONE, "S_BUSYTONE"},
   {S_IN_RINGING, "S_IN_RINGING"},
   {S_CF_DIALTONE, "S_CF_DIALTONE"},
   {S_CF_DIALING, "S_CF_DIALING"},
   {S_CF_CALLING, "S_CF_CALLING"},
   {S_CF_RINGBACK, "S_CF_RINGBACK"},
   {S_CF_ACTIVE, "S_CF_ACTIVE"},
   {S_CF_BUSYTONE, "S_CF_BUSYTONE"},
   {S_TEST_TONE, "S_TEST_TONE"},
   {S_FAX_MODEM, "S_FAX_MODEM"},
   {S_FAXT38, "S_FAXT38"},
   {S_FAXT38_REQ, "S_FAXT38_REQ"},
   {S_FAXT38_END, "S_FAXT38_END"},
   {S_DECT_NOT_REGISTERED, "S_DECT_NOT_REGISTERED"},
   {S_LINE_DISABLED, "S_LINE_DISABLED"},
   {S_LINE_STANDBY, "S_LINE_STANDBY"},
   {S_OFF_HOOK_DETECTION, "S_OFF_HOOK_DETECTION"},
   {S_ON_HOOK_DETECTION, "S_ON_HOOK_DETECTION"},
   {S_PPD_IDLE, "S_PPD_IDLE"},
   {TD_MAX_ENUM_ID, "STATE_MACHINE_STATES_t"}
};

/** names of Call types */
TD_ENUM_2_NAME_t TD_rgCallTypeName[] =
{
   {UNKNOWN_CALL_TYPE, "Unknown Call"},
   {EXTERN_VOIP_CALL, "External Call"},
   {LOCAL_CALL, "Local Call"},
   {PCM_CALL, "PCM Call"},
   {LOCAL_BOARD_CALL, "Local Board Call"},
   {FXO_CALL, "FXO Call"},
   {LOCAL_PCM_CALL, "Local PCM Call"},
   {LOCAL_BOARD_PCM_CALL, "Local Board PCM Call"},
   {DTMF_COMMAND, "DTMF Command"},
   {NOT_SUPPORTED, "Call not supported"},
   {TD_MAX_ENUM_ID, "CALL_TYPE_t"}
};

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Table of handled events with their handling functions.
    FXS state machine - S_DECT_NOT_REGISTERED state. */
static EVENT_ACTIONS_t ST_rgFXS_DectNotRegisteredState[] =
{
   { IE_DECT_REGISTER, ST_FXS_DectNotRegistered_Register },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   /* this event is never generated,
      it is only used to mark end of EVENT_ACTIONS_t table */
   { IE_END, 0}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_READY state. */
static EVENT_ACTIONS_t ST_rgFXS_ReadyState[] =
{
   { IE_HOOKOFF, ST_FXS_Ready_HookOff },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_Ready_FaultLineGkLow },
   { IE_RINGING, ST_FXS_Ready_Ringing },
   { IE_TEST_TONE, ST_FXS_Ready_TestTone },
   /* this event is never generated,
      it is only used to mark end of EVENT_ACTIONS_t table */
   { IE_END, 0}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_OUT_DIALTONE state. */
static EVENT_ACTIONS_t ST_rgFXS_OutDialtoneState[] =
{
   { IE_HOOKON, ST_FXS_OutDialtone_HookOn },
   { IE_DIALING, ST_FXS_OutDialtone_Dialing },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_OUT_DIALING state. */
static EVENT_ACTIONS_t ST_rgFXS_OutDialingState[] =
{
   { IE_HOOKON, ST_FXS_OutDialing_HookOn },
   { IE_DIALING, ST_FXS_OutDialing_Dialing },
   { IE_EXT_CALL_ESTABLISHED, ST_FXS_OutDialing_ExtCallEstablished },
   { IE_EXT_CALL_WRONG_NUM, ST_FXS_OutDialing_ExtCallWrongNumber },
   { IE_EXT_CALL_NO_ANSWER, ST_FXS_OutDialing_ExtCallNoAnswer },
   { IE_EXT_CALL_PROCEEDING, ST_FXS_OutDialing_ExtCallProceeding },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_BUSYTONE state. */
static EVENT_ACTIONS_t ST_rgFXS_BusytoneState[] =
{
   { IE_HOOKON, ST_FXS_Busytone_HookOn },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_OUT_CALLING state. */
static EVENT_ACTIONS_t ST_rgFXS_OutCallingState[] =
{
   { IE_HOOKON, ST_FXS_OutCalling_HookOn },
   { IE_RINGBACK, ST_FXS_OutCalling_Ringback },
   { IE_BUSY, ST_FXS_OutCalling_Busy },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_OUT_RINGBACK state. */
static EVENT_ACTIONS_t ST_rgFXS_OutRingbackState[] =
{
   { IE_HOOKON, ST_FXS_OutRingback_HookOn },
   { IE_ACTIVE, ST_FXS_OutRingback_Active },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_ACTIVE state. */
static EVENT_ACTIONS_t ST_rgFXS_ActiveState[] =
{
   { IE_HOOKON, ST_FXS_Active_HookOn },
   { IE_DIALING,  ST_FXS_Active_Dialing},
   { IE_CONFERENCE, ST_FXS_Active_Conference },
   { IE_END_CONNECTION, ST_FXS_Active_EndConnection },
   { IE_CNG_FAX, ST_FXS_Active_FaxModXxx },
   { IE_CNG_MOD, ST_FXS_Active_FaxModXxx },
   { IE_CED, ST_FXS_Active_FaxModXxx },
   { IE_AM, ST_FXS_Active_FaxModXxx },
   { IE_PR, ST_FXS_Active_FaxModXxx },
   { IE_V8BIS, ST_FXS_Active_FaxModXxx },
   { IE_V21L, ST_FXS_Active_FaxModXxx },
   { IE_V18A, ST_FXS_Active_FaxModXxx },
   { IE_V27, ST_FXS_Active_FaxModXxx },
   { IE_BELL, ST_FXS_Active_FaxModXxx },
   { IE_V22, ST_FXS_Active_FaxModXxx },
   { IE_V22ORBELL, ST_FXS_Active_FaxModXxx },
   { IE_V32AC, ST_FXS_Active_FaxModXxx },
   { IE_CAS_BELL, ST_FXS_Active_FaxModXxx },
   { IE_V21H, ST_FXS_Active_FaxModXxx },
   { IE_DIS, ST_FXS_Active_DIS},
   { IE_T38_REQ, ST_FXS_Active_T38Req },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_KPI_SOCKET_FAILURE, ST_FXS_xxx_Active_KpiSockFail },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
   FXS state machine - S_IN_RINGING state. */
static EVENT_ACTIONS_t ST_rgFXS_InRingingState[] =
{
   { IE_READY, ST_FXS_InRinging_Ready },
   { IE_HOOKON, ST_FXS_InRinging_Ready },
   { IE_HOOKOFF, ST_FXS_InRinging_HookOff },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_END, IFX_NULL}
};

/* Conference start */
/** Table of handled events with their handling functions.
    FXS state machine - S_CF_DIALTONE state. */
static EVENT_ACTIONS_t ST_rgFXS_CfDialtoneState[] =
{
   { IE_HOOKON, ST_FXS_CfDialtone_HookOn },
   { IE_FLASH_HOOK, ST_FXS_CfDialtone_FlashHook },
   { IE_DIALING, ST_FXS_CfDialtone_Dialing },
   { IE_END_CONNECTION, ST_FXS_CfDialtone_EndConnection },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_CF_DIALING state. */
static EVENT_ACTIONS_t ST_rgFXS_CfDialingState[] =
{
   { IE_HOOKON, ST_FXS_CfDialing_HookOn },
   { IE_FLASH_HOOK, ST_FXS_CfDialing_FlashHook },
   { IE_DIALING, ST_FXS_CfDialing_Dialing },
   { IE_END_CONNECTION, ST_FXS_CfDialing_EndConnection },
   { IE_EXT_CALL_ESTABLISHED, ST_FXS_CfDialing_ExtCallEstablished },
   { IE_EXT_CALL_WRONG_NUM, ST_FXS_CfDialing_ExtCallWrongNumber },
   { IE_EXT_CALL_NO_ANSWER, ST_FXS_CfDialing_ExtCallNoAnswer },
   { IE_EXT_CALL_PROCEEDING, ST_FXS_CfDialing_ExtCallProceeding },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_CF_CALLING state. */
static EVENT_ACTIONS_t ST_rgFXS_CfCallingState[] =
{
   { IE_HOOKON, ST_FXS_CfCalling_HookOn },
   { IE_FLASH_HOOK, ST_FXS_CfCalling_FlashHook },
   { IE_RINGBACK, ST_FXS_CfCalling_Ringback },
   { IE_END_CONNECTION, ST_FXS_CfCalling_EndConnection },
   { IE_BUSY , ST_FXS_CfCalling_Busy },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_CF_BUSYTONE state. */
static EVENT_ACTIONS_t ST_rgFXS_CfBusytoneState[] =
{
   { IE_HOOKON, ST_FXS_CfBusytone_HookOn },
   { IE_FLASH_HOOK, ST_FXS_CfBusytone_FlashHook },
   { IE_END_CONNECTION, ST_FXS_CfBusytone_EndConnection },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_CF_RINGBACK state. */
static EVENT_ACTIONS_t ST_rgFXS_CfRingbackState[] =
{
   { IE_HOOKON, ST_FXS_CfRingback_HookOn },
   { IE_FLASH_HOOK, ST_FXS_CfRingback_FlashHook },
   { IE_ACTIVE, ST_FXS_CfRingback_Active },
   { IE_END_CONNECTION, ST_FXS_CfRingback_EndConnection },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_CF_ACTIVE state. */
static EVENT_ACTIONS_t ST_rgFXS_CfActiveState[] =
{
   { IE_HOOKON, ST_FXS_CfActive_HookOn },
   { IE_DIALING,  ST_FXS_Active_Dialing},
   { IE_CONFERENCE, ST_FXS_CfActive_Conference },
   { IE_END_CONNECTION, ST_FXS_CfActive_EndConnection },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_KPI_SOCKET_FAILURE, ST_FXS_xxx_Active_KpiSockFail },
   { IE_END, IFX_NULL}
};
/* Conference end */

/** Table of handled events with their handling functions.
    FXS state machine - S_TESTONE state. */
static EVENT_ACTIONS_t ST_rgFXS_TestToneState[] =
{
   { IE_HOOKOFF, ST_FXS_TestTone_HookOff },
   { IE_HOOKON, ST_FXS_TestTone_HookOn },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_FAX_MODEM state. */
static EVENT_ACTIONS_t ST_rgFXS_FaxModemState[] =
{
   { IE_HOOKON, ST_FXS_FaxModem_HookOn },
   { IE_END_CONNECTION, ST_FXS_FaxModem_EndConnection },
   { IE_DIS, ST_FXS_FaxModem_DIS },
   { IE_END_DATA_TRANSMISSION, ST_FXS_FaxModem_EndDataTrans },
   { IE_T38_REQ, ST_FXS_FaxModem_T38Req },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_FAXT38 state. */
static EVENT_ACTIONS_t ST_rgFXS_FaxT38State[] =
{
   { IE_HOOKON, ST_FXS_FaxT38_HookOn },
   { IE_END_CONNECTION, ST_FXS_FaxT38_EndConnection },
   { IE_END_DATA_TRANSMISSION, ST_FXS_FaxT38_EndDataTrans },
   { IE_T38_ERROR, ST_FXS_FaxT38_EndDataTrans },
   { IE_T38_END, ST_FXS_FaxT38_T38End },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_FAXT38_REQ state. */
static EVENT_ACTIONS_t ST_rgFXS_FaxT38ReqState[] =
{
   { IE_HOOKON, ST_FXS_FaxT38Req_HookOn },
   { IE_END_CONNECTION, ST_FXS_FaxT38Req_EndConnection },
   { IE_END_DATA_TRANSMISSION, ST_FXS_FaxT38Req_EndDataTrans },
   { IE_T38_ERROR, ST_FXS_FaxT38Req_EndDataTrans },
   { IE_T38_REQ, ST_FXS_FaxT38Req_T38xxx },
   { IE_T38_ACK, ST_FXS_FaxT38Req_T38xxx },
   { IE_T38_END, ST_FXS_FaxT38Req_T38End },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    FXS state machine - S_FAXT38_END state. */
static EVENT_ACTIONS_t ST_rgFXS_FaxT38EndState[] =
{
   { IE_HOOKON, ST_FXS_FaxT38End_HookOn },
   { IE_END_CONNECTION, ST_FXS_FaxT38End_EndConnection },
   { IE_T38_END, ST_FXS_FaxT38End_T38End },
   { IE_RINGING, ST_FXS_xxx_Ringing },
   { IE_FAULT_LINE_GK_LOW, ST_FXS_xxx_FaultLineGkLow },
   { IE_END, IFX_NULL}
};

/** Table of avaible states with their EVENT_ACTIONS_t tables
    for FXS state machine. */
STATE_COMBINATION_t ST_rgFXSStates[] =
{
   { S_DECT_NOT_REGISTERED, ST_rgFXS_DectNotRegisteredState },
   { S_READY, ST_rgFXS_ReadyState },
   { S_OUT_DIALTONE, ST_rgFXS_OutDialtoneState },
   { S_OUT_DIALING, ST_rgFXS_OutDialingState },
   { S_OUT_CALLING, ST_rgFXS_OutCallingState },
   { S_OUT_RINGBACK, ST_rgFXS_OutRingbackState },
   { S_ACTIVE, ST_rgFXS_ActiveState },
   { S_BUSYTONE, ST_rgFXS_BusytoneState },
   { S_IN_RINGING, ST_rgFXS_InRingingState },
   { S_CF_DIALTONE, ST_rgFXS_CfDialtoneState },
   { S_CF_DIALING, ST_rgFXS_CfDialingState },
   { S_CF_CALLING, ST_rgFXS_CfCallingState },
   { S_CF_RINGBACK, ST_rgFXS_CfRingbackState },
   { S_CF_ACTIVE, ST_rgFXS_CfActiveState },
   { S_CF_BUSYTONE, ST_rgFXS_CfBusytoneState },
   { S_TEST_TONE, ST_rgFXS_TestToneState },
   { S_FAX_MODEM, ST_rgFXS_FaxModemState },
   { S_FAXT38, ST_rgFXS_FaxT38State },
   { S_FAXT38_REQ, ST_rgFXS_FaxT38ReqState },
   { S_FAXT38_END, ST_rgFXS_FaxT38EndState },
   { S_END, IFX_NULL }
};

/** Table of handled events with their handling functions.
    configure state machine - for S_READY state. */
static EVENT_ACTIONS_t ST_rgConfig_ReadyState[] =
{
   { IE_CONFIG_PEER, ST_Config_Ready_Peer },
   { IE_CONFIG, ST_Config_Ready_Config },
   { IE_END, IFX_NULL}
};

/** Table of handled events with their handling functions.
    configure state machine - S_ACTIVE state. */
static EVENT_ACTIONS_t ST_rgConfig_ActiveState[] =
{
   { IE_CONFIG_DIALING, ST_Config_Active_Dialing },
   { IE_CONFIG_HOOKON, ST_Config_Active_HookOn },
   { IE_END, IFX_NULL}
};

/** Table of avaible states with their EVENT_ACTIONS_t tables
    for configure state machine. */
STATE_COMBINATION_t ST_rgConfigStates[] =
{
   { S_READY, ST_rgConfig_ReadyState },
   { S_ACTIVE, ST_rgConfig_ActiveState },
   { S_END, IFX_NULL }
};

/** File descritor of data channel used in connection.
    Is set in ST_HandleState_FXS and ST_HandleState_Config
    and used in functions below. */
static IFX_int32_t fd_data_ch = -1;

/**
   Action when detected IE_RINGING event for all states except S_READY.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_xxx_Ringing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                CONNECTION_t* pConn)
{
   /* Phone is busy, so inform about it caller */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Phone No %d: State %s: Cannot start new call.\n",
          (IFX_int32_t) pPhone->nPhoneNumber,
          Common_Enum2Name(pPhone->rgStateMachine[FXS_SM].nState, TD_rgStateName)));

   /* send IE_BUSY event to peer that tried to call this phone */
   TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_BUSY, pPhone->nSeqConnId);
   pPhone->nIntEvent_FXS = IE_NONE;

   return IFX_SUCCESS;
} /* ST_FXS_xxx_Ringing() */

/**
   Action when detected IE_KPI_SOCKET_FAILURE event for active states.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_xxx_Active_KpiSockFail(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                           CONNECTION_t* pConn)
{
   /* Inform for which socket and for which det address problem occured */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
      ("Phone No %d: Error during socket operation for socket %d,\n"
       "%s dest addr %s:%d.\n",
       (IFX_int32_t) pPhone->nPhoneNumber, pConn->nUsedSocket,
       pPhone->pBoard->pIndentPhone,
       TD_GetStringIP(&pConn->oConnPeer.oRemote.oToAddr),
       TD_SOCK_PortGet(&pConn->oConnPeer.oRemote.oToAddr)));

   Common_ToneStop(pCtrl,pPhone);
   Common_TonePlay(pCtrl,pPhone, TD_ITN_ERROR);

   pPhone->nIntEvent_FXS = IE_NONE;

   return IFX_SUCCESS;
} /* */

/**
   Plays busy tone and releases resources.
   Used during S_OUT_DIALING and S_CF_DIALING states.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Dialing_Busy(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                             CONNECTION_t* pConn)
{

   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

#ifdef TD_DECT
   /* activate DECT so busy tone can be heard in handset */
   if (PHONE_TYPE_DECT == pPhone->nType &&
       IFX_TRUE != pPhone->bDectChActivated)
   {
      /* activate connection with DECT handset so busy tone can be heard */
      if (IFX_SUCCESS != TD_DECT_PhoneActivate(pPhone, pConn))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, phone No %d:TD_DECT_PhoneActivate.(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
      }
   } /* if (PHONE_TYPE_DECT == pPhone->nType) */
#endif /* TD_DECT */

   /* play busy tone */
   TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, BUSY_TONE_IDX, "Busy tone");
#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
      /* don't release reseources when in conference */
      if (NO_CONFERENCE == pPhone->nConfIdx)
      {
         Common_DTMF_Disable(pCtrl, pPhone);
#ifdef EASY336
         /* release signal detector */
         resources.nType = RES_DET;
         Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
      } /* if (NO_CONFERENCE != pPhone->nConfIdx) */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

#ifdef XT16
   pConn = IFX_NULL;
#endif
   /* don't use here TAPIDEMO_ClearPhone() when in conference */
   if (NO_CONFERENCE == pPhone->nConfIdx)
   {
      TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_BUSY, pPhone->pBoard);
   } /* if (pPhone->pBoard->fSingleFD) */

   return ret;
} /* ST_Dialing_Busy() */

/**
   Action when detected IE_HOOKON on event during S_ACTIVE, S_FAX_MODEM
   and T.38 states. Releases resources and sends notification to peer.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Active_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                              CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   if (UNKNOWN_CALL_TYPE != pConn->nType)
   {
      TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_END_CONNECTION,
                         pPhone->nSeqConnId);
   }
   /* check call direction */
   if (pPhone->nCallDirection == CALL_DIRECTION_TX)
   {
      /* use first connection */
      pConn = &pPhone->rgoConn[0];
#ifdef TAPI_VERSION4
      /* stop signal generator */
      Common_ToneStop(pCtrl, pPhone);
#endif /* TAPI_VERSION4 */

      TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);
#ifdef TAPI_VERSION4
      /* stop DTMF detection */
      Common_DTMF_Disable(pCtrl, pPhone);
#ifdef EASY336
      if (pConn->nType == LOCAL_CALL)
      {
         if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
         {
            resources.nType = RES_DET | RES_GEN | RES_CONN_ID | RES_LEC;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            resources.nVoipID_Nr = SVIP_RM_UNUSED;
         } /* if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal) */
         else
         {
            resources.nType = RES_ALL;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            resources.nVoipID_Nr = pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
         } /* if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal) */
      } /* if (pConn->nType == LOCAL_CALL) */
      else
      {
         resources.nType = RES_ALL;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         resources.nVoipID_Nr = SVIP_RM_UNUSED;
      } /* if (pConn->nType == LOCAL_CALL) */
      /* stop LEC */
      Common_LEC_Disable(pCtrl, pPhone);
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
   }
   else if (pPhone->nCallDirection == CALL_DIRECTION_RX)
   {
      /* if not TAPI_VERSION4 then everything is done in TAPIDEMO_ClearPhone()*/
      TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);
#ifdef TAPI_VERSION4
      if (pPhone->pBoard->fSingleFD)
      {
#ifdef EASY336
         Common_LEC_Disable(pCtrl, pPhone);
         if (pConn->nType == LOCAL_CALL)
         {
            if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
            {
               resources.nType = RES_CONN_ID | RES_LEC;
               resources.nConnID_Nr = SVIP_RM_UNUSED;
               resources.nVoipID_Nr = SVIP_RM_UNUSED;
            } /* if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal) */
            else
            {
               resources.nType = RES_ALL;
               resources.nConnID_Nr = SVIP_RM_UNUSED;
               resources.nVoipID_Nr =
                  pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
            } /* if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal) */
         } /* if (pConn->nType == LOCAL_CALL) */
         else
         {
            resources.nType = RES_ALL;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            resources.nVoipID_Nr = SVIP_RM_UNUSED;
         } /* if (pConn->nType == LOCAL_CALL) */
         Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
      } /* if (pPhone->pBoard->fSingleFD) */
#endif
   } /* if (pPhone->nCallDirection == CALL_DIRECTION_TX) */
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Unknown phone's direction (File: %s, line: %d)\n",
             __FILE__, __LINE__));
   } /* if (pPhone->nCallDirection == CALL_DIRECTION_TX) */

   /* connection has ended - reset connection type */
   pConn->nType = UNKNOWN_CALL_TYPE;

   return ret;
} /* ST_Active_HookOn() */

/**
   Action when detected IE_END_CONNECTION event during S_ACTIVE,
   S_FAX_MODEM and T.38 states. Releases resources and plays busy tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Active_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* check call direction */
   if (pPhone->nCallDirection == CALL_DIRECTION_TX)
   {
      /* use first connection */
#ifdef TAPI_VERSION4
      if (pPhone->pBoard->fSingleFD)
      {
#ifdef EASY336
         Common_DTMF_Disable(pCtrl, pPhone);
         Common_LEC_Disable(pCtrl, pPhone);
         resources.nType = RES_DET | RES_CONN_ID | RES_LEC;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
      } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */
   }
   else if (pPhone->nCallDirection == CALL_DIRECTION_RX)
   {
#ifdef TAPI_VERSION4
      if (pPhone->pBoard->fSingleFD)
      {
#ifdef EASY336
         if ((pConn->nType == LOCAL_CALL &&
              pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal) ||
             pConn->nType == EXTERN_VOIP_CALL)
         {
            Common_DTMF_Disable(pCtrl, pPhone);
         }
         Common_LEC_Disable(pCtrl, pPhone);
         resources.nType = RES_CONN_ID | RES_LEC;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         if ((pConn->nType == LOCAL_CALL &&
              pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal) ||
             pConn->nType == EXTERN_VOIP_CALL)
         {
            resources.nType |= RES_DET;
         }
         Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
      } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Unknown phone's call direction."
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }

#ifdef XT16
   Common_DTMF_Disable(pCtrl, pPhone);
#endif /* XT16 */

   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_BUSY, pPhone->pBoard);

   /* play busy tone */
   TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, BUSY_TONE_IDX, "Busy tone");

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      if (pConn->nType == LOCAL_CALL || pConn->nType == UNKNOWN_CALL_TYPE)
      {
         if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
         {
            resources.nType = RES_NONE;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
         }
         else
         {
            resources.nType = RES_CODEC | RES_VOIP_ID;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            resources.nVoipID_Nr =
               pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
            Common_ResourcesRelease(pPhone, &resources);
         }
      }
      else
      {
         resources.nType = RES_CODEC | RES_VOIP_ID;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         resources.nVoipID_Nr = SVIP_RM_UNUSED;
         Common_ResourcesRelease(pPhone, &resources);
      }
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */
   /* connection has ended - reset connection type */
   pConn->nType = UNKNOWN_CALL_TYPE;

   return ret;
} /* ST_Active_EndConnection() */

/**
   Function to handle flash hook during coference states.
   Removes peer, removes peer, changes state to S_CF_ACTIVE,
   if there are still peers left in conference,
   if not changes state to S_CF_BUSYTONE and plays busytone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Conference_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{

   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i;
   CONNECTION_t* pConnTmp;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* if waiting for called phone response (IE_RINGBACK or IE_ACTIVE)
      send message that we are ending this connection
      and remove this connection from conference */
   if ((S_CF_CALLING == pPhone->rgStateMachine[FXS_SM].nState) ||
       (S_CF_RINGBACK == pPhone->rgStateMachine[FXS_SM].nState))
   {
      /* for all conections */
      for (i = 0; i < pPhone->nConnCnt; i++)
      {
         /* if connection isn't active */
         if (IFX_FALSE == pPhone->rgoConn[i].fActive)
         {
            pConnTmp = &pPhone->rgoConn[i];
            /* notify called phone that this connection has ended */
            TAPIDEMO_SetAction(pCtrl, pPhone, pConnTmp, IE_READY,
                               pPhone->nSeqConnId);
            /* remove peer */
            CONFERENCE_RemovePeer(pCtrl, pPhone, &pConnTmp,
                                  &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);
         }
      } /* for (i = 0; i < pPhone->nConnCnt; i++) */
   }
   else if (S_CF_DIALING == pPhone->rgStateMachine[FXS_SM].nState)
   {
      /* for all conections */
      for (i = 0; i < pPhone->nConnCnt; i++)
      {
         /* if dialing external connection */
         if (IFX_FALSE == pPhone->rgoConn[i].fActive &&
             EXTERN_VOIP_CALL == pPhone->rgoConn[i].nType &&
             IFX_TRUE == pPhone->rgoConn[i].bExtDialingIn)
         {
#ifdef TAPI_VERSION4
            if (pPhone->pBoard->fSingleFD)
            {
#ifdef EASY336
               /* return codec */
               resources.nType = RES_CODEC | RES_VOIP_ID;
               resources.nConnID_Nr = SVIP_RM_UNUSED;
               resources.nVoipID_Nr = pPhone->rgoConn[i].voipID;
               Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
            }
#endif /* TAPI_VERSION4 */
            pPhone->nConnCnt--;
            pPhone->rgoConn[i].bExtDialingIn = IFX_FALSE;
            TD_TIMEOUT_STOP(pCtrl->nUseTimeoutSocket);
         }
      } /* for (i = 0; i < pPhone->nConnCnt; i++) */
   }

   /* check if there are still other connections in conference */
   if (0 < pPhone->nConnCnt)
   {
      /* get back to conference - change state to S_CF_ACTIVE */
      pPhone->rgStateMachine[FXS_SM].nState = S_CF_ACTIVE;
      return IFX_SUCCESS;
   }
   /* if no more peers in conference, then end conference */
   else
   {
      /* no more peers in conference - change state to S_CF_BUSYTONE */
      pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;

      /* use first connection structure */
      pConn = &pPhone->rgoConn[0];
      TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_BUSY, pPhone->pBoard);

      /* play busy tone */
      TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, BUSY_TONE_IDX, "Busy tone");
   } /* if (0 < pPhone->nConnCnt) */

   return ret;
} /* ST_Dialing_Busy() */

/**
   Function to handle external call dialing for S_OUT_DIALING/S_CF_DIALING state.

   Doesn't change phone state. New digit is dialed and new number will be send
   to peer. Only for external call.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Dialing_ExtCallDialing(CTRL_STATUS_t* pCtrl,
                                       PHONE_t* pPhone,
                                       CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t nDialedNum;

   /* Temporary buffor used to collected phone number */
   IFX_char_t buf[4];

   pPhone->nIntEvent_FXS = IE_NONE;

   /* During dialing external phone number */
   nDialedNum = Common_DigitsToNum(pPhone->pDialedNum, 0, pPhone->nDialNrCnt,
                                   pPhone->nSeqConnId);

   /* Dialing external phone number in progress */
   pPhone->nDialNrCnt = 0;

   /* Add dialed digit to collected phone number */
   snprintf(buf, 2,"%d", nDialedNum);

   if ((sizeof(pConn->oConnPeer.oRemote.rgCollectNumber) / sizeof(IFX_char_t)) >
       (strlen(pConn->oConnPeer.oRemote.rgCollectNumber) + strlen(buf)))
   {
      strcat(pConn->oConnPeer.oRemote.rgCollectNumber, buf);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Dialed number exceeds maximum length.\n",
             pPhone->nPhoneNumber));
      return IFX_ERROR;
   }

   /* get number from string */
   pConn->oConnPeer.nPhoneNum = atoi(pConn->oConnPeer.oRemote.rgCollectNumber);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
         ("Phone No %d: External called number %d - %s\n",
          pPhone->nPhoneNumber, pConn->oConnPeer.nPhoneNum,
          Common_Enum2Name(pConn->nType, TD_rgCallTypeName)));

#ifdef TD_USE_EXT_CALL_WORKAROUND_EASY508XX
   /* It is a workaround, to avoid problem with first packet being dropped
      for first external/PCM connection to EASY508xx boards */
   if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, pConn,
                       IE_EXT_CALL_WORKAROUND_EASY508xx, pPhone->nSeqConnId))
   {
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }
#endif /* TD_USE_EXT_CALL_WORKAROUND_EASY508XX */

   /* Send updated phone number to called phone */
   if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_EXT_CALL_SETUP,
                                       pPhone->nSeqConnId))
   {
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }

   /* Update time of sending message, needed by timeout */
   pConn->timeStartExtCalling = TD_OS_ElapsedTimeMSecGet(0);

   return ret;
} /* ST_Dialing_ExtCallDialing() */

/**
   Initialize local call, set phone and connection structure
   and send message to called phone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  nCalledNumber   - number of called phone

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Dialing_Dialing_Loc(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn,
                                    IFX_int32_t nCalledNumber)
{
   PHONE_t* pDstPhone;
   CONNECTION_t* pNewConn;
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
   IFX_boolean_t fWideBand = IFX_FALSE;

   /* Check for band type (WB/NB) */
   if (TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
   }

   pPhone->nDialNrCnt = 0;

   /* get called phone */
   pDstPhone = ABSTRACT_GetPHONE_OfCalledNum(pCtrl, nCalledNumber,
                                             pPhone->nSeqConnId);

   if (pDstPhone == IFX_NULL)
   {
      /* Could not found board, phone for this calling number */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Wrong phone number.\n",
             pPhone->nPhoneNumber));
      return IFX_ERROR;
   } /* if (pDstPhone == IFX_NULL) */

   if (pDstPhone == pPhone)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Calling his own phone number.\n",
             pPhone->nPhoneNumber));
      return IFX_ERROR;
   } /* if (pDstPhone == pPhone) */

#ifdef EASY336
   /* get codec */
   if (pPhone->nConnCnt == 0 &&
       pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
   {
      /* get codec and voip ID */
      resources.nType = RES_CODEC | RES_VOIP_ID;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      if (Common_ResourcesGet(pPhone, &resources) != IFX_SUCCESS)
         return IFX_ERROR;
   }
#endif /* EASY336 */


   pPhone->nCallDirection = CALL_DIRECTION_TX;
   /* Use new empty connection */
   pNewConn = &pPhone->rgoConn[pPhone->nConnCnt];
   pPhone->nConnCnt++;
#ifdef EASY336
   pNewConn->voipID = SVIP_RM_UNUSED;
#endif /* EASY336 */
   /* Timeslots are not set */
   pNewConn->oPCM.nTimeslot_RX = TIMESLOT_NOT_SET;
   pNewConn->oPCM.nTimeslot_TX = TIMESLOT_NOT_SET;

   /* set pointer to called phone */
   pNewConn->oConnPeer.oLocal.pPhone = pDstPhone;
   /* we are in some sort master, the called phone will be slave */
   pNewConn->oConnPeer.oLocal.fSlave = IFX_FALSE;
#ifdef TD_DECT
   /* for now set narrowband */
   pNewConn->oConnPeer.oLocal.fDectWideband = IFX_FALSE;
#endif /* TD_DECT */
   /* Save called stuff */
   pNewConn->oConnPeer.nPhoneNum = pDstPhone->nPhoneNumber;
   /* set local call type */
   if (IFX_SUCCESS != Common_SetLocalCallType(pPhone, pDstPhone, pNewConn))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Failed to get local call type. "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      /* On error this connection was not used */
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Making %s to number %d\n",
          pPhone->nPhoneNumber,
          Common_Enum2Name(pNewConn->nType, TD_rgCallTypeName),
          (IFX_int32_t) nCalledNumber));

   /* Save caller stuff */
   if ((pNewConn->nType == LOCAL_PCM_CALL) ||
       (pNewConn->nType == LOCAL_BOARD_PCM_CALL))
   {
      /* Save caller channel */
      pNewConn->nUsedCh = pPhone->nPCM_Ch;
      pNewConn->nUsedCh_FD = PCM_GetFD_OfCh(pPhone->nPCM_Ch, pPhone->pBoard);

      /* First time making connection with other pcm phone, but we could be
         connected with local phone(s) or external phone(s) */
      pPhone->fPCM_PeerCalled = IFX_TRUE;

      /* Map phone channel to pcm channel */
#if (!defined EASY336 && !defined XT16 && !defined EASY3201 && !defined EASY3201_EVS)
      if (IFX_SUCCESS != PCM_MapToPhone(pPhone->nPCM_Ch, pPhone->nPhoneCh,
                            IFX_TRUE, pPhone->pBoard, pPhone->nSeqConnId))
      {
         /* On error this connection was not used */
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
#endif /* (!defined EASY336 && !defined XT16 && !defined EASY3201) */
#ifndef EASY336
      /* Set timeslots for caller, this is master for local call */
      pNewConn->oPCM.nTimeslot_RX = PCM_GetTimeslots(TS_RX, fWideBand,
                                                     pPhone->nSeqConnId);
      pNewConn->oPCM.nTimeslot_TX = PCM_GetTimeslots(TS_TX, fWideBand,
                                                     pPhone->nSeqConnId);
#endif /* EASY336 */
   } /* if ((pNewConn->nType == LOCAL_PCM_CALL) ..*/
   else if (LOCAL_CALL == pConn->nType)
   {
#ifdef EASY3111
      /* special handling for EASY 3111 as extension board for vmmc */
      if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
      {
         if(IFX_SUCCESS != Easy3111_CallResourcesGet(pPhone, pCtrl, pNewConn))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: Failed to EASY 3111 resources. "
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
            pPhone->nConnCnt--;
            return IFX_ERROR;
         }
      }
      else
#endif /* EASY3111 */
      {
         pNewConn->nUsedCh_FD = pPhone->nPhoneCh_FD;
         pNewConn->nUsedCh = pPhone->nPhoneCh;
      }
   }
   else if (LOCAL_BOARD_CALL == pConn->nType)
   {
      pNewConn->nUsedCh_FD = pPhone->nDataCh_FD;
      pNewConn->nUsedCh = pPhone->nDataCh;
   } /* if ((pNewConn->nType == LOCAL_PCM_CALL) ... */

   ABSTRACT_SeqConnID_Set(pDstPhone, IFX_NULL, pPhone->nSeqConnId);

   /* send message to called phone */
   if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, pNewConn, IE_RINGING,
                                       pPhone->nSeqConnId))
   {
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }

   if (pCtrl->pProgramArg->oArgFlags.nCID)
   {
      IFX_int32_t nUsedFd;
#ifdef TD_DECT
      /* no CID for DECT phones */
      if (PHONE_TYPE_DECT != pDstPhone->nType)
#endif /* TD_DECT */
      {
#ifndef EASY336
         if (0 < pDstPhone->pBoard->nMaxCoderCh)
         {
            nUsedFd = VOIP_GetFD_OfCh(pDstPhone->nDataCh, pDstPhone->pBoard);
         }
         else
#endif /*  EASY336 */
         if (0 < pDstPhone->pBoard->nMaxAnalogCh)
         {
            nUsedFd = ALM_GetFD_OfCh(pDstPhone->pBoard, pDstPhone->nPhoneCh);
         }
         else
         {
            /* continue call without CID */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Phone No %d: Failed to get FD of phone No %d.\n",
                   pPhone->nPhoneNumber, pDstPhone->nPhoneNumber));
            return IFX_SUCCESS;
         }
         /* check connection type */
         if ((LOCAL_CALL == pNewConn->nType) ||
             (LOCAL_PCM_CALL == pNewConn->nType) ||
             (LOCAL_BOARD_PCM_CALL == pNewConn->nType))
         {
            /* check if phone in on-hook state */
            if (S_READY == pDstPhone->rgStateMachine[FXS_SM].nState)
            {
               /* CID can be only send locally and on same board. */
               /* Send ONHOOK CID message to called phone */
               if (IFX_ERROR == CID_Send (nUsedFd, pDstPhone,
                                   IFX_TAPI_CID_HM_ONHOOK, pPhone->pCID_Msg,
                                          pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Phone No %d: Failed to send CID.\n",
                         pPhone->nPhoneNumber));
               }
            }
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Phone No %d: No CID on this connection type %s (%d)\n",
                   pPhone->nPhoneNumber,
                   Common_Enum2Name(pNewConn->nType, TD_rgCallTypeName),
                   pNewConn->nType));
         }
      }
   } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */

#ifdef XT16
   Common_DTMF_Disable(pCtrl, pPhone);
#endif

   return IFX_SUCCESS;
} /* ST_Dialing_Dialing_Loc() */

/**
   Initialize external voip call, set phone and connection structure
   and send message to called phone.

   \param  pCtrl           - pointer to status control structure
   \param  pPhone          - pointer to PHONE
   \param  pConn           - pointer to phone connection
   \param  nCalledNumber   - number generated from dialed digits
   \param  nIP_addr        - for full ip call ip address of called phone
                             for normal voip call lowest part of ip address
   \param  pCalledIP       - called board's IP address (as a string)
                             set only for IPv6

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Dialing_Dialing_Ext(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn,
                                    IFX_int32_t nCalledNumber,
                                    IFX_uint32_t nIP_addr,
                                    IFX_char_t* pCalledIP)
{
   CONNECTION_t* pNewConn;
#ifdef EASY336
   SVIP_libStatus_t ret;
   RESOURCES_t resources;
   VOIP_DATA_CH_t *pCodec;
#endif /* EASY336 */
#ifdef TD_IPV6_SUPPORT
   IFX_int32_t nAddrFamily;
#endif /* TD_IPV6_SUPPORT */

   /* Start new external connection */
   pPhone->nDialNrCnt = 0;
   pPhone->fExtPeerCalled = IFX_TRUE;
   pPhone->nCallDirection = CALL_DIRECTION_TX;

   /* Use new empty connection */
   pNewConn = &pPhone->rgoConn[pPhone->nConnCnt];
   pPhone->nConnCnt++;

   /* reset structure */
   memset(pNewConn, 0, sizeof(CONNECTION_t));

   pNewConn->nType = EXTERN_VOIP_CALL;
   pNewConn->bExtDialingIn = IFX_TRUE;
   pNewConn->fActive = IFX_FALSE;
   pNewConn->bIPv6_Call = IFX_FALSE;
   TD_TIMEOUT_START(pCtrl->nUseTimeoutSocket);

   pNewConn->timeStartExtCalling = TD_OS_ElapsedTimeMSecGet(0);

#ifdef TD_IPV6_SUPPORT
   if(0 < strlen(pCalledIP))
   {
      /* Called board's IP address is in text form,
         so convert IP addresses from text to binary form */

      /* First check if it's IPv4 or IPv6 address */
      if (IFX_SUCCESS != Common_VerifyAddrIP(pCalledIP, &nAddrFamily))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, incorrect peer IP address (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         /* On error this connection was not used */
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
      memset(&pNewConn->oConnPeer.oRemote.oToAddr, 0,
             sizeof(pNewConn->oConnPeer.oRemote.oToAddr));
      TD_SOCK_FamilySet(&pNewConn->oConnPeer.oRemote.oToAddr, nAddrFamily);
      TD_SOCK_AddrIPSet(&pNewConn->oConnPeer.oRemote.oToAddr,
                        pCalledIP);

      if (AF_INET6 == nAddrFamily)
      {
         pNewConn->bIPv6_Call = IFX_TRUE;
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Peer address = %s\n",
          pPhone->nPhoneNumber, pCalledIP));
   }
   else
#endif /* TD_IPV6_SUPPORT */
   {
      /* Get called phone ip address */
      if (IFX_SUCCESS != Common_FillAddrByNumber(pCtrl, pPhone, pNewConn,
                                                 nIP_addr, IFX_FALSE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, getting peer address (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         /* On error this connection was not used */
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
   }

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType &&
       TD_NOT_SET == pPhone->nDataCh)
   {
      /* get data channel for dect phone (handset) */
      if (IFX_SUCCESS != TD_DECT_DataChannelAdd(pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, TD_DECT_DataChannelAdd failed\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
   }
#endif /* TD_DECT */
   /* check if external call is possible */
   if (0 >= pCtrl->nSumCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("%s: Unable to make external call, no coder channnel available\n",
             pPhone->pBoard->pszBoardName));
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* get resources for connection with main board */
      if (IFX_SUCCESS != Easy3111_CallResourcesGet(pPhone, pCtrl, pNewConn))
      {
         /* Failed to start connection */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Failed to get resource for extension board.\n"
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
   }
   else
#endif /* EASY3111 */
   {
      /* Save caller stuff */
      pNewConn->nUsedCh = pPhone->nDataCh;
      pNewConn->nUsedCh_FD = pPhone->nDataCh_FD;
#ifdef TAPI_VERSION4
      if (pPhone->pBoard->fUseSockets)
#endif
      {
         /* check if QoS and if using sockets */
         if (!pCtrl->pProgramArg->oArgFlags.nQos ||
             pCtrl->pProgramArg->oArgFlags.nQosSocketStart)
         {
            pNewConn->nUsedSocket = VOIP_GetSocket(pPhone->nDataCh,
                                        pPhone->pBoard, pNewConn->bIPv6_Call);
         }
      }
   }
   if (pCtrl->pProgramArg->oArgFlags.nQos)
   {
      /* QoS support */
      QOS_InitializePairStruct(pCtrl, pNewConn, pPhone->pBoard->nBoard_IDX);
   }

#ifdef EASY336
    pNewConn->voipID = SVIP_RM_UNUSED;
#endif /* EASY336 */

#ifdef EASY336
   if (pNewConn->voipID == SVIP_RM_UNUSED)
   {
      /* using IPv4 address of SVIP device */
      TD_SOCK_FamilySet(&pNewConn->oUsedAddr, AF_INET);

      /* get codec and voip ID */
      resources.nType = RES_CODEC | RES_VOIP_ID;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      if (Common_ResourcesGet(pPhone, &resources) != IFX_SUCCESS)
      {
         /* There is no more free data channels */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("No free codecs available "
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
      pNewConn->voipID = pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
      pCodec = Common_GetCodec(pCtrl, pNewConn->voipID, pPhone->nSeqConnId);
      if (pCodec != IFX_NULL)
      {
         pNewConn->nUsedCh = pCodec->nCh;
         TD_SOCK_PortSet(&pNewConn->oUsedAddr, pCodec->nUDP_Port);
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Common_GetCodec failed (File: %s, "
                "line: %d)\n",
                __FILE__, __LINE__));
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
      ret = SVIP_libDevIP_Get(pCodec->nDev,
               TD_SOCK_GetAddrIn(&pNewConn->oUsedAddr));
      if (ret != SVIP_libStatusOk)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_libDevIP_Get returned error %d (File: %s, "
                "line: %d)\n",
                ret, __FILE__, __LINE__));
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
   } /* if (pNewConn->voipID == SVIP_RM_UNUSED) */
#endif /* EASY336 */

#ifndef EASY336
   /* set used address */
#ifdef TD_IPV6_SUPPORT
   if (pNewConn->bIPv6_Call)
   {
      /* Save caller stuff */
      TD_SOCK_AddrCpy(&pNewConn->oUsedAddr, &pCtrl->oIPv6_Ctrl.oAddrIPv6);
   }
   else
#endif /* TD_IPV6_SUPPORT */
   {
      /* Save caller stuff */
      TD_SOCK_AddrCpy(&pNewConn->oUsedAddr, &pCtrl->oTapidemo_IP_Addr);
   }
#endif /* EASY336 */

#ifndef EASY336
   memcpy(pNewConn->oUsedMAC, pCtrl->oTapidemo_MAC_Addr, ETH_ALEN);
#else /* EASY336 */
#if defined(TD_IPV6_SUPPORT) && defined(CONFIG_LTQ_SVIP_NAT_PT)
   if (pNewConn->bIPv6_Call)
   {
      ret = SVIP_libMAC_AddressForVoIPv6Get(
         TD_SOCK_GetAddrIn(&pNewConn->oConnPeer.oRemote.oToAddr), pNewConn->oUsedMAC);
      if (ret != SVIP_libStatusOk)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_libMAC_AddressForVoIPv6Get(%s) returned error %d "
                "(File: %s, line: %d)\n",
                TD_GetStringIP(&pNewConn->oConnPeer.oRemote.oToAddr),
                ret, __FILE__, __LINE__));
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
   }
   else
#endif /* defined(TD_IPV6_SUPPORT) && defined(CONFIG_LTQ_SVIP_NAT_PT) */
   {
      ret = SVIP_libMAC_AddressForVoIPGet(
         TD_SOCK_AddrIPv4Get(&pNewConn->oConnPeer.oRemote.oToAddr),
         pNewConn->oUsedMAC);
      if (ret != SVIP_libStatusOk)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_libMAC_AddressForVoIPGet returned error %d (File: %s, "
                "line: %d)\n",
                ret, __FILE__, __LINE__));
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
   }
#endif /* EASY336 */

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fUseSockets)
#endif /* TAPI_VERSION4 */
   {
      if (!pCtrl->pProgramArg->oArgFlags.nQos ||
          pCtrl->pProgramArg->oArgFlags.nQosSocketStart)
      {
         TD_SOCK_PortSet(&pNewConn->oUsedAddr,
                         VOIP_GetSocketPort(pNewConn->nUsedSocket, pCtrl));
      }
#ifdef QOS_SUPPORT
      else
      {
         TD_SOCK_PortSet(&pNewConn->oUsedAddr,
                         ntohs(pNewConn->oSession.oPair.srcPort));
      }
#endif /* QOS_SUPPORT */
   } /* if (pPhone->pBoard->fUseSockets) */

   return IFX_SUCCESS;
}

/**
   Initialize pcm call, set phone and connection structure
   and send message to called phone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  nCalledNumber   - number generated from dialed digits
   \param  nIP_addr  - lowest part of ip address

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Dialing_Dialing_PCM(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn,
                                    IFX_int32_t nCalledNumber,
                                    IFX_uint32_t nIP_addr)
{
   CONNECTION_t* pNewConn;
   IFX_boolean_t fWideBand = IFX_FALSE;

   /* Check for band type (WB/NB) */
   if (TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
   }

   /* Is PCM supported? */
   if (pPhone->pBoard->nMaxPCM_Ch <= 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Warning, Phone No %d: %s: PCM not supported.\n",
                pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));
      return IFX_ERROR;
   }

   /* Is PCM configured */
   if ((!pCtrl->pProgramArg->oArgFlags.nPCM_Master) &&
       (!pCtrl->pProgramArg->oArgFlags.nPCM_Slave))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Failed to start PCM call: PCM is not set neither master nor slave"
             " (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   pPhone->nDialNrCnt = 0;
   pPhone->fPCM_PeerCalled = IFX_TRUE;
   pPhone->nCallDirection = CALL_DIRECTION_TX;

   /* Use new empty connection */
   pNewConn = &pPhone->rgoConn[pPhone->nConnCnt];
   pPhone->nConnCnt++;

   /* reset structure */
   memset(pNewConn, 0, sizeof(CONNECTION_t));

   pNewConn->nType = PCM_CALL;
   pNewConn->fActive = IFX_FALSE;
   pNewConn->bExtDialingIn = IFX_TRUE;
   /* Timeslots are not set */
   pNewConn->oPCM.nTimeslot_RX = TIMESLOT_NOT_SET;
   pNewConn->oPCM.nTimeslot_TX = TIMESLOT_NOT_SET;
   TD_TIMEOUT_START(pCtrl->nUseTimeoutSocket);

   pNewConn->timeStartExtCalling = TD_OS_ElapsedTimeMSecGet(0);

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* DECT phone reservers PCM channel for outgoing FXO calls */
      if (IFX_SUCCESS != TD_DECT_PCM_ChannelAdd(pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: unable to get PCM channel for DECT.\n",
                pPhone->nPhoneNumber));
         return IFX_ERROR;
      }
   }
#endif /* TD_DECT */
#ifdef EASY336
   pNewConn->voipID = SVIP_RM_UNUSED;
#endif /* EASY336 */
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* sets used channel and its FD and maps PCM channels */
      if (IFX_SUCCESS != Easy3111_CallResourcesGet(pPhone, pCtrl, pNewConn))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d: Failed to get resources."
                " (File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         /* On error this connection was not used */
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
   }
   else
#endif /* EASY3111 */
   {
      /* Save caller channel */
      pNewConn->nUsedCh = pPhone->nPCM_Ch;
      pNewConn->nUsedCh_FD = PCM_GetFD_OfCh(pPhone->nPCM_Ch, pPhone->pBoard);
      /* Map phone channel to pcm channel */
#if (!defined EASY336 && !defined XT16 && !defined EASY3201 && !defined EASY3201_EVS)
#ifdef TD_DECT
      if (PHONE_TYPE_DECT != pPhone->nType)
#endif /* TD_DECT */
      {
         /* Functions CONFERENCE_.. should be use only during conference */
         if (IFX_SUCCESS != PCM_MapToPhone(pPhone->nPCM_Ch, pPhone->nPhoneCh,
                               IFX_TRUE, pPhone->pBoard, pPhone->nSeqConnId))
         {
            /* On error this connection was not used */
            pPhone->nConnCnt--;
            return IFX_ERROR;
         }
      }
#endif /* (!defined EASY336 && !defined XT16 && !defined EASY3201) */
   }
   memcpy(pNewConn->oUsedMAC, pCtrl->oTapidemo_MAC_Addr, ETH_ALEN);
   TD_SOCK_AddrCpy(&pNewConn->oUsedAddr, &pCtrl->oTapidemo_IP_Addr);
   TD_SOCK_PortSet(&pNewConn->oUsedAddr, ADMIN_UDP_PORT);

   /* save peer number */
   pNewConn->oConnPeer.nPhoneNum = nCalledNumber;

   /* Prepare for PCM call */
   if (IFX_SUCCESS != Common_FillAddrByNumber(pCtrl, pPhone, pNewConn,
                                              nIP_addr, IFX_TRUE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, getting peer address (File: %s, line: %d)\n",
             __FILE__, __LINE__));

      /* On error this connection was not used */
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }

   /* Only master board chooses timeslots for connection
      Slave board receives timeslots from master board */
   if (pCtrl->pProgramArg->oArgFlags.nPCM_Master)
   {
      /* You are master board, so choose timeslot */
      pNewConn->oPCM.nTimeslot_RX = PCM_GetTimeslots(TS_RX, fWideBand,
                                                     pPhone->nSeqConnId);
      pNewConn->oPCM.nTimeslot_TX = pNewConn->oPCM.nTimeslot_RX;
   }

   return IFX_SUCCESS;
}

#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
/**
   Initialize fxo call, set phone and connection structure.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  nCalledFxoNumber   - number generated from dialed digits

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Dialing_Dialing_FXO(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn,
                                    IFX_int32_t nCalledFxoNumber)
{
   IFX_return_t ret = IFX_SUCCESS;
   CONNECTION_t* pNewConn;
   FXO_t* pUsedFxo = IFX_NULL;

   /* if using FXO */
   if (!pCtrl->pProgramArg->oArgFlags.nFXO)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Unable to make FXO Call Tapidemo started without FXO option. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Making FXO call, number %d\n",
          pPhone->nPhoneNumber, (IFX_int32_t) nCalledFxoNumber));

   /* Clear up digit buffer so we can make other call,
      activate another feature */
   pPhone->nDialNrCnt = 0;
   pPhone->nCallDirection = CALL_DIRECTION_TX;

   if (nCalledFxoNumber < 1 ||
       nCalledFxoNumber > pCtrl->nMaxFxoNumber)

   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Unable to make FXO Call - invalid FXO number %d.\n",
             nCalledFxoNumber));
      /* wrong FXO number. FXO numbers start from 1. */
      return IFX_ERROR;
   }
#ifdef EASY3111
   /* this board can be connected through PCM */
   if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
#endif /* EASY3111 */
   {
      /* check if phone on main board */
      if (0 != pPhone->pBoard->nBoard_IDX)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: Unable to make FXO Call, "
                "phone on extension board,\n"
                "%sFXO call can only be made from main board.\n",
                pPhone->nPhoneNumber, pPhone->pBoard->pIndentPhone));
         return IFX_ERROR;
      } /* check if phone on main board */
   }
   /** get pointer to FXO */
   pUsedFxo = ABSTRACT_GetFxo(nCalledFxoNumber, pCtrl);
   if (IFX_NULL == pUsedFxo)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Unable to make FXO Call - unable to get FXO for number %d.\n",
             nCalledFxoNumber));
      return IFX_ERROR;
   }

   if (S_READY != pUsedFxo->nState)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Unable to make FXO Call, "
             "FXO No %d not in ready state.\n",
             pPhone->nPhoneNumber, nCalledFxoNumber));
      return IFX_ERROR;
   }

   /* Use new empty connection */
   pNewConn = &pPhone->rgoConn[pPhone->nConnCnt];
   pPhone->nConnCnt++;
   pNewConn->nType = FXO_CALL;
   /* Timeslots are not set */
   pNewConn->oPCM.nTimeslot_RX = TIMESLOT_NOT_SET;
   pNewConn->oPCM.nTimeslot_TX = TIMESLOT_NOT_SET;

   /* Which phone is using FXO */
   pPhone->pFXO = pUsedFxo;
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* DECT phone reservers PCM channel for outgoing FXO calls */
      if (IFX_SUCCESS != TD_DECT_PCM_ChannelAdd(pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: unable to get PCM channel for DECT.\n",
                pPhone->nPhoneNumber));
         return IFX_ERROR;
      }
   }
#endif /* TD_DECT */
#ifdef EASY3111
   /* phone from this board can be connected through PCM */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      if (IFX_SUCCESS != Easy3111_CallResourcesGet(pPhone, pCtrl, pNewConn))
      {
         /* Failed to start connection */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Failed to get resource for extension board.\n"
                "(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
      if (IFX_SUCCESS != Easy3111_CallPCM_Activate(pPhone, pCtrl))
      {
         /* Failed to start connection */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d: Easy3111_CallPCM_Activate() failed.\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         Easy3111_CallResourcesRelease(pPhone, pCtrl, pNewConn);
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
   }
   else
#endif /* EASY3111 */
   {
      if (FXO_TYPE_SLIC121 == pUsedFxo->pFxoDevice->nFxoType)
      {
         /* use the ALM channel assigned to slic121 fxo */
         pNewConn->nUsedCh = pUsedFxo->nFxoCh;
         pNewConn->nUsedCh_FD = pUsedFxo->nFxoCh_FD;
      }
      else
      {
         /* use the pcm channel assigned to phone */
         pNewConn->nUsedCh = pPhone->nPCM_Ch;
         pNewConn->nUsedCh_FD = PCM_GetFD_OfCh(pPhone->nPCM_Ch, pPhone->pBoard);
      }
   }


   /* Must put a delay, otherwise we are too fast sometimes and
      phone detects last DTMF digit? Put delay 100 ms. */
   TD_OS_MSecSleep(100);

   /* Map PCM channels, DUSLIC PCM channels. */
   ret = FXO_StartConnection(pCtrl->pProgramArg, pUsedFxo,
                             pPhone, pNewConn, pPhone->pBoard);

   if (ret != IFX_SUCCESS)
   {
#ifdef EASY3111
      /* this board can be connected through PCM */
      if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
      {
         Easy3111_CallPCM_Deactivate(pPhone, pCtrl);
         Easy3111_CallResourcesRelease(pPhone, pCtrl, pNewConn);
      }
#endif /* EASY3111 */
      /* On error this connection was not used */
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }
   /* Now phone will hear new dialtone and can dial phone number. */
   return IFX_SUCCESS;
}
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */

/**
   Initialize local call, set phone and connection structure
   and send message to called phone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  nCalledNumber   - number of called phone

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Cf_Dialing_Dialing_Loc(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn,
                                       IFX_int32_t nCalledNumber)
{
   PHONE_t* pDstPhone;
   CONNECTION_t* pNewConn;
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
   IFX_int32_t nUsedFd, i;

   pDstPhone = ABSTRACT_GetPHONE_OfCalledNum(pCtrl, nCalledNumber,
                                             pPhone->nSeqConnId);

   if (pDstPhone == IFX_NULL)
   {
      /* Could not find board, phone for this calling number */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Calling wrong phone number.\n",
             pPhone->nPhoneNumber));
      return IFX_ERROR;
   } /* if (pDstPhone == IFX_NULL) */
   /* check if not callin own phone */
   if (pDstPhone == pPhone)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Calling our own phone number.\n",
             pPhone->nPhoneNumber));
      return IFX_ERROR;
   } /* if (pDstPhone == pPhone) */
   /* check if phone not in call */
   if (S_READY != pDstPhone->rgStateMachine[FXS_SM].nState)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Called Phone No %d not in ready state.\n",
             pPhone->nPhoneNumber, pDstPhone->nPhoneNumber));
      return IFX_ERROR;
   } /* if (pDstPhone == pPhone) */
#ifdef EASY336
   /* get codec */
   if(pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
   {
      resources.nType = RES_CODEC | RES_VOIP_ID;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      if (Common_ResourcesGet(pPhone, &resources) != IFX_SUCCESS)
      {
         return IFX_ERROR;
      }
   }
#endif /* EASY336 */

   for (i=0; i<pPhone->nConnCnt; i++)
   {
      if (LOCAL_CALL == pPhone->rgoConn[i].nType)
      {
         if (nCalledNumber == pPhone->rgoConn[i].oConnPeer.nPhoneNum)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Phone No %d: Calling phone that already is in conference.\n",
                   pPhone->nPhoneNumber));
            return IFX_ERROR;
         }
      } /* if local call */
   } /* for all connections */

   /* reset dialed digits count */
   pPhone->nDialNrCnt = 0;
   /* Use new empty connection */
   pNewConn = &pPhone->rgoConn[pPhone->nConnCnt];
   pPhone->nConnCnt++;

   if (IFX_SUCCESS != Common_SetLocalCallType(pPhone, pDstPhone, pNewConn))
   {
      /* Can use only PCM channels */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Unable to get call type. (File: %s, line: %d)\n",
             (IFX_int32_t) pPhone->nPhoneNumber, __FILE__, __LINE__));
      /* this connection was not used */
      pPhone->nConnCnt--;
      return IFX_ERROR;

   }
   if (LOCAL_CALL != pNewConn->nType)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Call type %s, only %s supported for conference."
             "(File: %s, line: %d)\n",
             (IFX_int32_t) pPhone->nPhoneNumber,
             Common_Enum2Name(pNewConn->nType, TD_rgCallTypeName),
             Common_Enum2Name(LOCAL_CALL, TD_rgCallTypeName),
             __FILE__, __LINE__));
   }
#ifdef EASY336
   if (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
   {
      pNewConn->voipID = pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
   }
   else
   {
      pNewConn->voipID = SVIP_RM_UNUSED;
   }
#endif /* EASY336 */
   pNewConn->oConnPeer.nPhoneNum = nCalledNumber;
   /* Save called stuff */
   pNewConn->oConnPeer.nPhoneNum = pDstPhone->nPhoneNumber;
   /* Link myself to called phone */
   pNewConn->oConnPeer.oLocal.pPhone = pDstPhone;
   /* we are in some sort master, the called phone will be slave */
   pNewConn->oConnPeer.oLocal.fSlave = IFX_FALSE;

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* Save caller stuff */
      pNewConn->nUsedCh = pPhone->nDectCh;
      pNewConn->nUsedCh_FD =
         TD_DECT_GetFD_OfCh(pPhone->nDectCh, pPhone->pBoard);
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* Save caller stuff */
      pNewConn->nUsedCh = pPhone->oEasy3111Specific.nOnMainPCM_Ch;
      pNewConn->nUsedCh_FD = pPhone->oEasy3111Specific.nOnMainPCM_ChFD;
   }
   else
#endif /* EASY3111 */
   {
      /* Save caller stuff */
      pNewConn->nUsedCh = pPhone->nPhoneCh;
      pNewConn->nUsedCh_FD = pPhone->nPhoneCh_FD;
   }

   ABSTRACT_SeqConnID_Set(pDstPhone, IFX_NULL, pPhone->nSeqConnId);

   if (IFX_SUCCESS != CONFERENCE_PeerLocal_Add(pCtrl, pPhone, pDstPhone,
                                               pNewConn))
   {
      /* On error this connection was not used */
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }

   /* I want the called phone to establish a local call with me */
   if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, pNewConn, IE_RINGING,
                                       pPhone->nSeqConnId))
   {
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }

   if (pCtrl->pProgramArg->oArgFlags.nCID)
   {
      /* CID can be only send locally and on same board. */
#ifdef TD_DECT
      /* no CID for DECT phones */
      if (PHONE_TYPE_DECT != pDstPhone->nType)
#endif /* TD_DECT */
      {
         /* Send ONHOOK CID message to called phone */
#ifndef EASY336
         if (0 < pDstPhone->pBoard->nMaxCoderCh)
         {
            nUsedFd = VOIP_GetFD_OfCh(pDstPhone->nDataCh, pDstPhone->pBoard);
         }
         else
#endif /*  EASY336 */
         if (0 < pDstPhone->pBoard->nMaxAnalogCh)
         {
            nUsedFd = ALM_GetFD_OfCh(pDstPhone->pBoard, pDstPhone->nPhoneCh);
         }
         else
         {
            /* continue call without CID */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Phone No %d: No FD to send CID.\n",
                   pPhone->nPhoneNumber));
            return IFX_SUCCESS;
         }
         if (IFX_ERROR == CID_Send (nUsedFd, pDstPhone, IFX_TAPI_CID_HM_ONHOOK,
                                    pPhone->pCID_Msg, pPhone))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Phone No %d: Failed to send CID.\n",
                   pPhone->nPhoneNumber));
            return IFX_ERROR;
         }
      }
   } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */

   return IFX_SUCCESS;
}

/**
   Initialize external voip call, set phone and connection structure
   and send message to called phone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  nCalledNumber   - number generated from dialed digits
   \param  nIP_addr  - for full ip call ip address of called phone
                       for normal voip call lowest part of ip address
   \param  pCalledIP - called board's IP address (as a string)
                       set only for IPv6

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Cf_Dialing_Dialing_Ext(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn,
                                       IFX_int32_t nCalledNumber,
                                       IFX_uint32_t nIP_addr,
                                       IFX_char_t* pCalledIP)
{
   CONNECTION_t* pNewConn;
#ifdef EASY336
   IFX_return_t ret = IFX_SUCCESS;
   VOIP_DATA_CH_t *pCodec;
#endif /* EASY336 */
#ifdef TD_IPV6_SUPPORT
   IFX_int32_t nAddrFamily;
#endif /* TD_IPV6_SUPPORT */

   /* Start new external connection */
   pPhone->nDialNrCnt = 0;
   /* get first unused connection structure */
   pNewConn = &pPhone->rgoConn[pPhone->nConnCnt];
   pPhone->nConnCnt++;
   /* set call type */
   pNewConn->nType = EXTERN_VOIP_CALL;
   pNewConn->bExtDialingIn = IFX_TRUE;
   pNewConn->fActive = IFX_FALSE;
   pNewConn->bIPv6_Call = IFX_FALSE;
   TD_TIMEOUT_START(pCtrl->nUseTimeoutSocket);

#ifdef EASY336
   pNewConn->voipID = SVIP_RM_UNUSED;
#endif /* EASY336 */

   /* set timeout */
   pNewConn->timeStartExtCalling = TD_OS_ElapsedTimeMSecGet(0);

   /* reset table */
   memset(pNewConn->oConnPeer.oRemote.rgCollectNumber, 0,
          sizeof(pNewConn->oConnPeer.oRemote.rgCollectNumber));

#ifdef TD_IPV6_SUPPORT
   if(0 < strlen(pCalledIP))
   {
      /* Called board's IP address is in text form,
         so convert IP addresses from text to binary form */

      /* First check if it's IPv4 or IPv6 address */
      if (IFX_SUCCESS != Common_VerifyAddrIP(pCalledIP, &nAddrFamily))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, incorrect peer IP address (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         /* On error this connection was not used */
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }
      memset(&pNewConn->oConnPeer.oRemote.oToAddr, 0,
             sizeof(pNewConn->oConnPeer.oRemote.oToAddr));
      TD_SOCK_FamilySet(&pNewConn->oConnPeer.oRemote.oToAddr, nAddrFamily);
      TD_SOCK_AddrIPSet(&pNewConn->oConnPeer.oRemote.oToAddr,
                        pCalledIP);

      if (AF_INET6 == nAddrFamily)
      {
         pNewConn->bIPv6_Call = IFX_TRUE;
      }

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Peer address = %s\n",
          pPhone->nPhoneNumber, pCalledIP));
   }
   else
#endif /* TD_IPV6_SUPPORT */
   /* Prepare for external call */
   if (IFX_SUCCESS != Common_FillAddrByNumber(pCtrl, pPhone, pNewConn,
                                              nIP_addr, IFX_FALSE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, getting peer address (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      /* On error this connection was not used */
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }
   /* check if external call is possible */
   if (0 >= pCtrl->nSumCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("%s: Unable to make external call, no coder channnel available\n",
             pPhone->pBoard->pszBoardName));
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }

   /* add peer to conference */
   if (IFX_SUCCESS != CONFERENCE_PeerExternal_Add(pCtrl, pPhone,
                                                  pNewConn))
   {
      /* On error this connection was not used */
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }
   if (pCtrl->pProgramArg->oArgFlags.nQos)
   {
      /* QoS support */
      QOS_InitializePairStruct(pCtrl, pNewConn, pPhone->pBoard->nBoard_IDX);
   }
   /* Save caller stuff */
#ifndef EASY336
   memcpy(pNewConn->oUsedMAC, pCtrl->oTapidemo_MAC_Addr, ETH_ALEN);
#else
#if defined(TD_IPV6_SUPPORT) && defined(CONFIG_LTQ_SVIP_NAT_PT)
   if (AF_INET6 == nAddrFamily)
   {
      ret = SVIP_libMAC_AddressForVoIPv6Get(
         TD_SOCK_GetAddrIn(&pNewConn->oConnPeer.oRemote.oToAddr), pNewConn->oUsedMAC);
   }
   else
#endif /* defined(TD_IPV6_SUPPORT) && defined(CONFIG_LTQ_SVIP_NAT_PT) */
   {
      ret = SVIP_libMAC_AddressForVoIPGet(
         TD_SOCK_AddrIPv4Get(&pNewConn->oConnPeer.oRemote.oToAddr), pNewConn->oUsedMAC);
   }

   if (ret != SVIP_libStatusOk)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_libMAC_AddressForVoIPGet returned error %d (File: %s, "
             "line: %d)\n",
             ret, __FILE__, __LINE__));
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }
#endif

#ifdef EASY336
   pCodec = Common_GetCodec(pCtrl, pNewConn->voipID, pPhone->nSeqConnId);
   if (pCodec != IFX_NULL)
   {
      TD_SOCK_PortSet(&pNewConn->oUsedAddr, pCodec->nUDP_Port);
   } /* if (pCodec != IFX_NULL) */
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Common_GetCodec failed (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      /* On error this connection was not used */
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }
   /* Save caller stuff - src IP is FW IP */
   ret = SVIP_libDevIP_Get(pCodec->nDev,
            TD_SOCK_GetAddrIn(&pNewConn->oUsedAddr));

   if (SVIP_libStatusOk != ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, SVIP_libDevIP_Get returned error %d (File: %s, line: %d)\n",
             ret, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#else /* EASY336 */
   /* set used address */
#ifdef TD_IPV6_SUPPORT
   if (pNewConn->bIPv6_Call)
   {
      /* Save caller stuff */
      TD_SOCK_AddrCpy(&pNewConn->oUsedAddr, &pCtrl->oIPv6_Ctrl.oAddrIPv6);
   }
   else
#endif /* TD_IPV6_SUPPORT */
   {
      /* Save caller stuff */
      TD_SOCK_AddrCpy(&pNewConn->oUsedAddr, &pCtrl->oTapidemo_IP_Addr);
   }
#endif /* EASY336 */

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fUseSockets)
#endif
   {
      if ( !pCtrl->pProgramArg->oArgFlags.nQos ||
           pCtrl->pProgramArg->oArgFlags.nQosSocketStart)
      {
         TD_SOCK_PortSet(&pNewConn->oUsedAddr,
                         VOIP_GetSocketPort(pNewConn->nUsedSocket, pCtrl));
      }
#ifdef QOS_SUPPORT
      else
      {
         TD_SOCK_PortSet(&pNewConn->oUsedAddr,
                         ntohs(pNewConn->oSession.oPair.srcPort));
      }
#endif /* QOS_SUPPORT */
   } /* if (pPhone->pBoard->fUseSockets) */
   return IFX_SUCCESS;
}

/**
   Initialize pcm call, set phone and connection structure
   and send message to called phone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  nCalledNumber   - number generated from dialed digits
   \param  nIP_addr  - lowest part of ip address

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Cf_Dialing_Dialing_PCM(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn,
                                       IFX_int32_t nCalledNumber,
                                       IFX_uint32_t nIP_addr)
{
   CONNECTION_t* pNewConn;
   IFX_boolean_t fWideBand = IFX_FALSE;

   /* Check for band type (WB/NB) */
   if (TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
   }

   /* Is PCM supported? */
   if (pPhone->pBoard->nMaxPCM_Ch <= 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Warning, Phone No %d: %s: PCM not supported.\n",
                pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));
      return IFX_ERROR;
   }

   /* check if using PCM */
   if ((pCtrl->pProgramArg->oArgFlags.nPCM_Master) ||
       (pCtrl->pProgramArg->oArgFlags.nPCM_Slave))
   {
      pPhone->nDialNrCnt = 0;
      /* Use new empty connection */
      pNewConn = &pPhone->rgoConn[pPhone->nConnCnt];
      pPhone->nConnCnt++;

      pNewConn->nType = PCM_CALL;
      pNewConn->fActive = IFX_FALSE;
      pNewConn->bExtDialingIn = IFX_TRUE;
      TD_TIMEOUT_START(pCtrl->nUseTimeoutSocket);

#ifdef EASY336
      pNewConn->voipID = SVIP_RM_UNUSED;
#endif /* EASY336 */

      /* set timeout */
      pNewConn->timeStartExtCalling = TD_OS_ElapsedTimeMSecGet(0);

      /* Timeslots are not set */
      pNewConn->oPCM.nTimeslot_RX = TIMESLOT_NOT_SET;
      pNewConn->oPCM.nTimeslot_TX = TIMESLOT_NOT_SET;

      if (IFX_SUCCESS != CONFERENCE_PeerPCM_Add(pCtrl, pPhone, pNewConn))
      {
         /* On error this connection was not used */
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }

      /* Prepare for PCM call */
      if (IFX_SUCCESS != Common_FillAddrByNumber(pCtrl, pPhone, pNewConn,
                                                 nIP_addr, IFX_TRUE))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, getting peer address (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         /* On error this connection was not used */
         pPhone->nConnCnt--;
         return IFX_ERROR;
      }

      /* Save called stuff */
      memcpy(pNewConn->oUsedMAC, pCtrl->oTapidemo_MAC_Addr, ETH_ALEN);
      pNewConn->oConnPeer.nPhoneNum = nCalledNumber;
      TD_SOCK_AddrCpy(&pNewConn->oUsedAddr, &pCtrl->oTapidemo_IP_Addr);
      TD_SOCK_PortSet(&pNewConn->oUsedAddr, ADMIN_UDP_PORT);

      /* Only master board chooses timeslots for connection */
      /* Slave board receives timeslots from master board */
      if (pCtrl->pProgramArg->oArgFlags.nPCM_Master)
      {
         /* You are master board, so choose timeslot */
         pNewConn->oPCM.nTimeslot_RX = PCM_GetTimeslots(TS_RX, fWideBand,
                                                        pPhone->nSeqConnId);
         pNewConn->oPCM.nTimeslot_TX = pNewConn->oPCM.nTimeslot_RX;
      }

      /* I want the called phone to establish a PCM call with me */
      if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, pNewConn, IE_RINGING,
                                          pPhone->nSeqConnId))
      {
         pPhone->nConnCnt--;
         /* Play busy tone and change phone state */
         PCM_FreeTimeslots(pNewConn->oPCM.nTimeslot_RX, pPhone->nSeqConnId);
         return IFX_ERROR;
      }
   } /* if ((pCtrl->pProgramArg->oArgFlags.nPCM_Master) ...*/
   else
   {
      /* PCM is not supported */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: PCM is not supported - called number %d."
             "(File: %s, line: %d)\n",
             (IFX_int32_t) pPhone->nPhoneNumber,
             (IFX_int32_t) nCalledNumber,
             __FILE__, __LINE__));
      return IFX_ERROR;
   } /* if ((oProgramArg.oArgFlags.nPCM_Master) ...*/

   return IFX_SUCCESS;
}

#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
/**
   Initialize fxo call, set phone and connection structure.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  nCalledFxoNumber   - number generated from dialed digits

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_CF_Dialing_Dialing_FXO(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn,
                                       IFX_int32_t nCalledFxoNumber)
{
   IFX_return_t ret = IFX_SUCCESS;
   CONNECTION_t* pNewConn;
   FXO_t* pUsedFxo = IFX_NULL;

   /* if using FXO */
   if (!pCtrl->pProgramArg->oArgFlags.nFXO)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Unable to make FXO Call Tapidemo started without FXO option. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (IFX_NULL != pPhone->pFXO)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Unable to add second FXO device to conference.\n"
             "%sOnly on FXO device can be added to conference\n",
             pPhone->nPhoneNumber, pPhone->pBoard->pIndentPhone));
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Making FXO call, number %d\n",
          pPhone->nPhoneNumber, (IFX_int32_t) nCalledFxoNumber));

   /* Clear up digit buffer so we can make other call,
      activate another feature */
   pPhone->nDialNrCnt = 0;

   if (nCalledFxoNumber < 1 ||
       nCalledFxoNumber > pCtrl->nMaxFxoNumber)

   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Unable to make FXO Call - invalid FXO number %d.\n",
             nCalledFxoNumber));
      /* wrong FXO number. FXO numbers start from 1. */
      return IFX_ERROR;
   }

#ifdef EASY3111
   /* this board can be connected through PCM */
   if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
#endif /* EASY3111 */
   {
      /* check if phone on main board */
      if (0 != pPhone->pBoard->nBoard_IDX)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: Unable to make FXO Call, "
                "phone on extension board,\n"
                "%sFXO call can only be made from main board.\n",
                pPhone->nPhoneNumber, pPhone->pBoard->pIndentPhone));
         return IFX_ERROR;
      } /* check if phone on main board */
   }
   /** get pointer to FXO */
   pUsedFxo = ABSTRACT_GetFxo(nCalledFxoNumber, pCtrl);
   if (IFX_NULL == pUsedFxo)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Unable to make FXO Call - unable to get FXO for number %d.\n",
             nCalledFxoNumber));
      return IFX_ERROR;
   }

   if (S_READY != pUsedFxo->nState)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Unable to make FXO Call, "
             "FXO No %d not in ready state.\n",
             pPhone->nPhoneNumber, nCalledFxoNumber));
      return IFX_ERROR;
   }

   /* Use new empty connection */
   pNewConn = &pPhone->rgoConn[pPhone->nConnCnt];
   pPhone->nConnCnt++;
   pNewConn->nType = FXO_CALL;
   /* Timeslots are not set */
   pNewConn->oPCM.nTimeslot_RX = TIMESLOT_NOT_SET;
   pNewConn->oPCM.nTimeslot_TX = TIMESLOT_NOT_SET;

   /* Which phone is using FXO */
   pPhone->pFXO = pUsedFxo;

   /* add fxo to conference */
   if (IFX_SUCCESS != CONFERENCE_PeerFXO_Add(pCtrl, pPhone, pNewConn, pUsedFxo))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Failed to add fxo %d to conference.\n",
             pPhone->nPhoneNumber, nCalledFxoNumber));
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }

   /* Must put a delay, otherwise we are too fast sometimes and
      phone detects last DTMF digit? Put delay 100 ms. */
   TD_OS_MSecSleep(100);

   /* Map PCM channels, DUSLIC PCM channels. */
   ret = FXO_StartConnection(pCtrl->pProgramArg, pUsedFxo,
                             pPhone, pNewConn, pPhone->pBoard);

   if (ret != IFX_SUCCESS)
   {
      /* On error this connection was not used */
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }
   /* Now phone will hear new dialtone and can dial phone number. */
   return IFX_SUCCESS;
}
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */

/* ============================= */
/* Global function definition     */
/* ============================= */

/**
   Handles a state transition for FXS state machine

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_HandleState_FXS(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                CONNECTION_t* pConn)
{
   IFX_int32_t i = 0, j = 0;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_boolean_t fContinue = IFX_TRUE;
   EVENT_ACTIONS_t* pActionsList;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);

   /* if pCtrl->pProgramArg->oArgFlags.nWait is set wait for user data */
   STATE_MACHINE_WAIT(pCtrl);

#ifdef TD_DECT
   /* no default data ch fd for DECT phone */
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* no data channel for DECT phone */
      fd_data_ch = TD_NOT_SET;
   }
   else
#endif /* TD_DECT */
   /* if connection is not configured,
      then used channel can be set to TD_NOT_SET */
   if (TD_NOT_SET == pConn->nUsedCh)
   {
      fd_data_ch = TD_NOT_SET;
   }
   /* get used data channel file descriptor */
   else if (pPhone->pBoard->nMaxCoderCh > 0 &&
            pPhone->pBoard->nMaxCoderCh > pConn->nUsedCh)
   {
      fd_data_ch = VOIP_GetFD_OfCh(pConn->nUsedCh, pPhone->pBoard);
   }
   else if (pPhone->pBoard->nMaxAnalogCh > 0 &&
            pPhone->pBoard->nMaxAnalogCh > pConn->nUsedCh)
   {
      /* if no data channel available then most of ioctls are used with phone
         channel*/
      fd_data_ch = Common_GetFD_OfCh(pPhone->pBoard, pConn->nUsedCh);
   }
   else
   {
      fd_data_ch = TD_NOT_SET;
   }

   /* for this state transition acquire new Seq Conn ID */
   if (S_READY == pPhone->rgStateMachine[FXS_SM].nState &&
       IE_HOOKOFF == pPhone->nIntEvent_FXS)
   {
      ABSTRACT_SeqConnID_Get(pPhone, IFX_NULL, pCtrl);
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("Phone No %d: FXS state: %s FXS event: %s\n",
       pPhone->nPhoneNumber,
       Common_Enum2Name(pPhone->rgStateMachine[FXS_SM].nState, TD_rgStateName),
       Common_Enum2Name(pPhone->nIntEvent_FXS, TD_rgIE_EventsName)));

   /* Looking for selected state */
   while (S_END !=  ST_rgFXSStates[i].nState)
   {
      if (ST_rgFXSStates[i].nState == pPhone->rgStateMachine[FXS_SM].nState)
      {
         /* Select proper array with events */
         pActionsList = ST_rgFXSStates[i].pEventActions;

         /* Looking for selected event for particular state */
         while(IFX_TRUE == fContinue)
         {
            /* check if end of EVENT_ACTIONS_t table */
            if (pActionsList[j].nEvent == IE_END)
            {
               /* Didn't find selected event for particular state */
               pPhone->nIntEvent_FXS = IE_NONE;
               /* leave while loop */
               fContinue = IFX_FALSE;
            }
            else if (pActionsList[j].nEvent == pPhone->nIntEvent_FXS)
            {
               /* Found selected event */
               /* remember if phone was in conference */
               IFX_int32_t nTmpConfIdx = pPhone->nConfIdx;
               /* remember connection call type */
               CALL_TYPE_t nOldCallType = pConn->nType;

               /* Execute action for selected event */
               /*Executing Your Event Here for sure*/
               ret = (*pActionsList[j].pAction)(pCtrl, pPhone, pConn);

               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW,
                  (TD_CONN_ID_INIT == pPhone->nSeqConnId) ?
                   TD_CONN_ID_NOT_SET : pPhone->nSeqConnId,
                   ("Phone No %d: Handling state executed, current FXS state: %s\n",
                    pPhone->nPhoneNumber,
                    Common_Enum2Name(pPhone->rgStateMachine[FXS_SM].nState,
                                     TD_rgStateName)));

               if (IFX_SUCCESS == ret)
               {
                  /* exception: for conference call progress TRACES will be used
                     in CONFERENCE_RemovePeer() and add peer functions */
                  if (NO_CONFERENCE == nTmpConfIdx &&
                      ((IE_READY != pActionsList[j].nEvent) ||
                       (NO_CONFERENCE == pPhone->nConfIdx)))

                  {
                     Common_PrintCallProgress(ST_rgFXSStates[i].nState,
                        pActionsList[j].nEvent, nOldCallType, pPhone, pConn,
                        pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal);
                  }
               } /* if (IFX_SUCCESS == ret) */
               /* leave while loop */
               fContinue = IFX_FALSE;
               if (S_READY == pPhone->rgStateMachine[FXS_SM].nState &&
                   TD_CONN_ID_INIT != pPhone->nSeqConnId)
               {
                  ABSTRACT_SeqConnID_Reset(pPhone, IFX_NULL);
               }
            }  /* if (pActionsList[j].nEvent == ..) */
            /* check next event */
            j++;
         } /* while(IFX_TRUE == fContinue) */
         /* phone FXS state found - leave for loop */
         break;
      } /* if phone FXS state found in ST_rgFXSStates table. */
      /* go to next state */
      i++;
   }
#ifdef TD_RM_DEBUG
   /* print number of resources */
   TD_RM_TRACE_PHONE_RESOURCES(pPhone, pPhone->nSeqConnId);
#endif /* TD_RM_DEBUG */
   /* if fContinue value is set to IFX_TRUE then
      pPhone->rgStateMachine[FXS_SM].nState was not found in
      ST_rgFXSStates state list */
   if (IFX_TRUE == fContinue)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
         (TD_CONN_ID_INIT == pPhone->nSeqConnId) ?
          TD_CONN_ID_NOT_SET : pPhone->nSeqConnId,
          ("Phone No %d: State %d not found in ST_rgFXS States table."
           "(File: %s, line: %d)\n",
           pPhone->nPhoneNumber, pPhone->rgStateMachine[FXS_SM].nState,
           __FILE__, __LINE__));
      pPhone->rgStateMachine[FXS_SM].nState = IE_NONE;
      return IFX_ERROR;
   } /* if (IFX_TRUE == fContinue) */
   return IFX_SUCCESS;
} /* ST_HandleState_FXS() */

/**
   Handles a state transition for configuration state machine.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_HandleState_Config(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn)

{
   int i=0, j=0;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_boolean_t fContinue = IFX_TRUE;
   EVENT_ACTIONS_t* pActionsList;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone->pBoard, IFX_ERROR);

   /* if pCtrl->pProgramArg->oArgFlags.nWait is set wait for user data */
   STATE_MACHINE_WAIT(pCtrl);

   /* get used data channel file descriptor */
#ifdef TD_DECT
   /* no default data ch fd for DECT phone */
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* no data channel for DECT phone */
      fd_data_ch = TD_NOT_SET;
   }
   else
#endif /* TD_DECT */
   /* if connection is not configured,
      then used channel can be set to TD_NOT_SET */
   if (TD_NOT_SET == pConn->nUsedCh)
   {
      fd_data_ch = TD_NOT_SET;
   }
   else if (pPhone->pBoard->nMaxCoderCh > 0 &&
            pPhone->pBoard->nMaxCoderCh > pConn->nUsedCh)
   {
      fd_data_ch = VOIP_GetFD_OfCh(pConn->nUsedCh, pPhone->pBoard);
   }
   else if (pPhone->pBoard->nMaxAnalogCh > 0 &&
            pPhone->pBoard->nMaxAnalogCh > pConn->nUsedCh)
   {
      fd_data_ch = Common_GetFD_OfCh(pPhone->pBoard, pConn->nUsedCh);
   }
   else
   {
      fd_data_ch = TD_NOT_SET;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW,
      (TD_CONN_ID_INIT == pPhone->nSeqConnId) ?
       TD_CONN_ID_NOT_SET : pPhone->nSeqConnId,
      ("Phone No %d: Config state: %s Config event: %s\n",
       pPhone->nPhoneNumber,
       Common_Enum2Name(pPhone->rgStateMachine[CONFIG_SM].nState, TD_rgStateName),
       Common_Enum2Name(pPhone->nIntEvent_Config, TD_rgIE_EventsName)));

   /* Looking for selected state */
   while (S_END !=  ST_rgConfigStates[i].nState)
   {
      if (ST_rgConfigStates[i].nState ==
          pPhone->rgStateMachine[CONFIG_SM].nState)
      {
         /* Select proper array with events */
         pActionsList = ST_rgConfigStates[i].pEventActions;

         /* Looking for selected event for particular state */
         while(IFX_TRUE == fContinue)
         {
            /* check if end of EVENT_ACTIONS_t table */
            if (pActionsList[j].nEvent == IE_END)
            {
               /* Didn't find selected event for particular state */
               pPhone->nIntEvent_Config = IE_NONE;
               /* leave while loop */
               fContinue = IFX_FALSE;
            }
            else if (pActionsList[j].nEvent == pPhone->nIntEvent_Config)
            {
               /* Execute action for selected event */
               ret = (*pActionsList[j].pAction)(pCtrl, pPhone, pConn);
               /* leave while loop */
               fContinue = IFX_FALSE;
            } /* if (pActionsList[j].nEvent == ..) */
            j++;
         } /* while(IFX_TRUE == fContinue) */
         /* phone Config state found - leave for loop */
         break;
      } /* if phone Config state found in ST_rgConfigStates table. */
      i++;
   }
   /* if fContinue value is set to IFX_TRUE then
      pPhone->rgStateMachine[CONFIG_SM].nState was not found in
      ST_rgConfigStates state list */
   if (IFX_TRUE == fContinue)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
         (TD_CONN_ID_INIT == pPhone->nSeqConnId) ?
          TD_CONN_ID_NOT_SET : pPhone->nSeqConnId,
          ("Phone No %d: State %d not found in ST_rgConfig States table."
           "(File: %s, line: %d)\n",
           pPhone->nPhoneNumber, pPhone->rgStateMachine[FXS_SM].nState,
           __FILE__, __LINE__));
      pPhone->rgStateMachine[CONFIG_SM].nState = IE_NONE;
      return IFX_ERROR;
   } /* if (IFX_TRUE == fContinue) */

   return IFX_SUCCESS;
} /* ST_HandleState_Config() */

/* ============================= */
/* Local function definition     */
/* ============================= */

/**
   Action when detected IE_DECT_REGISTER event for S_DECT_NOT_REGISTERED state.
   S_DECT_NOT_REGISTERED is initial state of DECT phone. DECT phone cannot be
   used untill it is registered - IE_DECT_REGISTER is detected.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_DectNotRegistered_Register(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* reset event */
   pPhone->nIntEvent_FXS = IE_NONE;

#ifdef TD_DECT
   /* state only for DECT phones */
   if (PHONE_TYPE_DECT != pPhone->nType)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: invalid state! not a DECT phone "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* set new state */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
#else /* TD_DECT */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, Phone No %d: invalid state! not a DECT phone "
          "(File: %s, line: %d)\n",
          pPhone->nPhoneNumber, __FILE__, __LINE__));
   return IFX_ERROR;
#endif /* TD_DECT */

   return ret;
} /* ST_FXS_Ready_TestTone() */

/**
   Action when detected IE_HOOKOFF event for S_READY state.

   Changes phone state to S_OUT_DIALTONE and plays dialtone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Ready_HookOff(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* set default values in PHONE_t structure */
   pPhone->nDialNrCnt = 0;
   pPhone->nConfIdx = NO_CONFERENCE;
   pPhone->fExtPeerCalled = IFX_FALSE;
   pPhone->fPCM_PeerCalled = IFX_FALSE;
   pPhone->nCallDirection = CALL_DIRECTION_TX;
   pPhone->nConnCnt = 0;
   /* reset pPhone->pConnFromLocal used for conference */
   pPhone->pConnFromLocal = IFX_NULL;
   pPhone->pFXO = IFX_NULL;
   /* reset event */
   pPhone->nIntEvent_FXS = IE_NONE;

   /* reset connection type */
   pConn->nType = UNKNOWN_CALL_TYPE;
#ifdef EASY336
   pConn->voipID = SVIP_RM_UNUSED;
#endif /* EASY336 */

   /* if Ground Start support is enabled then the IFX_TAPI_EVENT_FAULT_LINE_GK_LOW
      must be received before off-hook. */
   if (pCtrl->pProgramArg->oArgFlags.nGroundStart)
   {
      if (!pPhone->fHookOff_Allowed)
      {
         pPhone->nIntEvent_FXS = IE_NONE;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, Phone No %d: Off-Hook detected before IFX_TAPI_EVENT_FAULT_LINE_GK_LOW "
               "(File: %s, line: %d)\n",
               pPhone->nPhoneNumber, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } else
   /* if Ground Start support is enabled then the line is set in active
      when IFX_TAPI_EVENT_FAULT_LINE_GK_LOW event is received. */
   {
      /* Set line in active */
      ret = Common_LineFeedSet(pPhone, IFX_TAPI_LINE_FEED_ACTIVE);
   }

#if FAX_MANAGER_ENABLED
   printf("Feeding the Dial tone\n");
#endif
   /* set new state */
   pPhone->rgStateMachine[FXS_SM].nState = S_OUT_DIALTONE;

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      COM_MOD_DIGIT_DTMF_SVIP_TONE_DET_ENABLE_IF(pPhone)
      else
#endif /* EASY336 */
      {
#ifdef EASY336
         /* get signal detector and generator  */
         resources.nType = RES_DET | RES_GEN;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         if (Common_ResourcesGet(pPhone, &resources) != IFX_SUCCESS)
         {
            pPhone->rgStateMachine[FXS_SM].nState = S_READY;
            return IFX_ERROR;
         }
#endif /* EASY336 */
         Common_DTMF_Enable(pCtrl, pPhone);
      }
   }
   else /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */
   {
#ifndef TAPI_VERSION4
      /* enable the DTMF detection */
      Common_DTMF_SIG_Enable(pCtrl, pPhone);
      /* enable LEC */
      Common_LEC_Enable(pCtrl, pPhone);
#endif /* TAPI_VERSION4 */
   }

   /* send message for config detection test */
   COM_SEND_IDENTIFICATION(pPhone, IFX_NULL);
#if defined(TAPI_VERSION4) && defined(EASY336)
   /* If it's a test of DTMF digits... */
   COM_MOD_DIGIT_DTMF_SVIP_RETURN_FROM_FUNCTION(ret);
#endif /* defined(TAPI_VERSION4) && defined(EASY336) */
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* do not play dialtone for DECT phone - wait for timeout */
      ret = TD_DECT_DialtoneTimeoutAdd(pCtrl, pPhone);
   }
   else
#endif /* TD_DECT */
   {
      /* play dialtone */
	   /*Playing Dial tone*/
      TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, DIALTONE_IDX, "Dialtone");
   }

   return ret;
} /* ST_FXS_Ready_HookOff() */

/**
   Action when detected IE_FAULT_LINE_GK_LOW event for S_READY state.
   Changes line feeding mode to ACTIVE.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Ready_FaultLineGkLow(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   pPhone->nIntEvent_FXS = IE_NONE;
   if (pCtrl->pProgramArg->oArgFlags.nGroundStart)
   {
      pPhone->fHookOff_Allowed = IFX_TRUE;
      /* Set line in active */
      ret = Common_LineFeedSet(pPhone, IFX_TAPI_LINE_FEED_ACTIVE);
   }
   return ret;
}

/**
   Action when detected IE_RINGING event for S_READY state.
   Changes phone state to S_IN_RINGING, starts ringing and sends message
   to caller.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Ready_Ringing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_boolean_t fStartRinging = IFX_TRUE;
   IFX_boolean_t fWideBand = IFX_FALSE;

   /* Check for band type (WB/NB) */
   if (TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
   }

   /* set default values in PHONE_t structure */
   pPhone->nDialNrCnt = 0;
   pPhone->nConfIdx = NO_CONFERENCE;
   pPhone->fExtPeerCalled = IFX_FALSE;
   pPhone->fPCM_PeerCalled = IFX_FALSE;
   /* set call direction */
   pPhone->nCallDirection = CALL_DIRECTION_RX;
   pPhone->nConnCnt = 0;
#ifdef EASY336
#ifdef TAPI_VERSION4
   RESOURCES_t resources;
#endif /* TAPI_VERSION4 */
   pConn->voipID = SVIP_RM_UNUSED;
   VOIP_DATA_CH_t *pCodec;
#endif /* EASY336 */


   /* Only start ringing if no CID, because when CID is send
      with IFX_TAPI_CID_TX_SEQ_START, it does everything also
      the ringing. If we then start ringing here, DRV_ERROR will
      occur and CID will not work. */
   if (pCtrl->pProgramArg->oArgFlags.nCID)
   {
#ifndef XT16
      /* check call type */
      if (pConn->nType == FXO_CALL)
      {
         /* check if FXO structure is avaible */
         if (IFX_NULL != pPhone->pFXO)
         {
            /* if CID was sent for this FXO call then
               oCID_Msg.txMode is set to IFX_TAPI_CID_HM_ONHOOK */
            if (IFX_TAPI_CID_HM_ONHOOK == pPhone->pFXO->oCID_Msg.txMode)
            {
               fStartRinging = IFX_FALSE;
            }
         } /* if (IFX_NULL != pPhone->pFXO) */
      } /* if (pConn->nType != FXO_CALL) */
      /* check call type */
      if ((LOCAL_CALL == pConn->nType) || (LOCAL_PCM_CALL == pConn->nType) ||
          (EXTERN_VOIP_CALL == pConn->nType) || (PCM_CALL == pConn->nType) ||
          (LOCAL_BOARD_PCM_CALL == pConn->nType))
#endif
      {
         fStartRinging = IFX_FALSE;
      }
#ifdef TD_DECT
      /* no CID for DECT phones - ringing is needed */
      if (PHONE_TYPE_DECT == pPhone->nType)
      {
         fStartRinging = IFX_TRUE;
      }
#endif /* TD_DECT */
   } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */
   /* if ringing without CID */
   if (IFX_TRUE == fStartRinging)
   {
      /* start ringing */
      if (IFX_SUCCESS != Common_RingingStart(pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Common_RingingStart failed (File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
   }
   /* use CID ringing */
   else if ((EXTERN_VOIP_CALL == pConn->nType) || (PCM_CALL == pConn->nType))
   {
      /* start ringing */
      if (IFX_SUCCESS != CID_FromPeerSend(pPhone, pConn))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, CID_FromPeerSend failed (File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }
   }

   /* Set up timeslots for PCM calls */
   if (pConn->nType == PCM_CALL)
   {
      /* Only master board chooses timeslots for connection,
         slave board receives timeslots from master board
         for PCM loop phone that starts connection gets timeslots */
      if (pCtrl->pProgramArg->oArgFlags.nPCM_Master &&
          !pCtrl->pProgramArg->oArgFlags.nPCM_Loop)
      {
         /* You are master board, so choose timeslot */
         pConn->oPCM.nTimeslot_RX = PCM_GetTimeslots(TS_RX, fWideBand,
                                                     pPhone->nSeqConnId);
         pConn->oPCM.nTimeslot_TX = pConn->oPCM.nTimeslot_RX;
      }
#ifdef TD_USE_EXT_CALL_WORKAROUND_EASY508XX
      /* It is a workaround, to avoid problem with first packet being dropped
         for first external/PCM connection to EASY508xx boards */
      if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, pConn,
                          IE_EXT_CALL_WORKAROUND_EASY508xx, pPhone->nSeqConnId))
      {
         return IFX_ERROR;
      }
#endif /* TD_USE_EXT_CALL_WORKAROUND_EASY508XX */
   }
#ifdef EASY336
   else if (EXTERN_VOIP_CALL == pConn->nType)
   {
      if (pConn->voipID == SVIP_RM_UNUSED)
      {
         /* get codec */
         resources.nType = RES_CODEC | RES_VOIP_ID;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         if (Common_ResourcesGet(pPhone, &resources) != IFX_SUCCESS)
            return IFX_ERROR;
         pConn->voipID = pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
         pCodec = Common_GetCodec(pCtrl, pConn->voipID, pPhone->nSeqConnId);
         if (pCodec != IFX_NULL)
         {
            TD_SOCK_PortSet(&pConn->oUsedAddr, pCodec->nUDP_Port);
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Common_GetCodec failed (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
         }
         ret = SVIP_libDevIP_Get(pCodec->nDev,
                  TD_SOCK_GetAddrIn(&pConn->oUsedAddr));
         if (ret != SVIP_libStatusOk)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, SVIP_libDevIP_Get returned error %d "
                   "(File: %s, line: %d)\n",
                   ret, __FILE__, __LINE__));
         }
      } /* if (pConn->voipID == SVIP_RM_UNUSED) */
   } /* if (nAction != IE_BUSY && != IE_END_CONNECTION && != IE_RINGING */
#endif /* EASY336 */
#ifdef EASY3111
   /* check board type */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType &&
       /* for some call scenarios(conference) resources are already allocated */
       TD_NOT_SET == pPhone->oEasy3111Specific.nOnMainPCM_Ch)
   {
      /* check call type */
      if (LOCAL_CALL == pConn->nType ||
          EXTERN_VOIP_CALL == pConn->nType ||
          PCM_CALL == pConn->nType)
      {
         /* get resources from main board */
         if (IFX_SUCCESS != Easy3111_CallResourcesGet(pPhone, pCtrl, pConn))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: failed to get resources"
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
         }
      } /* check call type */
   } /* if (TYPE_DUSLIC_XT == pPhone->pBoard->nType) */
#endif /* EASY3111 */

   /* notice calling phone that we are in ringback state */
   TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_RINGBACK, pPhone->nSeqConnId);

   /* increase connections number */
   pPhone->nConnCnt++;

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_IN_RINGING;
   pPhone->nIntEvent_FXS = IE_NONE;
   return ret;
} /* ST_FXS_Ready_Ringing() */

/**
   Action when detected IE_TEST_TONE event for S_READY state.
   During modular test (ITM) with playing predefined tones phone
   changes phone state to S_TEST_TONE.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Ready_TestTone(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_TEST_TONE;
   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_Ready_TestTone() */

/**
   Action when detected IE_FAULT_LINE_GK_LOW event when line is in active mode.
   Error pinted.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_xxx_FaultLineGkLow(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   pPhone->nIntEvent_FXS = IE_NONE;
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, Phone No %d: IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END "
          "- Ground key low detected when line mode is active.\n",
          (IFX_int32_t) pPhone->nPhoneNumber));
   return ret;
}

/**
   Action when detected IE_HOOKON event for S_OUT_DIALTONE state.
   Changes phone state to S_READY, stops playing dialtone,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutDialtone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

#if FAX_MANAGER_ENABLED
   printf("Stoping the Dial tone it is hook on state\n");
#endif
   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

#ifdef EASY336
   COM_MOD_DIGIT_DTMF_SVIP_TONE_DET_DISABLE_RETURN(pPhone, ret);
#endif /* EASY336 */

   /* stop dialtone */
   Common_ToneStop(pCtrl, pPhone);

   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
      Common_DTMF_Disable(pCtrl, pPhone);
#ifdef EASY336
      /* release signal detector and generator  */
      resources.nType = RES_DET | RES_GEN;
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   return ret;
} /* ST_FXS_OutDialtone_HookOn() */

/**
   Action when detected IE_DIALING event for S_OUT_DIALTONE state.
   Stops playing dialtone, changes phone state to S_OUT_DIALING
   and forces IE_DIALING event handling.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutDialtone_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* set new state and event reset */
   pPhone->rgStateMachine[FXS_SM].nState = S_OUT_DIALING;
   pPhone->nIntEvent_FXS = IE_DIALING;
   pCtrl->bInternalEventGenerated = IFX_TRUE;

   /* stop dialtone */
   /*Stopping the tone*/
   ret = Common_ToneStop(pCtrl, pPhone);

   return ret;
} /* ST_FXS_OutDialtone_Dialing() */

/**
   Action when detected IE_HOOKON event for S_OUT_DIALING state.
   Changes phone state to S_READY cleans phone structure
   and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutDialing_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

#ifdef EASY336
   /* If it's a test of DTMF digits... */
   COM_MOD_DIGIT_DTMF_SVIP_TONE_DET_DISABLE_RETURN(pPhone, ret);
#endif /* EASY336 */

   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);

#ifndef XT16
   /* stop signal generator */
   ret = Common_ToneStop(pCtrl, pPhone);
#endif /* XT16 */

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
      /* stop DTMF detection */
      Common_DTMF_Disable(pCtrl, pPhone);
#ifdef EASY336
      if (EXTERN_VOIP_CALL == pConn->nType &&
          IFX_FALSE == pConn->fActive)
      {
         resources.nType = RES_DET | RES_GEN | RES_CODEC | RES_VOIP_ID;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         resources.nVoipID_Nr = SVIP_RM_UNUSED;
      }
      else
      {
         /* release signal detector and generator  */
         resources.nType = RES_DET | RES_GEN;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
      }
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   return ret;
} /* ST_FXS_OutDialing_HookOn() */

/**
   Action when detected IE_DIALING event for S_OUT_DIALING state.

   changes phone state to S_OUT_CALLING if dialed number
           is correct and indicates new call. It gets reasources
           needed for this call and sends message to peer,
   phone state S_OUT_DIALING  doesn't change if dialing number isn't
           finished,
   changes phone state to S_ACTIVE if dialed number indicates
           fxo call also gets reasources needed for this call,
   changes phone state to S_BUSYTONE if dialed number is invalid
           or call initialization fails e.g. phone couldn't get resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutDialing_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t called_number = 0;
   IFX_uint32_t nIP_addr = 0;
   CALL_TYPE_t call_type = UNKNOWN_CALL_TYPE;

   /** Called board's IP address */
   IFX_char_t rgCalledIP[TD_ADDRSTRLEN] = {0};

   /* We don't make new call, we just dial next digits for external call */
   if ((EXTERN_VOIP_CALL == pConn->nType ||
       PCM_CALL == pConn->nType) &&
       IFX_FALSE == pConn->fActive &&
       IFX_TRUE == pConn->bExtDialingIn)
   {
      if (IFX_SUCCESS != ST_Dialing_ExtCallDialing(pCtrl, pPhone, pConn))
      {
         /* play busy tone, clean phone structure and change phone state */
         ST_Dialing_Busy(pCtrl, pPhone, pConn);
         pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
      }
      return IFX_SUCCESS;
   }

   /* By default stay in S_OUT_DIALING state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_OUT_DIALING;
   pPhone->nIntEvent_FXS = IE_NONE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("Phone No %d: Dialed numbers count %d and number is %d\n",
       (int)pPhone->nPhoneNumber, (int)pPhone->nDialNrCnt,
       (int)pPhone->pDialedNum[pPhone->nDialNrCnt-1]));
   /* get call type */
   call_type = Common_CheckDialedNum(pPhone, &called_number, &nIP_addr,
                                     rgCalledIP, pCtrl);

   /* if making call print called number */
   if ((call_type != UNKNOWN_CALL_TYPE) && 
       (call_type != NOT_SUPPORTED))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Called number %s - %s\n",
             pPhone->nPhoneNumber, pPhone->pDialedNum,
             Common_Enum2Name(call_type, TD_rgCallTypeName)));
   }

   switch (call_type)
   {
      case NOT_SUPPORTED:
      {
         ST_Dialing_Busy(pCtrl, pPhone, pConn);
         pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
         break;
      }
      case LOCAL_CALL:
      {
         /* start local call */
         ret = ST_Dialing_Dialing_Loc(pCtrl, pPhone, pConn, called_number);
         if (IFX_ERROR == ret)
         {
            /* play busy tone, clean phone structure and change phone state */
            ST_Dialing_Busy(pCtrl, pPhone, pConn);
#if FAX_MANAGER_ENABLED
            printf("Playing Busy tone\n");
#endif
            pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
         }
         else
         {

#if FAX_MANAGER_ENABLED
            printf("In S_OUT_CALLING State \n");
#endif
            /* change phone state */
            pPhone->rgStateMachine[FXS_SM].nState = S_OUT_CALLING;
         }
         return ret;
         break;
      }
      case EXTERN_VOIP_CALL:
      {
#ifdef TD_IPV6_SUPPORT
         if(0 < strlen(rgCalledIP) && 0 == strcmp(rgCalledIP, TD_NO_NAME))
         {
            /* check if number was found in phone book */
            ret = IFX_ERROR;
         }
         else
#endif /* TD_IPV6_SUPPORT */
         {
            /* start external call */
            ret = ST_Dialing_Dialing_Ext(pCtrl, pPhone, pConn,
                     called_number, nIP_addr, rgCalledIP);
         }
         if (IFX_ERROR == ret)
         {
            /* play busy tone, clean phone structure and change phone state */
            ST_Dialing_Busy(pCtrl, pPhone, pConn);

            pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
         }
         return ret;
         break;
      }
      case PCM_CALL:
      {
         /* start local call */
         ret = ST_Dialing_Dialing_PCM(pCtrl, pPhone, pConn,
                                      called_number, nIP_addr);
         if (IFX_ERROR == ret)
         {
            /* play busy tone, clean phone structure and change phone state */
            ST_Dialing_Busy(pCtrl, pPhone, pConn);
            pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
         }
         return ret;
         break;
      }
#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
      case FXO_CALL:
      {
         /* start local call */
         ret = ST_Dialing_Dialing_FXO(pCtrl, pPhone, pConn, called_number);
         if (IFX_ERROR == ret)
         {
            /* play busy tone, clean phone structure and change phone state */
            ST_Dialing_Busy(pCtrl, pPhone, pConn);
            pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
         }
         else
         {
            /* change phone state */
            pPhone->rgStateMachine[FXS_SM].nState = S_ACTIVE;
         }
         return ret;
         break;
      }
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */
      default:
         break;
   } /* switch */

   return ret;
} /* ST_FXS_OutDialing_Dialing() */

/**
   Action when detected IE_EXT_CALL_ESTABLISHED event for S_OUT_DIALING state.

   Changes phone state to S_OUT_CALLING if receive confirmation event from
   second peer. It means dialed number is correct and indicates new call.
   Only for external call.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutDialing_ExtCallEstablished(CTRL_STATUS_t* pCtrl,
                                                  PHONE_t* pPhone,
                                                  CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   /* Dialed external phone number is correct,
   so finish establishing connection */

   pConn->bExtDialingIn = IFX_FALSE;
   TD_TIMEOUT_STOP(pCtrl->nUseTimeoutSocket);

   /* I want the called phone to establish a external call with me */
   if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_RINGING,
                                       pPhone->nSeqConnId))
   {
      pPhone->nConnCnt--;
      return IFX_ERROR;
   }

#ifdef XT16
   Common_DTMF_Disable(pCtrl, pPhone);
#endif
   /* Change phone state */
   pPhone->rgStateMachine[FXS_SM].nState = S_OUT_CALLING;
   pPhone->nCallDirection = CALL_DIRECTION_TX;

   return ret;
} /* ST_FXS_OutDialing_ExtCallEstablished */

/**
   Action when detected IE_EXT_CALL_WRONG_NUM event for S_OUT_DIALING state.

   Changes phone state to S_BUSYTONE if receive event IE_EXT_CALL_WRONG_NUM
   from second peer. It means dialed number is incorrect.
   Only for external call.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutDialing_ExtCallWrongNumber(CTRL_STATUS_t* pCtrl,
                                                  PHONE_t* pPhone,
                                                  CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* Dialed external phone number is wrong,
      so stop connection and play busy tone */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Phone No %d: Invalid external number entered\n",
          (IFX_int32_t) pPhone->nPhoneNumber));

   /* Play busy tone, clean phone structure and change phone state */
   ST_Dialing_Busy(pCtrl, pPhone, pConn);
   pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_OutDialing_ExtCallWrongNumber */

/**
   Action when detected IE_EXT_CALL_NO_ANSWER event for S_OUT_DIALING state.

   changes phone state to S_BUSYTONE if there is no respond
           from second peer. It means dialed number is incorrect.
           Only for external call.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutDialing_ExtCallNoAnswer(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* No answer from called side,
      so stop connection and play busy tone */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
      ("Phone No %d: No answer during %d [ms] from called phone.\n",
       (IFX_int32_t) pPhone->nPhoneNumber, TIMEOUT_FOR_EXTERNAL_CALL));

   /* Play busy tone, clean phone structure and change phone state */
   ST_Dialing_Busy(pCtrl, pPhone, pConn);
   pPhone->nIntEvent_FXS = IE_NONE;
   pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;

   return ret;
} /* ST_FXS_OutDialing_ExtCallNoAnswer */

/**
   Action when detected IE_EXT_CALL_PROCEEDING event for S_OUT_DIALING state.

   It means dialed number is incompleted. Waiting for next digits.
   Only for external call.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark Doesn't change phone state.
*/
IFX_return_t ST_FXS_OutDialing_ExtCallProceeding(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn)
{
   /* Stay in S_OUT_DIALING state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_OUT_DIALING;
   pPhone->nIntEvent_FXS = IE_NONE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("Phone No %d: Dialed number is incomplete\n",
       (int)pPhone->nPhoneNumber));

   pConn->timeStartExtCalling = TD_OS_ElapsedTimeMSecGet(0);

   return IFX_SUCCESS;
} /* ST_FXS_OutDialing_ExtCallProceeding */


/**
   Action when detected IE_HOOKON event for S_OUT_CALLING state.
   Changes phone state to S_READY, cleans phone structure
   and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutCalling_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* if not TAPI_VERSION4 everything is done in TAPIDEMO_ClearPhone() */
   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
      /* stop signal generator */
      Common_ToneStop(pCtrl, pPhone);
      /* stop DTMF detection */
      Common_DTMF_Disable(pCtrl, pPhone);
#ifdef EASY336
      if (pConn->nType == LOCAL_CALL &&
          !pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
      {
         resources.nType = RES_DET | RES_GEN;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
      }
      else
      {
         resources.nType = RES_DET | RES_GEN | RES_CODEC | RES_VOIP_ID;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         resources.nVoipID_Nr = SVIP_RM_UNUSED;
      }
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   return ret;
} /* ST_FXS_OutCalling_HookOn() */

/**
   Action when detected IE_RINGBACK event for S_OUT_CALLING state.
   Changes phone state to S_OUT_RINGBACK and plays ringback tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutCalling_Ringback(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_OUT_RINGBACK;
   pPhone->nIntEvent_FXS = IE_NONE;

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType &&
       IFX_TRUE != pPhone->bDectChActivated)
   {
      /* activate connection with DECT handset so ringback can be heard */
      if (IFX_SUCCESS != TD_DECT_PhoneActivate(pPhone, pConn))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, phone No %d:TD_DECT_PhoneActivate.(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
      }
   } /* if (PHONE_TYPE_DECT == pPhone->nType) */
#endif /* TD_DECT */

   /* play ringback tone */
   TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, RINGBACK_TONE_IDX, "Ringback tone");

   return ret;
} /* ST_FXS_OutCalling_Ringback() */

/**
   Action when detected IE_BUSY event for S_OUT_CALLING state.
   Changes phone status to S_BUSYTONE and plays busy tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutCalling_Busy(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
   pPhone->nIntEvent_FXS = IE_NONE;

#ifdef XT16
   pConn = IFX_NULL;
#endif /* XT16 */
   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_BUSY, pPhone->pBoard);

#ifdef TD_DECT
   /* activate DECT so busy tone can be heard in handset */
   if (PHONE_TYPE_DECT == pPhone->nType &&
       IFX_TRUE != pPhone->bDectChActivated)
   {
      /* activate connection with DECT handset so ringback can be heard */
      if (IFX_SUCCESS != TD_DECT_PhoneActivate(pPhone, pConn))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, phone No %d:TD_DECT_PhoneActivate.(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
      }
   } /* if (PHONE_TYPE_DECT == pPhone->nType) */
#endif /* TD_DECT */

   /* play busy tone */
   TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, BUSY_TONE_IDX, "Busy tone");

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      /* release signal detector */
      resources.nType = RES_DET;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   }  /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   return ret;
} /* ST_FXS_OutCalling_Busy() */

/**
   Action when detected IE_HOOKON event for S_OUT_RINGBACK state.
   Changes phone state to S_READY, stops playing ringback tone,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutRingback_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   pPhone->nDialNrCnt = 0;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   pConn = &pPhone->rgoConn[0];
   TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_READY, pPhone->nSeqConnId);

   /* stop ringback tone */
   ret = Common_ToneStop(pCtrl, pPhone);

   TAPIDEMO_ClearPhone(pCtrl, pPhone, IFX_NULL, IE_READY, pPhone->pBoard);

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      Common_DTMF_Disable(pCtrl, pPhone);
      if (pConn->nType == LOCAL_CALL)
      {
         if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
         {
            resources.nType = RES_DET | RES_GEN;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
         }
         else
         {
            resources.nType = RES_DET | RES_GEN | RES_CODEC | RES_VOIP_ID;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            resources.nVoipID_Nr = SVIP_RM_UNUSED;
         } /* if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal) */
      } /* if (pConn->nType == LOCAL_CALL) */
      else
      {
         resources.nType = RES_DET | RES_GEN | RES_CODEC | RES_VOIP_ID;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         resources.nVoipID_Nr = SVIP_RM_UNUSED;
      } /* if (pConn->nType == LOCAL_CALL) */
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   return ret;
} /* ST_FXS_OutRingback_HookOn() */

/**
   Action when detected IE_ACTIVE event for S_OUT_RINGBACK state.
   Changes phone state to S_ACTIVE, stops playing ringback tone,
   starts connection on called phone side.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_OutRingback_Active(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_ACTIVE;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* Connection is active */
   pConn->fActive = IFX_TRUE;

   /* stop ringback tone */
   Common_ToneStop(pCtrl, pPhone);

   /* We called the phone and it pick up, we have connection */
#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      if (pConn->nType == EXTERN_VOIP_CALL &&
          pPhone->connID == SVIP_RM_UNUSED)
      {
         Common_DTMF_Disable(pCtrl, pPhone);
         resources.nType = RES_DET | RES_GEN;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         Common_ResourcesRelease(pPhone, &resources);

         resources.nType = RES_DET | RES_GEN | RES_CONN_ID | RES_LEC;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         Common_ResourcesGet(pPhone, &resources);
         Common_DTMF_Enable(pCtrl, pPhone);
         Common_LEC_Enable(pCtrl, pPhone);
      }
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

#ifndef EASY336
   if ((PCM_CALL == pConn->nType) ||
       (LOCAL_PCM_CALL == pConn->nType) ||
       (LOCAL_BOARD_PCM_CALL == pConn->nType))
   {
      TAPIDEMO_PORT_DATA_t oPortData;
      oPortData.nType = PORT_FXS;
      oPortData.uPort.pPhopne = pPhone;
      PCM_StartConnection(&oPortData, pCtrl->pProgramArg, pPhone->pBoard, pConn,
                          pPhone->nSeqConnId);
   }
   else
#endif /* EASY336 */
   {
      /* Start coder */
      if ((LOCAL_CALL == pConn->nType &&
           pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal) ||
          LOCAL_BOARD_CALL == pConn->nType ||
          EXTERN_VOIP_CALL == pConn->nType)
      {
#ifdef TAPI_VERSION4
         if (pPhone->pBoard->fSingleFD)
         {
#ifdef EASY336
            pConn->voipID = pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
#if defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)
            if (TD_FAX_MODE_FDP != pCtrl->nFaxMode)
#endif /* defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW) */
            {
               ret = VOIP_StartCodec(pConn->nUsedCh, pConn,
                        ABSTRACT_GetBoard(pCtrl, IFX_TAPI_DEV_TYPE_VIN_SUPERVIP),
                        pPhone);
               /* check if codec started and printout codec type */
               if (IFX_SUCCESS == ret)
               {
                  Common_PrintEncoderType(pCtrl, pPhone, pConn);
               }
            } /* if (TD_FAX_MODE_FDP != pCtrl->nFaxMode) */
#if defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)
            else
            {
               /* T.38 supported - switch to T.38 */
               ret = SIGNAL_StoreSettings(pPhone, pConn);
               if (IFX_SUCCESS == ret)
               {
                  ret = TD_T38_SetUp(pPhone, pConn);
               }
               /* activate T.38 */
               if (IFX_SUCCESS == ret)
               {
                  ret = TD_T38_Start(pPhone, pConn->nUsedCh);
               }
               if (IFX_SUCCESS == ret)
               {
                  /* set new state */
                  pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38;
               }
            }
#endif /* defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW) */
#endif /* EASY336 */
         } /* if (pPhone->pBoard->fSingleFD) */
         else
#endif /* TAPI_VERSION4 */
         {
#ifndef TAPI_VERSION4
#if (defined(HAVE_T38_IN_FW) && defined(TD_FAX_MODEM) && defined(TD_FAX_T38))
            /* check fax mode */
            if (TD_FAX_MODE_FDP == pCtrl->nFaxMode &&
                EXTERN_VOIP_CALL == pConn->nType)
            {
               /* T.38 supported - switch to T.38 */
               ret = SIGNAL_StoreSettings(pPhone, pConn);
               if (IFX_SUCCESS == ret)
               {
                  ret = TD_T38_SetUp(pPhone, pConn);
               }
               /* activate T.38 */
               if (IFX_SUCCESS == ret)
               {
                  ret = TD_T38_Start(pPhone, pConn->nUsedCh);
               }
               if (IFX_SUCCESS == ret)
               {
                  /* set new state */
                  pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38;
               }
            }
            else
#endif /* (defined(HAVE_T38_IN_FW) && defined(TD_FAX_MODEM) && defined(TD_FAX_T38)) */
            {
               if (IFX_SUCCESS == VOIP_StartCodec(pConn->nUsedCh, pConn,
                                                  pPhone->pBoard, pPhone))
               {
                  Common_PrintEncoderType(pCtrl, pPhone, pConn);
               } /* if */
            }
#endif /* TAPI_VERSION4 */
         } /* if (pPhone->pBoard->fSingleFD) */
#ifdef QOS_SUPPORT
         /* QoS Support */
         if (pCtrl->pProgramArg->oArgFlags.nQos)
         {
/* no QoS for SVIP */
#ifndef EASY336
            QOS_StartSession(pConn, pPhone, TD_SET_ONLY_PORT,
                             pPhone->pBoard->nBoard_IDX);
#endif /* EASY336 */
         }
#endif /* QOS_SUPPORT */
         /* fax modem only avaible for voip calls */
         if (EXTERN_VOIP_CALL == pConn->nType)
         {
            /* TAPI SIGNAL SUPPORT */
            SIGNAL_Setup(pPhone, pConn);
         }
      } /* if ((LOCAL_CALL != pConn->nType) */
#ifndef EASY336
      else if (LOCAL_CALL == pConn->nType)
      {
         Common_MapLocalPhones(pPhone, pConn->oConnPeer.oLocal.pPhone,
                               TD_MAP_ADD);
      } /* else if (LOCAL_CALL == pConn->nType) */
#endif /* EASY336 */
   } /* if (PCM call) */
#ifdef EASY3111
   /* check call type */
   if (LOCAL_CALL == pConn->nType ||
       EXTERN_VOIP_CALL == pConn->nType ||
       PCM_CALL == pConn->nType)
   {
      /* check if EASY 3111 is extension for vmmc board */
      if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
      {
         /* activate connection to main */
         if (IFX_SUCCESS != Easy3111_CallPCM_Activate(pPhone, pCtrl))
         {
            /* print error trace */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: Failed to activate connection to main.\n"
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
         } /* activate connection to main */
      } /* check if EASY 3111 is extension for vmmc board */
   } /* check call type */
#endif /* EASY3111 */
   COM_ITM_TRACES(pCtrl, pPhone, pConn, IFX_NULL, TTP_RINGBACK_ACTIVE,
                  pPhone->nSeqConnId);

   return ret;
} /* ST_FXS_OutRingback_Active() */

/**
   Action when detected IE_HOOKON event for S_ACTIVE state.
   Changes phone state to S_READY, communicates with other peer,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Active_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* end connection - release resources, turn of lec, send message to peer */
   ret = ST_Active_HookOn(pCtrl, pPhone, pConn);

   return ret;
} /* ST_FXS_Active_HookOn() */

/**
   Action when detected IE_DIALING event for S_ACIVE or S_CF_ACTIVE state.
   Plays dialed digit tone on data channel mapped to PCM/ALM channel.
   Used only for FXO connection made with DECT handset.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Active_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TD_DECT
   IFX_int32_t nDataCh_FD;

   /* check if DECT phone in FXO call */
   if (PHONE_TYPE_DECT == pPhone->nType &&
       CALL_DIRECTION_TX == pPhone->nCallDirection &&
       IFX_TRUE == pPhone->fFXO_Call)
   {
      if (IFX_NULL == pPhone->pFXO)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, Phone No %d: Fxo pointer not set for FXO call."
               "(File: %s, line: %d)\n",
               pPhone->nPhoneNumber, __FILE__, __LINE__));
      }
      /* only one digit can be handled */
      else if ((1 == pPhone->nDialNrCnt) && ('\0' != pPhone->pDialedNum[0]))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: FXO call: Play tone %d on data ch %d\n",
                pPhone->nPhoneNumber, pPhone->pDialedNum[0],
                pPhone->pFXO->nDataCh));
         /* get FD */
         nDataCh_FD = VOIP_GetFD_OfCh(pPhone->pFXO->nDataCh, pPhone->pBoard);
         if (NO_DEVICE_FD == nDataCh_FD)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                 ("Err, %s failed to get FD of data ch %d."
                  "(File: %s, line: %d)\n",
                  pPhone->pBoard->pszBoardName, pPhone->pFXO->nDataCh,
                  __FILE__, __LINE__));
         }
         else
         {
            ret = TD_IOCTL(nDataCh_FD, IFX_TAPI_TONE_LOCAL_PLAY,
                     pPhone->pDialedNum[0], TD_DEV_NOT_SET, pPhone->nSeqConnId);
         }

      } /* if ((1 == pPhone->nDialNrCnt) && ('\0' != pPhone->pDialedNum[0])) */
      /* dialed digit was handled - reset buffer */
      pPhone->nDialNrCnt = 0;
      pPhone->pDialedNum[0] = '\0';
      pPhone->pDialedNum[1] = '\0';
   } /* if (dect handset in FXO call) */
#endif /* TD_DECT */

   COM_ST_FXS_ACTIVE_DIALING_HANDLE(pPhone);

   /* set new state and reset event */
   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_Active_Dialing() */

/**
   Action when detected IE_CONFERENCE event for S_ACTIVE state.
   If first connection is valid changes phone state to S_CF_DIALTONE,
      starts playing dialtone and starts conference.
   If first connection is invalid stays in S_ACTIVE state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark valid connections are LOCAL_CALL, EXTERN_VOIP_CALL and PCM_CALL
               also phone is call starter - caller
*/
IFX_return_t ST_FXS_Active_Conference(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_ERROR;

   /* by default stay in old state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_ACTIVE;
   pPhone->nIntEvent_FXS = IE_NONE;

   if (CALL_DIRECTION_TX != pPhone->nCallDirection)
   {
      /* only caller can start conference, do nothing */
      return IFX_SUCCESS;
   }
#ifdef FXO
   /* check if FXO connected to phone */
   if (IFX_NULL != pPhone->pFXO)
   {
      /* do flash-hook to prevent conference starting on called board */
      if (IFX_SUCCESS != FXO_FlashHook(pPhone->pFXO))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: Flash-hook on FXO No %d failed.\n"
                "(File: %s, line: %d)\n",
                (IFX_int32_t) pPhone->nPhoneNumber, pPhone->pFXO->nFXO_Number,
                __FILE__, __LINE__));
      }
   }
#endif /* FXO */
   /* check if first connection is valid */
   if ((EXTERN_VOIP_CALL != pPhone->rgoConn[0].nType) &&
       (LOCAL_CALL != pPhone->rgoConn[0].nType) &&
       (PCM_CALL != pPhone->rgoConn[0].nType) &&
       (FXO_CALL != pPhone->rgoConn[0].nType))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Unable to start conference first call must be:"
             "%sLOCAL_CALL, EXTERN_VOIP_CALL, PCM_CALL or FXO_CALL.\n"
             "%sThis is the %s.\n"
             "(File: %s, line: %d)\n",
             (IFX_int32_t) pPhone->nPhoneNumber,
             pPhone->pBoard->pIndentPhone,
             pPhone->pBoard->pIndentPhone,
             Common_Enum2Name(pPhone->rgoConn[0].nType, TD_rgCallTypeName),
             __FILE__, __LINE__));
      return IFX_SUCCESS;
   }
   /* start conference - check if conference can be started
      and get conference IDX */
   else if (IFX_ERROR == CONFERENCE_Start(pCtrl, pPhone))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Failed to start conference.(File: %s, line: %d)\n",
             (IFX_int32_t) pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else
   {
      /* conference started - set new state,
         new peer phone number can be dialed */
      pPhone->rgStateMachine[FXS_SM].nState = S_CF_DIALTONE;
      /* reset dialed number count */
      pPhone->nDialNrCnt = 0;
   }

   /* start dialtone */
   TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, DIALTONE_IDX, "Dialtone");

   return ret;
} /* ST_FXS_Active_Conference() */

/**
   Action when detected IE_END_CONNECTION event for S_ACTIVE state.
   Changes phone status to S_BUSYTONE and plays busy tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Active_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* end connection - release resources, play busy tone etc.. */
   ret = ST_Active_EndConnection(pCtrl, pPhone, pConn);

   return ret;
} /* ST_FXS_Active_EndConnection() */

/**
   Action when detected FAX/MODEM events for S_ACTIVE state.
   Changes phone state to S_FAX_MODEM and enables fax/modem transmition.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Active_FaxModXxx(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#ifdef TD_FAX_MODEM
   /* FAX/modem actions are only alowed for EXTERN_VOIP_CALL */
   TD_FAX_MODEM_CHECK_CALL_TYPE(pPhone, pConn);

   IFX_TAPI_SIG_DETECTION_t stSignal = {0};
   /* disable signal detection of signals that shouldn't be used in this
      Fax/Modem transmition */
   stSignal.sig = TD_SIGNAL_LIST;
   stSignal.sig_ext = TD_SIGNAL_LIST_EXT;

#if defined(HAVE_T38_IN_FW) && defined(TD_FAX_T38)
   if ((!pPhone->pBoard->nT38_Support) ||
       (TD_FAX_MODE_TRANSPARENT == pCtrl->nFaxMode))
#endif /* defined(HAVE_T38_IN_FW) && defined(TD_FAX_T38) */
   {
      stSignal.sig |= IFX_TAPI_SIG_DIS;
   }


   ret = SIGNAL_DisableDetection(pConn, &stSignal, pPhone);

   if (IFX_SUCCESS == ret)
   {
      /* update information about signals which are currently enabled */
      pPhone->stSignal.sig &= ~stSignal.sig;
      pPhone->stSignal.sig_ext &= ~stSignal.sig_ext;
      /* store current configuration */
      ret = SIGNAL_StoreSettings(pPhone, pConn);
   }

   if (IFX_SUCCESS == ret)
   {
      /* clear channel */
      ret = SIGNAL_ClearChannel(pPhone, pConn);
   }

   if (IFX_SUCCESS == ret)
   {
      ret = SIGNAL_DisableNLP(pPhone, pCtrl);
   }

   if (IFX_SUCCESS == ret)
   {
      stSignal.sig = IFX_TAPI_SIG_TONEHOLDING_END;
      stSignal.sig_ext = 0;
      ret = SIGNAL_EnableDetection(pConn, &stSignal, pPhone);
      if (IFX_SUCCESS == ret)
      {
         /* update information about signals which are currently enabled */
         pPhone->stSignal.sig |= stSignal.sig;
      }
   }
   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_FAX_MODEM;

   COM_ITM_TRACES(pCtrl, pPhone, pConn, IFX_NULL, TTP_SET_S_FAX_MODEM,
                  pPhone->nSeqConnId);
#endif /* TD_FAX_MODEM */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_Active_FaxModXxx() */

/**
   Action when detected IE_DIS event for S_ACTIVE state.
   Changes phone state to S_FAXT38_REQ and enables fax transmition.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Active_DIS(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                               CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#ifdef TD_FAX_MODEM
   /* FAX/modem actions are only alowed for EXTERN_VOIP_CALL */
   TD_FAX_MODEM_CHECK_CALL_TYPE(pPhone, pConn);

   /* currently only AR9, VR9, xRX300 and SVIP support FAX T.38 stack. */
#if defined(HAVE_T38_IN_FW) && defined(TD_FAX_T38)
   if ((pPhone->pBoard->nT38_Support) &&
       (TD_FAX_MODE_DEAFAULT == pCtrl->nFaxMode))
   {
      /* T.38 supported - switch to T.38 */
      ret = SIGNAL_StoreSettings(pPhone, pConn);
      if (IFX_SUCCESS == ret)
      {
         ret = TD_T38_SetUp(pPhone, pConn);
      }

      if (IFX_SUCCESS == ret)
      {
         /* Send T.38 request to the peer. */
         ret = TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_T38_REQ,
                                  pPhone->nSeqConnId);
      }

      /* clear information about peer. */
      pConn->bT38_PeerEndTransm = IFX_FALSE;
      /* set new state and reset event */
      pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38_REQ;
   }
   else
#endif /* defined(HAVE_T38_IN_FW) && defined(TD_FAX_T38) */
   {
      /* handle as any other fax modem signal */
      ST_FXS_Active_FaxModXxx(pCtrl, pPhone, pConn);
   }
#endif /* TD_FAX_MODEM */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_Active_DIS() */

/**
   Action when detected IE_T38_REQ event for S_ACTIVE state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
    Peer requested T.38 transmisison. Send IE_T38_ACK and start T.38.
*/
IFX_return_t ST_FXS_Active_T38Req(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /* FAX/modem actions are only alowed for EXTERN_VOIP_CALL */
   TD_FAX_MODEM_CHECK_CALL_TYPE(pPhone, pConn);

   /* currently only AR9, VR9, xRX300 and SVIP support FAX T.38 stack. */
   if ((pPhone->pBoard->nT38_Support) &&
       (TD_FAX_MODE_DEAFAULT == pCtrl->nFaxMode))
   {
      /* T.38 supported - switch to T.38 */
      ret = SIGNAL_StoreSettings(pPhone, pConn);
      if (IFX_SUCCESS == ret)
      {
         ret = TD_T38_SetUp(pPhone, pConn);
      }
      /* activate T.38 */
      if (IFX_SUCCESS == ret)
      {
         ret = TD_T38_Start(pPhone, pConn->nUsedCh);
      }

      if (IFX_SUCCESS == ret)
      {
         /* Send ack to the peer. */
         ret = TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_T38_ACK,
                                  pPhone->nSeqConnId);
      }

      if (IFX_SUCCESS == ret)
      {
         /* clear information about peer. */
         pConn->bT38_PeerEndTransm = IFX_FALSE;
         /* set new state and reset event */
         pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38;

         COM_ITM_TRACES(pCtrl, pPhone, pConn, IFX_NULL, TTP_SET_S_FAXT38,
                        pPhone->nSeqConnId);
      }
   }
   else
   {
      /* peer requested T.38 but this board doesn't support it */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Err, Phone No %d: peer requested T.38 but %s doesn't support it. "
            "(File: %s, line: %d)\n",
            pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName,
            __FILE__, __LINE__));
   }
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_Active_T38Req() */

/**
   Action when detected IE_HOOKON event for S_BUSYTONE state.
   Changes phone state to S_READY, stops playing busy tone,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_Busytone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* use first connection */
   pConn = &pPhone->rgoConn[0];

   /* stop busy tone - signal generator */
   ret = Common_ToneStop(pCtrl, pPhone);

   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);


#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      resources.nType = RES_GEN;
      if (pConn->nType == EXTERN_VOIP_CALL ||
         (pConn->nType == LOCAL_CALL &&
            pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal))
      {
         resources.nType |= RES_CODEC | RES_VOIP_ID;
      }
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      resources.nVoipID_Nr = SVIP_RM_UNUSED;
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   /* reset connection type */
   pConn->nType = UNKNOWN_CALL_TYPE;

   return ret;
} /* ST_FXS_Busytone_HookOn() */

/**
   Action when detected IE_HOOKON event for S_IN_RINGING state.
   Changes phone state to S_READY, stops phone ringing,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_InRinging_Ready(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   if (pCtrl->pProgramArg->oArgFlags.nPCM_Master)
   {
      /* Free timeslots for PCM calls */
      if (pConn->nType == PCM_CALL)
      {
         /* You are master board, so choose timeslot */
         PCM_FreeTimeslots(pConn->oPCM.nTimeslot_RX, pPhone->nSeqConnId);
      }
   }

#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) && !defined(EASY336) && !defined(XT16)
   if (pCtrl->pProgramArg->oArgFlags.nFXO)
   {
      if (pConn->nType == FXO_CALL)
      {
         /* Check if FXO connection is still in progress */
         if (pPhone->pFXO != IFX_NULL)
         {
            if (S_READY != pPhone->pFXO->nState)
            {
               TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_NONE,
                                   pPhone->pBoard);
            }
            else
            {
               /* reset FXO data - FXO call was ended for other phone state
                  transition*/
               pPhone->fFXO_Call = IFX_FALSE;
               pPhone->pFXO = IFX_NULL;
               pConn->nType = UNKNOWN_CALL_TYPE;
            }
         }
      }  /* if (pConn->nType == FXO_CALL) */
   } /* if (pCtrl->pProgramArg->oArgFlags.nFXO) */
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) && !defined(EASY336) && !defined(XT16) */

#ifdef TAPI_VERSION4
#if (defined(XT16) || defined(EASY336))
   if (pCtrl->pProgramArg->oArgFlags.nCID)
   {
      Common_CID_Disable(pPhone);
   }
#endif /* (defined(XT16) || defined(EASY336)) */
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      if (pConn->nType == EXTERN_VOIP_CALL)
      {
         resources.nType = RES_CODEC | RES_VOIP_ID;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         resources.nVoipID_Nr = SVIP_RM_UNUSED;
         Common_ResourcesRelease(pPhone, &resources);
      }
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif
   /* check if ringing stopped */
   if (IFX_SUCCESS != Common_RingingStop(pPhone))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, stop ringing failed (File: %s, line: %d)\n",
             __FILE__, __LINE__));
   }
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* use DECT phone state transition function */
      if (IFX_SUCCESS != TD_DECT_ST_InRinging_Ready(pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Phone No %d: TD_DECT_ST_InRinging_Ready failed\n"
                " (File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
      }
   }
#endif /* TD_DECT */

   TAPIDEMO_ClearPhone(pCtrl, pPhone, IFX_NULL, IE_READY, pPhone->pBoard);

   /* reset phone structure data */
   pPhone->nConnCnt = 0;
   pPhone->nCallDirection = CALL_DIRECTION_NONE;

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_InRinging_Ready() */

/**
   Action when detected IE_HOOKOFF event for S_IN_RINGING state.
   Changes phone state to S_ACTIVE initializes phone and connection
   on called side.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_InRinging_HookOff(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
   VOIP_DATA_CH_t *pOurCodec;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
#ifdef EASY336
   PHONE_t** pPhones;
   IFX_int32_t nPhones, i, j;
#endif /* EASY336 */
   BOARD_t* pBoard = pPhone->pBoard;
   /* reset event */
   pPhone->nIntEvent_FXS = IE_NONE;

   /* check if ringing stopped */
   ret = Common_RingingStop(pPhone);
   if (IFX_SUCCESS != ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Err on Common_RingingStop.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* activate connection with DECT handset */
      if (IFX_SUCCESS != TD_DECT_PhoneActivate(pPhone, pConn))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, phone No %d: TD_DECT_PhoneActivate.(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
#endif /* TD_DECT */
   /* We were called from somebody and pick up the phone, connection is made */
#ifdef TAPI_VERSION4
   if (!pPhone->pBoard->fSingleFD)
#endif /* TAPI_VERSION4 */
   {
#ifndef TAPI_VERSION4
      /* enable the DTMF detection */
      if (IFX_SUCCESS != Common_DTMF_SIG_Enable(pCtrl, pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: Err on Common_DTMF_SIG_Enable().\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* enable LEC */
      if (IFX_SUCCESS != Common_LEC_Enable(pCtrl, pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: Err on Common_LEC_Enable.\n"
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
         return IFX_ERROR;
      }
#endif /* TAPI_VERSION4 */
   } /* if (pPhone->pBoard->fSingleFD) */

   /* Set line in active */
   if (IFX_SUCCESS != Common_LineFeedSet(pPhone, IFX_TAPI_LINE_FEED_ACTIVE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Err on Common_LineFeedSet.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if ((PCM_CALL == pConn->nType) ||
       (LOCAL_PCM_CALL == pConn->nType) ||
       (LOCAL_BOARD_PCM_CALL == pConn->nType))
   {
      if ((pCtrl->pProgramArg->oArgFlags.nPCM_Master) ||
          (pCtrl->pProgramArg->oArgFlags.nPCM_Slave))
      {
         TAPIDEMO_PORT_DATA_t oPortData;

#if (defined(XT16) || defined(EASY336))
         if (pCtrl->pProgramArg->oArgFlags.nCID)
         {
            if (IFX_SUCCESS != Common_CID_Disable(pPhone))
            {
#ifndef EASY336
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Phone No %d: Err on Common_CID_Disable.\n"
                      "(File: %s, line: %d)\n",
                      pPhone->nPhoneNumber, __FILE__, __LINE__));
               return IFX_ERROR;
#endif /* EASY336 */
            }
         }

#endif /* (defined(XT16) || defined(EASY336)) */
         oPortData.nType = PORT_FXS;
         oPortData.uPort.pPhopne = pPhone;
         if (IFX_FALSE == PCM_StartConnection(&oPortData, pCtrl->pProgramArg,
                                              pBoard, pConn, pPhone->nSeqConnId))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Phone No %d: Err on PCM_StartConnection.\n"
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
            return IFX_ERROR;
         }
#if (!defined(EASY3201) && !defined(EASY3201_EVS) && !defined(XT16))
#ifdef EASY3111
         /* for this board type mapping is not needed */
         if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
#endif /* EASY3111 */
         {
            /* Map pcm channel with phone channel of initiator */
            if (IFX_SUCCESS != PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh,
                                  IFX_TRUE, pBoard, pPhone->nSeqConnId))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Phone No %d: Err on PCM_MapToPhone.\n"
                      "(File: %s, line: %d)\n",
                      pPhone->nPhoneNumber, __FILE__, __LINE__));
               return IFX_ERROR;
            }
         }
#endif /* EASY3201 && XT16*/
      } /* if */
   } /* if ((PCM_CALL == pConn->nType)... */
#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
   else if (pConn->nType == FXO_CALL)
   {
      /* FXO call */
      if (pCtrl->pProgramArg->oArgFlags.nFXO)
      {
         IFX_int32_t i;
         /* Stop ringing on all other phones. */
         for (i = 0; i < pBoard->nMaxPhones; i++)
         {
            /* check if not the phone that went off-hook
               and if ringing because of FXO_CALL */
            if ((pPhone != &pBoard->rgoPhones[i]) &&
                (S_IN_RINGING == pBoard->rgoPhones[i].rgStateMachine[FXS_SM].nState) &&
                (FXO_CALL == pBoard->rgoPhones[i].rgoConn[0].nType))
            {
               pBoard->rgoPhones[i].nIntEvent_FXS = IE_READY;
               pBoard->rgoPhones[i].rgoConn[0].nType = UNKNOWN_CALL_TYPE;
               pCtrl->bInternalEventGenerated = IFX_TRUE;
            }
         }
         if (pPhone->pFXO == IFX_NULL)
         {
            /* should not happend */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: FXO pointer not set for FXO call. "
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
            return IFX_ERROR;
         }
#ifdef EASY3111
         /* check board type */
         if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
         {
            /* get resources needed to establish connection to main */
            if (IFX_SUCCESS != Easy3111_CallResourcesGet(pPhone, pCtrl, pConn))
            {
               /* should not happend */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, Phone No %d: Easy3111_CallResourcesGet() failed "
                      "(File: %s, line: %d)\n",
                      pPhone->nPhoneNumber, __FILE__, __LINE__));
               return IFX_ERROR;
            }
            if (IFX_SUCCESS != Easy3111_CallPCM_Activate(pPhone, pCtrl))
            {
               /* should not happend */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, Phone No %d: Easy3111_CallPCM_Activate() failed "
                      "(File: %s, line: %d)\n",
                      pPhone->nPhoneNumber, __FILE__, __LINE__));
               Easy3111_CallResourcesRelease(pPhone, pCtrl, pConn);
               return IFX_ERROR;
            }
         }
#endif /* EASY3111 */
         /* Setup correct pcm channel that is used in FXO */
         pConn->nUsedCh = pPhone->pFXO->oConn.nUsedCh;
         pConn->nUsedCh_FD = PCM_GetFD_OfCh(pConn->nUsedCh,
                                            TD_GET_MAIN_BOARD(pCtrl));

         if (IFX_SUCCESS != FXO_ActivateConnection(pPhone->pFXO, pPhone,
                                                   pConn, pBoard))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Phone No %d: Err on FXO_ActivateConnection.\n"
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
#ifdef EASY3111
            if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
            {
               Easy3111_CallPCM_Deactivate(pPhone, pCtrl);
               Easy3111_CallResourcesRelease(pPhone, pCtrl, pConn);
            }
#endif /* EASY3111 */
            return IFX_ERROR;
         }

         if (pPhone->pFXO->fCID_FSK_Started)
         {
            if (IFX_SUCCESS != CID_StopFSK(pPhone->pFXO,
                                  VOIP_GetFD_OfCh(pPhone->pFXO->nDataCh, pBoard),
                                  pPhone->pFXO->nDataCh))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Phone No %d: Err on CID_StopFSK.\n"
                      "(File: %s, line: %d)\n",
                      pPhone->nPhoneNumber, __FILE__, __LINE__));
               return IFX_ERROR;
            }

         }
         pPhone->pFXO->fCID_FSK_Started = IFX_FALSE;
      }
   } /* else if (pConn->nType == FXO_CALL) */
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */
   else
   {
      /* Ordinary call */
#ifdef EASY336
      if (pConn->nType == LOCAL_CALL)
      {
         if (pCtrl->pProgramArg->oArgFlags.nCID)
         {
            /* disabele cid detection - release cid resource,
               should be done when IFX_TAPI_EVENT_CID_TX_SEQ_END is detected,
               but if this event doesn't occur next loacl call will fail
               if CID is not disabled */
            if (IFX_SUCCESS != Common_CID_Disable(pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_CID_Disable.\n(File: %s, line: %d)\n",
                       __FILE__, __LINE__));
               return IFX_ERROR;
            }
         }
         /* caller and connected phones' resource release */
         if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
         {

            if (IFX_SUCCESS != Common_ToneStop(pCtrl,
                                               pConn->oConnPeer.oLocal.pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ToneStop.\n(File: %s, line: %d)\n",
                       __FILE__, __LINE__));
               return IFX_ERROR;
            }


            if (IFX_SUCCESS != Common_DTMF_Disable(pCtrl,
                                                   pConn->oConnPeer.oLocal.pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                       __FILE__, __LINE__));
               return IFX_ERROR;
            }

            resources.nType = RES_DET | RES_GEN;
            if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                       &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                       __FILE__, __LINE__));

               /* Return to state before hooking off. Or there will be more errors.*/
               if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                      pConn->oConnPeer.oLocal.pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }

               return IFX_ERROR;
            }

            /* connID assigned to caller? */
            if (pConn->oConnPeer.oLocal.pPhone->connID != SVIP_RM_UNUSED)
            {
               /* if connID is assigned to caller, then caller is
                * already in call and we are creating a conference */

               /* get array of phones that are connected to the
                * caller, including caller itself */
               nPhones = Common_ConnID_PhonesGet(pCtrl,
                            pConn->oConnPeer.oLocal.pPhone->connID, &pPhones);
               if (nPhones > 0)
               {
                  /* release LEC on all connected
                     phones, including caller */
                  for (i = 0; i < nPhones; i++)
                  {
                     /* stop LEC */
                     if (IFX_SUCCESS != Common_LEC_Disable(pCtrl, pPhones[i]))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_LEC_Disable.\n"
                               "(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                        TD_OS_MemFree(pPhones);

                        /* Return to state before hooking off. Or there will be more errors.*/
                        /* If we don't get these resources, than caller phone stops working */
                        /* Even after hook-on */
                        resources.nConnID_Nr = SVIP_RM_UNUSED;
                        resources.nType = RES_DET | RES_GEN;
                        if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                               &resources))
                        {
                           TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                                 ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                   __FILE__, __LINE__));
                        }
                        if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                              pConn->oConnPeer.oLocal.pPhone))
                        {
                           TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                                 ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                                   __FILE__, __LINE__));
                        }

                        return IFX_ERROR;
                     }
                     /* release LEC */
                     resources.nType = RES_LEC;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pPhones[i],
                                                                &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n"
                               "(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                        TD_OS_MemFree(pPhones);

                        /* Return to state before hooking off. Or there will be more errors.*/
                        /* If we don't get these resources, than caller phone stops working */
                        /* Even after hook-on */
                        resources.nConnID_Nr = SVIP_RM_UNUSED;
                        resources.nType = RES_DET | RES_GEN;
                        if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                               &resources))
                        {
                           TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                                 ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                   __FILE__, __LINE__));
                        }
                        if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                              pConn->oConnPeer.oLocal.pPhone))
                        {
                           TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                                 ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                                   __FILE__, __LINE__));
                        }

                        return IFX_ERROR;
                     }
                  } /* for cycle on phones*/
                  TD_OS_MemFree(pPhones);
               } /* if (nPhones > 0) */
            } /* if connID is assigned to caller */
            else
            {
               /* N */
               /* get a connID for caller */
               resources.nType = RES_CONN_ID;
               resources.nConnID_Nr = SVIP_RM_UNUSED;
               if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                       &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));

                  /* Return to state before hooking off. Or there will be more errors.*/
                  /* If we don't get these resources, than caller phone stops working */
                  /* Even after hook-on */
                  resources.nConnID_Nr = SVIP_RM_UNUSED;
                  resources.nType = RES_DET | RES_GEN;
                  if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                         &resources))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }
                  if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                        pConn->oConnPeer.oLocal.pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }

                  return IFX_ERROR;
               }
            } /* if connID is assigned to caller */

            /* all REBs have been freed, connID assigned to
             * caller */
            /* now connect caller with callee */
            resources.nType = RES_NONE;
            resources.nConnID_Nr = pConn->oConnPeer.oLocal.pPhone->connID;
            if (IFX_SUCCESS != Common_ResourcesGet(pPhone, &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Return to state before hooking off. */
               /* Release resources which have been allocated in this transition */
               resources.nType = RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                          &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               resources.nConnID_Nr = SVIP_RM_UNUSED;
               resources.nType = RES_DET | RES_GEN;
               if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                          &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }
               if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                      pConn->oConnPeer.oLocal.pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }

               return IFX_ERROR;
            }

            /* all phones connected, get LECs */
            nPhones = Common_ConnID_PhonesGet(pCtrl, pPhone->connID, &pPhones);
            if (nPhones > 0)
            {
               /* allocate LEC on all connected
                  phones, including callee */
               for (i = 0; i < nPhones; i++)
               {
                  resources.nType = RES_LEC;
                  resources.nConnID_Nr = SVIP_RM_UNUSED;
                  if (IFX_SUCCESS != Common_ResourcesGet(pPhones[i], &resources))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));

                     /* Clean-up */
                     TD_OS_MemFree(pPhones);

                     /* Release resources which have been allocated in this transition */
                     resources.nType = RES_CONN_ID;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pPhone,
                                                                &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                     resources.nType = RES_CONN_ID;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                                &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease2.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                     resources.nConnID_Nr = SVIP_RM_UNUSED;
                     resources.nType = RES_DET | RES_GEN;
                     if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                                &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }
                     if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                            pConn->oConnPeer.oLocal.pPhone))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }

                     return IFX_ERROR;
                  }
                  /* Release resources which have been allocated in this transition */
                  /* start LEC */
                  if (IFX_SUCCESS != Common_LEC_Enable(pCtrl, pPhones[i]))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));

                     for (j=0; j <=i; j++)
                     {
                        resources.nType = RES_LEC;
                        if (IFX_SUCCESS != Common_ResourcesRelease(pPhones[j],
                                                                   &resources))
                        {
                           TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                                 ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                  __FILE__, __LINE__));
                        }
                     }
                     /* Clean-up */
                     TD_OS_MemFree(pPhones);
                     resources.nType = RES_CONN_ID;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pPhone,
                                                                &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                     resources.nType = RES_CONN_ID;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                                &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease2.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                     resources.nConnID_Nr = SVIP_RM_UNUSED;
                     resources.nType = RES_DET | RES_GEN;
                     if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                            &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }
                     if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                            pConn->oConnPeer.oLocal.pPhone))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }

                     return IFX_ERROR;
                  }
               } /* for cycle on phones*/
               TD_OS_MemFree(pPhones);
            } /* if (nPhones > 0) */

            /* get sigdet, siggen on caller */
            resources.nType = RES_DET | RES_GEN;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                   &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Release resources which have been allocated in this transition */
               /* all phones connected, release LECs */
               nPhones = Common_ConnID_PhonesGet(pCtrl, pPhone->connID, &pPhones);
               if (nPhones > 0)
               {
                  for (i=0; i < nPhones; i++)
                  {
                     resources.nType = RES_LEC;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pPhones[i],
                                                                &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                  }
                  /* Clean-up */
                  TD_OS_MemFree(pPhones);
               }
               if (IFX_SUCCESS != Common_ResourcesRelease(pPhone,
                                                          &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               resources.nType = RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                          &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease2.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }

               return IFX_ERROR;
            }

            /* start DTMF detection on caller */
            if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                  pConn->oConnPeer.oLocal.pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_DTMF_Enable.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Release resources which have been allocated in this transition */
               /* all phones connected, release LECs */
               nPhones = Common_ConnID_PhonesGet(pCtrl, pPhone->connID, &pPhones);
               if (nPhones > 0)
               {
                  for (i=0; i < nPhones; i++)
                  {
                     resources.nType = RES_LEC;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pPhones[i],
                                                                &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                  }
                  /* Clean-up */
                  TD_OS_MemFree(pPhones);
               }
               resources.nType = RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pPhone,
                                                          &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               resources.nType = RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                          &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease2.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }

               return IFX_ERROR;
            }
         }
         else /* if (!pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal */
         {
            /* use codecs for local calls */
            /* stop tone generation on caller */
            if (IFX_SUCCESS != Common_ToneStop(pCtrl,
                                               pConn->oConnPeer.oLocal.pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ToneStop.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               return IFX_ERROR;
            }

            /* disable DTMF detection on caller */
            if (IFX_SUCCESS != (Common_DTMF_Disable(pCtrl,
                                                    pConn->oConnPeer.oLocal.pPhone)))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               return IFX_ERROR;
            }

            /* release sigdet, siggen on caller */
            resources.nType = RES_DET | RES_GEN;
            if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                       &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Return to state before hooking off. Or there will be more errors.*/
               if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                      pConn->oConnPeer.oLocal.pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }

               return IFX_ERROR;
            }


            if (pConn->oConnPeer.oLocal.pPhone->connID == SVIP_RM_UNUSED)
            {
               /* if connID is assigned to caller, then caller is
                * already in call and we are creating a conference */

               /* get connID on caller */
               resources.nType = RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                      &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));

                  /* Return to state before hooking off. Or there will be more errors.*/
                  /* If we don't get these resources, than caller phone stops working */
                  /* Even after hook-on */
                  resources.nConnID_Nr = SVIP_RM_UNUSED;
                  resources.nType = RES_DET | RES_GEN;
                  if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                         &resources))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }
                  if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                        pConn->oConnPeer.oLocal.pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }

                  return IFX_ERROR;
               }

               resources.nType = RES_LEC;
               resources.nConnID_Nr = SVIP_RM_UNUSED;
               if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                      &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));

                     /* Return to state before hooking off. Or there will be more errors.*/
                     resources.nType = RES_CONN_ID;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                                &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }
                     if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                           pConn->oConnPeer.oLocal.pPhone))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }

                  return IFX_ERROR;
               }
            }

            /* get sigdet, siggen, LEC on caller */
            resources.nType = RES_DET | RES_GEN;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            if (IFX_SUCCESS != Common_ResourcesGet(pConn->oConnPeer.oLocal.pPhone,
                                                   &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

                  /* Return to state before hooking off. Or there will be more errors.*/
                  resources.nType = RES_CONN_ID | RES_LEC;
                  if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                             &resources))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }
                  if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                        pConn->oConnPeer.oLocal.pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }

               return IFX_ERROR;
            }

            /* start DTMF detection on caller */
            if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl,
                                                  pConn->oConnPeer.oLocal.pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_DTMF_Enable.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Return to state before hooking off. Or there will be more errors.*/
               resources.nType = RES_LEC | RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                      &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }
               return IFX_ERROR;
            }

            /* start LEC on caller */
            if (IFX_SUCCESS != Common_LEC_Enable(pCtrl,
                                                 pConn->oConnPeer.oLocal.pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Return to state before hooking off. Or there will be more errors.*/
               resources.nType = RES_LEC | RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                      &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }

               return IFX_ERROR;
            }

            /* get connID on callee */
            resources.nType = RES_CONN_ID;
            if (IFX_SUCCESS != Common_ResourcesGet(pPhone, &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Return to state before hooking off. Or there will be more errors.*/
               if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                    pConn->oConnPeer.oLocal.pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               resources.nType = RES_LEC | RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                      &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }

               return IFX_ERROR;
            }

            /* get sigdet, siggen, LEC, codec on callee */
            resources.nType = RES_DET | RES_GEN | RES_CODEC | RES_VOIP_ID;
            resources.nType |= RES_LEC;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            if (IFX_SUCCESS != Common_ResourcesGet(pPhone, &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Return to state before hooking off. Or there will be more errors.*/
               resources.nType = RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }
               if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                    pConn->oConnPeer.oLocal.pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               resources.nType = RES_LEC | RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                      &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }

               return IFX_ERROR;
            }

            /* start DTMF detection on callee */
            if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl, pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_DTMF_Enable.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               resources.nType = RES_DET | RES_GEN | RES_LEC | RES_CONN_ID;
               resources.nConnID_Nr = SVIP_RM_UNUSED;
               if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }
               if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                     pConn->oConnPeer.oLocal.pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_LEC_Disable.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               resources.nType = RES_LEC | RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                          &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }
               return IFX_ERROR;
            }

            /* start LEC on callee */
            if (IFX_SUCCESS != Common_LEC_Enable(pCtrl, pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Return to state before hooking off. Or there will be more errors.*/
               if (IFX_SUCCESS != Common_DTMF_Disable(pCtrl, pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               resources.nType = RES_DET | RES_GEN | RES_LEC | RES_CONN_ID;
               resources.nConnID_Nr = SVIP_RM_UNUSED;
               if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }
               if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                    pConn->oConnPeer.oLocal.pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               resources.nType = RES_LEC | RES_CONN_ID;
               if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                      &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }

               return IFX_ERROR;
            }

            pConn->voipID = pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
         }
      } /* if (pConn->nType == LOCAL_CALL) */
      else
      {
         /* EXTERN_VOIP_CALL */

         if (pCtrl->pProgramArg->oArgFlags.nCID)
         {
            Common_CID_Disable(pPhone);
         }

         /* get connID on caller */
         resources.nType = RES_CONN_ID;
         if (IFX_SUCCESS != Common_ResourcesGet(pPhone, &resources))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
         }
         /* get sigdet, siggen, LEC, codec on callee */
         resources.nType = RES_DET | RES_GEN | RES_LEC;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         if (IFX_SUCCESS != Common_ResourcesGet(pPhone, &resources))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err on Common_ResourcesGet.\n(File: %s, line: %d)\n",
                   __FILE__, __LINE__));

            /* Return to state before hooking off. Or there will be more errors.*/
            resources.nType = RES_CONN_ID;
            if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
            return IFX_ERROR;
         }

         /* start DTMF detection on callee */
         if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl, pPhone))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err on Common_DTMF_Enable.\n(File: %s, line: %d)\n",
                   __FILE__, __LINE__));

            /* Return to state before hooking off. Or there will be more errors.*/
            resources.nType = RES_DET | RES_GEN | RES_LEC | RES_CONN_ID;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
            return IFX_ERROR;
         }
         /* start LEC on callee */
         if (IFX_SUCCESS != Common_LEC_Enable(pCtrl, pPhone))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                   __FILE__, __LINE__));

            /* Return to state before hooking off. Or there will be more errors.*/
            if (IFX_SUCCESS != Common_DTMF_Disable(pCtrl, pPhone))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
            resources.nType = RES_DET | RES_GEN | RES_LEC | RES_CONN_ID;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
            return IFX_ERROR;
         }
      } /* if (pConn->nType == LOCAL_CALL) */
#endif /* EASY336 */

#ifdef XT16
      /** \todo connect caller with callee */
      /* start DTMF detection on callee */
      if (IFX_SUCCESS != Common_DTMF_Enable(pCtrl, pPhone))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err on Common_DTMF_Enable.\n(File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
#endif /* XT16 */

      /* Start coder */
      if ((EXTERN_VOIP_CALL == pConn->nType) ||
          (LOCAL_BOARD_CALL == pConn->nType) ||
          ((LOCAL_CALL == pConn->nType) &&
           (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)))
      {
#ifdef TAPI_VERSION4
         if (pPhone->pBoard->fSingleFD)
         {
#ifdef EASY336
            /* get codec data structure */
            pOurCodec = Common_GetCodec(pCtrl, pConn->voipID,
                                        pPhone->nSeqConnId);
            if (IFX_NULL == pOurCodec)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on Common_GetCodec.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));

               /* Return to state before hooking off. Or there will be more errors.*/
               if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                     pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               if (IFX_SUCCESS != Common_DTMF_Disable(pCtrl, pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               resources.nType = RES_DET | RES_GEN | RES_LEC | RES_CONN_ID;
               resources.nConnID_Nr = SVIP_RM_UNUSED;
               if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                          __FILE__, __LINE__));
               }
               if((LOCAL_CALL == pConn->nType) &&
                  (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal))
               {
                  if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                    pConn->oConnPeer.oLocal.pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  resources.nType = RES_LEC | RES_CONN_ID;
                  if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                         &resources))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }
               }
               return IFX_ERROR;
            }
            else
            {
               /* set data channel number */
               pConn->nUsedCh = pOurCodec->nCh;
            }
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
            if (TD_FAX_MODE_FDP != pCtrl->nFaxMode)
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
            {
               ret = VOIP_StartCodec(pConn->nUsedCh, pConn,
                                     ABSTRACT_GetBoard(pCtrl,
                                        IFX_TAPI_DEV_TYPE_VIN_SUPERVIP), pPhone);
               /* check if codec started and printout codec type */
               if (IFX_SUCCESS != ret)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on VOIP_StartCodec.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));

                  /* Return to state before hooking off. Or there will be more errors.*/
                  if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                        pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  if (IFX_SUCCESS != Common_DTMF_Disable(pCtrl, pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  resources.nType = RES_DET | RES_GEN | RES_LEC | RES_CONN_ID;
                  resources.nConnID_Nr = SVIP_RM_UNUSED;
                  if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }
                  if((LOCAL_CALL == pConn->nType) &&
                     (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal))
                  {
                     if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                       pConn->oConnPeer.oLocal.pPhone))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                     resources.nType = RES_LEC | RES_CONN_ID;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                            &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }
                  }
                  return IFX_ERROR;
               } /* if (IFX_SUCCESS != ret) */
               Common_PrintEncoderType(pCtrl, pPhone, pConn);
            } /* if (TD_FAX_MODE_FDP != pCtrl->nFaxMode) */
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
            else
            {
               /* T.38 supported - switch to T.38 */
               if (IFX_SUCCESS != SIGNAL_StoreSettings(pPhone, pConn))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on SIGNAL_StoreSettings.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));

                  /* Return to state before hooking off. Or there will be more errors.*/
                  if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                        pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  if (IFX_SUCCESS != Common_DTMF_Disable(pCtrl, pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  resources.nType = RES_DET | RES_GEN | RES_LEC | RES_CONN_ID;
                  resources.nConnID_Nr = SVIP_RM_UNUSED;
                  if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }
                  if((LOCAL_CALL == pConn->nType) &&
                     (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal))
                  {
                     if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                       pConn->oConnPeer.oLocal.pPhone))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                     resources.nType = RES_LEC | RES_CONN_ID;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                            &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }
                  }
                  return IFX_ERROR;
               }
               if (IFX_SUCCESS != TD_T38_SetUp(pPhone, pConn))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on TD_T38_SetUp.\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));

                  /* Return to state before hooking off. Or there will be more errors.*/
                  if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                        pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  if (IFX_SUCCESS != Common_DTMF_Disable(pCtrl, pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  resources.nType = RES_DET | RES_GEN | RES_LEC | RES_CONN_ID;
                  resources.nConnID_Nr = SVIP_RM_UNUSED;
                  if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }
                  if((LOCAL_CALL == pConn->nType) &&
                     (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal))
                  {
                     if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                       pConn->oConnPeer.oLocal.pPhone))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                     resources.nType = RES_LEC | RES_CONN_ID;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                            &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }
                  }
                  return IFX_ERROR;

               }
               /* activate T.38 */
               if (IFX_SUCCESS != TD_T38_Start(pPhone, pConn->nUsedCh))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err on TD_T38_Start\n(File: %s, line: %d)\n",
                         __FILE__, __LINE__));

                  /* Return to state before hooking off. Or there will be more errors.*/
                  if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                        pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  if (IFX_SUCCESS != Common_DTMF_Disable(pCtrl, pPhone))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_DTMF_Disable.\n(File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                  }
                  resources.nType = RES_DET | RES_GEN | RES_LEC | RES_CONN_ID;
                  resources.nConnID_Nr = SVIP_RM_UNUSED;
                  if (IFX_SUCCESS != Common_ResourcesRelease(pPhone, &resources))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                             __FILE__, __LINE__));
                  }
                  if((LOCAL_CALL == pConn->nType) &&
                     (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal))
                  {
                     if (IFX_SUCCESS != Common_LEC_Disable(pCtrl,
                                                       pConn->oConnPeer.oLocal.pPhone))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_LEC_Enable.\n(File: %s, line: %d)\n",
                               __FILE__, __LINE__));
                     }
                     resources.nType = RES_LEC | RES_CONN_ID;
                     if (IFX_SUCCESS != Common_ResourcesRelease(pConn->oConnPeer.oLocal.pPhone,
                                                            &resources))
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                              ("Err on Common_ResourcesRelease.\n(File: %s, line: %d)\n",
                                __FILE__, __LINE__));
                     }
                  }
                  return IFX_ERROR;
               }
            } /* if (TD_FAX_MODE_FDP != pCtrl->nFaxMode) */
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
#endif /* EASY336 */
         } /* if (pPhone->pBoard->fSingleFD) */
#else /* TAPI_VERSION4 */
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
         /* check if fax mode */
         if (TD_FAX_MODE_FDP == pCtrl->nFaxMode &&
             EXTERN_VOIP_CALL == pConn->nType)
         {
            /* T.38 supported - switch to T.38 */
            if (IFX_SUCCESS != SIGNAL_StoreSettings(pPhone, pConn))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on SIGNAL_StoreSettings.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               return IFX_ERROR;
            }

            if (IFX_SUCCESS != TD_T38_SetUp(pPhone, pConn))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on TD_T38_SetUp.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               return IFX_ERROR;
            }

            /* activate T.38 */
            if (IFX_SUCCESS != TD_T38_Start(pPhone, pConn->nUsedCh))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on TD_T38_Start\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               return IFX_ERROR;
            }
         }
         else
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
         {
            ret = VOIP_StartCodec(pConn->nUsedCh, pConn, pBoard, pPhone);
            if (IFX_SUCCESS != ret)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on VOIP_StartCodec.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               return IFX_ERROR;
            }
            Common_PrintEncoderType(pCtrl, pPhone, pConn);
         }
#endif /* TAPI_VERSION4 */
#ifdef QOS_SUPPORT
         /* QoS Support */
         if (pCtrl->pProgramArg->oArgFlags.nQos)
         {
/* no QoS for SVIP */
#ifndef EASY336
            if (IFX_SUCCESS != QOS_StartSession(pConn, pPhone,
                                                TD_SET_ADDRESS_PORT,
                                                pBoard->nBoard_IDX))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on QOS_StartSession.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
            if (!pCtrl->pProgramArg->oArgFlags.nQosSocketStart)
            {
               TD_SOCK_PortSet(&pConn->oUsedAddr,
                               ntohs(pConn->oSession.oPair.srcPort));
            }
#endif /* EASY336 */
         }
#endif /* QOS_SUPPORT */

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
         /* check if fax mode */
         if (TD_FAX_MODE_FDP == pCtrl->nFaxMode &&
             EXTERN_VOIP_CALL == pConn->nType)
         {
            /* do nothing */
         }
         else
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
         /* fax modem only avaible for voip calls */
         if (EXTERN_VOIP_CALL == pConn->nType)
         {
            /* TAPI SIGNAL SUPPORT */
            if (IFX_SUCCESS != SIGNAL_Setup(pPhone, pConn))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err on SIGNAL_Setup.\n(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               return IFX_ERROR;
            }
         }
      } /* ((EXTERN_VOIP_CALL == pConn->nType) ..*/
   } /* else - Ordinary call */
#ifdef EASY3111
   /* check call type */
   if (LOCAL_CALL == pConn->nType ||
       EXTERN_VOIP_CALL == pConn->nType ||
       PCM_CALL == pConn->nType)
   {
      /* check if EASY 3111 is extension for vmmc board */
      if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
      {
         /* activate connection to main */
         if (IFX_SUCCESS != Easy3111_CallPCM_Activate(pPhone, pCtrl))
         {
            /* print error trace */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: Failed to activate connection to main.\n"
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
         } /* activate connection to main */
      } /* check if EASY 3111 is extension for vmmc board */
   } /* check call type */
#endif /* EASY3111 */

   TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_ACTIVE, pPhone->nSeqConnId);

   /* connection is now active */
   pConn->fActive = IFX_TRUE;

   /* set new state and reset event */
#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /* check if fax mode */
   if (TD_FAX_MODE_FDP == pCtrl->nFaxMode &&
       EXTERN_VOIP_CALL == pConn->nType)
   {
      if (IFX_SUCCESS == ret)
      {
         /* set new state */
         pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38;
      }
      pPhone->nIntEvent_FXS = IE_NONE;
   }
   else
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
   {
      pPhone->rgStateMachine[FXS_SM].nState = S_ACTIVE;
      pPhone->nIntEvent_FXS = IE_NONE;
   }

   COM_ITM_TRACES(pCtrl, pPhone, pConn, IFX_NULL, TTP_RINGING_ACTIVE,
                  pPhone->nSeqConnId);

   return IFX_SUCCESS;
} /* ST_FXS_InRinging_HookOff() */

/* CONFERENCE */
/**
   Action when detected IE_HOOKON event for S_CF_DIALTONE state.
   Changes phone state to S_READY, stops playing dialtone, ends
   conference, cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialtone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* stop generation of dialtone */
   Common_ToneStop(pCtrl, pPhone);

   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      /* release tone generator and detector,
         DTMF and lec disable is done in CONFERENCE_End() */
      resources.nType = RES_DET | RES_GEN;
      resources.nVoipID_Nr = SVIP_RM_UNUSED;
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   return IFX_SUCCESS;
} /* ST_FXS_CfDialtone_HookOn() */

/**
   Action when detected IE_FLASH_HOOK event for S_CF_DIALTONE state.
   If there are still peers in conference changes phone state to S_CF_ACTIVE
      and stops playing dialtone.
   If there all peers have left conference changes phone state to S_CF_BUSYTONE
      stops playing dialtone, ends conference and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialtone_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* stop playing tone */
   Common_ToneStop(pCtrl, pPhone);

   /* leave this state and go into S_CF_ACTIVE if still peers in conference
      or if all peers have left conference play busy tone and go into
      S_CF_BUSYTONE */
   ret = ST_Conference_FlashHook(pCtrl, pPhone, pConn);

   return ret;
} /* ST_FXS_CfDialtone_FlashHook() */

/**
   Action when detected IE_DIALING event for S_CF_DIALTONE state.
   Stops playing dialtone, changes phone state to S_CF_DIALING
   and forces IE_DIALING event handling

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialtone_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* set new state */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_DIALING;
   pPhone->nIntEvent_FXS = IE_DIALING;
   pCtrl->bInternalEventGenerated = IFX_TRUE;

   /* stop playing tone */
   ret = Common_ToneStop(pCtrl, pPhone);

   return ret;
} /* ST_FXS_CfDialtone_Dialing() */

/**
   Action when detected IE_END_CONNECTION event for S_CF_DIALTONE state.
   Phone stays in S_CF_DIALTONE state and removes peer from conference.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialtone_EndConnection(CTRL_STATUS_t* pCtrl,
                                             PHONE_t* pPhone,
                                             CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* stay in current FXS state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_DIALTONE;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* remove peer connection */
   ret = CONFERENCE_RemovePeer(pCtrl, pPhone, &pConn,
                               &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);

   return ret;
} /* ST_FXS_CfDialtone_EndConnection() */

/**
   Action when detected IE_HOOKON event for S_CF_DIALING state.
   Changes phone state to S_READY, ends conference,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialing_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* stop playing tone */
   Common_ToneStop(pCtrl, pPhone);

   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      /* release tone generator and detector,
         DTMF and lec disable is done in CONFERENCE_End() */
      resources.nType = RES_DET | RES_GEN;
      resources.nVoipID_Nr = SVIP_RM_UNUSED;
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   return IFX_SUCCESS;
}/* ST_FXS_CfDialing_HookOn() */

/**
   Action when detected IE_FLASH_HOOK event for S_CF_DIALING state.
   If there are still peers in conference changes phone state to S_CF_ACTIVE.
   If there all peers have left conference changes phone state to S_CF_BUSYTONE,
      ends conference and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialing_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn)
{
   /* reset event */
   pPhone->nIntEvent_FXS = IE_NONE;

   /* leave this state and go into S_CF_ACTIVE if still peers in conference
      or if all peers have left conference play busy tone and go into
      S_CF_BUSYTONE */
   ST_Conference_FlashHook(pCtrl, pPhone, pConn);

   return IFX_SUCCESS;
} /* ST_FXS_CfDialing_FlashHook() */

/**
   Action when detected IE_DIALING event for S_CF_DIALING state.

   Changes phone state to S_CF_CALLING if dialed number
           is correct and indicates new call. It gets reasources
           needed for this call and sends message to peer.
   Phone state S_CF_DIALING  doesn't change if dialing number isn't
           finished.
   Changes phone state to S_CF_BUSYTONE if dialed number is invalid
           or call initialization fails e.g. phone couldn't get resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark valid connections are LOCAL_CALL, EXTERN_VOIP_CALL and PCM_CALL
               also phone must be conference starter
*/
IFX_return_t ST_FXS_CfDialing_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   CALL_TYPE_t call_type = UNKNOWN_CALL_TYPE;
   IFX_int32_t called_number;
   IFX_uint32_t nIP_addr = 0;
   IFX_int32_t i;
   /** Called board's IP address */
   IFX_char_t rgCalledIP[TD_ADDRSTRLEN] = {0};

   /* We need to check if we either make new connection
      or we just dial next digit for external call */
   for (i=0; i <= pPhone->nConnCnt; i++)
   {
      if (EXTERN_VOIP_CALL == pPhone->rgoConn[i].nType &&
          IFX_FALSE == pPhone->rgoConn[i].fActive &&
          IFX_TRUE == pPhone->rgoConn[i].bExtDialingIn)
      {
            /* We just dial new digit for existing external call */
            if (IFX_SUCCESS != ST_Dialing_ExtCallDialing(pCtrl, pPhone,
                                                         &pPhone->rgoConn[i]))
            {
               /* play busy tone, clean phone structure and change phone state */
               ST_Dialing_Busy(pCtrl, pPhone, &pPhone->rgoConn[i]);
               pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
            }
            return IFX_SUCCESS;
      } /* if */
   } /* for */
   /* by default stay in S_CF_DIALING state */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_DIALING;
   pPhone->nIntEvent_FXS = IE_NONE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("Phone No %d: Dialed numbers count %d and number is %d\n",
       (IFX_int32_t) pPhone->nPhoneNumber,
       (IFX_int32_t) pPhone->nDialNrCnt,
       (IFX_int32_t) pPhone->pDialedNum[pPhone->nDialNrCnt-1]));
   /* get call type */
   call_type = Common_CheckDialedNum(pPhone, &called_number, &nIP_addr,
                                     rgCalledIP, pCtrl);

   /* check if call type is valid for conference */
   if ((call_type != UNKNOWN_CALL_TYPE) && (LOCAL_CALL != call_type) &&
       (EXTERN_VOIP_CALL != call_type) && (PCM_CALL != call_type) &&
       (FXO_CALL != call_type))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Failed to add new peer to conference - must be "
             "LOCAL_CALL or EXTERN_VOIP_CALL or PCM_CALL not %s."
             "(File: %s, line: %d)\n",
             (IFX_int32_t) pPhone->nPhoneNumber,
             Common_Enum2Name(call_type, TD_rgCallTypeName),
             __FILE__, __LINE__));
         /* play busy tone and change phone state */
         ST_Dialing_Busy(pCtrl, pPhone, pConn);
         pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
      return IFX_ERROR;
   }
   /* if making call print called number and call type*/
   if (call_type != UNKNOWN_CALL_TYPE)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Called number %s - %s\n",
             pPhone->nPhoneNumber, pPhone->pDialedNum,
             Common_Enum2Name(call_type, TD_rgCallTypeName)));
      pPhone->nDialNrCnt = 0;
   }

   switch (call_type)
   {
      case LOCAL_CALL:
      {
         ret = ST_Cf_Dialing_Dialing_Loc(pCtrl, pPhone, pConn, called_number);
         if (IFX_ERROR == ret)
         {
            /* play busy tone, clean phone structure and change phone state */
            ST_Dialing_Busy(pCtrl, pPhone, pConn);
            pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
         }
         else
         {
            /* change phone state */
            pPhone->rgStateMachine[FXS_SM].nState = S_CF_CALLING;
         }
         return ret;
         break; /* LOCAL_CALL */
      }
      case EXTERN_VOIP_CALL:
      {
#ifdef TD_IPV6_SUPPORT
         if(0 < strlen(rgCalledIP) && 0 == strcmp(rgCalledIP, TD_NO_NAME))
         {
            /* check if number was found in phone book */
            ret = IFX_ERROR;
         }
         else
#endif /* TD_IPV6_SUPPORT */
         {
            /* start external call */
            ret = ST_Cf_Dialing_Dialing_Ext(pCtrl, pPhone, pConn,
                     called_number, nIP_addr, rgCalledIP);
         }
         if (IFX_ERROR == ret)
         {
            /* play busy tone, clean phone structure and change phone state */
            ST_Dialing_Busy(pCtrl, pPhone, pConn);
            pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
         }
         else
         {
            /* stay in S_CF_DIALING state */
         }
         return ret;
         break;/* EXTERN_VOIP_CALL */
      }
      case PCM_CALL:
      {

         /* start local call */
         ret = ST_Cf_Dialing_Dialing_PCM(pCtrl, pPhone, pConn,
                                         called_number, nIP_addr);
         if (IFX_ERROR == ret)
         {
            /* play busy tone, clean phone structure and change phone state */
            ST_Dialing_Busy(pCtrl, pPhone, pConn);
            pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
         }
         else
         {
            /* change phone state */
            pPhone->rgStateMachine[FXS_SM].nState = S_CF_CALLING;
         }
         return ret;
         break; /* PCM_CALL */
      }
#ifdef FXO
      case FXO_CALL:
      {

         /* start local call */
         ret = ST_CF_Dialing_Dialing_FXO(pCtrl, pPhone, pConn, called_number);
         if (IFX_ERROR == ret)
         {
            /* play busy tone, clean phone structure and change phone state */
            ST_Dialing_Busy(pCtrl, pPhone, pConn);
            pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
         }
         else
         {
            /* change phone state */
            pPhone->rgStateMachine[FXS_SM].nState = S_CF_ACTIVE;
         }
         return ret;
         break; /* PCM_CALL */
      }
#endif /* FXO */
      case UNKNOWN_CALL_TYPE:
         /* do nothing */
         break;
      default:
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: Unsupported call type - called number %d."
                "(File: %s, line: %d)\n",
                (IFX_int32_t) pPhone->nPhoneNumber,
                (IFX_int32_t) called_number,
                __FILE__, __LINE__));
         /* play busy tone and change phone state */
         ST_Dialing_Busy(pCtrl, pPhone, pConn);
         pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
         return IFX_ERROR;
         break;
      }
   } /* switch */

   return ret;
} /* ST_FXS_CfDialing_Dialing() */

/**
   Action when detected IE_EXT_CALL_ESTABLISHED event for S_CF_DIALING state.

   Changes phone state to S_CF_CALLING if receive confirmation event from
   second peer. It means dialed number is correct and indicates new call.
   Only for external call.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialing_ExtCallEstablished(CTRL_STATUS_t* pCtrl,
                                                  PHONE_t* pPhone,
                                                  CONNECTION_t* pConn)
{
   /* Dialed external phone number is correct,
      so finish establishing connection */

   /* I want the called phone to establish a external call with me */
   if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_RINGING,
                                       pPhone->nSeqConnId))
   {
      pPhone->nConnCnt--;
      /* Play busy tone and change phone state */
      ST_Dialing_Busy(pCtrl, pPhone, pConn);
      pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
      return IFX_SUCCESS;
   }
   /* Change phone state */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_CALLING;
   pConn->bExtDialingIn = IFX_FALSE;
   TD_TIMEOUT_STOP(pCtrl->nUseTimeoutSocket);

   return IFX_SUCCESS;
} /* ST_FXS_CfDialing_ExtCallEstablished(() */

/**
   Action when detected IE_EXT_CALL_WRONG_NUM event for S_CF_DIALING state.

   Changes phone state to S_BUSYTONE if receive event IE_EXT_CALL_WRONG_NUM
   from second peer. It means dialed number is incorrect.
   Only for external call.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialing_ExtCallWrongNumber(CTRL_STATUS_t* pCtrl,
                                                  PHONE_t* pPhone,
                                                  CONNECTION_t* pConn)
{
   IFX_int32_t i;
   IFX_return_t ret = IFX_SUCCESS;
   CONNECTION_t* pConnTmp;

   /* Dialed external phone number is wrong,
   so stop connection and play busy tone */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Phone No %d: Invalid external number entered\n",
          (IFX_int32_t) pPhone->nPhoneNumber));

   if (NO_CONFERENCE == pPhone->nConfIdx)
   {
      pConn = &pPhone->rgoConn[0];
      pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
      /* Play busy tone, clean phone structure and change phone state */
      ST_Dialing_Busy(pCtrl, pPhone, pConn);
   }
   else
   {
      pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
      pPhone->nIntEvent_FXS = IE_NONE;

      /* Remove the peer we tried to connect */
      for (i = 0; i < pPhone->nConnCnt; i++)
      {
         /* If connection isn't active */
         if (IFX_FALSE == pPhone->rgoConn[i].fActive)
         {
            pConnTmp = &pPhone->rgoConn[i];
            CONFERENCE_RemovePeer(pCtrl, pPhone, &pConnTmp,
                                  &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);
         }
      }

      /* Play busy tone */
      TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, BUSY_TONE_IDX, "Busy tone");
   } /* if (NO_CONFERENCE == pPhone->nConfIdx) */

   return IFX_SUCCESS;
} /* ST_FXS_CfDialing_ExtCallWrongNumber() */

/**
   Action when detected IE_EXT_CALL_NO_ANSWER event for S_CF_DIALING state.

   Changes phone state to S_BUSYTONE if there is no respond from second peer.
   It means dialed number is incorrect.
   Only for external call.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialing_ExtCallNoAnswer(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn)
{
   IFX_int32_t i;
   IFX_return_t ret = IFX_SUCCESS;
   CONNECTION_t* pConnTmp;

   /* No answer from called side,
    so stop connection and play busy tone */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Phone No %d: No answer during %d [ms] from called phone.\n",
          (IFX_int32_t) pPhone->nPhoneNumber, TIMEOUT_FOR_EXTERNAL_CALL));

   if (NO_CONFERENCE == pPhone->nConfIdx)
   {
      pConn = &pPhone->rgoConn[0];
      pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;
      /* Play busy tone, clean phone structure and change phone state */
      ST_Dialing_Busy(pCtrl, pPhone, pConn);
   }
   else
   {
      pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
      pPhone->nIntEvent_FXS = IE_NONE;

      /* Remove the peer we tried to connect */
      for (i = 0; i < pPhone->nConnCnt; i++)
      {
         /* If connection isn't active */
         if (IFX_FALSE == pPhone->rgoConn[i].fActive)
         {
            pConnTmp = &pPhone->rgoConn[i];
            CONFERENCE_RemovePeer(pCtrl, pPhone, &pConnTmp,
                                  &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);
         }
      }

      /* Play busy tone */
      TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, BUSY_TONE_IDX, "Busy tone");
   } /* if (NO_CONFERENCE == pPhone->nConfIdx) */

   return IFX_SUCCESS;
} /* ST_FXS_CfDialing_ExtCallNoAnswer() */

/**
   Action when detected IE_EXT_CALL_PROCEEDING event for S_CF_DIALING state.

   It means dialed number is incompleted.
   Waiting for next digits. Only for external call.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark Doesn't change phone state.
*/
IFX_return_t ST_FXS_CfDialing_ExtCallProceeding(CTRL_STATUS_t* pCtrl,
                                               PHONE_t* pPhone,
                                               CONNECTION_t* pConn)
{

   /* By default stay in S_OUT_DIALING state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_DIALING;
   pPhone->nIntEvent_FXS = IE_NONE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("Phone No %d: Dialed number is incomplete\n",
       (int)pPhone->nPhoneNumber));

   pConn->timeStartExtCalling = TD_OS_ElapsedTimeMSecGet(0);

   return IFX_SUCCESS;
}/* ST_FXS_CfDialing_ExtCallProceeding() */

/**
   Action when detected IE_END_CONNECTION event for S_CF_DIALING state.
   Phone stays in S_CF_DIALING state and removes peer from conference.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfDialing_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                            CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* stay in current FXS state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_DIALING;
   pPhone->nIntEvent_FXS = IE_NONE;

   ret = CONFERENCE_RemovePeer(pCtrl, pPhone, &pConn,
                               &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);

   return ret;
} /* ST_FXS_CfDialing_EndConnection() */

/**
   Action when detected IE_HOOKON event for S_CF_DIALING state.
   Changes phone state to S_READY, ends conference,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfCalling_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* clean up phone structure and end conference */
   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      /* release tone generator and detector,
         DTMF and lec disable is done in CONFERENCE_End() */
      resources.nType = RES_DET | RES_GEN;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      resources.nVoipID_Nr = SVIP_RM_UNUSED;

      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   }
   else
#endif /* TAPI_VERSION4 */
   {
      /* all is done in TAPIDEMO_ClearPhone() */
   }

   return IFX_SUCCESS;
} /* ST_FXS_CfCalling_HookOn() */

/**
   Action when detected IE_FLASH_HOOK event for S_CF_DIALING state.
   If there are still peers in conference changes phone state to S_CF_ACTIVE
      and sends end connection message to peer that we tring to connect to.
   If there all peers have left conference changes phone state to S_CF_BUSYTONE,
      sends end connection message to peer that we tring to connect to,
      ends conference, and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfCalling_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* leave this state and go into S_CF_ACTIVE if still peers in conference
      or if all peers have left conference play busy tone and go into
      S_CF_BUSYTONE */
   ret = ST_Conference_FlashHook(pCtrl, pPhone, pConn);

   return ret;
} /* ST_FXS_CfCalling_FlashHook() */

/**
   Action when detected IE_RINGBACK event for S_CF_DIALING state.
   Changes phone state to S_CF_RINGBACK and plays ringback tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfCalling_Ringback(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   pPhone->rgStateMachine[FXS_SM].nState = S_CF_RINGBACK;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* play ringback tone */
   TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, RINGBACK_TONE_IDX, "Ringback tone");

   return ret;
} /* ST_FXS_CfCalling_Ringback() */

/**
   Action when detected IE_END_CONNECTION event for S_CF_DIALING state.
   Phone stays in S_CF_CALLING state and removes peer from conference.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfCalling_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                            CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* stay in current FXS state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_CALLING;
   pPhone->nIntEvent_FXS = IE_NONE;

   ret = CONFERENCE_RemovePeer(pCtrl, pPhone, &pConn,
                               &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);

   return ret;
} /* ST_FXS_CfCalling_EndConnection() */

/**
   Action when detected IE_END_CONNECTION event for S_CF_DIALING state.
   Changes phone status to S_CF_BUSYTONE, removes peer from conference
   and plays busy tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfCalling_Busy(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn)
{
   IFX_int32_t i;
   IFX_return_t ret = IFX_SUCCESS;
   CONNECTION_t* pConnTmp;

   pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* remove the peer we tried to connect */
   for (i = 0; i < pPhone->nConnCnt; i++)
   {
      /* if connection isn't active */
      if (IFX_FALSE == pPhone->rgoConn[i].fActive)
      {
         pConnTmp = &pPhone->rgoConn[i];
         CONFERENCE_RemovePeer(pCtrl, pPhone, &pConnTmp,
                               &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);
      }
   }

   /* play busy tone */
   TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, BUSY_TONE_IDX, "Busy tone");

   return ret;
} /* ST_FXS_CfCalling_Busy() */

/**
   Action when detected IE_HOOKON event for S_CF_RINGBACK state.
   Changes phone state to S_READY, stops playing ringback tone, ends conference
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfRingback_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
   CONFERENCE_t* pConf = IFX_NULL;
   pPhone->nDialNrCnt = 0;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* Get conference */
   if ((NO_CONFERENCE != pPhone->nConfIdx) &&
       (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone in conference state has no conference index."
             " (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* stop playing ringback tone */
   Common_ToneStop(pCtrl, pPhone);

   if ((IFX_NULL != pConf) && (IFX_TRUE == pPhone->nConfStarter))
   {
      /* End with conference */
      TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);
   }

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      /* release tone generator and detector,
         DTMF and lec disable is done in CONFERENCE_End() */
      resources.nType = RES_DET | RES_GEN;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      resources.nVoipID_Nr = SVIP_RM_UNUSED;
      Common_ResourcesRelease(pPhone, &resources);
#endif
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   return IFX_SUCCESS;
} /* ST_FXS_CfRingback_HookOn() */

/**
   Action when detected IE_FLASH_HOOK event for S_CF_RINGBACK state.
   If there are still peers in conference changes phone state to S_CF_ACTIVE,
      sends end connection message to peer that we tring to connect to
      and stops playing ringback tone.
   If there all peers have left conference changes phone state to S_CF_BUSYTONE
      sends end connection message to peer that we tring to connect to, stops
      playing ringback tone, ends conference, starts busy tone
      and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfRingback_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn)
{
   /* stop playing ringback tone */
   Common_ToneStop(pCtrl, pPhone);

   /* leave this state and go into S_CF_ACTIVE if still peers in conference
      or if all peers have left conference play busy tone and go into
      S_CF_BUSYTONE */
   ST_Conference_FlashHook(pCtrl, pPhone, pConn);

   return IFX_SUCCESS;
} /* ST_FXS_CfRingback_FlashHook() */

/**
   Action when detected IE_ACTIVE event for S_CF_RINGBACK state.
   Changes phone state to S_ACTIVE, stops playing ringback tone,
   starts connection with new peer.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfRingback_Active(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
   IFX_int32_t ret;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
   IFX_int32_t i;

   /* Find needed connection */
   for (i=0; i < pPhone->nConnCnt; i++)
   {
      if(pPhone->rgoConn[i].nType == EXTERN_VOIP_CALL &&
         pPhone->rgoConn[i].fActive == IFX_FALSE)
      {
         pConn = &pPhone->rgoConn[i];
         break;
      }
   } /* for */

   if (pConn == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Phone No %d: Can't find proper connection.\n"
          "(File: %s, line: %d)\n",
          (IFX_int32_t) pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   pPhone->rgStateMachine[FXS_SM].nState = S_CF_ACTIVE;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* stop ringback tone */
   Common_ToneStop(pCtrl, pPhone);
   /* We called the phone and it pick up, we have connection */
#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      if ((pConn->nType == EXTERN_VOIP_CALL) &&
          (pPhone->connID == SVIP_RM_UNUSED))
      {
         /* get connID and LEC */
         resources.nType = RES_CONN_ID | RES_LEC;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         Common_ResourcesGet(pPhone, &resources);
         Common_LEC_Enable(pCtrl, pPhone);
      }
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   /* Connection is active */
   pConn->fActive = IFX_TRUE;

   /* Create new connection */
   if ((NO_CONFERENCE == pPhone->nConfIdx) ||
       (MAX_CONFERENCES < pPhone->nConfIdx))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Phone No %d: Invalid conference index %d for phone"
             " in conference state."
             " (File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->nConfIdx, __FILE__, __LINE__));
      return IFX_ERROR;
   }

#ifndef EASY336
   if ((PCM_CALL == pConn->nType) ||
       (LOCAL_PCM_CALL == pConn->nType) ||
       (LOCAL_BOARD_PCM_CALL == pConn->nType))
   {
      TAPIDEMO_PORT_DATA_t oPortData;
      oPortData.nType = PORT_FXS;
      oPortData.uPort.pPhopne = pPhone;
      PCM_StartConnection(&oPortData, pCtrl->pProgramArg, pPhone->pBoard, pConn,
                          pPhone->nSeqConnId);
   }
   else
#endif /* EASY336 */
   if ((EXTERN_VOIP_CALL == pConn->nType) ||
       (LOCAL_BOARD_CALL == pConn->nType) ||
       ((LOCAL_CALL == pConn->nType) &&
        pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal))
   {
     /* Start codec for new data channel that is
        part of conference. */
#ifdef TAPI_VERSION4
      if (pPhone->pBoard->fSingleFD)
      {
#ifdef EASY336
#if defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)
         if (TD_FAX_MODE_FDP != pCtrl->nFaxMode)
#endif /* defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW) */
         {
            ret = VOIP_StartCodec(pConn->nUsedCh, pConn,
                                  ABSTRACT_GetBoard(pCtrl,
                                  IFX_TAPI_DEV_TYPE_VIN_SUPERVIP), pPhone);
            /* check if codec started and print codec type */
            if (IFX_SUCCESS == ret)
            {
               Common_PrintEncoderType(pCtrl, pPhone, pConn);
            }
         } /* if (TD_FAX_MODE_FDP != pCtrl->nFaxMode) */
#if defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Fax channel can not be added to conference."
                   " (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            /* notify called phone that this connection has ended */
            TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_READY,
                               pPhone->nSeqConnId);
            /* remove peer */
            CONFERENCE_RemovePeer(pCtrl, pPhone, &pConn,
                                  &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);
            return IFX_ERROR;
         }
#endif /* defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW) */
#endif /* EASY336 */
      } /* if (pPhone->pBoard->fSingleFD) */
      else
#endif /* TAPI_VERSION4 */
#ifndef TAPI_VERSION4
      {
         if (IFX_SUCCESS == VOIP_StartCodec(pConn->nUsedCh, pConn,
                                            pPhone->pBoard, pPhone))
         {
            /* print used encoder type */
            Common_PrintEncoderType(pCtrl, pPhone, pConn);
         }
      } /* if (pPhone->pBoard->fSingleFD) */
#else /* TAPI_VERSION4 */
      ;
#endif /* TAPI_VERSION4 */
#ifdef QOS_SUPPORT
      /* QoS Support */
      if (pCtrl->pProgramArg->oArgFlags.nQos)
      {
/* no QoS for SVIP */
#ifndef EASY336
         QOS_StartSession(pConn, pPhone, TD_SET_ONLY_PORT,
                          pPhone->pBoard->nBoard_IDX);
#endif /* EASY336 */
      }
#endif /* QOS_SUPPORT */
   }
#ifndef EASY336
   else
   {
      Common_MapLocalPhones(pPhone, pConn->oConnPeer.oLocal.pPhone, TD_MAP_ADD);
   }
#endif /* EASY336 */

   /* print connection call progress data */
   Common_PrintCallProgress(S_OUT_RINGBACK, IE_ACTIVE, pConn->nType, pPhone,
                       pConn, pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal);

   return IFX_SUCCESS;
} /* ST_FXS_CfRingback_Active() */

/**
   Action when detected IE_ACTIVE event for S_CF_RINGBACK state.
   Phone stays in S_CF_RINGBACK state and removes peer from conference.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfRingback_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                             CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* stay in current FXS state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_RINGBACK;
   pPhone->nIntEvent_FXS = IE_NONE;

   ret = CONFERENCE_RemovePeer(pCtrl, pPhone, &pConn,
                               &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);

   return ret;
} /* ST_FXS_CfRingback_EndConnection() */

/**
   Action when detected IE_HOOKON event for S_CF_ACTIVE state.
   Changes phone state to S_READY, stops playing dialtone, ends conference,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfActive_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn)
{
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* CONFERENCE, master has left the conference so stop it */

   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      /* release tone generator and detector,
         DTMF and lec disable is done in CONFERENCE_End() */
      resources.nType = RES_DET | RES_GEN;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      resources.nVoipID_Nr = SVIP_RM_UNUSED;

      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   /* connection has ended - reset connection type */
   pConn->nType = UNKNOWN_CALL_TYPE;

   return IFX_SUCCESS;
}/* ST_FXS_CfActive_HookOn() */

/**
   Action when detected IE_CONFERENCE event for S_CF_ACTIVE state.
   If we didn't reached maximal count of connections in conference changes phone
   state to S_CF_DIALTONE and starts playing dialtone.
   If reached maximal count of connections in conference phone stays in
   S_CF_ACTIVE state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfActive_Conference(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   /* We are in the middle of connection, will make another one */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_DIALTONE;
   pPhone->nIntEvent_FXS = IE_NONE;

   pPhone->nDialNrCnt = 0;
#ifdef FXO
   /* check if FXO connected to phone */
   if (IFX_NULL != pPhone->pFXO)
   {
      /* do flash-hook to prevent conference starting on called board */
      if (IFX_SUCCESS != FXO_FlashHook(pPhone->pFXO))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: Flash-hook on FXO No %d failed.\n"
                "(File: %s, line: %d)\n",
                (IFX_int32_t) pPhone->nPhoneNumber, pPhone->pFXO->nFXO_Number,
                __FILE__, __LINE__));
      }
   }
#endif /* FXO */

   /* Check if we havent reached max peers in conference */
   /* We need to subtract 1 because we check nr of connections */
   if (MAX_PEERS_IN_CONF - 1 <= pPhone->nConnCnt)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: MAX peers number in conference reached.\n"
             "%sCan not add new peer to conference.\n",
             (IFX_int32_t) pPhone->nPhoneNumber,
             pPhone->pBoard->pIndentPhone));
      /* can not add new peer stay in S_CF_ACTIVE state */
      pPhone->rgStateMachine[FXS_SM].nState = S_CF_ACTIVE;
      return ret;
   }
   else
   {
      /* prepare connection strusture */
      memset(&pPhone->rgoConn[pPhone->nConnCnt], 0, sizeof(CONNECTION_t));
   }

   /* play dialtone */
   TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, DIALTONE_IDX, "Dialtone");

   return ret;
} /* ST_FXS_CfActive_Conference() */

/**
   Action when detected IE_CONFERENCE event for S_CF_ACTIVE state.
   If more than one peer in conference phone stays in S_CF_ACTIVE state
   and removes peer from conference.
   If last peer has left conference changes phone status to S_CF_BUSYTONE,
   ends conference and plays busy tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfActive_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                           CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   CONFERENCE_t* pConf = IFX_NULL;

   /* stay in current FXS state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_ACTIVE;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* One of the phones left us and sended IE_END_CONNECTION,
      so remove it from conference. */
   /* Get conference */
   if ((NO_CONFERENCE != pPhone->nConfIdx) &&
       (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone in conference state has no conference index."
             " (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (IFX_NULL != pConf)
   {
      /* if there are still peers in conference then remove peer */
      if (0 < pPhone->nConnCnt)
      {
         CONFERENCE_RemovePeer(pCtrl, pPhone, &pConn, pConf);
      }
      /* if no more peers in conference, then end conference */
      if (0 >= pPhone->nConnCnt)
      {
         /* no more peers in conference - change state */
         pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;

         /* use first connection structure */
         pConn = &pPhone->rgoConn[0];
         TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_BUSY, pPhone->pBoard);

         /* play busy tone */
         TD_CPTG_PLAY_CHECK(ret, pCtrl, pPhone, BUSY_TONE_IDX, "Busy tone");
      } /* (1 < pConf->nPeersCnt) */
   } /* if (IFX_NULL != pConf) */

   return ret;
} /* ST_FXS_CfActive_EndConnection() */

/**
   Action when detected IE_HOOKON event for S_CF_BUSYTONE state.
   Changes phone state to S_READY, stops playing busy tone,
   ends conference if active, cleans phone structure
   and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfBusytone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* stop busy tone */
   Common_ToneStop(pCtrl, pPhone);

   TAPIDEMO_ClearPhone(pCtrl, pPhone, pConn, IE_READY, pPhone->pBoard);


#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      /* release tone generator and detector,
         DTMF and lec disable is done in CONFERENCE_End() */
      resources.nType = RES_DET | RES_GEN;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      resources.nVoipID_Nr = SVIP_RM_UNUSED;

      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */

   /* reset connection type */
   pConn->nType = UNKNOWN_CALL_TYPE;

   return IFX_SUCCESS;
} /* ST_FXS_CfBusytone_HookOn() */

/**
   Action when detected IE_FLASH_HOOKON event for S_CF_BUSYTONE state.
   If there are still peers in conference changes phone state
   to S_CF_ACTIVE and stops playing busy tone.
   If all peers have left conference stays in S_CF_BUSYTONE state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfBusytone_FlashHook(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn)
{
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* check if there are other connections */
   if (0 < pPhone->nConnCnt)
   {
      /* get back to S_CF_ACTIVE state and continue conference */
      pPhone->rgStateMachine[FXS_SM].nState = S_CF_ACTIVE;

      /* stop busy tone */
      Common_ToneStop(pCtrl, pPhone);
   }

   return IFX_SUCCESS;
} /* ST_FXS_CfBusytone_FlashHook() */

/**
   Action when detected IE_FLASH_HOOKON event for S_CF_BUSYTONE state.
   Phone stays in S_CF_BUSYTONE state and removes peer from conference.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_CfBusytone_EndConnection(CTRL_STATUS_t* pCtrl,
                                             PHONE_t* pPhone,
                                             CONNECTION_t* pConn)
{
   IFX_return_t ret;

   /* stay in current FXS state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_CF_BUSYTONE;
   pPhone->nIntEvent_FXS = IE_NONE;

   ret = CONFERENCE_RemovePeer(pCtrl, pPhone, &pConn,
                               &pCtrl->rgoConferences[pPhone->nConfIdx - 1]);

   return IFX_SUCCESS;
} /* ST_FXS_CfBusytone_EndConnection() */

/* CONFERENCE END */

/**
   Action when detected IE_HOOKOFF event for S_TEST_TONE state.
   Plays tested tone.
   Happens only when making modular test (ITM) of predefined tones.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_TestTone_HookOff(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
   IFX_int32_t nToneNumber;

   pPhone->rgStateMachine[FXS_SM].nState = S_TEST_TONE;
   pPhone->nIntEvent_FXS = IE_NONE;

#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
#ifdef EASY336
      resources.nType = RES_DET | RES_GEN;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      Common_ResourcesGet(pPhone, &resources);
      Common_DTMF_Enable(pCtrl, pPhone);
#endif /* EASY336 */
      /* Set line in active */
   } /* if (pPhone->pBoard->fSingleFD) */
   else
#endif /* TAPI_VERSION4 */
   {
#ifndef TAPI_VERSION4
      /* enable the DTMF detection */
      Common_DTMF_SIG_Enable(pCtrl, pPhone);
#endif /* TAPI_VERSION4 */
   } /* if (pPhone->pBoard->fSingleFD) */

   /* Set line to standby mode */
   Common_LineFeedSet(pPhone, IFX_TAPI_LINE_FEED_ACTIVE);

   /* stop signal generator */
   Common_ToneStop(pCtrl, pPhone);

   /* set tone number */
   nToneNumber = pPhone->pDialedNum[0];
   /* check if valid tone number */
   if ((0 < nToneNumber && 13 > nToneNumber) ||
       (27 < nToneNumber && 32 > nToneNumber) ||
       17 == nToneNumber || 22 == nToneNumber)
   {
      TD_OS_MSecSleep(500);
#ifdef TAPI_VERSION4
      if (pPhone->pBoard->fSingleFD)
      {
         /* play tone */
         ret = Common_TonePlay(pCtrl, pPhone, nToneNumber);
      } /* if (pPhone->pBoard->fSingleFD) */
      else
#endif /* TAPI_VERSION4 */
      {
         /* play tone */
         ret = TD_IOCTL(pPhone->nPhoneCh_FD, IFX_TAPI_TONE_LOCAL_PLAY,
                  (IFX_int32_t) nToneNumber, TD_DEV_NOT_SET, pPhone->nSeqConnId);
      }
      if (IFX_SUCCESS == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("ITM: Phone No %d: playing tone %d.\n",
                pPhone->nPhoneNumber, nToneNumber));
         ret = IFX_SUCCESS;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("ITM: Phone No %d: Failed to play tone."
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("ITM: Phone No %d: Tone number out of range."
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }

   return ret;
} /* ST_FXS_TestTone_HookOff() */

/**
   Action when detected IE_HOOKOFF event for S_TEST_TONE state.
   Turns off resources after tone test.
   Happens only when making modular test (ITM) of predefined tones.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_TestTone_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   pPhone->rgStateMachine[FXS_SM].nState = S_READY;
   pPhone->nIntEvent_FXS = IE_NONE;

   /* stop signal generator */
   Common_ToneStop(pCtrl, pPhone);
#ifdef TAPI_VERSION4
   if (pPhone->pBoard->fSingleFD)
   {
      Common_DTMF_Disable(pCtrl, pPhone);
#ifdef EASY336
      resources.nType = RES_DET | RES_GEN;
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
   } /* if (pPhone->pBoard->fSingleFD) */
   else
#endif /* TAPI_VERSION4 */
   {
#ifndef TAPI_VERSION4
      /* enable the DTMF detection */
      Common_DTMF_SIG_Disable(pCtrl, pPhone);
#endif /* TAPI_VERSION4 */
   } /* if (pPhone->pBoard->fSingleFD) */
      /* Set line to standby mode */
   ret = Common_LineFeedSet(pPhone, IFX_TAPI_LINE_FEED_STANDBY);

   return ret;
} /* ST_FXS_TestTone_HookOn() */

/**
   Action when detected IE_HOOKON event for S_FAX state.
   Changes phone state to S_READY, communicates with other peer,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxModem_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#ifdef TD_FAX_MODEM
   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;

   SIGNAL_RestoreChannel(pPhone, pConn);
   /* end connection - release resources, turn of lec, send message to peer */
   ret = ST_Active_HookOn(pCtrl, pPhone, pConn);
#endif /* TD_FAX_MODEM */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_Fax_HookOn() */

/**
   Action when detected IE_END_CONNECTION event for S_FAX state.
   Peer finished call. Changes phone status to S_BUSYTONE and plays busy tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxModem_EndConnection(CTRL_STATUS_t* pCtrl,
                                            PHONE_t* pPhone,
                                            CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#ifdef TD_FAX_MODEM
   pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;

   SIGNAL_RestoreChannel(pPhone, pConn);
   /* end connection - release resources, play busy tone etc.. */
   ret = ST_Active_EndConnection(pCtrl, pPhone, pConn);
#endif /* TD_FAX_MODEM */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_Fax_EndConnection() */

/**
   Action when detected IE_DIS event for S_FAX state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxModem_DIS(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                 CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38))

   /* currently only AR9, VR9, xRX300 and SVIP support FAX T.38 stack. */
#ifdef HAVE_T38_IN_FW
   if ((pPhone->pBoard->nT38_Support) &&
       (TD_FAX_MODE_DEAFAULT == pCtrl->nFaxMode))
   { /* T.38 supported - switch to T.38 */
      ret = TD_T38_SetUp(pPhone, pConn);

      /* Send T.38 request to the peer. */
      TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_T38_REQ, pPhone->nSeqConnId);

      /* clear information about peer. */
      pConn->bT38_PeerEndTransm = IFX_FALSE;
      /* set new state and reset event */
      pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38_REQ;
   }
#endif /* HAVE_T38_IN_FW */
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_Fax_DIS() */

/**
   Action when detected IE_END_DATA_TRANSMISSION event for S_FAX_MODEM state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxModem_EndDataTrans(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                          CONNECTION_t* pConn)
{
   IFX_int32_t ret = IFX_SUCCESS;

#ifdef TD_FAX_MODEM

   /* reset Fax/Modem signal detection */
   SIGNAL_HandlerReset(pPhone, pConn);
   /* enable fax/modem signal detectors */
   SIGNAL_Setup(pPhone, pConn);

   /* restore channel settings if channel setting were changed.*/
   ret = SIGNAL_RestoreChannel(pPhone, pConn);

   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_ACTIVE;
#endif /* TD_FAX_MODEM */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_Fax_EndDataTrans() */

/**
   Action when detected IE_T38_REQ event for S_FAX state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
   Peer requested T.38 transmission. Start T.38 and send IE_T38_ACK to the peer.
*/
IFX_return_t ST_FXS_FaxModem_T38Req(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38))

   /* currently only AR9, VR9, xRX300 and SVIP support FAX T.38 stack. */
#ifdef HAVE_T38_IN_FW
   if ((pPhone->pBoard->nT38_Support) &&
       (TD_FAX_MODE_DEAFAULT == pCtrl->nFaxMode))
   {
      /* T.38 supported */
      ret = TD_T38_SetUp(pPhone, pConn);
      /* activate T.38 */
      if (IFX_SUCCESS == ret)
      {
         ret = TD_T38_Start(pPhone, pConn->nUsedCh);
      }

      if (IFX_SUCCESS == ret)
      {
         /* Send T.38 request to the peer. */
         ret = TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_T38_ACK,
                                  pPhone->nSeqConnId);
      }

      if (IFX_SUCCESS == ret)
      {
         /* set new state and reset event */
         pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38;
         COM_ITM_TRACES(pCtrl, pPhone, pConn, IFX_NULL, TTP_SET_S_FAXT38,
                        pPhone->nSeqConnId);
      }
   }
#endif /* HAVE_T38_IN_FW */
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38_T38Req() */

/**
   Action when detected IE_HOOKON event for S_FAXT38 state.
   Changes phone state to S_READY, communicates with other peer,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;

   /* Read T.38 session statistics */
   if (TD_T38_GetStatistics(pPhone, pConn->nUsedCh) == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Err, Phone No %d: Get T.38 session statistics failed. "
            "(File: %s, line: %d)\n",
            pPhone->nPhoneNumber, __FILE__, __LINE__));
   }

   /* stop T.38 session */
   if (TD_T38_Stop(pPhone, pConn->nUsedCh) == IFX_ERROR)
      ret = IFX_ERROR;

   if (SIGNAL_RestoreChannel(pPhone, pConn) == IFX_ERROR)
      ret = IFX_ERROR;

   /* end connection - release resources, turn of lec, send message to peer */
   if (ST_Active_HookOn(pCtrl, pPhone, pConn) == IFX_ERROR)
      ret = IFX_ERROR;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38_HookOn() */

/**
   Action when detected IE_END_CONNECTION event for S_FAXT38 state.
   Peer finished call. Changes phone status to S_BUSYTONE and plays busy tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38_EndConnection(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;

   /* Read T.38 session statistics */
   if (TD_T38_GetStatistics(pPhone, pConn->nUsedCh) == IFX_ERROR)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Err, Phone No %d: Get T.38 session statistics failed. "
            "(File: %s, line: %d)\n",
            pPhone->nPhoneNumber, __FILE__, __LINE__));
   }

   if (TD_T38_Stop(pPhone, pConn->nUsedCh) == IFX_ERROR)
      ret = IFX_ERROR;

   if (SIGNAL_RestoreChannel(pPhone, pConn) == IFX_ERROR)
      ret = IFX_ERROR;
   /* end connection - release resources, play busy tone etc.. */
   if (ST_Active_EndConnection(pCtrl, pPhone, pConn) == IFX_ERROR)
      ret = IFX_ERROR;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38_EndConnection() */

/**
   Action when detected IE_END_DATA_TRANSMISSION event for S_FAXT38 state.
   Return to voice connection.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38_EndDataTrans(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))

   /* continue as t.38 transmision if fax mode is set to TD_FAX_MODE_FDP */
   if (TD_FAX_MODE_FDP != pCtrl->nFaxMode)
   {
      /* Read T.38 session statistics */
      if (TD_T38_GetStatistics(pPhone, pConn->nUsedCh) == IFX_ERROR)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, Phone No %d: Get T.38 session statistics failed. "
               "(File: %s, line: %d)\n",
               pPhone->nPhoneNumber, __FILE__, __LINE__));
      }

      if (TD_T38_Stop(pPhone, pConn->nUsedCh) == IFX_ERROR)
         ret = IFX_ERROR;

      /* reset Fax/Modem signal detection */
      if (SIGNAL_HandlerReset(pPhone, pConn) == IFX_ERROR)
         ret = IFX_ERROR;
      /* enable fax/modem signal detectors */
      if (SIGNAL_Setup(pPhone, pConn) == IFX_ERROR)
         ret = IFX_ERROR;

      /* restore channel settings. */
      ret = SIGNAL_RestoreChannel(pPhone, pConn);

      if (ret == IFX_SUCCESS)
      {
         if (pConn->bT38_PeerEndTransm)
         {
            /* Peer finished T.38 connection. Start voice codec. */
            ret = VOIP_StartCodec(pConn->nUsedCh, pConn,
                                  pPhone->pBoard, pPhone);
            /* set new state and reset event */
            pPhone->rgStateMachine[FXS_SM].nState = S_ACTIVE;
            if (IFX_SUCCESS == ret)
            {
               COM_ITM_TRACES(pCtrl, pPhone, pConn, IFX_NULL,
                              TTP_T_38_CODEC_START, pPhone->nSeqConnId);
            }
         }
         else
         {
            /* set new state and reset event */
            pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38_END;
         }
      }
      /* Send voice request to the peer. */
      TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_T38_END, pPhone->nSeqConnId);
   }
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38_EndDataTrans() */

/**
   Action when detected IE_T38_END event for S_FAXT38 state.
   Peer informed us that finished T.38 transmission.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38_T38End(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))

   pConn->bT38_PeerEndTransm = IFX_TRUE;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38_T38End() */

/**
   Action when detected IE_HOOKON event for S_FAXT38_REQ state.
   Changes phone state to S_READY, communicates with other peer,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38Req_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;

   if (SIGNAL_RestoreChannel(pPhone, pConn) == IFX_ERROR)
      ret = IFX_ERROR;

   /* end connection - release resources, turn of lec, send message to peer */
   if (ST_Active_HookOn(pCtrl, pPhone, pConn) == IFX_ERROR)
      ret = IFX_ERROR;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38Req_HookOn() */

/**
   Action when detected IE_END_CONNECTION event for S_FAXT38_REQ state.
   Peer finished call. Changes phone status to S_BUSYTONE and plays busy tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38Req_EndConnection(CTRL_STATUS_t* pCtrl,
                                            PHONE_t* pPhone,
                                            CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;

   if (SIGNAL_RestoreChannel(pPhone, pConn) == IFX_ERROR)
      ret = IFX_ERROR;
   /* end connection - release resources, play busy tone etc.. */
   if (ST_Active_EndConnection(pCtrl, pPhone, pConn) == IFX_ERROR)
      ret = IFX_ERROR;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38Req_EndConnection() */

/**
   Action when detected IE_END_DATA_TRANSMISSION event for S_FAXT38_REQ state.
   Return to voice connection.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38Req_EndDataTrans(CTRL_STATUS_t* pCtrl,
                                           PHONE_t* pPhone,
                                           CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))

   /* continue as t.38 transmision if fax mode is set to TD_FAX_MODE_FDP,
      if TD_FAX_MODE_FDP is set then phone should not be in this state */
   if (TD_FAX_MODE_FDP != pCtrl->nFaxMode)
   {
      /* reset Fax/Modem signal detection */
      if (SIGNAL_HandlerReset(pPhone, pConn) == IFX_ERROR)
         ret = IFX_ERROR;
      /* enable fax/modem signal detectors */
      if (SIGNAL_Setup(pPhone, pConn) == IFX_ERROR)
         ret = IFX_ERROR;

      /* restore channel settings. */
      ret = SIGNAL_RestoreChannel(pPhone, pConn);

      if (ret == IFX_SUCCESS)
      {
         if (pConn->bT38_PeerEndTransm)
         {
            /* Peer finished T.38 connection. Start voice codec. */
            ret = VOIP_StartCodec(pConn->nUsedCh, pConn,
                                  pPhone->pBoard, pPhone);
            /* set new state and reset event */
            pPhone->rgStateMachine[FXS_SM].nState = S_ACTIVE;
            if (IFX_SUCCESS == ret)
            {
               COM_ITM_TRACES(pCtrl, pPhone, pConn, IFX_NULL,
                              TTP_T_38_CODEC_START, pPhone->nSeqConnId);
            }
         }
         else
         {
            /* set new state and reset event */
            pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38_END;
         }
      }
      /* Send voice request to the peer. */
      TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_T38_END, pPhone->nSeqConnId);
   }
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38Req_EndDataTrans() */

/**
   Action when detected IE_T38_END event for S_FAXT38_REQ state.
   Peer informed us that finished T.38 transmission.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38Req_T38End(CTRL_STATUS_t* pCtrl,
                                     PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   pConn->bT38_PeerEndTransm = IFX_TRUE;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38Req_T38End() */

/**
   Action when detected IE_T38_REQ, IE_T38_ACK events for S_FAXT38_REQ state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
    IE_T38_REQ means that both sides detected DIS and sent IE_T38_REQ.
    Start T.38 transmission.
    IE_T38_ACK is not sent to the peer becuase we also sent IE_T38_REQ
    to the peer which now should be in S_FAXT38 state.
*/
IFX_return_t ST_FXS_FaxT38Req_T38xxx(CTRL_STATUS_t* pCtrl,
                                     PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))

   if (pPhone->pBoard->nT38_Support)
   { /* T.38 supported */
      /* activate T.38 */
      ret = TD_T38_Start(pPhone, pConn->nUsedCh);
      if (IFX_SUCCESS == ret)
      {
         /* set new state */
         pPhone->rgStateMachine[FXS_SM].nState = S_FAXT38;

         COM_ITM_TRACES(pCtrl, pPhone, pConn, IFX_NULL, TTP_SET_S_FAXT38,
                        pPhone->nSeqConnId);
      }
   }
   else
   { /* should not happend */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: (%s) Board does not support T.38 but it "
             "received S_FAXT38_REQ and is in IE_T38_REQ state. It should "
             "not happend!!!. "
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName,
             __FILE__, __LINE__));
   }
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;
   return ret;
} /* ST_FXS_FaxT38Req_EndConnection() */

/**
   Action when detected IE_HOOKON event for S_FAXT38_END state.
   Changes phone state to S_READY, communicates with other peer,
   cleans phone structure and turns off unused resources.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38End_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /* set new state and reset event */
   pPhone->rgStateMachine[FXS_SM].nState = S_READY;

   /* end connection - release resources, turn off lec, send message to peer */
   ret = ST_Active_HookOn(pCtrl, pPhone, pConn);
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38End_HookOn() */

/**
   Action when detected IE_END_CONNECTION event for S_FAXT38_END state.
   Peer finished call. Changes phone status to S_BUSYTONE and plays busy tone.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38End_EndConnection(CTRL_STATUS_t* pCtrl,
                                            PHONE_t* pPhone,
                                            CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   pPhone->rgStateMachine[FXS_SM].nState = S_BUSYTONE;

   /* end connection - release resources, play busy tone etc.. */
   ret = ST_Active_EndConnection(pCtrl, pPhone, pConn);
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
   pPhone->nIntEvent_FXS = IE_NONE;

   return ret;
} /* ST_FXS_FaxT38End_EndConnection() */

/**
   Action when detected IE_T38_END event for S_FAXT38_END state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_FXS_FaxT38End_T38End(CTRL_STATUS_t* pCtrl,
                                     PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))

   ret = VOIP_StartCodec(pConn->nUsedCh, pConn, pPhone->pBoard, pPhone);
   if (IFX_SUCCESS == ret)
   {
      COM_ITM_TRACES(pCtrl, pPhone, pConn, IFX_NULL, TTP_T_38_CODEC_START,
                     pPhone->nSeqConnId);
   }
   /* set new state */
   pPhone->rgStateMachine[FXS_SM].nState = S_ACTIVE;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
   pPhone->nIntEvent_FXS = IE_NONE;
   return ret;
} /* ST_FXS_FaxT38End_T38End() */

/* CONFIGURE STATE MACHINE FUNCTIONS */

/**
   Action when detected IE_CONFIG_PEER event for S_READY state.
   We received configure request from peer - use Common_ConfigureTAPI()
   to change phone settings.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Config_Ready_Peer(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                  CONNECTION_t* pConn)
{
   /* stay in old state and reset event */
   pPhone->rgStateMachine[CONFIG_SM].nState = S_READY;
   pPhone->nIntEvent_Config = IE_NONE;

   /* do not send configuration request */
   Common_ConfigureTAPI(pCtrl, pPhone->pBoard, pPhone, pConn->nFeatureID,
                        fd_data_ch, IFX_FALSE, IFX_FALSE, pPhone->nSeqConnId);

   return IFX_SUCCESS;
} /* ST_Config_Ready_Peer() */

/**
   Action when detected IE_CONFIG event for S_READY state.
   Changes phone state to S_ACIVE.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Config_Ready_Config(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn)
{
  /* change state and reset event */
  pPhone->rgStateMachine[CONFIG_SM].nState = S_ACTIVE;
  pPhone->nIntEvent_Config = IE_NONE;

  /* set dialed digits count */
  pPhone->nDialNrCnt = 1;
  /* set first digit star '*' */
  pPhone->pDialedNum[0] = DIGIT_STAR;

  return IFX_SUCCESS;
} /* ST_Config_Ready_Config() */

/**
   Action when detected IE_DIALING event for S_ACTIVE state.
   Checks if valid number of digits entered.
   If valid number of digits - use Common_ConfigureTAPI() to change phone
      configuration according to dialed number
      and change configure state to S_READY.
   If invalid number of digits - wait for another digit
      and stay in S_ACTIVE state.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark valid number of digits is one '*'(star digit)
              followed by four numbers
*/
IFX_return_t ST_Config_Active_Dialing(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t dialed_number = 0;
   IFX_uint32_t nIP_addr = 0;
   CALL_TYPE_t call_type = UNKNOWN_CALL_TYPE;

   /* reset event */
   pPhone->nIntEvent_Config = IE_NONE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: Dialed numbers count %d and number is %d\n",
          pPhone->nPhoneNumber, pPhone->nDialNrCnt,
          (IFX_int32_t) pPhone->pDialedNum[pPhone->nDialNrCnt-1]));

   call_type = Common_CheckDialedNum(pPhone, &dialed_number,
                                     &nIP_addr, IFX_NULL, pCtrl);
   /* valid number of digits was dialed */
   if (DTMF_COMMAND == call_type)
   {
      if (pPhone->rgStateMachine[FXS_SM].nState == S_ACTIVE)
      {
         /* phone FXS state machine is in S_ACTIVE state - call in progress
            - send configuration request to peer if this option
            is avaible for this DTMF command. */
         ret = Common_ConfigureTAPI(pCtrl, pPhone->pBoard, pPhone,
                                    dialed_number, fd_data_ch, IFX_FALSE,
                                    IFX_TRUE, pPhone->nSeqConnId);
      }
      else
      {
         /* config all phones and do not send configuration request */
         ret = Common_ConfigureTAPI(pCtrl, pPhone->pBoard, pPhone,
                                    dialed_number, fd_data_ch, IFX_TRUE,
                                    IFX_FALSE, pPhone->nSeqConnId);
      }
      /* change phone state */
      pPhone->rgStateMachine[CONFIG_SM].nState = S_READY;

      pPhone->nDialNrCnt = 0;
      pPhone->fConfTAPI_InConn = IFX_FALSE;

   }
   /* check if number of digits needed for DTMF_COMMAND is exceeded */
   else if (5 < pPhone->nDialNrCnt)
   {
      /* change phone state */
      pPhone->rgStateMachine[CONFIG_SM].nState = S_READY;
   }
   else
   {
      /* stay in old phone state */
      pPhone->rgStateMachine[CONFIG_SM].nState = S_ACTIVE;
   }

   return ret;
} /* ST_Config_Active_Dialing() */

/**
   Action when detected IE_HOOKON event for S_ACTIVE state.
   Changes phone state to S_READY.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
*/
IFX_return_t ST_Config_Active_HookOn(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                     CONNECTION_t* pConn)
{
   /* change phone state and reset event */
   pPhone->rgStateMachine[CONFIG_SM].nState = S_READY;
   pPhone->nIntEvent_Config = IE_NONE;

   return IFX_SUCCESS;
} /* ST_Config_Active_HookOn() */





