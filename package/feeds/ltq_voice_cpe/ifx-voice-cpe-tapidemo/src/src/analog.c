/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file analog.c
   \date 2006-04-14
   \brief ALM module implementation for application.

   This file includes methods which initialize ALM module and work with
   phone channels.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "common.h"
#include "analog.h"

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Map phone channel to phone chanel.

   \param nPhoneCh    - target phone channel
   \param nAddPhoneCh - which phone channel to add
   \param fDoMapping  - flag if mapping should be done (TD_MAP_ADD),
                        or unmapping (TD_MAP_REMOVE)
   \param pBoard - pointer to board
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t ALM_MapToPhone(IFX_int32_t nPhoneCh,
                            IFX_int32_t nAddPhoneCh,
                            TD_MAP_ADD_REMOVE_t fDoMapping,
                            BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_PHONE_t phonemap;
   IFX_int32_t fd_ch = -1;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nPhoneCh), nPhoneCh, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nAddPhoneCh), nAddPhoneCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxAnalogCh < nPhoneCh), nPhoneCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxAnalogCh < nAddPhoneCh), nAddPhoneCh,
                      IFX_ERROR);

   memset(&phonemap, 0, sizeof(IFX_TAPI_MAP_PHONE_t));

   fd_ch = ALM_GetFD_OfCh(pBoard, nPhoneCh);

   phonemap.nPhoneCh = nAddPhoneCh;
#ifdef TAPI_VERSION4
   phonemap.nChType = IFX_TAPI_MAP_TYPE_DEFAULT;
   if (pBoard->fSingleFD)
   {
      phonemap.dev = 0;
      phonemap.ch = nPhoneCh;
   }

   nDevTmp = phonemap.dev;

#endif /* TAPI_VERSION4 */

   if (TD_MAP_ADD == fDoMapping)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Map phone ch %d to phone ch %d\n",
          (int) nAddPhoneCh, (int) nPhoneCh));


      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_PHONE_ADD,
                     (IFX_int32_t) &phonemap, nDevTmp, nSeqConnId);

      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, mapping phone ch %d to phone ch %d using fd %d. "
             "(File: %s, line: %d)\n",
             (int) nAddPhoneCh, (int) nPhoneCh, (int) fd_ch,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Unmap phone ch %d from phone ch %d\n",
             (int) nAddPhoneCh, (int) nPhoneCh));


      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_PHONE_REMOVE,
                     (IFX_int32_t) &phonemap, nDevTmp, nSeqConnId);

      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, unmapping phone ch %d from phone ch %d (fd %d). "
             "(File: %s, line: %d)\n",
             (int) nAddPhoneCh, (int) nPhoneCh, (int) fd_ch,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* ALM_MapToPhone() */


/**
   Get file descriptor of device for phone channel.

   \param pBoard - pointer to board
   \param nPhoneCh - phone channel

   \return device connected to this channel or NO_DEVICE_FD if none
*/
IFX_int32_t ALM_GetFD_OfCh(BOARD_t* pBoard, IFX_int32_t nPhoneCh)
{
   TD_PTR_CHECK(pBoard, NO_DEVICE_FD);
   TD_PARAMETER_CHECK((0 > nPhoneCh), nPhoneCh, NO_DEVICE_FD);
   TD_PARAMETER_CHECK((pBoard->nMaxAnalogCh < nPhoneCh), nPhoneCh,
                       NO_DEVICE_FD);

   return Common_GetFD_OfCh(pBoard, nPhoneCh);
} /* ALM_GetFD_OfCh() */

