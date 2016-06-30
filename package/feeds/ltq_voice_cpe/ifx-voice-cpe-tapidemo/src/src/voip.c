/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/


/**
   \file voip.c
   \date 2006-04-04
   \brief Working with coder and signal module.

   This file implements methods to handle with data channel resources, channel
   mapping, start/stop coder/encoder, hanling data packets from dev or socket,
   initialization, ...
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "ifx_types.h"
#include "tapidemo.h"
#include "cid.h"
#include "voip.h"

#ifdef EASY336
#include "svip_io.h"
#include "board_easy336.h"
#include "lib_svip.h"
#include "lib_svip_rm.h"
#include "common.h"
#endif /* EASY336 */
#include "abstract.h"

/* ============================= */
/* Global Defines                */
/* ============================= */

#ifdef EASY336
#define IFX_SVIP_SYS_COD_NUM  (1*16)
#define IFX_SVIP_UDP_END      ((IFX_SVIP_UDP_START) + (IFX_SVIP_SYS_COD_NUM))
/** VoFW destination UDP port number. */
#define FW_PORT     6000
#endif /* EASY336 */

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Array of pointers to strings with codecs names. */
IFX_char_t* pCodecName[IFX_TAPI_ENC_TYPE_MAX + 1];
/** Array to convert enums IFX_TAPI_COD_LENGTH_t
    to coresponding values names. */
IFX_char_t* oFrameLen[FRAME_SIZE_COUNT];

/** Some of the configuration for VoIP module */
TD_VOIP_CFG_t oVoipCfg = {
   /* when changing default values please update help description */
   { IFX_FALSE, IFX_TAPI_PKT_EV_OOB_NO, IFX_TAPI_PKT_EV_OOBPLAY_PLAY }
};

/* --------------------------------------------------------- */
/*             RTP Payload Type Tables START                 */
/* --------------------------------------------------------- */

/** Payload types table - Upstream */
enum
{
   RTP_ENC_TYPE_MLAW_UP = 0x00,
   RTP_ENC_TYPE_ALAW_UP = 0x08,
   RTP_ENC_TYPE_G729_UP = 0x12,
   RTP_ENC_TYPE_G723_53_UP = 0x04,
   RTP_ENC_TYPE_G723_63_UP = 0x04,
#ifndef STREAM_1_1
   RTP_ENC_TYPE_G7221_24_UP = 0x73,
   RTP_ENC_TYPE_G7221_32_UP = 0x74,
   RTP_ENC_TYPE_G722_64_UP = 0x09,
#endif /* STREAM_1_1 */
   RTP_ENC_TYPE_G728_UP = 0x0F,
   RTP_ENC_TYPE_G729_E_UP = 0x61,
   RTP_ENC_TYPE_G726_16_UP = 0x63,
   RTP_ENC_TYPE_G726_24_UP = 0x64,
   RTP_ENC_TYPE_G726_32_UP = 0x65,
   RTP_ENC_TYPE_G726_40_UP = 0x66,
   RTP_ENC_TYPE_ILBC_133_UP = 0x67,
   RTP_ENC_TYPE_ILBC_152_UP = 0x67,
   RTP_ENC_TYPE_LIN16_16_UP = 0x70,
   RTP_ENC_TYPE_LIN16_8_UP = 0x71,
   RTP_ENC_TYPE_AMR_4_75_UP = 0x72,
   RTP_ENC_TYPE_AMR_5_9_UP = 0x72,
   RTP_ENC_TYPE_AMR_5_15_UP = 0x72,
   RTP_ENC_TYPE_AMR_6_7_UP = 0x72,
   RTP_ENC_TYPE_AMR_7_4_UP = 0x72,
   RTP_ENC_TYPE_AMR_7_95_UP = 0x72,
   RTP_ENC_TYPE_AMR_10_2_UP = 0x72,
   RTP_ENC_TYPE_AMR_12_2_UP = 0x72,
   RTP_ENC_TYPE_MLAW_VBD_UP = 0x75,
   RTP_ENC_TYPE_ALAW_VBD_UP = 0x76

};

/** Payload types table - Downstream */
enum
{
   RTP_ENC_TYPE_MLAW_DOWN = 0x00,
   RTP_ENC_TYPE_ALAW_DOWN = 0x08,
   RTP_ENC_TYPE_G729_DOWN = 0x12,
   RTP_ENC_TYPE_G723_53_DOWN = 0x04,
   RTP_ENC_TYPE_G723_63_DOWN = 0x04,
#ifndef STREAM_1_1
   RTP_ENC_TYPE_G7221_24_DOWN = 0x73,
   RTP_ENC_TYPE_G7221_32_DOWN = 0x74,
   RTP_ENC_TYPE_G722_64_DOWN = 0x09,
#endif /* STREAM_1_1 */
   RTP_ENC_TYPE_G728_DOWN = 0x0F,
   RTP_ENC_TYPE_G729_E_DOWN = 0x61,
   RTP_ENC_TYPE_G726_16_DOWN = 0x63,
   RTP_ENC_TYPE_G726_24_DOWN = 0x64,
   RTP_ENC_TYPE_G726_32_DOWN = 0x65,
   RTP_ENC_TYPE_G726_40_DOWN = 0x66,
   RTP_ENC_TYPE_ILBC_133_DOWN = 0x67,
   RTP_ENC_TYPE_ILBC_152_DOWN = 0x67,
   RTP_ENC_TYPE_LIN16_16_DOWN = 0x70,
   RTP_ENC_TYPE_LIN16_8_DOWN = 0x71,
   RTP_ENC_TYPE_AMR_4_75_DOWN = 0x72,
   RTP_ENC_TYPE_AMR_5_9_DOWN = 0x72,
   RTP_ENC_TYPE_AMR_5_15_DOWN = 0x72,
   RTP_ENC_TYPE_AMR_6_7_DOWN = 0x72,
   RTP_ENC_TYPE_AMR_7_4_DOWN = 0x72,
   RTP_ENC_TYPE_AMR_7_95_DOWN = 0x72,
   RTP_ENC_TYPE_AMR_10_2_DOWN = 0x72,
   RTP_ENC_TYPE_AMR_12_2_DOWN = 0x72,
   RTP_ENC_TYPE_MLAW_VBD_DOWN = 0x75,
   RTP_ENC_TYPE_ALAW_VBD_DOWN = 0x76
};

/* --------------------------------------------------------- */
/*               RTP Payload Type Tables END                 */
/* --------------------------------------------------------- */

/** Jitter buffer configuration. */

/** Packet adaptation */
static const IFX_int32_t JB_PACKET_ADAPTATION = IFX_TAPI_JB_PKT_ADAPT_VOICE;
/** Local adaptation */
static const IFX_int32_t JB_LOCAL_ADAPTATION  = IFX_TAPI_JB_LOCAL_ADAPT_ON;
/** Play out dealy, value between 0..0x13 */
static const IFX_int32_t JB_SCALING    = 0x13;
/** Initial size of jitter buffer in timestamps of 125 us */
static const IFX_int32_t JB_INITIAL_SIZE      = 0x0050;
/** Maximum size of jitter buffer in timestamps of 125 us */
static const IFX_int32_t JB_MAXIMUM_SIZE      = 0x0C80;
/** Minimum size of jitter buffer in timestamps of 125 us */
static const IFX_int32_t JB_MINIMUM_SIZE      = 0x0050;

/** Structure holding codecs with default packetisation length  */
typedef struct _CODEC_LEN_t
{
   IFX_TAPI_ENC_TYPE_t nCodecName;
   IFX_TAPI_COD_LENGTH_t nDefaultLenght;
} CODEC_LEN_t;

/** List of combinations of codecs with default packetisation length */
static const CODEC_LEN_t CODEC_LEN[] =
{
   { IFX_TAPI_ENC_TYPE_G723_63, IFX_TAPI_COD_LENGTH_30 },
   { IFX_TAPI_ENC_TYPE_G723_53, IFX_TAPI_COD_LENGTH_30 },
   /* Codec below doesn't work */
   { IFX_TAPI_ENC_TYPE_G728, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_G729, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_MLAW, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_ALAW, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_MLAW_VBD, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_ALAW_VBD, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_G726_16, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_G726_24, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_G726_32, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_G726_40, IFX_TAPI_COD_LENGTH_10 },
   /* Codec below doesn't work */
   { IFX_TAPI_ENC_TYPE_G729_E, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_ILBC_133, IFX_TAPI_COD_LENGTH_30 },
   { IFX_TAPI_ENC_TYPE_ILBC_152, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_LIN16_8, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_LIN16_16, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_AMR_4_75, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_AMR_5_15, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_AMR_5_9, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_AMR_6_7, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_AMR_7_4, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_AMR_7_95, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_AMR_10_2, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_AMR_12_2, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_G722_64, IFX_TAPI_COD_LENGTH_10 },
   { IFX_TAPI_ENC_TYPE_G7221_24, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_G7221_32, IFX_TAPI_COD_LENGTH_20 },
   { IFX_TAPI_ENC_TYPE_MAX, IFX_TAPI_COD_LENGTH_ZERO }
};

/** Current encoder type to use */
IFX_TAPI_ENC_TYPE_t eCurrEncType = TD_DEFAULT_CODER_TYPE;

/** Current packetisation time to use */
static IFX_int32_t nCurrPacketTime = TD_DEFAULT_PACKETISATION_TIME;

/** Number of free socket available */
static IFX_int32_t nFreeSocketPortAvail = 0;

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/**
   Function to set array of pointers to codec names.

   \return IFX_void_t
*/
IFX_void_t VOIP_SetCodecNames(IFX_void_t)
{
   IFX_int32_t i;

   for (i=0; i<IFX_TAPI_ENC_TYPE_MAX + 1; i++)
   {
      /* set begining value */
      pCodecName[i] = "NO CODEC";
   }

   /* point to strings with codecs names */
   pCodecName[IFX_TAPI_ENC_TYPE_G723_63] = "G.723.1 with 6.3kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G723_53] = "G.723.1 with 5.3kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G728] = "G.728 with 16kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G729] = "G.729A with 8kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_MLAW] = "G.711 u-Law with 64kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_ALAW] = "G.711 A-Law with 64kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_MLAW_VBD] = "G.711 u-Law VBD with 64kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_ALAW_VBD] = "G.711 A-Law VBD with 64kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G726_16] = "G.726 with 16kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G726_24] = "G.726 with 24kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G726_32] = "G.726 with 32kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G726_40] = "G.726 with 40kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G729_E] = "G.729E with 11.8kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_ILBC_133] = "iLBC with 13.3kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_ILBC_152] = "iLBC with 15.2kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_LIN16_8] = "Linear Codec with 8kHz";
   pCodecName[IFX_TAPI_ENC_TYPE_LIN16_16] = "Linear Codec with 16kHz";
   pCodecName[IFX_TAPI_ENC_TYPE_AMR_4_75] = "AMR with 4.75kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_AMR_5_15] = "AMR with 5.15kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_AMR_5_9] = "AMR with 5.9kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_AMR_6_7] = "AMR with 6.7kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_AMR_7_4] = "AMR with 7.4kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_AMR_7_95] = "AMR with 7.95kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_AMR_10_2] = "AMR with 10.2kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_AMR_12_2] = "AMR with 12.2kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G722_64] = "G.722 with 64kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G7221_24] = "G.722.1 with 24kbit/s";
   pCodecName[IFX_TAPI_ENC_TYPE_G7221_32] = "G.722.1 with  32kbit/s";

   /* set frames length in ms */
   oFrameLen[IFX_TAPI_COD_LENGTH_ZERO]="0ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_2_5]="2.5ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_5]="5ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_5_5]="5.5ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_10]="10ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_11]="11ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_20]="20ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_30]="30ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_40]="40ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_50]="50ms";
   oFrameLen[IFX_TAPI_COD_LENGTH_60]="60ms";

   return;
}

/** Description of IFX_TAPI_PKT_EV_OOB_t values. */
TD_ENUM_2_NAME_t TD_rgTapiOOB_EventDesc[] =
{
   {IFX_TAPI_PKT_EV_OOB_DEFAULT, "default"},
   {IFX_TAPI_PKT_EV_OOB_NO,      TD_STR_TAPI_PKT_EV_OOB_NO},
   {IFX_TAPI_PKT_EV_OOB_ONLY,    TD_STR_TAPI_PKT_EV_OOB_ONLY},
   {IFX_TAPI_PKT_EV_OOB_ALL,     TD_STR_TAPI_PKT_EV_OOB_ALL},
   {IFX_TAPI_PKT_EV_OOB_BLOCK,   TD_STR_TAPI_PKT_EV_OOB_BLOCK},
   {TD_MAX_ENUM_ID,              "IFX_TAPI_PKT_EV_OOB_t"}
};

/** Description of IFX_TAPI_PKT_EV_OOB_t values. */
TD_ENUM_2_NAME_t TD_rgTapiOOB_EventPlayDesc[] =
{
   {IFX_TAPI_PKT_EV_OOBPLAY_DEFAULT,   "default"},
   {IFX_TAPI_PKT_EV_OOBPLAY_PLAY,      TD_STR_TAPI_PKT_EV_OOBPLAY_PLAY},
   {IFX_TAPI_PKT_EV_OOBPLAY_MUTE,      TD_STR_TAPI_PKT_EV_OOBPLAY_MUTE},
   {IFX_TAPI_PKT_EV_OOBPLAY_APT_PLAY,  TD_STR_TAPI_PKT_EV_OOBPLAY_ATP_PLAY},
   {TD_MAX_ENUM_ID,                    "IFX_TAPI_PKT_EV_OOBPLAY_t"}
};

/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   VoIP Init.

   \param pProgramArg  - pointer to program arguments
   \param pBoard - board handle

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t VOIP_Init(PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard)
{
   IFX_int32_t i = 0;
   IFX_TAPI_PKT_RTP_PT_CFG_t rtpPTConf;
   IFX_TAPI_JB_CFG_t jbCfgVoice;
   IFX_TAPI_PKT_RTP_CFG_t rtpConf;
   IFX_TAPI_PKT_EV_OOB_DTMF_t rtpOobDTMF;
   IFX_int32_t nRtpOobDTMF_Arg;
   IFX_return_t ret;
#ifdef TAPI_VERSION4
   IFX_TAPI_ENC_VAD_CFG_t oVadCfg;
#endif /* TAPI_VERSION4 */
   IFX_TAPI_ENC_TYPE_t nEncTypeTmp = eCurrEncType;
   IFX_int32_t nPacketTimeTmp = nCurrPacketTime;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_uint32_t nIoctlArg = TD_NOT_SET;
#ifndef STREAM_1_1
   IFX_int32_t fd_ch = -1;
#endif /* STREAM_1_1 */

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);
   if (0 >= pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Err, VOIP_Init: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_ERROR;
   }

   memset(&rtpPTConf, 0, sizeof(IFX_TAPI_PKT_RTP_PT_CFG_t));

   /* Payload types table - Upstream */

   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_MLAW]       = RTP_ENC_TYPE_MLAW_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_ALAW]       = RTP_ENC_TYPE_ALAW_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G729]       = RTP_ENC_TYPE_G729_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G723_53]    = RTP_ENC_TYPE_G723_53_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G723_63]    = RTP_ENC_TYPE_G723_63_UP;
#ifndef STREAM_1_1
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G7221_24]   = RTP_ENC_TYPE_G7221_24_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G7221_32]   = RTP_ENC_TYPE_G7221_32_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G722_64]    = RTP_ENC_TYPE_G722_64_UP;
#endif /* STREAM_1_1 */
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G726_16]    = RTP_ENC_TYPE_G726_16_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G726_24]    = RTP_ENC_TYPE_G726_24_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G726_32]    = RTP_ENC_TYPE_G726_32_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G726_40]    = RTP_ENC_TYPE_G726_40_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G728]       = RTP_ENC_TYPE_G728_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G729_E]     = RTP_ENC_TYPE_G729_E_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_ILBC_133]   = RTP_ENC_TYPE_ILBC_133_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_ILBC_152]   = RTP_ENC_TYPE_ILBC_152_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_LIN16_16]   = RTP_ENC_TYPE_LIN16_16_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_LIN16_8]    = RTP_ENC_TYPE_LIN16_8_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_AMR_4_75]   = RTP_ENC_TYPE_AMR_4_75_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_AMR_5_9]    = RTP_ENC_TYPE_AMR_5_9_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_AMR_5_15]   = RTP_ENC_TYPE_AMR_5_15_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_AMR_6_7]    = RTP_ENC_TYPE_AMR_6_7_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_AMR_7_4]    = RTP_ENC_TYPE_AMR_7_4_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_AMR_7_95]   = RTP_ENC_TYPE_AMR_7_95_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_AMR_10_2]   = RTP_ENC_TYPE_AMR_10_2_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_AMR_12_2]   = RTP_ENC_TYPE_AMR_12_2_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_MLAW_VBD]   = RTP_ENC_TYPE_MLAW_VBD_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_ALAW_VBD]   = RTP_ENC_TYPE_ALAW_VBD_UP;

   /* Payload types table - Downstream */

   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_MLAW]     = RTP_ENC_TYPE_MLAW_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_ALAW]     = RTP_ENC_TYPE_ALAW_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G729]     = RTP_ENC_TYPE_G729_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G723_53]  = RTP_ENC_TYPE_G723_53_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G723_63]  = RTP_ENC_TYPE_G723_63_DOWN;
#ifndef STREAM_1_1
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G7221_24] = RTP_ENC_TYPE_G7221_24_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G7221_32] = RTP_ENC_TYPE_G7221_32_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G722_64]  = RTP_ENC_TYPE_G722_64_DOWN;
#endif /* STREAM_1_1 */
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G726_16]  = RTP_ENC_TYPE_G726_16_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G726_24]  = RTP_ENC_TYPE_G726_24_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G726_32]  = RTP_ENC_TYPE_G726_32_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G726_40]  = RTP_ENC_TYPE_G726_40_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G728]     = RTP_ENC_TYPE_G728_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G729_E]   = RTP_ENC_TYPE_G729_E_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_ILBC_133] = RTP_ENC_TYPE_ILBC_133_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_ILBC_152] = RTP_ENC_TYPE_ILBC_152_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_LIN16_16] = RTP_ENC_TYPE_LIN16_16_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_LIN16_8]  = RTP_ENC_TYPE_LIN16_8_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_AMR_4_75] = RTP_ENC_TYPE_AMR_4_75_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_AMR_5_9]  = RTP_ENC_TYPE_AMR_5_9_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_AMR_5_15] = RTP_ENC_TYPE_AMR_5_15_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_AMR_6_7]  = RTP_ENC_TYPE_AMR_6_7_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_AMR_7_4]  = RTP_ENC_TYPE_AMR_7_4_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_AMR_7_95] = RTP_ENC_TYPE_AMR_7_95_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_AMR_10_2] = RTP_ENC_TYPE_AMR_10_2_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_AMR_12_2] = RTP_ENC_TYPE_AMR_12_2_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_MLAW_VBD] = RTP_ENC_TYPE_MLAW_VBD_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_ALAW_VBD] = RTP_ENC_TYPE_ALAW_VBD_DOWN;

   memset(&jbCfgVoice, 0, sizeof(IFX_TAPI_JB_CFG_t));

   /* Adaptive JB */
   jbCfgVoice.nJbType = IFX_TAPI_JB_TYPE_ADAPTIVE;
   /* Optimization for voice */
   jbCfgVoice.nPckAdpt = JB_PACKET_ADAPTATION;
   jbCfgVoice.nLocalAdpt = JB_LOCAL_ADAPTATION;
   jbCfgVoice.nScaling = JB_SCALING;
   /* Initial JB size 10 ms = 0x0050 * 125 us */
   jbCfgVoice.nInitialSize = JB_INITIAL_SIZE;
   /* Minimum JB size 10 ms = 0x0050 * 125 us */
   jbCfgVoice.nMinSize = JB_MINIMUM_SIZE;
   /* Maximum JB size 180 ms = 0x05A0 * 125 us */
   jbCfgVoice.nMaxSize = JB_MAXIMUM_SIZE;

   memset(&pBoard->rtpConf, 0, sizeof(pBoard->rtpConf));

   /* Set default RTP and RTCP settings */
   /* Start value for the sequence number. */
   pBoard->rtpConf.nSeqNr = TD_DEFAULT_PKT_RTP_CFG_SEQ_NR;

   /* Synchronization source value for the voice and SID packets.
      Note: A change in the SSRC leads to a reset of the RTCP statistics.*/
   pBoard->rtpConf.nSsrc = TD_DEFAULT_PKT_RTP_CFG_SSRC;

   pBoard->rtpConf.nTimestamp = TD_DEFAULT_PKT_RTP_CFG_TIME_STAMP;
   pBoard->rtpConf.nEventPlayPT = TD_DEFAULT_PKT_RTP_CFG_EVENT_PLAY;

   /*  Payload type to be used for RFC 2833 frames in encoder direction
      (upstream). */
   pBoard->rtpConf.nEventPT = TD_DEFAULT_PKT_RTP_OOB_PAYLOAD;
   /*  Defines whether the received RFC 2833 packets have to be played out.
       Set parameter as defined in \ref IFX_TAPI_PKT_EV_OOBPLAY_t enumerator. */
   pBoard->rtpConf.nPlayEvents = oVoipCfg.oRtpConf.nOobPlayEvents;
   rtpConf = pBoard->rtpConf;

   /* OOB setting for DTMF */
   memset(&rtpOobDTMF, 0, sizeof(rtpOobDTMF));
#ifndef TAPI_VERSION4
   rtpOobDTMF = oVoipCfg.oRtpConf.nOobEvents;
#else /* TAPI_VERSION4 */
   rtpOobDTMF.nTransmitMode = oVoipCfg.oRtpConf.nOobEvents;
#endif /* TAPI_VERSION4 */

   for (i = 0; i < pBoard->nMaxCoderCh; i++)
   {
      pBoard->pDataChStat[i].fMapped = IFX_FALSE;
      pBoard->pDataChStat[i].fCodecStarted = IFX_FALSE;
      pBoard->pDataChStat[i].fStartCodec = IFX_TRUE;
      pBoard->pDataChStat[i].fDefaultVad = IFX_TRUE;
      pBoard->pDataChStat[i].nCodecType = nEncTypeTmp;
      pBoard->pDataChStat[i].nFrameLen = nPacketTimeTmp;
      pBoard->pDataChStat[i].nBitPack = IFX_TAPI_COD_RTP_BITPACK;
      pBoard->pDataChStat[i].nPrevCodecType = nEncTypeTmp;
      pBoard->pDataChStat[i].nPrevFrameLen = nPacketTimeTmp;
      pBoard->pDataChStat[i].fRestoreChannel = IFX_FALSE;

      memset(&pBoard->pDataChStat[i].oPrevLECcfg, 0,
             sizeof(IFX_TAPI_WLEC_CFG_t));
      pBoard->pDataChStat[i].oPrevLECcfg.nType = pProgramArg->nLecCfg;
      pBoard->pDataChStat[i].oPrevLECcfg.bNlp = IFX_TAPI_WLEC_NLP_ON;

#ifndef STREAM_1_1
      memset(&pBoard->pDataChStat[i].tapi_jb_PrevConf, 0, \
             sizeof(IFX_TAPI_JB_CFG_t));
      pBoard->pDataChStat[i].tapi_jb_PrevConf.nJbType =
         IFX_TAPI_JB_TYPE_ADAPTIVE;
      pBoard->pDataChStat[i].tapi_jb_PrevConf.nPckAdpt = JB_PACKET_ADAPTATION;
      pBoard->pDataChStat[i].tapi_jb_PrevConf.nLocalAdpt = JB_LOCAL_ADAPTATION;
      pBoard->pDataChStat[i].tapi_jb_PrevConf.nScaling  = JB_SCALING;
      pBoard->pDataChStat[i].tapi_jb_PrevConf.nInitialSize = JB_INITIAL_SIZE;
      pBoard->pDataChStat[i].tapi_jb_PrevConf.nMaxSize = JB_MAXIMUM_SIZE;
      pBoard->pDataChStat[i].tapi_jb_PrevConf.nMinSize = JB_MINIMUM_SIZE;

      fd_ch = VOIP_GetFD_OfCh(i, pBoard);

      /* Program the channel (addressed by fd) with
         the specified payload types. */
#ifdef TAPI_VERSION4
      rtpPTConf.dev = pBoard->pDataChStat[i].nDev;
      rtpPTConf.ch = pBoard->pDataChStat[i].nCh;
      nDevTmp = rtpPTConf.dev;
#endif /* TAPI_VERSION4 */
      /* set RTP payload type table */
      ret = TD_IOCTL(fd_ch, IFX_TAPI_PKT_RTP_PT_CFG_SET, &rtpPTConf,
               nDevTmp, TD_CONN_ID_INIT);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("   ioctl failed for i = %d, (File: %s, line: %d)\n",
                i, __FILE__, __LINE__));
         return ret;
      }
      /* Set jitter buffer */
#ifdef TAPI_VERSION4
      jbCfgVoice.dev = pBoard->pDataChStat[i].nDev;
      jbCfgVoice.ch = pBoard->pDataChStat[i].nCh;
      nDevTmp = jbCfgVoice.dev;
#endif /* TAPI_VERSION4 */
      ret = TD_IOCTL(fd_ch, IFX_TAPI_JB_CFG_SET, &jbCfgVoice,
               nDevTmp, TD_CONN_ID_INIT);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("   ioctl failed for i = %d, (File: %s, line: %d)\n",
                i, __FILE__, __LINE__));
         return ret;
      }

      /* Set RTP and RTCP */
#ifdef TAPI_VERSION4
      rtpConf.dev = pBoard->pDataChStat[i].nDev;
      rtpConf.ch = pBoard->pDataChStat[i].nCh;
      nDevTmp = rtpConf.dev;
#endif /* TAPI_VERSION4 */
      ret = TD_IOCTL(fd_ch, IFX_TAPI_PKT_RTP_CFG_SET, &rtpConf,
               nDevTmp, TD_CONN_ID_INIT);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("   ioctl failed for i = %d, (File: %s, line: %d)\n",
                i, __FILE__, __LINE__));
         return ret;
      }

      /* OOB setting for DTMF */
#ifdef TAPI_VERSION4
      rtpOobDTMF.dev = pBoard->pDataChStat[i].nDev;
      rtpOobDTMF.ch = pBoard->pDataChStat[i].nCh;
      nRtpOobDTMF_Arg = (IFX_int32_t)&rtpOobDTMF;
#else /* TAPI_VERSION4 */
      nRtpOobDTMF_Arg = rtpOobDTMF;
#endif /* TAPI_VERSION4 */
      ret = TD_IOCTL(fd_ch, IFX_TAPI_PKT_EV_OOB_DTMF_SET, nRtpOobDTMF_Arg,
               nDevTmp, TD_CONN_ID_INIT);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("   ioctl failed for i = %d, (File: %s, line: %d)\n",
                i, __FILE__, __LINE__));
      }
      /* Voice Activity Detection (VAD) configuration */
#ifdef TAPI_VERSION4
      oVadCfg.dev = pBoard->pDataChStat[i].nDev;
      oVadCfg.ch = pBoard->pDataChStat[i].nCh;
      oVadCfg.vadMode = pProgramArg->nVadCfg;
      nIoctlArg = (IFX_uint32_t) &oVadCfg;
      nDevTmp = oVadCfg.dev;
#else /* TAPI_VERSION4 */
      nIoctlArg = pProgramArg->nVadCfg;
#endif /* TAPI_VERSION4 */
      ret = TD_IOCTL(fd_ch, IFX_TAPI_ENC_VAD_CFG_SET,
               nIoctlArg, nDevTmp, TD_CONN_ID_INIT);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("   ioctl failed for i = %d, vad = %d, "
                "(File: %s, line: %d)\n",
                i, pProgramArg->nVadCfg, __FILE__, __LINE__));
         return ret;
      }
#endif /* STREAM_1_1 */
   } /* for */
#ifdef EASY336
   SVIP_NAT_LibInit();
#endif /* EASY336 */

#ifdef TAPI_VERSION4
   if (pBoard->fUseSockets)
#endif
   {
      nFreeSocketPortAvail += pBoard->nMaxCoderCh;
   }
   /* set default codec and packetisation in PROGRAM_ARG_t structure */
   if (0 == pProgramArg->nEnCoderType)
   {
      pProgramArg->nEnCoderType = eCurrEncType;
   }
   if (0 == pProgramArg->nPacketisationTime)
   {
      pProgramArg->nPacketisationTime = nCurrPacketTime;
   }
   return IFX_SUCCESS;
} /* VOIP_Init() */


/**
   VoIP release.

   \param pProgramArg  - pointer to program arguments
   \param pBoard - board handle

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t VOIP_Release(PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard)
{
   return IFX_SUCCESS;
} /* VOIP_Release() */

/**
   Check if user set correct VOIP encoder type.

   \param nEncoderType - new encoder type

   \return IFX_SUCCESS if OK, IFX_ERROR if errors
*/
IFX_return_t VOIP_CheckEncoderType(IFX_int32_t nEncoderType)
{
   IFX_int32_t i;

   for(i=0;;i++)
   {
      if ( nEncoderType == CODEC_LEN[i].nCodecName )
      {
         return IFX_SUCCESS;
      }
      if ( IFX_TAPI_ENC_TYPE_MAX == CODEC_LEN[i].nCodecName )
      {
         return IFX_ERROR;
      }
   }
} /* VOIP_CheckEncoderType */

/**
   Set VOIP encoder, vocoder type.

   \param pBoard        - pointer to board
   \param nEncoderType  - new encoder type
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS if OK, IFX_ERROR if errors
   \remark this function sets only eCurrEncType,
           eCurrEncType is used during VOIP_Init() to set codec, this function
           shouldn't be used after VOIP_Init()
*/
IFX_return_t VOIP_SetEncoderType(BOARD_t* pBoard, IFX_int32_t nEncoderType,
                                 IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret;
   IFX_TAPI_ENC_TYPE_t encoder_type = TD_DEFAULT_CODER_TYPE;

   memset(&encoder_type, 0, sizeof(encoder_type));

   /* Check if user set correct encoder type */
   ret = VOIP_CheckEncoderType(nEncoderType);

   if (IFX_SUCCESS == ret)
   {
      /* Check if codec is available for specific board */
      ret = VOIP_CodecAvailabilityCheck(pBoard,
                                        (IFX_TAPI_ENC_TYPE_t) (nEncoderType),
                                        nSeqConnId);
   }

   if ((0 > nEncoderType) || (IFX_TAPI_ENC_TYPE_MAX <= nEncoderType) ||
       IFX_ERROR == ret)
   {
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("User encoder type is invalid. Used default one.\n"));

      /* Don't send error, but use default value. */
      encoder_type = TD_DEFAULT_CODER_TYPE;
   }
   else
   {
      encoder_type = (IFX_TAPI_ENC_TYPE_t) (nEncoderType);
   }

   eCurrEncType = encoder_type;

   return IFX_SUCCESS;
} /* VOIP_SetEncoderType() */


/**
   Set packetization time to be used.

   \param nFrameLen - number of frame len (packetisation time) enum
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS if OK, IFX_ERROR if errors
   \remark this function sets only nCurrPacketTime according to eCurrEncType,
           eCurrEncType is used during VOIP_Init() to set codec, this function
           shouldn't be used after VOIP_Init()
*/
IFX_return_t VOIP_SetFrameLen(IFX_int32_t nFrameLen, IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret;
   if ((0 >= nFrameLen) || (FRAME_SIZE_COUNT <= nFrameLen))
   {
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("User packetisation time is invalid. Used default one.\n"));

      /* Don't return error, but use default value. */
      ret = VOIP_SetDefaultFrameLen(nSeqConnId);
      return ret;
   }
   else
   {
      nCurrPacketTime = nFrameLen;
   }

   return IFX_SUCCESS;
} /* VOIP_SetFrameLen() */

/**
   Set default packetization time.

   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS if OK, IFX_ERROR if errors
   \remark this function sets only nCurrPacketTime according to eCurrEncType,
           eCurrEncType is used during VOIP_Init() to set codec, this function
           shouldn't be used after VOIP_Init()
*/
IFX_return_t VOIP_SetDefaultFrameLen(IFX_uint32_t nSeqConnId)
{
   IFX_int32_t frame_len = IFX_ERROR;

   /* Get default frame length */
   frame_len = VOIP_GetCodecDefaultFrameLen(eCurrEncType);

   if ((0 >= frame_len) || (FRAME_SIZE_COUNT < frame_len))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("Err, Can not find default frame length for encoder type %s,\n"
            "(File: %s, line: %d)\n",
            pCodecName[eCurrEncType], __FILE__, __LINE__));
      return IFX_ERROR;
   }

   nCurrPacketTime = frame_len;

   return IFX_SUCCESS;
} /* VOIP_SetDefaultFrameLen() */

/**
   Get default packetization time.

   \param nCodec  - number of codec

   \return lowest supported packetization size for selected codec,
           IFX_ERROR if not found
*/
IFX_int32_t VOIP_GetCodecDefaultFrameLen(IFX_TAPI_ENC_TYPE_t nCodec)
{
   CODEC_LEN_t* pTmpCodLen;

   pTmpCodLen = (CODEC_LEN_t*)CODEC_LEN;

   /* look through table to get default packetization */
   do
   {
      if (nCodec == pTmpCodLen->nCodecName)
      {
         /* return found packetisation */
         return pTmpCodLen->nDefaultLenght;
      }
      pTmpCodLen++;
   }
   while (IFX_TAPI_ENC_TYPE_MAX != pTmpCodLen->nCodecName);

   return IFX_ERROR;
} /* VOIP_GetCodecDefaultFrameLen() */

/**
   Check if codec is available.

   \param pBoard     - polinter to board structure
   \param nCodType   - codec type number
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS if codec supported, IFX_ERROR if not or error
*/
IFX_return_t VOIP_CodecAvailabilityCheck(BOARD_t* pBoard,
                                         IFX_TAPI_COD_TYPE_t nCodType,
                                         IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_CAP_t oCapList;
   IFX_int32_t nRet, nFd;
#ifdef TAPI_VERSION4
   IFX_int32_t i;
   BOARD_t* pTmpBoard;
#endif /* TAPI_VERSION4 */
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK(((0>nCodType) || (IFX_TAPI_ENC_TYPE_MAX <= nCodType)),
                       nCodType, IFX_ERROR);
#ifdef TAPI_VERSION4
   /* for SVIP board resource manager is used, when XT16 is connected to SVIP
      coder channel from SVIP board will be used*/
   if (0 >= pBoard->nMaxCoderCh)
   {
      /* for all boards */
      for (i=0; i<pBoard->pCtrl->nBoardCnt; i++)
      {
         pTmpBoard = &pBoard->pCtrl->rgoBoards[i];
         /* data channels from this board will be used */
         if (0 < pBoard->nMaxCoderCh)
         {
            pBoard = pTmpBoard;
            break;
         } /* if (0 < pBoard->nMaxCoderCh) */
      } /* for all boards */
   } /* if (0 >= pBoard->nMaxCoderCh) */
#endif /* TAPI_VERSION4 */
   /* check if board has data channels */
   if (0 >= pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("%s: Err, VOIP_CodecAvailabilityCheck: "
             "board doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_ERROR;
   } /* if (0 >= pBoard->nMaxCoderCh) */
   /* asumption was made that first channel is a data channel */
   nFd = pBoard->nDevCtrl_FD;
   memset(&oCapList, 0, sizeof(IFX_TAPI_CAP_t));
   /* checking if codec available */
   oCapList.captype = IFX_TAPI_CAP_TYPE_CODEC;
   oCapList.cap = nCodType;

#ifdef TAPI_VERSION4
   nDevTmp = oCapList.dev;
#endif /* TAPI_VERSION4 */

   /* check capability */
   nRet = TD_IOCTL(nFd, IFX_TAPI_CAP_CHECK, (IFX_int32_t) &oCapList,
                      nDevTmp, nSeqConnId);
   TD_RETURN_IF_ERROR(nRet);
   /* if returned 0 then this capability is not supported */
   if (0 < nRet)
   {
      /* codec is supported */
      return IFX_SUCCESS;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
         ("%s: Err, Codec %s is not supported.\n",
          pBoard->pszBoardName, pCodecName[nCodType]));
   return IFX_ERROR;
} /* VOIP_CodecAvailabilityCheck() */

/**
   Set VOIP encoder, vocoder type, and packetization time.

   \param pCtrl   - handle to control structure

   \return IFX_SUCCESS if OK, IFX_ERROR if errors
*/
IFX_return_t VOIP_SetCodec(CTRL_STATUS_t* pCtrl)
{
   IFX_return_t ret;
   BOARD_t* pBoard;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   pBoard = TD_GET_MAIN_BOARD(pCtrl);
   /* We can use codec only for board with codec channel */
   if (0 >= pBoard->nMaxCoderCh)
   {
      /* There is no coder channel on board */
      /* Don't need codec */
      return IFX_SUCCESS;
   } /* if (0 >= pBoard->nMaxCoderCh) */

   /* Check if user set encoder */
   if (pCtrl->pProgramArg->oArgFlags.nEncTypeDef)
   {
      VOIP_SetEncoderType(pBoard, pCtrl->pProgramArg->nEnCoderType,
                          TD_CONN_ID_INIT);
   }

   if (pCtrl->pProgramArg->oArgFlags.nFrameLen)
   {
      ret = VOIP_SetFrameLen(pCtrl->pProgramArg->nPacketisationTime,
                             TD_CONN_ID_INIT);
   }
   else
   {
      ret = VOIP_SetDefaultFrameLen(TD_CONN_ID_INIT);
   }

   if (IFX_SUCCESS == ret)
   {
      if ((0 > eCurrEncType) || (IFX_TAPI_ENC_TYPE_MAX <= eCurrEncType))
      {
        TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
             ("Err, Invalid index of array 'pCodecName'.\n"));
        return IFX_ERROR;
      }
      else
      {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Using encoder type %s and packetisation time %s.\n",
             pCodecName[eCurrEncType], oFrameLen[nCurrPacketTime]));
      }
   }

   return ret;
} /* VOIP_SetCodec */

#ifdef EASY336

/**
   Get VoIP ID of other peer - can be used oly when external call is done
   between phones handled by the same instance of TAPIDEMO.

   \param pSrcConn   - pointer to source connection
   \param pCtrl      - pointer to control strucure
   \param pVoipIdNum - pointer to VoipId, output value, set when valid
                       destination connection is found
   \param nSeqConnId - Seq Conn ID

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t VOIP_SvipGetVoipIdOfConn(CONNECTION_t* pSrcConn,
                                      CTRL_STATUS_t* pCtrl,
                                      IFX_int32_t* pVoipIdNum,
                                      IFX_uint32_t nSeqConnId)
{
   PHONE_t* pDstPhone = IFX_NULL;
   IFX_boolean_t fFreeConn;
   CONNECTION_t* pDstConn = IFX_NULL;
   PHONE_t* pSrcPhone = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pSrcConn, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pVoipIdNum, IFX_ERROR);

   /* get source phone */
   pSrcPhone = ABSTRACT_GetPHONE_OfConn(pCtrl, pSrcConn);
   /* check if phone is available */
   if (IFX_NULL == pSrcPhone)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Failed to get phone for connection %d. "
             "(File: %s, line: %d)\n",
             (IFX_int32_t) pSrcConn, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get destination phone */
   pDstPhone = ABSTRACT_GetPHONE_OfCalledNum(pCtrl,
                  pSrcConn->oConnPeer.nPhoneNum, nSeqConnId);
   /* check if phone is available */
   if (IFX_NULL == pDstPhone)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Failed to get phone with number %d. (File: %s, line: %d)\n",
             pSrcConn->oConnPeer.nPhoneNum, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* get destination connection between phones */
   pDstConn = ABSTRACT_GetConnOfPhone(pDstPhone, EXTERN_VOIP_CALL,
                                      pDstPhone->nPhoneNumber,
                                      pSrcPhone->nPhoneNumber,
                                      &pSrcConn->oConnPeer.oRemote.oToAddr,
                                      &fFreeConn, pDstPhone->nSeqConnId);
   /* IFX_NULL was returned if no connection was found */
   if (IFX_NULL == pDstConn)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Failed to connection of Phone No. %d. (File: %s, line: %d)\n",
             pDstPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* must be connection in use */
   if (IFX_TRUE == fFreeConn)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Invalid connection of Phone No. %d. (File: %s, line: %d)\n",
             pDstPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set VoIP ID */
   *pVoipIdNum = pDstConn->voipID;
   return IFX_SUCCESS;
}
/**
   Configure VoIP session.

   \param nFd        - file desctriptor of device to configure
   \param pConn      - handle to connection structure
   \param pBoard     - board handle
   \param pPhone     - pointer to PHONE_t
   \param pOurCodec  - used data channel codec configuration structure
   \param nPktType   - type of packet to configure (if not set to Fax then
                       default type is used)

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t VOIP_SvipSessionStart(const IFX_int32_t nFd,
                                   CONNECTION_t* pConn,
                                   BOARD_t* pBoard, PHONE_t* pPhone,
                                   const VOIP_DATA_CH_t *pOurCodec,
                                   const IFX_TAPI_PKT_NET_TYPE_t nPktType)
{

   VOIP_DATA_CH_t *pPeerCodec = IFX_NULL;
   IFX_TAPI_PKT_NET_CFG_t PktNetCfg;
   /* used for printouts */
   IFX_uint16_t nSrcPort, nDstPort;
   IFX_int32_t nDstDev;
   IFX_uint8_t* pRemoteMac;
#ifndef CONFIG_LTQ_SVIP_NAT_PT
   SVIP_NAT_Rule_t NatRule;
   IFX_int32_t nDstIp = 0;
   struct in_addr nIpSrc, nIpDst;
#else /* CONFIG_LTQ_SVIP_NAT_PT */
   SVIP_NAT_PT_IO_Rule_t NatRule;
   SVIP_IP_ADDR_UNION_t *pDstIp = NULL;
   SVIP_IP_ADDR_TYPE_t  nDstIpType = SVIP_IPV4_ADDR_TYPE;
   char addrBuf[INET6_ADDRSTRLEN];
#endif /* CONFIG_LTQ_SVIP_NAT_PT */
   IFX_int32_t nDstVoipId = SVIP_RM_UNUSED;
   IFX_return_t ret;
   /* flag that is indicating that remote call is made
      between two phones on the same board */
   IFX_boolean_t bLocalCall = IFX_FALSE;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pOurCodec, IFX_ERROR);

   /* check call type and set other peer VoIP ID if available */
   if (LOCAL_CALL == pConn->nType)
   {
      nDstVoipId =
         pConn->oConnPeer.oLocal.pPhone->pVoipIDs[pConn->oConnPeer.oLocal.pPhone->nVoipIDs-1];
      bLocalCall = IFX_TRUE;
   } /* if (LOCAL_CALL == pConn->nType) */
   else
   {
#ifdef TD_IPV6_SUPPORT
      if (AF_INET6 == TD_SOCK_FamilyGet(&pConn->oConnPeer.oRemote.oToAddr))
      {
         if (IFX_TRUE == TD_SOCK_AddrCompare(&pConn->oConnPeer.oRemote.oToAddr,
                                 &pBoard->pCtrl->oIPv6_Ctrl.oAddrIPv6, IFX_FALSE))
         {
            /* get VoIP ID */
            if (IFX_SUCCESS == VOIP_SvipGetVoipIdOfConn(pConn, pBoard->pCtrl,
                                  &nDstVoipId, pPhone->nSeqConnId))
            {
               /* configure the same way as local call */
               bLocalCall = IFX_TRUE;
            }
         }
      }
      else
#endif /* TD_IPV6_SUPPORT */
      {
         if (IFX_TRUE == TD_SOCK_AddrCompare(&pConn->oConnPeer.oRemote.oToAddr,
                                 &pBoard->pCtrl->pProgramArg->oMy_IP_Addr, IFX_FALSE))
         {
            /* get VoIP ID */
            if (IFX_SUCCESS == VOIP_SvipGetVoipIdOfConn(pConn, pBoard->pCtrl,
                                  &nDstVoipId, pPhone->nSeqConnId))
            {
               /* configure the same way as local call */
               bLocalCall = IFX_TRUE;
            }
         }
      }
   } /* if (LOCAL_CALL == pConn->nType) */

   /* check call type */
   if (IFX_TRUE == bLocalCall)
   {
      pPeerCodec = Common_GetCodec(pBoard->pCtrl, nDstVoipId,
                                   pPhone->nSeqConnId);
      if (pPeerCodec == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Phone No %d: Failed to get codec. (File: %s, line: %d)\n",
                pConn->oConnPeer.oLocal.pPhone->nPhoneNumber,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      nSrcPort = htons(pOurCodec->nUDP_Port);
      nDstPort = htons(pPeerCodec->nUDP_Port);
      nDstDev = pPeerCodec->nDev;
      pRemoteMac = IFX_NULL;

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("%s: Setting up a session on dev %d ch %d to connect with\n"
             "%scoder on dev %d ch %d.\n",
             pBoard->pszBoardName, pOurCodec->nDev, pOurCodec->nCh,
             pBoard->pIndentBoard, pPeerCodec->nDev, pPeerCodec->nCh));
   }
   else
   {
      /* port numbers in address structure should already be in network order */
      nSrcPort = TD_SOCK_PortGet(&pConn->oUsedAddr);
      nDstPort = TD_SOCK_PortGet(&pConn->oConnPeer.oRemote.oToAddr);
      nDstDev = -1;
      pRemoteMac = pConn->oConnPeer.oRemote.MAC;

#ifndef CONFIG_LTQ_SVIP_NAT_PT
      nDstIp = TD_SOCK_AddrIPv4Get(&pConn->oConnPeer.oRemote.oToAddr);
#else /* CONFIG_LTQ_SVIP_NAT_PT */
      pDstIp = TD_SOCK_GetAddrIn(&pConn->oConnPeer.oRemote.oToAddr);
#ifdef TD_IPV6_SUPPORT
      if (AF_INET6 == TD_SOCK_FamilyGet(&pConn->oConnPeer.oRemote.oToAddr))
      {
         nDstIpType = SVIP_IPV6_ADDR_TYPE;
      }
      else
#endif  /* TD_IPV6_SUPPORT */
      {
         nDstIpType = SVIP_IPV4_ADDR_TYPE;
      }
#endif /* CONFIG_LTQ_SVIP_NAT_PT */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("%s: Setting up a session on device %d ch %d for VoIP session.\n",
             pBoard->pszBoardName,
             pOurCodec->nDev, pOurCodec->nCh));
   }
   /* set structures used for session configuration */
#ifndef CONFIG_LTQ_SVIP_NAT_PT
   if (SVIP_libStatusOk !=
       SVIP_libVoIP_CFG_Set(pOurCodec->nDev, pOurCodec->nCh,
                            nSrcPort, nDstPort,
                            nDstDev, IFX_FALSE, pRemoteMac,
                            nDstIp, &PktNetCfg, &NatRule))
#else /* CONFIG_LTQ_SVIP_NAT_PT */
   if (SVIP_libStatusOk !=
       SVIP_libVoIP_CFG_Set(pOurCodec->nDev, pOurCodec->nCh,
                            nSrcPort, nDstPort,
                            nDstDev, IFX_FALSE, pRemoteMac,
                            nDstIpType, pDstIp, &PktNetCfg,
                            &NatRule))
#endif /* CONFIG_LTQ_SVIP_NAT_PT */
   {
      return IFX_ERROR;
   }

#ifndef CONFIG_LTQ_SVIP_NAT_PT
   memset(&nIpDst, 0, sizeof(nIpDst));
   memset(&nIpSrc, 0, sizeof(nIpSrc));
#endif /* CONFIG_LTQ_SVIP_NAT_PT */

   /* check transmission type - can be set to fax */
   if (Fax == nPktType)
   {
      PktNetCfg.nPktType = nPktType;
   }
#ifndef CONFIG_LTQ_SVIP_NAT_PT
   /* copy data for easier printout */
   nIpSrc.s_addr = PktNetCfg.nIp.v4.IpSrc;
   nIpDst.s_addr = PktNetCfg.nIp.v4.IpDst;
#endif /* CONFIG_LTQ_SVIP_NAT_PT */
   /* printout session parameters */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("####### Configure Ethernet session ########\n"
          "\tDestination MAC %02X:%02X:%02X:%02X:%02X:%02X\n"
          "\tSource      MAC %02X:%02X:%02X:%02X:%02X:%02X\n"
          "\tPorts: Rec-%d, Des-%d, Sou-%d\n"
          "\tchan %d, dev %d\n"
          "\tType: %s, Using: %s\n",
          PktNetCfg.MacDst[0],
          PktNetCfg.MacDst[1],
          PktNetCfg.MacDst[2],
          PktNetCfg.MacDst[3],
          PktNetCfg.MacDst[4],
          PktNetCfg.MacDst[5],
          PktNetCfg.MacSrc[0],
          PktNetCfg.MacSrc[1],
          PktNetCfg.MacSrc[2],
          PktNetCfg.MacSrc[3],
          PktNetCfg.MacSrc[4],
          PktNetCfg.MacSrc[5],
          ntohs(PktNetCfg.nUdpRcv),
          ntohs(PktNetCfg.nUdpDst),
          ntohs(PktNetCfg.nUdpSrc),
          PktNetCfg.ch, PktNetCfg.dev,
          PktNetCfg.nPktType == Voice ? "Voice" :
          PktNetCfg.nPktType == Fax ? "Fax" :
          PktNetCfg.nPktType == FaxTrace ? "FaxTrace" : "unknown",
          PktNetCfg.nIpMode == IFX_TAPI_PKT_NET_IPV4 ? "IPV4" : "IPV6"));
#ifndef CONFIG_LTQ_SVIP_NAT_PT
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tIP Dst %s,\n",
          inet_ntoa(nIpDst)));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tIP Src %s\n",
          inet_ntoa(nIpSrc)));
#else /* CONFIG_LTQ_SVIP_NAT_PT */
   inet_ntop((PktNetCfg.nIpMode == IFX_TAPI_PKT_NET_IPV6) ? AF_INET6 : AF_INET,
             (PktNetCfg.nIpMode == IFX_TAPI_PKT_NET_IPV6) ?
             (IFX_void_t*) PktNetCfg.nIp.v6.IpDst : (IFX_void_t*)&PktNetCfg.nIp.v4.IpDst,
             addrBuf, sizeof(addrBuf));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tIP Dst %s,\n",
          addrBuf));
   inet_ntop((PktNetCfg.nIpMode == IFX_TAPI_PKT_NET_IPV6) ? AF_INET6 : AF_INET,
             (PktNetCfg.nIpMode == IFX_TAPI_PKT_NET_IPV6) ?
             (IFX_void_t*)PktNetCfg.nIp.v6.IpSrc : (IFX_void_t*)&PktNetCfg.nIp.v4.IpSrc,
             addrBuf, sizeof(addrBuf));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tIP Src %s,\n",
          addrBuf));
#endif /* CONFIG_LTQ_SVIP_NAT_PT */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("\tnVLAN_Tagging %d\n"
          "###############\n",
          PktNetCfg.nVLAN_Tagging));

   ret = TD_IOCTL (nFd, IFX_TAPI_PKT_NET_CFG_SET, (IFX_int32_t)&PktNetCfg,
            PktNetCfg.dev, pPhone->nSeqConnId);
   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Failed to configure packet header information for "
             "dev %d, ch %d. (File: %s, line: %d)\n",
             PktNetCfg.dev, PktNetCfg.ch, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* SVIP library is signalling that no NAT rule should be added by setting
    * port value to 0 */
   if (NatRule.locUDP != 0)
   {
#ifndef CONFIG_LTQ_SVIP_NAT_PT
      memset(&nIpDst, 0, sizeof(nIpDst));
      memset(&nIpSrc, 0, sizeof(nIpSrc));

      nIpDst.s_addr = NatRule.remIP;
      nIpSrc.s_addr = NatRule.locIP;
#endif  /* CONFIG_LTQ_SVIP_NAT_PT */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("####### Adding NAT rule ########\n"
             "\tDestination MAC %02X:%02X:%02X:%02X:%02X:%02X\n"
             "\tSource      MAC %02X:%02X:%02X:%02X:%02X:%02X\n"
             "\tUDP port: %d\n",
             NatRule.remMAC[0],
             NatRule.remMAC[1],
             NatRule.remMAC[2],
             NatRule.remMAC[3],
             NatRule.remMAC[4],
             NatRule.remMAC[5],
             NatRule.locMAC[0],
             NatRule.locMAC[1],
             NatRule.locMAC[2],
             NatRule.locMAC[3],
             NatRule.locMAC[4],
             NatRule.locMAC[5],
             ntohs(NatRule.locUDP)));

#ifndef CONFIG_LTQ_SVIP_NAT_PT
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("\tIP Dst %s,\n",
             inet_ntoa(nIpDst)));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("\tIP Src %s\n",
             inet_ntoa(nIpSrc)));
#else  /* CONFIG_LTQ_SVIP_NAT_PT */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("\tIP Dst %s,\n",
             inet_ntop(nDstIpType, (void *)&NatRule.remIP,
                       addrBuf, sizeof(addrBuf))));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("\tIP type Dst %d Src %d\n",
             NatRule.remTypeIP, NatRule.locTypeIP));

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("\tIP Src %s\n",
             inet_ntop(AF_INET, (void *)&NatRule.locIP,
                       addrBuf, sizeof(addrBuf))));
#endif  /* CONFIG_LTQ_SVIP_NAT_PT */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("################################\n"));

#ifndef CONFIG_LTQ_SVIP_NAT_PT
      ret = SVIP_NAT_RuleAdd (&NatRule);
#else  /* CONFIG_LTQ_SVIP_NAT_PT */
      ret = SVIP_NAT_PT_RuleAdd (&NatRule);
#endif  /* CONFIG_LTQ_SVIP_NAT_PT */
      if (ret != SVIP_libStatusOk)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Adding SVIP NAT rule for device %d, channel %d failed. "
               "(File: %s, line: %d)\n",
               pOurCodec->nDev,
               pOurCodec->nCh,
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* if (NatRule.locUDP != 0) */
   return IFX_SUCCESS;
}
#endif /* EASY336 */

/**
   Starts the encoder/decoder.

   \param nDataCh  - data channel
   \param pConn - handle to connection structure
   \param pBoard - board handle
   \param pPhone - pointer to PHONE_t

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
   \remarks for TAPI3 pConn is not used
            for TAPI4 nDataCh is not used
*/
IFX_return_t VOIP_StartCodec(IFX_int32_t nDataCh, CONNECTION_t* pConn,
                             BOARD_t* pBoard, PHONE_t* pPhone)
{
   IFX_int32_t data_fd = -1;
   IFX_TAPI_ENC_CFG_t oEncoderCfg;
   IFX_TAPI_DEC_CFG_t oDecoderCfg;
   IFX_return_t ret;
   VOIP_DATA_CH_t *pOurCodec = IFX_NULL;
#ifdef TAPI_VERSION4
   IFX_TAPI_ENC_MODE_t encMode;
   IFX_TAPI_DEC_t dec;
#endif /* TAPI_VERSION4 */
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_uint32_t nIoctlArg = TD_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
#ifdef EASY3111
   /* check board type */
   if (TYPE_DUSLIC_XT == pBoard->nType)
   {
      /* for board combination with extension with DuSLIC-xT chip and main board
         with chip other than DuSLIC-xT e.g. AR9, VR9.. data channels from main
         board are used */
      pBoard = TD_GET_MAIN_BOARD(pBoard->pCtrl);
   }
#endif /* EASY3111 */
   /* check if board has coders */
   if (0 >= pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("VOIP_StartCodec: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_ERROR;
   }
   /* get data channel codec structure */
#ifdef EASY336
   pOurCodec = Common_GetCodec(pBoard->pCtrl, pConn->voipID, pPhone->nSeqConnId);
   if (pOurCodec == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Failed to get codec. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
#else
   pOurCodec = &pBoard->pDataChStat[nDataCh];
#endif
   /* get data channel FD */
#ifdef TAPI_VERSION4
   if (pBoard->fSingleFD)
   {
      data_fd = pBoard->nDevCtrl_FD;
   }
   else
#endif /* TAPI_VERSION4 */
   {
      data_fd = VOIP_GetFD_OfCh(nDataCh, pBoard);
   }

   /* check if codec is already started and if it can be started */
   if (pOurCodec->fStartCodec && !pOurCodec->fCodecStarted)
   {
#ifndef TAPI_VERSION4
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
           ("DataCh No %d: Start encoder/decoder: %s,\n"
            "             packet length %s on fd %d\n",
            (int) nDataCh,
            pCodecName[(int)pOurCodec->nCodecType],
            oFrameLen[(int)pOurCodec->nFrameLen],
            (int) data_fd));
#else /* TAPI_VERSION4 */
#ifdef EASY336
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
           ("Start encoder/decoder on dev %d, data ch %d\n",
            (int) pOurCodec->nDev, (int) pOurCodec->nCh));
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

#ifndef STREAM_1_1
/* start             Configure encoder                                  start */
      /* From the version 1.2.7.0 of TAPI drivers both codec and frame
        length should be changed by the IFX_TAPI_ENC_CFG_SET function */
      oEncoderCfg.nEncType = pOurCodec->nCodecType;
      oEncoderCfg.nFrameLen = pOurCodec->nFrameLen;
      oEncoderCfg.AAL2BitPack = pOurCodec->nBitPack;

      /* print bitpack setting if nonstandard */
      if (oEncoderCfg.AAL2BitPack == IFX_TAPI_COD_AAL2_BITPACK)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Using ITU-T I366.2 Bit Alignment for G.726 codecs\n"));
      }

      /* Set coder type and packetisation time */
#ifdef TAPI_VERSION4
      if (pBoard->fSingleFD)
      {
         oEncoderCfg.dev = pOurCodec->nDev;
         oEncoderCfg.ch = pOurCodec->nCh;
         nDevTmp = oEncoderCfg.dev;
      } /* if (pBoard->fSingleFD) */
#endif /*TAPI_VERSION4*/
      ret = TD_IOCTL(data_fd, IFX_TAPI_ENC_CFG_SET, (int) &oEncoderCfg,
                     nDevTmp, pPhone->nSeqConnId);
      /* check if configuration was successfull */
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Failed to set encoder %s, frame length %s.\n"
                "(File: %s, line: %d)\n",
                pCodecName[oEncoderCfg.nEncType],
                oFrameLen[oEncoderCfg.nFrameLen],
                __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* send iformation about used codec (decoder) */
      COM_MOD_SEND_INFO_USED_CODEC(
         ABSTRACT_GetPHONE_OfConn(pBoard->pCtrl, pConn),
         nDataCh, oEncoderCfg.nEncType, oEncoderCfg.nFrameLen, IFX_FALSE);
#endif /* STREAM_1_1 */
/*   end             Configure encoder                                  end   */
/* start             Configure decoder                                  start */
      /* set decoder configuration, basicaly needed only by
         ITU-T I366.2 Bit Alignment for G.726 */
      oDecoderCfg.AAL2BitPack = oEncoderCfg.AAL2BitPack;
      /* packet loss handled according to codec type */
      oDecoderCfg.Plc = IFX_TAPI_DEC_PLC_CODEC;
      /* Set decoder type */
#ifdef TAPI_VERSION4
      if (pBoard->fSingleFD)
      {
         oDecoderCfg.ch = oEncoderCfg.ch;
         oDecoderCfg.dev = oEncoderCfg.dev;
         nDevTmp = oDecoderCfg.dev;
      } /* if (pBoard->fSingleFD) */
#endif /*TAPI_VERSION4*/

      ret = TD_IOCTL(data_fd, IFX_TAPI_DEC_CFG_SET, (int) &oDecoderCfg,
                        nDevTmp, pPhone->nSeqConnId);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, Failed to configure decoder. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
/*   end             Configure decoder                                  end   */
#ifdef EASY336
      /* Configure session */
      if (IFX_SUCCESS != VOIP_SvipSessionStart(data_fd, pConn, pBoard, pPhone,
                                               pOurCodec, Voice))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Failed to configure VoIP session. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
#endif /* EASY336 */
/* start             Start encoder                                      start */
      /* Start encoding */
#ifdef TAPI_VERSION4
      if (pBoard->fSingleFD)
      {
         encMode.dev = pOurCodec->nDev;
         encMode.ch = pOurCodec->nCh;
         nIoctlArg = (IFX_uint32_t) &encMode;
         nDevTmp = encMode.dev;
      } /* if (pBoard->fSingleFD) */
      else
#endif /* TAPI_VERSION4 */
      {
         nIoctlArg = 0;
      } /* if (pBoard->fSingleFD) */

      ret = TD_IOCTL(data_fd, IFX_TAPI_ENC_START, nIoctlArg,
                        nDevTmp, pPhone->nSeqConnId);
      if ( IFX_ERROR == ret )
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Start encoder failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
/*   end             Start encoder                                      end   */
/* start             Start decoder                                      start */
      /* Start decoding */
#ifdef TAPI_VERSION4
      if (pBoard->fSingleFD)
      {
         dec.dev = pOurCodec->nDev;
         dec.ch = pOurCodec->nCh;
         nIoctlArg = (IFX_uint32_t) &dec;
         nDevTmp = dec.dev;
      } /* if (pBoard->fSingleFD) */
      else
#endif /* TAPI_VERSION4 */
      {
         nIoctlArg = 0;
      } /* if (pBoard->fSingleFD) */

      ret = TD_IOCTL(data_fd, IFX_TAPI_DEC_START, nIoctlArg,
                        nDevTmp, pPhone->nSeqConnId);

      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Start decoder failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
/*   end             Start decoder                                      end   */
      /* set codec started flag */
      pOurCodec->fCodecStarted = IFX_TRUE;

   } /* if */

   return IFX_SUCCESS;
} /* VOIP_StartCodec() */

/**
   Stops the encoder/decoder.

   \param nDataCh  - data channel
   \param pConn - handle to connection structure
   \param pBoard - board handle
   \param pPhone - pointer to PHONE_t

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
   \remarks for TAPI3 pConn is not used
            for TAPI4 nDataCh is not used
*/
IFX_return_t VOIP_StopCodec(IFX_int32_t nDataCh, CONNECTION_t* pConn,
                            BOARD_t* pBoard, PHONE_t* pPhone)
{
   IFX_int32_t data_fd = -1;
   IFX_return_t ret;
#ifdef TAPI_VERSION4
   VOIP_DATA_CH_t *pCodec = IFX_NULL;
   IFX_TAPI_ENC_MODE_t encMode;
   IFX_TAPI_DEC_t dec;
#endif /* TAPI_VERSION4 */
#ifdef EASY336
#ifndef CONFIG_LTQ_SVIP_NAT_PT
   SVIP_NAT_Rule_t NatRule;
#else  /* CONFIG_LTQ_SVIP_NAT_PT */
   SVIP_NAT_PT_IO_Rule_t NatRule;
#endif  /* CONFIG_LTQ_SVIP_NAT_PT */
#endif /* EASY336 */
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_uint32_t nIoctlArg = TD_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
#ifdef EASY3111
   /* check board type */
   if (TYPE_DUSLIC_XT == pBoard->nType)
   {
      /* for board combination with extension with DuSLIC-xT chip and main board
         with chip other than DuSLIC-xT e.g. AR9, VR9.. data channels from main
         board are used */
      pBoard = TD_GET_MAIN_BOARD(pBoard->pCtrl);
   }
#endif /* EASY3111 */
   if (0 >= pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("VOIP_StopCodec: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_ERROR;
   }

#ifdef EASY336
   memset(&NatRule, 0, sizeof(NatRule));
#endif /* EASY336 */

#ifndef TAPI_VERSION4
   data_fd = VOIP_GetFD_OfCh(nDataCh, pBoard);
#else/* TAPI_VERSION4 */
   if (pBoard->fSingleFD)
      data_fd = pBoard->nDevCtrl_FD;
#endif/* TAPI_VERSION4 */

#ifdef EASY336
   pCodec = Common_GetCodec(pBoard->pCtrl, pConn->voipID, pPhone->nSeqConnId);
#endif /* EASY336 */

#ifdef TAPI_VERSION4
   if (pCodec == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Failed to get codec. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* TAPI_VERSION4 */

#ifndef TAPI_VERSION4
   if (IFX_TRUE == pBoard->pDataChStat[nDataCh].fCodecStarted)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
           ("DataCh No %d: Stop encoder/decoder on fd %d\n",
           (int) nDataCh, (int) data_fd));
   }
#else /* TAPI_VERSION4 */
   if (IFX_TRUE == pCodec->fCodecStarted)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
           ("DataCh No %d: Stop encoder/decoder on dev %d\n",
           (int) pCodec->nCh, (int) pCodec->nDev));
   }
#endif /* TAPI_VERSION4 */

      /* Stop encoding */
#ifdef TAPI_VERSION4
   if (pBoard->fSingleFD)
   {
      encMode.dev = pCodec->nDev;
      encMode.ch = pCodec->nCh;
      nIoctlArg = (IFX_uint32_t) &encMode;
      nDevTmp = encMode.dev;
   } /* if (pBoard->fSingleFD) */
   else
#endif /* TAPI_VERSION4 */
   {
      nIoctlArg = 0;
   } /* if (pBoard->fSingleFD) */

   ret = TD_IOCTL(data_fd, IFX_TAPI_ENC_STOP, nIoctlArg,
                     nDevTmp, pPhone->nSeqConnId);

   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Stop encoder failed. (File: %s, line: %d)\n",
           __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Stop decoding */
#ifdef TAPI_VERSION4
   if (pBoard->fSingleFD)
   {
      dec.dev = pCodec->nDev;
      dec.ch = pCodec->nCh;
      nIoctlArg = (IFX_uint32_t) &dec;
      nDevTmp = dec.dev;
   } /* if (pBoard->fSingleFD) */
   else
#endif/* TAPI_VERSION4 */
   {
      nIoctlArg = 0;
   } /* if (pBoard->fSingleFD) */

   ret = TD_IOCTL(data_fd, IFX_TAPI_DEC_STOP, nIoctlArg,
            nDevTmp, pPhone->nSeqConnId);
   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Stop decoder failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

#ifndef TAPI_VERSION4
   pBoard->pDataChStat[nDataCh].fCodecStarted = IFX_FALSE;
#else/* TAPI_VERSION4 */
   pCodec->fCodecStarted = IFX_FALSE;
#endif/* TAPI_VERSION4 */

#ifdef EASY336

   if (pConn->nType == LOCAL_CALL)
      NatRule.locUDP = htons(pCodec->nUDP_Port);
   else
      NatRule.locUDP = htons(TD_SOCK_PortGet(&pConn->oUsedAddr));

#ifndef CONFIG_LTQ_SVIP_NAT_PT
   if (SVIP_NAT_RuleRemove (&NatRule) != SVIP_libStatusOk)
#else  /* CONFIG_LTQ_SVIP_NAT_PT */
   if (SVIP_NAT_PT_RuleRemove (&NatRule) != SVIP_libStatusOk)
#endif  /* CONFIG_LTQ_SVIP_NAT_PT */
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Removing SVIP NAT rule for device %d, channel %d failed. "
            "(File: %s, line: %d)\n",
            pCodec->nDev,
            pCodec->nCh,
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* EASY336 */

   return IFX_SUCCESS;
} /* VOIP_StopCodec() */

#ifdef TAPI_VERSION4
/**
   Starts the fax channel.

   \param pCtrl - handle to control structure
   \param pConn  - pointer to phone connection
   \param pBoard - board handle
   \param pPhone - pointer to PHONE_t

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t VOIP_FaxStart(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn,
                           BOARD_t* pBoard, PHONE_t* pPhone)
{
#ifdef EASY336
   IFX_return_t ret = IFX_ERROR;
   VOIP_DATA_CH_t *pOurCodec = IFX_NULL;
   /** \todo: add this strucutre to TAPI3 */
   IFX_TAPI_T38_SESS_CFG_t t38SessCfg;

   memset (&t38SessCfg, 0, sizeof (IFX_TAPI_T38_SESS_CFG_t));
#endif
   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   if (pBoard->nMaxCoderCh == 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("VOIP_FaxStart: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_SUCCESS;
   }

#ifdef EASY336
   /* get data channel codec structure */
   pOurCodec = Common_GetCodec(pCtrl, pConn->voipID, pPhone->nSeqConnId);
   if (pOurCodec == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Failed to get codec. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (pOurCodec->fStartCodec && !pOurCodec->fCodecStarted)
   {
      if (pConn->nType == LOCAL_CALL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("############################################\n"));
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId, (
               "Local FAX connection\n"));
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("############################################\n"));
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("############################################\n"));
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId, (
                     "VoIP FAX connection\n"));
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("############################################\n"));
      }
      /* Configure session */
      if (IFX_SUCCESS != VOIP_SvipSessionStart(pBoard->nDevCtrl_FD, pConn,
                                               pBoard, pPhone, pOurCodec, Fax))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Failed to configure Fax session. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
      memcpy(&t38SessCfg, &pBoard->oT38_Config.oT38SessCfg,
             sizeof(IFX_TAPI_T38_SESS_CFG_t));
#else /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
      t38SessCfg.nRateManagement = IFX_TAPI_T38_TRANS_TCF;
      t38SessCfg.nProtocol = IFX_TAPI_T38_UDP;
      t38SessCfg.nBitRateMax = 14400;
      t38SessCfg.nUDPErrCorr = IFX_TAPI_T38_RED;
      t38SessCfg.nUDPBuffSizeMax = 1000;
      t38SessCfg.nUDPDatagramSizeMax = 1000;
      t38SessCfg.nT38Ver = 0;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
      t38SessCfg.dev = pOurCodec->nDev;
      t38SessCfg.ch = pOurCodec->nCh;
      /* Start session with predefined T.38 capabilities t38SessCfg */
      if (pBoard->fSingleFD)
      {
         ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_T38_SESS_START,
                  (IFX_int32_t) &t38SessCfg, t38SessCfg.dev, pPhone->nSeqConnId);
      } /* if (pBoard->fSingleFD) */
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("FAX Start failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
         return IFX_ERROR;
      }
      pOurCodec->fCodecStarted = IFX_TRUE;
   } /* if */
#endif /* EASY336 */
   return IFX_SUCCESS;
} /* VOIP_FaxStart() */

/**
   Stopss the fax channel.

   \param pCtrl - handle to control structure
   \param pConn  - pointer to phone connection
   \param pBoard - board handle
   \param pPhone - pointer to PHONE_t

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t VOIP_FaxStop(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn,
                          BOARD_t* pBoard, PHONE_t* pPhone)
{
#ifdef EASY336
#ifndef CONFIG_LTQ_SVIP_NAT_PT
   SVIP_NAT_Rule_t NatRule;
#else  /* CONFIG_LTQ_SVIP_NAT_PT */
   SVIP_NAT_PT_IO_Rule_t NatRule;
#endif  /* CONFIG_LTQ_SVIP_NAT_PT */
   VOIP_DATA_CH_t *pCodec = IFX_NULL;
   /** \todo: add these strucutre to TAPI3 */
   IFX_TAPI_T38_SESS_STOP_t t38SessStop;
   IFX_TAPI_T38_SESS_STATISTICS_t t38SessStat;
   IFX_return_t ret = IFX_ERROR;

   memset (&t38SessStat, 0, sizeof (IFX_TAPI_T38_SESS_STATISTICS_t));
   memset (&t38SessStop, 0, sizeof (IFX_TAPI_T38_SESS_STOP_t));
#endif /* EASY336 */

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   if (pBoard->nMaxCoderCh == 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("VOIP_FaxStop: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_SUCCESS;
   }

#ifdef EASY336
   memset(&NatRule, 0, sizeof(NatRule));
   pCodec = Common_GetCodec(pCtrl, pConn->voipID, pPhone->nSeqConnId);
   if (pCodec == IFX_NULL)
      return IFX_ERROR;

   /* Stop fax */
   t38SessStat.dev = pCodec->nDev;
   t38SessStat.ch = pCodec->nCh;
   if (pBoard->fSingleFD)
   {
      ret = TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_T38_SESS_STATISTICS_GET,
               (IFX_int32_t) &t38SessStat, t38SessStat.dev, pPhone->nSeqConnId);
   } /* if (pBoard->fSingleFD) */
   if (ret != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("T38 get stat failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("############################################\n"));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId, (
         "Fax session channel %d statistics:\n"
         "nFaxSessState %d\n"
         "nT38VerMajor %d\n"
         "nT38VerMin %d\n"
         "SessInfo %d\n"
         "nFdpStand %d\n"
         "nPktLost %d\n"
         "nPktRecov %d\n"
         "nPktGroupLost %d\n"
         "nFttRsp %d\n"
         "nPagesTx %d\n"
         "nLineBreak %d\n"
         "nV21FrmBreak %d\n"
         "nEcmFrmBreak %d\n",
         pCodec->nCh,
         t38SessStat.nFaxSessState,
         t38SessStat.nT38VerMajor,
         t38SessStat.nT38VerMin,
         t38SessStat.SessInfo,
         t38SessStat.nFdpStand,
         t38SessStat.nPktLost,
         t38SessStat.nPktRecov,
         t38SessStat.nPktGroupLost,
         t38SessStat.nFttRsp,
         t38SessStat.nPagesTx,
         t38SessStat.nLineBreak,
         t38SessStat.nV21FrmBreak,
         t38SessStat.nEcmFrmBreak
         ));
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
      ("############################################\n"));

   t38SessStop.dev = pCodec->nDev;
   t38SessStop.ch = pCodec->nCh;
   if (IFX_ERROR == TD_IOCTL(pBoard->nDevCtrl_FD, IFX_TAPI_T38_SESS_STOP,
                       (IFX_int32_t)&t38SessStop,
                       t38SessStop.dev, pPhone->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("T38 Stop failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pCodec->fCodecStarted = IFX_FALSE;

   if (pConn->nType == LOCAL_CALL)
      NatRule.locUDP = htons(pCodec->nUDP_Port);
   else
      NatRule.locUDP = htons(TD_SOCK_PortGet(&pConn->oUsedAddr));

#ifndef CONFIG_LTQ_SVIP_NAT_PT
   if (SVIP_NAT_RuleRemove (&NatRule) != SVIP_libStatusOk)
#else /* CONFIG_LTQ_SVIP_NAT_PT */
   if (SVIP_NAT_PT_RuleRemove (&NatRule) != SVIP_libStatusOk)
#endif /* CONFIG_LTQ_SVIP_NAT_PT */
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Removing SVIP NAT rule for device %d, channel %d failed. "
            "(File: %s, line: %d)\n",
            pCodec->nDev, pCodec->nCh,
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* EASY336 */
   return IFX_SUCCESS;
} /* VOIP_FaxStop() */
#endif /* TAPI_VERSION4 */

/**
   Set codec flag if coder can be started or not.

   \param nDataCh - data channel
   \param fStartCodec - IFX_TRUE set flag to IFX_TRUE,
                        or IFX_FALSE is IFX_FALSE
   \param pBoard - board handle

   \return Flag is set to proper value
*/
IFX_void_t VOIP_SetCodecFlag(IFX_int32_t nDataCh, IFX_boolean_t fStartCodec,
                             BOARD_t* pBoard)
{



   TD_PTR_CHECK(pBoard,);
   if (0 == pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ERR,
            ("VOIP_SetCodecFlag: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return;
   }
   TD_PARAMETER_CHECK((0 > nDataCh), nDataCh,);
   TD_PARAMETER_CHECK((pBoard->nMaxCoderCh <= nDataCh), nDataCh,);
   TD_PTR_CHECK(pBoard->pDataChStat,);

   pBoard->pDataChStat[nDataCh].fStartCodec = fStartCodec;
} /* VOIP_SetCodecFlag() */


/**
   Return free data channel.

   \param pBoard - pointer to board
   \param nSeqConnId  - Seq Conn ID

   \return free data channel or NO_FREE_DATA_CH
*/
IFX_int32_t VOIP_GetFreeDataCh(BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   IFX_uint32_t i = 0;
   IFX_int32_t free_ch = NO_FREE_DATA_CH;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   if (0 == pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("VOIP_GetFreeDataCh: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_SUCCESS;
   }

   for (i = 0; i < pBoard->nMaxCoderCh; i++)
   {
      if (IFX_FALSE == pBoard->pDataChStat[i].fMapped)
      {
         free_ch = i;
         break;
      }
   }

   return free_ch;
} /* VOIP_GetFreeDataCh() */


/**
   Free mapped data channel.

   \param nDataCh - data channel number
   \param pBoard - board handle
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS - data channel is freed, otherwise IFX_ERROR
*/
IFX_return_t VOIP_FreeDataCh(IFX_int32_t nDataCh, BOARD_t* pBoard,
                             IFX_uint32_t nSeqConnId)
{
   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   if (0 == pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("VOIP_VOIP_FreeDataCh: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_SUCCESS;
   }
   TD_PARAMETER_CHECK((0 > nDataCh), nDataCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxCoderCh <= nDataCh), nDataCh, IFX_ERROR);

   pBoard->pDataChStat[nDataCh].fMapped = IFX_FALSE;

   return IFX_SUCCESS;
} /* VOIP_FreeDataCh() */


/**
   Reserve data channel as mapped.

   \param nDataCh       - data channel number
   \param pBoard        - pointer to board
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS - data channel is reserved, otherwise IFX_ERROR
*/
IFX_return_t VOIP_ReserveDataCh(IFX_int32_t nDataCh, BOARD_t* pBoard,
                                IFX_uint32_t nSeqConnId)
{


   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   if (0 == pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("VOIP_ReserveDataCh: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_SUCCESS;
   }
   TD_PARAMETER_CHECK((0 > nDataCh), nDataCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxCoderCh <= nDataCh), nDataCh, IFX_ERROR);

   if (IFX_TRUE == pBoard->pDataChStat[nDataCh].fMapped)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("Data ch %d, already reserved\n", (int) nDataCh));
      return IFX_ERROR;
   }

   pBoard->pDataChStat[nDataCh].fMapped = IFX_TRUE;

   return IFX_SUCCESS;
} /* VOIP_GetFreeDataCh() */


/**
   Release VOIP socket.

   \param pCtrl - handle to control status structure

  \return socket or NO_SOCKET on error
*/
IFX_void_t VOIP_ReleaseSockets(CTRL_STATUS_t* pCtrl)
{
   IFX_uint32_t i = 0;
#ifdef TAPI_VERSION4
   IFX_uint32_t j = 0;
#endif

   /* check input parameters */
   TD_PTR_CHECK(pCtrl,);
   TD_PTR_CHECK(pCtrl->rgnSockets,);
   TD_PTR_CHECK(pCtrl->rgoBoards,);

#ifdef TAPI_VERSION4
   /* for all boards */
   for (j=0; j<pCtrl->nBoardCnt; j++)
   {
      /* check if sockets are used */
      if (IFX_FALSE != pCtrl->rgoBoards[j].fUseSockets)
      {
         /* leave the loop */
         break;
      }
   }
   /* j will be equal to board count if non of the boards uses sockets */
   if (j >= pCtrl->nBoardCnt)
   {
      return;
   }
#endif
   for (i = 0; i < pCtrl->nSumCoderCh; i++)
   {
      if (pCtrl->rgnSockets[i].nSocket < 0)
      {
         /* This one is not open, skip it. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Socket not open, skip it.\n"));
         continue;
      }

      if (IFX_SUCCESS != TD_OS_SocketClose(pCtrl->rgnSockets[i].nSocket))
      {
         /* Err closing socket */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, closing socket with fd %d. (File: %s, line: %d)\n",
               (int) pCtrl->rgnSockets[i].nSocket, __FILE__, __LINE__));
         pCtrl->rgnSockets[i].nSocket = TD_NOT_SET;
      }
   }
#ifdef TD_IPV6_SUPPORT
   if (pCtrl->pProgramArg->oArgFlags.nUseIPv6)
   {
      for (i = 0; i < pCtrl->nSumCoderCh; i++)
      {
         if (pCtrl->rgnSocketsIPv6[i].nSocket < 0)
         {
            /* This one is not open, skip it. */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW,  TD_CONN_ID_INIT,
               ("Socket not open, skip it.\n"));
            continue;
         }
         if (IFX_SUCCESS != TD_OS_SocketClose(pCtrl->rgnSocketsIPv6[i].nSocket))
         {
            /* Err closing socket */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, closing socket with fd %d. (File: %s, line: %d)\n",
                (int) pCtrl->rgnSocketsIPv6[i].nSocket, __FILE__, __LINE__));
            pCtrl->rgnSocketsIPv6[i].nSocket = TD_NOT_SET;
         }
      }
   }
#endif /* TD_IPV6_SUPPORT */
} /* VOIP_ReleaseSockets() */


/**
   Return VOIP socket according to channel idx.

   \param nChIdx - channel index
   \param pBoard - board handle

   \return socket or NO_SOCKET on error
*/
IFX_int32_t VOIP_GetSocket(IFX_int32_t nChIdx, BOARD_t* pBoard,
                           IFX_boolean_t bIPv6Call)
{
   /* check input parameters */
   TD_PTR_CHECK(pBoard, NO_SOCKET);
   if (0 == pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ERR,
            ("VOIP_GetSocket: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_SUCCESS;
   }

#ifdef TD_IPV6_SUPPORT
   if (pBoard->pCtrl->pProgramArg->oArgFlags.nUseIPv6)
   {
      TD_PTR_CHECK(pBoard->rgnSocketsIPv6, NO_SOCKET);
   }
   else
#endif /* TD_IPV6_SUPPORT */
   {
      TD_PARAMETER_CHECK((IFX_TRUE == bIPv6Call), bIPv6Call, NO_SOCKET);
   }
   TD_PTR_CHECK(pBoard->rgnSockets, NO_SOCKET);
   TD_PARAMETER_CHECK((0 > nChIdx), nChIdx, NO_SOCKET);
   TD_PARAMETER_CHECK((pBoard->nMaxCoderCh <= nChIdx), nChIdx, NO_SOCKET);

#ifdef TD_IPV6_SUPPORT
   if (pBoard->pCtrl->pProgramArg->oArgFlags.nUseIPv6 &&
       IFX_TRUE == bIPv6Call)
   {
      return pBoard->rgnSocketsIPv6[nChIdx];
   }
#endif /* TD_IPV6_SUPPORT */
   return pBoard->rgnSockets[nChIdx];
} /* VOIP_GetSocket() */


/**
   Return VOIP port which is used by this socket.

   \param nSocket - socket number
   \param pCtrl - pointer to control structure

   \return socket or NO_SOCKET on error
*/
IFX_int32_t VOIP_GetSocketPort(IFX_int32_t nSocket, CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t i = 0;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->rgnSockets, IFX_ERROR);
#ifdef TD_IPV6_SUPPORT
   if (pCtrl->pProgramArg->oArgFlags.nUseIPv6)
   {
      TD_PTR_CHECK(pCtrl->rgnSocketsIPv6, IFX_ERROR);
   }
#endif /* TD_IPV6_SUPPORT */
   TD_PARAMETER_CHECK((0 > nSocket), nSocket, IFX_ERROR);

   for (i = 0; i < pCtrl->nSumCoderCh; i++)
   {
#ifdef TD_IPV6_SUPPORT
      if (pCtrl->pProgramArg->oArgFlags.nUseIPv6)
      {
         if (pCtrl->rgnSocketsIPv6[i].nSocket == nSocket)
         {
            return pCtrl->rgnSocketsIPv6[i].nPort;
         }
      }
#endif /* TD_IPV6_SUPPORT */
      if (pCtrl->rgnSockets[i].nSocket == nSocket)
      {
         return pCtrl->rgnSockets[i].nPort;
      }
   }

   return NO_SOCKET;
} /* VOIP_GetSocket() */


/**
   Map phone channel to data channel.

   \param nDataCh       - target data channel
   \param nDev          - device number
   \param nAddPhoneCh   - which phone channel to add
   \param fDoMapping    - IFX_TRUE do mapping, IFX_FALSE do unmapping
   \param pBoard        - board handle
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
   \remarks nDev is used only for TAPI_VERSION4
*/
IFX_return_t VOIP_MapPhoneToData(IFX_int32_t nDataCh, IFX_uint16_t nDev,
                                 IFX_int32_t nAddPhoneCh,
                                 IFX_boolean_t fDoMapping,
                                 BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_int32_t fd_ch = -1;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   if (0 == pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("VOIP_MapPhoneToData: board %s doesn't have coder channels.\n",
             pBoard->pszBoardName));
      return IFX_SUCCESS;
   }
   TD_PARAMETER_CHECK((0 > nDataCh), nDataCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxCoderCh <= nDataCh), nDataCh, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nAddPhoneCh), nAddPhoneCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxAnalogCh < nAddPhoneCh), nAddPhoneCh,
      IFX_ERROR);

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

#ifdef TAPI_VERSION4
   if (!pBoard->fSingleFD)
#endif /* TAPI_VERSION4 */
   {
      /* Get file descriptor for this channel */
      fd_ch = VOIP_GetFD_OfCh(nDataCh, pBoard);
   }

#ifdef TAPI_VERSION4
   datamap.dev = nDev;
   datamap.ch = nDataCh;
   nDevTmp = datamap.dev;
#endif /* TAPI_VERSION4 */
   datamap.nDstCh = nAddPhoneCh;

   if (IFX_TRUE == fDoMapping)
   {
      /* Map phone channel to data channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Map phone ch %d to data ch %d\n",
          (int)nAddPhoneCh, (int)nDataCh));

      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_DATA_ADD, (IFX_int32_t) &datamap,
               nDevTmp, nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
              ("Err, mapping phone ch %d to data ch %d using fd %d. "
               "              (File: %s, line: %d)\n",
              (int) nAddPhoneCh, (int) nDataCh, (int) fd_ch,
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   else
   {
      /* Unmap phone channel to data channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Unmap phone ch %d from data ch %d\n",
          (int)nAddPhoneCh, (int)nDataCh));

      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_DATA_REMOVE, (IFX_int32_t) &datamap,
               nDevTmp, nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
              ("Err, unmapping phone ch %d from data ch %d using fd %d. "
               "              (File: %s, line: %d)\n",
              (int) nAddPhoneCh, (int) nDataCh, (int) fd_ch,
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* VOIP_MapPhoneToData() */

#ifdef TD_IPV6_SUPPORT
/**
   Initialize socket for incoming voice packets for IPv6

   \param pCtrl   - pointer to control structure
   \param pProgramArg - pointer to program arguments
   \param nChIdx - channel index
   \param nPort  - which port is socket using

   \return socket number or NO_SOCKET if error.
*/
IFX_int32_t VOIP_InitUdpSocketIPv6(CTRL_STATUS_t *pCtrl,
                                   PROGRAM_ARG_t* pProgramArg,
                                   IFX_int32_t nChIdx,
                                   IFX_int32_t* nPort)
{
   IFX_int32_t nSockFd = NO_SOCKET;
   TD_OS_sockAddr_t oAddrIPv6;

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, NO_SOCKET);
   TD_PTR_CHECK(nPort, NO_SOCKET);
   TD_PARAMETER_CHECK((0 > nChIdx), nChIdx, NO_SOCKET);

   if(IFX_SUCCESS != TD_SOCK_IPv6Create(TD_OS_SOC_TYPE_DGRAM, &nSockFd))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, InitUdpSockeIPv6t: Can't create UDP socket, %s. "
             "(File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return NO_SOCKET;
   }

   TD_SOCK_AddrCpy(&oAddrIPv6, &pCtrl->oIPv6_Ctrl.oAddrIPv6);

/*   my_addr.sin_port = htons(VOICE_UDP_PORT + nChIdx + nFreeSocketPortAvail);*/

   /* According to the standard it is recomended to send RTP packets on even
    ports while odd ones are reserved for RTCP */

   TD_SOCK_PortSet(&oAddrIPv6,
                   (VOICE_UDP_PORT + 2 * nChIdx + nFreeSocketPortAvail));

   if(IFX_SUCCESS != TD_OS_SocketBind(nSockFd, &oAddrIPv6))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, InitUdpSocket: Can't bind to port, %s. "
            "(File: %s, line: %d)\n",
            strerror(errno), __FILE__, __LINE__));
      close(nSockFd);
      return NO_SOCKET;
   }

#ifdef LINUX
   /* make the socket non blocking */
   fcntl(nSockFd, F_SETFL, O_NONBLOCK);
#endif /* LINUX */

   *nPort = TD_SOCK_PortGet(&oAddrIPv6);

   return nSockFd;
} /* () */
#endif /* TD_IPV6_SUPPORT */

/**
   Initialize socket for incoming voice packets

   \param pProgramArg - pointer to program arguments
   \param nChIdx - channel index
   \param nPort  - which port is socket using

   \return socket number or NO_SOCKET if error.
*/
IFX_int32_t VOIP_InitUdpSocket(PROGRAM_ARG_t* pProgramArg,
                               IFX_int32_t nChIdx,
                               IFX_int32_t* nPort)
{
   TD_OS_socket_t socFd = NO_SOCKET;
   TD_OS_sockAddr_t my_addr;

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, NO_SOCKET);
   TD_PTR_CHECK(nPort, NO_SOCKET);
   TD_PARAMETER_CHECK((0 > nChIdx), nChIdx, NO_SOCKET);

   if(IFX_SUCCESS != TD_OS_SocketCreate(TD_OS_SOC_TYPE_DGRAM, &socFd))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, InitUdpSocket: Can't create UDP socket, %s. "
            "(File: %s, line: %d)\n",
            strerror(errno), __FILE__, __LINE__));
      return NO_SOCKET;
   }

   memset(&my_addr, 0, sizeof(my_addr));
   TD_SOCK_FamilySet(&my_addr, AF_INET);

/*   my_addr.sin_port = htons(VOICE_UDP_PORT + nChIdx + nFreeSocketPortAvail);*/

   /* According to the standard it is recomended to send RTP packets on even
    ports while odd ones are reserved for RTCP */

   TD_SOCK_PortSet(&my_addr,
                   (VOICE_UDP_PORT + 2 * nChIdx + nFreeSocketPortAvail));

   if (0 != TD_SOCK_AddrIPv4Get(&pProgramArg->oMy_IP_Addr))
   {
      TD_SOCK_AddrIPv4Set(&my_addr,
                          TD_SOCK_AddrIPv4Get(&pProgramArg->oMy_IP_Addr));
   }
   else
   {
      TD_SOCK_AddrIPv4Set(&my_addr, htonl(INADDR_ANY));
   }

   if(IFX_SUCCESS != TD_OS_SocketBind(socFd, &my_addr))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, InitUdpSocket: Can't bind to port, %s. "
            "(File: %s, line: %d)\n",
            strerror(errno), __FILE__, __LINE__));
      TD_OS_SocketClose(socFd);
      return NO_SOCKET;
   }

#ifdef LINUX
   /* make the socket non blocking */
   fcntl(socFd, F_SETFL, O_NONBLOCK);
#endif /* LINUX */

   *nPort = TD_SOCK_PortGet(&my_addr);

   return socFd;
} /* VOIP_InitUdpSocket() */


/**
   Handles data from socket

   \param pPhone - pointer to PHONE_t
   \param pConn  - pointer to phone connection
   \param fQosHandlesData - if IFX_TRUE QoS is handling data,
                            if IFX_FALSE we handle data.

   \return IFX_SUCCESS on ok otherwise IFX_ERROR
*/
IFX_return_t VOIP_HandleSocketData(PHONE_t* pPhone,
                                   CONNECTION_t* pConn,
                                   IFX_boolean_t fQosHandlesData)
{
   /* buffer size increased because packets exceed 400 B
      for some codecs with bigger packetization lengths */
   static IFX_char_t buf[MAX_PACKET_SIZE];
   IFX_return_t ret = IFX_SUCCESS;
   TD_OS_sockAddr_t oFromAddr;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pConn->nUsedSocket), pConn->nUsedSocket, IFX_ERROR);

   ret = TD_OS_SocketRecvFrom(pConn->nUsedSocket, buf, sizeof(buf), &oFromAddr);

   /* aditional check if peer address is the same as packet from address */
   if (IFX_TRUE !=  TD_SOCK_AddrCompare(&pConn->oConnPeer.oRemote.oToAddr,
                                        &oFromAddr, IFX_TRUE))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Received from [%s]:%d\n",
             TD_GetStringIP(&oFromAddr),
             TD_SOCK_PortGet(&oFromAddr)));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Should be from [%s]:%d\n",
             TD_GetStringIP(&pConn->oConnPeer.oRemote.oToAddr),
             TD_SOCK_PortGet(&pConn->oConnPeer.oRemote.oToAddr)));
   }

   /* check if recvfrom() error */
   if (0 > ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, reading from socket %d, %s. (File: %s, line: %d)\n",
            (IFX_int32_t) pConn->nUsedSocket, strerror(errno),
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if recvfrom() read data */
   else if (0 == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("No data read from socket %d.\n",
             (IFX_int32_t) pConn->nUsedSocket));
      return IFX_ERROR;
   }
   /* data read handle it */
   else
   {
      /* check if QoS handles data */
      if (IFX_TRUE == fQosHandlesData)
      {
         /* UDP redirection is handling voice streaming */
         return IFX_SUCCESS;
      }
      /* send packets only when connection in active state */
      if (TD_GET_STATE_INFO(pPhone->rgStateMachine[FXS_SM].nState,
                            TD_STATE_DATA_TRANSMIT))
      {
         /* application handles the data not QoS */
         ret = TD_OS_DeviceWrite(pConn->nUsedCh_FD, buf, ret);
         /* check if write successfull */
         if (0 > ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                 ("Err, writing data to device, %s. (File: %s, line: %d)\n",
                  strerror(errno), __FILE__, __LINE__));
            return IFX_ERROR;
         }
         else if (0 == ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("No data written to device.\n"));
            return IFX_ERROR;
         }
      } /* if right state */
   } /* if ( 0 >/=/< ret ) */
   return IFX_SUCCESS;
} /* VOIP_HandleSocketData() */


/**
   Handles data from chip

   \param pCtrl - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param nDataCh_FD - data channel file descriptor
   \param pConn - pointer to phone connection
   \param fQosHandlesData - if IFX_TRUE QoS is handling data,
                            if IFX_FALSE we handle data.

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t VOIP_HandleData(CTRL_STATUS_t* pCtrl,
                             PHONE_t* pPhone,
                             IFX_int32_t nDataCh_FD,
                             CONNECTION_t* pConn,
                             IFX_boolean_t fQosHandlesData)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t len = 0;
   /* buffer size increased because packets exceed 400 B
      for some codecs with bigger packetization lengths */
   static IFX_char_t buf[MAX_PACKET_SIZE];

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Get data from driver - DTMF, talking on phone */
   ret = TD_OS_DeviceRead(nDataCh_FD, buf, sizeof(buf));
   /* check if no error occured during read */
   if (0 > ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("No data read from device, fd %d. (File: %s, line: %d)\n",
            (int) nDataCh_FD, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if QoS handles data */
   if (IFX_TRUE == fQosHandlesData)
   {
      /* Don`t use application to handle the data, QoS is used */
      return IFX_SUCCESS;
   }
   /* check if data was read */
   if (0 < ret)
   {
      /* send packets only when connection in active state */
      if (TD_GET_STATE_INFO(pPhone->rgStateMachine[FXS_SM].nState,
                            TD_STATE_DATA_TRANSMIT))
      {
         /* check connection type */
         if (LOCAL_CALL == pConn->nType ||
             LOCAL_BOARD_CALL == pConn->nType)
         {
            /* local call send packet to data channel not supported for
             * conference */
            if (NO_CONFERENCE == pPhone->nConfIdx)
            {
               /* Local call - write to data channel */
               len = TD_OS_DeviceWrite(pConn->oConnPeer.oLocal.pPhone->nDataCh_FD,
                                       buf, ret);
               /* check if write successfull */
               if (0 > len)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err, writing data to device %d, %s. "
                         "(File: %s, line: %d)\n",
                         (int) pConn->oConnPeer.oLocal.pPhone->nDataCh_FD,
                         strerror(errno), __FILE__, __LINE__));
                  return IFX_ERROR;
               }
               else if (0 == len)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                        ("No data written to device %d.\n",
                         (int) pConn->oConnPeer.oLocal.pPhone->nDataCh_FD));
                  return IFX_ERROR;
               }
            } /* NO_CONFERENCE == pPhone->nConfIdx */
         }
         else if (EXTERN_VOIP_CALL == pConn->nType)
         {
            /* External call send data through socket */
            len = TD_OS_SocketSendTo(pConn->nUsedSocket, buf, ret,
                                     &pConn->oConnPeer.oRemote.oToAddr);
            if (ret != len)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, calling sendto(), %s, socket %d, data ch %d."
                      " (File: %s, line: %d)\n",
                      strerror(errno), (int) pConn->nUsedSocket,
                      (int) pConn->nUsedCh, __FILE__, __LINE__));
               return IFX_ERROR;
            }
         } /* if (... == pConn->nType) */
      } /* if in active state state */
   } /* if data was read */

   return IFX_SUCCESS;
} /* VOIP_HandleData() */

/**
   Reads data from file descriptor (can be data channel or socket) to empty it.

   \param nFd  - file descriptor to read from

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t VOIP_DiscardData(IFX_int32_t nFd)
{
   static IFX_char_t buf[MAX_PACKET_SIZE];

   /* check input parameters */
   TD_PARAMETER_CHECK((0 >= nFd), nFd, IFX_ERROR);

   /* read data from fd */
   if (0 > TD_OS_DeviceRead(nFd, buf, sizeof(buf)))
   {
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

/**
   Get file descriptor of device for data channel.

   \param nDataCh - data channel
   \param pBoard - board handle

   \return device connected to this channel or NO_DEVICE_FD if none
*/
IFX_int32_t VOIP_GetFD_OfCh(IFX_int32_t nDataCh, BOARD_t* pBoard)
{
   /* check input parameters */
   TD_PTR_CHECK(pBoard, NO_DEVICE_FD);
   TD_PARAMETER_CHECK((0 > nDataCh), nDataCh, NO_DEVICE_FD);
   TD_PARAMETER_CHECK((pBoard->nMaxCoderCh <= nDataCh), nDataCh, NO_DEVICE_FD);

   return Common_GetFD_OfCh(pBoard, nDataCh);
} /* VOIP_GetFD_OfCh() */

