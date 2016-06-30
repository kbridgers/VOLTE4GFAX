#ifndef _COMMON_H
#define _COMMON_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : common.h
   Description :
*******************************************************************************/
/**
   \file common.c
   \date 2005-10-01
   \brief Common for tapidemo enums, structure, macros and defines.
*/
/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef LINUX
#include "tapidemo_config.h"
#ifdef EASY336
#include <linux/autoconf.h>
#endif /* EASY336 */
#endif /* LINUX */

/** system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LINUX
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
   #include <netdb.h>
#include <sys/types.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pwd.h>
#include <ctype.h>
   #include <net/if.h>
#include <syslog.h>
#endif /* LINUX */

#ifdef VXWORKS
   #include <vxWorks.h>
   #include <sockLib.h>
   #include <inetLib.h>
   #include <ioLib.h>
   #include <sys/ioctl.h>
   #include <sysLib.h>
   #include <tickLib.h>
   #include <taskLib.h>
   #include <hostLib.h>
   #include <time.h>
   #include <taskHookLib.h>
   #include <sigLib.h>
   #include <inetLib.h>
#ifdef EASY336
   #include <natProtocol.h>
#endif
#endif /* VXWORKS */

#include "ifx_types.h"
#include "td_osmap.h"

#ifdef DANUBE
/* Compilation flag OLD_BSP is used for BSP which is compiled by LXDB */
   #ifdef OLD_BSP
      #include "asm/danube/port.h"
   #else
      #include "asm/ifx/ifx_gpio.h"
   #endif
#endif /* DANUBE */

#ifdef TD_DECT

/* Workaround - so IFX_TRUE and IFX_FALSE will not be defined for the second
   time in file ifx_types.h and ifx_common_defs.h. Workaround should be used
   until both ifx_types.h and ifx_common_defs.h must be used */
#define IFX_TRUE
#define IFX_FALSE
#include "ifx_common_defs.h"
#undef IFX_TRUE
#undef IFX_FALSE

#include "IFX_DECT_Stack.h"
#include "IFX_DECT_CSU.h"

#endif /* TD_DECT */

#ifdef HAVE_DRV_TAPI_HEADERS
   #include "drv_tapi_io.h"
#else
    #error "drv_tapi is missing"
#endif

#ifdef TAPI_VERSION4
   #include "ifxos_debug.h"
#endif

#ifdef QOS_SUPPORT
/* QOS_INIT_SESSION defined in drv_qos.h */
    #ifdef HAVE_DRV_TAPI_HEADERS
       #include "drv_tapi_qos_io.h"
    #else
        #error "drv_tapi is missing"
    #endif
#endif /* QOS_SUPPORT */

#ifdef HAVE_DRV_TAPI_HEADERS
   #include "drv_tapi_event_io.h"
#else
    #error "drv_tapi is missing"
#endif

#ifdef LINUX
#ifdef EASY336
#if defined(CONFIG_LTQ_SVIP_NAT) || defined(CONFIG_IFX_SVIP_NAT)
#include <linux/ifx_svip_nat_io.h>
#endif /* defined(CONFIG_LTQ_SVIP_NAT) || defined(CONFIG_IFX_SVIP_NAT) */
#ifdef CONFIG_LTQ_SVIP_NAT_PT
#include <linux/ltq_svip_nat_pt_io.h>
#endif /* CONFIG_LTQ_SVIP_NAT_PT */
#endif /* EASY336 */
#endif /* LINUX */

#ifdef EASY336
#include "lib_svip_rm.h"
#endif

#include "td_socket.h"
#ifdef EVENT_LOGGER_DEBUG
#include "el_ioctl.h"
#endif /* EVENT_LOGGER_DEBUG */

/* early VxWorks versions did not have snprintf */
#if defined(VXWORKS) && defined(NO_SNPRINTF)
#define snprintf(res, n, args...) sprintf(res, ##args)
#endif /* VXWORKS && NO_SNPRINTF */

extern size_t strnlen (__const char *__string, size_t __maxlen);

#ifdef LINUX
extern int snprintf(char *str, size_t size, const char *format, ...);
extern int gethostname(char *name, size_t len);
#endif /* LINUX */

/* ============================= */
/* Debug interface               */
/* ============================= */

/** Trace redirection setting. */
typedef enum _TRACE_REDIRECTION_t
{
   /** No redirection */
   TRACE_REDIRECTION_NONE = 0,
   /** Redirection to file */
   TRACE_REDIRECTION_FILE = 1,
   /** Redirection to local syslog */
   TRACE_REDIRECTION_SYSLOG_LOCAL = 2,
   /** Redirection to remote syslog */
   TRACE_REDIRECTION_SYSLOG_REMOTE = 3
} TRACE_REDIRECTION_t;

/** Trace levels */
typedef enum _DEBUG_LEVEL_t
{
   /** None reporting */
   DBG_LEVEL_OFF = TD_OS_PRN_LEVEL_OFF,
   /** Error reporting */
   DBG_LEVEL_HIGH = TD_OS_PRN_LEVEL_HIGH,
   /** Status reporting */
   DBG_LEVEL_NORMAL = TD_OS_PRN_LEVEL_NORMAL,
   /** More info reporting */
   DBG_LEVEL_LOW = TD_OS_PRN_LEVEL_LOW
} DEBUG_LEVEL_t;

/**
   Stores the trace string in temporal variable.

   \param ...     - variable number of arguments

   \return

   \remark
      Macro is used to remove parenthesis from TRACE macro's message argument
      Used only for sending Trace messages through communication socket.
*/
#ifdef VXWORKS
#define COM_GENERATE_STRING(ARGS...) \
   snprintf(g_buf, TD_COM_BUF_SIZE - 1, ## ARGS)
#else /* VXWORKS */
#define COM_GENERATE_STRING(...) \
   snprintf(g_buf, TD_COM_BUF_SIZE - 1, __VA_ARGS__)
#endif /* VXWORKS */

#define TD_CONN_ID_INIT       0x00FFFF00
#define TD_CONN_ID_EVT        0x00FFFF01
#define TD_CONN_ID_ERR        0x00FFFF02
#define TD_CONN_ID_ITM        0x00FFFF03
#define TD_CONN_ID_T38_TEST   0x00FFFF04
#define TD_CONN_ID_PHONE_BOOK 0x00FFFF06
#define TD_CONN_ID_MSG        0x00FFFF07
#define TD_CONN_ID_DBG        0x00FFFF08
#define TD_CONN_ID_HELP       0x00FFFF09
#define TD_CONN_ID_NOT_SET    0x00FFFF0A
#define TD_CONN_ID_DECT       0x00FFFF0B

#define TD_DEV_NOT_SET        0

/** configure device to use polling mode */
#define TD_USE_POLLING_MODE   -1

#define TD_LOW_LEVEL_ERR_FORMAT_STR    "          - LL error description: %s\n"

#define TD_USE_CONN_ID_IF_SET(m_nConnID_Default, m_nConnID_Alt) \
   ( (TD_CONN_ID_INIT == (m_nConnID_Default)) ? \
   (m_nConnID_Alt) : (m_nConnID_Default))

/** used for TD_TRACE */
extern IFX_boolean_t g_bITM_Print;

#define TD_TRACE(m_name, m_level, m_ConnID, m_message) \
      do \
      { \
         g_bITM_Print = IFX_FALSE; \
         if ((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
             (m_level >= g_pITM_Ctrl->nComDbgLevel) && \
             (COM_MODULAR_TEST == g_pITM_Ctrl->nTestType) && \
             (NO_SOCKET != g_pITM_Ctrl->nComOutSocket)) { \
            /* do this complicated check only once */ \
            g_bITM_Print = IFX_TRUE; \
         } else if ((m_level) >= TD_OS_PRN_USR_LEVEL_GET(m_name)) { \
         } else { \
            /* do nothing more */ \
            break; \
         } \
         /* generate string for easier handling */  \
         COM_GENERATE_STRING m_message; \
         ABSTRACT_ParseTrace(m_ConnID); \
         if (g_bUseEL == IFX_TRUE) { \
            if ((m_level) >= TD_OS_PRN_USR_LEVEL_GET(m_name)) \
               Common_SendMsgToEL(g_buf_parsed); \
         } else { \
         TD_OS_PRN_USR_DBG_NL(m_name, m_level, (g_buf_parsed)); \
         } \
         if(IFX_TRUE == g_bITM_Print) \
         { \
            COM_SEND_TRACE_MESSAGE(m_level, g_buf_parsed); \
         } \
      } while(0)

/** printf define - for easier detection of printf in code */
#define TD_PRINTF          printf
/**
   Prints a trace message and sends it through communication socket.

   \param name     - Name of the trace group
   \param level    - level of this message
   \param message  - a printf compatible formated string + opt. arguments

   \return

   \remark
      The string will be redirected via printf if level is higher or equal to
      the actual level for this trace group ( please see SetTraceLevel ).
*/
#define TRACE(name,level,message) \
      do \
      { \
         COM_GENERATE_STRING message;\
         if ((g_bUseEL == IFX_TRUE) && (TD_EventLoggerFd > 0)) { \
            if ((level) >= TD_OS_PRN_USR_LEVEL_GET(name)) \
               Common_SendMsgToEL(g_buf); \
         } else { \
            TD_OS_PRN_USR_DBG_NL(name,level,message); \
         } \
         if((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
            (level >= g_pITM_Ctrl->nComDbgLevel) && \
            (COM_MODULAR_TEST == g_pITM_Ctrl->nTestType)) \
         { \
            if(NO_SOCKET != g_pITM_Ctrl->nComOutSocket)\
            { \
               COM_SEND_TRACE_MESSAGE(level, g_buf);\
            } \
         } \
      } while(0)

#ifdef TD_IPV6_SUPPORT

#define TRACE_IPV6(m_message)

/* \
   TD_TRACE(TAPIDEMO, DBG_LEVEL_OFF, TD_CONN_ID_DBG, m_message) */

#else

#define TRACE_IPV6(m_message)

#endif /* TD_IPV6_SUPPORT */
/** print message */
#define TAPIDEMO_PRINTF(m_ConnID, m_message) \
      do \
      { \
          /* generate string for easier handling */  \
          COM_GENERATE_STRING m_message; \
          ABSTRACT_ParseTrace(m_ConnID); \
          if ((g_bUseEL == IFX_TRUE) && (TD_EventLoggerFd > 0)) { \
                Common_SendMsgToEL(g_buf_parsed); \
          } else { \
             printf (g_buf_parsed); \
          } \
          /* if modular test send msg to control PC */ \
          if((IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) && \
             (COM_MODULAR_TEST == g_pITM_Ctrl->nTestType)) \
          { \
             if(NO_SOCKET != g_pITM_Ctrl->nComOutSocket)\
             { \
               /* send string without prefix - control PC has problems */ \
               /* with recognizing test traces with prefix */ \
               COM_SEND_TRACE_MESSAGE(DBG_LEVEL_OFF, g_buf);\
             } \
          } \
      } while(0)

TD_OS_PRN_USR_MODULE_DECL(TAPIDEMO);

/* ============================= */
/* Debug interface               */
/* ============================= */

/* macros for asserts */
#ifdef LINUX
#if 1
#define TD_ASSERT(m_expr, m_fd, m_pBoard, m_dev, m_ch) \
 do \
 { \
   if (!(m_expr)) \
   { \
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, \
            (#m_expr" failed\n\tin file \"%s\" on line %d, %s dev %d, ch %d\n", \
             __FILE__, __LINE__, m_pBoard->pszBoardName, m_dev, m_ch)); \
      Common_ErrorHandle (m_fd, m_pBoard, m_dev); \
   } \
 } while(0)
#else /* 1 */
#define TD_ASSERT(expr, fd, pBoard, dev, ch) if(!(expr)) {                    \
      printf(#expr" failed in file \"%s\" on line %d, %s dev %d, "            \
      "ch %d\n",                                                              \
      __FILE__, __LINE__, pBoard->pszBoardName, dev, ch);                     \
      Common_ErrorHandle (fd, pBoard, dev);                                   \
   }
#if 0
   else                                                                       \
   {                                                                          \
     TRACE (TAPIDEMO, DBG_LEVEL_LOW, (#expr", %s dev %d, ch %d\n",            \
     pBoard->pszBoardName, dev, ch));                                         \
   }
#endif /* 0 */
#endif /* 1 */
#endif /* LINUX */

#ifdef VXWORKS
#define TD_ASSERT(expr, fd, pBoard, dev, ch) if(!(expr)) {                    \
      IFX_char_t* sFileName = __FILE__;                                       \
      Common_StripPath(&sFileName);                                           \
      printf(#expr" failed in file \"%s\" on line %d, %s dev %d, "            \
      "ch %d\n",                                                              \
      sFileName, __LINE__, pBoard->pszBoardName, dev, ch);                    \
      Common_ErrorHandle (fd, pBoard, dev);                                   \
   }
#if 0
   else                                                                       \
   {                                                                          \
     TRACE (TAPIDEMO, DBG_LEVEL_LOW, (#expr", %s dev %d, ch %d\n",            \
     pBoard->pszBoardName, dev, ch));                                         \
   }
#endif /* 0 */
#endif /* VXWORKS */

/** Check if pointer is NULL.
       m_ptr  - pointer to check
       m_ret   - value to retunr in case of error. */
#define TD_PTR_CHECK(m_ptr, m_ret) if(m_ptr == IFX_NULL) {                   \
        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,                \
              ("Err, Invalid pointer ("#m_ptr"). (File: %s, line: %d)\n",      \
               __FILE__, __LINE__));                                           \
        return m_ret;                                                            \
        }

/** Check condition and print value of parameters that will help in debug.
       m_expr  - condition to check, if true then return ret
       m_param - parameter to print in case of expr being false
       m_ret   - value to return in case of error. */
#define TD_PARAMETER_CHECK(m_expr, m_param, m_ret) if(m_expr) {            \
        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,                  \
              ("Err, Invalid parameter ("#m_param"=%d). "                      \
              "(File: %s, line: %d)\n",                                        \
               (IFX_int32_t) m_param, __FILE__, __LINE__));                  \
        return m_ret;                                    \
        }

/**
 * return IFX_ERROR if ret is IFX_ERROR
 */
#define TD_RETURN_IF_ERROR(m_ret) \
   do \
   { \
      if (IFX_ERROR == m_ret) \
      { \
         return IFX_ERROR; \
      } \
   } while (0)

/** print error trace for function
 * \param m_ret - function return value
 * \param m_pszFunctionName  - name of failed function */
#define TD_PRINT_ON_FUNC_ERR(m_ret, m_pszFunctionName, m_nSeqConnId) \
   do \
   { \
      if (IFX_SUCCESS != m_ret) \
      { \
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_nSeqConnId, \
               ("Err, Function %s() failed (ret = %d). " \
                "(File: %s, line: %d)\n", \
                m_pszFunctionName, m_ret, __FILE__, __LINE__)); \
      } \
   } while (0)

/** check pointer and break out from switch's case
 * \param m_pVar - pointer to check
 * \param m_pszFunctionName  - name of failed function */
#define TD_PTR_CHECK_IN_CASE(m_pVar, m_nSeqConnId) \
   if (IFX_NULL == m_pVar) \
   { \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_nSeqConnId, \
            ("Err, Invalid pointer "#m_pVar"." \
             "(File: %s, line: %d)\n", \
             __FILE__, __LINE__)); \
      break; \
   }

/** print error info */
#define TD_ERR_HANDLING_YES      IFX_TRUE
/** do not print error info */
#define TD_ERR_HANDLING_NO       IFX_FALSE

/** macro is used to call ioctl() so it can be easyly changed
   to function that for example prints ioctl name or makes error handling,
   it should be used only for TAPI drivers (not sockets) */
#define TD_IOCTL(m_nFd, m_nIoctl, m_pArg, m_nDev, m_nSCon_ID) \
   TD_DEV_Ioctl(m_nFd, m_nIoctl, (IFX_uint32_t) m_pArg , \
      #m_nIoctl, m_nDev, m_nSCon_ID, TD_ERR_HANDLING_YES, __FILE__, __LINE__)


/** macro is used to call ioctl() so it can be easyly changed
   to function that for example prints ioctl name or makes error handling,
   it should be used only for TAPI drivers (not sockets) */
#define TD_IOCTL_NO_ERR(m_nFd, m_nIoctl, m_pArg, m_nDev, m_nSCon_ID) \
   TD_DEV_Ioctl(m_nFd, m_nIoctl, (IFX_uint32_t) m_pArg , \
      #m_nIoctl, m_nDev, m_nSCon_ID, TD_ERR_HANDLING_NO, __FILE__, __LINE__)

#ifdef DANUBE
/**
   Call single pin configuration.

   \param nFd - GPIO port file descriptor.
   \param nFct - IOCTL command.
   \param nPort - GPIO port.
   \param nPin - GPIO pin.
   \param nValue - GPIO value.
   \param nModule - GPIO module ID.
   \param nRet - Return value: IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
#ifdef OLD_BSP
#define TD_GPIO_IOCTL_PARM(nFd, nFct, nPort, nPin, nValue, nRet)\
{ \
   struct danube_port_ioctl_parm param = {0}; \
   memset(&param, 0, sizeof (param)); \
   param.port = nPort;\
   param.pin = nPin;\
   param.value = nValue;\
   nRet = TD_OS_DeviceControl(nFd, nFct, (IFX_ulong_t) &param);\
   if (IFX_SUCCESS != nRet)\
   { \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG, \
            ("Err, ioctl: "#nFct", (%d, %d), value: %d failed.\n",\
             nPort, nPin, nValue));\
   }\
};
#else /* OLD_BSP */
#define TD_GPIO_IOCTL_PARM(nFd, nFct, nPort, nPin, nValue, nModule ,nRet)\
{ \
   struct ifx_gpio_ioctl_parm param = {0};\
   memset(&param, 0, sizeof (param)); \
   param.port = nPort;\
   param.pin = nPin;\
   param.value = nValue;\
   param.module = nModule;\
   nRet = TD_OS_DeviceControl(nFd, nFct, (IFX_ulong_t) &param);\
   if (IFX_SUCCESS != nRet)\
   { \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG, \
            ("Err, ioctl: "#nFct", (%d, %d), value: %d failed.\n",\
             nPort, nPin, nValue));\
   }\
};
#endif /* OLD_BSP */
#endif /* DANUBE */

/** Check if function returned error.
   fct - function
   ret - could be TD_RETURN or TD_NO_RETURN. If it is set to TD_RETURN
         then return is called in case of error */
#define TD_FUNC_ASSERT(expr, pPhone, fct, dev, ch, ret)                        \
        if (expr != IFX_SUCCESS)                                               \
        {                                                                      \
           TRACE(TAPIDEMO, DBG_LEVEL_HIGH,                                     \
                ("Error, Phone No %d: "#fct" failed. "                         \
                 "(%s, dev %d, ch %d) "                                        \
                 "(File: %s, line: %d)\n",                                     \
                 pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName,           \
                 dev, ch, __FILE__, __LINE__));                                \
           if (ret)                                                            \
           {                                                                   \
              return IFX_ERROR;                                                \
           }                                                                   \
        }

/* Play call progres tone, check if successfull and print apropriate traces */
#define TD_CPTG_PLAY_CHECK(m_ret, m_pCtrl, m_pPhone, m_nTone, m_pszToneName) \
   m_ret = Common_TonePlay(m_pCtrl, m_pPhone, m_nTone); \
   if (IFX_SUCCESS == m_ret) \
   { \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, m_pPhone->nSeqConnId, \
            ("Phone No %d: Play %s.\n", m_pPhone->nPhoneNumber, m_pszToneName)); \
      COM_MOD_CONFERENCE_BUSY_TRACE(m_pPhone, m_nTone); \
   } \
   else \
   { \
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, m_pPhone->nSeqConnId, \
            ("Phone No %d: Failed to play %s. (File: %s, line: %d)\n", \
             m_pPhone->nPhoneNumber, m_pszToneName, __FILE__, __LINE__)); \
   }

/** Add fd and set new max fd if needed - device version */
#define TD_DEV_FD_SET(nFd, pFds, nMaxFd) \
   do { \
      TD_OS_DevFdSet(nFd, pFds); \
      if (nMaxFd < nFd) \
         nMaxFd = nFd; \
   } while (0)

/** Add fd and set new max fd if needed - socket version */
#define TD_SOC_FD_SET(nFd, pFds, nMaxFd) \
   do { \
      TD_OS_SocFdSet(nFd, pFds); \
      if (nMaxFd < nFd) \
         nMaxFd = nFd; \
   } while (0)

/** Reset timout flag */
#define TD_TIMEOUT_RESET(nUseTimeout)  { nUseTimeout = 0; }
/** Increase timout flag - another resource(phones or FXO) needes
    to use timeout */
#define TD_TIMEOUT_START(nUseTimeout)  { nUseTimeout++; }
/** Decrease timout flag - resource(phones or FXO) doesn't need timeout now */
#define TD_TIMEOUT_STOP(nUseTimeout)   if (nUseTimeout > 0)   \
                          {   nUseTimeout--;  }
/** Check if timeout should be used */
#define TD_TIMEOUT_CHECK(nUseTimeout)  (0 != nUseTimeout)

#ifdef TAPI_VERSION4
#define TD_SET_CH_AND_DEV(oStruct, nChannel, nDevice)   \
    do \
    { \
       oStruct.ch = nChannel; \
       oStruct.dev = nDevice; \
    } while (0);
#endif /* TAPI_VERSION4 */

#ifdef TD_RM_DEBUG
/** Macro used to trace currently allocated resources.
    \param pTmpPhone - pointer to phone structure
    \param m_nSCon_ID - seq conn id */
#define TD_RM_TRACE_PHONE_RESOURCES(m_pTmpPhone, m_nSCon_ID) \
   do \
   { \
      if (m_pTmpPhone->pBoard->pCtrl->pProgramArg->oArgFlags.nRM_Count) \
      { \
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, m_nSCon_ID, \
               ("Phone No %d: RESOURCES:\n" \
                " ConnId %d\n" \
                " VoipId %d\n" \
                " Gen %d\n" \
                " Det %d\n" \
                " Coder %d\n" \
                " Lec %d\n", \
                m_pTmpPhone->nPhoneNumber, \
                m_pTmpPhone->oRM_Debug.nConnIdCnt, \
                m_pTmpPhone->oRM_Debug.nVoipIdCnt, \
                m_pTmpPhone->oRM_Debug.nGenCnt, \
                m_pTmpPhone->oRM_Debug.nDetCnt, \
                m_pTmpPhone->oRM_Debug.nCodecCnt, \
                m_pTmpPhone->oRM_Debug.nLecCnt)); \
      } \
   } while (0)
#endif /* TD_RM_DEBUG */

/** returns pointer to main board
    \param m_pCtrl - pointer to control structure */
#define TD_GET_MAIN_BOARD(m_pCtrl) &m_pCtrl->rgoBoards[TD_MAIN_BOARD_NUMBER]

/** returns smaller of two numbers
    \param m_number1 - first number to check
    \param m_number2 - second number to check */
#define TD_GET_MIN(m_number1, m_number2) \
   (((m_number1) > (m_number2)) ? (m_number2) : (m_number1))

/* ============================= */
/* Global Defines                */
/* ============================= */

/* those two enums are different only by name
   so to simplyfy code define is used */
#define  TD_LINE_MODE_t    IFX_TAPI_LINE_MODE_t

typedef struct _PHONE_t PHONE_t;
typedef struct _FXO_t FXO_t;
typedef struct _BOARD_t BOARD_t;
typedef struct _CTRL_STATUS_t CTRL_STATUS_t;
typedef struct _PROGRAM_ARG_t PROGRAM_ARG_t;
typedef struct _TAPIDEMO_DEVICE_CPU_t TAPIDEMO_DEVICE_CPU_t;
typedef struct _TAPIDEMO_DEVICE_FXO_t TAPIDEMO_DEVICE_FXO_t;

/** UDP port for voice data (VOICE_UDP_PORT .. (VOICE_UDP_PORT +
 * 2*MAX_SYS_CH_RES))
    According to the standard it is recomended to send RTP packets on even
    ports while odd ones are reserved for RTCP */
#ifdef EASY336
enum { VOICE_UDP_PORT = SVIP_UDP_FROM };
#else /* EASY336 */
enum { VOICE_UDP_PORT = 5002 };
#endif /* EASY336 */

#if (defined(EASY336) || defined(XT16))
/** Wait time for calibration */
enum { SVIP_XT16_WAIT_CALIBRATION_POLLTIME = 300 };
#endif

/** UDP port for making/ending voip/pcm/... calls. */
enum { ADMIN_UDP_PORT = 5000 };
enum { ADMIN_UDP_PORT_IPV6 = ADMIN_UDP_PORT };

#ifdef USE_FILESYSTEM
enum { MAX_FULL_FILE_LEN = 256 };
#endif /* USE_FILESYSTEM */
/** Phone digit representation */
typedef enum
{
   DIGIT_ONE = 0x01,
   DIGIT_TWO = 0x02,
   DIGIT_THREE = 0x03,
   DIGIT_FOUR = 0x04,
   DIGIT_FIVE = 0x05,
   DIGIT_SIX = 0x06,
   DIGIT_SEVEN = 0x07,
   DIGIT_EIGHT = 0x08,
   /** Used to start PCM calls */
   DIGIT_NINE = 0x09,
   DIGIT_STAR = 0x0A,
   DIGIT_ZERO = 0x0B,
   /** Used to start conference, calling another party */
   DIGIT_HASH = 0x0C
} DIGIT_t;

/** Sign for no parameters for ioctl() call */
enum { NO_PARAM = 0 };

/** Flag for no device FD */
enum { NO_DEVICE_FD = -1 };

/** Flag for no GPIO FD */
enum { NO_GPIO_FD = -1 };

/** Flag for no socket */
enum { NO_SOCKET = -1 };

/** Flag for no pipe */
enum { NO_PIPE = -1 };

/** Flag for no file */
enum { NO_FILE = -1 };

/** Flag for no channel file descriptor */
enum { NO_CHANNEL_FD = -1 };

/** Flag for no channel */
enum { NO_CHANNEL = -1 };

/** Flag for no used channel */
enum { NO_USED_CHANNEL = -1 };

/** Flag for not set value */
enum { TD_NOT_SET = -1 };

/** Flag for no board idx */
enum { NO_BOARD_IDX = -1 };

/** Flag for no phone number */
enum { NO_PHONE_NUMBER = -1 };

/** Flag for PCM call, will be send to external phone */
enum { CALL_FLAG_PCM = 0x80 };

/** Flag for VoIP call, will be send to external phone */
enum { CALL_FLAG_VOIP = 0x00 };

/** Flag for VoIP call, will be send to external phone */
enum { CALL_FLAG_UNKNOWN = 0xFF };

/** Flag indicating start of command message */
enum { COMM_MSG_START_FLAG = 0xFF };

/** Flag indicating end of command message */
enum { COMM_MSG_END_FLAG = 0x77 };

/** Max. length of phone number or number of digits than can be dialed */
enum { MAX_DIALED_NUM = 24 };

/** Min. value of phone number */
#ifdef TAPI_VERSION4
   enum { MIN_PHONE_NUM = 100 };
#else
   enum { MIN_PHONE_NUM = 1 };
#endif

/** Max. length of first 3 number of IP address for different network */
enum { DIFF_NET_IP_LEN = 12 };

/** Use restor default option. */
enum { RESTORE_DEFAULT = -1 };

/** Length for 32 bit IP address in characters "xxx.xxx.xxx.xxx". */
enum { TD_IP_ADDRESS_LEN = 16 };

/** Length for MAC address in characters "xx:xx:xx:xx:xx:xx". */
enum { MAC_ADDRESS_LEN = 18 };

/** Length for port number "nnnnn, 0..65535". */
enum { PORT_NUM_LEN = 5 };

/** Turn off CID support */
enum { TD_TURN_OFF_CID = 10 };

/** call progress tones names */
typedef enum _CPT_NAME_t
{
   NO_TONE = -1,
   DIAL_TONE,
   SECOND_DIAL_TONE,
   BUSY_TONE,
   RINGBACK_TONE,
   CONGESTION_TONE,
   HOWLER_TONE,
   STOP_TONE,
   MAX_TONE
}CPT_NAME_t;

/** tones numbers for indicating internal events/problems */
typedef enum
{
   TD_ITN_SET_BAND_WIDE = 50,
   TD_ITN_SET_BAND_NARROW = 51,
   TD_ITN_ERROR = 52
} INDICATION_TONE_NUMBER_t;

/** Call progress tones indexes for more check drv_tapi_tone.c
    or Predefined Tones Table in tapi documentation. */
typedef enum _TONE_IDX_t
{
   DIALTONE_IDX = 25,
   RINGBACK_TONE_IDX = 26,
   BUSY_TONE_IDX = 27
} TONE_IDX_t;

/** tone index that will be detected if only one CPTD can be activated for given
 *  channel */
#define TD_DEFAULT_CPT_TO_DETECT_ON_FXO BUSY_TONE_IDX

#ifdef EASY336
/** resource type */
typedef enum _RESOURCE_TYPE_t
{
   RES_NONE = 0x00,
   RES_VOIP_ID = 0x01,
   RES_CONN_ID = 0x02,
   RES_DET = 0x04,
   RES_GEN = 0x08,
   RES_LEC = 0x10,
   RES_CODEC = 0x20,
   /** all resources voip_id, conn_id, sig_det, sig_gen, lec and codec */
   RES_ALL = 0xFF
} RESOURCE_TYPE_t;
#endif /* EASY336 */

/** number of state machines */
enum { MAX_STATE_MACHINE = 2 };

/** Maximum valid port number. */
enum { MAX_PORT_NUM = 65535 };

#ifndef ETH_ALEN
/** Octets in one ethernet address */
#define ETH_ALEN 6
#endif

/** mask that can be used to get lowest IP byte */
#define TD_IP_LOWEST_BYTE_MASK            0x000000FF

/** mask to get state info part */
#define TD_STATE_MASK            0xFFFF0000
/** if this flag is set then during this state packets are transmited between
    phones - used by tapidemo packet handling functions */
#define TD_STATE_DATA_TRANSMIT   0x00010000
/** if this flag is set then this state is used for fax, modem or t.38 */
#define TD_STATE_FAX_MODEM       0x00020000
/** if this flag is set then this state is used for conferences */
#define TD_STATE_CONFERENCE      0x00040000
/** if this flag is set then during this state
    tone is played on phone channel */
#define TD_STATE_TONE_PLAY       0x00080000
/** if this flag is set then this state is used for Phone Plug Detection */
#define TD_STATE_PPD             0x00100000
/** if this flag is set then this state is used for BACUS_TEST */
#define TD_STATE_ITM             0x01000000



/** max size of message that will be received - should not be changed
    in future versions of TAPIDEMO */
#define TD_MAX_COMM_MSG_SIZE              512
/** size of COMM_MSG_DATA_1_t structure */
#define TD_MAX_COMM_MSG_SIZE_DATA_1       32

/** max size of peer name that is send in message through
    communication socket */
#define TD_MAX_PEER_NAME_LEN        20

/** message version
 * change history:
 * 2010-05-24  - creation of message versioning V=01
 * 2011-08-25  - added conn ID to message V=02 */
/** for this message version CID sending was introduced */
#define TD_COMM_MSG_VER_CID            0x01
/** for this message version Seq Conn ID sending was introduced */
#define TD_COMM_MSG_VER_CONN_ID        0x02
/** current version */
#define TD_COMM_MSG_CURRENT_VERSION    TD_COMM_MSG_VER_CONN_ID

/** returns non zero value if nInfo flag is set
      \param nState - phone state
      \param nInfo  - state flag to check
 */
#define TD_GET_STATE_INFO(nState, nInfo) (((nState) & TD_STATE_MASK) & (nInfo))

/** States of state machine. Enums represent names of states.
    TD_STATE_... flags are set to notify about state aditional features,
    e.g. S_OUT_DIALTONE = 0x0001 | TD_STATE_TONE_PLAY - during this state tone
    is played. This can help to turn off tone when state is changed e.g.
    TD_GET_STATE_INFO(nState, TD_STATE_TONE_PLAY) will return non zero value
    if tone is played during this state. */
typedef enum
{
   /** Phone is on-hook. */
   S_READY           = 0x0000,
   /** Phone is off-hook, and tone is played(dialtone),
       waiting for dialed digits. */
   S_OUT_DIALTONE    = 0x0001 | TD_STATE_TONE_PLAY,
   /** Phone is off-hook, one or more digits were dialed,
       waiting for more digits to complete number that can be called. */
   S_OUT_DIALING     = 0x0002,
   /** FXO call in progress. */
   S_OUT_FXO_CALL    = 0x0003,
   /** Phone number was dialed waiting for peer confirmation. */
   S_OUT_CALLING     = 0x0004,
   /** Waiting for peer to make hook-off, tone is played(ringback). */
   S_OUT_RINGBACK    = 0x0005 | TD_STATE_TONE_PLAY,
   /** Connection in progress, data should be transmitted. */
   S_ACTIVE          = 0x0006 | TD_STATE_DATA_TRANSMIT,
   /** Connection failed or peer disconected, tone is played(busy tone). */
   S_BUSYTONE        = 0x0007 | TD_STATE_TONE_PLAY,
   /** Phone is ringing, waiting for hook-off to establish connection. */
   S_IN_RINGING      = 0x0008,
   /** Phone is waiting for dialed digits to establish new connection,
       phone is in conference, tone is played(dialtone). */
   S_CF_DIALTONE     = 0x0009 | TD_STATE_CONFERENCE | TD_STATE_TONE_PLAY,
   /** One or more digits were dialed, waiting for more digits to complete
       number that can be called, phone is in conference. */
   S_CF_DIALING      = 0x000A | TD_STATE_CONFERENCE,
   /** Phone number was dialed waiting for peer confirmation, phone is in
       conference. */
   S_CF_CALLING      = 0x000B | TD_STATE_CONFERENCE,
   /** Waiting for peer to make hook-off, phone is in conference,
      tone is played(ringback). */
   S_CF_RINGBACK     = 0x000C | TD_STATE_CONFERENCE | TD_STATE_TONE_PLAY,
   /** Conference connections in progress, data should be transmitted,
       phone is in conference. */
   S_CF_ACTIVE       = 0x000D | TD_STATE_DATA_TRANSMIT | TD_STATE_CONFERENCE,
   /** Connection failed , phone is in conference, tone is played(busy tone). */
   S_CF_BUSYTONE     = 0x000E | TD_STATE_CONFERENCE | TD_STATE_TONE_PLAY,
   /** Tone playing is tested, ITM specific state. */
   S_TEST_TONE       = 0x000F | TD_STATE_ITM,
   /** fax/modem transmission in progress, data should be transmitted,
       fax/modem specific state. */
   S_FAX_MODEM       = 0x0010 | TD_STATE_DATA_TRANSMIT | TD_STATE_FAX_MODEM,
   /** t.38 transmission in progress, data should be transmitted,
       fax/modem specific state. */
   S_FAXT38          = 0x0011 | TD_STATE_DATA_TRANSMIT | TD_STATE_FAX_MODEM,
   /** t.38 transmission request recived, fax/modem specific state. */
   S_FAXT38_REQ      = 0x0012 | TD_STATE_FAX_MODEM,
   /** t.38 transmission end recived, fax/modem specific state. */
   S_FAXT38_END      = 0x0013 | TD_STATE_FAX_MODEM,
   /** dect handset is not registered. */
   S_DECT_NOT_REGISTERED   = 0x0014,
   /** Phone Plug Detection. Line is disabled. */
   S_LINE_DISABLED            = 0x0015 | TD_STATE_PPD,
   /** Phone Plug Detection. Phone is connected. Line is set to STANDBY. */
   S_LINE_STANDBY             = 0x0016 | TD_STATE_PPD,
   /** Phone Plug Detection. Off-Hook detection procedure is ongoing. */
   S_OFF_HOOK_DETECTION       = 0x0017 | TD_STATE_PPD,
   /** Phone Plug Detection. On-Hook detection procedure is ongoing. */
   S_ON_HOOK_DETECTION        = 0x0018 | TD_STATE_PPD,
   /** Phone Plug Detection. Idle state. */
   S_PPD_IDLE                 = 0x0019 | TD_STATE_PPD,
   /** Last state - used to mark end of state tables */
   S_END             = 0xFFFF
} STATE_MACHINE_STATES_t;

/** Internal events - WARNING new events should be added
 *  at end of the list before IE_END,
 *  change of the order results in incompatibility of messages
 *  sent between different versions of tapidemo */
typedef enum
{
   IE_NONE = 0,
   IE_READY,
   IE_HOOKOFF,
   IE_HOOKON,
   IE_FLASH_HOOK,
   IE_DIALING,
   IE_RINGING,
   IE_RINGBACK,
   IE_ACTIVE,
   IE_CONFERENCE,
   IE_END_CONNECTION,
   IE_BUSY,
   IE_CNG_FAX,
   IE_CNG_MOD,
   IE_AM,
   IE_PR,
   IE_CED,
   IE_CED_END,
   IE_DIS,
   IE_V8BIS,
   IE_V21L,
   IE_V18A,
   IE_V27,
   IE_BELL,
   IE_V22,
   IE_V22ORBELL,
   IE_V32AC,
   IE_CAS_BELL,
   IE_V21H,
   IE_END_DATA_TRANSMISSION,
   IE_CONFIG,
   IE_CONFIG_DIALING,
   IE_CONFIG_HOOKON,
   IE_CONFIG_PEER,
   IE_EXT_CALL_SETUP,
   IE_EXT_CALL_PROCEEDING,
   IE_EXT_CALL_ESTABLISHED,
   IE_EXT_CALL_WRONG_NUM,
   IE_EXT_CALL_NO_ANSWER,
   IE_EXT_CALL_WORKAROUND_EASY508xx,
   IE_T38_ERROR,
   IE_T38_REQ,
   IE_T38_ACK,
   IE_T38_END,
   IE_DECT_REGISTER,
   IE_DECT_UNREGISTER,
   IE_TEST_TONE,
   IE_PHONEBOOK_UPDATE,
   IE_PHONEBOOK_UPDATE_RESPONSE,
   IE_FAULT_LINE_GK_LOW,
   IE_KPI_SOCKET_FAILURE,
   /** this event is never generated,
      it is only used to mark end of EVENT_ACTIONS_t table
      WARNING new events should be added
      at end of the list before IE_END,
      change of the order results in incompatibility of messages
      sent between different versions of tapidemo */
   IE_END
} INTERNAL_EVENTS_t;

/** structure with event and pointer to action that should be taken
    when this event ococcurs */
typedef struct _EVENT_ACTIONS_t
{
   INTERNAL_EVENTS_t nEvent;
   IFX_return_t (*pAction)();
} EVENT_ACTIONS_t;

/** structure with state and pointer to table of EVENT_ACTIONS_t
    with all handled events */
typedef struct _STATE_COMBINATION_t
{
   STATE_MACHINE_STATES_t nState;
   EVENT_ACTIONS_t* pEventActions;
} STATE_COMBINATION_t;

/** structure with current state
    and pointer to table of STATE_COMBINATION_t for all states */
typedef struct _STATE_MACHINES_t
{
   STATE_MACHINE_STATES_t nState;
   STATE_COMBINATION_t* pStateCombination;
} STATE_MACHINES_t;

/** types of state machines */
typedef enum _STATE_MACHINE_TYPE_t
{
   /** configuration state machine */
   CONFIG_SM = 0,
   /** FXS state machine */
   FXS_SM = 1
} STATE_MACHINE_TYPE_t;

/** call direction */
typedef enum _CALL_DIRECTION_TYPE_t
{
   /** no call direction - no call */
   CALL_DIRECTION_NONE = 0,
   /** transmit direction - set for caller phone */
   CALL_DIRECTION_TX,
   /** receive direction - set for called phone */
   CALL_DIRECTION_RX
} CALL_DIRECTION_TYPE_t;


/** max enum ID */
#define TD_MAX_ENUM_ID  0xffffffff
/** max length of name array */
#define TD_MAX_NAME     64

/** Max path length to download files. */
enum { MAX_PATH_LEN = 256 };

/** Max line's length in .tapidemorc. */
enum { MAX_LINE_LEN = 512 };


/** structure to assigne enum name to its value (id) */
typedef struct _TD_ENUM_2_NAME_t
{
   IFX_int32_t nID;
   IFX_char_t* rgName;
} TD_ENUM_2_NAME_t;

/** error code for Common_Enum2Name() function. This enums are used as
    index for TD_rgEnum2NameErrorCode[] table. */
typedef enum _TD_ENUM_2_NAME_ERR_CODE_t
{
   /** enum value not found in the table */
   ENUM_VALUE_NOT_FOUND = 0,
   /** NULL pointer */
   ENUM_NULL_POINTER
} TD_ENUM_2_NAME_ERR_CODE_t;

/** enum used by macros to decide if call return. */
typedef enum _TD_RETURN_CODE_t
{
   TD_NO_RETURN = 0,
   TD_RETURN = 1
} TD_RETURN_CODE_t;
/** enum used as an argument to mapping functions */
typedef enum _TD_MAP_ADD_REMOVE_t
{
   /** map channels */
   TD_MAP_ADD = 0,
   /** unmap channels */
   TD_MAP_REMOVE = 1
} TD_MAP_ADD_REMOVE_t;
/* Wide/Narrow-Band settings */
typedef enum _TD_BAND_WIDE_NARROW_t
{
   /* NarrowBand */
   TD_BAND_NARROW = 0,
   /* WideBand */
   TD_BAND_WIDE = 1
} TD_BAND_WIDE_NARROW_t;

/** number of star digits that must be used to dial phone number
    with full IP address */
#define NUM_OF_STARS_IN_IP 5

/** Max number of available FW filenames */
#define TD_PRAM_FW_FILE_MAX      4

#if (!defined(EASY336) && !defined(XT16))
/** number of digits of phone number x */
#define TD_PHONE_NUM_LEN      1
#else /* (!defined(EASY336) && !defined(XT16)) */
/** number of digits of phone number xxx */
#define TD_PHONE_NUM_LEN      3
#endif /* (!defined(EASY336) && !defined(XT16)) */

#define TD_PHONE_STRING       "Phone No : "

/** length of string holding representation of two uint32 with string end
    and ';' char in between nubers */
#define TD_TWO_UINT32_STRING_LEN     22

/** JB(jitter buffer) data format - was TAPI version dependant */
#define TD_JB_DATA_FORMAT        "%u"

/** RTCP data format - was TAPI version dependant */
#define TD_RTCP_DATA_FORMAT        "%u"

#ifdef LINUX
#if defined (HAVE_DRV_VMMC_HEADERS) || defined (EASY3201_EVS)
/** Default path to download files. */
#define DEF_DOWNLOAD_PATH        "/lib/firmware/"
/** alternate download path for backward compability */
#define TD_ALTERNATE_DOWNLOAD_PATH     "/opt/ifx/downloads/"
#else /* HAVE_DRV_VMMC_HEADERS */
/** Default path to download files. */
#define DEF_DOWNLOAD_PATH        "/opt/lantiq/downloads/"
/** alternate download path for backward compability */
#define TD_ALTERNATE_DOWNLOAD_PATH     "/opt/ifx/downloads/"
#endif /* HAVE_DRV_VMMC_HEADERS || EASY3201_EVS */
#endif /* LINUX */

#ifdef VXWORKS
/** Default path to download files. */
#define DEF_DOWNLOAD_PATH        "/romfs/"
#endif /* VXWORKS */

/** path to device files directory */
#define TD_DEV_PATH              "/dev/"

/** name of file with path to downloads (FW and BBD files) */
#define DOWNLOAD_FILE_NAME       ".tapidemorc"
/** name of variable with path to downloads in DOWNLOAD_FILE_NAME */
#define DOWNLOAD_VARIABLE       "DOWNLOADS"

#ifdef TD_IPV6_SUPPORT
/** Phonebook default path */
#define TD_PHONEBOOK_PATH        "/tmp"

/** Phonebook name */
#define TD_PHONEBOOK_FILE            "tapidemo_phonebook"

#define TD_NO_NAME               "no name"
#endif /* TD_IPV6_SUPPORT */

/** max device name length with number e.g. /dev/vmmc10 */
#define MAX_DEV_NAME_LEN_WITH_NUMBER      50

/** string is invalid or some other problem occured */
#define TD_INVALID_STRING     "invalid string"

#define TD_TIMER_FIFO_NAME    "/tmp/TimerFifo"

/*
   enum description - can be used for help printing
 */
#define TD_STR_TAPI_PKT_EV_OOB_NO \
   "no generation of RTP packets - no DTMF suppression"
#define TD_STR_TAPI_PKT_EV_OOB_ONLY \
   "generate RTP packets         - suppress tone (DTMF only)"
#define TD_STR_TAPI_PKT_EV_OOB_ALL \
   "generate RTP packets         - no DTMF suppression"
#define TD_STR_TAPI_PKT_EV_OOB_BLOCK \
   "no generation of RTP packets - suppress tone (DTMF only)"

#define TD_STR_TAPI_PKT_EV_OOBPLAY_PLAY \
   "RFC 2833 packets are played out"
#define TD_STR_TAPI_PKT_EV_OOBPLAY_MUTE \
   "RFC 2833 packets are muted"
#define TD_STR_TAPI_PKT_EV_OOBPLAY_ATP_PLAY \
   "RFC 2833 packets are played out - using alternative  payload type"

/* --------------------------------------------------------- */
/*                    CONFERENCE START                       */
/* --------------------------------------------------------- */

/** representing local or remote call */
typedef enum _CALL_TYPE_t
{
   UNKNOWN_CALL_TYPE = 0,
   EXTERN_VOIP_CALL = 1,
   LOCAL_CALL = 2,
   PCM_CALL = 3,
   LOCAL_BOARD_CALL = 5,
   FXO_CALL = 6,
   LOCAL_PCM_CALL = 7,
   LOCAL_BOARD_PCM_CALL = 8,
   DTMF_COMMAND = 9,
   NOT_SUPPORTED = 10
} CALL_TYPE_t;

/** Conference status */
/** Also used for flag if not in conference */
enum { NO_CONFERENCE = 0 };

#ifndef EASY336
/** Max peers in conference */
enum { MAX_PEERS_IN_CONF = 4 };
#else /* EASY336 */
/** Max peers in conference */
enum { MAX_PEERS_IN_CONF = 16 };
#endif /* EASY336 */

/** How many conferences can we have. Cannot be bigger than number of phones. */
#ifndef EASY336
enum { MAX_CONFERENCES = 2 };
#else /* EASY336 */
enum { MAX_CONFERENCES = 16 };
#endif /* EASY336 */

/** Flag for no free data channel */
enum { NO_FREE_DATA_CH = -1 };

/** Status no external peer in conference */
enum { NO_EXTERNAL_PEER = 0 };

/** Status no PCM peer in conference */
enum { NO_PCM_PEER = 0 };

/** Status no local peer in conference */
enum { NO_LOCAL_PEER = 0 };

/** No value is set */
enum { TD_NO_VALUE = -1 };

/** No device is specified */
enum { NO_DEV_SPECIFIED = 0 };

/** used to mark if function should return device or channel data */
enum {
   /** device data is used */
   TYPE_DEVICE = 1,
   /** channel data is used */
   TYPE_CHANNEL
};

/** number of the main board in boards array */
#define TD_MAIN_BOARD_NUMBER     0

/** for pipe() fd[0] is readable and fd[1] is writeable,
    enum used get readable and writeable FDs */
typedef enum _TD_PIPE_IN_OUT_t
{
   /* readable pipe */
   TD_PIPE_OUT = 0,
   /* writeable pipe */
   TD_PIPE_IN = 1
}TD_PIPE_IN_OUT_t;

#ifdef TD_DECT
/** holds call handle and handset ID/number for one channel */
typedef struct _TD_DECT_CHANNEL_TO_HANDSET_t
{
   /** call handle - pointer */
   IFX_int32_t nCallHdl;
   /** handset ID/number */
   IFX_int32_t nHandsetId;
}TD_CHANNEL_TO_HANDSET_t;

/** control structure for DECT timeout for DIALTONE for single channel */
typedef struct _TD_DECT_DIALTONE_TIMEOUT_CTRL_t
{
   /** start time */
   IFX_time_t timeDialtoneStart;
   /** set if timeout is used */
   IFX_boolean_t bTimeoutActive;
}TD_DECT_DIALTONE_TIMEOUT_CTRL_t;

/** structure used to hold dect stack specific data */
typedef struct _TD_DECT_STACK_CTRL_t
{
   /** pipe file descriptor used to write data */
   IFX_int32_t nDectPipeFdIn;
   /** pipe file pointer used to write data */
   TD_OS_Pipe_t* pDectPipeFpIn;
   /** number of DECT channels */
   IFX_int32_t nDectChNum;
   /** DECT channel data (handset ID and call handle) */
   TD_CHANNEL_TO_HANDSET_t* pChToHandset;
   /** DECT initialized flag */
   IFX_boolean_t nDectInitialized;
   /** Pointer to path to FW files to download */
   IFX_char_t* psPathToDwnldFiles;
   /** dialtone timeout data */
   TD_DECT_DIALTONE_TIMEOUT_CTRL_t* pDialtoneTimeoutCtrl;
   /** Communication socket to send data For Production Test Tool */
   IFX_int32_t nToCliFd;
   /** used for communication with Production Test Tool  */
   struct sockaddr_un oToCliFILE_name;
   /* size of oToCliFILE_name */
   IFX_int32_t nToCliSize;
}TD_DECT_STACK_CTRL_t;

/** DECT events sent to tapidemo currently not all are used */
typedef enum _TD_DECT_EVENT_t
{
   TD_DECT_EVENT_NONE = 0,
   TD_DECT_EVENT_HOOK_OFF,
   TD_DECT_EVENT_HOOK_ON,
   TD_DECT_EVENT_FLASH,
   TD_DECT_EVENT_DIGIT,
   TD_DECT_EVENT_REGISTER,
   TD_DECT_EVENT_WIDEBAND_ENABLE,
   TD_DECT_EVENT_WIDEBAND_DISABLE,
   TD_DECT_EVENT_DECT_WORKING,
   TD_DECT_EVENT_END
} TD_DECT_EVENT_t;

/** dect stack data send in DECT message */
typedef struct _TD_DECT_STACK_DATA_t
{
   /** received diggits buffer */
   IFX_char_t acRecvDigits[IFX_DECT_MAX_DIGITS];
}TD_DECT_STACK_DATA_t;

/** message that is send from callback functions */
typedef struct _TD_DECT_MSG_t
{
   /** DECT channel number */
   IFX_int32_t nDectCh;
   /** event number */
   TD_DECT_EVENT_t nDectEvent;
   /** additional datal */
   TD_DECT_STACK_DATA_t oData;
}TD_DECT_MSG_t;

/** Phone Type */
typedef enum _PHONE_TYPE_t
{
   /** invalid phone type */
   PHONE_TYPE_INVALID,
   /** analog phone type */
   PHONE_TYPE_ANALOG,
   /** dect phone type */
   PHONE_TYPE_DECT
} PHONE_TYPE_t;
#endif /* TD_DECT */

#ifdef TD_FAX_MODEM
/**
   Mode of FAX transmission.
 */
typedef enum _TD_FAX_MODE_t
{
   /** Only transparetn fax transmission can be used */
   TD_FAX_MODE_TRANSPARENT = 0,
   /** Call is started as T.38 transmission (Fax Data Pump) */
   TD_FAX_MODE_FDP,
   /** Fax transmission is configured according to detected signals */
   TD_FAX_MODE_DEAFAULT
} TD_FAX_MODE_t;
#endif /* TD_FAX_MODEM */

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
/**
   FAX T.38 configuration data.
 */
 typedef struct _TD_T38_CONFIG_t
 {
#ifdef TD_T38_FAX_TEST
    /** fax stack session configuration */
    IFX_TAPI_T38_FAX_CFG_t oT38FaxCfg;
    /** fax data pump(fdp) configuration */
    IFX_TAPI_T38_FDP_CFG_t oT38FdpCfg;
#endif /* TD_T38_FAX_TEST */
    /** session configuration */
    IFX_TAPI_T38_SESS_CFG_t oT38SessCfg;
 } TD_T38_CONFIG_t;

#ifdef TD_T38_FAX_TEST
 /**
   Structure with FAX T.38 TEST data
 */
 typedef struct _TD_T38_FAX_TEST_CTRL_t
 {
    /** socket fd */
    TD_OS_socket_t nComSocket;
    /** flag to check if fax test should be use,
        set to false if fax test initialization fails */
    IFX_boolean_t bUseFaxTest;
 } TD_T38_FAX_TEST_CTRL_t;
#endif /* TD_T38_FAX_TEST */
#endif /* (def(TD_FAX_MODEM) && def(TD_FAX_T38) && def(HAVE_T38_IN_FW)) */

#ifdef EASY3111
/** EASY 3111 specific data, used when EASY 3111 is used as extension board
    with other than EASY 3201 board */
typedef struct _TD_EASY311_PHONE_SPECIFIC_t
{
   /** if IFX_TRUE then connection to main board is active */
   IFX_boolean_t fToMainConnActive;
   /** PCM channel on main board, used also as flag,
       if set to TD_NOT_SET, then connection resorces aren't allocated */
   IFX_int32_t nOnMainPCM_Ch;
   /** Holds last PCM channel on main board. Used only for ending conference.
       PCM channel on main could be already freed in TAPIDEMO_ClearPhone()
       but not unmapped */
   IFX_int32_t nOnMainPCM_Ch_Bak;
   /** PCM channel FD on main board */
   IFX_int32_t nOnMainPCM_ChFD;
   /** Timeslot(s) used to send voice between boards */
   IFX_int32_t nTimeslot;
} TD_EASY311_PHONE_SPECIFIC_t;
#endif /* EASY3111 */

/** Conference support */
/* no conferencing support in static RM */
typedef struct _CONFERENCE_t
{
   /** Conference status, IFX_TRUE this conference is active, IFX_FALSE
       is free for use. */
   IFX_boolean_t fActive;

   /** Number of external peers in conference */
   IFX_int32_t nExternalPeersCnt;

   /** Number of PCM peers (channels) in conference */
   IFX_int32_t nPCM_PeersCnt;
} CONFERENCE_t;


/* --------------------------------------------------------- */
/*                    CONFERENCE END                         */
/* --------------------------------------------------------- */


/** Represents called peer */
typedef struct _PEER_t
{
   /** Called phone number */
   IFX_int32_t nPhoneNum;

   /** Structure for external phone */
   struct {
      /** Addr we send data to */
      TD_OS_sockAddr_t oToAddr;
      /** MAC address of this peer. */
      IFX_uint8_t MAC[ETH_ALEN];
      /* Keeps dialed number, except network part -
         used in external connection */
      IFX_char_t rgCollectNumber[4];
#ifdef TD_IPV6_SUPPORT
      /** holds IPv6 address */
      struct sockaddr_in6 oToAddrIPv6;
#endif /* TD_IPV6_SUPPORT */
   } oRemote;
   /** Structure for PCM phone */
   struct {
      /** Addr we send data to */
      TD_OS_sockAddr_t oToAddr;
   } oPCM;
   /** Pointer to connection structure - other local phone */
   struct {
      /** Pointer to other phone */
      struct _PHONE_t* pPhone;
      /** Flag status used for retrieving right timeslot index pair.
          Called phone represents slave, caller phone represents master.
          IFX_TRUE if we are slave, otherwise IFX_FALSE for master */
      IFX_boolean_t fSlave;
      /** if set wideband is used for DECT<->DECT call */
      IFX_boolean_t fDectWideband;
   } oLocal;
} PEER_t;


/** Represents connection */
typedef struct _CONNECTION_t
{
   /** Connection status, IFX_TRUE - active, IFX_FALSE - not active
       Set only for caller when changing state from ringback to active
       and ringing to active. */
   IFX_boolean_t fActive;

   /** Connection status, IFX_TRUE - active, IFX_FALSE - not active
       workaround for new SM use fActive flag */
   IFX_boolean_t fPCM_Active;

   /* ---------------------------------------------------------------
          Information of channels, sockets used to call the phone
    */

   /** 2 - local, 1 - remote, 3 - pcm, ... */
   CALL_TYPE_t nType;

   /** For external actions (IP + port) */
   TD_OS_sockAddr_t oUsedAddr;

   /** IFX_TRUE if IPv6 call */
   IFX_boolean_t bIPv6_Call;

   /** For external actions (MAC) */
   IFX_uint8_t oUsedMAC[ETH_ALEN];

   /** Socket on which call to peer was made (only VoIP or PCM connections). */
   IFX_int32_t nUsedSocket;

   /** Channel on which call to peer was made (can be data, pcm).
       It depends of the flag fType. */
   IFX_int32_t nUsedCh;

   /** Channel file descriptor */
   IFX_int32_t nUsedCh_FD;

   /** External dialing status, IFX_TRUE this is external dialing */
   IFX_boolean_t bExtDialingIn;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /** Informs if peer finished T.38 transmission. IFX_TRUE - peer finished
       T.38 transmission. */
   IFX_boolean_t bT38_PeerEndTransm;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   /** Time when phone started waiting on second peer in external call */
   IFX_time_t timeStartExtCalling;

#ifdef EASY336
   /** pointer to used voipID */
   IFX_int32_t voipID;
#endif /* EASY336 */

   /** Feature number to use/configure */
   IFX_int32_t nFeatureID;

   /** PCM call specific data */
   struct {
      /** Timeslot RX index */
      IFX_int32_t nTimeslot_RX;
      /** Timeslot TX index */
      IFX_int32_t nTimeslot_TX;
   } oPCM;
   /* -------------------------------------------
               Information of called phone
    */
   /** Called peer */
   PEER_t oConnPeer;

#ifdef QOS_SUPPORT
    struct {
        /** QOS_SUPPORT: 1 - session is started, 0 - session stopped */
        IFX_int32_t nInSession;
        /** Session pair */
        QOS_INIT_SESSION oPair;
    } oSession;
#endif

} CONNECTION_t;

#ifdef EASY336
/** structure used to count LineIDs asigned to ConnID */
typedef struct _TD_CONN_ID_LINE_ID_CNT_t
{
   /** number of ConnID */
   IFX_int32_t nConnID;
   /** number of LineIDs */
   IFX_int32_t nLineID_Cnt;
} TD_CONN_ID_LINE_ID_CNT_t;
#endif /* EASY336 */

#ifdef TD_RM_DEBUG
/** structure used to count allocated resources
    created for debug purposes */
typedef struct _TD_RM_DEBUG_t
{
   /** number of voip IDs */
   IFX_int32_t nVoipIdCnt;
   /** number of connection IDs */
   IFX_int32_t nConnIdCnt;
   /** number of signal detectors */
   IFX_int32_t nDetCnt;
   /** number of signal generators */
   IFX_int32_t nGenCnt;
   /** number of LEC resources */
   IFX_int32_t nLecCnt;
   /** number of codec resources */
   IFX_int32_t nCodecCnt;
} TD_RM_DEBUG_t;
#endif /* TD_RM_DEBUG */

/** holds array with ascii representation of dialed digits with
    dialed digits count */
typedef struct _TD_CID_DTMF_DIGITS_t
{
   /** array of dialed digits in ascii */
   IFX_char_t     acDialed[MAX_DIALED_NUM];
   /** number of dialed digits */
   IFX_uint8_t    nCnt;
   /** if IFX_TRUE then DTMF detecion
       on data channel connected to FXO is enabled */
   IFX_boolean_t    nDTMF_DetectionEnabled;
}TD_CID_DTMF_DIGITS_t;

/** Connection structure, represents abstraction object. Basically represents
    phone with phone channel and device on which phone events are recorded. */
struct _PHONE_t
{
#if (defined EASY336) || (defined XT16)
   /** connID */
   IFX_int32_t connID;
   /** lineID */
   IFX_int32_t lineID;
   /** array of voipIDs */
   IFX_int32_t *pVoipIDs;
   /** number of voipIDs in the array */
   IFX_int32_t nVoipIDs;
#endif /* (defined EASY336) || (defined XT16) */
   /** Sequential Conn ID number given to this connection - used for tracing. */
   IFX_uint32_t nSeqConnId;
#ifdef TD_RM_DEBUG
   /** number of allocated resources
      created for debug purposes */
   TD_RM_DEBUG_t oRM_Debug;
#endif /* TD_RM_DEBUG */
   /** Device number */
   IFX_uint16_t nDev;
   /** Phone number */
   IFX_int32_t        nPhoneNumber;
   /** Phone channel number */
   IFX_int32_t        nPhoneCh;
   /** Data channel number, phone channel is mapped to this data channel at
       start. Used for DTMF and other stuff. */
   IFX_int32_t        nDataCh;
   /** PCM channel number */
   IFX_int32_t        nPCM_Ch;
   /** Socket used for voice, data streaming */
   IFX_int32_t        nSocket;
   /** Device file descriptor this structure is connected to (events coming
       from phone channel. Basically first device is connected to first
       structure, second device to second structure, etc. */
   IFX_int32_t        nPhoneCh_FD;
   /** Device file descriptor this structure is connected to (events coming
       from data channel. */
   IFX_int32_t        nDataCh_FD;
#ifdef TD_DECT
   /** number of dect channel (used for DECT phones). */
   IFX_int32_t       nDectCh;
   /** type of phone (analog or dect) */
   PHONE_TYPE_t       nType;
   /** DECT channel activated flag */
   IFX_int32_t        bDectChActivated;
   /** Count of digits dialed on DECT handset to be handled. */
   IFX_int32_t        nDectDialNrCnt;
   /** Buffer with digits dialed on DECT handset to be handled. */
   IFX_char_t         pDectDialedNum[MAX_DIALED_NUM];
#endif /* TD_DECT */
   /** CID: description and number of source phone */
   IFX_TAPI_CID_MSG_t* pCID_Msg;

   /** Count of dialed numbers. */
   IFX_int32_t        nDialNrCnt;
   /** Array of dialed numbers. */
   IFX_char_t         pDialedNum[MAX_DIALED_NUM];

   /** Array of connections with other phones */
   CONNECTION_t rgoConn[MAX_PEERS_IN_CONF];

   /** Count of connections. This number - 1 represents number of
       active connections. Index to last one in array represents empty
       connection. */
   IFX_int32_t nConnCnt;

/* STATIC RM doesn't support conferencing */
   /** CONFERENCE, conference index which it belongs if greater than zero,
      otherwise does not belong to conference.
      When using to get conference structure
      use the nConfIdx value decrased by 1 */
   IFX_uint32_t nConfIdx;

   /** CONFERENCE, this phone started conference:
       IFX_FALSE - no,
       IFX_TRUE - yes */
   IFX_boolean_t nConfStarter;

   /** We called local peer?
       IFX_FALSE - no,
       IFX_TRUE - yes */
   IFX_boolean_t fLocalPeerCalled;

   /** We called external peer?
       IFX_FALSE - no,
       IFX_TRUE - yes */
   IFX_boolean_t fExtPeerCalled;

   /** We called PCM peer?
       IFX_FALSE - no,
       IFX_TRUE - yes */
   IFX_boolean_t fPCM_PeerCalled;

   /** If local call then this phone channel send us some action. Used when
    * removing local phone from conference. */
   CONNECTION_t* pConnFromLocal;

   /* To which board belongs this phone */
   BOARD_t* pBoard;

   /** FXO, this phone started FXO call:
       IFX_FALSE - no,
       IFX_TRUE - yes */
   IFX_boolean_t fFXO_Call;

   /** To which FXO belongs this phone */
   FXO_t* pFXO;

   /** True if we are in connection and pressed digit 'STAR' to make some
       adjustments. Otherwise false. */
   IFX_boolean_t fConfTAPI_InConn;

   /** For analog phones: holding true if phone is in wideband, otherwise false.
       This flag is also used when we end phone connection or exit from tapidemo
       to set back the default values (NarrowBand).
       For DECT phones: true if handset can use WideBand (can use
       IFX_DECTNG_CODEC_G722_64 codec) */
   IFX_boolean_t fWideBand_Cfg;

   /** Holding true if phone is in wideband for PCM calls, otherwise false.
       This flag is also used when we end phone connection to set back the
       default values (NarrowBand). */
   IFX_boolean_t fWideBand_PCM_Cfg;

   /** Holding true if phone AGC is configurd, otherwise false. This flag
       is also used when we end phone connection or exit from tapidemo to
       set back the default values (no AGC). */
   IFX_boolean_t fAGC_Cfg;

#ifdef EASY336
   /** true if CID detection is enabled on phone */
   IFX_boolean_t fCID_Enabled;
#endif /* EASY336 */
#ifdef EASY3111
   TD_EASY311_PHONE_SPECIFIC_t oEasy3111Specific;
#endif /* EASY3111 */
   /** Used when Ground Start support is enabled.
      If Ground Start support is enabled then the IFX_TAPI_EVENT_FAULT_LINE_GK_LOW
      must be received before off-hook. This variable is set to true when
      IFX_TAPI_EVENT_FAULT_LINE_GK_LOW is received. */
   IFX_boolean_t fHookOff_Allowed;

   /** Current state of phone. */
   STATE_MACHINES_t rgStateMachine[MAX_STATE_MACHINE];

   /** New internal FXS event to handle */
   INTERNAL_EVENTS_t nIntEvent_FXS;

   /** New internal config event to handle */
   INTERNAL_EVENTS_t nIntEvent_Config;

   /** Caller or Called - transmit and receive direction */
   CALL_DIRECTION_TYPE_t nCallDirection;

   /** Signals enabled for phone */
   IFX_TAPI_SIG_DETECTION_t stSignal;

   /** external peer name to use with CID */
   IFX_char_t szPeerName[TD_MAX_PEER_NAME_LEN];

   /** number of mappings for local calls that were done on this phone,
      used to decide if umapping should be done */
   IFX_int32_t nLocalMappingsCnt;
};


/** Connection structure, represents abstraction object. Basically represents
    FXO with channel fd. */
struct _FXO_t
{
   /** FXO number (internal phone number or timeslot index) which follows
       the last phone number */
   IFX_int32_t        nFXO_Number;
   /** pointer to FXO device data */
   TAPIDEMO_DEVICE_FXO_t* pFxoDevice;

   /** FXO channel */
   IFX_int32_t        nFxoCh;

   /** PCM channel number which is used for FXO connection on main board. */
   IFX_int32_t        nPCM_Ch;

   /** Data channel on main board used for CID RX between first and second ring.
       Afterthat this channel is released. */
   IFX_int32_t        nDataCh;

   /** Device file descriptor this structure is connected to (events coming
       from data channel. */
   IFX_int32_t        nFxoCh_FD;

   /** Current state of FXO. */
   STATE_MACHINE_STATES_t nState;

   /** information about detected dtmf digits - ETSI DTMF CID standard uses DTMF
       digits to send caller number */
   TD_CID_DTMF_DIGITS_t oCID_DTMF;

   /** CID message  */
   IFX_TAPI_CID_MSG_t oCID_Msg;

   /** Only one phone can be connected to this FXO. */
   CONNECTION_t oConn;

   /** Time when ringing event has occured. */
   IFX_time_t timeRingEventTimeout;

   /** Indication that first ring stop event has occured. Its use for
       CID RX FSK support. */
   IFX_boolean_t fFirstStopRingEventOccured;

   /** Indication that CID FSK is started */
   IFX_boolean_t fCID_FSK_Started;

   /** Sequential Conn ID number given to this connection - used for tracing. */
   IFX_uint32_t nSeqConnId;

   /* FXO direction marker IFX_TRUE when incoming call, when not IFX_FALSE
      used to distinguish with connection structure use */
   IFX_boolean_t fIncomingFXO;
};


/** represents startup mapping table */
/* mapping is done by RM */
typedef struct _STARTUP_MAP_TABLE_t
{
   /** Phone channel, index */
   IFX_int32_t nPhoneCh;

   /** Connected data channel, just index, if -1 no connection to data
       channel */
   IFX_int32_t nDataCh;

   /** Connected PCM channel, just index, if -1 no connection to PCM
       channel.
       NOTICE: PCM channel is not mapped at start, but when PCM call
       is started.  */
   IFX_int32_t nPCM_Ch;

} STARTUP_MAP_TABLE_t;


/** Structure holding status for data channel */
typedef struct _VOIP_DATA_CH_t
{
   /* mapping managed by RM */
   /** IFX_FALSE if its free, not mapped. Otherwise IFX_TRUE if mapped. */
   IFX_boolean_t fMapped;

   /** Status of encoder/decoder. IFX_TRUE - started, IFX_FALSE - stopped */
   IFX_boolean_t fCodecStarted;

   /** IFX_FALSE - don't start codec, IFX_TRUE - start codec */
   IFX_boolean_t fStartCodec;

   /** IFX_TRUE - default VAD configuration,
      IFX_FALSE - VAD configuration changed */
   IFX_boolean_t fDefaultVad;

   /** Which type of coder to use. */
   IFX_TAPI_ENC_TYPE_t nCodecType;

   /** Length of frame in miliseconds OR packetisation time. */
   IFX_int32_t nFrameLen;

   /** ITU-T I366.2 Bit Alignment for G.726 codecs (IETF RFC3550),
       Used only by G.726. */
   IFX_TAPI_COD_AAL2_BITPACK_t nBitPack;

   /** Which type of coder was used before fax/modem connection*/
   IFX_TAPI_ENC_TYPE_t nPrevCodecType;

   /** What length of frame was used before fax/modem connection*/
   IFX_int32_t nPrevFrameLen;

   /** Settings of LEC before Fax/modem connection */
   IFX_TAPI_WLEC_CFG_t oPrevLECcfg;

   /** IFX_FALSE if no need to restore channel after fax/modem transmission */
   IFX_boolean_t fRestoreChannel;

   /**Setting of Jitter buffor before Fax/modem connection */
   IFX_TAPI_JB_CFG_t tapi_jb_PrevConf;

#ifdef TAPI_VERSION4
   /** Device number */
   IFX_uint16_t nDev;

   /** Channel number */
   IFX_uint16_t nCh;

   /** Port number */
   IFX_uint16_t nUDP_Port;
#endif /* TAPI_VERSION4 */
} VOIP_DATA_CH_t;


/** Structure holding status for socket */
typedef struct _SOCKET_STAT_t
{
   /** IFX_FALSE if its free, not used. Otherwise IFX_TRUE if used. */
   IFX_boolean_t fUsed;

   /** Handle to socket */
   IFX_int32_t nSocket;

   /** Port used */
   IFX_int32_t nPort;
} SOCKET_STAT_t;


/* --------------------------------------------------------- */
/*                 MULTIPLE BOARDS START                     */
/* --------------------------------------------------------- */

/** Max supporting boards */
enum { MAX_BOARDS = 4 };

/** No extension board is detected */
#define NO_EXT_BOARD_DETECT -1

/** Max supporting phones on all boards together */
#ifndef EASY336
enum { MAX_ALL_PHONES = 16 };
#else /* EASY336 */
enum { MAX_ALL_PHONES = 96 };
#endif /* EASY336 */

/** Types of board */
typedef enum
{
   TYPE_VINETIC = 0,
   TYPE_DANUBE = 1,
   TYPE_DUSLIC_XT = 2,
   TYPE_DUSLIC = 3,
   TYPE_VINAX = 4,
   TYPE_AR9 = 5,
   TYPE_SVIP = 6,
   TYPE_XT16 = 7,
   TYPE_TERIDIAN = 8,
   TYPE_VR9 = 9,
   TYPE_XWAY_XRX300 = 10,
   TYPE_NONE = 11
} BOARD_TYPE_t;

/** Types of FXO devices */
typedef enum
{
   FXO_TYPE_DUSLIC = 0,
   FXO_TYPE_TERIDIAN = 1,
   FXO_TYPE_SLIC121 = 2,
   FXO_TYPE_NONE = 3
} FXO_TYPE_t;

/** Types of devices */
typedef enum
{
   TAPIDEMO_DEV_CPU = 1,
   TAPIDEMO_DEV_FXO = 2,
   TAPIDEMO_DEV_NONE = 3
} TAPIDEMO_DEVICE_TYPE_t;

/** Types of chip with version */
typedef enum
{
   TD_CHIP_TYPE_UNKNOWN = 0,
   TD_CHIP_TYPE_DXT_V1_3 = 1,
   TD_CHIP_TYPE_DXT_V1_4 = 2
} TD_CHIP_TYPE_t;

/** Max. len of device path (/dev/vin...) */
enum { DEV_NAME_LEN = 32 };

/** Structure representing FXO device. */
struct _TAPIDEMO_DEVICE_FXO_t
{
   /** FXO device. Can be duslic or teridian */
   FXO_TYPE_t nFxoType;

   /** pointer to FXO name string */
   IFX_char_t* pFxoTypeName;

   /** FXO device number of given type counted from 1 */
   IFX_int32_t nDevNum;

   /** number of FXO channels available for this device */
   IFX_int32_t nNumOfCh;

   /** FXO device FD */
   IFX_int32_t nDevFD;

   /** Country selection. */
   IFX_uint8_t nCountry;

   /** pointer to table of FXO channel structures, allocated during
       initializtion */
   FXO_t* rgoFXO;

   /** last event on FXO device */
   IFX_TAPI_EVENT_t oTapiEventOnFxo;

   /** string used to adjust indention in traces */
   IFX_char_t* pIndent;

   /* To which board belongs this FXO */
   BOARD_t* pBoard;
};

/** Structure representing CPU device. */
struct _TAPIDEMO_DEVICE_CPU_t
{
   /** FW files data */
   IFX_char_t* pszDRAM_FW_File;

   /** FW files code */
   IFX_char_t* pszPRAM_FW_File[TD_PRAM_FW_FILE_MAX];
   /** number of available FW file names */
   IFX_int32_t nPRAM_FW_FileNum;

   /** FW file optional name specified, but board can initialize without it,
    * this can happen for DxT where FW is located ROM */
   IFX_boolean_t bPRAM_FW_FileOptional;

   /** Coefficient file (BBD, CRAM) */
   IFX_char_t* pszBBD_CRAM_File[2];

   /** Pointer to device dependent structure */
   IFX_void_t* pDevDependStuff;

   /** Access mode for device. */
   IFX_int32_t AccessMode;

   /** Device physical base address. */
   unsigned long nBaseAddress;

   /** Device irq number. */
   IFX_int32_t nIrqNum;

   /** chip type with version */
   TD_CHIP_TYPE_t nChipSubType;

   /** Pointer to function which initializes device */
   IFX_return_t (*Init)       (BOARD_t* pBoard, IFX_char_t* pPath);

   /** Pointer to function which read verion */
   IFX_return_t (*Version)    (BOARD_t* pBoard);

   /** Pointer to function which Handles the DRIVER error codes and stack */
   IFX_void_t   (*DecodeErrno)(IFX_int32_t nCode, IFX_uint32_t nSeqConnId);

   /** Pointer to function which returns number of last driver error */
   IFX_return_t   (*GetErrno) (IFX_int32_t nFd);

   /** Pointer to function which fills in firmware pointers */
   IFX_return_t (*FillinFwPtr) (BOARD_TYPE_t nChipType, IFX_void_t* pDrvInit,
                                TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                IFX_char_t* pPath);

   /** Pointer to function which releases firmware pointers */
   IFX_void_t  (*ReleaseFwPtr) (IFX_void_t* pDrvInit);
};

/** Structure representing device. */
typedef struct _TAPIDEMO_DEVICE_t
{
   /** device type (CPU or FXO) */
   TAPIDEMO_DEVICE_TYPE_t nType;

   /** Pointer to next device structure. */
   struct _TAPIDEMO_DEVICE_t *pNext;

   union
   {
      TAPIDEMO_DEVICE_CPU_t   stCpu;
      TAPIDEMO_DEVICE_FXO_t   stFxo;
   } uDevice;
} TAPIDEMO_DEVICE_t;

#ifdef EVENT_COUNTER
   /* debugging feature checking if events have been lost between
      TAPI driver and tapidemo. */
typedef struct EventSeqNo
{
   /** event sequence number of the next device specific event. */
   IFX_uint32_t nEventDevSeqNo;
   /** event sequence number of the next channel specific event. */
   IFX_uint32_t pEventChSeqNo[32];
} EventSeqNo_t;
#endif

/** Structure representing board and info about it. */
struct _BOARD_t
{
   /** Board idx */
   IFX_int32_t nBoard_IDX;

   /** Board id, used to open right device */
   IFX_int32_t nID;

   /** Board type */
   BOARD_TYPE_t nType;

   /** Board name */
   const char* pszBoardName;

   /** Prefix used to adjust indention in traces */
   IFX_char_t* pIndentBoard;

   /** Prefix used to adjust indention in traces */
   IFX_char_t* pIndentPhone;

   /** Number of analog ch on board. */
   IFX_int32_t nMaxAnalogCh;

   /** Max number of phones (analog + DECT) */
   IFX_int32_t nMaxPhones;

   /** Number of analog ch on board. */
   IFX_int32_t nMaxDectCh;

   /** Number of coder, signalling (data) ch on board. */
   IFX_int32_t nMaxCoderCh;

   /** Number of T.38 channels. */
   IFX_int32_t nMaxT38;

#ifdef SLIC121_FXO
   /** Number of FXO channels supported by FW (currently only in SLIC121) */
   IFX_int32_t nMaxFxo;
#endif /* SLIC121_FXO */

   /** Number of used channels for open/close, events, streaming, ...
       Used, because some systems have only analog channels, coder channels are
       not present. In SVIP this field is preserved, but not used so often
       as before, nMaxAnalogCh or nMaxCoderCh is used instead */
   IFX_int32_t nUsedChannels;

   /** Number of pcm ch on board */
   IFX_int32_t nMaxPCM_Ch;

   /** System file descriptor */
   IFX_int32_t nSystem_FD;

   /** Usage of Sys FD, 0 - not used */
   IFX_boolean_t nUseSys_FD;

   /** Device control file descriptor */
   IFX_int32_t nDevCtrl_FD;

   /** Channel file descriptors */
   IFX_int32_t* nCh_FD;

   /** if IFX_TRUE, T.38 is supported */
   IFX_boolean_t nT38_Support;

#if (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /** Capabilities of the T.38 stack implementation. */
   IFX_TAPI_T38_CAP_t stT38Cap;

   /** T.38 configuration */
   TD_T38_CONFIG_t oT38_Config;
#endif /* (defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

   /** Device name without numbers, like /dev/vmmc, /dev/vin, ... */
   IFX_char_t pszChDevName[DEV_NAME_LEN];

   /** Device control name numbers, like /dev/vmmc[board id]0, ... */
   IFX_char_t pszCtrlDevName[DEV_NAME_LEN];

   /** System device name, like /dev/easy3332/[board id], ... */
   IFX_char_t pszSystemDevName[DEV_NAME_LEN];

   /** Number of chips on board */
   IFX_int32_t nChipCnt;

   /** Startup mapping table */
   const STARTUP_MAP_TABLE_t* pChStartupMapping;

   /** Custom mapping table */
   STARTUP_MAP_TABLE_t* pChCustomMapping;

   /** Idication that Coefficient file (BBD, CRAM) was changed */
   IFX_boolean_t fBBD_Changed;

   /** Status array of data channels */
   VOIP_DATA_CH_t* pDataChStat;

   /** Default RTP and RTCP settings */
   IFX_TAPI_PKT_RTP_CFG_t rtpConf;

   /** Status of used pcm channels, IFX_FALSE - used, IFX_TRUE - not used */
   IFX_boolean_t* fPCM_ChFree;

   /** Socket on which we expect incoming data (events and voice)
       Also used to send data to external peer. There is linear
       transformation  between sockets and data channel. Socket
       with index 1 belongs to data channel 1, socket with index
       2 to data channel 2, etc. */
   IFX_int32_t* rgnSockets;
#ifdef TD_IPV6_SUPPORT
   IFX_int32_t* rgnSocketsIPv6;
#endif /* TD_IPV6_SUPPORT */

   /** Hold array of phones */
   PHONE_t* rgoPhones;

   /** Pointer to control structure */
   CTRL_STATUS_t *pCtrl;

   /** Pointer to device info structure */
   TAPIDEMO_DEVICE_t *pDevice;

   /** used to store board event data */
   IFX_TAPI_EVENT_t tapiEvent;

#ifdef TAPI_VERSION4
   /** Holding true if there is a single file descriptor */
   IFX_boolean_t fSingleFD;

   /** Holding true if sockets are used for voice streaming through codecs */
   IFX_boolean_t fUseSockets;

   /** Holding amount of data channels on device */
   IFX_int32_t nDataChOnDev; 
#endif /* TAPI_VERSION4 */

   /** Pointer to function which initializes board */
   IFX_return_t (*Init)       (BOARD_t* pBoard, PROGRAM_ARG_t* pProgramArg);

   /** Pointer to function which release board structure */
   IFX_void_t   (*RemoveBoard) (BOARD_t* pBoard);
#ifdef EVENT_COUNTER
   EventSeqNo_t evDbg[3];
#endif

#ifdef USE_FILESYSTEM
   /** Pointer to keep FW filename value from .tapidemorc. */
   IFX_char_t *pFWFileName;
   /** Pointer to keep DRAM filename value from .tapidemorc. */
   IFX_char_t *pDRAMFileName;
   /** Pointer to keep BBD filename value from .tapidemorc. */
   IFX_char_t *pBBDFileName;
#endif /* USE_FILESYSTEM */
};

/** Max combinations */
enum { MAX_COMBINATIONS = 12 };

/** Max boards in one combination */
enum { MAX_BOARD_IN_COMBINATION = MAX_BOARDS };

/** structure for different board combinations */
typedef struct _BOARD_COMBINATION_ELEM_t
{
   /** board ID */
   IFX_int32_t nBoard_ID;
   /** board type */
   BOARD_TYPE_t nBoardType;
} BOARD_COMBINATION_ELEM_t;



/** Structure holding combination */
typedef struct _BOARD_COMBINATION_t
{
   /** identifier */
   IFX_int32_t nIdx;
   /** number of boards */
   IFX_int32_t nBoardCnt;
   /** array with boards parameters */
   BOARD_COMBINATION_ELEM_t nBoards[MAX_BOARD_IN_COMBINATION];
} BOARD_COMBINATION_t;

/* --------------------------------------------------------- */
/*                 MULTIPLE BOARDS END                       */
/* --------------------------------------------------------- */


/** Flags for command line arguments */
typedef struct _ARG_FLAGS_t
{
   /** If 1 then CID is send when calling other phone, but only local */
   IFX_uint32_t nCID : 1;
   /** If 1 then conference is used */
   IFX_uint32_t nConference : 1;
   /** If 1 then help is displayed */
   IFX_uint32_t nHelp : 1;
   /** If 1 then version is displayed */
   IFX_uint32_t nVersion : 1;
#ifdef LINUX
   /** If 1 then configuration is displayed */
   IFX_uint32_t nConfigure : 1;
#endif /* LINUX */
   /** If 1 then wait after each state machine step */
   IFX_uint32_t nWait : 1;
   /** If 1 then download CRAM file */
   /*IFX_uint32_t nCRAM : 1;*/
   /** 1 if quality of service is used, otherwise 0 */
   IFX_uint32_t nQos : 1;
   /** 1 if quality of service is used, otherwise 0 */
   IFX_uint32_t nQosSocketStart : 1;
#ifdef TD_IPV6_SUPPORT
   /** 1 if quality of service is used, otherwise 0 */
   IFX_uint32_t nAddrIsSetIPv6 : 1;
   /** 1 if quality of service is used, otherwise 0 */
   IFX_uint32_t nUseIPv6 : 1;
#endif /* TD_IPV6_SUPPORT */
   /** 1 if PCM is used and we are master, otherwise 0 */
   IFX_uint32_t nPCM_Master : 1;
   /** 1 if PCM is used and we are slave, otherwise 0  */
   IFX_uint32_t nPCM_Slave : 1;
   /** 1 for new PCM coder type, otherwise 0  */
   IFX_uint32_t nPCM_TypeDef : 1;
   /** 1 for new encoder type, otherwise 0  */
   IFX_uint32_t nEncTypeDef : 1;
   /** 1 for new packetisation time, otherwise 0  */
   IFX_uint32_t nFrameLen : 1;
   /** 1 for calls to different network, otherwise 0  */
   IFX_uint32_t nDiffNet : 1;
   /** 1 if FXO is used, otherwise 0 */
   IFX_uint32_t nFXO : 1;
   /** 1 if local UDP will be tested, otherwise 0 - external UDP is used */
   IFX_uint32_t nQOS_Local : 1;
   /** 1 if local loop PCM will be used, otherwise 0 */
   IFX_uint32_t nPCM_Loop : 1;
   /** 1 to use coders for local connection, 0 to use phone mapping */
   IFX_uint32_t nUseCodersForLocal : 1;
   /** 1 to use PCM wideband, 0 to use PCM narrowband */
   IFX_uint32_t nUsePCM_WB : 1;
   /** 1 to run Tapidemo as daemon */
   IFX_uint32_t nDaemon : 1;
   /** path to download has been changed */
   IFX_uint32_t nUseCustomDownloadsPath : 1;
   /** do not use LEC - obsolete */
   IFX_uint32_t nNoLEC : 1;
   /** 1 if Ground Start is supported */
   IFX_uint32_t nGroundStart : 1;
#ifdef DXT
   /** 1 if use polling mode in LL driver - now only for DxT */
   IFX_uint32_t nPollingMode : 1;
#endif /* DXT */
   /** if not 0 then DECT is not initialized */
   IFX_uint32_t nNoDect : 1;
   /** if not 0 flag -b is not used */
   IFX_uint32_t nUseCombinationBoard : 1;
   /** 1 if Phone Plug Detection is disabled */
   IFX_uint32_t nDisablePpd : 1;
#ifdef TD_IPV6_SUPPORT
   /** path to phonebook has been changed */
   IFX_uint32_t nUseCustomPhonebookPath : 1;
#endif /* TD_IPV6_SUPPORT */
#ifdef TD_RM_DEBUG
   /** if not 0 resource counting results are printed
       created for debug purposes */
   IFX_uint32_t nRM_Count : 1;
#endif /* TD_RM_DEBUG */
#ifdef DXT
   /** do not download FW for DxT */
   IFX_uint32_t nDxT_RomFW : 1;
#endif /* DXT */
} ARG_FLAGS_t;


/** type of phone port */
typedef enum _TAPIDEMO_PORT_TYPE_t
{
   /** FXO port */
   PORT_FXO,
   /** FXS port */
   PORT_FXS
} TAPIDEMO_PORT_TYPE_t;

/** port data structure */
typedef struct _TAPIDEMO_PORT_DATA_t
{
   /** port type(fxo/fxs) */
   TAPIDEMO_PORT_TYPE_t nType;
   /** port data */
   union
   {
      /** FXS data */
      PHONE_t* pPhopne;
      /** FXO data */
      FXO_t*   pFXO;
   } uPort;
} TAPIDEMO_PORT_DATA_t;

/** Test types performed by test script on Control PC*/
typedef enum _COM_TEST_TYPE_t
{
   COM_NO_TEST = -1,
   COM_MODULAR_TEST = 0,
   COM_BULK_CALL_TEST = 1 /* default */
} COM_TEST_TYPE_t;

/** Modular subtest types performed by test script on Control PC*/
typedef enum _COM_SUB_TEST_TYPE_t
{
   /** default */
   COM_SUBTEST_NONE              = 0x0000,
   COM_SUBTEST_HOOKSTATE         = 0x0001,
   COM_SUBTEST_HOOKFLASH         = 0x0002,
   COM_SUBTEST_PULSEDIGIT        = 0x0004,
   COM_SUBTEST_DTMFDIGIT         = 0x0008,
   COM_SUBTEST_CONFERENCE        = 0x0010,
   COM_SUBTEST_FXO               = 0x0020,
   COM_SUBTEST_FAX               = 0x0040,
   COM_SUBTEST_MODEM             = 0x0080,
   COM_SUBTEST_FAX_T_38          = 0x0100,
   COM_SUBTEST_SYSINIT           = 0x0200,
   COM_SUBTEST_CID               = 0x0400,
   COM_SUBTEST_LEC               = 0x0800,
   COM_SUBTEST_CONFIG_DETECTION  = 0x1000
} COM_SUB_TEST_TYPE_t;

/** holds received from test script CID data */
typedef struct _TD_CID_TEST_DATA_t
{
   /** Caller ID structure pointer */
   IFX_TAPI_CID_MSG_t* pCID_Msg;
   /** number of phone on which CID will be played */
   IFX_int32_t nPhoneNumber;
   /** IFX_TRUE if number from test script is set */
   IFX_boolean_t bNumberIsSet;
   /** IFX_TRUE if name from test script is set */
   IFX_boolean_t bNameIsSet;
} TD_CID_TEST_DATA_t;

/** LEC test data */
typedef struct _TD_LEC_TEST_DATA_t
{
   /** LEC settings */
   IFX_TAPI_WLEC_CFG_t oLEC_Settings;
   /** event IFX_TAPI_EVENT_FAXMODEM_CED was detected */
   IFX_boolean_t bDetecedEventCED;
   /** event IFX_TAPI_EVENT_FAXMODEM_PR was detected */
   IFX_boolean_t bDetecedEventPR;
   /** event IFX_TAPI_EVENT_FAXMODEM_CEDEND was detected */
   IFX_boolean_t bDetecedEventCEDEND;
   /** LEC already was reseted */
   IFX_boolean_t bLEC_Disabled;
} TD_LEC_TEST_DATA_t;

/** List of supported FXO devices. */
typedef enum _TD_FXO_DEV_t
{
   /** teridian */
   TD_FXO_TER             = 0x01,
   /** duslic, Clare */
   TD_FXO_DUS             = 0x02,
   /** SLIC 121 */
   TD_FXO_SLIC121         = 0x04
} TD_FXO_DEV_t;

/** Arguments used for '-x' option. */
#define TD_CMD_ARG_FXO_TERIDIAN   "t"
#define TD_CMD_ARG_FXO_DUSLIC     "d"
#define TD_CMD_ARG_FXO_SLIC121    "s"

/** Pipe names */
#define LOG_THREAD_SYNCH_PIPE "td_log_synch_pipe"
#define SOCKET_THREAD_SYNCH_PIPE "td_socket_synch_pipe"
#define REDIRECT_LOG_PIPE "td_redirect_log_pipe"
#define DECT_PIPE "td_dect_pipe"

/** Structure holding pipe */
typedef struct _TD_PIPE_t
{
   /** Pipe names */
   IFX_char_t rgName[30];
   /** Array with file pointers to pipe */
   TD_OS_Pipe_t* rgFp[2];
   /** Array with file descriptors to pipe */
   IFX_int32_t rgFd[2];
} TD_PIPE_t;
#ifdef TD_IPV6_SUPPORT
/** Structure IPv6 configuration */
typedef struct _TD_IPV6_CTRL_t
{
    /** Contains board's IP address for network connections */
    TD_OS_sockAddr_t oAddrIPv6;
    /** address is set flag IFX_TRUE is address is already set */
    IFX_boolean_t nAddrIsSet;
    /** Socket fd for IPv6 */
    TD_OS_socket_t nSocketFd;
} TD_IPV6_CTRL_t;
#endif /* TD_IPV6_SUPPORT */

#ifdef TD_PPD
/* Flags used for PPD arguments. */
#define TD_PPD_FLAG_T1     0x1
#define TD_PPD_FLAG_T2     0x2
#define TD_PPD_FLAG_T3     0x4
#define TD_PPD_FLAG_CAP    0x8
#endif /* TD_PPD */

/** Demo structure holding command line arguments as flags */
struct _PROGRAM_ARG_t
{
    /** Set parameters */
    ARG_FLAGS_t oArgFlags;
    /** Contains IPv4 address for connection in network format */
    TD_OS_sockAddr_t oMy_IP_Addr;
#ifdef TD_IPV6_SUPPORT
    /** Contains IPv6 address for connection in network format */
    TD_OS_sockAddr_t oMy_IPv6_Addr;
#endif /* TD_IPV6_SUPPORT */
#ifdef LINUX
    /* name of used network interface (used by admin socket) */
    IFX_char_t aNetInterfaceName[TD_MAX_NAME];
#endif /* LINUX */
    /** Contains first three numbers of IP address when calling different
        network. Syntax: xxx.xxx.xxx{.yyy - added by us} */
    struct in_addr oDiffNetIP;
    /** Path to FW, BBD filenames to download */
    IFX_char_t sPathToDwnldFiles[MAX_PATH_LEN];
#ifdef USE_FILESYSTEM
    /** FW filename  */
    IFX_char_t sFwFileName[MAX_PATH_LEN];
    /** DRAM filename */
    IFX_char_t sDramFileName[MAX_PATH_LEN];
    /** BBD filename */
    IFX_char_t sBbdFileName[MAX_PATH_LEN];
#endif /* USE_FILESYSTEM */
    /** Board combination */
    IFX_int32_t nBoardComb;
    /** Debug, trace level */
    DEBUG_LEVEL_t nDbgLevel;
#ifdef EASY336
    /** RM debug, trace level */
    DEBUG_LEVEL_t nRmDbgLevel;
#endif /* EASY336 */
    /** TAPI debug trace level */
    IFX_TAPI_DEBUG_REPORT_SET_t nTapiDbgLevel;
#ifdef TD_DECT
    /** DECT debug trace level of DECT process */
    DEBUG_LEVEL_t nDectDbgLevel;
#endif /* TD_DECT */
#ifdef TD_PPD
    /** Flags for PPD arguments. Indicates arguments which are changed by
        user. */
    IFX_uint32_t nPpdFlag;
    /** Timeout for T1 timer. */
    IFX_uint32_t nTimeoutT1;
    /** Timeout for T2 timer. */
    IFX_uint32_t nTimeoutT2;
    /** Timeout for T3 timer. */
    IFX_uint32_t nTimeoutT3;
    /** Telephone capacitance threshold. */
    IFX_uint32_t nCapacitance;
#endif /* TD_PPD */
    /** CID standard - this variable should be only used to get command line
        arguement and set default value, to check current CID configuration use
        function CID_GetStandard() */
    IFX_int32_t nCIDStandard;
    /** Encoder type */
    IFX_int32_t nEnCoderType;
    /** Packetisation time */
    IFX_int32_t nPacketisationTime;
    /** Voice Activity Detection (VAD) settings */
    IFX_TAPI_ENC_VAD_t nVadCfg;
    /** LEC type */
    IFX_TAPI_WLEC_TYPE_t  nLecCfg;
#ifdef TD_DECT
    /** Echo Canceller (EC) type selection for DECT */
    IFX_TAPI_EC_TYPE_t nDectEcCfg;
#endif
    /** Trace redirection */
    TRACE_REDIRECTION_t nTraceRedirection;
    /** FXO devices which should be used. */
    IFX_uint32_t nFxoMask;
#ifdef TD_IPV6_SUPPORT
    /** Path to phone book */
    IFX_char_t sPathToPhoneBook[MAX_PATH_LEN];
#endif /* TD_IPV6_SUPPORT */
};

/** Multithreading control structure */
typedef struct _TD_MULTITHREAD_CTRL_t
{
   struct
   {
      /** Mutex used to lock state machine */
      TD_OS_mutex_t mutexStateMachine;
   } oMutex;
   struct
   {
      /** Lock used for synchronization socket thread with the main thread */
      TD_OS_lock_t oSocketThreadStopLock;
      /** Lock used for synchronization log thread with the main thread */
      TD_OS_lock_t oLogThreadStopLock;
   } oLock;
   struct
   {
      /** Socket thread control stucture */
      TD_OS_ThreadCtrl_t oHandleSocketThreadCtrl;
      /** set to IFX_TRUE if thread is started, otherwise set to IFX_FALSE */
      IFX_boolean_t bSocketThreadStarted;
      /** Log thread control stucture */
      TD_OS_ThreadCtrl_t oHandleLogThreadCtrl;
      /** set to IFX_TRUE if thread is started, otherwise set to IFX_FALSE */
      IFX_boolean_t bLogThreadStarted;
   } oThread;
   struct
   {
      /** Pipe used for synchronization socket thread with the main thread */
      TD_PIPE_t oSocketSynch;
      /** Pipe used for synchronization log thread with the main thread */
      TD_PIPE_t oLogSynch;
      /** Pipe used to redirect log */
      TD_PIPE_t oRedirectLog;
      /** Pipe used for communication from DECT stack */
      TD_PIPE_t oDect;
   } oPipe;
} TD_MULTITHREAD_CTRL_t;

/** Types of msg used for synch threads */
typedef enum
{
   /** Update timeout */
   TD_SYNCH_THREAD_MSG_SET_TIMEOUT = 1,
   /** Stop thread */
   TD_SYNCH_THREAD_MSG_STOP = 2
} TD_SYNCH_THREAD_MSG_t;

/** Specific which select() is used */
typedef enum
{
   /** Use for device select */
   TD_SELECT_DEVICE,
   /** Use for socket select */
   TD_SELECT_SOCKET
} TD_SELECT_TYPE_t;

/** EOL */
#define TD_EOL '\n'

#ifdef TD_IPV6_SUPPORT
/** Phonebook size - counting from 00 to 99 */
enum { TD_PHONEBOOK_SIZE = 100 };

/** Phonebook prefix size.
    It set for 3 because phonebook can have max 99 entries + '\n'. */
enum { TD_PHONEBOOK_PREFIX_LEN = 3 };

/** Phonebook boardname size */
enum { TD_PHONEBOOK_BOARDNAME_LEN = 18 };

/** Multicast address - range of the group of all IPv6 routers
    within scope 2 (link-local) - according to RFC 4291 */
#define TD_MULTICAST_ADDR "FF02::2"

/** Broadcast port */
#define TD_BROAD_CAST_PORT "9876"
/** Multicast port */
#define TD_MULTI_CAST_PORT "9877"

/** Phonebook structure */
typedef struct _TD_PHONEBOOK_t
{
   /** Board's prefix */
   IFX_char_t rgPrefix[TD_PHONEBOOK_PREFIX_LEN];
   /** Board's IP address */
   IFX_char_t rgAddressIP[TD_ADDRSTRLEN];
   /** Board's name */
   IFX_char_t rgBoardname[TD_PHONEBOOK_BOARDNAME_LEN];
} TD_PHONEBOOK_t;

/** Broadcast or multicast settings */
typedef struct _TD_BROAD_MULTI_CAST_t
{
   /** Socket */
   TD_OS_socket_t nSocket;
   /** Address info structure. */
   struct addrinfo* aiAddrInfo;
} TD_BROAD_MULTI_CAST_t;

#endif /* TD_IPV6_SUPPORT */

/** Internal Test Module control structure */
typedef struct _TD_ITM_CONTROL_t
{
   /** Is Internal Test Module enabled flag. */
   IFX_boolean_t fITM_Enabled;
   /** Socket used for communication with control PC (incoming) */
   TD_OS_socket_t nComInSocket;
   /** Port used for communication with control PC (incoming) */
   IFX_int32_t nComInPort;
   /** Socket used for communication with control PC (outgoing) */
   TD_OS_socket_t nComOutSocket;
   /** Port used for communication with control PC (outgoing) */
   IFX_int32_t nComOutPort;
   /** BroadCast data, broadcast is used by control PC  */
   struct
   {
      /** Socket used for receiving broadcast messages */
      TD_OS_socket_t nSocket;
      /** Is verify system init enabled flag. */
      TD_OS_sockAddr_t oBrCastAddr;
   } oBroadCast;
   /** Reset communication - close sockets to
       open them again with new address or sockets */
   IFX_boolean_t bCommunicationReset;
   /** Change address IP (request to send IP was send) */
   IFX_boolean_t bChangeIPAddress;
   /** IFX_TRUE if there was no problem with using custom path */
   IFX_boolean_t bNoProblemWithDwldPath;
   /** Verify System Init modular test data */
   struct
   {
      /** Expected command IDs from Control PC */
      IFX_int32_t nExpectedID;
      /** Command IDs received from Control PC */
      IFX_int32_t nReceivedID;
      /** Used to hold test data */
      IFX_char_t aParametrs[TD_TWO_UINT32_STRING_LEN];
      /** Waiting for file download. */
      IFX_boolean_t fFileDwnld;
      /** Is verify system init enabled flag. */
      IFX_boolean_t fEnabled;
   } oVerifySytemInit;
   /** Remote (send to control PC) debug, trace level */
   DEBUG_LEVEL_t nComDbgLevel;
   /** Test type beeing performed (bulk call or modular) */
   COM_TEST_TYPE_t nTestType;
   /** Modular sub test type beeing performed */
   IFX_uint32_t nSubTestType;
   /** if IFX_TRUE use custom CPTs */
   IFX_boolean_t nUseCustomCpt;
   /** array with tapidemo version */
   IFX_char_t rgoTapidemoVersion[TD_MAX_NAME];
   /** array with ITM version */
   IFX_char_t rgoITM_Version[TD_MAX_NAME];
   /** array with firmware version */
   IFX_char_t rgoFW_Version[TD_MAX_NAME];
   /** Array with program options (ARGV). */
   IFX_char_t** rgsArgv;
   /** Number of program arguments (ARGC). */
   IFX_int32_t nArgc;
   /** Default program arguments. */
   PROGRAM_ARG_t oDefaultProgramArg;
   /** Caller Id test data used for test. */
   TD_CID_TEST_DATA_t oCID_Test;
   /** LEC data test used for test. */
   TD_LEC_TEST_DATA_t oLEC_Test;
} TD_ITM_CONTROL_t;

#ifdef TD_IPV6_SUPPORT
/** cast type */
typedef enum
{
   /** BroadCast */
   TD_CAST_BROAD = 0,
   /** MultiCast */
   TD_CAST_MULTI = 1,
   /** Number of cast types */
   TD_CAST_MAX = 2
} TD_CAST_TYPE_t;
#endif /* TD_IPV6_SUPPORT */

/** Control structure for all channels, holding status of connections and
    also program arguments */
struct _CTRL_STATUS_t
{
   /** Holds array of boards */
   BOARD_t rgoBoards[MAX_BOARDS];
   /** Holds array of conferences */
   /* conferencing not supported by static RM */
   CONFERENCE_t rgoConferences[MAX_CONFERENCES];
   /** Conference index, if 0 no conference yet */
   IFX_uint32_t nConfCnt;
   /** Program arguments */
   PROGRAM_ARG_t* pProgramArg;
   /** Holds control device file descriptor, like for /dev/vin10, but
       highest file descriptor, used for FD_ISSET */
   IFX_int32_t nMaxFdDevice;
   /** Holds control device file descriptor, like for /dev/vin10, but
       highest file descriptor, used for FD_ISSET */
   IFX_int32_t nMaxFdSocket;
   /** Contains IP address for connection but as number */
   IFX_uint32_t nMy_IPv4_Addr;
   /** Contains our ip address */
   TD_OS_sockAddr_t oTapidemo_IP_Addr;
#ifdef TD_IPV6_SUPPORT
   /** IPv6 specific configuration */
   TD_IPV6_CTRL_t oIPv6_Ctrl;
#endif /* TD_IPV6_SUPPORT */
   /** Contains our MAC address */
   IFX_uint8_t oTapidemo_MAC_Addr[ETH_ALEN];
#if (defined EASY336 || defined XT16)
   /** Contains veth0 ip address */
   TD_OS_sockAddr_t oVeth0_IP_Addr;
   /** Contains veth0 MAC address */
   IFX_uint8_t oVeth0_MAC_Addr[ETH_ALEN];
#endif /* (defined EASY336 || defined XT16) */
   /** Boards in combination */
   IFX_int32_t nBoardCnt;
   /** Sum of all data channels on all boards. */
   IFX_int32_t nSumCoderCh;
   /** Sum of all phones on all boards (analog + DECT). */
   IFX_int32_t nSumPhones;
   /** Sum of all pcm channels on all boards. */
   IFX_int32_t nSumPCM_Ch;
   /** Max number of FXOs */
   IFX_int32_t nMaxFxoNumber;

   /** Socket on which we expect incoming data (events and voice)
       Also used to send data to external peer. */
   SOCKET_STAT_t* rgnSockets;
#ifdef TD_IPV6_SUPPORT
   SOCKET_STAT_t *rgnSocketsIPv6;
#endif /* TD_IPV6_SUPPORT */

#ifdef TD_IPV6_SUPPORT
   /** Phone book */
   TD_PHONEBOOK_t rgoPhonebook[TD_PHONEBOOK_SIZE];
   /** number of phone book entries */
   IFX_int32_t nPhonebookIndex;
   /** Flag to disable phone book printing. This is needed becasue, by default,
       phone book is printed after adding each new entry. During initialization
       many boards are added by receiving response to broadcast/multicast.
       Thanks to this flag phone book is printed only once during
       initialization. */
   IFX_boolean_t bPhonebookShow;
   /** Broadcast and multicast settings */
   TD_BROAD_MULTI_CAST_t rgoCast[TD_CAST_MAX];
#endif /* TD_IPV6_SUPPORT */

   /** Socket used for administration of calls (make/end/...) */
   TD_OS_socket_t nAdminSocket;
#ifdef TD_FAX_MODEM
#if (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /** fax test control structure */
   TD_T38_FAX_TEST_CTRL_t oFaxTestCtrl;
#endif /* (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
   /* used in tapidemo fax transmission mode */
   TD_FAX_MODE_t nFaxMode;
#endif /* TD_FAX_MODEM */
   /* socket used for syslog */
   IFX_int32_t nSyslogSocket;
   /** socket used for syslog */
   IFX_int32_t nSyslogChildPid;
   /** set if internal event generated from local phone or fxo,
       this flag is set together with connection pointer on destination phone
       e.g. p_dst_phone->pConnFromLocal = p_conn */
   IFX_boolean_t bInternalEventGenerated;
   /** flag with number of resources(phones and FXO channels)
       that need timeout check */
   IFX_int32_t nUseTimeoutDevice;
   /** flag with number of resources(phones and FXO channels)
       that need timeout check */
   IFX_int32_t nUseTimeoutSocket;
   /** Multithreading control structure */
   TD_MULTITHREAD_CTRL_t oMultithreadCtrl;
#ifdef EASY336
   /** pointer to array with LineID count for ConnIDs */
   TD_CONN_ID_LINE_ID_CNT_t* pConnID_LinID_Cnt;
#endif /* EASY336 */

#ifdef TD_TIMER
   /** timer FD */
   IFX_int32_t nTimerFd;
   /** timer FIFO queue FD */
   IFX_int32_t nTimerMsgFd;
#endif /* TD_TIMER */

#ifdef TD_DECT
   /** FD of pipe to read DECT messages */
   IFX_int32_t nDectPipeFdOut;
   /** DECT stack control data */
   TD_DECT_STACK_CTRL_t oDectStackCtrl;
   /** paging key FD */
   IFX_int32_t viPagingKeyFd;
   /** Communication socket to receive data For Production Test Tool */
   IFX_int32_t viFromCliFd;
#endif /* TD_DECT */
   /** Internal Test Module control data */
   TD_ITM_CONTROL_t oITM_Ctrl;
   /** Sequential Conn ID number that will be used by new connection
       - used for tracing. */
   IFX_uint32_t nSeqConnIdCnt;
};

/** types of data in COMM_MSG_DATA_1_t */
typedef enum _TD_COMM_MSG_DATA_1_TYPE_t
{
   /** data isn't set */
   TD_COMM_MSG_DATA_1_NONE = 0,
   /** CID data */
   TD_COMM_MSG_DATA_1_CID = 1,
   /** Conn ID data */
   TD_COMM_MSG_DATA_1_CONN_ID = 2
}TD_COMM_MSG_DATA_1_TYPE_t;

/** holds peer name for CID */
typedef struct _TD_COMM_MSG_DATA_1_CID_t
{
   /** type of data - must be TD_COMM_MSG_DATA_1_CID */
   TD_COMM_MSG_DATA_1_TYPE_t nType; /* 4 (4) */
   IFX_char_t aPeerName[TD_MAX_PEER_NAME_LEN]; /* 20 (24) */
}TD_COMM_MSG_DATA_1_CID_t;

/** holds peer name for CID */
typedef struct _TD_COMM_MSG_DATA_1_CONN_t
{
   /** type of data - must be TD_COMM_MSG_DATA_1_CONN_ID */
   TD_COMM_MSG_DATA_1_TYPE_t nType; /* 4 (4) */
   /** Sequential Connection ID */
   IFX_uint32_t nSeqConnID; /* 4 (8) */
   /** reserved field can be changed to hold additional data,
     * if changing structure think about backward compatibility and
     * changing name to represent functionality */
   IFX_char_t aReserved[(24 - 8)]; /* 24 - 8 (24) */
}TD_COMM_MSG_DATA_1_CONN_t;

/** data structures for message no 1, each new structure must contain
    TD_COMM_MSG_DATA_1_TYPE_t as first data member and new structure must be
    smaller than aReserved field (size of aReserved cannot be changed),
    check TD_COMM_MSG_t description for more info */
typedef union _TD_COMM_MSG_DATA_1_t
{
   /** array defined to set maximal size of union */
   IFX_char_t aReserved[TD_MAX_COMM_MSG_SIZE_DATA_1]; /* 32 */
   /** CID data */
   TD_COMM_MSG_DATA_1_CID_t oCID;
   /** Seq Conn ID */
   TD_COMM_MSG_DATA_1_CONN_t oSeqConnId;
}TD_COMM_MSG_DATA_1_t;


/** Message used to communicate between boards, structure can be changed only
    by adding elements right before nMarkEnd or by adding structures to unions,
    Union size cannot change due to those changes. When changin structure
    TD_COMM_MSG_CURRENT_VERSION must also be changed */
typedef struct _TD_COMM_MSG_t
{
   /** Mark start of message - must match, otherwise wrong message */
   IFX_uint32_t nMarkStart; /* 4 - field size (4 - overall size) */

   /** message version  */
   IFX_uint32_t nMsgVersion; /* 4 (8) */

   /** message version  */
   IFX_uint32_t nMsgLength; /* 4 (12) */

   /** Phone number of message sender */
   IFX_uint32_t nSenderPhoneNum; /* 4 (16) */

   /** Phone number of message receiver */
   IFX_uint32_t nReceiverPhoneNum; /* 4 (20) */

   /** Port number of message sender for VOICE streaming */
   IFX_int32_t nSenderPort; /* 4 (24) */

   /** message sender MAC */
   IFX_uint8_t MAC[ETH_ALEN];  /* 6 (30) */

   /** Action (OFF_HOOK, ...) */
   IFX_uint8_t nAction;  /* 1 (31) */

   /** 0x80 - PCM or 0x00 - VoIP call */
   IFX_uint8_t fPCM;  /* 1 (32) */

   /** Timeslot RX index, used in PCM connection,
       for VoIP call set to TD_T38_FAX_TEST_FDP_MARKER
       if call is started in FDP (fax data pump) mode */
   IFX_int32_t nTimeslot_RX;  /* 4 (36) */
   /** Timeslot TX index, used only in PCM connection */
   IFX_int32_t nTimeslot_TX;  /* 4 (40) */

   /** message sender board idx */
   IFX_int32_t nBoard_IDX;  /* 4 (44) */

   /** Configure phone with some features (number defines which one)
       also nAction must be set to IE_CONFIG_PEER,
       mainly used to set feature(e.g. codec) for both peers during call,
       feature is usually set with DTMF codes */
   IFX_int32_t nFeatureID;  /* 4 (48) */

   /** holds various data that can be send */
   TD_COMM_MSG_DATA_1_t oData1; /* 32 (80) */
   /** holds various data that can be send */
   TD_COMM_MSG_DATA_1_t oData2; /* 32 (112) */
   /* NEW DATA CAN BE ADDED ONLY RIGHT BEFORE nMarkEnd
      check structure description for more info */
   /** Mark end of message - must match, otherwise wrong message */
   IFX_uint32_t nMarkEnd;  /* 4 (116) */
} TD_COMM_MSG_t;

/** Type of mapping */
typedef enum _MAPPING_TYPE_t { NO_MAPPING = -1 } MAPPING_TYPE_t;

/** Structure holding status for trace redirection */
typedef struct _TRACE_REDIRECTION_CTRL_t
{
   /** Contains IP address for connection to syslog */
   TD_OS_sockAddr_t oSyslog_IP_Addr;
   /** Path to log file*/
   IFX_char_t sPathToLogFiles[MAX_PATH_LEN];
   /** Socket used for communication with remote syslog */
   TD_OS_socket_t nSyslogSocket;
   /* Holding file descriptor for log file */
   TD_OS_File_t* pFileLogFD;
   /* Start redirecting log */
   IFX_boolean_t fStartRedirectLogs;
} TRACE_REDIRECTION_CTRL_t;

/** Holds VoIP module settings */
typedef struct
{
   /** RTP configuration */
   struct {
      /** Print or not to print settings,
          if IFX_TRUE settings are printed during startup */
      IFX_boolean_t bPrintSettings;
      /** Event packets generation and auto-suppression of DTMF tones.*/
      IFX_TAPI_PKT_EV_OOB_t nOobEvents;
      /** Defines the playout of received RFC 2833 event packets. */
      IFX_TAPI_PKT_EV_OOBPLAY_t nOobPlayEvents;
   }oRtpConf;
} TD_VOIP_CFG_t;

/** Enum for PCM wide/narrow band */
typedef enum _TD_PCM_BAND_t
{
   /** Wideband */
   TD_WIDEBAND = 1,
   /** Narrowband */
   TD_NARROWBAND = 2
} TD_PCM_BAND_t;

/** PCM specific data */
typedef struct
{
   /** Sample rate, minimum value is 512 kHz,
      should be set during PCM configuration */
   IFX_TAPI_PCM_IF_DCLFREQ_t nPCM_Rate;
   /** MAX number of timeslots */
   IFX_int32_t nMaxTimeslotNum;
   /** Used PCM band */
   TD_PCM_BAND_t nBand;
   /** Narrowband codec type */
   IFX_TAPI_PCM_RES_t ePCM_WB_Resolution;
   /** Wideband codec type */
   IFX_TAPI_PCM_RES_t ePCM_NB_Resolution;
} TAPIDEMO_PCM_DATA_t;

#define SEPARATE(m_ConnId) \
TAPIDEMO_PRINTF(m_ConnId, ("############################################\n"));

/* ============================= */
/* Global Variables              */
/* ============================= */

/** Frequencies strings */
extern const char * TAPIDEMO_PCM_FREQ_STR[];

extern TD_VOIP_CFG_t oVoipCfg;
extern TD_ENUM_2_NAME_t TD_rgTapiOOB_EventDesc[];
extern TD_ENUM_2_NAME_t TD_rgTapiOOB_EventPlayDesc[];

/** PCM data */
extern TAPIDEMO_PCM_DATA_t g_oPCM_Data;
/** Used to print codec name */
extern TD_ENUM_2_NAME_t TD_rgEnumPCM_CodecName[];

/** max length of buffer used to sending traces to control PC */
#define TD_COM_BUF_SIZE     1024

/* global buffer for sending ITM messages - basic one */
extern IFX_char_t g_buf[TD_COM_BUF_SIZE];
/* global buffer for sending ITM messages - with prefix */
extern IFX_char_t g_buf_parsed[TD_COM_BUF_SIZE];

extern IFX_boolean_t g_bBBD_Dwld;

extern IFX_int32_t TD_EventLoggerFd;
extern IFX_boolean_t g_bUseEL;

/* ============================= */
/* Global Structures             */
/* ============================= */

 #ifdef EASY336
/** Structure for requesting allocation/deallocation of resources */
typedef struct _RESOURCES_t
{
   /* resources to get/release */
   IFX_int32_t nType;
   /* connID number to use */
   IFX_int32_t nConnID_Nr;
   /* voipID number to release */
   IFX_int32_t nVoipID_Nr;
} RESOURCES_t;
#endif

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* fd operations */
extern IFX_return_t Common_Open(IFX_char_t* pszName, BOARD_t *pBoard);
extern IFX_return_t Common_Set_FDs(BOARD_t* pBoard);
extern IFX_void_t Common_Close_FDs(BOARD_t* pBoard);
extern IFX_int32_t Common_GetFD_OfCh(BOARD_t* pBoard, IFX_int32_t nChNum);

/* FXO fd operations */
extern IFX_int32_t Common_GetNextFxoDevCtrlFd(CTRL_STATUS_t* pCtrl,
                                              TAPIDEMO_DEVICE_FXO_t** ppFxoDev,
                                              IFX_boolean_t fOmitSlic121Fxo);

/* board initialization */
extern IFX_return_t Common_RegisterBoard(BOARD_t* pBoard);
extern IFX_return_t Common_GetVersions(BOARD_t* pBoard);

IFX_return_t Common_SetPhonesAndUsedChannlesNumber(BOARD_t* pBoard,
                                                   IFX_uint32_t nSeqConnId);
IFX_return_t Common_GetCapabilities(BOARD_t* pBoard, IFX_uint32_t nSeqConnId);
IFX_return_t Common_DevVersionGet(BOARD_t* pBoard, IFX_uint32_t nSeqConnId);
extern IFX_char_t* Common_GetTraceIndention(IFX_int32_t nSize);
#ifdef USE_FILESYSTEM
IFX_return_t Common_CheckFileExists(IFX_char_t* pszFullFilename);
IFX_return_t Common_CreateFullFilename(IFX_char_t* pszFilename,
                                     IFX_char_t* pszPath,
                                     IFX_char_t* pszFullFilename);
#ifndef VXWORKS
IFX_return_t Common_SetFwFilesFromTapidemorc(BOARD_t* pBoard,
                                             TAPIDEMO_DEVICE_CPU_t* pCpuDevice);
#endif /* VXWORKS */
IFX_return_t Common_SetFwFilenames(BOARD_t* pBoard,
                                        TAPIDEMO_DEVICE_CPU_t* pCpuDevice);
IFX_boolean_t Common_FindPRAM_FW(TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                   IFX_char_t* pszPath,
                                   IFX_char_t* pszFullFilename);
IFX_boolean_t Common_FindBBD_CRAM(TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                  IFX_char_t* pszPath,
                                  IFX_char_t* pszFullFilename);
#ifndef VXWORKS
IFX_return_t Common_CheckDownloadPath(IFX_char_t* psPath,
                                      TAPIDEMO_DEVICE_CPU_t* pCpuDevice,
                                      IFX_boolean_t bPrintTrace,
                                      IFX_uint32_t nSeqConnId);
extern IFX_return_t Common_SetDownloadPath(IFX_char_t* sPathToDwnldFiles,
                                           BOARD_t* pBoard,
                                           TAPIDEMO_DEVICE_CPU_t* pCpuDevice);
#endif /* VXWORKS */
#endif  /* USE_FILESYSTEM */

IFX_return_t Common_ConfigureTAPI(CTRL_STATUS_t* pCtrl,
                                  BOARD_t* pBoard,
                                  PHONE_t* pPhone,
                                  IFX_int32_t nAction,
                                  IFX_int32_t nDataCh_FD,
                                  IFX_boolean_t fConfigureAll,
                                  IFX_boolean_t fConfigureConn,
                                  IFX_uint32_t nSeqConnId);

extern IFX_return_t Common_FillAddrByNumber(CTRL_STATUS_t* pCtrl,
                                            PHONE_t* pPhone,
                                            CONNECTION_t* pConn,
                                            IFX_uint32_t nIP_Addr,
                                            IFX_boolean_t fIsPCM_Num);
extern IFX_int32_t Common_DigitsToNum(IFX_char_t* prgnDialNum,
                                      IFX_int32_t nFirstDigit,
                                      IFX_int32_t nDialNrCnt,
                                      IFX_uint32_t nSeqConnId);

extern CALL_TYPE_t Common_CheckDialedNumForFull_IP(IFX_char_t* prgnDialNum,
                                                   IFX_int32_t nDialNrCnt,
                                                   IFX_uint32_t* nIpAddr,
                                                   IFX_uint32_t nSeqConnId);
extern CALL_TYPE_t Common_CheckDialedNum(PHONE_t* pPhone,
                                         IFX_int32_t* nDialedNum,
                                         IFX_uint32_t* nIpAddr,
                                         IFX_char_t* pCalledIP,
                                         CTRL_STATUS_t* pCtrl);

extern IFX_void_t Common_PrintCallProgress(STATE_MACHINE_STATES_t current_state,
                                           INTERNAL_EVENTS_t new_event,
                                           CALL_TYPE_t nOldCallType,
                                           PHONE_t* pPhone, CONNECTION_t* pConn,
                                           IFX_int32_t nUseCodersForLocal);
extern IFX_void_t Common_PrintEncoderType(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                          CONNECTION_t* pConn);
IFX_void_t Common_ToneIndicationCfg(CTRL_STATUS_t* pCtrl);
extern IFX_return_t Common_TonePlay(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                                    IFX_int32_t nTone);
extern IFX_return_t Common_ToneStop(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
extern IFX_return_t Common_JBStatistic(IFX_int32_t nDataCh,
                                       IFX_int32_t nDev,
                                       IFX_boolean_t fReset,
                                       IFX_boolean_t fShow,
                                       BOARD_t* pBoard);

extern IFX_return_t Common_RTCPStatistic(IFX_int32_t fd,
                                         IFX_boolean_t fReset,
                                         IFX_boolean_t fShow);

extern IFX_void_t Common_ErrorHandle(IFX_int32_t nFD, BOARD_t* pBoard,
   IFX_uint16_t nDev);
#ifdef TAPI_VERSION4
#ifdef EASY336
extern IFX_return_t Common_PCM_ConfigSet(BOARD_t* pBoard,
                                         RM_SVIP_PCM_CFG_t* pcm_cfg);
extern IFX_return_t Common_ResourcesGet(PHONE_t* pPhone,
                                        const RESOURCES_t* pResources);
extern IFX_return_t Common_ResourcesRelease(PHONE_t* pPhone,
                                            const RESOURCES_t* pResources);
extern VOIP_DATA_CH_t* Common_GetCodec(CTRL_STATUS_t* pCtrl,
                                       IFX_int32_t voipID,
                                       IFX_uint32_t nSeqConnId);
extern VOIP_DATA_CH_t* Common_GetFirstCodecOfPhone(PHONE_t* pPhone);
#endif /* EASY336 */
extern IFX_return_t Common_DTMF_Enable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
extern IFX_return_t Common_DTMF_Disable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
#ifdef EASY336
extern IFX_int32_t Common_ConnID_PhonesGet(CTRL_STATUS_t* pCtrl,
                                           IFX_int32_t connID,
                                           PHONE_t** pPhones[]);
#endif /* EASY336 */

extern IFX_return_t Common_PulseEnable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
extern IFX_return_t Common_PulseDisable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);

extern IFX_return_t Common_CID_Enable(PHONE_t* pPhone, IFX_boolean_t bCID_AS);
extern IFX_return_t Common_CID_Disable(PHONE_t* pPhone);
extern IFX_return_t Common_SetChipCount(BOARD_t* pBoard);
#ifdef LINUX
extern IFX_return_t Common_WriteByte(IFX_uint32_t nAddr, IFX_uint8_t nValue);
#endif
#else /* TAPI_VERSION4 */
IFX_return_t Common_DTMF_SIG_Enable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
IFX_return_t Common_DTMF_SIG_Disable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
#endif /* TAPI_VERSION4 */
IFX_return_t Common_CommMsgSet(TD_COMM_MSG_t* pMsg, IFX_char_t* pBuff,
                               IFX_int32_t nBuffLen);
extern IFX_return_t Common_LineFeedSet(PHONE_t* pPhone, TD_LINE_MODE_t mode);

extern IFX_return_t Common_RingingStart(PHONE_t* pPhone);
extern IFX_return_t Common_RingingStop(PHONE_t* pPhone);

#if defined(EASY336) || !defined(TAPI_VERSION4)
extern IFX_return_t Common_LEC_Enable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
extern IFX_return_t Common_LEC_Disable(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone);
#endif /* defined(EASY336) || !defined(TAPI_VERSION4) */

IFX_return_t Common_LEC_On_PCM_Ch(FXO_t* pFXO, IFX_int32_t nPCM_Ch,
                                  IFX_enDis_t nEnable, BOARD_t* pBoard);
extern IFX_return_t Common_SetVad(PHONE_t* pPhone, BOARD_t* pBoard,
                                   IFX_int32_t nVadCfg);
extern IFX_void_t Common_StripPath (IFX_char_t** sPath);

extern TAPIDEMO_DEVICE_t * Common_GetDevice_CPU(TAPIDEMO_DEVICE_t* pDevice);
extern IFX_return_t Common_AddDevice(BOARD_t* pBoard,
                                      TAPIDEMO_DEVICE_t* pDevice);
extern IFX_return_t Common_RemoveDeviceList(BOARD_t* pBoard);
extern IFX_char_t * Common_Enum2Name(IFX_int32_t nEnum,
                                    TD_ENUM_2_NAME_t *pEnumName);
extern IFX_int32_t Common_Name2Enum(IFX_char_t* pString,
                                    TD_ENUM_2_NAME_t *pEnumName,
                                    IFX_boolean_t bPrintError);
IFX_return_t Common_MapLocalPhones(PHONE_t* pSrcPhone, PHONE_t* pDstPhone,
                                   TD_MAP_ADD_REMOVE_t nMapFlag);
extern IFX_boolean_t Common_IsNumber(IFX_char_t* pValue);
IFX_return_t Common_SetLocalCallType(PHONE_t *pPhoneSrc, PHONE_t *pPhoneDst,
                                     CONNECTION_t *pConn);
extern IFX_return_t Common_StartThread(TD_OS_ThreadCtrl_t* pThreadCtrl,
                                    IFX_int32_t (*pFun)(TD_OS_ThreadParams_t*),
                                    IFX_char_t* pThreadName,
                                    TD_PIPE_t* pSynchPipe,
                                    TD_OS_lock_t* pLockThreadStop);
extern IFX_void_t Common_StopThread(TD_OS_ThreadCtrl_t* pThreadCtrl,
                               TD_PIPE_t* pSynchPipe,
                               TD_OS_lock_t* pLockThreadStop);
extern IFX_return_t Common_PreparePipe(TD_PIPE_t* pPipe);
#ifdef LINUX
IFX_void_t Common_CheckTraceLen(IFX_char_t* pBuffer);
#endif /* LINUX */

#ifdef DANUBE
/* GPIO */
extern IFX_int32_t Common_GPIO_OpenPort(IFX_char_t* pGpioPortName);
extern IFX_return_t Common_GPIO_ClosePort(IFX_char_t* pGpioPortName,
                                          IFX_int32_t nGPIO_PortFd);

extern IFX_return_t Common_GPIO_ReservePin(IFX_int32_t nFd, IFX_int32_t nPort,
                                           IFX_int32_t nPin, IFX_int32_t nModule);
extern IFX_return_t Common_GPIO_FreePin(IFX_int32_t nFd, IFX_int32_t nPort,
                                        IFX_int32_t nPin, IFX_int32_t nModule);
#endif /* DANUBE */

#ifdef EVENT_COUNTER
extern IFX_void_t EventSeqNoCheck(BOARD_t *pBoard, IFX_TAPI_EVENT_t *pEvent);
extern IFX_char_t *Common_Enum2Name(IFX_int32_t nEnum, TD_ENUM_2_NAME_t *pEnumName);
#endif /* EVENT_COUNTER */
extern TD_ENUM_2_NAME_t TD_rgTapiEventName[];
extern TD_ENUM_2_NAME_t TD_rgEnumToneIdx[];
extern IFX_void_t* Common_MemCalloc(IFX_size_t num, IFX_size_t size);

#ifdef LINUX
IFX_int32_t Common_CreateFifo(IFX_char_t * pcName);
IFX_int32_t Common_OpenFifo(IFX_char_t * pcName, IFX_int32_t iFlags);
#endif /* LINUX */

#ifdef TD_IPV6_SUPPORT
/* Phonebook */
IFX_return_t Common_VerifyAddrIP(IFX_char_t *pWord, IFX_int32_t *pAddrFamily);
IFX_return_t Common_PhonebookGetFromFile(CTRL_STATUS_t* pCtrl);
IFX_return_t Common_PhonebookSendEntry(CTRL_STATUS_t* pCtrl, IFX_int32_t nAction,
                                       TD_OS_sockAddr_t* pDstAddrStorage);
IFX_return_t Common_PhonebookHandleEvents(CTRL_STATUS_t* pCtrl,
                                          TD_OS_sockAddr_t* pSockAddrStorage,
                                          IFX_int32_t nEvent,
                                          IFX_char_t* pBoardname);
IFX_return_t Common_PhonebookAddEntry(CTRL_STATUS_t* pCtrl,
                                      IFX_char_t* pAddrIP,
                                      const IFX_char_t* pBoardname,
                                      IFX_boolean_t *bEntryAdded);
IFX_void_t Common_PhonebookShow(TD_PHONEBOOK_t* pPhoneBook);
IFX_boolean_t Common_PhonebookIsDuplicate(TD_PHONEBOOK_t* pPhoneBook,
                                          IFX_int32_t nPhonebookSize,
                                          IFX_char_t* pAddrIP);
IFX_return_t Common_PhonebookGetIpAddr(CTRL_STATUS_t* pCtrl,
                                       IFX_char_t* pDialedNum,
                                       IFX_char_t* pCalledIP);
IFX_return_t Common_BroadcastSocketOpen(IFX_char_t* pIfName,
                                        IFX_char_t* pBroadcastPort,
                                        TD_BROAD_MULTI_CAST_t* pBroadcast);
IFX_return_t Common_MulticastSocketOpen(IFX_char_t* pIfName,
                                        IFX_char_t* pMulticastIP,
                                        IFX_char_t* pMulticastPort,
                                        TD_BROAD_MULTI_CAST_t* pMulticast);
IFX_return_t Common_PhonebookInit(CTRL_STATUS_t* pCtrl);

IFX_return_t Common_SocketAllocateIPv6(BOARD_t *pBoard);
#endif /* TD_IPV6_SUPPORT */
#ifdef TD_PPD
IFX_return_t Common_SetPpdCfg(BOARD_t *pBoard, IFX_uint32_t nCh,
                              IFX_uint32_t nSeqConnId);
#endif /* TD_PPD */

IFX_return_t ABSTRACT_ParseTrace(IFX_uint32_t nConnId);
IFX_void_t Common_SendMsgToEL(IFX_char_t *pMsg);

#endif /* _COMMON_H */

