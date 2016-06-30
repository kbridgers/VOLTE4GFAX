#ifndef _TD_SOCKET_H
#define _TD_SOCKET_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : td_socket.h
   Description : Functions for handling sockets and address structures
                 for IPv4 and IPv6.
 ****************************************************************************
   \file td_socket.h

   \remarks

   \note Changes:
*******************************************************************************/

#ifdef LINUX
#include "tapidemo_config.h"
#endif

#ifdef TD_IPV6_SUPPORT
   /** max length of IP representation */
   #define TD_ADDRSTRLEN INET6_ADDRSTRLEN
#else
   /** max length of IP representation */
   #define TD_ADDRSTRLEN INET_ADDRSTRLEN
#endif /* TD_IPV6_SUPPORT */

/** name of default interface name */
#define TD_DEFAULT_NET_IF_NAME "eth0"

/** max length of interface name */
#define TD_IFNMASIZ     16

extern IFX_char_t g_acIP_AddrStr[TD_ADDRSTRLEN];
IFX_char_t *TD_GetStringIP(TD_OS_sockAddr_t* pSocAddr);

#ifdef TD_IPV6_SUPPORT
IFX_int_t TD_SOCK_IPv6Create(
                  IFXOS_socketType_t socType,
                  TD_OS_socket_t     *pSocketFd);
IFX_int_t TD_IFXOS_MAP_SocketBind(
                  TD_OS_socket_t    socFd,
                  TD_OS_sockAddr_t  *pSocAddr);
IFX_int_t TD_IFXOS_MAP_SocketSendTo(TD_OS_socket_t socFd,
                                    IFX_char_t *pBuffer,
                                    IFX_int_t bufSize_byte,
                                    TD_OS_sockAddr_t *pSocAddr);
IFX_int_t TD_IFXOS_MAP_SocketRecvFrom(
                  TD_OS_socket_t socFd,
                  IFX_char_t     *pBuffer,
                  IFX_int_t      bufSize_byte,
                  TD_OS_sockAddr_t  *pSocAddr);
#endif /* TD_IPV6_SUPPORT */

IFX_boolean_t TD_SOCK_AddrCompare(TD_OS_sockAddr_t  *pSocAddrA,
                                  TD_OS_sockAddr_t  *pSocAddrB,
                                  IFX_boolean_t nTestPort);
IFX_return_t TD_SOCK_PortSet(TD_OS_sockAddr_t  *pSocAddr, IFX_int32_t nPort);
IFX_int32_t TD_SOCK_PortGet(TD_OS_sockAddr_t  *pSocAddr);
IFX_return_t TD_SOCK_AddrIPv4Set(TD_OS_sockAddr_t  *pSocAddr,
                                 IFX_uint32_t nAddrIPv4);
IFX_uint32_t TD_SOCK_AddrIPv4Get(TD_OS_sockAddr_t  *pSocAddr);
IFX_return_t TD_SOCK_FamilySet(TD_OS_sockAddr_t  *pSocAddr, IFX_int32_t nFamily);
IFX_int32_t TD_SOCK_FamilyGet(TD_OS_sockAddr_t  *pSocAddr);

#ifdef TD_IPV6_SUPPORT
IFX_return_t TD_SOCK_IPv6ScopeSet(TD_OS_sockAddr_t  *pSocAddr,
                                  IFX_uint32_t nScopeId);
IFX_uint32_t TD_SOCK_IPv6ScopeGet(TD_OS_sockAddr_t  *pSocAddr);
#endif /* TD_IPV6_SUPPORT */

IFX_return_t TD_SOCK_AddrCpy(TD_OS_sockAddr_t  *pSocAddrDst,
                              TD_OS_sockAddr_t  *pSocAddrSrc);
IFX_return_t TD_SOCK_AddrIPSet(TD_OS_sockAddr_t  *pSocAddr,
                               IFX_char_t  *pszAddr);

IFX_void_t* TD_SOCK_GetAddrIn(TD_OS_sockAddr_t *p);
#endif /* _TD_SOCKET_H */


