/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_socket.c
   \date 2011-03-03
   \brief  Functions for handling sockets and address structures
           for IPv4 and IPv6.

   Functions for handling sockets and address structures for IPv4 and IPv6.
*/

#include "common.h"
#include "td_ifxos_map.h"
#include "itm/com_client.h"
#include "td_socket.h"

IFX_char_t g_acIP_AddrStr[TD_ADDRSTRLEN];

/**
   Get string representation of IP address.


   \param pSocAddr   - IP address structure

\return
   - pointer to string with address representation
*/
IFX_char_t *TD_GetStringIP(TD_OS_sockAddr_t* pSocAddr)
{
#ifdef TD_IPV6_SUPPORT
   const IFX_char_t *pRet;
   IFX_char_t acIP_AddrStr[TD_ADDRSTRLEN];
   IFX_char_t* pFormatString = IFX_NULL;
   IFX_void_t* pIP_Addr;
#endif /* TD_IPV6_SUPPORT */

   memset(g_acIP_AddrStr, 0, TD_ADDRSTRLEN * sizeof(IFX_char_t));
   strcpy(g_acIP_AddrStr, "address not set");

   TD_PTR_CHECK(pSocAddr, g_acIP_AddrStr);

#ifdef TD_IPV6_SUPPORT
   if (AF_INET == pSocAddr->ss_family)
   {
      pFormatString = "%s";
      pIP_Addr = &((struct sockaddr_in *) pSocAddr)->sin_addr;
   }
   else if (AF_INET6 == pSocAddr->ss_family)
   {
      //pFormatString = "[%s]";
      pFormatString = "%s";
      pIP_Addr = &((struct sockaddr_in6 *) pSocAddr)->sin6_addr;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, invalid address family %d. (File: %s, line: %d)\n",
             pSocAddr->ss_family, __FILE__, __LINE__));
      strcpy(g_acIP_AddrStr, "invalid address family");
      return g_acIP_AddrStr;
   }
   pRet = inet_ntop(pSocAddr->ss_family, pIP_Addr, acIP_AddrStr,
                    TD_ADDRSTRLEN);

   if (pRet != acIP_AddrStr)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, inet_ntop failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      strcpy(g_acIP_AddrStr, "inet_ntop failed");
   }
   else
   {
      snprintf(g_acIP_AddrStr, sizeof(g_acIP_AddrStr),
               pFormatString, acIP_AddrStr);
   }
#else /* TD_IPV6_SUPPORT */
   strncpy(g_acIP_AddrStr, inet_ntoa(pSocAddr->sin_addr),
           sizeof(g_acIP_AddrStr));
#endif /* TD_IPV6_SUPPORT */

   return g_acIP_AddrStr;
}

#ifdef TD_IPV6_SUPPORT
/**
   This function creates a TCP/IP, UDP/IP or raw socket for IPv6.

\param
   socType     specifies the type of the socket
               - IFXOS_SOC_TYPE_STREAM: TCP/IP socket
               - IFXOS_SOC_TYPE_DGRAM:  UDP/IP socket
\param
   pSocketFd   specifies the pointer where the value of the socket should be
               set. Value will be greater or equal zero

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR   if operation failed
*/
IFX_int_t TD_SOCK_IPv6Create(
                  IFXOS_socketType_t socType,
                  TD_OS_socket_t     *pSocketFd)
{
   TD_PTR_CHECK(pSocketFd, IFX_ERROR);
   /* arg3 = 0: do not specifiy the protocol */
   if((*pSocketFd = socket(AF_INET6, socType, 0)) == -1)
   {
      return IFX_ERROR;
   }
#ifdef EASY336
   {
/* #warning workaround:
added reuse for SVIP with IPv6 support, without it it didn't work */
      IFX_int32_t tr = 1;
      if (setsockopt(*pSocketFd, SOL_SOCKET, SO_REUSEADDR,
                     &tr, sizeof(tr)) == -1)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, setsockopt failed (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      }
   }
#endif /* EASY336 */
   return IFX_SUCCESS;
}

/**
   Assignes a local address to a TCP/IP, UDP/IP or raw socket.

\param
   socFd       specifies the socket should be bind to the address
               Value has to be greater or equal zero
\param
   pSocAddr    specifies a pointer to the addr structure

\return
   - IFX_SUCCESS in case of success
   - IFX_ERROR if operation failed
*/
IFX_int_t TD_IFXOS_MAP_SocketBind(
                  TD_OS_socket_t    socFd,
                  TD_OS_sockAddr_t  *pSocAddr)
{
   IFX_int_t ret;

   TD_PTR_CHECK(pSocAddr, IFX_ERROR);

   ret = bind(
            (int)socFd,
            (struct sockaddr*)pSocAddr,
            sizeof(TD_OS_sockAddr_t) );

   if (ret != 0)
   {
      return IFX_ERROR;
   }

   return IFX_SUCCESS;

}
/**
   Send data to specific address

\param
   socFd          specifies the socket. Value has to be greater or equal zero
\param
   pBuffer        specifies the pointer to a buffer where the data will be copied
\param
   bufSize_byte   specifies the size in byte of the buffer 'pBuffer'
\param
   pSocAddr    specifies a pointer to the TD_OS_sockAddr_t structure

\return
   Returns the number of received bytes. Returns a negative value if an error
   occured
*/
IFX_int_t TD_IFXOS_MAP_SocketSendTo(TD_OS_socket_t socFd,
                                    IFX_char_t *pBuffer,
                                    IFX_int_t bufSize_byte,
                                    TD_OS_sockAddr_t *pSocAddr)
{
   int ret;

   TD_PTR_CHECK(pBuffer, IFX_ERROR);
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);

   ret = (IFX_int_t)sendto((int)socFd, (const char*)pBuffer,
                       (int)bufSize_byte, 0,
                       (struct sockaddr *) pSocAddr, sizeof(TD_OS_sockAddr_t));

   return ret;
}

/**
   Receive data and get source IP addr.

\param
   socFd          specifies the socket. Value has to be greater or equal zero
\param
   pBuffer        specifies the pointer to a buffer where the data will be copied
\param
   bufSize_byte   specifies the size in byte of the buffer 'pBuffer'
\param
   pSocAddr    specifies a pointer to the TD_OS_sockAddr_t structure

\return
   Returns the number of received bytes. Returns a negative value if an error
   occured
*/
IFX_int_t TD_IFXOS_MAP_SocketRecvFrom(
                  TD_OS_socket_t socFd,
                  IFX_char_t     *pBuffer,
                  IFX_int_t      bufSize_byte,
                  TD_OS_sockAddr_t  *pSocAddr)
{
   int ret;
   unsigned int pFromlen = sizeof(TD_OS_sockAddr_t);

   TD_PTR_CHECK(pBuffer, IFX_ERROR);
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);

   ret = (IFX_int_t)recvfrom((int)socFd,
                             (char*)pBuffer, (int)bufSize_byte, 0,
                             (struct sockaddr *)pSocAddr, &pFromlen);

   return ret;
}
#endif /* TD_IPV6_SUPPORT */

/**
   Set port number.

   \param pSocAddrA  - IP address structure
   \param pSocAddrB  - IP address structure
   \param nTestPort  - IFX_TRUE if alos port number must be compared

   \return IFX_TRUE if addresses don't match, otherwise IFX_FALSE on success
*/
IFX_boolean_t TD_SOCK_AddrCompare(TD_OS_sockAddr_t  *pSocAddrA,
                                  TD_OS_sockAddr_t  *pSocAddrB,
                                  IFX_boolean_t nTestPort)
{
   /* check input parameters */
   TD_PTR_CHECK(pSocAddrA, IFX_FALSE);
   TD_PTR_CHECK(pSocAddrB, IFX_FALSE);

#ifdef TD_IPV6_SUPPORT
   /* must be the same family to match */
   if (pSocAddrA->ss_family != pSocAddrB->ss_family)
   {
      return IFX_FALSE;
   }
   /* check for IPv4, both use the same family so now checking only one */
   if (AF_INET == pSocAddrA->ss_family)
   {
      /* IP compare */
      if (((struct sockaddr_in *)pSocAddrA)->sin_addr.s_addr ==
           ((struct sockaddr_in *)pSocAddrB)->sin_addr.s_addr)
      {
         /* check ports if needed */
         if (IFX_TRUE == nTestPort)
         {
            if (((struct sockaddr_in *)pSocAddrA)->sin_port !=
                ((struct sockaddr_in *)pSocAddrB)->sin_port)
            {
               return IFX_FALSE;
            }
         } /* if (IFX_TRUE == nTestPort) */
         return IFX_TRUE;
      } /* IP compare */
      return IFX_FALSE;
   }
   /* check for IPv6, both use the same family so now checking only one */
   else if (AF_INET6 == pSocAddrA->ss_family)
   {
      /* IP compare */
      if (0 == memcmp(((struct sockaddr_in6 *)pSocAddrA)->sin6_addr.s6_addr,
                      ((struct sockaddr_in6 *)pSocAddrB)->sin6_addr.s6_addr,
                      sizeof(((struct sockaddr_in6 *)pSocAddrA)->sin6_addr)))
      {
         /* check ports if needed */
         if (IFX_TRUE == nTestPort)
         {
            if (((struct sockaddr_in6 *)pSocAddrA)->sin6_port !=
                ((struct sockaddr_in6 *)pSocAddrB)->sin6_port)
            {
               return IFX_FALSE;
            }
         } /* if (IFX_TRUE == nTestPort) */
         return IFX_TRUE;
      } /* IP compare */
      return IFX_FALSE;
   }
#else /* TD_IPV6_SUPPORT */
   /* IP compare */
   if (pSocAddrA->sin_addr.s_addr == pSocAddrB->sin_addr.s_addr)
   {
      /* check ports if needed */
      if (IFX_TRUE == nTestPort)
      {
         if (pSocAddrA->sin_port != pSocAddrB->sin_port)
         {
            return IFX_FALSE;
         }
      } /* if (IFX_TRUE == nTestPort) */
      return IFX_TRUE;
   } /* IP compare */
#endif /* TD_IPV6_SUPPORT */
   return IFX_FALSE;
}

/**
   Set port number.

   \param pSocAddr   - IP address structure
   \param nPort      - port number in host order

\return
   - IFX_SUCCESS on success
   - IFX_ERROR on failure
*/
IFX_return_t TD_SOCK_PortSet(TD_OS_sockAddr_t  *pSocAddr, IFX_int32_t nPort)
{
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);

#ifdef TD_IPV6_SUPPORT
   /* according to address family set port number in network order */
   if (AF_INET == pSocAddr->ss_family)
   {
      ((struct sockaddr_in *)pSocAddr)->sin_port = htons(nPort);
   }
   else if (AF_INET6 == pSocAddr->ss_family)
   {
      ((struct sockaddr_in6 *)pSocAddr)->sin6_port = htons(nPort);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, Invalid family %d. (File: %s, line: %d)\n",
             pSocAddr->ss_family, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#else /* TD_IPV6_SUPPORT */
   pSocAddr->sin_port = htons(nPort);
#endif /* TD_IPV6_SUPPORT */
   return IFX_SUCCESS;
}

/**
   Get port number in host order.

   \param pSocAddr   - IP address structure
   \param nPort      - port number in host order

\return
   - port number in host order
   - IFX_ERROR on failure
*/
IFX_int32_t TD_SOCK_PortGet(TD_OS_sockAddr_t  *pSocAddr)
{
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);

#ifdef TD_IPV6_SUPPORT
   if (AF_INET == pSocAddr->ss_family)
   {
      return ntohs(((struct sockaddr_in *)pSocAddr)->sin_port);
   }
   else if (AF_INET6 == pSocAddr->ss_family)
   {
      return ntohs(((struct sockaddr_in6 *)pSocAddr)->sin6_port);
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
         ("Err, Invalid family %d. (File: %s, line: %d)\n",
          pSocAddr->ss_family, __FILE__, __LINE__));
   return IFX_ERROR;
#else /* TD_IPV6_SUPPORT */
   return ntohs(pSocAddr->sin_port);
#endif /* TD_IPV6_SUPPORT */
}

/**
   Set IPv4 address.

   \param pSocAddr   - IP address structure
   \param nAddrIPv4  - IP address in host order

\return
   - IFX_SUCCESS on success
   - IFX_ERROR on failure
*/
IFX_return_t TD_SOCK_AddrIPv4Set(TD_OS_sockAddr_t  *pSocAddr,
                                 IFX_uint32_t nAddrIPv4)
{
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);

#ifdef TD_IPV6_SUPPORT
   if (AF_INET == pSocAddr->ss_family)
   {
      ((struct sockaddr_in *)pSocAddr)->sin_addr.s_addr = htonl(nAddrIPv4);
      return IFX_SUCCESS;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
         ("Err, Invalid family %d. (File: %s, line: %d)\n",
          pSocAddr->ss_family, __FILE__, __LINE__));
   return IFX_ERROR;
#else /* TD_IPV6_SUPPORT */
   pSocAddr->sin_addr.s_addr = htonl(nAddrIPv4);
   return IFX_SUCCESS;
#endif /* TD_IPV6_SUPPORT */
}

/**
   Get IPv4 address.

   \param pSocAddr   - IP address structure

\return
   - IP address in host order
   - 0 on failure
*/
IFX_uint32_t TD_SOCK_AddrIPv4Get(TD_OS_sockAddr_t  *pSocAddr)
{
   TD_PTR_CHECK(pSocAddr, 0);

#ifdef TD_IPV6_SUPPORT
   if (AF_INET == pSocAddr->ss_family)
   {
      return ntohl(((struct sockaddr_in *)pSocAddr)->sin_addr.s_addr);
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
         ("Err, Invalid family %d. (File: %s, line: %d)\n",
          pSocAddr->ss_family, __FILE__, __LINE__));
   return 0;
#else /* TD_IPV6_SUPPORT */
   return ntohl(pSocAddr->sin_addr.s_addr);
#endif /* TD_IPV6_SUPPORT */
}

/**
   Set address family ID.

   \param pSocAddr   - IP address structure
   \param nFamily    - family ID number

\return
   - IFX_SUCCESS on success
   - IFX_ERROR on failure
*/
IFX_return_t TD_SOCK_FamilySet(TD_OS_sockAddr_t  *pSocAddr, IFX_int32_t nFamily)
{
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);

#ifdef TD_IPV6_SUPPORT
   pSocAddr->ss_family = nFamily;
#else /* TD_IPV6_SUPPORT */
   pSocAddr->sin_family = nFamily;
#endif /* TD_IPV6_SUPPORT */

   return IFX_SUCCESS;
}

/**
   Get addres family ID.

   \param pSocAddr   - IP address structure

\return
   - family address ID number
   - IFX_ERROR on failure
*/
IFX_int32_t TD_SOCK_FamilyGet(TD_OS_sockAddr_t  *pSocAddr)
{
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);

#ifdef TD_IPV6_SUPPORT
   return pSocAddr->ss_family;
#else /* TD_IPV6_SUPPORT */
   return pSocAddr->sin_family;
#endif /* TD_IPV6_SUPPORT */
}

#ifdef TD_IPV6_SUPPORT
/**
   Set scope ID.

   \param pSocAddr   - IP address structure
   \param nScopeId   - scope ID number

\return
   - IFX_SUCCESS on success
   - IFX_ERROR on failure
*/
IFX_return_t TD_SOCK_IPv6ScopeSet(TD_OS_sockAddr_t  *pSocAddr,
                                  IFX_uint32_t nScopeId)
{
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);
   TD_PARAMETER_CHECK((AF_INET6 != pSocAddr->ss_family),
                      pSocAddr->ss_family, IFX_ERROR);

   ((struct sockaddr_in6 *)pSocAddr)->sin6_scope_id = nScopeId;

   return IFX_SUCCESS;
}

/**
   Get scope ID.

   \param pSocAddr   - IP address structure

\return
   - scope ID number
   - IFX_ERROR on failure
*/
IFX_uint32_t TD_SOCK_IPv6ScopeGet(TD_OS_sockAddr_t  *pSocAddr)
{
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);
   TD_PARAMETER_CHECK((AF_INET6 != pSocAddr->ss_family),
                      pSocAddr->ss_family, IFX_ERROR);

   return ((struct sockaddr_in6 *)pSocAddr)->sin6_scope_id;
}
#endif /* TD_IPV6_SUPPORT */

/**
   Copy IP address structure.

   \param pSocAddrDst   - IP address structure to copy to
   \param pSocAddrSrc   - IP address structure to copy from

\return
   - IFX_SUCCESS on success
   - IFX_ERROR on failure
*/
IFX_return_t TD_SOCK_AddrCpy(TD_OS_sockAddr_t  *pSocAddrDst,
                             TD_OS_sockAddr_t  *pSocAddrSrc)
{
   TD_PTR_CHECK(pSocAddrDst, IFX_ERROR);
   TD_PTR_CHECK(pSocAddrSrc, IFX_ERROR);

   memcpy(pSocAddrDst, pSocAddrSrc, sizeof(TD_OS_sockAddr_t));

   return IFX_SUCCESS;
}

/**
   Set IP address in address structure
   according to string representation of IP address.

   \param pSocAddr   - IP address structure
   \param pszAddr    - string with IP address

\return
   - IFX_SUCCESS on success
   - IFX_ERROR on failure
*/
IFX_return_t TD_SOCK_AddrIPSet(TD_OS_sockAddr_t  *pSocAddr,
                               IFX_char_t  *pszAddr)
{
   /* check input parameters */
   TD_PTR_CHECK(pSocAddr, IFX_ERROR);
   TD_PTR_CHECK(pszAddr, IFX_ERROR);
#ifdef TD_IPV6_SUPPORT
   IFX_int32_t nRet;

   if (AF_INET == pSocAddr->ss_family)
   {
      nRet = inet_pton(AF_INET, pszAddr,
                       &((struct sockaddr_in *)pSocAddr)->sin_addr);
   }
   else if (AF_INET6 == pSocAddr->ss_family)
   {
      nRet = inet_pton(AF_INET6, pszAddr,
                       &((struct sockaddr_in6 *)pSocAddr)->sin6_addr);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, Invalid addr family %d. (File: %s, line: %d)\n",
             pSocAddr->ss_family, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (0 > nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, inet_pton failed family %s - family %d str %s,"
             " (File: %s, line: %d)\n",
             strerror(errno), pSocAddr->ss_family, pszAddr,
             __FILE__, __LINE__));
   }
   else
   {
      return IFX_SUCCESS;
   }
#else /* TD_IPV6_SUPPORT */
   if (0 != inet_aton(pszAddr, &((struct sockaddr_in *)pSocAddr)->sin_addr))
   {
      return IFX_SUCCESS;
   }
#endif /* TD_IPV6_SUPPORT */
   return IFX_ERROR;
}
/**
   Get sockaddr_in or sockaddr_in6 from sockaddr.

   \param addr - pointer to struct sockaddr

   \return sockaddr_in or sockaddr_in6
*/
IFX_void_t* TD_SOCK_GetAddrIn(TD_OS_sockAddr_t *addr)
{
   TD_PTR_CHECK(addr, IFX_NULL);

#ifdef TD_IPV6_SUPPORT
   if(addr->ss_family == AF_INET)
   {
      return &(((struct sockaddr_in*)addr)->sin_addr);
   }
   else if(addr->ss_family == AF_INET6)
   {
      return &(((struct sockaddr_in6*)addr)->sin6_addr);
   }
#else
   return &addr->sin_addr;
#endif /* TD_IPV6_SUPPORT */

   return IFX_NULL;
} /* Common_GetAddrIn */


