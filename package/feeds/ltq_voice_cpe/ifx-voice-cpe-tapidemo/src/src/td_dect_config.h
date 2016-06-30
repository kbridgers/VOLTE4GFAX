
#ifndef __TD_DECT_CONFIG_H__
#define __TD_DECT_CONFIG_H__

/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_dect_config.h
   \date 2010-05-27
   \brief Declarations of functions to read and write configuration file.

   This file includes methods which configure DECT module with use of
   configuration file.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "ifx_common_defs.h"
             
/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* File td_dect_config.h | c is used not only by Tapidemo, e.g. CLI */
/* application also uses these files. */
/* In such case we need to add following redefinitions to avoid compilation issue. */
#ifndef TAPIDEMO_APP

#define TD_PTR_CHECK(m_ptr, m_ret) {}
#define TD_PARAMETER_CHECK(m_expr, m_param, m_ret) {}

#define TRACE(name,level,message) \
      do \
      { \
         printf message; \
      } while(0)

#define TD_TRACE(name,level, m_ConnID, message) \
      do \
      { \
         printf message; \
      } while(0)

#define IFX_char_t            char8
#define IFX_uint8_t           uchar8
#define IFX_uint16_t          uint16
#define IFX_int32_t           int32
#define IFX_NULL              NULL
#define IFX_return_t          int32

#define IFX_ERROR -1

#define TD_DECT_CONFIG_IFX_CM_SetRcCfgData   IFX_CM_SetRcCfgData
#define TD_DECT_CONFIG_GetRcCfgData          IFX_CM_GetRcCfgData
#define TD_DECT_CONFIG_RcTpcSet              ifx_setrc_Tpc
#define TD_DECT_CONFIG_RcTpcGet              ifx_getrc_Tpc
#define TD_DECT_CONFIG_RcBmcSet              ifx_setrc_Bmc
#define TD_DECT_CONFIG_RcBmcGet              ifx_getrc_Bmc
#define TD_DECT_CONFIG_RcFreqGet             ifx_getrc_Freq
#define TD_DECT_CONFIG_RcFreqSet             ifx_setrc_Freq
#define TD_DECT_CONFIG_RcOscGet              ifx_getrc_Osc
#define TD_DECT_CONFIG_RcOscSet              ifx_setrc_Osc
#define TD_DECT_CONFIG_RcGfskGet             ifx_getrc_Gfsk
#define TD_DECT_CONFIG_RcGfskSet             ifx_setrc_Gfsk
#define TD_DECT_CONFIG_RcRfpiGet             ifx_getrc_Rfpi
#define TD_DECT_CONFIG_RcRfpiSet             ifx_setrc_Rfpi
#define TD_DECT_CONFIG_RcRfModeGet           ifx_getrc_RfMode
#define TD_DECT_CONFIG_RcRfModeSet           ifx_setrc_RfMode

/** A type for handling boolean issues. */
typedef enum {
   /** false */
   IFX_FALSE = 0,
   /** true */
   IFX_TRUE = 1
} IFX_boolean_t;

/** enum used by macros to decide if call return. */
typedef enum _TD_RETURN_CODE_t
{
   TD_NO_RETURN = 0,
   TD_RETURN = 1
} TD_RETURN_CODE_t;

#endif /* TAPIDEMO_APP */

#define IFX_CM_MAX_TAG_LEN                64
#define IFX_CM_MAX_BUFF_LEN               5192
#define IFX_CM_BUF_SIZE_50K               10240
#define IFX_CM_MAX_FILENAME_LEN           256
#define IFX_CM_FILE_RC_CONF               "/flash/rc.conf"
/* BMC params */
#define IFX_CM_TAG_BMC                    "BmcParams"
#define IFX_CM_TAG_BMC_ELEMENT_CNT        16
#define IFX_CM_TAG_BMC_RSSIFREE           "BmcParams_0_RssiFreeLevel"
#define IFX_CM_TAG_BMC_RSSIBUSY           "BmcParams_0_RssiBusyLevel"
#define IFX_CM_TAG_BMC_BRLIMIT            "BmcParams_0_BearerChgLimit"
#define IFX_CM_TAG_BMC_DEFANT             "BmcParams_0_DefaultAntenna"
#define IFX_CM_TAG_BMC_WOPNSF             "BmcParams_0_WOPNSF"
#define IFX_CM_TAG_BMC_WWSF               "BmcParams_0_WWSF"
#define IFX_CM_TAG_BMC_DRONCTRL           "BmcParams_0_CNTUPCtrlReg"
#define IFX_CM_TAG_BMC_DVAL               "BmcParams_0_DelayReg"
#define IFX_CM_TAG_BMC_HOFRAME            "BmcParams_0_HandOverEvalper"
#define IFX_CM_TAG_BMC_SYNCMODE           "BmcParams_0_SYNCMCtrlReg"
/* TPC params */
#define IFX_CM_TAG_TPC                    "TransPower"
#define IFX_CM_TAG_TPC_ELEMENT_CNT        25
/* FREQ params */
#define IFX_CM_TAG_FREQ                   "CtrySet"
#define IFX_CM_TAG_FREQ_ELEMENT_CNT       5
/* OSC params */
#define IFX_CM_TAG_OSC                    "Osctrim"
#define IFX_CM_TAG_OSC_ELEMENT_CNT        5
/* Gfsk params */
#define IFX_CM_TAG_GFSK                   "Gfsk"
#define IFX_CM_TAG_GFSK_ELEMENT_CNT       4
/* RFPI params */
#define IFX_CM_TAG_RFPI                   "Rfpi"
#define IFX_CM_TAG_RFPI_ELEMENT_CNT       8
/* Rfmode params */
#define IFX_CM_TAG_RFMODE                 "Rfmode"
#define IFX_CM_TAG_RFMODE_ELEMENT_CNT     5

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_int32_t TD_DECT_CONFIG_IFX_CM_SetRcCfgData (const IFX_char_t * pFileName,
                                                const IFX_char_t * pTag,
                                                IFX_int32_t nDataCount,
                                                const IFX_char_t * pData, ...);
IFX_int32_t TD_DECT_CONFIG_GetRcCfgData(IFX_char_t *pFileName,
                                        IFX_char_t *pTag,
                                        IFX_char_t *pData,
                                        IFX_char_t *pRetValue);


IFX_return_t TD_DECT_CONFIG_RcTpcSet(x_IFX_DECT_TransmitPowerParam *pxTPCParams);
IFX_return_t TD_DECT_CONFIG_RcTpcGet(x_IFX_DECT_TransmitPowerParam *pxTPCParams);

IFX_return_t TD_DECT_CONFIG_RcBmcSet(x_IFX_DECT_BMCRegParams *pxBMC);
IFX_return_t TD_DECT_CONFIG_RcBmcGet(x_IFX_DECT_BMCRegParams *pxBMC);

IFX_return_t TD_DECT_CONFIG_RcFreqGet(IFX_uint8_t *pucFreqTx,
                                      IFX_uint8_t *pucFreqRx,
                                      IFX_uint8_t *pucFreqRange);
IFX_return_t TD_DECT_CONFIG_RcFreqSet(IFX_uint8_t ucFreqTx,
                                      IFX_uint8_t ucFreqRx,
                                      IFX_uint8_t ucFreqRange);

IFX_return_t TD_DECT_CONFIG_RcOscGet(IFX_uint16_t *puiOscTrimValue,
                                     IFX_uint8_t *pucP10Status);
IFX_return_t TD_DECT_CONFIG_RcOscSet(IFX_uint16_t uiOscTrimValue,
                                     IFX_uint8_t ucP10Status);

IFX_return_t TD_DECT_CONFIG_RcGfskGet(IFX_uint16_t *puiGfskValue);
IFX_return_t TD_DECT_CONFIG_RcGfskSet(IFX_uint16_t uiGfskValue);


IFX_return_t TD_DECT_CONFIG_RcRfpiGet(IFX_uint8_t *pcRFPI);
IFX_return_t TD_DECT_CONFIG_RcRfpiSet(IFX_uint8_t *pcRFPI);


IFX_return_t TD_DECT_CONFIG_RcRfModeGet(IFX_uint8_t *pucRFMode,
                                        IFX_uint8_t *pucChannelNumber,
                                        IFX_uint8_t *pucSlotNumber);
IFX_return_t TD_DECT_CONFIG_RcRfModeSet(IFX_uint8_t ucRFMode,
                                        IFX_uint8_t ucChannelNumber,
                                        IFX_uint8_t ucSlotNumber);
#endif /*__TD_DECT_CONFIG_H__*/

