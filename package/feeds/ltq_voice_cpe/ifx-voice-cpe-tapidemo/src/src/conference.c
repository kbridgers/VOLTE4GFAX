/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file conference.c
   \date 2005-12-01
   \brief Conferencing support for application.

   Implements methods to add new peer to existing connection by mapping
   new channels with used one.
   Those channels must be unmapped when conference ends.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "ifx_types.h"
#include "tapidemo.h"
#include "conference.h"
#include "pcm.h"
#include "voip.h"
#include "analog.h"
#include "qos.h"
#include "state_trans.h"
#include "abstract.h"
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#ifdef EASY336
#include "board_easy336.h"
#include "lib_svip.h"
#endif

#ifdef EASY3111
   #include "board_easy3111.h"
#endif /* EASY3111 */

#ifdef FXO
   #include "common_fxo.h"
   #include "event_handling.h"
#endif /* FXO */

#ifdef TD_DECT
   #include "td_dect.h"
#endif /* TD_DECT */

/* ============================= */
/* Local structures              */
/* ============================= */


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

/**
   Map local phone with PCM channel.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to connection
   \param fMap    - map/unmap flag

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_MapLocalPCM(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn,
                                    TD_MAP_ADD_REMOVE_t fMap)
{
   IFX_return_t nRet = IFX_SUCCESS;
#if (!defined(EASY3201) && !defined(EASY3201_EVS) && !defined(EASY336))
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      nRet = TD_DECT_MapToPCM(pPhone->nDectCh, pConn->nUsedCh, fMap,
                TD_GET_MAIN_BOARD(pCtrl), pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(nRet, "TD_DECT_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* Unmap previous called data ch
         from phone channel that is leaving conference. */
      nRet = PCM_MapToPCM((TD_MAP_ADD == fMap ?
                           pPhone->oEasy3111Specific.nOnMainPCM_Ch :
                           pPhone->oEasy3111Specific.nOnMainPCM_Ch_Bak),
                          pConn->nUsedCh,
                          (TD_MAP_ADD == fMap ? IFX_TRUE : IFX_FALSE),
                          TD_GET_MAIN_BOARD(pCtrl), pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(nRet, "PCM_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* EASY3111 */
   {
      /* Unmap previous called data ch
         from phone channel that is leaving conference. */
      nRet = PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh,
                (TD_MAP_ADD == fMap ? IFX_TRUE : IFX_FALSE),
                pPhone->pBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(nRet, "PCM_MapToPhone", pPhone->nSeqConnId);
   }
#endif /* (!defined(EASY3201) && !defined(EASY336)) */
   return nRet;
}

/**
   Map local phone with PCM channel.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to connection
   \param fMap    - map/unmap flag

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_MapLocalALM(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn,
                                    TD_MAP_ADD_REMOVE_t fMap)
{
   IFX_return_t nRet = IFX_SUCCESS;
#if (!defined(EASY3201) && !defined(EASY3201_EVS) && !defined(EASY336))
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      nRet = TD_DECT_MapToAnalog(pPhone->nDectCh, pConn->nUsedCh, fMap,
                                 TD_GET_MAIN_BOARD(pCtrl), pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(nRet, "TD_DECT_MapToAnalog", pPhone->nSeqConnId);
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* Unmap previous called data ch
         from phone channel that is leaving conference. */
      nRet = PCM_MapToPhone((TD_MAP_ADD == fMap ?
                             pPhone->oEasy3111Specific.nOnMainPCM_Ch :
                             pPhone->oEasy3111Specific.nOnMainPCM_Ch_Bak),
                            pConn->nUsedCh,
                            (TD_MAP_ADD == fMap ? IFX_TRUE : IFX_FALSE),
                            TD_GET_MAIN_BOARD(pCtrl), pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(nRet, "PCM_MapToPhone", pPhone->nSeqConnId);
   }
   else
#endif /* EASY3111 */
   {
      /* Unmap previous called data ch
         from phone channel that is leaving conference. */
      nRet = ALM_MapToPhone(pPhone->nPhoneCh, pConn->nUsedCh,
                            fMap, pPhone->pBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(nRet, "PCM_MapToPhone", pPhone->nSeqConnId);
   }
#endif /* (!defined(EASY3201) && !defined(EASY336)) */
   return nRet;
}
/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   Put phone on hold, because call to another phone will be made.

   \todo Implement it.

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PutOnHold(IFX_void_t)
{
   return IFX_SUCCESS;
} /* CONFERENCE_PutOnHold() */

/**
   Resume phone, which is on hold.

   \todo Implement it.

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_ResumeOnHold(IFX_void_t)
{
   return IFX_SUCCESS;
} /* CONFERENCE_ResumeOnHold() */


/**
   Configure LEC and turn it on or off for analog phone or for PCM.

   \todo Implement it.

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t CONFERENCE_Set_LEC(IFX_void_t)
{
   return IFX_SUCCESS;
} /* CONFERENCE_Set_LEC() */


/**
   Search for first free conference and return index of it.

   \param pCtrl  - handle to connection control structure
   \param nSeqConnId  - Seq Conn ID

   \return conference index if free found, otherwise NO_CONFERENCE

*/
IFX_int32_t CONFERENCE_GetIdx(CTRL_STATUS_t* pCtrl, IFX_uint32_t nSeqConnId)
{
   IFX_int32_t i = 0;
   IFX_int32_t nConfIdx = NO_CONFERENCE;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, NO_CONFERENCE);

   for (i = 0; i < MAX_CONFERENCES; i++)
   {
      if (IFX_FALSE == pCtrl->rgoConferences[i].fActive)
      {
         nConfIdx = i + 1;
         break;
      }
   }
   if (NO_CONFERENCE == nConfIdx)
   {
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("Err, Reached maximum num of conferences that are possible, "
            "MAX=%d. (File: %s, line: %d)\n",
            MAX_CONFERENCES, __FILE__, __LINE__));
   }
   else
   {
      pCtrl->rgoConferences[nConfIdx - 1].fActive = IFX_TRUE;
      pCtrl->nConfCnt++;
   }

   return nConfIdx;
} /* CONFERENCE_GetIdx() */


/**
   Release chosen conference index and clear CONFERENCE_t structure content.

   \param pConfIdx - pointer to conference index of phone
   \param pCtrl  - handle to connection control structure
   \param nSeqConnId  - Seq Conn ID

   \return IFX_ERROR if an failed, otherwise IFX_SUCCESS

*/
IFX_return_t CONFERENCE_ReleaseIdx(IFX_uint32_t* pConfIdx, CTRL_STATUS_t* pCtrl,
                                   IFX_uint32_t nSeqConnId)
{

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PARAMETER_CHECK((NO_CONFERENCE >= *pConfIdx), *pConfIdx, IFX_ERROR);
   TD_PARAMETER_CHECK((MAX_CONFERENCES < *pConfIdx), *pConfIdx, IFX_ERROR);

   /* check if conference is in use */
   if (IFX_FALSE == pCtrl->rgoConferences[*pConfIdx - 1].fActive)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("Err, Conference with Idx %d already released. "
            "(File: %s, line: %d)\n",
            *pConfIdx, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* release conference Idx it's, also done by memset */
   pCtrl->rgoConferences[*pConfIdx - 1].fActive = IFX_FALSE;
   /* Clear conference content. */
   memset(&pCtrl->rgoConferences[*pConfIdx - 1], 0, sizeof(CONFERENCE_t));
   /* set conference index to NO_CONFERENCE */
   *pConfIdx = NO_CONFERENCE;
   /* one less conference is used */
   pCtrl->nConfCnt--;

   return IFX_SUCCESS;
} /* CONFERENCE_ReleaseIdx() */


/**
   Start conference, put this PHONE into new state.

   \param pCtrl  - handle to connection control structure
   \param pPhone - pointer to PHONE

   \return IFX_ERROR if an failed, otherwise IFX_SUCCESS

   \todo when on hold will be implemented, new button, for example * and then
         all can talk and listen.
*/
IFX_return_t CONFERENCE_Start(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone)
{
   IFX_return_t ret = IFX_ERROR;
   CONFERENCE_t* pConf;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

   /* Can start conference only if already in connection with one
      party and we started call andwe are not in conference yet. */
   if ((1 == pPhone->nConnCnt) &&
       (IFX_TRUE == pPhone->rgoConn[0].fActive) &&
       (CALL_DIRECTION_TX == pPhone->nCallDirection))
   {
      if (NO_CONFERENCE == pPhone->nConfIdx)
      {
         /* We start conference, because more than two phones
            are in connection. */
         pPhone->nConfIdx = CONFERENCE_GetIdx(pCtrl, pPhone->nSeqConnId);
         if (NO_CONFERENCE == pPhone->nConfIdx)
         {
            /* All conferences are occupied. */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("All conferences occupied.\n"));
            return IFX_ERROR;
         }
         /* check if first conference isn't started already */
         if (IFX_TRUE != pPhone->nConfStarter)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
                  ("Phone No %d: Conference started\n",
                   pPhone->nPhoneNumber));
            pPhone->nConfStarter = IFX_TRUE;
            pConf = &pCtrl->rgoConferences[pPhone->nConfStarter - 1];
            /* check type of first call and add first connection type
               to conference structure */
            if (EXTERN_VOIP_CALL == pPhone->rgoConn[0].nType)
            {
               if (NO_EXTERNAL_PEER == pConf->nExternalPeersCnt)
               {
                  pConf->nExternalPeersCnt++;
               }
            }
            else if (LOCAL_CALL == pPhone->rgoConn[0].nType)
            {
               /* no action */
            }
            else if (PCM_CALL == pPhone->rgoConn[0].nType)
            {
               if (NO_PCM_PEER == pConf->nPCM_PeersCnt)
               {
                  pConf->nPCM_PeersCnt++;
               }
            }
#ifdef FXO
            else if (FXO_CALL == pPhone->rgoConn[0].nType)
            {
               if (IFX_NULL == pPhone->pFXO)
               {
                  /* Wrong input arguments */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err, Phone No %d: FXO pointer not set for FXO call. "
                         "(File: %s, line: %d)\n",
                         pPhone->nPhoneNumber, __FILE__, __LINE__));
               }
               /* for FXOs that use PCM increase PCM call number */
               else if (FXO_TYPE_SLIC121 != pPhone->pFXO->pFxoDevice->nFxoType)
               {
                  if (NO_PCM_PEER == pConf->nPCM_PeersCnt)
                  {
                     pConf->nPCM_PeersCnt++;
                  }
               }
            }
#endif /* FXO */
            /* conference started successfully */
            ret = IFX_SUCCESS;
         } /* if (IFX_TRUE != pPhone->nConfStarter) */
         else
         {
            /* Wrong input arguments */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: Conference allready started for this phone. "
                   "(File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
            CONFERENCE_ReleaseIdx(&pPhone->nConfIdx, pCtrl, pPhone->nSeqConnId);
            ret = IFX_ERROR;
         } /* if (IFX_TRUE != pPhone->nConfStarter) */

      } /* if (NO_CONFERENCE == pPhone->nConfIdx) */
      else
      {
         /* Wrong input arguments */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Conference allready started. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         ret = IFX_ERROR;
      } /* if (NO_CONFERENCE == pPhone->nConfIdx) */
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: UNABLE to start conference "
             "No connection to the phone exists OR we are not the initiator.\n",
             pPhone->nPhoneNumber));
      ret = IFX_ERROR;
   } /* only one active connection started where phone is caller/initiator */

   return ret;
} /* CONFERENCE_Start() */


/**
   Add local phone to conference.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pDstPhone - pointer to called PHONE (local phone which
                   will be added to conference, connection )
   \param pConn - pointer to local connection

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR

   \remark local calls can only be made on same board
*/
IFX_return_t CONFERENCE_PeerLocal_Add(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                      PHONE_t* pDstPhone, CONNECTION_t* pConn)
{
   CONFERENCE_t* pConf = IFX_NULL;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pDstPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Get conference if exists */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

   if ((IFX_NULL != pConf) && (IFX_TRUE == pPhone->nConfStarter))
   {
      /* If we have conference and we are the initiator */
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Adding local Phone No %d to conference\n",
             pPhone->nPhoneNumber, pDstPhone->nPhoneNumber));
#ifdef EASY3111
      /* to do mapping resources must be allocated */
      if (TYPE_DUSLIC_XT == pDstPhone->pBoard->nType)
      {
         /* set call type */
         pDstPhone->rgoConn[0].nType = LOCAL_CALL;
         if (IFX_ERROR == Easy3111_CallResourcesGet(pDstPhone, pCtrl,
                                                    &pDstPhone->rgoConn[0]))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Failed to get resources for Phone No %d."
                   "(File: %s, line: %d)\n",
                   pDstPhone->nPhoneNumber, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
#endif /* EASY3111 */
      /* do mapping for all connections */
      for (i = 0; i < pPhone->nConnCnt - 1; i++)
      {
         pOldConn = &pPhone->rgoConn[i];
         switch (pOldConn->nType)
         {
            case LOCAL_CALL:
#ifndef EASY336
               /* Map previous called phone ch with new called phone ch. */
               ret = Common_MapLocalPhones(pOldConn->oConnPeer.oLocal.pPhone,
                                           pDstPhone, TD_MAP_ADD);
               TD_PRINT_ON_FUNC_ERR(ret, "Common_MapLocalPhones",
                                    pPhone->nSeqConnId);

#endif /* EASY336 */
               break;
            case EXTERN_VOIP_CALL:
               /* Because of external call also map phone channel to
                  all data channel(s) that are participating in conference on
                  this board. */
#ifndef EASY336
#ifndef TAPI_VERSION4
#ifdef TD_DECT
               if (PHONE_TYPE_DECT == pDstPhone->nType)
               {
                  ret = TD_DECT_MapToData(pDstPhone->nDectCh,
                           pOldConn->nUsedCh, TD_MAP_ADD,
                           TD_GET_MAIN_BOARD(pCtrl), pDstPhone->nSeqConnId);
                  TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToData",
                                       pPhone->nSeqConnId);
               }
               else
#endif /* TD_DECT */
#ifdef EASY3111
               if (TYPE_DUSLIC_XT == pDstPhone->pBoard->nType)
               {
                  ret = PCM_MapToData(pDstPhone->oEasy3111Specific.nOnMainPCM_Ch,
                           pOldConn->nUsedCh, IFX_TRUE,
                           TD_GET_MAIN_BOARD(pCtrl), pPhone->nSeqConnId);
                  TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData",
                                       pPhone->nSeqConnId);
               }
               else
#endif /* EASY3111 */
               {
                  ret = VOIP_MapPhoneToData(pOldConn->nUsedCh, pDstPhone->nDev,
                           pDstPhone->nPhoneCh, IFX_TRUE,
                           pDstPhone->pBoard, pPhone->nSeqConnId);
                  TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData",
                                       pPhone->nSeqConnId);
               }
#endif /* TAPI_VERSION4 */
#endif /* EASY336 */
               break;
            case PCM_CALL:
               ret = CONFERENCE_MapLocalPCM(pCtrl, pDstPhone, pOldConn,
                                            TD_MAP_ADD);
               TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                    pPhone->nSeqConnId);
               break;
#ifdef FXO
            case FXO_CALL:
               TD_PTR_CHECK_IN_CASE(pPhone->pFXO, pPhone->nSeqConnId);
               if (FXO_TYPE_SLIC121 == pPhone->pFXO->pFxoDevice->nFxoType)
               {
                  ret = CONFERENCE_MapLocalALM(pCtrl, pDstPhone, pOldConn,
                                               TD_MAP_ADD);
                  TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalALM",
                                       pPhone->nSeqConnId);
               }
               else
               {
                  ret = CONFERENCE_MapLocalPCM(pCtrl, pDstPhone, pOldConn,
                                               TD_MAP_ADD);
                  TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                       pPhone->nSeqConnId);
               }
               break;
#endif /* FXO */
            default:
               /* Unknown call, do nothing */
            break;
         } /* switch */
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Failed to add Phone No %d to conn No %d type %s."
                   "(File: %s, line: %d)\n",
                   pDstPhone->nPhoneNumber, i,
                   Common_Enum2Name(pOldConn->nType, TD_rgCallTypeName),
                   __FILE__, __LINE__));
         }
      } /* for */

   } /* if */

   return ret;
} /* CONFERENCE_PeerLocal_Add() */

/**
   Remove local phone from conference.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pUsedBoard - pointer to BOARD
   \param pConn - pointer to local connection that will be removed

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR

   \remark local calls can only be made on same board
*/
IFX_return_t CONFERENCE_PeerLocal_Remove(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                         BOARD_t* pUsedBoard,
                                         CONNECTION_t* pConn)
{
   CONFERENCE_t* pConf = IFX_NULL;
   IFX_return_t ret = IFX_SUCCESS;
#ifndef EASY336
   IFX_int32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;
#endif
#if (defined(TAPI_VERSION4) && defined(EASY336))
   RESOURCES_t resources;
#endif
   PHONE_t* pLocalPhone;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pUsedBoard, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Get conference if exists */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

   pLocalPhone = pConn->oConnPeer.oLocal.pPhone;
   if (IFX_TRUE == pConn->fActive)
   {
      /* print call progress trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Disconnected %s with PhoneNo %d, PhoneCh %d\n",
             pPhone->nPhoneNumber,
             Common_Enum2Name(pConn->nType, TD_rgCallTypeName),
             pConn->oConnPeer.nPhoneNum, pLocalPhone->nPhoneCh));
   }
   /* LOCAL */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
        ("Phone No %d: Removing local party from conference, "
         "called phone ch %d\n",
         pPhone->nPhoneNumber, (IFX_int32_t) pLocalPhone->nPhoneCh));
#ifndef EASY336
   /* Unmap called phone ch with conference starter phone channel. */
   ret = Common_MapLocalPhones(pPhone, pLocalPhone, TD_MAP_REMOVE);
   TD_PRINT_ON_FUNC_ERR(ret, "Common_MapLocalPhones", pPhone->nSeqConnId);

   /* do unmapping for all existing connections */
   for (i=0; i<pPhone->nConnCnt; i++)
   {
      /* get connection */
      pOldConn = &pPhone->rgoConn[i];
      /* for connection that is now removed take no action */
      if (pConn == pOldConn)
      {
         /* skip this one */
         continue;
      }
      /* check connection type */
      switch(pOldConn->nType)
      {
         case LOCAL_CALL:
         {
            /* Unmap previous called phone ch
               from phone channel that is leaving conference. */
            ret = Common_MapLocalPhones(pOldConn->oConnPeer.oLocal.pPhone,
                                        pLocalPhone, TD_MAP_REMOVE);
            TD_PRINT_ON_FUNC_ERR(ret, "Common_MapLocalPhones",
                                 pPhone->nSeqConnId);
            break;
         }
         case EXTERN_VOIP_CALL:
         {
#ifndef TAPI_VERSION4
#ifdef TD_DECT
            if (PHONE_TYPE_DECT == pLocalPhone->nType)
            {
               ret = TD_DECT_MapToData(pLocalPhone->nDectCh,
                                       pOldConn->nUsedCh, TD_MAP_REMOVE,
                                       pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToData",
                                    pPhone->nSeqConnId);
            }
            else
#endif /* TD_DECT */
#ifdef EASY3111
            if (TYPE_DUSLIC_XT == pLocalPhone->pBoard->nType)
            {
               /* Unmap previous called data ch
                  from phone channel that is leaving conference. */
               ret = PCM_MapToData(
                  pLocalPhone->oEasy3111Specific.nOnMainPCM_Ch_Bak,
                  pOldConn->nUsedCh, IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData",
                                    pPhone->nSeqConnId);
            }
            else
#endif /* EASY3111 */
            {
               /* Unmap previous called data ch
                  from phone channel that is leaving conference. */
               ret = VOIP_MapPhoneToData(pOldConn->nUsedCh,
                        pLocalPhone->nDev, pLocalPhone->nPhoneCh,
                        IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData",
                                    pPhone->nSeqConnId);
            }
#endif /* TAPI_VERSION4 */
            break;
         }
         case PCM_CALL:
         {
            ret = CONFERENCE_MapLocalPCM(pCtrl, pLocalPhone, pOldConn,
                                         TD_MAP_REMOVE);
            TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                 pPhone->nSeqConnId);
            break;
         }
#ifdef FXO
         case FXO_CALL:
         {
            TD_PTR_CHECK_IN_CASE(pPhone->pFXO, pPhone->nSeqConnId)
            if (FXO_TYPE_SLIC121 == pPhone->pFXO->pFxoDevice->nFxoType)
            {
               ret = CONFERENCE_MapLocalALM(pCtrl, pLocalPhone, pOldConn,
                                            TD_MAP_REMOVE);
               TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalALM",
                                    pPhone->nSeqConnId);
            }
            else
            {
               ret = CONFERENCE_MapLocalPCM(pCtrl, pLocalPhone, pOldConn,
                                            TD_MAP_REMOVE);
               TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                    pPhone->nSeqConnId);
            }
            break;
         }
#endif /* FXO */
         break;
         default:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, on restoring mapping for phone ch %d. "
                   "(File: %s, line: %d)\n",
                   (int) pLocalPhone->nPhoneCh,
                   __FILE__, __LINE__));
            break;
      } /* switch(pOldConn->nType) */
   } /* for all conections of phone */
#endif /* EASY336 */

   if(pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)
   {
#ifndef TAPI_VERSION4
      /* Stop codec on this data channel connected to remote peer */
      VOIP_StopCodec(pConn->nUsedCh, pConn, pPhone->pBoard, pPhone);
#else
#ifdef EASY336
      VOIP_StopCodec(pConn->nUsedCh, pConn,
                     ABSTRACT_GetBoard(pCtrl,
                                       IFX_TAPI_DEV_TYPE_VIN_SUPERVIP),
                     pPhone);
      /* return codec */
      resources.nType = RES_CODEC | RES_VOIP_ID;
      resources.nConnID_Nr = SVIP_RM_UNUSED;
      resources.nVoipID_Nr = pConn->voipID;
      Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
   }

   return ret;
}

/**
   Add external phone to conference.

   \param pCtrl - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pConn - pointer to connection

   \return IFX_SUCCESS if ok otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerExternal_Add(CTRL_STATUS_t* pCtrl,
                                         PHONE_t* pPhone,
                                         CONNECTION_t* pConn)
{
   CONFERENCE_t* pConf = IFX_NULL;
   IFX_return_t ret = IFX_SUCCESS;
#ifndef EASY336
   PHONE_t *pLocalPhone;
   IFX_uint32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;
#endif
#ifdef EASY336
   RESOURCES_t resources;
   VOIP_DATA_CH_t *pCodec;
#endif
   BOARD_t *pUsedBoard = IFX_NULL;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Get conference */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* set as true so data channel will be allocated */
      pPhone->fExtPeerCalled = IFX_TRUE;
      pUsedBoard = pPhone->pBoard;
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* set as true so data channel will be allocated */
      pPhone->fExtPeerCalled = IFX_TRUE;
      /* for DuSLIC-xT resources from main board are used */
      pUsedBoard = TD_GET_MAIN_BOARD(pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }
   /* check if default data channel is available */
   if (IFX_FALSE == pPhone->fExtPeerCalled)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: PeerExternal_Add() -> using default data channel\n",
             (int) pPhone->nPhoneNumber));

      /* First time making connection with other external phone */
      /* but we could be connected with local or PCM phone(s) */
      pPhone->fExtPeerCalled = IFX_TRUE;
      /* Save caller channel */
      pConn->nUsedCh = pPhone->nDataCh;
      pConn->nUsedCh_FD = pPhone->nDataCh_FD;
#ifdef TAPI_VERSION4
      if (pUsedBoard->fUseSockets)
#endif
      {
         /* set used socket number */
         pConn->nUsedSocket = VOIP_GetSocket(pConn->nUsedCh, pUsedBoard,
                                             pConn->bIPv6_Call);
      }

#ifdef EASY336
      if (pConn->voipID == SVIP_RM_UNUSED)
      {
         /* using IPv4 address of svip device */
         TD_SOCK_FamilySet(&pConn->oUsedAddr, AF_INET);
         /* get codec */
         resources.nType = RES_CODEC | RES_VOIP_ID;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         if (Common_ResourcesGet(pPhone, &resources) != IFX_SUCCESS)
         {
            /* There is no more free data channels */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, No free codecs available. (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            /* Put this PHONE back to active for other phones in conference. */
            return IFX_ERROR;
         }
         pConn->voipID = pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
         pCodec = Common_GetCodec(pCtrl, pConn->voipID, pPhone->nSeqConnId);
         if (pCodec != IFX_NULL)
         {
            pConn->nUsedCh = pCodec->nCh;
            TD_SOCK_PortSet(&pConn->oUsedAddr, pCodec->nUDP_Port);
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Common_GetCodec failed (File: %s, line: %d)\n",
                __FILE__, __LINE__));
            /* Put this PHONE back to active for other phones in conference. */
            return IFX_ERROR;
         }

         ret = SVIP_libDevIP_Get(pCodec->nDev,
            TD_SOCK_GetAddrIn(&pConn->oUsedAddr));

         if (ret != SVIP_libStatusOk)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, SVIP_libDevIP_Get returned error %d (File: %s, line: %d)\n",
                ret, __FILE__, __LINE__));
               /* Put this PHONE back to active for other phones in
                * conference. */
               return IFX_ERROR;
         }

      }
#endif /* EASY336 */

   }
   else if ((IFX_TRUE == pPhone->nConfStarter)
          && (IFX_NULL != pConf))
   {
      /* This channel is already in connection with other phone, so
         we must use different channel for new external phone */
      /* So primary data channel is used for dialing the number
         and when ext call, a new free data channel must be used
         and leave old one in state talking */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: PeerExternal_Add() -> using new data channel\n",
             (int) pPhone->nPhoneNumber));

#ifndef EASY336
      /* Get free data channel */
      pConn->nUsedCh = VOIP_GetFreeDataCh(pUsedBoard, pPhone->nSeqConnId);

      if (NO_FREE_DATA_CH == pConn->nUsedCh)
      {
         /* There is no more free data channels */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, No more free data channels. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         /* Put this PHONE back to active for other phones in conference. */
         return IFX_ERROR;
      }
#else /* EASY336 */
      if (pConn->voipID == SVIP_RM_UNUSED)
      {
         /* using IPv4 address of device */
         TD_SOCK_FamilySet(&pConn->oUsedAddr, AF_INET);
         /* get codec */
         resources.nType = RES_CODEC | RES_VOIP_ID;
         resources.nConnID_Nr = SVIP_RM_UNUSED;
         if (Common_ResourcesGet(pPhone, &resources) != IFX_SUCCESS)
         {
            /* There is no more free data channels */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, No free codecs available. (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
            /* Put this PHONE back to active for other phones in conference. */
            return IFX_ERROR;
         }
         pConn->voipID = pPhone->pVoipIDs[pPhone->nVoipIDs - 1];
         pCodec = Common_GetCodec(pCtrl, pConn->voipID, pPhone->nSeqConnId);
         if (pCodec != IFX_NULL)
         {
            pConn->nUsedCh = pCodec->nCh;
            TD_SOCK_PortSet(&pConn->oUsedAddr, pCodec->nUDP_Port);
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, Common_GetCodec failed (File: %s, line: %d)\n",
                __FILE__, __LINE__));
            /* Put this PHONE back to active for other phones in conference. */
            return IFX_ERROR;
         }

         ret = SVIP_libDevIP_Get(pCodec->nDev,
            TD_SOCK_GetAddrIn(&pConn->oUsedAddr));

         if (ret != SVIP_libStatusOk)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                 ("Err, SVIP_libDevIP_Get returned error %d (File: %s, "
                  "line: %d)\n",
                  ret, __FILE__, __LINE__));
               /* Put this PHONE back to active for other phones in
                * conference. */
               return IFX_ERROR;
         }
      }
#endif /* EASY336 */

      /* Save caller channel */
#ifndef EASY336
      pConn->nUsedCh_FD = VOIP_GetFD_OfCh(pConn->nUsedCh, pUsedBoard);
#ifdef TAPI_VERSION4
      if (pUsedBoard->fUseSockets)
#endif
      {
         /* set used socket fd number */
         pConn->nUsedSocket = VOIP_GetSocket(pConn->nUsedCh, pUsedBoard,
                                             pConn->bIPv6_Call);
      }
      /*pConn->nUsedSocket = VOIP_GetFreeSocket(pCtrl);*/

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Use data ch %d, socket %d "
             "for new external connection.\n",
             (int) pPhone->nPhoneNumber, (int) pConn->nUsedCh,
             (int) pConn->nUsedSocket));
#endif /* EASY336 */

      /* Map initiator to this new data channel - its important that initiator
         is mapped first, so its also mapped to signal module. */
#ifndef EASY336
#ifndef TAPI_VERSION4
#ifdef TD_DECT
      if (PHONE_TYPE_DECT == pPhone->nType)
      {
         ret = TD_DECT_MapToData(pPhone->nDectCh, pConn->nUsedCh, TD_MAP_ADD,
                                 pUsedBoard, pPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToData", pPhone->nSeqConnId);
      }
      else
#endif /* TD_DECT */
#ifdef EASY3111
      if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
      {
         ret = PCM_MapToData(pPhone->oEasy3111Specific.nOnMainPCM_Ch,
                  pConn->nUsedCh, IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
      }
      else
#endif /* EASY3111 */
      {
         ret = VOIP_MapPhoneToData(pConn->nUsedCh, pPhone->nDev,
                  pPhone->nPhoneCh, IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData", pPhone->nSeqConnId);
      }
#endif /* TAPI_VERSION4 */
      VOIP_ReserveDataCh(pConn->nUsedCh, pUsedBoard, pPhone->nSeqConnId);
#endif /* EASY336 */

#ifndef TAPI_VERSION4
      /* stop playing tone on added data channel - workaround? */
      if (0 <= pConn->nUsedCh_FD)
      {
         TD_IOCTL(pConn->nUsedCh_FD, IFX_TAPI_TONE_STOP, NO_PARAM,
            TD_DEV_NOT_SET, pPhone->nSeqConnId);
      }
#endif /* TAPI_VERSION4 */
   }

#ifndef EASY336
   /* for all connections */
   for (i = 0; i < pPhone->nConnCnt - 1; i++)
   {
      pOldConn = &pPhone->rgoConn[i];

      switch (pOldConn->nType)
      {
         case LOCAL_CALL:
            /* Because of external call also map phone channel to
               data channel */
            pLocalPhone = (pOldConn->oConnPeer).oLocal.pPhone;
#ifndef TAPI_VERSION4
#ifdef TD_DECT
            if (PHONE_TYPE_DECT == pLocalPhone->nType)
            {
               ret = TD_DECT_MapToData(pLocalPhone->nDectCh, pConn->nUsedCh,
                        TD_MAP_ADD, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToData",
                                    pPhone->nSeqConnId);
            }
            else
#endif /* TD_DECT */
#ifdef EASY3111
            if (TYPE_DUSLIC_XT == pLocalPhone->pBoard->nType)
            {
               ret = PCM_MapToData(pLocalPhone->oEasy3111Specific.nOnMainPCM_Ch,
                        pConn->nUsedCh, IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
            }
            else
#endif /* EASY3111 */
            {
               ret = VOIP_MapPhoneToData(pConn->nUsedCh, pPhone->nDev,
                        pLocalPhone->nPhoneCh, IFX_TRUE,
                        pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData",
                                    pPhone->nSeqConnId);
            }
#endif /* TAPI_VERSION4 */
            /* Only map phones if first time calling external phone and
               no conference before. */

         break;
         case EXTERN_VOIP_CALL:
            /* External call, do nothing, if phone channel is mapped to two data
               channels then connection between the data channels is implemented
               automatically by the TAPI*/
         break;
         case PCM_CALL:
            /* Map PCM to other data channels in conference */
            ret = PCM_MapToData(pOldConn->nUsedCh, pConn->nUsedCh,
                                IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
            TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
         break;
#ifdef FXO
         case FXO_CALL:
            TD_PTR_CHECK_IN_CASE(pPhone->pFXO, pPhone->nSeqConnId)
            if (FXO_TYPE_SLIC121 == pPhone->pFXO->pFxoDevice->nFxoType)
            {
               /* Map ALM to other data channels in conference */
               ret = VOIP_MapPhoneToData(pConn->nUsedCh, 0, pOldConn->nUsedCh,
                        IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData",
                                    pPhone->nSeqConnId);
            }
            else
            {
               /* Map PCM to other data channels in conference */
               ret = PCM_MapToData(pOldConn->nUsedCh, pConn->nUsedCh,
                                   IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
            }
         break;
#endif /* FXO */
         default:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Unhandled call type %d. (File: %s, line: %d)\n",
                   pOldConn->nType, __FILE__, __LINE__));
         break;
      }
   } /* for */
#endif /* EASY336 */

   if (IFX_NULL != pConf)
   {
      pConf->nExternalPeersCnt++;
   }

   return ret;
} /* CONFERENCE_PeerExternal_Add() */

/**
   Remove external phone from conference.

   \param pCtrl - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pUsedBoard - pointer to BOARD
   \param pConn - pointer to connection

   \return IFX_SUCCESS if ok otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerExternal_Remove(CTRL_STATUS_t* pCtrl,
                                            PHONE_t* pPhone,
                                            BOARD_t* pUsedBoard,
                                            CONNECTION_t* pConn)
{
   CONFERENCE_t* pConf = IFX_NULL;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;
#ifndef TAPI_VERSION4
   PHONE_t* pLocalPhone;
#endif
#ifdef EASY336
   RESOURCES_t resources;
#endif

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pUsedBoard, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Get conference if exists */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

   if (IFX_TRUE == pConn->fActive)
   {
      /* print call progress trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
         ("Phone No %d: Disconnected %s with PhoneNo %d\n%son %s\n",
          pPhone->nPhoneNumber,
          Common_Enum2Name(pConn->nType, TD_rgCallTypeName),
          pConn->oConnPeer.nPhoneNum,
          pUsedBoard->pIndentPhone,
          TD_GetStringIP(&pConn->oConnPeer.oRemote.oToAddr)));
   }
   /* check if external dialing in progress */
   if (IFX_TRUE == pConn->bExtDialingIn)
   {
      pConn->bExtDialingIn = IFX_FALSE;
      TD_TIMEOUT_STOP(pCtrl->nUseTimeoutSocket);
   }
   /* REMOTE */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
        ("Phone No %d: Removing called external party [%s]:%d\n"
         "%sfrom conference, used data ch %d for calling.\n",
         pPhone->nPhoneNumber,
         TD_GetStringIP(&pConn->oConnPeer.oRemote.oToAddr),
         TD_SOCK_PortGet(&pConn->oConnPeer.oRemote.oToAddr),
         pUsedBoard->pIndentPhone,
         (IFX_int32_t) pConn->nUsedCh));

   /* if not default data channel */
   if (pConn->nUsedCh != pPhone->nDataCh)
   {
#ifndef EASY336
#ifndef TAPI_VERSION4
#ifdef TD_DECT
      if (PHONE_TYPE_DECT == pPhone->nType)
      {
         ret = TD_DECT_MapToData(pPhone->nDectCh, pConn->nUsedCh,
                  TD_MAP_REMOVE, pUsedBoard, pPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToData", pPhone->nSeqConnId);
      }
      else
#endif /* TD_DECT */
#ifdef EASY3111
      if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
      {
         /* Unmap previous called data ch
            from phone channel that is leaving conference. */
         ret = PCM_MapToData(pPhone->oEasy3111Specific.nOnMainPCM_Ch_Bak,
                  pConn->nUsedCh, IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
      }
      else
#endif /* EASY3111 */
      {
         ret = VOIP_MapPhoneToData(pConn->nUsedCh, pPhone->nDev,
                  pPhone->nPhoneCh, IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
         TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData", pPhone->nSeqConnId);
      }
#endif /* TAPI_VERSION4 */
      VOIP_FreeDataCh(pConn->nUsedCh, pUsedBoard, pPhone->nSeqConnId);
#endif /* EASY336 */
   }
#ifdef TD_DECT
   else if (PHONE_TYPE_DECT == pPhone->nType)
   {
      ret = TD_DECT_DataChannelRemove(pPhone);
      TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_DataChannelRemove", pPhone->nSeqConnId);
   }
#endif /* TD_DECT */

   /* do unmapping for all existing connections */
   for (i=0; i<pPhone->nConnCnt; i++)
   {
      pOldConn = &pPhone->rgoConn[i];
      /* for connection that is now removed take no action */
      if (pConn == pOldConn)
      {
         /* skip this connection */
         continue;
      }
      switch(pOldConn->nType)
      {
         case LOCAL_CALL:
#ifndef TAPI_VERSION4
            pLocalPhone = pOldConn->oConnPeer.oLocal.pPhone;
#ifdef TD_DECT
            if (PHONE_TYPE_DECT == pLocalPhone->nType)
            {
               ret = TD_DECT_MapToData(pLocalPhone->nDectCh, pConn->nUsedCh,
                        TD_MAP_REMOVE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToData",
                                    pPhone->nSeqConnId);
            }
            else
#endif /* TD_DECT */
#ifdef EASY3111
            if (TYPE_DUSLIC_XT == pLocalPhone->pBoard->nType)
            {
               /* Unmap previous called data ch that is leaving conference
                  from PCM channel. */
               ret = PCM_MapToData(
                        pLocalPhone->oEasy3111Specific.nOnMainPCM_Ch_Bak,
                        pConn->nUsedCh, IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
            }
            else
#endif /* EASY3111 */
            {
               /* Unmap called phone channel
                  from data channel used by ended connection */
               ret = VOIP_MapPhoneToData(pConn->nUsedCh, pPhone->nDev,
                        pLocalPhone->nPhoneCh, IFX_FALSE, pUsedBoard,
                        pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData",
                                    pPhone->nSeqConnId);
            }
#endif /* TAPI_VERSION4 */
            break;
         case EXTERN_VOIP_CALL:
            break;
         case PCM_CALL:
#ifdef EASY3111
            if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
#endif /* EASY3111 */
            {
               /* check if default data channel and phone channel */
               if (pPhone->nPCM_Ch == pOldConn->nUsedCh &&
                   pPhone->nDataCh == pConn->nUsedCh)
               {
                  /* unammping is not needed */
                  break;
               }
            }
            /* Unmap called phone PCM channel
               from data channel used by ended connection */
            ret = PCM_MapToData(pOldConn->nUsedCh, pConn->nUsedCh,
                                IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
            TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
            break;
#ifdef FXO
         case FXO_CALL:
            TD_PTR_CHECK_IN_CASE(pPhone->pFXO, pPhone->nSeqConnId)
            if (FXO_TYPE_SLIC121 == pPhone->pFXO->pFxoDevice->nFxoType)
            {
               /* unmap ALM to other data channels in conference */
               ret = VOIP_MapPhoneToData(pConn->nUsedCh, 0, pOldConn->nUsedCh,
                        IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData",
                                    pPhone->nSeqConnId);
            }
            else
            {
               /* Unmap called phone PCM channel
                  from data channel used by ended connection */
               ret = PCM_MapToData(pOldConn->nUsedCh, pConn->nUsedCh,
                                   IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
            }
            break;
#endif /* FXO */
         default:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, restoring mapping for phone ch %d. "
                   "(File: %s, line: %d)\n",
                   (int) pConn->oConnPeer.oLocal.pPhone->nPhoneCh,
                   __FILE__, __LINE__));
            break;
      } /* switch(pOldConn->nType) */
   } /* for all conections of phone */

#ifndef TAPI_VERSION4
   /* Stop codec on this data channel connected to remote peer */
   VOIP_StopCodec(pConn->nUsedCh, pConn, pPhone->pBoard, pPhone);
#else
#ifdef EASY336
   VOIP_StopCodec(pConn->nUsedCh, pConn,
                  ABSTRACT_GetBoard(pCtrl,
                                    IFX_TAPI_DEV_TYPE_VIN_SUPERVIP),
                  pPhone);
   /* return codec */
   resources.nType = RES_CODEC | RES_VOIP_ID;
   resources.nConnID_Nr = SVIP_RM_UNUSED;
   resources.nVoipID_Nr = pConn->voipID;
   Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */

   if (IFX_NULL != pConf)
   {
      pConf->nExternalPeersCnt--;
      if (NO_EXTERNAL_PEER == pConf->nExternalPeersCnt)
      {
         pPhone->fExtPeerCalled = IFX_FALSE;
      }
   }
#ifdef QOS_SUPPORT
   if (pCtrl->pProgramArg->oArgFlags.nQos)
   {
/* no QoS for SVIP */
#ifndef EASY336
      /* QoS Support */
      QOS_StopSession(pConn, pPhone);
#endif /* EASY336 */
   }
#endif /* QOS_SUPPORT */
   return ret;
}

/**
   Add PCM channel to conference.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to connection

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerPCM_Add(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_SUCCESS;
   CONFERENCE_t* pConf = IFX_NULL;
#if (!defined EASY336 && !defined XT16)
   IFX_uint32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;
#endif
   BOARD_t *pUsedBoard = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Get conference */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

#ifdef TD_DECT
   /* check if DECT */
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* set as true so PCM channel will not be allocated */
      pPhone->fPCM_PeerCalled = IFX_TRUE;
      pUsedBoard = pPhone->pBoard;
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   /* check if DxT */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* set as true so PCM channel will be allocated */
      pPhone->fPCM_PeerCalled = IFX_TRUE;
      /* for DuSLIC-xT resources from main board are used */
      pUsedBoard = TD_GET_MAIN_BOARD(pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }
   /* check if using default PCM channel */
   if (IFX_FALSE == pPhone->fPCM_PeerCalled)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("AddPeerPCM() -> using default PCM channel\n"));

      /* First time making connection with other pcm phone, but we could be
         connected with local phone(s) or external phone(s) */
      pPhone->fPCM_PeerCalled = IFX_TRUE;

      /* Save caller channel */
      pConn->nUsedCh = pPhone->nPCM_Ch;
      pConn->nUsedCh_FD = PCM_GetFD_OfCh(pPhone->nPCM_Ch, pUsedBoard);
   } /* if */
#if (!defined EASY336 && !defined XT16)
   else if ((IFX_TRUE == pPhone->nConfStarter) && (IFX_NULL != pConf))
   {
      /* This pcm channel is already in connection with other pcm phone
         or no PCM channel is reserved by phone,
         so we must use different channel for new pcm phone */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("AddPeerPCM() -> using new PCM channel\n"));

      /* Get free pcm channel */
      pConn->nUsedCh = PCM_GetFreeCh(pUsedBoard);
      if (NO_FREE_PCM_CH == pConn->nUsedCh)
      {
         /* There is no more free pcm channels */
         /* Wrong input arguments */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, No more free pcm channels. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         /* Put this PHONE back to active for other phones in conference. */
         return IFX_ERROR;
      }

      PCM_ReserveCh(pConn->nUsedCh, pUsedBoard, pPhone->nSeqConnId);

      /* get FD */
      pConn->nUsedCh_FD = PCM_GetFD_OfCh(pConn->nUsedCh, pUsedBoard);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
           ("Use pcm ch %d, socket %d for new external connection.\n",
            (int) pConn->nUsedCh, (int) pConn->nUsedCh_FD));
   } /* else if */

#if (!defined EASY336 && !defined XT16 && !defined EASY3201 && !defined EASY3201_EVS)
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      ret = TD_DECT_MapToPCM(pPhone->nDectCh, pConn->nUsedCh, TD_MAP_ADD,
                             pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* Map pcm channel to pcm channel */
      ret = PCM_MapToPCM(pPhone->oEasy3111Specific.nOnMainPCM_Ch,
               pConn->nUsedCh, IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* EASY3111 */
   {
      /* Map phone channel to pcm channel */
      ret = PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh,
               IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPhone", pPhone->nSeqConnId);
   }
#endif /* (!defined EASY336 && !defined XT16 && !defined EASY3201) */

   if (IFX_TRUE == pPhone->nConfStarter)
   {
      for (i = 0; i < (pPhone->nConnCnt - 1); i++)
      {
         pOldConn = &pPhone->rgoConn[i];
         switch (pOldConn->nType)
         {
            case LOCAL_CALL:
               ret = CONFERENCE_MapLocalPCM(pCtrl,
                        pOldConn->oConnPeer.oLocal.pPhone,
                        pConn, TD_MAP_ADD);
               TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                    pPhone->nSeqConnId);
            break;
            case EXTERN_VOIP_CALL:
               /* Map PCM to data channel */
               ret = PCM_MapToData(pConn->nUsedCh, pOldConn->nUsedCh,
                                   IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
            break;
            case PCM_CALL:
               /* We have pcm connection, so map pcm ch to this pcm ch. */
               ret = PCM_MapToPCM(pConn->nUsedCh, pOldConn->nUsedCh,
                        IFX_TRUE, pPhone->pBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
            break;
#ifdef FXO
            case FXO_CALL:
               TD_PTR_CHECK_IN_CASE(pPhone->pFXO, pPhone->nSeqConnId)
               if (FXO_TYPE_SLIC121 == pPhone->pFXO->pFxoDevice->nFxoType)
               {
                  /* unmap ALM to other data channels in conference */
                  ret = PCM_MapToPhone(pConn->nUsedCh, pOldConn->nUsedCh,
                           IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
                  TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPhone",
                                       pPhone->nSeqConnId);
               }
               else
               {
                  /* We have pcm connection, so map pcm ch to this pcm ch. */
                  ret = PCM_MapToPCM(pConn->nUsedCh, pOldConn->nUsedCh,
                           IFX_TRUE, pPhone->pBoard, pPhone->nSeqConnId);
                  TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
               }
            break;
#endif /* FXO */
            default:
               /* Unknown call, do nothing */
            break;
         }
      } /* for */

      if (IFX_NULL != pConf)
      {
         pConf->nPCM_PeersCnt++;
      }
   } /* if */
#endif /* !EASY336 && !XT16 */

   return ret;
} /* CONFERENCE_PeerPCM_Add() */


/**
   Remove PCM channel to conference.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pUsedBoard - pointer to BOARD
   \param pConn   - pointer to connection

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerPCM_Remove(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       BOARD_t* pUsedBoard, CONNECTION_t* pConn)
{
   IFX_return_t ret = IFX_ERROR;
   CONFERENCE_t* pConf = IFX_NULL;
   IFX_uint32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;
   TAPIDEMO_PORT_DATA_t oPortData;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Get conference */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

   if (IFX_TRUE == pConn->fActive)
   {
      /* print call progress trace */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
            ("Phone No %d: Disconnected %s with PhoneNo %d, on %s\n",
             pPhone->nPhoneNumber,
             Common_Enum2Name(pConn->nType, TD_rgCallTypeName),
             pConn->oConnPeer.nPhoneNum,
             TD_GetStringIP(&pConn->oConnPeer.oPCM.oToAddr)));
   }
   /* PCM */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
        ("Phone No %d: Removing called pcm party from conference, pcm ch %d\n",
         pPhone->nPhoneNumber, (int) pConn->nUsedCh));

   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType ||
       pPhone->nPCM_Ch != pConn->nUsedCh)
   {
      ret = PCM_FreeCh(pConn->nUsedCh, pUsedBoard);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_FreeCh", pPhone->nSeqConnId);
   }
#ifdef TD_DECT
   else if (PHONE_TYPE_DECT == pPhone->nType)
   {
      ret = TD_DECT_PCM_ChannelRemove(pPhone);
      TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_PCM_ChannelRemove",
                           pPhone->nSeqConnId);
   }
#endif /* TD_DECT */
#if !defined EASY3201 && !defined EASY3201_EVS
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      ret = TD_DECT_MapToPCM(pPhone->nDectCh, pConn->nUsedCh,
                             TD_MAP_REMOVE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* Unmap conference starter PCM channel from
         PCM channel of ended connection*/
      ret = PCM_MapToPCM(pPhone->oEasy3111Specific.nOnMainPCM_Ch_Bak,
               pConn->nUsedCh, IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* EASY3111 */
   {
      /* Unmap conference starter phone channel from
         PCM channel of ended connection*/
      ret = PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh,
               IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPhone", pPhone->nSeqConnId);
   }
#endif /* EASY3201 */
   /* do unmapping for all existing connections */
   for (i=0; i<pPhone->nConnCnt; i++)
   {
      pOldConn = &pPhone->rgoConn[i];
      /* for connection that is now removed take no action */
      if (pConn == pOldConn)
      {
         /* skip this one */
         continue;
      }
      switch(pOldConn->nType)
      {
         case LOCAL_CALL:
            ret = CONFERENCE_MapLocalPCM(pCtrl,
                     pOldConn->oConnPeer.oLocal.pPhone,
                     pConn, TD_MAP_REMOVE);
            TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                 pPhone->nSeqConnId);
            break;
         case EXTERN_VOIP_CALL:
            /* Unmap phone channel from
               data channel of connection that ended */
            ret = PCM_MapToData(pConn->nUsedCh, pOldConn->nUsedCh,
                                IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
            TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
            break;
         case PCM_CALL:
            /* Unmap phone channel from
               PCM channel of connection that ended */
            ret = PCM_MapToPCM(pConn->nUsedCh, pOldConn->nUsedCh,
                               IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
            TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
            break;
#ifdef FXO
         case FXO_CALL:
            TD_PTR_CHECK_IN_CASE(pPhone->pFXO, pPhone->nSeqConnId)
            if (FXO_TYPE_SLIC121 == pPhone->pFXO->pFxoDevice->nFxoType)
            {
               /* unmap ALM to other data channels in conference */
               ret = PCM_MapToPhone(pConn->nUsedCh, pOldConn->nUsedCh,
                        IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPhone", pPhone->nSeqConnId);
            }
            else
            {
               /* Unmap phone channel from
                  PCM channel of connection that ended */
               ret = PCM_MapToPCM(pConn->nUsedCh, pOldConn->nUsedCh,
                                  IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
            }
            break;
#endif /* FXO */
         default:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, restoring mapping for phone ch %d. "
                   "(File: %s, line: %d)\n",
                   (int) pConn->oConnPeer.oLocal.pPhone->nPhoneCh,
                   __FILE__, __LINE__));
            break;
      } /* switch(pOldConn->nType) */
   } /* for all conections of phone */

   oPortData.nType = PORT_FXS;
   oPortData.uPort.pPhopne = pPhone;
   /* deactivate PCM and free timeslots */
   PCM_EndConnection(&oPortData, pCtrl->pProgramArg, pUsedBoard, pConn,
                     pPhone->nSeqConnId);

   if (IFX_NULL != pConf)
   {
      pConf->nPCM_PeersCnt--;
      if (NO_PCM_PEER == pConf->nPCM_PeersCnt)
      {
         pPhone->fPCM_PeerCalled = IFX_FALSE;
      }
   }

   return ret;
} /* CONFERENCE_PeerPCM_Add() */

#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)

/**
   Add fxo using PCM to conference.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to connection
   \param pFxo    - pointer to FXO structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerFXO_PCM_Add(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn, FXO_t* pFxo)
{
   IFX_return_t ret = IFX_SUCCESS;
   CONFERENCE_t* pConf = IFX_NULL;
#if (!defined EASY336 && !defined XT16)
   IFX_uint32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;
#endif
   BOARD_t *pUsedBoard = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pFxo, IFX_ERROR);

   /* Get conference */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

#ifdef TD_DECT
   /* check if DECT */
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* set as true so PCM channel will be allocated */
      pPhone->fPCM_PeerCalled = IFX_TRUE;
      pUsedBoard = pPhone->pBoard;
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   /* check if DxT */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* set as true so PCM channel will be allocated */
      pPhone->fPCM_PeerCalled = IFX_TRUE;
      /* for DuSLIC-xT resources from main board are used */
      pUsedBoard = TD_GET_MAIN_BOARD(pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }
   /* check if using default PCM channel */
   if (IFX_FALSE == pPhone->fPCM_PeerCalled)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("AddPeerFXO_PCM() -> using default PCM channel\n"));

      /* First time making connection with other pcm phone, but we could be
         connected with local phone(s) or external phone(s) */
      pPhone->fPCM_PeerCalled = IFX_TRUE;

      /* Save caller channel */
      pConn->nUsedCh = pPhone->nPCM_Ch;
      pConn->nUsedCh_FD = PCM_GetFD_OfCh(pPhone->nPCM_Ch, pUsedBoard);
   } /* if */
   else if ((IFX_TRUE == pPhone->nConfStarter) && (IFX_NULL != pConf))
   {
      /* This pcm channel is already in connection with other pcm phone
         or no PCM channel is reserved by phone,
         so we must use different channel for new pcm phone */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("AddPeerFXO_PCM() -> using new PCM channel\n"));

      /* Get free pcm channel */
      pConn->nUsedCh = PCM_GetFreeCh(pUsedBoard);
      if (NO_FREE_PCM_CH == pConn->nUsedCh)
      {
         /* There is no more free pcm channels */
         /* Wrong input arguments */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("No more free pcm channels. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         /* Put this PHONE back to active for other phones in conference. */
         return IFX_ERROR;
      }

      PCM_ReserveCh(pConn->nUsedCh, pUsedBoard, pPhone->nSeqConnId);

      /* get FD */
      pConn->nUsedCh_FD = PCM_GetFD_OfCh(pConn->nUsedCh, pUsedBoard);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Use pcm ch %d, fd %d for new FXO connection.\n",
             (int) pConn->nUsedCh, (int) pConn->nUsedCh_FD));
   } /* else if */

#if (!defined EASY336 && !defined XT16 && !defined EASY3201 && !defined EASY3201_EVS)
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      ret = TD_DECT_MapToPCM(pPhone->nDectCh, pConn->nUsedCh, TD_MAP_ADD,
                             pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* Map pcm channel to pcm channel */
      ret = PCM_MapToPCM(pPhone->oEasy3111Specific.nOnMainPCM_Ch,
               pConn->nUsedCh, IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* EASY3111 */
   {
      /* Map phone channel to pcm channel */
      ret = PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh,
               IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPhone", pPhone->nSeqConnId);
   }
#endif /* (!defined EASY336 && !defined XT16 && !defined EASY3201) */

   if (IFX_TRUE == pPhone->nConfStarter)
   {
      for (i = 0; i < (pPhone->nConnCnt - 1); i++)
      {
         pOldConn = &pPhone->rgoConn[i];
         switch (pOldConn->nType)
         {
            case LOCAL_CALL:
               ret = CONFERENCE_MapLocalPCM(pCtrl,
                        pOldConn->oConnPeer.oLocal.pPhone,
                        pConn, TD_MAP_ADD);
               TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                    pPhone->nSeqConnId);
            break;
            case EXTERN_VOIP_CALL:
               /* Map PCM to data channel */
               ret = PCM_MapToData(pConn->nUsedCh, pOldConn->nUsedCh,
                                   IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
            break;
            case PCM_CALL:
               /* We have pcm connection, so map pcm ch to this pcm ch. */
               ret = PCM_MapToPCM(pConn->nUsedCh, pOldConn->nUsedCh,
                        IFX_TRUE, pPhone->pBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
            break;
            case FXO_CALL:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, Phone No %d: only one FXO connection can be made in "
                      "single conference. (File: %s, line: %d)\n",
                      pPhone->nPhoneNumber, __FILE__, __LINE__));
            break;
            default:
               /* Unknown call, do nothing */
            break;
         }
      } /* for */

      if (IFX_NULL != pConf)
      {
         pConf->nPCM_PeersCnt++;
      }
   } /* if */

   return ret;
} /* */

/**
   Add fxo using PCM to conference.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pUsedBoard - pointer to BOARD
   \param pConn   - pointer to connection
   \param pFxo    - pointer to FXO structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerFXO_PCM_Remove(CTRL_STATUS_t* pCtrl,
                                           PHONE_t* pPhone,
                                           BOARD_t *pUsedBoard,
                                           CONNECTION_t* pConn, FXO_t* pFxo)
{
   IFX_return_t ret = IFX_SUCCESS;
   CONFERENCE_t* pConf = IFX_NULL;
#if (!defined EASY336 && !defined XT16)
   IFX_uint32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;
#endif

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pUsedBoard, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pFxo, IFX_ERROR);

   /* Get conference */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

   /* FXO */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
        ("Phone No %d: Removing called FXO No %d from conference.\n",
         pPhone->nPhoneNumber, (int) pFxo->nFXO_Number));

   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType ||
       pPhone->nPCM_Ch != pConn->nUsedCh)
   {
      ret = PCM_FreeCh(pConn->nUsedCh, pUsedBoard);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_FreeCh", pPhone->nSeqConnId);
   }
#ifdef TD_DECT
   else if (PHONE_TYPE_DECT == pPhone->nType)
   {
      ret = TD_DECT_PCM_ChannelRemove(pPhone);
      TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_PCM_ChannelRemove", pPhone->nSeqConnId);
   }
#endif /* TD_DECT */
#if !defined EASY3201 && !defined EASY3201_EVS
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      ret = TD_DECT_MapToPCM(pPhone->nDectCh, pConn->nUsedCh,
               TD_MAP_REMOVE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "TD_DECT_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* TD_DECT */
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* Unmap conference starter PCM channel from
         PCM channel of ended connection*/
      ret = PCM_MapToPCM(pPhone->oEasy3111Specific.nOnMainPCM_Ch_Bak,
               pConn->nUsedCh, IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
   }
   else
#endif /* EASY3111 */
   {
      /* Unmap conference starter phone channel from
         PCM channel of ended connection*/
      ret = PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh,
               IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
      TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPhone", pPhone->nSeqConnId);
   }
#endif /* EASY3201 */
   /* do unmapping for all existing connections */
   for (i=0; i<pPhone->nConnCnt; i++)
   {
      pOldConn = &pPhone->rgoConn[i];
      /* for connection that is now removed take no action */
      if (pConn == pOldConn)
      {
         /* skip this one */
         continue;
      }
      switch(pOldConn->nType)
      {
         case LOCAL_CALL:
            ret = CONFERENCE_MapLocalPCM(pCtrl,
                     pOldConn->oConnPeer.oLocal.pPhone,
                     pConn, TD_MAP_REMOVE);
            TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                 pPhone->nSeqConnId);
            break;
         case EXTERN_VOIP_CALL:
            /* Unmap phone channel from
               data channel of connection that ended */
            ret = PCM_MapToData(pConn->nUsedCh, pOldConn->nUsedCh,
                                IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
            TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToData", pPhone->nSeqConnId);
            break;
         case PCM_CALL:
            /* Unmap phone channel from
               PCM channel of connection that ended */
            ret = PCM_MapToPCM(pConn->nUsedCh, pOldConn->nUsedCh,
                               IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
            TD_PRINT_ON_FUNC_ERR(ret, "PCM_MapToPCM", pPhone->nSeqConnId);
            break;
         case FXO_CALL:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Phone No %d: only one FXO connection can be made in "
                   "single conference. (File: %s, line: %d)\n",
                   pPhone->nPhoneNumber, __FILE__, __LINE__));
            break;
         default:
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, restoring mapping for phone ch %d. "
                   "(File: %s, line: %d)\n",
                   (int) pConn->oConnPeer.oLocal.pPhone->nPhoneCh,
                   __FILE__, __LINE__));
            break;
      } /* switch(pOldConn->nType) */
   } /* for all conections of phone */

   /* end fxo connection */
   ret = FXO_EndConnection(pCtrl->pProgramArg, pFxo, pPhone, pConn,
                           TD_GET_MAIN_BOARD(pCtrl));
   TD_PRINT_ON_FUNC_ERR(ret, "FXO_EndConnection", pPhone->nSeqConnId);

   if (IFX_NULL != pConf)
   {
      pConf->nPCM_PeersCnt--;
      if (NO_PCM_PEER == pConf->nPCM_PeersCnt)
      {
         pPhone->fPCM_PeerCalled = IFX_FALSE;
      }
   }

   return ret;
} /* */

/**
   Add fxo using ALM to conference.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to connection
   \param pFxo    - pointer to FXO structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerFXO_ALM_Add(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                        CONNECTION_t* pConn, FXO_t* pFxo)
{
   CONFERENCE_t* pConf = IFX_NULL;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;
   BOARD_t* pUsedBoard = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pFxo, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Get conference if exists */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

#ifdef EASY3111
   /* check phone type */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* set board pointer */
      pUsedBoard = TD_GET_MAIN_BOARD(pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      /* set board pointer */
      pUsedBoard = pPhone->pBoard;
   }

   /* set used channel */
   pConn->nUsedCh = pFxo->nFxoCh;
   pConn->nUsedCh_FD = pFxo->nFxoCh_FD;

   if ((IFX_NULL != pConf) && (IFX_TRUE == pPhone->nConfStarter))
   {
      /* If we have conference and we are the initiator */
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Adding ALM FXO no %d to conference\n",
             pPhone->nPhoneNumber, pFxo->nFXO_Number));
      /* do mapping for all connections */
      for (i = 0; i < pPhone->nConnCnt - 1; i++)
      {
         pOldConn = &pPhone->rgoConn[i];
         switch (pOldConn->nType)
         {
            case LOCAL_CALL:
#ifndef EASY336
               /* Map previous called phone ch with new called phone ch. */
               ret = CONFERENCE_MapLocalALM(pCtrl, pOldConn->oConnPeer.oLocal.pPhone,
                                            pConn, TD_MAP_ADD);
               TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalALM",
                                    pPhone->nSeqConnId);

#endif /* EASY336 */
               break;
            case EXTERN_VOIP_CALL:
               /* Because of external call also map phone channel to
                  all data channel(s) that are participating in conference on
                  this board. */
#ifndef EASY336
#ifndef TAPI_VERSION4
               ret = VOIP_MapPhoneToData(pOldConn->nUsedCh, 0, pFxo->nFxoCh,
                       IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData",
                                    pPhone->nSeqConnId);
#endif /* TAPI_VERSION4 */
#endif /* EASY336 */
               break;
            case PCM_CALL:
               ret = PCM_MapToPhone(pOldConn->nUsedCh, pConn->nUsedCh,
                        IFX_TRUE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                    pPhone->nSeqConnId);
               break;
            case FXO_CALL:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, Phone No %d: only one FXO connection can be made in "
                      "single conference. (File: %s, line: %d)\n",
                      pPhone->nPhoneNumber, __FILE__, __LINE__));
               break;
            default:
               /* Unknown call, do nothing */
            break;
         } /* switch */
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Failed to add Phone No %d to conn No %d type %s."
                   "(File: %s, line: %d)\n",
                   pFxo->nFXO_Number, i,
                   Common_Enum2Name(pOldConn->nType, TD_rgCallTypeName),
                   __FILE__, __LINE__));
         }
      } /* for */
   }
   return ret;
} /* () */

/**
   Remove fxo using ALM from conference.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to connection
   \param pFxo    - pointer to FXO structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerFXO_ALM_Remove(CTRL_STATUS_t* pCtrl,
                                           PHONE_t* pPhone,
                                           CONNECTION_t* pConn, FXO_t* pFxo)
{
   CONFERENCE_t* pConf = IFX_NULL;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   CONNECTION_t* pOldConn = IFX_NULL;
   BOARD_t* pUsedBoard = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pFxo, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Get conference if exists */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

#ifdef EASY3111
   /* check phone type */
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* set board pointer */
      pUsedBoard = TD_GET_MAIN_BOARD(pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      /* set board pointer */
      pUsedBoard = pPhone->pBoard;
   }

   if ((IFX_NULL != pConf) && (IFX_TRUE == pPhone->nConfStarter))
   {
      /* If we have conference and we are the initiator */
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Removing ALM FXO no %d to conference\n",
             pPhone->nPhoneNumber, pFxo->nFXO_Number));
      /* do mapping for all connections */
      for (i = 0; i < pPhone->nConnCnt; i++)
      {
         pOldConn = &pPhone->rgoConn[i];
         /* for connection that is now removed take no action */
         if (pConn == pOldConn)
         {
            /* skip this one */
            continue;
         }
         switch (pOldConn->nType)
         {
            case LOCAL_CALL:
#ifndef EASY336
               /* Map previous called phone ch with new called phone ch. */
               ret = CONFERENCE_MapLocalALM(pCtrl, pOldConn->oConnPeer.oLocal.pPhone,
                                            pConn, TD_MAP_REMOVE);
               TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalALM",
                                    pPhone->nSeqConnId);

#endif /* EASY336 */
               break;
            case EXTERN_VOIP_CALL:
               /* Because of external call also map phone channel to
                  all data channel(s) that are participating in conference on
                  this board. */
#ifndef EASY336
#ifndef TAPI_VERSION4
               ret = VOIP_MapPhoneToData(pOldConn->nUsedCh, 0, pFxo->nFxoCh,
                        IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "VOIP_MapPhoneToData",
                                    pPhone->nSeqConnId);
#endif /* TAPI_VERSION4 */
#endif /* EASY336 */
               break;
            case PCM_CALL:
               ret = PCM_MapToPhone(pOldConn->nUsedCh, pConn->nUsedCh,
                        IFX_FALSE, pUsedBoard, pPhone->nSeqConnId);
               TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_MapLocalPCM",
                                    pPhone->nSeqConnId);
               break;
            case FXO_CALL:
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Err, Phone No %d: only one FXO connection can be made in "
                      "single conference. (File: %s, line: %d)\n",
                      pPhone->nPhoneNumber, __FILE__, __LINE__));
               break;
            default:
               /* Unknown call, do nothing */
            break;
         } /* switch */
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Err, Failed to add fxo No %d to conn No %d type %s."
                   "(File: %s, line: %d)\n",
                   pFxo->nFXO_Number, i,
                   Common_Enum2Name(pOldConn->nType, TD_rgCallTypeName),
                   __FILE__, __LINE__));
         }
      } /* for */
   }

   /* end fxo connection */
   ret = FXO_EndConnection(pCtrl->pProgramArg, pFxo, pPhone, pConn,
                           TD_GET_MAIN_BOARD(pCtrl));
   TD_PRINT_ON_FUNC_ERR(ret, "FXO_EndConnection",
                        pPhone->nSeqConnId);

   return ret;
} /* () */
/**
   Add fxo to conference.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to connection
   \param pFxo    - pointer to FXO structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerFXO_Add(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                   CONNECTION_t* pConn, FXO_t* pFxo)
{
   IFX_return_t nRet = IFX_SUCCESS;

   if (FXO_TYPE_SLIC121 == pFxo->pFxoDevice->nFxoType)
   {
      nRet = CONFERENCE_PeerFXO_ALM_Add(pCtrl, pPhone, pConn, pFxo);
      TD_PRINT_ON_FUNC_ERR(nRet, "CONFERENCE_PeerFXO_ALM_Add",
                           pPhone->nSeqConnId);
   }
   else
   {
      nRet = CONFERENCE_PeerFXO_PCM_Add(pCtrl, pPhone, pConn, pFxo);
      TD_PRINT_ON_FUNC_ERR(nRet, "CONFERENCE_PeerFXO_PCM_Add",
                           pPhone->nSeqConnId);
   }

   return nRet;
} /* */

/**
   Remove fxo from conference.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to connection
   \param pFxo    - pointer to FXO structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerFXO_Remove(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                       BOARD_t *pUsedBoard,
                                       CONNECTION_t* pConn, FXO_t* pFxo)
{
   IFX_return_t nRet = IFX_SUCCESS;

   if (FXO_TYPE_SLIC121 == pFxo->pFxoDevice->nFxoType)
   {
      nRet = CONFERENCE_PeerFXO_ALM_Remove(pCtrl, pPhone, pConn, pFxo);
      TD_PRINT_ON_FUNC_ERR(nRet, "CONFERENCE_PeerFXO_ALM_Remove",
                           pPhone->nSeqConnId);
   }
   else
   {
      nRet = CONFERENCE_PeerFXO_PCM_Remove(pCtrl, pPhone, pUsedBoard,
                                           pConn, pFxo);
      TD_PRINT_ON_FUNC_ERR(nRet, "CONFERENCE_PeerFXO_PCM_Remove",
                           pPhone->nSeqConnId);
   }

   return nRet;
} /* */

/**
   Remove fxo from conference after detecting BUSY tone on FXO.

   \param pCtrl   - pointer to status control structure
   \param pFxo    - pointer to FXO structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PeerFXO_RemoveBusy(CTRL_STATUS_t* pCtrl, FXO_t* pFxo)
{
   PHONE_t *pPhone;
   CONNECTION_t *pConn = IFX_NULL;
   IFX_int32_t i;

   /* get phone */
   pPhone = FXO_GetPhone(pCtrl, pFxo->nFXO_Number);
   if (IFX_NULL == pPhone)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFxo->nSeqConnId,
            ("Err, FXO No %d: Failed to get phone. (File: %s, line: %d)\n",
             pFxo->nFXO_Number, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* check if in conference */
   if (NO_CONFERENCE == pPhone->nConfIdx)
   {
      /* removing FXO peer only if in conference */
      return IFX_SUCCESS;
   }

   /* get connection */
   for (i=0; i < pPhone->nConnCnt; i++)
   {
      if (FXO_CALL == pPhone->rgoConn[i].nType)
      {
         pConn = &pPhone->rgoConn[i];
         break;
      }
   } /* for all connections in conference */

   /* check if connection was found */
   if (IFX_NULL == pConn)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFxo->nSeqConnId,
            ("Err, FXO No %d: Unable to get connection with Phone No %d. "
             "(File: %s, line: %d)\n",
             pFxo->nFXO_Number, pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* set end connection event and handle state transition */
   pPhone->nIntEvent_FXS = IE_END_CONNECTION;
   while (pPhone->nIntEvent_FXS != IE_NONE)
   {
      ST_HandleState_FXS(pCtrl, pPhone, pConn);
   }
   if (IFX_TRUE == pCtrl->bInternalEventGenerated)
   {
      EVENT_InternalEventHandle(pCtrl);
   }

   return IFX_SUCCESS;
} /* */
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */

/**
   Remove peer from conference.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE (conference starter)
   \param ppConn - pointer to pointer of phone connections that will be removed
   \param pConf  - pointer to conference structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR

   \remark Each data channel can be connected only to one external, but
           can be connected/mapped with many local or pcm peers.
           Because of deleting connection, pConn will point to first
           connection.
*/
IFX_return_t CONFERENCE_RemovePeer(CTRL_STATUS_t* pCtrl,
                                   PHONE_t* pPhone,
                                   CONNECTION_t** ppConn,
                                   CONFERENCE_t* pConf)
{
   IFX_return_t ret = IFX_SUCCESS;
   CONNECTION_t *pConn = IFX_NULL;
   BOARD_t *pUsedBoard = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConf, IFX_ERROR);
   TD_PTR_CHECK(ppConn, IFX_ERROR);

   pConn = *ppConn;

   if (IFX_NULL == pConn)
   {
      /* Connections are missing */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
           ("Err, Phone No %d: No phone connections to delete. "
            "(File: %s, line: %d)\n",
            pPhone->nPhoneNumber, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      pUsedBoard = TD_GET_MAIN_BOARD(pCtrl);
   }
   else
#endif /* EASY3111 */
   {
      pUsedBoard = pPhone->pBoard;
   }
   /* removing local call */
   if (LOCAL_CALL == pConn->nType)
   {
      /* remove local peer from conference */
      ret = CONFERENCE_PeerLocal_Remove(pCtrl, pPhone, pUsedBoard, pConn);
      TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_PeerLocal_Remove",
                           pPhone->nSeqConnId);
   } /* if */
   else if (EXTERN_VOIP_CALL == pConn->nType)
   {
      /* remove external peer from conference */
      ret = CONFERENCE_PeerExternal_Remove(pCtrl, pPhone, pUsedBoard, pConn);
      TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_PeerExternal_Remove",
                           pPhone->nSeqConnId);
   } /* else if */
   else if (PCM_CALL == pConn->nType)
   {
      /* remove PCM peer from conference */
      ret = CONFERENCE_PeerPCM_Remove(pCtrl, pPhone, pUsedBoard, pConn);
      TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_PeerPCM_Remove",
                           pPhone->nSeqConnId);
   } /* else if */
#ifdef FXO
   else if (FXO_CALL == pConn->nType)
   {
      /* remove FXO peer from conference */
      ret = CONFERENCE_PeerFXO_Remove(pCtrl, pPhone, pUsedBoard, pConn,
                                      pPhone->pFXO);
      TD_PRINT_ON_FUNC_ERR(ret, "CONFERENCE_PeerPCM_Remove",
                           pPhone->nSeqConnId);
   } /* if */
#endif /* FXO */

   /* if peer successfully removed */
   if (IFX_SUCCESS == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: Peer removed, now remove phone connection.\n",
             pPhone->nPhoneNumber));

      /* conection isn't active */
      pConn->fActive = IFX_FALSE;
      pConn->bIPv6_Call = IFX_FALSE;
      pConn->bExtDialingIn = IFX_FALSE;
      pConn->nType = UNKNOWN_CALL_TYPE;
      /* Remove this connection from array. Move last one to this location
         and clear last one, if we are not the last one. */
      if (0 < pPhone->nConnCnt)
      {
         if (pConn != &pPhone->rgoConn[(pPhone->nConnCnt - 1)])
         {
            /* move data from existing connection
               to ended connection structure */
            memcpy(pConn, &pPhone->rgoConn[pPhone->nConnCnt - 1],
                   sizeof(CONNECTION_t));
         }
         /* reset connection structure */
         memset(&pPhone->rgoConn[pPhone->nConnCnt - 1],
                0, sizeof(CONNECTION_t));
         /* Status we have one less peer */
         pPhone->nConnCnt--;
      } /* if (0 < pPhone->nConnCnt) */
   } /* if (IFX_SUCCESS == ret) */

   /* pConn has changed, because one peer, connection was removed. */
   *ppConn = &pPhone->rgoConn[0];
   return ret;
} /* CONFERENCE_RemovePeer() */


/**
   Clear everything after conference is finished.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pConf  - pointer to conference structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_End(CTRL_STATUS_t* pCtrl,
                            PHONE_t* pPhone,
                            CONFERENCE_t* pConf)
{


   IFX_return_t ret = IFX_SUCCESS;
   CONNECTION_t* pConn = IFX_NULL;
#ifdef TAPI_VERSION4
#ifdef EASY336
   RESOURCES_t resources;
#endif
#endif /* TAPI_VERSION4 */

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConf, IFX_ERROR);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: ending conference (IDX %d, nConnCnt %d).\n",
          pPhone->nPhoneNumber, pPhone->nConfIdx, pPhone->nConnCnt));
   /* end all connections */
   while (0 < pPhone->nConnCnt)
   {
      /* get pointer to last connection structure */
      pConn = &pPhone->rgoConn[pPhone->nConnCnt - 1];
      /* We started conference so end it */
      if (IFX_TRUE == pConn->fActive)
      {
         /* Send all phones which are in active connection BUSY */
         TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_END_CONNECTION,
                            pPhone->nSeqConnId);
      }
      else
      {/* for all conections */
         /* those resources were reserved during dialing */
         if (IFX_TRUE == pConn->bExtDialingIn)
         {
#ifdef TAPI_VERSION4
#ifdef EASY336
            /* return codec */
            resources.nType = RES_CODEC | RES_VOIP_ID;
            resources.nConnID_Nr = SVIP_RM_UNUSED;
            resources.nVoipID_Nr = pConn->voipID;
            Common_ResourcesRelease(pPhone, &resources);
#endif
#endif /* TAPI_VERSION4 */
            pConn->bExtDialingIn = IFX_FALSE;
            TD_TIMEOUT_STOP(pCtrl->nUseTimeoutSocket);
         }
         /* Send all phones which are called (last phone ringing, ...) READY */
         /* Send called phone (is ringing) to stop ringing */
         TAPIDEMO_SetAction(pCtrl, pPhone, pConn, IE_READY, pPhone->nSeqConnId);
      }
      /* end this connection */
      CONFERENCE_RemovePeer(pCtrl, pPhone, &pConn, pConf);
   }
#ifdef EASY3111
   /* EASY 3111 specific actions */
   if (IFX_ERROR == Easy3111_ClearPhone(pPhone, pCtrl, &pPhone->rgoConn[0]))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, Phone No %d: Easy3111_ClearPhone() failed.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
   }
#endif /* EASY3111 */
   /* release this conference index */
   CONFERENCE_ReleaseIdx(&pPhone->nConfIdx, pCtrl, pPhone->nSeqConnId);
   /* clear phone data */
   pPhone->nConnCnt = 0;
   /* set conference as ended */
   pPhone->nConfStarter = IFX_FALSE;

#ifdef TAPI_VERSION4
   /* stop DTMF detection */
   Common_DTMF_Disable(pCtrl, pPhone);
#ifdef EASY336
   resources.nType = RES_CONN_ID | RES_LEC;
   /* stop LEC */
   Common_LEC_Disable(pCtrl, pPhone);
   Common_ResourcesRelease(pPhone, &resources);
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
         ("Phone No %d: Conference has ended.\n",
          pPhone->nPhoneNumber));

   return ret;
} /* CONFERENCE_End() */

