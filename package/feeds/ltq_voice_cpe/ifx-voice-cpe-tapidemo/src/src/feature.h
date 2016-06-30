#ifndef _FEATURE_H
#define _FEATURE_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : feature.h
   Date        : 2006-04-06
   Description : This file enchance tapi demo with additional features:
                 - AGC ( Automated Gain Control )
   \file

   \remarks

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/** AGC related stuff */
/** min/max parameters: */

/** "Compare Level", this is the target level in 'dB', MAX is 0dB. */
#define TAPIDEMO_AGC_CONFIG_COM_MAX         0
/** "Compare Level", this is the target level in 'dB', MIN is -50dB. */
#define TAPIDEMO_AGC_CONFIG_COM_MIN       -50
/** "Maximum Gain", maximum gain that we'll be applied to the signal in
    'dB', MAX is 48dB. */
#define TAPIDEMO_AGC_CONFIG_GAIN_MAX       48
/** "Maximum Gain", maximum gain that we'll be applied to the signal in
    'dB', MIN is 0dB. */
#define TAPIDEMO_AGC_CONFIG_GAIN_MIN        0
/** "Maximum Attenuation for AGC", maximum attenuation that we'll be applied
    to the signal in 'dB', MAX is 0dB. */
#define TAPIDEMO_AGC_CONFIG_ATT_MAX         0
/** "Maximum Attenuation for AGC", maximum attenuation that we'll be applied
    to the signal in 'dB', MIN is -42dB. */
#define TAPIDEMO_AGC_CONFIG_ATT_MIN       -42
/** "Minimum Input Level", signals below this threshold won't be processed
    by AGC in 'dB', MAX is -25 dB. */
#define TAPIDEMO_AGC_CONFIG_LIM_MAX       -25
/** "Minimum Input Level", signals below this threshold won't be processed
    by AGC in 'dB', MIN is -60 dB. */
#define TAPIDEMO_AGC_CONFIG_LIM_MIN       -60

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t FEATURE_AGC_Enable(IFX_int32_t nDataCh_FD,
                                PHONE_t* pPhone,
                                IFX_boolean_t fEnable);

IFX_return_t FEATURE_AGC_Cfg(IFX_int32_t nDataCh_FD,
                             PHONE_t* pPhone,
                             IFX_int32_t nAGC_CompareLvl,
                             IFX_int32_t nAGC_MaxGain,
                             IFX_int32_t nAGC_MaxAttenuation,
                             IFX_int32_t nAGC_MinInputLvl);

IFX_return_t FEATURE_SetSpeechRange(PHONE_t* pPhone,
                                    IFX_int32_t nDataCh_FD,
                                    IFX_TAPI_LINE_TYPE_t eLineType,
                                    IFX_TAPI_COD_TYPE_t eCoderType,
                                    IFX_boolean_t fWideBand);

IFX_return_t FEATURE_SetPCM_SpeechRange(PHONE_t* pPhone,
                                        IFX_boolean_t fWideBand);

#endif /* _FEATURE_H */

