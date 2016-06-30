
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : qos.c
   Date        : 2005-11-29
   Description : This file contains the implementation of the functions for
                 the tapi demo working with quality of service
   \file

   \remarks QOS now only works for first called peer, not an array of peers.

   \note Changes: Only support in LINUX at the moment.
                  Look drv_qos.h for further info.
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "abstract.h"
#include "voip.h"
#include "qos.h"
#ifdef LINUX
#include "tapidemo_config.h"
#endif

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


#ifdef QOS_SUPPORT
/**
   Starts qos session.

   \param pConn   - pointer to phone connection
   \param pPhone  - pointer to phone structure
*/
IFX_return_t QOS_StartSessionOnSocket(CONNECTION_t* pConn,
                                      PHONE_t* pPhone)
{
#ifdef FIO_QOS_ON_SOCKET_START
   IFX_return_t ret = IFX_SUCCESS;
   TD_OS_sockAddr_t oAddrTo;
   QOS_INIT_SESSION_ON_SOCKET nOnSocketArg;
   TD_OS_socket_t nSockFdRemote;
#endif
   CTRL_STATUS_t* pCtrl = IFX_NULL;

   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

   /* get control structure */
   pCtrl = pPhone->pBoard->pCtrl;
   TD_PARAMETER_CHECK((!pCtrl->pProgramArg->oArgFlags.nQosSocketStart),
                      pCtrl->pProgramArg->oArgFlags.nQosSocketStart, IFX_ERROR);

#ifdef FIO_QOS_ON_SOCKET_START
   /* Remote call : start qos session */
   /* Also use QoS or udp redirect on local calls */
   /*   if ((EXTERN_VOIP_CALL == pConn->nType) */
   if (!(
            (PCM_CALL != pConn->nType) &&
            (pCtrl->pProgramArg->oArgFlags.nQos) &&
            (0 == pConn->oSession.nInSession)
         ))
   {
      return ret;
   }
   /* check call type */
   if (EXTERN_VOIP_CALL == pConn->nType)
   {
      /* get addr and socket FD */
      TD_SOCK_AddrCpy(&oAddrTo, &pConn->oConnPeer.oRemote.oToAddr);
      nOnSocketArg.fd = pConn->nUsedSocket;
   }
   else
   {
      /* #warning local call with QoS will not work for all cases:
         - if not using default mapping 0:0 1:1 but 0:1 1:0,
         - for phones on dxt extension board for AR9 */
#ifdef TD_IPV6_SUPPORT
      if (pConn->bIPv6_Call)
      {
         /* get addr and socket FD */
         TD_SOCK_AddrCpy(&oAddrTo, &pCtrl->oIPv6_Ctrl.oAddrIPv6);
      }
      else
#endif /* TD_IPV6_SUPPORT */
      {
         /* get addr and socket FD */
         TD_SOCK_AddrCpy(&oAddrTo, &pCtrl->oTapidemo_IP_Addr);
      }
      /* need to know port of socket used by second phone,
         first get socket and then get port*/
      nSockFdRemote = VOIP_GetSocket(pConn->oConnPeer.oLocal.pPhone->nDataCh,
                                     pConn->oConnPeer.oLocal.pPhone->pBoard,
                                     pConn->bIPv6_Call);
      TD_SOCK_PortSet(&oAddrTo, VOIP_GetSocketPort(nSockFdRemote, pCtrl));
      /* get socket */
      nOnSocketArg.fd = VOIP_GetSocket(pPhone->nDataCh, pPhone->pBoard,
                                       pConn->bIPv6_Call);
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: FIO_QOS_ON_SOCKET_START socket %d addr [%s]:%d.\n",
          pPhone->nPhoneNumber, nOnSocketArg.fd,
          TD_GetStringIP(&oAddrTo), TD_SOCK_PortGet(&oAddrTo)));
   /* connect socket */
   if (IFX_SUCCESS != TD_OS_SocketConnect(nOnSocketArg.fd,
                         &oAddrTo, sizeof(oAddrTo)))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
            ("Err, failed to connect socket %d - %s. (File: %s, line: %d)\n",
             nOnSocketArg.fd, strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }

   ret = TD_IOCTL(pConn->nUsedCh_FD, FIO_QOS_ON_SOCKET_START,
            &nOnSocketArg, TD_DEV_NOT_SET, pPhone->nSeqConnId);
   TD_RETURN_IF_ERROR(ret);

   pConn->oSession.nInSession = 1;
   return IFX_SUCCESS;
#else /* FIO_QOS_ON_SOCKET_START */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
        ("Err, Compiled without FIO_QOS_ON_SOCKET_START. "
         "(File: %s, line: %d)\n",
         __FILE__, __LINE__));
   return IFX_ERROR;
#endif /* FIO_QOS_ON_SOCKET_START */
}

/**
   Disconnects qos (on socket) session.
   This is needed because when making another call socket is still connected
   to previous destination and received packets are causing error messages.

   \param pConn   - pointer to phone connection
   \param pPhone  - pointer to phone structure
*/
IFX_return_t QOS_StopSessionOnSocket(CONNECTION_t* pConn,
                                     PHONE_t* pPhone)
{
#ifdef FIO_QOS_ON_SOCKET_START
   TD_OS_sockAddr_t oSockAddr;
   IFX_int32_t nSocket;

   memset(&oSockAddr, 0, sizeof(oSockAddr));

   /* "Connectionless sockets may dissolve the association by connecting to
      an address with the sa_family member of sockaddr set to AF_UNSPEC
      (supported on Linux since kernel 2.2)." - from connect() man */
   TD_SOCK_FamilySet(&oSockAddr, AF_UNSPEC);

   /* getting socket FD */
   if (EXTERN_VOIP_CALL == pConn->nType)
   {
      nSocket = pConn->nUsedSocket;
   }
   else
   {
      nSocket = VOIP_GetSocket(pPhone->nDataCh, pPhone->pBoard,
                               pConn->bIPv6_Call);
   }

   /* disconnectiong socket */
   if (IFX_SUCCESS != TD_OS_SocketConnect(nSocket,
                                          &oSockAddr, sizeof(oSockAddr)))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
         ("Err, failed to un-connect socket %d - %s. (File: %s, line: %d)\n",
          pConn->nUsedSocket, strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
#else /* FIO_QOS_ON_SOCKET_START */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
        ("Err, Compiled without FIO_QOS_ON_SOCKET_START. "
         "(File: %s, line: %d)\n",
         __FILE__, __LINE__));
   return IFX_ERROR;
#endif /* FIO_QOS_ON_SOCKET_START */
}

/**
   Turn QoS Service on

   \param pProgramArg - pointer to program arguments
*/
IFX_void_t QOS_TurnServiceOn(PROGRAM_ARG_t *pProgramArg)
{
   /* check input arguemnts */
   TD_PTR_CHECK(pProgramArg,);
} /* QOS_TurnServiceOn() */


/**
   Initialize qos pair, only used with external calls.

   \param pCtrl        - pointer to status control structure
   \param pConn        - pointer to phone connection
   \param nDevIdx      - device index
*/
IFX_void_t QOS_InitializePairStruct(CTRL_STATUS_t* pCtrl,
                                    CONNECTION_t* pConn,
                                    IFX_int32_t nDevIdx)
{

/*   IFX_int32_t qos_port = -1; */

   /* check input arguemnts */
   TD_PTR_CHECK(pCtrl,);
   TD_PTR_CHECK(pConn,);

   /* no need to do anything here if using On Socket Start,
      session structure is not used*/
   if (pCtrl->pProgramArg->oArgFlags.nQosSocketStart)
   {
      return;
   }
    /* Initialize pair struct */
   if ((pCtrl->pProgramArg->oArgFlags.nQos) ||
       (0 == pConn->oSession.nInSession))
   {
      pConn->oSession.oPair.srcPort = htons(VOICE_UDP_QOS_PORT +
                                            pConn->nUsedCh * 2 + nDevIdx);

      pConn->oSession.oPair.srcAddr =
         htonl(TD_SOCK_AddrIPv4Get(&pCtrl->pProgramArg->oMy_IP_Addr));
      /* pConn->oSession.oPair.destPort = htons(qos_port); */
      pConn->oSession.oPair.destAddr =
         htonl(TD_SOCK_AddrIPv4Get(&pConn->oConnPeer.oRemote.oToAddr));
   }
} /* QOS_InitializePairStruct() */


/**
   Cleans QoS session

   \param pCtrl       - pointer to status control structure
   \param pProgramArg - pointer to program arguments
*/
IFX_void_t QOS_CleanUpSession(CTRL_STATUS_t* pCtrl, PROGRAM_ARG_t *pProgramArg)
{
   IFX_int32_t i = 0, j, nDataFd;
   IFX_return_t ret;
   BOARD_t* pBoard;

   /* check input arguemnts */
   TD_PTR_CHECK(pCtrl,);
   TD_PTR_CHECK(pProgramArg,);

   /* Clean up qos sessions */
   if (pProgramArg->oArgFlags.nQos)
   {
      /* for all boards */
      for (j=0; j<pCtrl->nBoardCnt; j++)
      {
         /* get next board */
         pBoard = &pCtrl->rgoBoards[j];
         /* for all data channels */
         for (i = 0; i < pBoard->nMaxCoderCh; i++)
         {
            nDataFd = VOIP_GetFD_OfCh(i, pBoard);
            if (NO_DEVICE_FD != nDataFd)
            {
               /* stops the entire qos support, it deactivates and deletes all
                  sessions on all channels, according to description
                  in drv_tapi_qos_io.h */
               ret = TD_IOCTL(nDataFd, FIO_QOS_CLEAN, 0,
                        TD_DEV_NOT_SET, TD_CONN_ID_INIT);
               /* clean up on one channel should be sufficient */
               break;
            }
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                    ("Err, FIO_QOS_CLEAN %s failed to get FD of ch %d.\n"
                     "(File: %s, line: %d)\n",
                     pBoard->pszBoardName, i, __FILE__, __LINE__));
            }
         }
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("QoS: Device cleaned up\n"));
   }

} /* QOS_CleanUpSession() */


/**
   Check if QOS Session handles this connection.

   \param  pCtrl        - pointer to status control structure
   \param  pConn        - pointer to phone connection
   \param  nSeqConnId   - Seq Conn ID

   \return IFX_TRUE if QoS is handling data, otherwise return IFX_FALSE
*/
IFX_boolean_t QOS_HandleService(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn,
                                IFX_uint32_t nSeqConnId)
{
   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* In case of QoS support, packets are not to be handled by the application
      once a qos session was initiated. */
   if (pCtrl->pProgramArg->oArgFlags.nQos &&
       1 == pConn->oSession.nInSession)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Qos - UDP redirect will handle data instead of tapidemo.\n"));
      return IFX_TRUE;
   }

   return IFX_FALSE;
} /* QOS_HandleService() */


/**
   Starts qos session.

   \param pConn   - pointer to phone connection
   \param pPhone  - pointer to phone structure
   \param nAction - flag to setup port, address of session
   \param nDevIdx - device id (number)
*/
IFX_return_t QOS_StartSession(CONNECTION_t* pConn,
                              PHONE_t* pPhone,
                              TD_QOS_ACTION_t nAction,
                              IFX_int32_t nDevIdx)
{
   IFX_return_t ret = IFX_SUCCESS;
   CTRL_STATUS_t* pCtrl = IFX_NULL;

   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

   pCtrl = pPhone->pBoard->pCtrl;

   /* Remote call : start qos session */
   /* Also use QoS or udp redirect on local calls */
   /*   if ((EXTERN_VOIP_CALL == pConn->nType) */
   if ((PCM_CALL != pConn->nType) &&
       (pCtrl->pProgramArg->oArgFlags.nQos) &&
       (0 == pConn->oSession.nInSession))
   {
      if (pCtrl->pProgramArg->oArgFlags.nQosSocketStart)
      {
         return QOS_StartSessionOnSocket(pConn, pPhone);
      }
#ifdef TD_IPV6_SUPPORT
      if (IFX_TRUE == pConn->bIPv6_Call)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
              ("Err, Unable to make IPv6 connection with FIO_QOS_START. "
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
#endif /* TD_IPV6_SUPPORT */
      if (EXTERN_VOIP_CALL == pConn->nType)
      {
         /* Start qos session */
         if (TD_SET_ADDRESS_PORT == nAction)
         {
            /* For caller */
            pConn->oSession.oPair.srcPort = htons( VOICE_UDP_QOS_PORT +
                                                   pConn->nUsedCh * 2 +
                                                   nDevIdx);
            pConn->oSession.oPair.srcAddr =
               TD_SOCK_AddrIPv4Get(&pCtrl->pProgramArg->oMy_IP_Addr);
            pConn->oSession.oPair.destPort =
               htons(TD_SOCK_PortGet(&pConn->oConnPeer.oRemote.oToAddr));
            pConn->oSession.oPair.destAddr =
               TD_SOCK_AddrIPv4Get(&pConn->oConnPeer.oRemote.oToAddr);
         }
         else if (TD_SET_ONLY_PORT == nAction)
         {
            /* For called */
            pConn->oSession.oPair.destPort =
               htons(TD_SOCK_PortGet(&pConn->oConnPeer.oRemote.oToAddr));
         }
      }
      else /* Local call */
      {
         /* #warning local call with QoS will not work for all cases:
            - if not using default mapping 0:0 1:1 but 0:1 1:0,
            - for phones on dxt extension board for AR9 */
         /* set port numbers according to phone device and channel. */
         pConn->oSession.oPair.srcPort =
            htons(pConn->nUsedCh * 2 + nDevIdx * 100 + 1);
         pConn->oSession.oPair.destPort =
            htons(pConn->oConnPeer.oLocal.pPhone->nDataCh * 2
               + pConn->oConnPeer.oLocal.pPhone->pBoard->nBoard_IDX * 100 + 1);
         pConn->oSession.oPair.destAddr =
            htonl(TD_SOCK_AddrIPv4Get(&pCtrl->pProgramArg->oMy_IP_Addr));
         pConn->oSession.oPair.srcAddr =
            htonl(TD_SOCK_AddrIPv4Get(&pCtrl->pProgramArg->oMy_IP_Addr));
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
            ("Phone No %d: FIO_QOS_START --> src:0x%08X:%d, dst:0x%08X:%d\n",
             pPhone->nPhoneNumber,
            (int) pConn->oSession.oPair.srcAddr,
            (int) pConn->oSession.oPair.srcPort,
            (int) pConn->oSession.oPair.destAddr,
            (int) pConn->oSession.oPair.destPort));

      ret = TD_IOCTL(pConn->nUsedCh_FD, FIO_QOS_START, &pConn->oSession.oPair,
               TD_DEV_NOT_SET, pPhone->nSeqConnId);

      if ( IFX_SUCCESS == ret )
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
               ("Phone No %d: QoS started, data ch %d,"
                "src port: %d, dst port: %d\n",
                pPhone->nPhoneNumber,
                pConn->nUsedCh,
                pConn->oSession.oPair.srcPort,
                pConn->oSession.oPair.destPort));
      }
      pConn->oSession.nInSession = 1;
   }
   return ret;
} /* QOS_StartSession() */


/**
   Stops qos session.

   \param pConn   - pointer to phone connection
   \param pPhone  - pointer to phone structure
*/
IFX_return_t QOS_StopSession(CONNECTION_t* pConn,
                             PHONE_t* pPhone)
{

   IFX_return_t ret = IFX_SUCCESS;
   CTRL_STATUS_t* pCtrl = IFX_NULL;

   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);

   pCtrl = pPhone->pBoard->pCtrl;

   /* Remote call : stop qos session */
/* Also use QoS or udp redirect on local calls */
   if ((PCM_CALL != pConn->nType)
        && (pCtrl->pProgramArg->oArgFlags.nQos)
        && (1 == pConn->oSession.nInSession))
   {
      if (pCtrl->pProgramArg->oArgFlags.nQosSocketStart)
      {
         if (EXTERN_VOIP_CALL == pConn->nType)
         {
            /* Stop qos session */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                  ("Phone No %d: FIO_QOS_STOP:\n"
                   "%s-src:[%s]:%d,\n"
                   "%s-dst:[%s]:%d\n",
                   pPhone->nPhoneNumber,
                   pPhone->pBoard->pIndentPhone,
                   TD_GetStringIP(&pConn->oUsedAddr),
                   TD_SOCK_PortGet(&pConn->oUsedAddr),
                   pPhone->pBoard->pIndentPhone,
                   TD_GetStringIP(&pConn->oConnPeer.oRemote.oToAddr),
                   TD_SOCK_PortGet(&pConn->oConnPeer.oRemote.oToAddr)));
         }
      }
      else
      {
         /* Stop qos session */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
               ("Phone No %d: FIO_QOS_STOP: src:0x%08X:%d, dst:0x%08X:%d\n",
                pPhone->nPhoneNumber,
                (int) pConn->oSession.oPair.srcAddr,
                (int) pConn->oSession.oPair.srcPort,
                (int) pConn->oSession.oPair.destAddr,
                (int) pConn->oSession.oPair.destPort));
      }

      ret = TD_IOCTL(pConn->nUsedCh_FD, FIO_QOS_STOP, 0,
               TD_DEV_NOT_SET, pPhone->nSeqConnId);
      TD_RETURN_IF_ERROR(ret);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pPhone->nSeqConnId,
           ("Phone No %d: QoS stopped, data ch %d\n",
            pPhone->nPhoneNumber,
            pConn->nUsedCh));

      if (pCtrl->pProgramArg->oArgFlags.nQosSocketStart)
      {
         QOS_StopSessionOnSocket(pConn, pPhone);
      }

      pConn->oSession.nInSession = 0;
   }
   return ret;
} /* QOS_StopSession() */

/* ---------------------------------------------------------------------- */
#else /* QOS_SUPPORT */
/* ---------------------------------------------------------------------- */

IFX_void_t QOS_TurnServiceOn(PROGRAM_ARG_t *pProgramArg)
{
   TD_PTR_CHECK(pProgramArg,);

   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("QoS support not available\n"));

   pProgramArg->oArgFlags.nQos = 0;
}


IFX_void_t QOS_InitializePairStruct(CTRL_STATUS_t* pCtrl,
                                    CONNECTION_t* pConn,
                                    IFX_int32_t nDevIdx)
{
   /* empty function */
}


IFX_void_t QOS_CleanUpSession(CTRL_STATUS_t* pCtrl, PROGRAM_ARG_t *pProgramArg)
{
   /* empty function */
}


IFX_boolean_t QOS_HandleService(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn,
                                IFX_uint32_t nSeqConnId)
{
   /* Don't have QoS support so application can handle the data. */
   return IFX_FALSE;
}


IFX_return_t QOS_StartSession(CONNECTION_t* pConn,
                              PHONE_t* pPhone,
                              TD_QOS_ACTION_t nAction,
                              IFX_int32_t nDevIdx)
{
   /* empty function */
   return IFX_SUCCESS;
}


IFX_return_t QOS_StopSession(CONNECTION_t* pConn,
                             PHONE_t* pPhone)
{
   /* empty function */
   return IFX_SUCCESS;
}

#endif /* QOS_SUPPORT */

