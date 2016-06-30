#ifndef _VOIP_H
#define _VOIP_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : voip.h
   Date        : 2006-04-04
   Description : This file enchance tapi demo with VOIP support.
 ****************************************************************************
   \file  voip.h

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */


#ifndef TAPI_VERSION4
/** Out of band payload type
    should be payload value unused by any codec
    (check RTP Payload Type Tables) */
#define TD_DEFAULT_PKT_RTP_OOB_PAYLOAD 0x62
#else /* TAPI_VERSION4 */
/** Out of band payload type
    should be payload value unused by any codec
    (check RTP Payload Type Tables) */
#define TD_DEFAULT_PKT_RTP_OOB_PAYLOAD 0x60
#endif /* TAPI_VERSION4 */

#ifdef TAPI_VERSION4
/** Default value for RTP config */
#define TD_DEFAULT_PKT_RTP_CFG_SEQ_NR 0xA
/** Default value for RTP config */
#define TD_DEFAULT_PKT_RTP_CFG_SSRC 0xB
#else /* TAPI_VERSION4 */
/** Default value for RTP config */
#define TD_DEFAULT_PKT_RTP_CFG_SSRC 0
/** Default value for RTP config */
#define TD_DEFAULT_PKT_RTP_CFG_SEQ_NR 0
#endif

/** Default value for RTP config */
#define TD_DEFAULT_PKT_RTP_CFG_EVENT 0x7E

#ifndef TAPI_VERSION4
/** Default value for RTP config */
#define TD_DEFAULT_PKT_RTP_CFG_EVENT_PLAY 0x62
#else /* TAPI_VERSION4 */
/** Default value for RTP config */
#define TD_DEFAULT_PKT_RTP_CFG_EVENT_PLAY 0x60
#endif /* TAPI_VERSION4 */

/** Default value for RTP config */
#define TD_DEFAULT_PKT_RTP_CFG_TIME_STAMP 0xC

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/** this value should be checked,
   it's value should the same as number of IFX_TAPI_COD_LENGTH_t enums */
#define FRAME_SIZE_COUNT 11

/** Max size of packet in bytes. */
#define MAX_PACKET_SIZE    800
/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/** Default encoder type */
enum { TD_DEFAULT_CODER_TYPE = IFX_TAPI_ENC_TYPE_MLAW };

/** Packetisation time is no longer given in ms
    but described by enum value from IFX_TAPI_COD_LENGTH_t */
enum { TD_DEFAULT_PACKETISATION_TIME = IFX_TAPI_COD_LENGTH_10 };

/* ============================= */
/* Global Variable declaration   */
/* ============================= */

extern IFX_char_t* pCodecName[IFX_TAPI_ENC_TYPE_MAX + 1];
extern IFX_char_t* oFrameLen[FRAME_SIZE_COUNT];
#ifdef EASY336
extern IFX_TAPI_ENC_TYPE_t eCurrEncType;
#endif /* EASY336 */

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_void_t VOIP_SetCodecNames(IFX_void_t);
IFX_return_t VOIP_Init(PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard);
IFX_return_t VOIP_Release(PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard);
IFX_return_t VOIP_CheckEncoderType(IFX_int32_t nEncoderType);
IFX_return_t VOIP_SetEncoderType(BOARD_t* pBoard, IFX_int32_t nEncoderType,
                                 IFX_uint32_t nSeqConnId);
IFX_return_t VOIP_SetFrameLen(IFX_int32_t nFrameLen, IFX_uint32_t nSeqConnId);
IFX_return_t VOIP_SetDefaultFrameLen(IFX_uint32_t nSeqConnId);
IFX_int32_t VOIP_GetCodecDefaultFrameLen(IFX_TAPI_ENC_TYPE_t nCodec);
IFX_return_t VOIP_CodecAvailabilityCheck(BOARD_t* pBoard,
                                         IFX_TAPI_COD_TYPE_t nCodType,
                                         IFX_uint32_t nSeqConnId);
IFX_return_t VOIP_SetCodec(CTRL_STATUS_t* pCtrl);
extern IFX_return_t VOIP_StartCodec(IFX_int32_t nDataCh, CONNECTION_t* pConn, 
                                    BOARD_t* pBoard, PHONE_t* pPhone);
extern IFX_return_t VOIP_StopCodec(IFX_int32_t nDataCh, CONNECTION_t* pConn, 
                                   BOARD_t* pBoard, PHONE_t* pPhone);
#ifdef TAPI_VERSION4
IFX_return_t VOIP_FaxStart(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn, 
                           BOARD_t* pBoard, PHONE_t* pPhone);
IFX_return_t VOIP_FaxStop(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn, 
                          BOARD_t* pBoard, PHONE_t* pPhone);
#endif /* TAPI_VERSION4 */
IFX_void_t VOIP_SetCodecFlag(IFX_int32_t nDataCh, IFX_boolean_t fStartCodec,
                             BOARD_t* pBoard);

IFX_int32_t VOIP_GetFreeDataCh(BOARD_t* pBoard, IFX_uint32_t nSeqConnId);
IFX_return_t VOIP_FreeDataCh(IFX_int32_t nDataCh, BOARD_t* pBoard,
                             IFX_uint32_t nSeqConnId);
IFX_return_t VOIP_ReserveDataCh(IFX_int32_t nDataCh, BOARD_t* pBoard,
                                IFX_uint32_t nSeqConnId);

IFX_void_t VOIP_ReleaseSockets(CTRL_STATUS_t* pCtrl);
IFX_int32_t VOIP_GetSocket(IFX_int32_t nChIdx, BOARD_t* pBoard,
                           IFX_boolean_t bIPv6Call);
IFX_int32_t VOIP_GetSocketPort(IFX_int32_t nSocket, CTRL_STATUS_t* pCtrl);

IFX_return_t VOIP_MapPhoneToData(IFX_int32_t nDataCh, IFX_uint16_t nDev,
                                 IFX_int32_t nAddPhoneCh,
                                 IFX_boolean_t fDoMapping,
                                 BOARD_t* pBoard, IFX_uint32_t nSeqConnId);
#ifdef TD_IPV6_SUPPORT
IFX_int32_t VOIP_InitUdpSocketIPv6(CTRL_STATUS_t *pCtrl,
                                   PROGRAM_ARG_t* pProgramArg,
                                   IFX_int32_t nChIdx,
                                   IFX_int32_t* nPort);
#endif /* TD_IPV6_SUPPORT */
IFX_int32_t VOIP_InitUdpSocket(PROGRAM_ARG_t* pProgramArg,
                               IFX_int32_t nChIdx,
                               IFX_int32_t* nPort);
IFX_return_t VOIP_HandleSocketData(PHONE_t* pPhone,
                                   CONNECTION_t* pConn,
                                   IFX_boolean_t fHandleData);
IFX_return_t VOIP_HandleData(CTRL_STATUS_t* pCtrl,
                             PHONE_t* pPhone,
                             IFX_int32_t nDataCh_FD,
                             CONNECTION_t* pConn,
                             IFX_boolean_t fHandleData);
IFX_return_t VOIP_DiscardData(IFX_int32_t nFd);
IFX_int32_t VOIP_GetFD_OfCh(IFX_int32_t nDataCh, BOARD_t* pBoard);

#endif /* _VOIP_H */

