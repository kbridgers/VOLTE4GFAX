#ifndef _COMMON_FXO_H
#define _COMMON_FXO_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file common_fxo.h
   \date 2007-05-31
   \brief Function prototypes for FXO.

*/

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

#ifdef DUSLIC_FXO
/** duslic fxo name */
#define DUSLIC_FXO_NAME          "Clare"
#endif /* DUSLIC_FXO */

#ifdef TERIDIAN_FXO
/** teridian fxo name */
#define TERIDIAN_FXO_NAME        "Teridian"
#endif /* TERIDIAN_FXO */

#ifdef SLIC121_FXO
/** SLIC 121 fxo name */
#define SLIC121_FXO_NAME         "Slic121"
#endif /* SLIC121_FXO */

/** both 'A' and 'D' can used as start digit for DTMF CID */
#define TD_CID_DTMF_DIGIT_START_A            'A'
/** both 'A' and 'D' can used as start digit for DTMF CID */
#define TD_CID_DTMF_DIGIT_START_D            'D'
/** 'C' is used as stop digit for DTMF CID */
#define TD_CID_DTMF_DIGIT_STOP_C             'C'

/** when start and stop digits are used at least one more digit for number must
    be detected for DTMF CID */
#define TD_CID_DTMF_DIGIT_CNT_MIN            3
/** number of all start/stop digits used for DTMF CID*/
#define TD_CID_DTMF_START_STOP_DIGIT_CNT     2

/** start initialiazation string */
#define TD_FXO_DEV_INIT_START    "start initialization"
/** device initialiazation ended successfully string */
#define TD_FXO_DEV_INIT_END      "initialization ended"
/** device initialiazation ended with error string */
#define TD_FXO_DEV_INIT_FAILED   "initialization failed"

/** Print initialiation string.
    pFxoSetupData - pointer to initialized FXO_SETUP_DATA_t structure
    pString - null terminated string */
#define TD_FXO_TRACE_DEV_INIT(pFxoSetupData, pString) \
   do \
   { \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT, \
            ("-----  %s device No %d (%s%s%d0) %s.  -----\n", \
             pFxoSetupData->pFxoType->rgName, pFxoSetupData->nFxoDevNum, \
             TD_DEV_PATH, \
             Common_Enum2Name(pFxoSetupData->pFxoType->nID, TD_rgFxoDevName), \
             pFxoSetupData->nFxoDevNum, pString)); \
   } while (0)

/** FXO device prefix is created from "%s No %d "
    where %s replaced with FXO device_name and %d is replaced with device number.
    Trace indention length is length of 
    device_name + 4 (strlen(" No ")) + 1 (device number is in range <1,9>) +2 (strlen(": ")),
    last three elements are constant and TD_FXO_INDENT is a sum of them */
#define TD_FXO_INDENT                     7

/** Maximal number of FXO devices of one type (teridian or duslic/clare) */
#define TD_FXO_MAX_DEVICE_NUMBER          9
/** Maximal number of FXO channels on one device */
#define TD_FXO_MAX_CHANNEL_NUMBER         9

/** reset setup data with default values */
#define FXO_RESET_SETUP_DATA(oSetupData) \
   do \
   { \
      oSetupData.pFxoType = TD_rgFxoName; \
      oSetupData.nFxoDevNum = NO_DEV_SPECIFIED; \
      oSetupData.nDevFd = -1; \
   } while (0)

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/** FXO Setup data - used to initialize FXOs */
typedef struct _FXO_SETUP_DATA_t
{
   /** pair of FXO type enum and name string */
   TD_ENUM_2_NAME_t* pFxoType;
   /** number of device */
   IFX_int32_t nFxoDevNum;
   /** device FD */
   IFX_int32_t nDevFd;
} FXO_SETUP_DATA_t;

/** data of FXO channels FDs */
typedef struct _FXO_CHANNELS_FDS_t
{
   /** table of FDs */
   IFX_int32_t rgoFD[TD_FXO_MAX_CHANNEL_NUMBER];
   /** number of opened FDs */
   IFX_int32_t nFxoChNum;
} FXO_CHANNELS_FDS_t;

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Global variable declaration   */
/* ============================= */

/** FXO devices names */
extern TD_ENUM_2_NAME_t TD_rgFxoDevName[];

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t FXO_Setup(BOARD_t* pBoard);
IFX_char_t* FXO_SetFxoNameString(IFX_char_t* pDevName, IFX_int32_t nDevNum,
                                 IFX_int32_t nType, IFX_int32_t nChNum);
IFX_return_t FXO_GetVersion(TAPIDEMO_DEVICE_FXO_t* pFxoDevice);
IFX_return_t FXO_CleanUpFxoDevice(TAPIDEMO_DEVICE_FXO_t* pFxoDevice);
FXO_t* FXO_GetFxoOfDataCh(IFX_int32_t nDataCh, CTRL_STATUS_t* pCtrl);
IFX_return_t FXO_DTMF_DetectionStart(FXO_t* pFXO, BOARD_t* pBoard);
IFX_return_t FXO_DTMF_DetectionStop(FXO_t* pFXO, BOARD_t* pBoard);
IFX_return_t FXO_FlashHook(FXO_t* pFXO);
PHONE_t* FXO_GetPhone(CTRL_STATUS_t* pCtrl, IFX_int32_t nFxoNum);
IFX_return_t FXO_ReceivedCID_DTMF_Send(FXO_t* pFXO, BOARD_t* pBoard);
IFX_return_t FXO_RefreshTimeout(FXO_t* pFXO);
IFX_return_t FXO_StartConnection(PROGRAM_ARG_t* pProgramArg,
                                 FXO_t* pFXO,
                                 PHONE_t* pPhone,
                                 CONNECTION_t* pConn,
                                 BOARD_t* pBoard);
IFX_return_t FXO_EndConnection(PROGRAM_ARG_t* pProgramArg,
                               FXO_t* pFXO,
                               PHONE_t* pPhone,
                               CONNECTION_t* pConn,
                               BOARD_t* pBoard);
IFX_return_t FXO_PrepareConnection(PROGRAM_ARG_t* pProgramArg,
                                   FXO_t* pFXO,
                                   BOARD_t* pBoard);
IFX_return_t FXO_ActivateConnection(FXO_t* pFXO,
                                    PHONE_t* pPhone,
                                    CONNECTION_t* pConn,
                                    BOARD_t* pBoard);


#endif /* _COMMON_FXO_H */
