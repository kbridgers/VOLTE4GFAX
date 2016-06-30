
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file abstract.c
   \date 2006-01-25
   \brief Abstraction methods between application and board, chip.

   Functions here return right phone, according to phone number, socket,
   data or phone channel and also for the right board.
*/


/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "abstract.h"
#include "voip.h"
#include "pcm.h"

/* ============================= */
/* Global Structures             */
/* ============================= */

extern TD_ENUM_2_NAME_t TD_rgCallTypeName[];

/** board ID (usually last byte of IPv4 address) */
IFX_uint8_t g_nID = 0x00;

/** set to IFX_TRUE if prefix before each trace should be printed */
IFX_boolean_t g_bAddPrefix = IFX_TRUE;

#define TD_PREFIX_NAME              "td"
#define TD_PREFIX_NAME_LENGTH       2
/** prefix length with two spaces and number(8 diggits) */
#define TD_PREFIX_LENGTH            (TD_PREFIX_NAME_LENGTH + 1 + 8 + 1)

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */

#ifndef EASY336
/**
   Return pointer to startup mapping table for specific board.

   \param pBoard - pointer to board structure

   \return Pointer to startup mapping table is returned.
*/

const STARTUP_MAP_TABLE_t* ABSTRACT_GetStartupMapTbl(BOARD_t* pBoard)
{
   return pBoard->pChStartupMapping;
} /* ABSTRACT_GetStartupMapTbl() */
#endif /* EASY336 */

/**
   Map channels at startup.

   \param pCtrl - handle to connection control structure
   \param pBoard - pointer to board structure
   \param nSeqConnId    - Seq Conn ID here usually set to 0 (initialization)

   \return IFX_SUCCESS everything ok, otherwise IFX_ERROR

   \remark By default mapping is 0 - 0 - 0, 1 - 1 - 1. It means that phone
           channel 0 is mapped to data 0 and pcm 0
*/
IFX_return_t ABSTRACT_DefaultMapping(CTRL_STATUS_t* pCtrl, BOARD_t* pBoard,
                                     IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
#if (!defined EASY336 && !defined XT16)
   IFX_int32_t data_ch = 0;
   IFX_int32_t fd_ch = -1;
#endif
   const STARTUP_MAP_TABLE_t* p_map_table = IFX_NULL;
#if (!defined EASY336 && !defined XT16)
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
#endif /* (!defined EASY336 && !defined XT16) */

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

   p_map_table = pBoard->pChStartupMapping;
#ifdef TAPI_VERSION4
   /* #warning !! for now can handle only first device */
   datamap.dev = 0;
   datamap.nPlayStart = IFX_TAPI_MAP_DATA_UNCHANGED;
   datamap.nRecStart = IFX_TAPI_MAP_DATA_UNCHANGED;
   datamap.nChType = IFX_TAPI_MAP_TYPE_PHONE;
#endif /* TAPI_VERSION4 */
   if (0 >= pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("%s: This board doesn't have coder channels\n",
          pBoard->pszBoardName));
   }

   for (i = 0; i < pBoard->nMaxAnalogCh; i++)
   {
      pBoard->rgoPhones[i].nDataCh = TD_NOT_SET;
      pBoard->rgoPhones[i].nDataCh_FD = TD_NOT_SET;
      pBoard->rgoPhones[i].nPCM_Ch = TD_NOT_SET;
#if (!defined EASY336 && !defined XT16)
      data_ch = p_map_table[i].nDataCh;
      if ((data_ch >= 0) &&
          (0 < pBoard->nMaxCoderCh))
      {
         /* Map phone channel x to data channel y */
         datamap.nDstCh = p_map_table[i].nPhoneCh;
#ifdef TAPI_VERSION4
         datamap.ch = data_ch;
         nDevTmp = datamap.dev;
#endif /* TAPI_VERSION4 */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("%s: Map phone ch %d to data ch %d\n",
             pBoard->pszBoardName, datamap.nDstCh, (int) data_ch));

         /* Get file descriptor for this data channel */
         fd_ch = Common_GetFD_OfCh(pBoard, data_ch);

         ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_DATA_ADD,
                           (IFX_int32_t) &datamap, nDevTmp, nSeqConnId);
         if (IFX_ERROR == ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, failed mapping phone ch %d to data ch %d using fd &d. \n",
                (int) data_ch, (int) fd_ch));
            return IFX_ERROR;
         }

         VOIP_ReserveDataCh(data_ch, pBoard, nSeqConnId);

         pBoard->rgoPhones[i].nDataCh = data_ch;
      } /* if (data_ch >= 0) */

      pBoard->rgoPhones[i].nPhoneCh = p_map_table[i].nPhoneCh;

      if (pBoard->nMaxPCM_Ch > 0)
      {
         if (p_map_table[i].nPCM_Ch >= 0)
         {
            pBoard->rgoPhones[i].nPCM_Ch = p_map_table[i].nPCM_Ch;
            PCM_ReserveCh(p_map_table[i].nPCM_Ch, pBoard, TD_CONN_ID_INIT);
         }
      }
#else /* (!defined EASY336 && !defined XT16) */
      /* set COD and PCM channel nr. equal to phone channel nr., though this
       * is not true. This is just for Common_GetFD_OfCh function to
       * satisfy channel number range verification. */
      pBoard->rgoPhones[i].nDataCh = pBoard->rgoPhones[i].nPhoneCh;
      pBoard->rgoPhones[i].nPCM_Ch = pBoard->rgoPhones[i].nPhoneCh;
#endif /* (!defined EASY336 && !defined XT16) */
   } /* for */

   return ret;
} /* ABSTRACT_DefaultMapping() */

#ifndef EASY336
/**
   Unmaps all default mapping of channels (phone ch to data, pcm ch).

   \param pCtrl - handle to connection control structure
   \param pBoard - pointer to board structure
   \param nSeqConnId    - Seq Conn ID here usually set to 0 (initialization)

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t ABSTRACT_UnmapDefaults(CTRL_STATUS_t* pCtrl, BOARD_t* pBoard,
                                    IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
#if (!defined EASY336 && !defined XT16)
   IFX_int32_t fd_ch = -1;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
#endif /* (!defined EASY336 && !defined XT16) */

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

   if (pBoard->nMaxCoderCh > 0)
   {
      for (i = 0; i < pBoard->nMaxAnalogCh; i++)
      {
#if (!defined EASY336 && !defined XT16)
         datamap.nDstCh = pBoard->pChStartupMapping[i].nPhoneCh;
#ifdef TAPI_VERSION4
         datamap.ch = pBoard->pChStartupMapping[i].nDataCh;
         nDevTmp = datamap.dev;
#endif /* TAPI_VERSION4 */

         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("%s: Unmap phone ch %d from data ch %d\n",
             pBoard->pszBoardName,
             (IFX_int32_t)pBoard->pChStartupMapping[i].nPhoneCh,
             (IFX_int32_t)pBoard->pChStartupMapping[i].nDataCh));

         fd_ch = Common_GetFD_OfCh(pBoard,
                                   pBoard->pChStartupMapping[i].nDataCh);
         if (IFX_ERROR == fd_ch)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, Unable to get FD for ch %d. (File: %s, line: %d)\n",
                pBoard->pChStartupMapping[i].nDataCh,
                __FILE__, __LINE__));
            return IFX_ERROR;
         }

         ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_DATA_REMOVE,
                        (IFX_int32_t) &datamap, nDevTmp, nSeqConnId);
         if (IFX_ERROR == ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, unmapping phone ch %d from data ch %d using fd %d. "
                "(File: %s, line: %d)\n",
                (IFX_int32_t)pBoard->pChStartupMapping[i].nPhoneCh,
                pBoard->pChStartupMapping[i].nDataCh, (int) fd_ch,
                __FILE__, __LINE__));
            return IFX_ERROR;
         }

         /* Tell this data channel it is not mapped with phone channel */
         /* Because at startup it is already not mapped in our application, but
            its mapped only in VINETIC CHIP. */
         VOIP_FreeDataCh(pBoard->pChStartupMapping[i].nDataCh, pBoard,
                         nSeqConnId);

         /* check if PCM resources are available */
         if (pBoard->nMaxPCM_Ch > 0)
         {
            /* PCM channels are not mapped at startup with phone channels,
               they are just reserved. */
            PCM_FreeCh(pBoard->pChStartupMapping[i].nPCM_Ch, pBoard);
         }
#endif /* (!defined EASY336 && !defined XT16) */

         pBoard->rgoPhones[i].nDataCh = -1;
         pBoard->rgoPhones[i].nDataCh_FD = -1;
         pBoard->rgoPhones[i].nPhoneCh = i;
         pBoard->rgoPhones[i].nPCM_Ch = -1;
      }
   }

   return ret;
} /* ABSTRACT_UnmapDefaults() */

/**
   Map channels using custom map table.

   \param pBoard - pointer to board structure
   \param nMappingsNumber - number of mappings
   \param nSeqConnId    - Seq Conn ID here usually set to 0 (initialization)

   \return IFX_SUCCESS everything ok, otherwise IFX_ERROR

*/
IFX_return_t ABSTRACT_CustomMapping(BOARD_t* pBoard,
                                    IFX_int32_t nMappingsNumber,
                                    IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0, j;
   IFX_int32_t data_ch = 0;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_int32_t fd_ch = -1;
   const STARTUP_MAP_TABLE_t* p_map_table = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pBoard->pChCustomMapping, IFX_ERROR);

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

   /* get mapping structure */
   p_map_table = pBoard->pChCustomMapping;

   /* reset phone structures */
   for (i = 0; i < pBoard->nMaxAnalogCh; i++)
   {
      pBoard->rgoPhones[i].nDataCh = -1;
      pBoard->rgoPhones[i].nDataCh_FD = -1;
      pBoard->rgoPhones[i].nPCM_Ch = -1;

   } /* for all phones */
   /* map according to custom mapping table */
   for (j = 0; nMappingsNumber > j;j++)
   {
      for (i=0; i<pBoard->nMaxAnalogCh; i++)
      {
         /* check if right phone channel */
         if (p_map_table[j].nPhoneCh == pBoard->rgoPhones[i].nPhoneCh)
         {
            break;
         }
      }/* for all phones */
      /* check if pone channel was found */
      if (i >= pBoard->nMaxAnalogCh)
      {
         /* Wrong input arguments */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, Failed to find phone ch %d. (File: %s, line: %d)\n",
             p_map_table[j].nPhoneCh, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      else
      {
         /* get data channel number */
         data_ch = p_map_table[j].nDataCh;
         if (0 <= data_ch &&
             pBoard->nMaxCoderCh > data_ch)
         {
            /* Map phone channel x to data channel y */
            datamap.nDstCh = p_map_table[j].nPhoneCh;
#ifdef TAPI_VERSION4
            nDevTmp = datamap.dev;
#endif /* TAPI_VERSION4 */

            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("Map phone ch %d to data ch %d\n",
                datamap.nDstCh, (int) data_ch));

            /* Get file descriptor for this data channel */
            fd_ch = Common_GetFD_OfCh(pBoard, data_ch);

            ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_DATA_ADD,
                           (IFX_int32_t) &datamap, nDevTmp, nSeqConnId);
            if (IFX_ERROR == ret)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, mapping phone ch %d to data ch %d using fd &d."
                   "(File: %s, line: %d)\n",
                   (int) data_ch, (int) fd_ch, __FILE__, __LINE__));
               return IFX_ERROR;
            }

            VOIP_ReserveDataCh(data_ch, pBoard, nSeqConnId);

            pBoard->rgoPhones[i].nDataCh = data_ch;
            pBoard->rgoPhones[i].nDataCh_FD = fd_ch;
         }
         /* check if mapping was specified */
         else if (NO_MAPPING == p_map_table[j].nDataCh)
         {
            /* skip this channel */
            continue;
         }
         else
         {
            /* Wrong input arguments */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, Invalid data ch %d. (File: %s, line: %d)\n",
                p_map_table[j].nDataCh, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
      if (p_map_table[i].nPCM_Ch >= 0)
      {
         pBoard->rgoPhones[i].nPCM_Ch = p_map_table[i].nPCM_Ch;
         /* for test purpose we don't need to reserve PCM channel */
         /* PCM_ReserveCh(p_map_table[i].nPCM_Ch, pBoard); */
      }
   } /* for all mappings */

   return ret;
} /* ABSTRACT_CustomMapping() */

/**
   Unmaps all custom mapping of channels (phone ch to data, pcm ch).

   \param pBoard - pointer to board structure
   \param nSeqConnId    - Seq Conn ID here usually set to 0 (initialization)

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t ABSTRACT_UnmapCustom(BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   IFX_int32_t fd_ch = -1;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

   if (pBoard->nMaxCoderCh > 0)
   {
      for (i = 0; i < pBoard->nMaxAnalogCh; i++)
      {
         datamap.nDstCh = pBoard->rgoPhones[i].nPhoneCh;
#ifdef TAPI_VERSION4
         nDevTmp = datamap.dev;
#endif /* TAPI_VERSION4 */
         if (pBoard->rgoPhones[i].nDataCh >= 0)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("Unmap phone ch %d from data ch %d\n",
                (int)datamap.nDstCh, (int)pBoard->rgoPhones[i].nDataCh));

            fd_ch = Common_GetFD_OfCh(pBoard, pBoard->rgoPhones[i].nDataCh);

            ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_DATA_REMOVE,
                           (IFX_int32_t) &datamap, nDevTmp, nSeqConnId);
            if (IFX_ERROR == ret)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, unmapping phone ch %d from data ch %d using fd %d."
                   " (File: %s, line: %d)\n",
                   (int) i, (int) i, (int) fd_ch, __FILE__, __LINE__));
               return IFX_ERROR;
            }

            /* Tell this data channel it is not mapped with phone channel */
            /* Because at startup it is already not mapped in our application, but
               its mapped only in VINETIC CHIP. */
            VOIP_FreeDataCh(pBoard->rgoPhones[i].nDataCh, pBoard, nSeqConnId);

            /* PCM channels are not mapped at startup with phone channels, they are
               just reserved. */

            pBoard->rgoPhones[i].nDataCh = -1;
            pBoard->rgoPhones[i].nPhoneCh = i;
            pBoard->rgoPhones[i].nPhoneCh_FD =
               Common_GetFD_OfCh(pBoard, pBoard->rgoPhones[i].nPhoneCh);
         }
      } /* for (i = 0; i < pBoard->nMaxAnalogCh; i++) */
   } /* if (pBoard->nMaxCoderCh > 0) */

   return ret;
} /* ABSTRACT_UnmapCustom() */
#endif /* EASY336 */

/**
   Get pointer of PHONE according to phone channel.

   \param pBoard    - pointer to board structure
   \param nPhoneDev - phone device
   \param nPhoneCh - phone channel
   \param nSeqConnId    - Seq Conn ID here usually set to 0 (initialization)

   \return pointer to PHONE with this phone channel or IFX_NULL if none
*/
PHONE_t* ABSTRACT_GetPHONE_OfPhoneCh(BOARD_t *pBoard,
                                     IFX_uint16_t nPhoneDev,
                                     IFX_int32_t nPhoneCh,
                                     IFX_uint32_t nSeqConnId)
{
#ifdef TAPI_VERSION4
   IFX_int32_t i = 0;
#endif /* TAPI_VERSION4 */

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_NULL);
   TD_PARAMETER_CHECK((0 > nPhoneCh), nPhoneCh, IFX_NULL);
   TD_PARAMETER_CHECK((pBoard->nMaxAnalogCh <= nPhoneCh), nPhoneCh, IFX_NULL);

#ifndef TAPI_VERSION4
   return &pBoard->rgoPhones[nPhoneCh];

#else /* TAPI_VERSION4 */

   for (i = 0; i < pBoard->nMaxAnalogCh; i++)
   {
      if (pBoard->rgoPhones[i].nDev == nPhoneDev &&
          pBoard->rgoPhones[i].nPhoneCh == nPhoneCh)
      {
         return &pBoard->rgoPhones[i];
      }
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
      ("Board, phone for device %d phone channel %d does not exist\n",
       nPhoneDev, (int) nPhoneCh));
   return IFX_NULL;
#endif /* TAPI_VERSION4 */

} /* ABSTRACT_GetPHONE_ByPhoneCh() */


/**
   Get pointer of PHONE according to data channel.

   \param nDataCh - data channel
   \param pConn   - pointer to pointer of phone connections
   \param pBoard - pointer to board structure

   \return pointer to PHONE with this data channel or IFX_NULL if none
*/
PHONE_t* ABSTRACT_GetPHONE_OfDataCh(IFX_int32_t nDataCh,
                                    CONNECTION_t** pConn,
                                    BOARD_t* pBoard)
{
   IFX_int32_t i = 0;
   PHONE_t* phone = IFX_NULL;
   IFX_int32_t j = 0;

   /* check input parameters */
   TD_PTR_CHECK(pConn, IFX_NULL);
   TD_PTR_CHECK(pBoard, IFX_NULL);
   TD_PARAMETER_CHECK((0 > nDataCh), nDataCh, IFX_NULL);
   TD_PARAMETER_CHECK((pBoard->nMaxCoderCh <= nDataCh), nDataCh, IFX_NULL);

   for (i = 0; i < pBoard->nMaxPhones; i++)
   {
      if (0 < pBoard->rgoPhones[i].nConnCnt)
      {
         /* Connections to other phones exists and maybe this phone
            uses additional mapped data channel for connection. */
         for (j = 0; j < pBoard->rgoPhones[i].nConnCnt; j++)
         {
            if ((EXTERN_VOIP_CALL == pBoard->rgoPhones[i].rgoConn[j].nType) &&
                (nDataCh == pBoard->rgoPhones[i].rgoConn[j].nUsedCh))
            {
               phone = &pBoard->rgoPhones[i];
               *pConn = &pBoard->rgoPhones[i].rgoConn[j];
               return phone;
            }
         }
      }
      /*else*/ if (nDataCh == pBoard->rgoPhones[i].nDataCh)
      {
         /* Will use first one, free connection */
         phone = &pBoard->rgoPhones[i];
         /**pConn = &pBoard->rgoPhones[i].rgoConn[0];*/
         return phone;
      }
   }

   return phone;
} /* ABSTRACT_GetPHONE_OfDataCh() */

#if (!defined EASY336 && !defined TAPI_VERSION4)
/**
   Get pointer of PHONE according to pcm channel.

   \param pCtrl - pointer to status control structure
   \param nPCM_Ch - pcm channel
   \param pConn - pointer to list of connections
   \param pBoard - pointer to board structure

   \return pointer to PHONE with this phone channel or IFX_NULL if none
*/
PHONE_t* ABSTRACT_GetPHONE_OfPCM_Ch(CTRL_STATUS_t* pCtrl,
                                    IFX_int32_t nPCM_Ch,
                                    CONNECTION_t** pConn,
                                    BOARD_t* pBoard)
{
   IFX_int32_t i = 0;
   PHONE_t* phone = IFX_NULL;
   IFX_int32_t j = 0;

   /* check input parameters */
   TD_PTR_CHECK(pConn, IFX_NULL);
   TD_PTR_CHECK(pBoard, IFX_NULL);
   TD_PTR_CHECK(pCtrl, IFX_NULL);
   TD_PARAMETER_CHECK((0 > nPCM_Ch), nPCM_Ch, IFX_NULL);
   TD_PARAMETER_CHECK((pBoard->nMaxPCM_Ch <= nPCM_Ch), nPCM_Ch, IFX_NULL);

   for (i = 0; i < pBoard->nMaxAnalogCh; i++)
   {
      if (0 < pBoard->rgoPhones[i].nConnCnt)
      {
         /* Connections to other phones exists and maybe this phone
            uses additional mapped data channel for connection. */
         for (j = 0; j < pBoard->rgoPhones[i].nConnCnt; j++)
         {
            if ((PCM_CALL == pBoard->rgoPhones[i].rgoConn[j].nType)
                && (nPCM_Ch == pBoard->rgoPhones[i].rgoConn[j].nUsedCh))
            {
               phone = &pBoard->rgoPhones[i];
               *pConn = &pBoard->rgoPhones[i].rgoConn[j];
               return phone;
            }
         }
      }
      /*else*/
      if (nPCM_Ch == pBoard->rgoPhones[i].nPCM_Ch)
      {
         /* Will use first one, free connection */
         phone = &pBoard->rgoPhones[i];
         return phone;
      }
   }

   return phone;
} /* ABSTRACT_GetPHONE_OfPCM_Ch() */
#endif /* #if (!defined EASY336 && !defined TAPI_VERSION4) */

/**
   Get PHONE according to socket.

   \param nSocket - socket
   \param pConn   - pointer to pointer of phone connections
   \param pBoard - pointer to board structure

   \return pointer to PHONE or IFX_NULL if none
*/
PHONE_t* ABSTRACT_GetPHONE_OfSocket(IFX_int32_t nSocket,
                                    CONNECTION_t** ppConn,
                                    BOARD_t* pBoard)
{
   IFX_int32_t i = 0;
   IFX_int32_t j = 0;
#ifdef EASY3111
   IFX_int32_t k;
   PHONE_t *pPhone;
#endif /* EASY3111 */

   /* check input parameters */
   TD_PARAMETER_CHECK((NO_SOCKET == nSocket), nSocket, IFX_NULL);
   TD_PTR_CHECK(ppConn, IFX_NULL);
   TD_PTR_CHECK(pBoard, IFX_NULL);

   /* reset data */
   *ppConn = IFX_NULL;
   for (i = 0; i < pBoard->nMaxAnalogCh; i++)
   {
      if (0 < pBoard->rgoPhones[i].nConnCnt)
      {
         /* Connections to other phones exists and maybe this phone
            uses additional socket for connection. */
         for (j = 0;j < pBoard->rgoPhones[i].nConnCnt; j++)
         {
            if ((EXTERN_VOIP_CALL == pBoard->rgoPhones[i].rgoConn[j].nType) &&
                (nSocket == pBoard->rgoPhones[i].rgoConn[j].nUsedSocket))
            {
               *ppConn = &pBoard->rgoPhones[i].rgoConn[j];
               return &pBoard->rgoPhones[i];
            }
         }
      }
   }
   for (i = 0; i < pBoard->nMaxAnalogCh; i++)
   {
      if (nSocket == pBoard->rgoPhones[i].nSocket)
      {
         /* Don't have connection yet, use first one */
         *ppConn = IFX_NULL;
         return &pBoard->rgoPhones[i];
      }
   }
#ifdef EASY3111
   for (k=0; k<pBoard->pCtrl->nBoardCnt; k++)
   {
      if (TYPE_DUSLIC_XT == pBoard->pCtrl->rgoBoards[k].nType)
      {
         /* for all phones */
         for (i = 0; i < pBoard->pCtrl->rgoBoards[k].nMaxAnalogCh; i++)
         {
            pPhone = &pBoard->pCtrl->rgoBoards[k].rgoPhones[i];
            if (0 < pPhone->nConnCnt)
            {
               /* Connections to other phones exists and maybe this phone
                  uses additional socket for connection. */
               for (j = 0; j < pPhone->nConnCnt; j++)
               {
                  if ((EXTERN_VOIP_CALL == pPhone->rgoConn[j].nType) &&
                      (nSocket == pPhone->rgoConn[j].nUsedSocket))
                  {
                     *ppConn = &pPhone->rgoConn[j];
                     return pPhone;
                  }
               }
            }
         } /* for all phones on board */
      }
   }
#endif /* EASY3111 */
   /* Failed to get phone. */
   return IFX_NULL;
} /* ABSTRACT_GetPHONE_OfSocket() */

/**
   Get phone accrording to connection structure address.

   \param pCtrl - pointer to control structure
   \param pConn - pointer to connection

   \return pointer to PHONE or IFX_NULL if none
 */
PHONE_t* ABSTRACT_GetPHONE_OfConn(const CTRL_STATUS_t* pCtrl,
                                  const CONNECTION_t* pConn)
{
   IFX_int32_t nBoard, nPhone, nConn;
   PHONE_t* pPhone = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_NULL);
   TD_PTR_CHECK(pConn, IFX_NULL);

   for (nBoard=0; nBoard < pCtrl->nBoardCnt; nBoard++)
   {
      for (nPhone = 0; nPhone < pCtrl->rgoBoards[nBoard].nMaxPhones;
            nPhone++)
      {
         pPhone = &pCtrl->rgoBoards[nBoard].rgoPhones[nPhone];
         for (nConn = 0; nConn < MAX_PEERS_IN_CONF; nConn++)
         {
            if (pConn == &pPhone->rgoConn[nConn])
            {
               return pPhone;
            }
         }
      }
   }
   return IFX_NULL;
}

/**
   Get PHONE according to called number.

   \param pCtrl     - pointer to status control structure
   \param nCalledNum - called number of phone channel
   \param nSeqConnId    - Seq Conn ID here usually set to 0 (initialization)

   \return pointer to BOARD with this phone channel or IFX_NULL if none
*/
PHONE_t* ABSTRACT_GetPHONE_OfCalledNum(CTRL_STATUS_t* pCtrl,
                                       IFX_int32_t nCalledNum,
                                       IFX_uint32_t nSeqConnId)
{
   IFX_int32_t i = 0;
   BOARD_t* board = IFX_NULL;
   IFX_int32_t j = 0;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_NULL);

   for (i = 0; i < MAX_BOARDS; i++)
   {
      board = &pCtrl->rgoBoards[i];
      if (board != IFX_NULL)
      {
         for (j = 0; j < board->nMaxPhones; j++)
         {
            if (board->rgoPhones[j].nPhoneNumber == nCalledNum)
            {
               return &board->rgoPhones[j];
            }
         }
      }
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Phone with number %d does not exist\n",
          (int) nCalledNum));

   return IFX_NULL;
} /* ABSTRACT_GetPHONE_OfCalledNum() */

#ifdef FXO
/**
   Get pointer of FXO according to fxo number.

   \param nFxoNum   - FXO (duslic or teridian) number
   \param pCtrl     - pointer to board

   \return pointer to FXO with this data channel or IFX_NULL if none
*/
FXO_t* ABSTRACT_GetFxo(IFX_int32_t nFxoNum, CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t nBoard, nCh;
   TAPIDEMO_DEVICE_t* pDevice;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_NULL);
   TD_PARAMETER_CHECK((0 > nFxoNum), nFxoNum, IFX_NULL);
   TD_PARAMETER_CHECK((pCtrl->nMaxFxoNumber < nFxoNum), nFxoNum, IFX_NULL);

   /* for all boards */
   for (nBoard = 0; nBoard < pCtrl->nBoardCnt; nBoard++)
   {
      /* get first device */
      pDevice = pCtrl->rgoBoards[nBoard].pDevice;
      /* for all devices */
      while (IFX_NULL != pDevice)
      {
         /* check if FXO device */
         if (TAPIDEMO_DEV_FXO == pDevice->nType)
         {
            /* for all FXO channels on this device */
            for (nCh=0; nCh < pDevice->uDevice.stFxo.nNumOfCh; nCh++)
            {
               /* if correct number */
               if (nFxoNum == pDevice->uDevice.stFxo.rgoFXO[nCh].nFXO_Number)
               {
                  return &pDevice->uDevice.stFxo.rgoFXO[nCh];
               }
            } /* for all FXO channels on this device */
         } /* check if FXO device */
         /* get next device */
         pDevice = pDevice->pNext;
      } /* while (IFX_NULL != pDevice) */
   } /* for all boards */

   return IFX_NULL;
} /* ABSTRACT_GetFxo() */
#endif /* FXO */

/**
   Get pointer of device according to device FD.

   \param nDevFd    - device FD
   \param pCtrl     - pointer to control structure

   \return pointer to device with this FD or IFX_NULL if none
*/
TAPIDEMO_DEVICE_t* ABSTRACT_GetDeviceOfDevFd(IFX_int32_t nDevFd,
                                             CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t nBoard;
   TAPIDEMO_DEVICE_t* pDevice;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_NULL);
   TD_PARAMETER_CHECK((0 > nDevFd), nDevFd, IFX_NULL);

   /* for all boards */
   for (nBoard = 0; nBoard < pCtrl->nBoardCnt; nBoard++)
   {
      /* get first device */
      pDevice = pCtrl->rgoBoards[nBoard].pDevice;
      /* for all devices */
      while (IFX_NULL != pDevice)
      {
         /* check if FXO device */
         if (TAPIDEMO_DEV_FXO == pDevice->nType)
         {
            /* if correct device FD */
            if (nDevFd == pDevice->uDevice.stFxo.nDevFD)
            {
               /* device was found */
               return pDevice;
            }
         } /* check if FXO device */
         /* check if CPU device */
         else if (TAPIDEMO_DEV_CPU == pDevice->nType)
         {
            /* if correct device FD */
            if (nDevFd == pCtrl->rgoBoards[nBoard].nDevCtrl_FD)
            {
               /* device was found */
               return pDevice;
            }
         }
         /* get next device */
         pDevice = pDevice->pNext;
      } /* while (IFX_NULL != pDevice) */
   } /* for all boards */

   return IFX_NULL;
} /* ABSTRACT_GetFxo() */

/**
   Get connection from this phone according to call type, caller ch, called ch.

   \param pPhone  - pointer to phone (in SVIP case this is caller peer)
   \param nCallType  - type of call (also type of channels are defined data, pcm)
   \param nCalledPhoneNum  - called phone number
   \param nCallerPhoneNum  - caller phone number
   \param oCallerIPAddr    - caller ip address
   \param fFreeConn  - flag which indicates the connection is free (first time)
   \param nSeqConnId - Seq Conn ID

   \return pointer to PHONE with this data channel or IFX_NULL if none
           flag is set according if connection is free or already used
*/
CONNECTION_t* ABSTRACT_GetConnOfPhone(PHONE_t* pPhone,
                                      CALL_TYPE_t nCallType,
                                      IFX_int32_t nCalledPhoneNum,
                                      IFX_int32_t nCallerPhoneNum,
                                      TD_OS_sockAddr_t* pCallerIPAddr,
                                      IFX_boolean_t* fFreeConn,
                                      IFX_uint32_t nSeqConnId)
{
   IFX_int32_t i = 0;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_NULL);
   TD_PTR_CHECK(pCallerIPAddr, IFX_NULL);
   TD_PTR_CHECK(fFreeConn, IFX_NULL);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
      ("Phone No %d: Look for connection %s,\n"
       "%scaller no %d and called no %d\n",
       pPhone->nPhoneNumber, Common_Enum2Name(nCallType, TD_rgCallTypeName),
       pPhone->pBoard->pIndentPhone, nCallerPhoneNum, nCalledPhoneNum));

#ifdef TAPI_VERSION4
   nDevTmp = pPhone->nDev;
#endif /* TAPI_VERSION4 */

   if (pPhone->nConnCnt == 0)
   {
      /* Using first connection */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Phone No %d: Use first, free connection of phone on dev %d ch %d.\n",
          pPhone->nPhoneNumber, (int) nDevTmp, pPhone->nPhoneCh));

      *fFreeConn = IFX_TRUE;
      return &pPhone->rgoConn[0];
   } /* if (pPhone->nConnCnt == 0) */
   else
   {
      for (i = 0; i < pPhone->nConnCnt; i++)
      {
         if ((pPhone->rgoConn[i].nType == nCallType) &&
             (pPhone->rgoConn[i].oConnPeer.nPhoneNum == nCallerPhoneNum) &&
             (pPhone->nPhoneNumber == nCalledPhoneNum))
         {
            switch (nCallType)
            {
            case EXTERN_VOIP_CALL:
               {
                  if (IFX_TRUE == TD_SOCK_AddrCompare(pCallerIPAddr,
                                     &pPhone->rgoConn[i].oConnPeer.oRemote.oToAddr,
                                     IFX_FALSE))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                        ("Phone No %d: Found external voip connection num %d.\n",
                         pPhone->nPhoneNumber, (int) i));

                     *fFreeConn = IFX_FALSE;
                     return &pPhone->rgoConn[i];
                  } /* if strcmp(...)==0 */
               }
               break; /* EXTERN_VOIP_CALL */
#ifndef EASY336
            case PCM_CALL:
               {
#ifndef TAPI_VERSION4
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                     ("Phone No %d: Found PCM connection\n"
                      "%sConnID %d, PhoneCh %d.\n",
                      pPhone->nPhoneNumber,
                      pPhone->pBoard->pIndentPhone,
                      (int) i, (int) pPhone->nPhoneCh));
#endif /* TAPI_VERSION4 */
                  *fFreeConn = IFX_FALSE;
                  return &pPhone->rgoConn[i];
               }
               break; /* PCM_CALL */
#endif /* EASY336 */
            default:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Phone No %d: Unknown call type %d.\n"
                   "%s(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, nCallType,
                   pPhone->pBoard->pIndentPhone, __FILE__, __LINE__));
               break;
            } /* switch */
         } /* if (pPhone->rgoConn[i].nType == nCallType) && ... */
      } /* for */
   } /* if (pPhone->nConnCnt == 0) */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Phone No %d: Use last free connection on dev %d ch %d.\n",
             pPhone->nPhoneNumber, pPhone->nDev, (int) pPhone->nPhoneCh));

   *fFreeConn = IFX_TRUE;
   /* Connection was not found so use last, free one */
   return &pPhone->rgoConn[pPhone->nConnCnt];
} /* ABSTRACT_GetConnOfPhone() */

#if (!defined EASY336 && !defined TAPI_VERSION4)
/**
   Get connection and phone according to data channel.

   \param pPhone - pointer to phone
   \param nDataCh - data channel
   \param nCallType - type of call

   \return IFX_SUCCESS if phone and connection found, if else return IFX_ERROR
*/
IFX_return_t ABSTRACT_GetConnOfDataCh(PHONE_t** ppPhone, CONNECTION_t** ppConn,
                                      BOARD_t* pBoard, IFX_int32_t nDataCh)
{
   IFX_int32_t i, j;
#ifdef EASY3111
   IFX_int32_t k;
#endif /* EASY3111 */

   /* check input parameters */
   TD_PTR_CHECK(ppPhone, IFX_ERROR);
   TD_PTR_CHECK(ppConn, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nDataCh), nDataCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxCoderCh <= nDataCh), nDataCh, IFX_ERROR);

   /* reset data */
   *ppConn = IFX_NULL;
   *ppPhone = IFX_NULL;
   /* for all phones */
   for (i = 0; i < pBoard->nMaxAnalogCh; i++)
   {
      /* set phone */
      *ppPhone = &pBoard->rgoPhones[i];
      /* for all connections */
      for (j = 0; j < (*ppPhone)->nConnCnt; j++)
      {
         /* get connection */
         *ppConn = &(*ppPhone)->rgoConn[j];
         /* check call type */
         if (EXTERN_VOIP_CALL == (*ppConn)->nType ||
             LOCAL_BOARD_CALL == (*ppConn)->nType)
         {
            /* check used data channel */
            if (nDataCh == (*ppConn)->nUsedCh)
            {
               /* connection and phone that use data channel number nDataCh
                  are found */
               return IFX_SUCCESS;
            }
         } /* check call type */
      } /* for all phone connections */
   } /* for all phones on board */
#ifdef EASY3111
   for (k=0; k<pBoard->pCtrl->nBoardCnt; k++)
   {
      if (TYPE_DUSLIC_XT == pBoard->pCtrl->rgoBoards[k].nType)
      {
         /* for all phones */
         for (i = 0; i < pBoard->pCtrl->rgoBoards[k].nMaxAnalogCh; i++)
         {
            /* set phone */
            *ppPhone = &pBoard->pCtrl->rgoBoards[k].rgoPhones[i];
            /* for all connections */
            for (j = 0; j < (*ppPhone)->nConnCnt; j++)
            {
               /* get connection */
               *ppConn = &(*ppPhone)->rgoConn[j];
               /* check call type */
               if (EXTERN_VOIP_CALL == (*ppConn)->nType)
               {
                  /* check used data channel */
                  if (nDataCh == (*ppConn)->nUsedCh)
                  {
                     /* connection and phone that use data channel
                        number nDataCh are found */
                     return IFX_SUCCESS;
                  }
               } /* check call type */
            } /* for all phone connections */
         } /* for all phones on board */
      }
   }
#endif /* EASY3111 */
   /* failed to get connection */
   return IFX_ERROR;
} /* ABSTRACT_GetConnOfDataCh() */
#endif /* (!defined EASY336 && !defined TAPI_VERSION4) */

/**
   Get connection of local called phone.

   \param pPhone   - pointer to PHONE
   \param nPhoneCh - called phone channel
   \param nType    - expected call type

   \return phone connection, IFX_NULL on error.
*/
CONNECTION_t* ABSTRACT_GetConnOfLocal(PHONE_t* pPhone, PHONE_t* pCalledPhone,
                                      CALL_TYPE_t nType)
{
   IFX_int32_t i = 0;

   /* check input parameters */
   TD_PTR_CHECK(pPhone, IFX_NULL);

   if (0 == pPhone->nConnCnt)
   {
      /* Don't have any connection, return first one. Mostly executes on slave,
         called phones. */
      return &pPhone->rgoConn[0];
   }

   for (i = 0; i < pPhone->nConnCnt; i++)
   {
      /* This loop executes only on the master, caller phone. Called phones
         have only one connection. */
      /* Check if connection has expected type */
      if (nType == pPhone->rgoConn[i].nType)
      {
         /* Search for this connection, if exists. */
         if (pPhone->rgoConn[i].oConnPeer.oLocal.pPhone == pCalledPhone)
         {
            return &pPhone->rgoConn[i];
         }
      } /* if (nType == pPhone->rgoConn[i].nType) */
   } /* for */

   /* This connection does not exists, return last free one. */
   return &pPhone->rgoConn[pPhone->nConnCnt];
} /* ABSTRACT_GetConnOfLocal() */

/* TODO: Add AR9
   Fix list in drv */
/**
   Get BOARD according to passed board type.

   \param pCtrl - pointer to status control structure
   \param devType - board type

   \return pointer to BOARD or IFX_NULL if none
*/
BOARD_t* ABSTRACT_GetBoard(CTRL_STATUS_t* pCtrl, IFX_TAPI_DEV_TYPE_t devType)
{
   IFX_int32_t i;
   BOARD_TYPE_t boardType = TYPE_NONE;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_NULL);

   switch (devType)
   {
#ifdef TAPI_VERSION4
      case IFX_TAPI_DEV_TYPE_NONE:
         boardType = TYPE_NONE;
         break;
#endif /* TAPI_VERSION4 */
      case IFX_TAPI_DEV_TYPE_DUSLIC_S:
         boardType = TYPE_DUSLIC;
         break;
      case IFX_TAPI_DEV_TYPE_DUSLIC_XT:
         boardType = TYPE_DUSLIC_XT;
         break;
      case IFX_TAPI_DEV_TYPE_VINETIC:
         boardType = TYPE_VINETIC;
         break;
      case IFX_TAPI_DEV_TYPE_INCA_IPP:
         boardType = TYPE_DANUBE;
         break;
      case IFX_TAPI_DEV_TYPE_VINETIC_CPE:
         boardType = TYPE_VINETIC;
         break;
#ifdef TAPI_VERSION4
      case IFX_TAPI_DEV_TYPE_VOICE_MACRO:
         boardType = TYPE_NONE;
         break;
#endif /* TAPI_VERSION4 */
      case IFX_TAPI_DEV_TYPE_VINETIC_S:
         boardType = TYPE_VINETIC;
         break;
#ifdef TAPI_VERSION4
      case IFX_TAPI_DEV_TYPE_VOICESUB_GW:
         boardType = TYPE_NONE;
#endif /* TAPI_VERSION4 */
         break;
      case IFX_TAPI_DEV_TYPE_VIN_SUPERVIP:
         boardType = TYPE_SVIP;
         break;
#ifdef TAPI_VERSION4
      case IFX_TAPI_DEV_TYPE_VIN_XT16:
         boardType = TYPE_XT16;
         break;
#endif /* TAPI_VERSION4 */
      case IFX_TAPI_DEV_TYPE_DUSLIC:
         boardType = TYPE_DUSLIC;
         break;
      default:
         boardType = TYPE_NONE;
   }

   for (i = 0; i < pCtrl->nBoardCnt; i++)
      if (pCtrl->rgoBoards[i].nType == boardType)
         return &pCtrl->rgoBoards[i];

   return IFX_NULL;
}

#ifndef TAPI_VERSION4
IFX_int32_t ABSTRACT_GetDataChannelOfFD(IFX_int32_t nFd, BOARD_t* pBoard)
{
   IFX_int32_t nCh;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((nFd < 0), nFd, IFX_ERROR);

   /* for all data channels */
   for (nCh=0; nCh < pBoard->nMaxCoderCh; nCh++)
   {
      if (nFd == pBoard->nCh_FD[nCh])
      {
         return nCh;
      }
   }
   return IFX_ERROR;
}
#endif

/**
   Initialize sequential conn id count. IPv4 address must be set first.

   \param pCtrl   - pointer to status control structure

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
IFX_return_t ABSTRACT_SeqConnID_Init(CTRL_STATUS_t* pCtrl)
{
   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->pProgramArg, IFX_ERROR);

   pCtrl->nSeqConnIdCnt = TD_SOCK_AddrIPv4Get(&pCtrl->pProgramArg->oMy_IP_Addr);
   pCtrl->nSeqConnIdCnt &= TD_IP_LOWEST_BYTE_MASK;
   if (0 == pCtrl->nSeqConnIdCnt)
   {
      pCtrl->nSeqConnIdCnt = 255;
   }
   g_nID = (IFX_uint8_t) pCtrl->nSeqConnIdCnt;
   pCtrl->nSeqConnIdCnt <<= 24;

   return IFX_SUCCESS;
}

/**
   Get new sequential conn id for new connection.

   \param pPhone  - pointer to phone structure
   \param pFXO    - pointer to fxo
   \param pCtrl   - pointer to status control structure

   \return sequential connection id number
*/
IFX_uint32_t ABSTRACT_SeqConnID_Get(PHONE_t *pPhone, FXO_t* pFXO,
                                    CTRL_STATUS_t* pCtrl)
{
   pCtrl->nSeqConnIdCnt++;
   /* assume tha input parameters are ok */
   if (IFX_NULL != pPhone)
   {
      pPhone->nSeqConnId = pCtrl->nSeqConnIdCnt;

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: Set new Conn ID %08X\n",
                pPhone->nPhoneNumber, pPhone->nSeqConnId));
   }
   if (IFX_NULL != pFXO)
   {
      pFXO->nSeqConnId = ++(pCtrl->nSeqConnIdCnt);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
               ("FXO No %d: Set new Conn ID %08X\n",
                pFXO->nFXO_Number, pFXO->nSeqConnId));
   }
   return pCtrl->nSeqConnIdCnt;
}
/**
   Set sequential conn id for connection.

   \param pPhone  - pointer to phone structure
   \param pFXO    - pointer to fxo
   \param nSeqConnId    - Seq Conn ID

   \return sequential connection id number
*/
IFX_uint32_t ABSTRACT_SeqConnID_Set(PHONE_t *pPhone, FXO_t* pFXO,
                                    IFX_uint32_t nSeqConnId)
{
   /* assume tha input parameters are ok */
   if (IFX_NULL != pPhone)
   {
      pPhone->nSeqConnId = nSeqConnId;

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: Set Conn ID %08X\n",
                pPhone->nPhoneNumber, pPhone->nSeqConnId));

      return pPhone->nSeqConnId;
   }
   if (IFX_NULL != pFXO)
   {
      pFXO->nSeqConnId = nSeqConnId;

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
               ("FXO No %d: Set Conn ID %08X\n",
                pFXO->nFXO_Number, pFXO->nSeqConnId));

      return pFXO->nSeqConnId;
   }
   return IFX_ERROR;
}

/**
   Reset sequential conn id for connection.

   \param pPhone  - pointer to phone structure

   \return sequential connection id number
*/
IFX_int32_t ABSTRACT_SeqConnID_Count(PHONE_t * const pPhone,
                                     FXO_t * const pFXO)
{
   CTRL_STATUS_t *pCtrl = IFX_NULL;
   BOARD_t *pBoard = IFX_NULL;
   IFX_int32_t nConnCnt = 0;
   IFX_int32_t nBoardCnt, nPhoneCnt;
   IFX_uint32_t nSeqConnId = TD_CONN_ID_NOT_SET;
#ifdef FXO
   TAPIDEMO_DEVICE_t* pDevice;
   IFX_int32_t nFxoCh;
#endif /* FXO */

   /* get control structure from phone */
   if (IFX_NULL != pPhone &&
       IFX_NULL != pPhone->pBoard)
   {
      pCtrl = pPhone->pBoard->pCtrl;
      nSeqConnId = pPhone->nSeqConnId;
   } /* get control structure from phone */
   /* get control structure from fxo */
   if (IFX_NULL != pFXO &&
       IFX_NULL != pFXO->pFxoDevice &&
       IFX_NULL != pFXO->pFxoDevice->pBoard)
   {
      pCtrl = pFXO->pFxoDevice->pBoard->pCtrl;
      nSeqConnId = pFXO->nSeqConnId;
   } /* get control structure from fxo */
   /* must have ctrl struct and conn id set */
   if (IFX_NULL == pCtrl || TD_CONN_ID_NOT_SET == nSeqConnId) {
      return 0;
   }

   /* for all boards */
   for (nBoardCnt=0; nBoardCnt < pCtrl->nBoardCnt; nBoardCnt++)
   {
      pBoard = &pCtrl->rgoBoards[nBoardCnt];
      /* for all phones */
      for (nPhoneCnt=0; nPhoneCnt < pBoard->nMaxPhones; nPhoneCnt++)
      {
         /* check for conn id occurence */
         if (nSeqConnId == pBoard->rgoPhones[nPhoneCnt].nSeqConnId)
         {
            nConnCnt++;
         }
      } /* for all phones */
#ifdef FXO
      /* get first device */
      pDevice = pCtrl->rgoBoards[nBoardCnt].pDevice;
      /* untill last device is reached */
      while (IFX_NULL != pDevice)
      {
         /* if FXO device */
         if (TAPIDEMO_DEV_FXO == pDevice->nType)
         {
            /* for all FXO channels */
            for (nFxoCh=0; nFxoCh < pDevice->uDevice.stFxo.nNumOfCh; nFxoCh++)
            {
               /* check for conn id occurence */
               if (nSeqConnId == pDevice->uDevice.stFxo.rgoFXO[nFxoCh].nSeqConnId)
               {
                  nConnCnt++;
               }
            } /* for all FXO channels */
         } /* if FXO device */
         pDevice = pDevice->pNext;
      } /* while (IFX_NULL != pDevice) */
#endif /* FXO */
   } /* for all boards */

   return nConnCnt;
}

/**
   Reset sequential conn id for connection.

   \param pPhone  - pointer to phone structure

   \return sequential connection id number
*/
IFX_return_t ABSTRACT_SeqConnID_Reset(PHONE_t *pPhone, FXO_t* pFXO)
{
   IFX_char_t *pLastConnId = "";

   /* check for number of conn id occurence */
   if (1 == ABSTRACT_SeqConnID_Count(pPhone, pFXO))
   {
      pLastConnId = "- last occurence";
   }

   /* assume tha input parameters are ok */
   if (IFX_NULL != pPhone)
   {
      if (TD_NOT_SET != pPhone->nSeqConnId)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Release Conn ID %08X %s\n",
             pPhone->nPhoneNumber, pPhone->nSeqConnId, pLastConnId));
         pPhone->nSeqConnId = TD_CONN_ID_INIT;
      }
   }
   if (IFX_NULL != pFXO)
   {
      if (TD_NOT_SET != pFXO->nSeqConnId)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
            ("FXO No %d: Release Conn ID %08X %s\n",
             pFXO->nFXO_Number, pFXO->nSeqConnId, pLastConnId));
         pFXO->nSeqConnId = TD_CONN_ID_INIT;
      }
   }

   return IFX_SUCCESS;
}

/**
   Add prefix to buffer.

   \param pBuf    - pointer string
   \param nConnId - conn id
   \param pSize   - pointer to actual size of string

   \return number of chars that were added
*/
IFX_int32_t ABSTRACT_PrefixAdd(IFX_char_t *pBuf, IFX_uint32_t nConnId,
                               IFX_uint32_t *pSize)
{
   *pSize += TD_PREFIX_LENGTH;
   /* validate string length */
   if (*pSize >= (TD_COM_BUF_SIZE - 1))
   {
      TD_PRINTF("warning: trace too long - unable to add prefix "
                "(File: %s, line: %d):\n",
                __FILE__, __LINE__);
      return 0;
   }
   switch (nConnId)
   {
   case TD_CONN_ID_INIT:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X init  ", g_nID);
      break;
   case TD_CONN_ID_EVT:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X event ", g_nID);
      break;
   case TD_CONN_ID_ERR:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X error ", g_nID);
      break;
   case TD_CONN_ID_ITM:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X itm   ", g_nID);
      break;
   case TD_CONN_ID_T38_TEST:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X t.38  ", g_nID);
      break;
   case TD_CONN_ID_PHONE_BOOK:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X pbook ", g_nID);
      break;
   case TD_CONN_ID_MSG:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X msg   ", g_nID);
      break;
   case TD_CONN_ID_DBG:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X debug ", g_nID);
      break;
   case TD_CONN_ID_HELP:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X help  ", g_nID);
      break;
   case TD_CONN_ID_NOT_SET:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X       ", g_nID);
      break;
   case TD_CONN_ID_DECT:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %02X dect  ", g_nID);
      break;
   default:
      snprintf(pBuf, TD_COM_BUF_SIZE, TD_PREFIX_NAME" %08X ", nConnId);
      break;
   }
   return TD_PREFIX_LENGTH;
}

/**
 * Copy string from g_buf to g_buf_parsed and Add prefix for each new line
 * of traces.
 * 
 * \param nConnId    - sequential connection ID
 * 
 * \return IFX_SUCCESS
 */
IFX_return_t ABSTRACT_ParseTrace(IFX_uint32_t nConnId)
{
   static IFX_boolean_t bPrefixPrinted = IFX_FALSE;
   IFX_char_t *pIn = g_buf, *pOut = g_buf_parsed;
   IFX_uint32_t nSize = 0;

   if (IFX_TRUE == g_bAddPrefix) {
      if (IFX_FALSE == bPrefixPrinted)
      {
         pOut += ABSTRACT_PrefixAdd(pOut, nConnId, &nSize);
         bPrefixPrinted = IFX_TRUE;
      }
   }

   do
   {
      /* validate length of string */
      if (nSize >= (TD_COM_BUF_SIZE - 1))
      {
         TD_PRINTF("warning: trace too long (File: %s, line: %d):\n",
                   __FILE__, __LINE__);
         break;
      }
      *pOut = *pIn;
      if ('\0' == *pIn)
      {
         break;
      }
      nSize++;
      pOut++;
      if (IFX_TRUE == g_bAddPrefix) {
         if ('\n' == *pIn)
         {
            if ('\0' != *(pIn + 1))
            {
               pOut += ABSTRACT_PrefixAdd(pOut, nConnId, &nSize);
               bPrefixPrinted = IFX_TRUE;
            }
            else
            {
               bPrefixPrinted = IFX_FALSE;
            }
         }
      }
      pIn++;
   } while ( 1 );

   return IFX_SUCCESS;
}



