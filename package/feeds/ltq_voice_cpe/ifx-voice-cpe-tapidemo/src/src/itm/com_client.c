/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file com_client.c
   \date 2007-05-10
   \brief Implementation of communication module with control PC.

*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "com_client.h"

/* ============================= */
/* Global Structures             */
/* ============================= */

/** used for simplifing TD_TRACE macro */
IFX_boolean_t g_bITM_Print = IFX_FALSE;

/** global pointer to ITM control structure */
TD_ITM_CONTROL_t* g_pITM_Ctrl;

/** global buffer for trace messages */
IFX_char_t g_buf[TD_COM_BUF_SIZE];

/** global buffer for trace messages after adding conn ID info */
IFX_char_t g_buf_parsed[TD_COM_BUF_SIZE];

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

#ifdef LINUX

/**
   Send message through communication socket

   \param nSocket  - handler to communication socket
   \param msg      - pointer to the message string
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_SendMessage(TD_OS_socket_t nSocket, IFX_char_t* msg,
                             IFX_uint32_t nSeqConnId)
{
   IFX_int32_t size = 0;
   IFX_int32_t ret = 0;

   /* check input parameters */
   TD_PTR_CHECK(msg, IFX_ERROR);

   if (NO_SOCKET == nSocket)
   {
      /* no connection to TCL script do not print trace */
      return IFX_ERROR;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId, ("COM_SendMessage()\n"));

   size = strlen(msg);
   TD_PARAMETER_CHECK((0 > size), size, IFX_ERROR);

   ret = TD_OS_SocketSend(nSocket, msg, size);
   if (ret != size)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("Err, No data sent through communication socket,"
             "%s. (File: %s, line: %d)\n",
            strerror(errno), __FILE__, __LINE__));
      TD_OS_SocketClose(nSocket);
      g_pITM_Ctrl->nComOutSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}/* COM_SendMessage() */

/**
   Initialize TCP socket used for communication with Control PC

   \param unIp   - IP address
   \param direction   - determines the direction of socket (in/out)

   \return socket number or NO_SOCKET if error
*/
TD_OS_socket_t COM_InitCommunication(IFX_uint32_t unIp,
                                     IFX_boolean_t direction)
{
   TD_OS_socket_t socFd;
   TD_OS_sockAddr_t my_addr, serv_addr;

   if (COM_DIRECTION_OUT == direction)
   {
      /* Messages sent to the Control PC use TCP*/
      if (IFX_SUCCESS != TD_OS_SocketCreate(TD_OS_SOC_TYPE_STREAM, &socFd))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Can't create communication socket, %s."
                "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
         return NO_SOCKET;
      }
   }
   else
   {
      /* Messages received from the Control PC use UDP*/
      if (IFX_SUCCESS != TD_OS_SocketCreate(TD_OS_SOC_TYPE_DGRAM, &socFd))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Can't create communication socket, %s."
                "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
         return NO_SOCKET;
      }
   }


   if (COM_DIRECTION_IN == direction)
   {
      bzero((IFX_char_t*) &my_addr, (IFX_int32_t) sizeof(my_addr));
      TD_SOCK_FamilySet(&my_addr, AF_INET);
      TD_SOCK_PortSet(&my_addr, g_pITM_Ctrl->nComInPort);
      TD_SOCK_AddrIPv4Set(&my_addr, unIp ? unIp : htonl(INADDR_ANY));

      if (IFX_SUCCESS != TD_OS_SocketBind(socFd, &my_addr))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
               ("Err, ITM: Can't bind communication socket to port, %s. "
                "(File: %s, line: %d)\n",
               strerror(errno), __FILE__, __LINE__));
         return NO_SOCKET;
      }
   }

#ifdef LINUX
   /* make the socket non blocking */
   fcntl(socFd, F_SETFL, O_NONBLOCK);
#endif /* LINUX */

   if (COM_DIRECTION_OUT == direction)
   {
      bzero((IFX_char_t*) &serv_addr, (IFX_int32_t) sizeof(serv_addr));
      TD_SOCK_FamilySet(&serv_addr, AF_INET);
      TD_SOCK_PortSet(&serv_addr, g_pITM_Ctrl->nComOutPort);
      TD_SOCK_AddrIPv4Set(&serv_addr, unIp);

      /* We can use TD_OS_SocketConnect, but there will be error
         "Operation now in progress" from IFX-OS" */
      if (IFX_SUCCESS != TD_OS_SocketConnect(socFd, &serv_addr,
                                             sizeof(serv_addr)))
      {
         /* Socket is non blocking so ignore "Operation now in progress"*/
         if (EINPROGRESS != errno)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Can't connect with server, %s. (File: %s, line: %d)\n",
                   strerror(errno), __FILE__, __LINE__));
            return NO_SOCKET;
         }
      }
   }

   return socFd;
} /* COM_InitCommunicationSocket() */

/**
   Get broadcast address for specified Network Interface name.

   \param pIfName    - pointer string with Network Interface name

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_BrCastAddrSet(IFX_char_t* pIfName)
{
   struct ifreq oIfr;
   struct sockaddr_in *pSockAddr;
   TD_OS_socket_t nSock;

   TD_PTR_CHECK(pIfName, IFX_ERROR);

   memset(&oIfr, 0, sizeof(struct ifreq));

   /* open socket */
   nSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
   if (-1 == nSock)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err: ITM: socket(), %s. (File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* copy interface name */
   strcpy(oIfr.ifr_name, pIfName);
   /* Get the broadcast address */
   if(0 > ioctl(nSock, SIOCGIFBRDADDR, &oIfr))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: ioctl(SIOCGIFBRDADDR) failed %s. (File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      close(nSock);
      return IFX_ERROR;
   }
   /* socket no longer needed */
   close(nSock);
   /* set pointer to address structure */
   pSockAddr = (struct sockaddr_in*) &oIfr.ifr_broadaddr;

   /* clear structure and save broadcast address */
   memset(&g_pITM_Ctrl->oBroadCast.oBrCastAddr, 0,
          sizeof(g_pITM_Ctrl->oBroadCast.oBrCastAddr));
   memcpy(&g_pITM_Ctrl->oBroadCast.oBrCastAddr,
          pSockAddr, sizeof(struct sockaddr_in));

   return IFX_SUCCESS;
}

/**
   Initialize TCP socket used for receiving/sending broadcast message
   from control PC.

   \param none

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_BrCastSocketOpen(IFX_void_t)
{
   IFX_int32_t nOn = 1;

   /* check if socket already created */
   if (NO_SOCKET != g_pITM_Ctrl->oBroadCast.nSocket)
   {
      TD_OS_SocketClose(g_pITM_Ctrl->oBroadCast.nSocket);
      g_pITM_Ctrl->oBroadCast.nSocket = NO_SOCKET;
   }
   /* Messages received from the Control PC use UDP*/
   if (IFX_SUCCESS != TD_OS_SocketCreate(TD_OS_SOC_TYPE_DGRAM,
                         &g_pITM_Ctrl->oBroadCast.nSocket))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Can't create communication socket, %s."
             "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
      g_pITM_Ctrl->oBroadCast.nSocket = NO_SOCKET;
      return IFX_ERROR;
   }
   /* enable broadcast */
   if (setsockopt(g_pITM_Ctrl->oBroadCast.nSocket, SOL_SOCKET, SO_BROADCAST,
                  &nOn, sizeof(nOn)) < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: setsockopt(socFd, SOL_SOCKET, SO_BROADCAST, ..) "
             "failed - %s.\n"
             "(File: %s, line: %d)\n", strerror(errno), __FILE__, __LINE__));
      TD_OS_SocketClose(g_pITM_Ctrl->oBroadCast.nSocket);
      g_pITM_Ctrl->oBroadCast.nSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   /* set family and port */
   TD_SOCK_FamilySet(&g_pITM_Ctrl->oBroadCast.oBrCastAddr, AF_INET);
   TD_SOCK_PortSet(&g_pITM_Ctrl->oBroadCast.oBrCastAddr, COM_BROADCAST_PORT);

   /* bind socket */
   if (IFX_SUCCESS !=
       TD_OS_SocketBind(g_pITM_Ctrl->oBroadCast.nSocket,
                        &g_pITM_Ctrl->oBroadCast.oBrCastAddr))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
            ("Err, ITM: Can't bind communication socket to port, %s. "
             "(File: %s, line: %d)\n",
            strerror(errno), __FILE__, __LINE__));
      TD_OS_SocketClose(g_pITM_Ctrl->oBroadCast.nSocket);
      g_pITM_Ctrl->oBroadCast.nSocket = NO_SOCKET;
      return IFX_ERROR;
   }

   /* make the socket non blocking */
   fcntl(g_pITM_Ctrl->oBroadCast.nSocket, F_SETFL, O_NONBLOCK);

   return IFX_SUCCESS;
}

/**
   Reply to control PC to let it know board IP address.

   \param pCtrl   - pointer to control structure
   \param pAddrFrom  - pointer to structure holding control PC address

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t COM_BrCastResponseSend(CTRL_STATUS_t* pCtrl,
                                    TD_OS_sockAddr_t *pAddrFrom)
{
   IFX_char_t aBuf[MAX_ITM_MESSAGE_TO_SEND];
   BOARD_t* pBoard;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pAddrFrom, IFX_ERROR);

   /* get main board */
   pBoard = TD_GET_MAIN_BOARD(pCtrl);

   /* set message string */
   sprintf(aBuf,
           "AUTODETECTION_RESPONSE:{ "
             " BOARD_NAME {%s} "
             " BOARD_TYPE {%s} "
             " IP_ADDR {%s} "
             " CONNECTED {%s} "
           "}%s",
           pBoard->pszBoardName,
           Common_Enum2Name(pBoard->nType, TD_rgBoardTypeName),
           TD_GetStringIP(&pCtrl->pProgramArg->oMy_IP_Addr),
           (NO_SOCKET == g_pITM_Ctrl->nComOutSocket) ? "NO" : "YES",
           COM_MESSAGE_ENDING_SEQUENCE);

   /* send message */
   if (0 > TD_OS_SocketSendTo(g_pITM_Ctrl->oBroadCast.nSocket,
                              aBuf, strlen(aBuf), pAddrFrom))
   {
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Change communication socket and set rfds array.

   \param pctrl   - pointer to control structure
   \param pRFds   - pointer to fd set

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t COM_ResetCommunicationSockets(CTRL_STATUS_t* pCtrl,
                                           TD_OS_socFd_set_t* pRFds)
{

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pRFds, IFX_ERROR);

   /* if request to change IP address via TCL scripts*/
   if (IFX_TRUE == g_pITM_Ctrl->bCommunicationReset)
   {
      if (IFX_TRUE == g_pITM_Ctrl->bChangeIPAddress)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
               ("Address of board has been changed.\n"));

         TD_OS_SocFdClr(pCtrl->nAdminSocket, pRFds);
         TD_OS_SocketClose(pCtrl->nAdminSocket);
         pCtrl->nAdminSocket = NO_SOCKET;
         /* save new IP address */
         TD_SOCK_AddrIPv4Set(&pCtrl->oTapidemo_IP_Addr,
                             inet_addr(g_pITM_Ctrl->oVerifySytemInit.aParametrs));
         pCtrl->nMy_IPv4_Addr =
            inet_addr(g_pITM_Ctrl->oVerifySytemInit.aParametrs) & 0xffffff00;
         TD_SOCK_AddrIPv4Set(&pCtrl->pProgramArg->oMy_IP_Addr,
                             inet_addr(g_pITM_Ctrl->oVerifySytemInit.aParametrs));
         /* get administartion socket */
         pCtrl->nAdminSocket = TAPIDEMO_InitAdminSocket();

         if (pCtrl->nAdminSocket == NO_SOCKET)
         {
            /* Without this socket we cannot make calls. */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ITM,
                  ("Err, ITM: Could not Reset Admin Communication Socket. "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            return IFX_ERROR;
         }
         /* Also add administration socket for handling calls */
         TD_SOC_FD_SET (pCtrl->nAdminSocket, pRFds, pCtrl->nMaxFdSocket);
      }

      TD_OS_SocFdClr(g_pITM_Ctrl->nComInSocket, pRFds);
      TD_OS_SocFdClr(g_pITM_Ctrl->oBroadCast.nSocket, pRFds);
      COM_CleanUpComSockets();
      COM_COMMUNICATION_INIT(pCtrl->pProgramArg->aNetInterfaceName,
                             pCtrl->pProgramArg->oMy_IP_Addr, TD_CONN_ID_ITM);


      if(NO_SOCKET != g_pITM_Ctrl->nComInSocket)
      {
         TD_SOC_FD_SET (g_pITM_Ctrl->nComInSocket, pRFds,
                        pCtrl->nMaxFdSocket);
      }

      if(NO_SOCKET != g_pITM_Ctrl->oBroadCast.nSocket)
      {
         TD_SOC_FD_SET (g_pITM_Ctrl->oBroadCast.nSocket,
                        pRFds, pCtrl->nMaxFdSocket);
      }
      /* communication reset finished */
      g_pITM_Ctrl->bCommunicationReset = IFX_FALSE;
      g_pITM_Ctrl->bChangeIPAddress = IFX_FALSE;
   } /* if (IFX_TRUE == pCtrl->bCommunicationReset) */
   return IFX_SUCCESS;
} /* */



/**
   Close communication sockets

   \param none

   \return IFX_void_t
*/
IFX_void_t COM_CleanUpComSockets(IFX_void_t)
{
   if (IFX_NULL == g_pITM_Ctrl)
   {
      return;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
         ("ITM: Close communication sockets.\n"));

   if (NO_SOCKET != g_pITM_Ctrl->nComInSocket)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("ITM: Closing command socket\n"));
      TD_OS_SocketClose(g_pITM_Ctrl->nComInSocket);
      g_pITM_Ctrl->nComInSocket = NO_SOCKET;
   }
   if (NO_SOCKET != g_pITM_Ctrl->oBroadCast.nSocket)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("ITM: Closing BroadCast socket\n"));
      TD_OS_SocketClose(g_pITM_Ctrl->oBroadCast.nSocket);
      g_pITM_Ctrl->oBroadCast.nSocket = NO_SOCKET;
   }
   if (NO_SOCKET != g_pITM_Ctrl->nComOutSocket)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
            ("ITM: Closing connection with Control PC\n"));
      TD_OS_SocketClose(g_pITM_Ctrl->nComOutSocket);
      g_pITM_Ctrl->nComOutSocket = NO_SOCKET;
   }
   return;
}/*  */

#endif /* LINUX */

