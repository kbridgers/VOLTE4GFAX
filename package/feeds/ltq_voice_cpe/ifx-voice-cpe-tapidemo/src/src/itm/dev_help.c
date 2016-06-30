/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file dev_help.c
   \date 2007-08-20
   \brief Implementation of developer help functions.

*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "common.h"
#include "tapidemo.h"
#include "drv_tapi_strerrno.h"

/* ============================= */
/* Local Structures              */
/* ============================= */

/** holds number of FD, its name and pointer to next structure */
typedef struct _TD_NAME_OF_FD_t
{
   /** FD number */
   IFX_int32_t nFd;
   /** name of FD */
   IFX_char_t acName[DEV_NAME_LEN];
   /** pointer this channels board */
   BOARD_t *pBoard;
   /** pointer to next structure */
   struct _TD_NAME_OF_FD_t *pNext;
} TD_NAME_OF_FD_t;

#ifdef TAPI_VERSION4
/** used to read dev and ch from TAPI_VERSION4 structures */
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** Channel 'module' index. */
   IFX_uint16_t ch;
} TD_TAPI_V4_CH_DEV_GET_t;
#endif /* TAPI_VERSION4 */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** pointer only available in this file to hold list of FDs with names */
static TD_NAME_OF_FD_t* g_FdNameList = IFX_NULL;

/* IFX_TRUE if info about all called ioctls should be printed */
IFX_boolean_t g_bPrintIoctl = IFX_FALSE;

/* Unknown device name */
#define TD_UNKNOWN_DEVICE_NAME "Unknown"

/* ============================= */
/* Local function declaration    */
/* ============================= */

IFX_void_t TD_DEV_ErrorHandle(IFX_int32_t nFD, IFX_int32_t nIoctl,
                              IFX_uint32_t nArg, IFX_char_t* psIoctlName,
                              IFX_uint16_t nDev, IFX_uint32_t nSeqConnId,
                              IFX_uint32_t nErrHandling,
                              IFX_char_t* psFile, IFX_int32_t nLine);

/* ============================= */
/* Local macro definition        */
/* ============================= */

#ifdef LINUX
/** send msg with ioctl data
 *  \param m_pFdName    - string with FD name
 *  \param m_pszIoctl   - string with ioctl name
 *  \param m_nRet       - result of ioctl
 *  \param m_nSeqConnId - seq conn id */
#define COM_SEND_IOCTL_MSG(m_pFdName, m_pszIoctl, m_nRet, m_nSeqConnId) \
   do \
   { \
      /* if ITM on then send msg to control PC */ \
      if (IFX_TRUE == g_pITM_Ctrl->fITM_Enabled && \
          NO_SOCKET != g_pITM_Ctrl->nComOutSocket) \
      { \
         IFX_char_t buf[MAX_ITM_MESSAGE_TO_SEND]; \
         sprintf(buf, \
                 "IOCTL_CALL:{" \
                     " TARGET {%s} " \
                     " NAME {%s} " \
                     " RESULT {%d} " \
                 "}%s", \
                 m_pFdName, m_pszIoctl, m_nRet, COM_MESSAGE_ENDING_SEQUENCE); \
         COM_SendMessage(g_pITM_Ctrl->nComOutSocket, buf, m_nSeqConnId); \
      } \
   } while ( 0 )
#else /* LINUX */
/** send msg with ioctl data
 *  \param m_pFdName    - string with FD name
 *  \param m_pszIoctl   - string with ioctl name
 *  \param m_nRet       - result of ioctl
 *  \param m_nSeqConnId - seq conn id */
#define COM_SEND_IOCTL_MSG(m_pFdName, m_pszIoctl, m_nRet, m_nSeqConnId)
#endif /* LINUX */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Function to print Data Channels data.

   \param pBoard  - pointer to board structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t TD_DEV_PrintMappedDataCh(BOARD_t* pBoard)
{
   IFX_int32_t nCounter;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   for (nCounter = 0; nCounter < pBoard->nMaxCoderCh; nCounter++)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
            ("MAPPING : BOARD : fMapped(%d), nDataCh(%d), "
             "nSocket(%d), nDataCh_Fd(%d) \n",
             pBoard->pDataChStat[nCounter].fMapped,
             nCounter,
             pBoard->rgnSockets[nCounter],
             pBoard->nCh_FD[nCounter]));
   }

   return IFX_SUCCESS;
} /* */

/**
   Function to print phones connections data.

   \param pBoard  - pointer to board structure

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t TD_DEV_PrintConnections(BOARD_t* pBoard)
{
   IFX_int32_t i, j;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   for (i = 0; i < pBoard->nMaxAnalogCh; i++)
   {
      if (0 < pBoard->rgoPhones[i].nConnCnt)
      {
         for (j = 0; j < pBoard->rgoPhones[i].nConnCnt; j++)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
               ("PHONE(%d, %d) : CONNECTION : fActive(%d), nType(%d),"
                " nUsedCh(%d), nUsedSocket(%d), nUsedCh_Fd(%d).\n",
                i, j,
                pBoard->rgoPhones[i].rgoConn[j].fActive,
                pBoard->rgoPhones[i].rgoConn[j].nType,
                pBoard->rgoPhones[i].rgoConn[j].nUsedCh,
                pBoard->rgoPhones[i].rgoConn[j].nUsedSocket,
                pBoard->rgoPhones[i].rgoConn[j].nUsedCh_FD));
         }
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_DBG,
            ("PHONE(%d) : Number of connections is 0.\n", i));
      }
   }

   return IFX_SUCCESS;
} /* */

/*
   Purpose of this functions is to be able to easily track usage of ioctls().
   Thanks to TD_IOCTL macro in all places where ioctl() was used TAPIDEMO will
   print trace with ioctl name, file name and line number, TD_IOCTL uses the
   same arguments as ioctl - this made the changes quicker, also for improving
   performance this macro can directly call ioctl() instead of calling debug
   function, furthermore it will be helpfull for tests (ITM) - info about
   ioctls() called during test will be send to test scripts so all tapi ioctls
   couold be listed after test end.

   ioctl trace is listing all ioctls used, it can help
   to confirm if right set of ioctls was used.
   */

/**
   Gets data structure of FD if available.

   \param nFD        - number of FD

   \return pointer to structure or IFX_NULL if FD is not registered
*/
TD_NAME_OF_FD_t* TD_DEV_IoctlFdDataGet(IFX_int32_t nFd)
{
   /* get pointer to structure list */
   TD_NAME_OF_FD_t* pNext = g_FdNameList;

   /* check untill the last one (pNext->pNext is set to IFX_NULL) */
   while (IFX_NULL != pNext)
   {
      /* check FD number */
      if (pNext->nFd == nFd)
      {
         /* return pointer to name */
         return pNext;
      }
      /* check next */
      pNext = pNext->pNext;
   } /* while (IFX_NULL != pNext) */
   /* no name was registered for this FD */
   return IFX_NULL;
}

/**
   Set name for given FD. Such name can be retrived with
   TD_DEV_IoctlGetNameOfFd().

   \param nFD        - number of FD
   \param pszFdName  - pointer to zero terminated string with name for FD
   \param pBoard     - pointer to board structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DEV_IoctlSetNameOfFd(IFX_int32_t nFd, IFX_char_t* pszFdName,
                                     BOARD_t *pBoard)
{
   /* get pointer to structure list */
   TD_NAME_OF_FD_t* pNext = g_FdNameList;
   TD_NAME_OF_FD_t* pTmpNext;
   IFX_int32_t nStringLen = 0;

   /* check input parameters */
   TD_PTR_CHECK(pszFdName, IFX_ERROR);

   /* open function returned error - this error will not be handled here */
   if (0 > nFd)
   {
      return IFX_SUCCESS;
   }

   /* get string length */
   nStringLen = strlen(pszFdName);

   /* check string length */
   if (0 >= nStringLen ||
       DEV_NAME_LEN <= nStringLen)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DBG,
            ("Err, Invalid string length %d for fd %d. (File: %s, line: %d)\n",
             nStringLen, nFd, __FILE__, __LINE__));
      return IFX_ERROR;
   }

#if 0
/* debug trace - enable in case of problems after code modifications */
   /* print data of FD that will be added */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DBG,
         ("FD %d: %s\n", nFd, pszFdName));
#endif

   /* get last pointer or structure with the same FD number */
   if (IFX_NULL != pNext)
   {
      if (pNext->nFd == nFd)
      {
         /* if the same FD number then set new name */
         strncpy(pNext->acName, pszFdName, DEV_NAME_LEN);
         pNext->pBoard = pBoard;
         return IFX_SUCCESS;
      }
      while (IFX_NULL != pNext->pNext)
      {
         pNext = pNext->pNext;
         if (pNext->nFd == nFd)
         {
            /* if the same FD number then set new name */
            strncpy(pNext->acName, pszFdName, DEV_NAME_LEN);
            pNext->pBoard = pBoard;
            return IFX_SUCCESS;
         }
      } /* while (IFX_NULL != pNext->pNext) */
   } /* if (IFX_NULL != pNext) */
   /* allocate memory */
   pTmpNext = TD_OS_MemCalloc(1, sizeof(TD_NAME_OF_FD_t));

   /* check if memory is allocated */
   if (IFX_NULL == pTmpNext)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, Failed to allocate memory for %s. (File: %s, line: %d)\n",
             pszFdName, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if first pointer is set */
   if (IFX_NULL == g_FdNameList)
   {
      g_FdNameList = pTmpNext;
   }
   else
   {
      pNext->pNext = pTmpNext;
   }
   /* set structure */
   pTmpNext->nFd = nFd;
   pTmpNext->pBoard = pBoard;
   pTmpNext->pNext = IFX_NULL;
   strncpy(pTmpNext->acName, pszFdName, DEV_NAME_LEN);

   return IFX_SUCCESS;
}

/**
   Cleanup structures for ioctl() trace.

   \param none

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_DEV_IoctlCleanUp(IFX_void_t)
{
   /* get pointer to structure list */
   TD_NAME_OF_FD_t* pNext = g_FdNameList;
   TD_NAME_OF_FD_t* pTemp = IFX_NULL;

   /* for all allocated structures */
   while (IFX_NULL != pNext)
   {
      /* get next pointer */
      pTemp = pNext->pNext;
      /* free memory */
      TD_OS_MemFree(pNext);
      /* set to next */
      pNext = pTemp;
   }
   return IFX_SUCCESS;
}

/**
   Used insted of ioctl() to print info about called ioctl() that is called.
   In case of ioctl error handling of error is called.

   \param nFD        - number of FD for ioctl to be called on
   \param nIoctl     - ioctl number
   \param nArg       - ioctl argument
   \param pszIoctl   - ioctl name (string)
   \param nDev       - device number
   \param nSeqConnId - conn id
   \param nErrHandling  - if set TD_ERR_HANDLING_NO,
                          then no error info is printed
   \param pszFile    - file name where function was used
   \param nLine      - file line where function was used

   \return whatever ioctl returns
*/
IFX_int32_t TD_DEV_Ioctl(IFX_int32_t nFd, IFX_int32_t nIoctl, IFX_uint32_t nArg,
                         IFX_char_t* pszIoctl, IFX_uint16_t nDev,
                         IFX_uint32_t nSeqConnId, IFX_uint32_t nErrHandling,
                         IFX_char_t* pszFile, IFX_int32_t nLine)
{
   IFX_char_t* pFdName = IFX_NULL;
   IFX_int32_t nRet;
   IFX_char_t aName[DEV_NAME_LEN];
   TD_NAME_OF_FD_t* pFd_Data;

   /* call ioctl */
   nRet = TD_OS_DeviceControl(nFd, nIoctl, nArg);

   /* return if printout is not needeed */
   if (IFX_TRUE == g_bPrintIoctl)
   {
      /* get FD name */
      pFd_Data = TD_DEV_IoctlFdDataGet(nFd);
      /* in case of unknown name get FD number */
      if (IFX_NULL == pFd_Data)
      {
         sprintf(aName, "%d", nFd);
         pFdName = aName;
      }
      else
      {
         pFdName = pFd_Data->acName;
      }

      /* print ioctl name */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Calling ioctl %s on %s (ret %d, arg %u)\n"
             "   in file: %s on line: %d\n",
                       /* check if name is available, print name or number */
             pszIoctl, pFdName, nRet, nArg, pszFile, nLine));

      /* if ITM on then send msg to control PC */
      COM_SEND_IOCTL_MSG(pFdName, pszIoctl, nRet, nSeqConnId);
   }
   if (IFX_ERROR == nRet)
   {
      TD_DEV_ErrorHandle(nFd, nIoctl, nArg, pszIoctl, nDev, nSeqConnId,
                         nErrHandling, pszFile, nLine);
   }
   /* call ioctl() */
   return nRet;
}

/**
   Handle ioctl error. If unable to call IFX_TAPI_LASTERR,
   then print ioctl name and arguments.

   \param nFD           - number of FD for ioctl to be called on
   \param nIoctl        - ioctl number
   \param nArg          - ioctl argument
   \param psIoctlName   - ioctl name (string)
   \param nDev          - device number
   \param nSeqConnId    - conn id
   \param nErrHandling  - if set TD_ERR_HANDLING_NO, then no error info is printed
   \param psFile        - file name where function was used
   \param nLine         - file line where function was used

   \return whatever ioctl returns
*/
IFX_void_t TD_DEV_ErrorHandle(IFX_int32_t nFD, IFX_int32_t nIoctl,
                              IFX_uint32_t nArg, IFX_char_t* psIoctlName,
                              IFX_uint16_t nDev, IFX_uint32_t nSeqConnId,
                              IFX_uint32_t nErrHandling,
                              IFX_char_t* psFile, IFX_int32_t nLine)
{
   IFX_TAPI_Error_t oError;
   IFX_int32_t i = 0, j = 0;
   IFX_return_t ret = IFX_SUCCESS;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t *pCpuDevice = IFX_NULL;
#ifdef TAPI_VERSION4
   TD_TAPI_V4_CH_DEV_GET_t *pTapi4_Struct = IFX_NULL;
#endif /* TAPI_VERSION4 */
   TD_NAME_OF_FD_t* pFdObject;

#ifdef TAPI_VERSION4
   if (255 < nArg)
   {
      pTapi4_Struct = (TD_TAPI_V4_CH_DEV_GET_t*) nArg;
   }
#endif /* TAPI_VERSION4 */

   pFdObject = TD_DEV_IoctlFdDataGet(nFD);

   if (TD_ERR_HANDLING_NO != nErrHandling)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, ioctl %s failed\n", psIoctlName));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("     ioctl argument %u, fd %d (%s), dev %d, board %s\n",
              nArg, nFD,
             (IFX_NULL != pFdObject) ? pFdObject->acName : "unknown node",
              nDev,
             (IFX_NULL != pFdObject && IFX_NULL != pFdObject->pBoard) ?
             pFdObject->pBoard->pszBoardName : "not set"));

#ifdef TAPI_VERSION4
      if (IFX_NULL != pTapi4_Struct)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("     from ioctl argument: ch %d, dev %d\n",
                pTapi4_Struct->ch, pTapi4_Struct->dev));
      }
#endif /* TAPI_VERSION4 */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("     file: %s line: %d\n", psFile, nLine));
   }

   if (IFX_NULL == pFdObject || IFX_NULL == pFdObject->pBoard)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, board for fd %d not set, unable to handle error further.\n",
             nFD));
      return;
   }
   if (IFX_TAPI_LASTERR == nIoctl)
   {
      /* ioctl IFX_TAPI_LASTERR failed, do not call IFX_TAPI_LASTERR again
         to prevent infinite loop */
      return;
   }

   memset(&oError, 0, sizeof(IFX_TAPI_Error_t));

#ifdef TAPI_VERSION4
   /* according to TAPI_V4_UM_PR_Rev2.4.pdf this is not needed,
      but this is how it was implemented before in tapidemo,
      leaving it just in case */
   oError.nDev = nDev;
#endif /* TAPI_VERSION4 */

   ret = TD_DEV_Ioctl(pFdObject->pBoard->nDevCtrl_FD, IFX_TAPI_LASTERR,
            (IFX_uint32_t) &oError,
            psIoctlName, nDev, nSeqConnId, nErrHandling, __FILE__, __LINE__);

   if ((IFX_SUCCESS != ret) || (TD_ERR_HANDLING_NO == nErrHandling))
   {
      return;
   }

   if (-1 == oError.nCode || IFX_SUCCESS == oError.nCode)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, IFX_TAPI_LASTERR error code not set - 0x%X\n"
             "(File: %s, line: %d)\n", oError.nCode, __FILE__, __LINE__));
      return;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
         ("     ch %d, dev %d\n",
          oError.nCh, oError.nDev));


   if (oError.nCnt >= IFX_TAPI_MAX_ERROR_ENTRIES)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, received errors count %d too big, showing %d errors.\n"
             "(File: %s, line: %d)\n",
             oError.nCnt, IFX_TAPI_MAX_ERROR_ENTRIES, __FILE__, __LINE__));
   }

   if ((pDevice =
           Common_GetDevice_CPU(pFdObject->pBoard->pDevice)) == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
           ("Err, CPU device not found. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
   }
   else
   {
      pCpuDevice = &pDevice->uDevice.stCpu;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId, ("  From driver:\n"));

   for (j = 0; j < TAPI_ERRNO_CNT; ++j)
   {
      if (TAPI_drvErrnos[j] == (oError.nCode & 0xFFFF))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("        - code 0x%08X, string %s\n",
                oError.nCode, TAPI_drvErrStrings[j]));
         break;
      }
   }

   if (j == TAPI_ERRNO_CNT)
   {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("        - code 0x%08X\n",
                oError.nCode));
   }

   for (i = 0; i < TD_GET_MIN(oError.nCnt, IFX_TAPI_MAX_ERROR_ENTRIES); ++i)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("        - Error Entry No %d\n", i));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("          - file %s, line %d\n",
             oError.stack[i].sFile, oError.stack[i].nLine));

      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("          - HL Code 0x%X\n", oError.stack[i].nHlCode));
      for (j = 0; j < TAPI_ERRNO_CNT; ++j)
      {
         if (TAPI_drvErrnos[j] == (oError.stack[i].nHlCode & 0xFFFF))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("          - HL error description: %s\n", TAPI_drvErrStrings[j]));
         }
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("          - LL Code 0x%X\n", oError.stack[i].nLlCode));
      if (IFX_NULL != pCpuDevice)
      {
         /* Device displays LL error code and LL description. */
         pCpuDevice->DecodeErrno(oError.stack[i].nLlCode, nSeqConnId);
      }
#ifdef TAPI_VERSION4
      for (j = 0; j < IFX_TAPI_MAX_ERRMSG; ++j)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("          - Msg[%d]: 0x%08X\n", j, oError.stack[i].msg[j]));
      }
#endif /* TAPI_VERSION4 */
   } /* for */

   return;
} /*  */

/**
   Get name for given FD. Such name can be set with
   TD_DEV_IoctlSetNameOfFd().

   \param nFD        - number of FD

   \return Name of dev if set, 'Unknown' string, if not.
*/
IFX_char_t* TD_DEV_IoctlGetNameOfFd(IFX_int32_t nFd, IFX_uint32_t nSeqConnId)
{
   TD_NAME_OF_FD_t* pResult;

   /* Retrieve data */
   pResult = TD_DEV_IoctlFdDataGet(nFd);

   if (IFX_NULL == pResult)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, nSeqConnId,
            ("Warning, Can't get dev name from fd.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return TD_UNKNOWN_DEVICE_NAME;
   }

   /* Return name */
   return pResult->acName;
}
