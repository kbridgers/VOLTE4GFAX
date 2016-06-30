#ifndef _PCM_H
#define _PCM_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : pcm.h
   Date        : 2006-03-29
   Description : This file enchance tapi demo with PCM support.
 ****************************************************************************
   \file pcm.h

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ALaw, uLaw, Linear and 8 or 16 bit codecs can be used. */

#if !defined EASY3201 && !defined EASY3201_EVS
/** default narrow band resolution */
#define TD_DEFAULT_PCM_NB_RESOLUTION IFX_TAPI_PCM_RES_NB_ALAW_8BIT
/** default wide band resolution */
#define TD_DEFAULT_PCM_WB_RESOLUTION IFX_TAPI_PCM_RES_WB_ALAW_8BIT
/** used for print */
#define TD_STR_TAPI_PCM_NB_RESOLUTION_DEFAULT \
   "IFX_TAPI_PCM_RES_NB_ALAW_8BIT"
#else /* EASY3201 */
/** default narrow band resolution */
#define TD_DEFAULT_PCM_NB_RESOLUTION IFX_TAPI_PCM_RES_NB_LINEAR_16BIT
/** default wide band resolution */
#define TD_DEFAULT_PCM_WB_RESOLUTION IFX_TAPI_PCM_RES_WB_LINEAR_16BIT
/** used for print */
#define TD_STR_TAPI_PCM_NB_RESOLUTION_DEFAULT \
   "IFX_TAPI_PCM_RES_NB_LINEAR_16BIT"
#endif /* EASY3201 */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/** timeslots direction */
typedef enum _TS_DIRECTION_t
{
   /** receiving timeslot */
   TS_RX = 1,
   /** transmiting timeslot */
   TS_TX = 2
} TS_DIRECTION_t;

/** Flag for no pcm channel */
enum { NO_FREE_PCM_CH = -1 };

/** Flag for no timeslot */
enum { NO_FREE_TIMESLOT = -1 };

/** Flag for timeslot is not set */
enum { TIMESLOT_NOT_SET = -1 };

/** structure to assign codec type to number of timeslots and
    band it requires */
typedef struct _TD_CODEC_TIMESLOT_t
{
   /** Type of PCM codec */
   IFX_TAPI_PCM_RES_t nCodecType;
   /** Number of timeslots used in one direction */
   IFX_int32_t nTimeslotsNum;
   /** Codec band (NB/WB) */
   TD_PCM_BAND_t nBand;
} TD_CODEC_TIMESLOT_t;

/* ============================= */
/* Global Variable declaration   */
/* ============================= */

extern IFX_int32_t const TAPIDEMO_PCM_FREQ_MAX_TIMESLOT_NUMBER[];

/* ============================= */
/* Global function declaration   */
/* ============================= */
#ifdef TD_HAVE_DRV_BOARD_HEADERS
IFX_return_t PCM_ConfigBoardDrv(IFX_boolean_t fMaster, BOARD_t* pBoard,
                                IFX_boolean_t fLocalLoop);
#endif /* TD_HAVE_DRV_BOARD_HEADERS */
IFX_int32_t PCM_GetTimeslots(TS_DIRECTION_t nTS_direction,
                             IFX_boolean_t fWideBand, IFX_uint32_t nSeqConnId);
IFX_return_t PCM_FreeTimeslots(IFX_int32_t nTimeslot, IFX_uint32_t nSeqConnId);
IFX_return_t PCM_Init(PROGRAM_ARG_t* pProgramArg);
IFX_return_t PCM_SetActivation(TAPIDEMO_PORT_DATA_t* pPortData,
                            IFX_int32_t nPCM_Ch_FD, IFX_boolean_t fActivate,
                            IFX_int32_t nTimeslot_RX, IFX_int32_t nTimeslot_TX,
                            IFX_boolean_t fWideBand, IFX_uint32_t nSeqConnId);
IFX_boolean_t PCM_CheckPhoneNum(IFX_char_t* prgnDialNum,
                                IFX_int32_t nDialNrCnt);
IFX_boolean_t PCM_StartConnection(TAPIDEMO_PORT_DATA_t* pPortData,
                                  PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard, 
                                  CONNECTION_t* pConn, IFX_uint32_t nSeqConnId);
IFX_boolean_t PCM_EndConnection(TAPIDEMO_PORT_DATA_t* pPortData,
                                PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard,
                                CONNECTION_t* pConn, IFX_uint32_t nSeqConnId);
IFX_return_t PCM_MapToPhone(IFX_int32_t nDstCh, IFX_int32_t nAddCh,
                            IFX_boolean_t fDoMapping,
                            BOARD_t* pBoard, IFX_uint32_t nSeqConnId);
IFX_return_t PCM_MapToPCM(IFX_int32_t nDstCh, IFX_int32_t nAddCh,
                          IFX_boolean_t fDoMapping,
                          BOARD_t* pBoard, IFX_uint32_t nSeqConnId);
IFX_return_t PCM_MapToData(IFX_int32_t nDstCh, IFX_int32_t nAddCh,
                           IFX_boolean_t fDoMapping,
                           BOARD_t* pBoard, IFX_uint32_t nSeqConnId);
IFX_return_t PCM_FreeCh(IFX_int32_t nPCM_Ch, BOARD_t* pBoard);
IFX_return_t PCM_ReserveCh(IFX_int32_t nPCM_Ch, BOARD_t* pBoard,
                           IFX_uint32_t nSeqConnId);
IFX_int32_t PCM_GetFreeCh(BOARD_t* pBoard);
IFX_int32_t PCM_GetFD_OfCh(IFX_int32_t nPCM_Ch, BOARD_t* pBoard);
IFX_return_t PCM_CfgInterface(BOARD_t* pBoard);
IFX_return_t PCM_Release(PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard);
IFX_return_t PCM_CodecSet(IFX_int32_t nPCM_Codec);
IFX_int32_t PCM_CodecBand(IFX_int32_t nPCM_Codec);

#endif /* _PCM_H */

