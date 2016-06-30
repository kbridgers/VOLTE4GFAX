#ifndef _TD_OSMAP_H
#define _TD_OSMAP_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file td_osmap.h
   This file contains the includes and the defines specific to the OS.
*/

#include "ifx_types.h"     /* ifx type definitions */
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#ifndef HAVE_IFX_ULONG_T
   #warning please update your ifx_types.h, using local definition of IFX_ulong_t
   /* unsigned long type - valid for 32bit systems only */
   typedef unsigned long               IFX_ulong_t;
   #define HAVE_IFX_ULONG_T
#endif /* HAVE_IFX_ULONG_T */

#ifndef HAVE_IFX_LONG_T
   #warning please update your ifx_types.h, using local definition of IFX_long_t
   /* long type - valid for 32bit systems only */
   typedef long                        IFX_long_t;
   #define HAVE_IFX_LONG_T
#endif /* HAVE_IFX_LONG_T */

#ifndef HAVE_IFX_INTPTR_T
   #warning please update your ifx_types.h, using local definition of IFX_intptr_t
   typedef IFX_long_t                  IFX_intptr_t;
   #define HAVE_IFX_INTPTR_T
#endif /* HAVE_IFX_INTPTR_T */

#ifndef HAVE_IFX_SIZE_T
   #warning please update your ifx_types.h, using local definition of IFX_size_t
   typedef IFX_ulong_t                 IFX_size_t;
   #define HAVE_IFX_SIZE_T
#endif /* HAVE_IFX_SIZE_T */

#ifndef HAVE_IFX_SSIZE_T
   #warning please update your ifx_types.h, using local definition of IFX_ssize_t
   typedef IFX_long_t                  IFX_ssize_t;
   #define HAVE_IFX_SSIZE_T
#endif /* HAVE_IFX_SSIZE_T */

#include "ifxos_memory_alloc.h"
#include "ifxos_copy_user_space.h"
#include "ifxos_event.h"
#include "ifxos_mutex.h"
#include "ifxos_lock.h"
#include "ifxos_time.h"
#include "ifxos_thread.h"
#include "ifxos_pipe.h"
#include "ifxos_print_io.h"
#include "ifxos_socket.h"
#include "ifx_fifo.h"  /* fifo (used for streaming) */
#include "ifxos_device_access.h"
#include "ifxos_debug.h"

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Name mapping tables           */
/* ============================= */

/*
   Mapping table - Kernel-space / User-space data exchange
*/
#define TD_OS_CpyKern2Usr               IFXOS_CpyToUser
#define TD_OS_CpyUsr2Kern               IFXOS_CpyFromUser

/*
   Mapping table - Mutex handling
*/
#define TD_OS_mutex_t                   IFXOS_mutex_t
#define TD_OS_MutexInit                 IFXOS_MutexInit
#define TD_OS_MutexDelete               IFXOS_MutexDelete
#define TD_OS_MutexGet                  IFXOS_MutexGet
#define TD_OS_MutexRelease              IFXOS_MutexRelease

/*
   Mapping table - Lock handling
*/
#define TD_OS_lock_t                    IFXOS_lock_t
#define TD_OS_LockInit                  IFXOS_LockInit
#define TD_OS_LockGet                   IFXOS_LockGet
#define TD_OS_LockRelease               IFXOS_LockRelease
#define TD_OS_LockDelete                IFXOS_LockDelete

/*
   Mapping table - Thread handling
*/
#define TD_OS_THREAD_PRIO_HIGH          IFXOS_THREAD_PRIO_HIGH
#define TD_OS_THREAD_PRIO_HIGHEST       IFXOS_THREAD_PRIO_HIGHEST
#define TD_OS_ThreadCtrl_t              IFXOS_ThreadCtrl_t
#define TD_OS_ThreadParams_t            IFXOS_ThreadParams_t
#define TD_OS_ThreadInit                IFXOS_ThreadInit
#define TD_OS_ThreadDelete              IFXOS_ThreadDelete
#define TD_OS_ThreadShutdown            IFXOS_ThreadShutdown
#define TD_OS_ThreadIdGet               IFXOS_ThreadIdGet
#define TD_OS_ProcessIdGet              IFXOS_ProcessIdGet

/*
   Mapping table - Pipe handling
*/
#ifdef VXWORKS
/* IFX-OS hasn't got implementation for pipe on vxWorks */
/* We need our own */
typedef FILE            TD_OS_Pipe_t;
#define TD_OS_PipeCreate                TD_IFXOS_MAP_PipeCreate
#define TD_OS_PipeOpen                  TD_IFXOS_MAP_PipeOpen
#define TD_OS_PipeClose                 TD_IFXOS_MAP_PipeClose
#define TD_OS_PipePrintf                TD_IFXOS_MAP_PipePrintf
#define TD_OS_PipeRead                  TD_IFXOS_MAP_PipeRead
#else /* VXWORKS */
#define TD_OS_Pipe_t                    IFXOS_Pipe_t
#define TD_OS_PipeCreate                IFXOS_PipeCreate
#define TD_OS_PipeOpen                  IFXOS_PipeOpen
#define TD_OS_PipeClose                 IFXOS_PipeClose
#define TD_OS_PipePrintf                IFXOS_PipePrintf
#define TD_OS_PipeRead                  IFXOS_PipeRead
#endif /* VXWORKS */

/*
   Mapping table - Socket handling
*/
#define TD_OS_SocketCleanup   IFXOS_SocketCleanup
#define TD_OS_SocketCreate    IFXOS_SocketCreate
#define TD_OS_SocketClose     IFXOS_SocketClose
#define TD_OS_SocketShutdown  IFXOS_SocketShutdown
#define TD_OS_SocketSelect    IFXOS_SocketSelect
#define TD_OS_SocketRecv      IFXOS_SocketRecv
/* #define TD_OS_SocketRecvFrom  IFXOS_SocketRecvFrom */
#define TD_OS_SocketSend      IFXOS_SocketSend
/* #define TD_OS_SocketSendTo    IFXOS_SocketSendTo */
/* #define TD_OS_SocketBind      IFXOS_SocketBind */
#define TD_OS_SocketListen    IFXOS_SocketListen
#define TD_OS_SocketAccept    IFXOS_SocketAccept
/* #define TD_OS_SocketConnect   IFXOS_SocketConnect */
#define TD_OS_SocketAton      IFXOS_SocketAton
#define TD_OS_SocFdSet        IFXOS_SocFdSet
#define TD_OS_SocFdClr        IFXOS_SocFdClr
#define TD_OS_SocFdIsSet      IFXOS_SocFdIsSet
#define TD_OS_SocFdZero       IFXOS_SocFdZero

#define TD_OS_SOC_TYPE_STREAM    IFXOS_SOC_TYPE_STREAM
#define TD_OS_SOC_TYPE_DGRAM     IFXOS_SOC_TYPE_DGRAM
#define TD_OS_socket_t           IFXOS_socket_t
/* #define TD_OS_sockAddr_t         IFXOS_sockAddr_t */
#define TD_OS_socFd_set_t        IFXOS_socFd_set_t
#define TD_OS_SOC_NO_WAIT        IFXOS_SOC_NO_WAIT
#define TD_OS_SOC_WAIT_FOREVER   IFXOS_SOC_WAIT_FOREVER

#ifdef TD_IPV6_SUPPORT

/* functions with separate implementation */
#define TD_OS_SocketRecvFrom  TD_IFXOS_MAP_SocketRecvFrom
#define TD_OS_SocketSendTo    TD_IFXOS_MAP_SocketSendTo
#define TD_OS_SocketBind      TD_IFXOS_MAP_SocketBind

#define TD_OS_SocketConnect(m_nSock, m_pAddr, m_nSize) \
   IFXOS_SocketConnect(m_nSock, TD_OS_IFXOS_ADDR_MAP m_pAddr, m_nSize)

#define TD_OS_sockAddr_t      struct sockaddr_storage

/** can be used for connect and IPv4 functions */
#define TD_OS_IFXOS_ADDR_MAP   (IFXOS_sockAddr_t*)

#else /* TD_IPV6_SUPPORT */
/* if no IPv6 support then use IFXOS functions */
#define TD_OS_SocketRecvFrom  IFXOS_SocketRecvFrom
#define TD_OS_SocketSendTo    IFXOS_SocketSendTo
#define TD_OS_SocketBind      IFXOS_SocketBind

#define TD_OS_SocketConnect   IFXOS_SocketConnect

#define TD_OS_sockAddr_t      IFXOS_sockAddr_t

#define TD_OS_IFXOS_ADDR_MAP

#endif /* TD_IPV6_SUPPORT */

   /*
   Mapping table - Device access handling
*/
#define TD_OS_DeviceOpen      IFXOS_DeviceOpen
#define TD_OS_DeviceClose     IFXOS_DeviceClose
#define TD_OS_DeviceWrite     IFXOS_DeviceWrite
#define TD_OS_DeviceRead      IFXOS_DeviceRead
#define TD_OS_DeviceControl   IFXOS_DeviceControl
#define TD_OS_DeviceSelect    IFXOS_DeviceSelect
#define TD_OS_DevFdSet        IFXOS_DevFdSet
#define TD_OS_DevFdIsSet      IFXOS_DevFdIsSet
#define TD_OS_DevFdZero       IFXOS_DevFdZero

#define TD_OS_devFd_set_t     IFXOS_devFd_set_t
#define TD_OS_NO_WAIT         IFXOS_NO_WAIT
#define TD_OS_WAIT_FOREVER    IFXOS_WAIT_FOREVER

/*
   Mapping table - File access handling
*/
#define TD_OS_FOpen        IFXOS_FOpen
#define TD_OS_FClose       IFXOS_FClose
#define TD_OS_FRead        IFXOS_FRead
#define TD_OS_FWrite       IFXOS_FWrite
#define TD_OS_FFlush       IFXOS_FFlush
#define TD_OS_FEof         IFXOS_FEof
#define TD_OS_Stat         IFXOS_Stat
#define TD_OS_FileLoad     IFXOS_FileLoad
#define TD_OS_FileWrite    IFXOS_FileWrite
#define TD_OS_FMemOpen     IFXOS_FMemOpen
#define TD_OS_FMemClose    IFXOS_FMemClose

#define TD_OS_File_t                IFXOS_File_t
#define TD_OS_stat_t                IFXOS_stat_t
#define TD_OS_STDERR                IFXOS_STDERR
#define TD_OS_STDOUT                IFXOS_STDOUT
#define TD_OS_STDIN                 IFXOS_STDIN
#define TD_OS_OPEN_MODE_READ        IFXOS_OPEN_MODE_READ
#define TD_OS_OPEN_MODE_READ_BIN    IFXOS_OPEN_MODE_READ_BIN
#define TD_OS_OPEN_MODE_WRITE       IFXOS_OPEN_MODE_WRITE
#define TD_OS_OPEN_MODE_WRITE_BIN   IFXOS_OPEN_MODE_WRITE_BIN
#define TD_OS_OPEN_MODE_APPEND      IFXOS_OPEN_MODE_APPEND
#define TD_OS_OPEN_MODE_READ_APPEND IFXOS_OPEN_MODE_READ_APPEND

/*
   Mapping table - Print IO handling
*/
#define TD_OS_GetChar                IFXOS_GetChar
#define TD_OS_PutChar                IFXOS_PutChar
#define TD_OS_FGets                  IFXOS_FGets
#define TD_OS_FPrintf                IFXOS_FPrintf
#define TD_OS_SNPrintf               IFXOS_SNPrintf
#define TD_OS_VSNPrintf              IFXOS_VSNPrintf

/*
   Mapping table - Memory Allocation (Application Space)
*/
#define TD_OS_MemAlloc   IFXOS_MemAlloc
#define TD_OS_MemFree    IFXOS_MemFree
#define TD_OS_MemCalloc  Common_MemCalloc

/*
   Mapping table - Time (Application Space)
*/
#define TD_OS_MSecSleep             IFXOS_MSecSleep
#define TD_OS_SecSleep              IFXOS_SecSleep
#define TD_OS_ElapsedTimeMSecGet    IFXOS_ElapsedTimeMSecGet
#define TD_OS_ElapsedTimeSecGet     IFXOS_ElapsedTimeSecGet
#define TD_OS_SysTimeGet            IFXOS_SysTimeGet

/*
   Mapping table - Debug (Application Space)
*/
#define TD_OS_PRN_USR_DBG_NL        IFXOS_PRN_USR_DBG_NL
#define TD_OS_PRN_USR_ERR_NL        IFXOS_PRN_USR_ERR_NL
#define TD_OS_PRN_USR_MODULE_DECL   IFXOS_PRN_USR_MODULE_DECL
#define TD_OS_PRN_USR_MODULE_CREATE IFXOS_PRN_USR_MODULE_CREATE
#define TD_OS_PRN_USR_LEVEL_SET     IFXOS_PRN_USR_LEVEL_SET
#define TD_OS_PRN_USR_LEVEL_GET     IFXOS_PRN_USR_LEVEL_GET
#define TD_OS_PRN_LEVEL_OFF        IFXOS_PRN_LEVEL_OFF
#define TD_OS_PRN_LEVEL_HIGH       IFXOS_PRN_LEVEL_HIGH
#define TD_OS_PRN_LEVEL_NORMAL     IFXOS_PRN_LEVEL_NORMAL
#define TD_OS_PRN_LEVEL_LOW        IFXOS_PRN_LEVEL_LOW


#endif /* _TD_OSMAP_H */
