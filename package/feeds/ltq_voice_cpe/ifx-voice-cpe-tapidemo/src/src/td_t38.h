#ifndef _TD_T38_H
#define _TD_T38_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : td_t38.h
   Description : This file provides functions for fax T.38 transmission.
 ****************************************************************************
   \file td_t38.h

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "tapidemo.h"
#include "common.h"

/* ============================= */
/* Global Defines                */
/* ============================= */

#ifdef TD_T38_FAX_TEST

/*******************************************************************************
*      DEFAULT FAX DATA PUMP CONFIGURATION                                     *
*******************************************************************************/
/** Modulation buffer size (in units of 0.625ms).
       Range:
       - Minimum: 1 (0.625ms)
       - Maximum: 320 (200ms) */
#define TD_T38_FAX_DEF_FDP_MOBSZ    320

/** Required modulation buffer fill level (in units of 0.625ms)
    before for modulation starts. */
#define TD_T38_FAX_DEF_FDP_MOBSM    320

/** Required modulation buffer fill level (in units of 0.625ms)
    before the modulation requests more data. */
#define TD_T38_FAX_DEF_FDP_MOBRD    223

/** Required demodulation buffer level (in units of 0.625ms)
    before the demodulator sends data. */
#define TD_T38_FAX_DEF_FDP_DMBSD    32

/*******************************************************************************
*      DEFAULT INITIAL PARAMETERS FOR THE STACK SESSION                        *
*******************************************************************************/

/** Data pump demodulation gain RX. */
#define TD_T38_FAX_DEF_CFG_GAIN_RX              3

/** Data pump demodulation gain TX. */
#define TD_T38_FAX_DEF_CFG_GAIN_TX              -4

/** The T.38 options define protocol features and can be ORed. */
#define TD_T38_FAX_DEF_CFG_OPT_MASK    \
   (IFX_TAPI_T38_FEAT_NON | IFX_TAPI_T38_FEAT_ASN1)

/** IFP packets send interval (in ms). */
#define TD_T38_FAX_DEF_CFG_IFPSI                20

/** Number of packets to calculate FEC (Forward Error Correction)
    when redundancy is used during the session. */
#define TD_T38_FAX_DEF_CFG_RED_PCK_FEC          0

/** Number of packets to calculate FEC (Forward Error Correction)
    when forward error correction is used during the session. */
#define TD_T38_FAX_DEF_CFG_FEC_PCK_FEC          0

/** Data wait time (in ms). */
#define TD_T38_FAX_DEF_CFG_DWT                  0

/** Timeout for start of T.38 modulation (in ms). */
#define TD_T38_FAX_DEF_CFG_MOD_START_TIME       0

/** Time to insert spoofing during automatic modulation (in ms). */
#define TD_T38_FAX_DEF_CFG_SPOOF_INS_TIME       0

/** Desired output power level (in -dBm). */
#define TD_T38_FAX_DEF_CFG_OUT_POW_LEVEL        13

/** Number of additional recovery data packets sent on high-speed
    FAX transmissions when redundancy is used during the session. */
#define TD_T38_FAX_DEF_CFG_RED_PCK_RECOV_HIG    1

/** Number of additional recovery data packets sent on lowspeed
    FAX transmissions when redundancy is used during the session. */
#define TD_T38_FAX_DEF_CFG_RED_PCK_RECOV_LOW    4

/** Number of additional recovery data packets sent on high-speed
    FAX transmissions, forward error correction is used during the session. */
#define TD_T38_FAX_DEF_CFG_FEC_PCK_RECOV_HIG    0

/** Number of additional recovery data packets sent on lowspeed
    FAX transmissions, forward error correction is used during the session. */
#define TD_T38_FAX_DEF_CFG_FEC_PCK_RECOV_LOW    0

/** Number of additional recovery T30_INDICATOR packets. */
#define TD_T38_FAX_DEF_CFG_PCK_RECOV_IND        0

/** Data bytes of NSX field. The number of valid data is set in 'nNsxLen'. */
#define TD_T38_FAX_DEF_CFG_NSX                  "\xCF"

/** Length (bytes) of valid data in the 'aNsx' field. */
#define TD_T38_FAX_DEF_CFG_NSX_LEN              2

/** number of chars needed for one byte */
#define TD_T38_ONE_BYTE_IN_HEX                  2

/** fill pStr with hexadecimal representation of NSX buffer */
#define TD_T38_GET_NSX_STRING(m_pStr, m_nCounter, m_pNSX, m_nNSX_Len, m_nTmpVal, m_ConnID) \
   m_nCounter = 0; \
   sprintf(&m_pStr[0], "0x%02X", m_pNSX[m_nCounter]); \
   for (m_nCounter=1; m_nCounter < m_nNSX_Len; m_nCounter++) \
   { \
      m_nTmpVal = (4 + (TD_T38_ONE_BYTE_IN_HEX * (m_nCounter - 1))); \
      if((m_nTmpVal >= TD_MAX_NAME) || (m_nCounter >= IFX_TAPI_T38_NSXLEN)) \
      { \
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_ConnID, \
               ("Err, Can't set hexadecimal representation of NSX buffer.\n" \
                "(File: %s, line: %d)\n", __FILE__, __LINE__)); \
         return IFX_ERROR; \
      } \
      sprintf(&m_pStr[m_nTmpVal], "%02X", m_pNSX[m_nCounter]); \
   }

#endif /* TD_T38_FAX_TEST */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

extern TD_ENUM_2_NAME_t TD_rgT38EcMode_Name[];
extern TD_ENUM_2_NAME_t TD_rgT38Protocol_Name[];
extern TD_ENUM_2_NAME_t TD_rgT38RMM_Name[];

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
IFX_return_t TD_T38_GetStatistics(PHONE_t* pPhone, IFX_int32_t nFaxCh);
IFX_return_t TD_T38_SetUp(PHONE_t* pPhone, CONNECTION_t* pConn);
IFX_return_t TD_T38_Start(PHONE_t* pPhone, IFX_int32_t nFaxCh);
IFX_return_t TD_T38_Stop(PHONE_t* pPhone, IFX_int32_t nFaxCh);
IFX_return_t TD_T38_Init(BOARD_t* pBoard);

#endif /* _TD_T38_H */

