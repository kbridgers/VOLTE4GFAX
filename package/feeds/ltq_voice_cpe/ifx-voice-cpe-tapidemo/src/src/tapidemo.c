/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file tapidemo.c
   \date 2003-03-24
   \brief Demonstrates a simple PBX with voice over IP on Linux or VxWorks OS.

   \note Changes:
      Date: 28.11.2005 it is working on new board with new drivers, kernel.
                       CID structure changed.
                       Removed EXCHFD_SUPPORT, its obsolete.
                       Extracted QOS, TAPI SIGNALLING into different files.
      Date: 15.12.2005 Added conference support.
      Date: 21.04.2006 Major changes, added PCM, Features support. Also added
                       Event handling (bitfields or messages).
      Date: 07.12.2006 Major changes, support for FXO, DANUBE with CPE
                       extension board, multiple boards, parsing argumens in
                       VxWorks.
      Date: 31.05.2007 Added support for board DuSLIC-XT.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef VXWORKS
#include <selectLib.h>
#include <inetLib.h>
#include <applUtilLib.h>
#endif

#ifdef LINUX
#include "tapidemo_config.h"
#endif

#include "tapidemo_version.h"
#include "td_ifxos_map.h"
#include "tapidemo.h"
#include "abstract.h"
#include "analog.h"
#include "cid.h"
#include "conference.h"
#include "common.h"
#include "event_handling.h"
#include "feature.h"
#include "parse_cmd_arg.h"
#include "pcm.h"
#include "qos.h"
#include "state_trans.h"
#include "lib_tapi_signal.h"
#include "voip.h"

#ifdef FXO
   #include "common_fxo.h"
#endif /* FXO */

#ifdef EASY50510
   #include "board_easy50510.h"
#endif /* EASY50510 */

#ifdef DXT
   #include "board_easy3111.h"
#endif /* DXT */

#ifdef EASY336
#include "lib_svip.h"
#include "lib_svip_rm.h"
#include "board_easy336.h"
#ifdef WITH_VXT
#include "board_xt16.h"
#endif /* WITH_VXT */
#endif /* EASY336 */

#ifdef XT16
/*#include "lib_svip.h"*/
/*#include "board_easy336.h"*/
#include "board_xt16.h"
#endif

#if (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
#include "./t38_fax_test/td_t38_fax_test.h"
#endif /* (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

#ifdef TD_DECT
#include "td_dect.h"
#endif /* TD_DECT */

#ifdef TD_TIMER
#include "td_timer.h"
#endif /* TD_TIMER */

#ifdef EASY3111
#include "board_easy3111.h"
#endif /* EASY3111 */

#ifdef TD_IPV6_SUPPORT
#include <ifaddrs.h>
#endif /* TD_IPV6_SUPPORT */

#ifdef LINUX
   #include "./itm/control_pc.h"
#endif /* LINUX */

/* ============================= */
/* Debug interface               */
/* ============================= */

TD_OS_PRN_USR_MODULE_CREATE(TAPIDEMO, TD_OS_PRN_LEVEL_HIGH);

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Holding information about each connection */
static CTRL_STATUS_t oCtrlStatus;
/** Holding program arguments */
static PROGRAM_ARG_t oProgramArg;
int incoming_call_flag=0;

/** List of combinations */
const BOARD_COMBINATION_t BOARD_COMBINATIONS[MAX_COMBINATIONS] =
{
   /* comb idx, board cnt, list of boards
                           {board id, type, sub type, init (normal/gpio) } */
   { ONE_CPE, 1, { {1, TYPE_VINETIC},
                   {0}, {0}, {0} } },
   /* reserved value */
   { BOARD_COMB_RESERVED, 1, { {1, TYPE_NONE},
                               {0}, {0}, {0} } },
   { ONE_VMMC, 1, { {1, TYPE_DANUBE},
                    {0}, {0}, {0} } },
   { ONE_VMMC_ONE_CPE, 2, { {1, TYPE_DANUBE},
                            {1, TYPE_VINETIC},
                            {0}, {0} } },
   { ONE_DXT, 1, { {1, TYPE_DUSLIC_XT},
                   {0}, {0}, {0} } },
   { TWO_DXTS, 2, { {1, TYPE_DUSLIC_XT},
                    {2, TYPE_DUSLIC_XT},
                    {0}, {0} } },
   { ONE_VMMC_VINAX, 1, { {1, TYPE_VINAX},
                    {0}, {0}, {0} } },
   { ONE_VMMC_EASY508XX, 1, { {1, TYPE_AR9},
                    {0}, {0}, {0} } },
   { X_SVIP, 1, { {1, TYPE_SVIP},
                  {0}, {0}, {0} } },
   { X_SVIP_X_XT16, 2, { {1, TYPE_SVIP},
                         {1, TYPE_XT16},
                         {0}, {0} } },
   { X_XT16, 1, { {1, TYPE_XT16},
                    {0}, {0}, {0} } },
   { ONE_EASY80910, 1, { {1, TYPE_VR9},
                       {0}, {0}, {0} } },
};

/** Contains board combination names. */
const IFX_char_t* BOARD_COMB_NAMES[MAX_COMBINATIONS] =
{
   "single board",
   "reserved",
   "EASY50712",
   "EASY50712 with EASY50510",
   "EASY3201",
   "EASY3201 with EASY3111",
   "EASY80800",
   "EASY508xx",
   "EASY336",
   "EASY336 and xT-16s",
   "ONE xT-16",
   "EASY80910"
};

#ifdef EASY336
/** PCM configuration */
RM_SVIP_PCM_CFG_t pcm_cfg;
#endif

/** Max log's length */
enum { MAX_LOG_LEN = 2048 };

#ifdef LINUX
/** Max message's length for external syslog: 2048+5
    Look into function TAPIDEMO_SendLogRemote() for more details */
enum { MAX_EXT_SYSLOG_MSG_LEN = 2053 };

/** max size of buffer to store network interfaces names */
#define TD_MAX_SIOCGIFCONF_BUF         2048

#endif

/** Holding trace redirection settings */
TRACE_REDIRECTION_CTRL_t oTraceRedirection;

IFX_int32_t TD_EventLoggerFd = -1;


/* ============================= */
/* Local function declaration    */
/* ============================= */

IFX_return_t TAPIDEMO_BlockSignals(IFX_void_t);
static IFX_return_t TAPIDEMO_Setup(BOARD_t* pBoard,
                                   CTRL_STATUS_t* pCtrl,
                                   IFX_int32_t* nPhoneNum);
TD_OS_socket_t TAPIDEMO_InitAdminSocket(IFX_void_t);
IFX_return_t TAPIDEMO_HandleCallSeq(CTRL_STATUS_t* pCtrl,
                                    TD_OS_socket_t nSockFd);
#if defined(LINUX)
IFX_int32_t TAPIDEMO_InitSyslogSocket(IFX_void_t);
IFX_return_t TAPIDEMO_SendLogRemote(IFX_char_t* pBuffer,
   TD_OS_socket_t nSyslogSocket);
IFX_return_t TAPIDEMO_SendLog(IFX_char_t* pMsg);
IFX_return_t TAPIDEMO_PreapareLogRedirection(IFX_void_t);
IFX_return_t TAPIDEMO_CreateDaemon(CTRL_STATUS_t* pCtrl);
#endif
IFX_return_t TAPIDEMO_SetMainBoard(BOARD_t* pBoard);
IFX_return_t TAPIDEMO_CheckBoardCombination(BOARD_t* pBoard);
IFX_return_t TAPIDEMO_SetExtBoard(BOARD_t* pBoard);
IFX_boolean_t TAPIDEMO_ExtBoardCheckForDuplicate(IFX_int32_t nType,
                                                 IFX_int32_t nID);
IFX_return_t TAPIDEMO_StartBoard(IFX_int32_t nIndex);
IFX_return_t TAPIDEMO_RebuildBoardArray(IFX_int32_t nIndex);

#include "faxstruct.h"		/*Definition Will be in this File "FAX_MANAGER_ENABLED"*/

#if FAX_MANAGER_ENABLED
#include <sys/stat.h>
#include <fcntl.h>
/****/
/*End of FAX structures*/

int FaxFifoFd;							//Declaring as a global data so that all functions can access directly
int LteFifoFd;
/**
 * Creating the FAX manager Node to Communicate with the other devices
 * This function is not used. As the node will get created from the
 * parent application
 * */
int fax_mgr_create_pipe()
{
	int RetValue;
	umask(0);
	RetValue=mknod(FAX_FIFO, S_IFIFO|0666, 0);
	if(RetValue < 0)
	{
		printf("Error in creating the node FAX: Manager\n");
	}
}
/*****************************************************************************/

int open_LTE_Pipe()
{
	FaxFifoFd = open(FAX_FIFO, O_WRONLY | O_APPEND);
	if (FaxFifoFd<0)
	{
		printf("Error in opening the FiFO\n");
	}
	fcntl(FaxFifoFd,F_SETFL,0);
}
/*Read Will carried out in a pipe*/
int open_FAX_Pipe()
{
	LteFifoFd = open(LTE_FIFO, O_RDONLY|O_ASYNC);
	if (LteFifoFd<0)
	{
		printf("Error in opening the LTE FiFO\n");
	}
	fcntl(LteFifoFd,F_SETFL,0);
}




#define PACKET_SIZE 4
#define FAX_DATA_PACKET_START '%'
#define FAX_DATA_PACKET_END	'*'

//Sending Event to the Pipe
int sendEventToPipe(FAXEvent_t FaxEvent)
{
	FAXEventPacket_t FaxEventPacket;
	unsigned char str[4];
	str[0]=FAX_DATA_PACKET_START;
	str[1]=FaxEvent.FAXEventType;
	str[2]=FaxEvent.FAXEventData;
	str[3]=FAX_DATA_PACKET_END;

	//FaxEventPacket.PacketStart=FAX_DATA_PACKET_START;
	//FaxEventPacket.PacketStop=FAX_DATA_PACKET_END;
	///FaxEventPacket.FAXData=FaxEvent;

	write(FaxFifoFd,str,PACKET_SIZE);
}


int close_LTE_Pipe()
{
	close(FaxFifoFd);				//Closing the pipe
}

/****/

#endif					//End of FAX_MANAGER macro



/* ============================= */
/* Local function definition     */
/* ============================= */

/* SIGNAL HANDLER */
static IFX_return_t TAPIDEMO_RegisterSigHandler(IFX_void_t);
/*static IFX_void_t TAPIDEMO_CleanUp(IFX_int32_t nSignal);*/
static void TAPIDEMO_CleanUp(int nSignal);
static IFX_int32_t TAPIDEMO_HandleDeviceThread(CTRL_STATUS_t* pCtrl);
#ifdef LINUX
static IFX_int32_t TAPIDEMO_HandleLogThread(TD_OS_ThreadParams_t *pThread);
#endif
static IFX_int32_t TAPIDEMO_HandleSocketThread(TD_OS_ThreadParams_t *pThread);

#ifdef  VXWORKS
/** Holds application task ID. */
static IFX_int32_t nTapiDemo_TID = -1;

/**
   This function is called after task is deleted in clear up everything.

   \param pTcb - pointer to deleted task's WIND_TCB
*/
void deleteHook(WIND_TCB* pTcb)
{
   WIND_TCB* evlog_tcb = IFX_NULL;

   evlog_tcb = taskTcb(nTapiDemo_TID);
   if (evlog_tcb == pTcb)
   {
      /* Only call this when our task is killed */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT, ("deleteHook()\n"));

      TAPIDEMO_CleanUp(0);

      if (taskDeleteHookDelete((FUNCPTR) deleteHook) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, delete task handler when task is deleted. "
               "(File: %s, line: %d)\n",
               __FILE__, __LINE__));
      }
   }
}
#endif /* VXWORKS */

/**
   Block all signals on specific thread.

   Signal handlers are shared between all threads: when a thread calls sigaction(),
   it sets how the signal is handled not only for itself, but for all other
   threads in the program as well.
   On the other hand, signal masks are per-thread: each thread chooses which
   signals it blocks independently of others.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TAPIDEMO_BlockSignals(IFX_void_t)
{
#ifdef LINUX
   static sigset_t signal_mask;
   IFX_return_t ret;
   sigemptyset (&signal_mask);
   sigaddset (&signal_mask, SIGINT);
   sigaddset (&signal_mask, SIGTERM);
   sigaddset (&signal_mask, SIGABRT);
   sigaddset (&signal_mask, SIGQUIT);

   ret = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
   if (ret != 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Block signals for thread.\n"
            "(File: %s, line: %d)\n", __FILE__, __LINE__));
     return IFX_ERROR;
   }
#else
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("Unsupported function TAPIDEMO_BlockSig()\n"));
#endif
   return IFX_SUCCESS;
} /* TAPIDEMO_BlockSignals */

/**
   Register signal handler which is used when task is deleted from LINUX by
   'kill' or CTRL + C command and from VXWORKS by 'td' command. So task can
   cleanup everything.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
static IFX_return_t TAPIDEMO_RegisterSigHandler(IFX_void_t)
{
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
      ("TAPIDEMO_RegisterSigHandler()\n"));
#ifdef LINUX
   /*sighandler_t sig_handler = TAPIDEMO_CleanUp;*/
   struct sigaction act;

   act.sa_handler = TAPIDEMO_CleanUp;
   act.sa_flags = SA_NODEFER; /* Do not prevent signal from being blocked. */
   if (sigaction(SIGABRT, &act, IFX_NULL) < 0) /* CTRL + C */
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, adding signal handler. "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (sigaction(SIGQUIT, &act, IFX_NULL) < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, adding signal handler. "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (sigaction(SIGINT, &act, IFX_NULL) < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, adding signal handler. "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (sigaction(SIGTERM, &act, IFX_NULL) < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, adding signal handler. "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (sigaction(SIGPIPE, &act, IFX_NULL) < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, adding signal handler. "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
#elif VXWORKS
   if (taskDeleteHookAdd((FUNCPTR) deleteHook) != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, adding delete task handler. "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* VXWORKS */

   return IFX_SUCCESS;
} /* TAPIDEMO_RegisterSigHandler */


/**
   Clean up memory after task ends, close all open connections, ...

   \param nSignal - in LINUX signal number in VXWORKS not used
*/
static void TAPIDEMO_CleanUp(int nSignal)
{
   IFX_int32_t i = 0;
   IFX_int32_t ch = 0;
   PHONE_t* pPhone = IFX_NULL;
   BOARD_t* pBoard = IFX_NULL;
#ifdef EASY336
   SVIP_RM_Status_t ret;
#endif
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t *pCpuDevice = IFX_NULL;
#ifndef TAPI_VERSION4
   IFX_TAPI_SIG_DETECTION_t dtmfDetection;
#endif
#ifdef EVENT_LOGGER_DEBUG
   EL_IoctlRegister_t stReg;
   IFX_int32_t nRet;
#endif

#ifdef LINUX
   if ((nSignal == SIGINT) || (nSignal == SIGTERM) || (nSignal == SIGSTOP)
       || (nSignal == SIGABRT) ||(nSignal == SIGQUIT) ||(nSignal == SIGCHLD))
   {
#endif /* LINUX */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("TAPIDEMO_CleanUp()\n"));

      /* check if thread started */
      if (IFX_TRUE == oCtrlStatus.oMultithreadCtrl.oThread.bSocketThreadStarted)
      {
         /* Stop thread: handle sockets */
         Common_StopThread(&oCtrlStatus.oMultithreadCtrl.oThread.oHandleSocketThreadCtrl,
                           &oCtrlStatus.oMultithreadCtrl.oPipe.oSocketSynch,
                           &oCtrlStatus.oMultithreadCtrl.oLock.oSocketThreadStopLock);
         /* reset thread flag */
         oCtrlStatus.oMultithreadCtrl.oThread.bSocketThreadStarted = IFX_FALSE;
      }

      if (IFX_SUCCESS !=
          TD_OS_MutexDelete(&oCtrlStatus.oMultithreadCtrl.oMutex.mutexStateMachine))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, TD_OS_MutexDelete failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
      }

#if (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
      TD_T38_FAX_TEST_DeInit(&oCtrlStatus);
#endif /* (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */

#ifdef TAPI_VERSION4
      /* disable event detection */
      EVENT_DetectionDeInit(&oCtrlStatus);
#endif /* TAPI_VERSION4 */

      /* Cleanup stuff. */
      for (i = 0; i < oCtrlStatus.nBoardCnt; i++)
      {
         pBoard = &oCtrlStatus.rgoBoards[i];

         if (pBoard == IFX_NULL)
         {
            continue;
         }

         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
               ("Clean all phones for this board\n"));

         for (ch = 0; ch < pBoard->nMaxAnalogCh; ch++)
         {
            if (IFX_NULL == pBoard->rgoPhones)
            {
               /* memory already freed or wasn't allocated */
               break;
            }
            pPhone = &pBoard->rgoPhones[ch];

            if (pPhone == IFX_NULL)
            {
               continue;
            }

            /* End with all connections, ringing, phone calls,
               cleanup mappings. Set phone to state S_READY.*/
            if ((&pPhone->rgoConn[0] != IFX_NULL) &&
                (pPhone->rgStateMachine[FXS_SM].nState != S_READY))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                     ("Remove all connections for this phone\n"));

               pPhone->nIntEvent_FXS = IE_HOOKON;
               while (pPhone->nIntEvent_FXS != IE_NONE)
               {
                  ST_HandleState_FXS(&oCtrlStatus, pPhone,
                                     &pPhone->rgoConn[0]);
               }
               if (IFX_TRUE == oCtrlStatus.bInternalEventGenerated)
               {
                  EVENT_InternalEventHandle(&oCtrlStatus);
               }
            }

            /* Set line feed to disabled */
            Common_LineFeedSet(pPhone, IFX_TAPI_LINE_FEED_DISABLED);

            if (pPhone->pCID_Msg != IFX_NULL)
            {
               if (pPhone->pCID_Msg->message != IFX_NULL)
               {
                  TD_OS_MemFree(pPhone->pCID_Msg->message);
                  pPhone->pCID_Msg->message = IFX_NULL;
               }
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                     ("Release CID messages from phone.\n"));
               TD_OS_MemFree(pPhone->pCID_Msg);
               pPhone->pCID_Msg = IFX_NULL;
            }
         } /* for (ch = 0; ... */

         /* disable the DTMF detection */
#ifndef TAPI_VERSION4
         memset(&dtmfDetection, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));
         /* Stop detection of DTMF tones from local interface */
         dtmfDetection.sig = IFX_TAPI_SIG_DTMFTX;
         if (0 < pBoard->nMaxCoderCh)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                  ("%s: Disable DTMF detection on all data channels \n",
                   pBoard->pszBoardName));
         }
         for (ch = 0; ch < pBoard->nMaxCoderCh; ch++)
         {
            if (IFX_NULL == pBoard->nCh_FD)
            {
               /* memory already freed or wasn't allocated */
               break;
            }
            if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[ch],
                                  IFX_TAPI_SIG_DETECT_DISABLE,
                                  (IFX_int32_t) &dtmfDetection,
                                  TD_DEV_NOT_SET, TD_CONN_ID_INIT))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, IFX_TAPI_SIG_DETECT_DISABLE failed "
                   "(File: %s, line: %d)\n", __FILE__, __LINE__));
            }
         }
#endif /* TAPI_VERSION4 */
#ifdef TD_DECT
         /* DECT cleanup only for first board */
         if (0 == i)
         {
            /* close DECT specific FDs and clean up after DECT initialization */
            TD_DECT_Clean(&oCtrlStatus);
         }
#endif /* TD_DECT */
         if (oCtrlStatus.pProgramArg->oArgFlags.nQos)
         {
/* no QoS for SVIP */
#ifndef EASY336
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
               ("QOS_CleanUp()\n"));
            /* QoS Support, cleanup for all boards. */
            QOS_CleanUpSession(&oCtrlStatus, oCtrlStatus.pProgramArg);
#endif /* EASY336 */
         }

         VOIP_Release(oCtrlStatus.pProgramArg, pBoard);

         /* \todo Only one should be set to master? */
         if ((oCtrlStatus.pProgramArg->oArgFlags.nPCM_Master) ||
             (oCtrlStatus.pProgramArg->oArgFlags.nPCM_Slave))
         {
            PCM_Release(oCtrlStatus.pProgramArg, pBoard);
         }

#ifndef EASY336
         /* Unmapp all mapped channels */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Unmap default mapped channels.\n"));
         ABSTRACT_UnmapDefaults(&oCtrlStatus, pBoard, TD_CONN_ID_INIT);
#endif

#ifdef EASY336
         ret = SVIP_RM_DeInit(&pcm_cfg);
         if (ret != SVIP_RM_Success)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Resource manager deinit failed with error code %d. "
                  "(File: %s, line: %d)\n",
                  ret, __FILE__, __LINE__));
         }
         if (IFX_NULL != pcm_cfg.pDevsSVIP)
         {
            TD_OS_MemFree(pcm_cfg.pDevsSVIP);
            pcm_cfg.pDevsSVIP = IFX_NULL;
         }
         if (IFX_NULL != pcm_cfg.pDevsXT16)
         {
            TD_OS_MemFree(pcm_cfg.pDevsXT16);
            pcm_cfg.pDevsXT16 = IFX_NULL;
         }
#endif

         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Close opened file descriptors.\n"));
         /* Close devices, sockets, ... for all boards. */
         Common_Close_FDs(pBoard);

         if ((pDevice = Common_GetDevice_CPU(pBoard->pDevice)) == IFX_NULL)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, CPU device not found. (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
         }
         else
         {
            pCpuDevice = &pDevice->uDevice.stCpu;

            /* when changing country settings memory is allocated
               for new BBD file name, it should be freed */
            if ((pCpuDevice->pszBBD_CRAM_File[0] != IFX_NULL) &&
                (IFX_TRUE ==pBoard->fBBD_Changed))
            {
               TD_OS_MemFree(pCpuDevice->pszBBD_CRAM_File[0]);
               pCpuDevice->pszBBD_CRAM_File[0] = IFX_NULL;
            }
         }

         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
               ("Remove board %s.\n", pBoard->pszBoardName));
         /* Release boards and free memory. */
         pBoard->RemoveBoard(pBoard);

         pBoard = IFX_NULL;
      } /* for (i = 0; ... */

#ifdef TD_TIMER
      IFX_TIM_Shut(TD_TIMER_FIFO_NAME);
      /* close timer FD */
      if (0 <= oCtrlStatus.nTimerFd)
      {
         TD_OS_DeviceClose(oCtrlStatus.nTimerFd);
         oCtrlStatus.nTimerFd = TD_NOT_SET;
      }
#endif /* TD_TIMER */

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("Close opened sockets.\n"));

      if (oCtrlStatus.rgnSockets != IFX_NULL)
      {
         if (!oCtrlStatus.pProgramArg->oArgFlags.nQos ||
             oCtrlStatus.pProgramArg->oArgFlags.nQosSocketStart)
         {
            if (oCtrlStatus.nSumCoderCh > 0)
            {
               VOIP_ReleaseSockets(&oCtrlStatus);
            }

            TD_OS_MemFree(oCtrlStatus.rgnSockets);
            oCtrlStatus.rgnSockets = IFX_NULL;
#ifdef TD_IPV6_SUPPORT
            if (IFX_NULL != oCtrlStatus.rgnSocketsIPv6)
            {
               TD_OS_MemFree(oCtrlStatus.rgnSocketsIPv6);
               oCtrlStatus.rgnSocketsIPv6 = IFX_NULL;
            }
#endif /* TD_IPV6_SUPPORT */
         }
      }
      if (0 <= oCtrlStatus.nAdminSocket)
      {
         /* Close socket used for call administration */
         TD_OS_SocketClose(oCtrlStatus.nAdminSocket);
         oCtrlStatus.nAdminSocket = NO_SOCKET;
      }

#ifdef TD_IPV6_SUPPORT
      if (0 <= oCtrlStatus.rgoCast[TD_CAST_BROAD].nSocket)
      {
         /* Close socket used for broadcast */
         close(oCtrlStatus.rgoCast[TD_CAST_BROAD].nSocket);
         oCtrlStatus.rgoCast[TD_CAST_BROAD].nSocket = NO_SOCKET;
      }
      if (IFX_NULL != oCtrlStatus.rgoCast[TD_CAST_BROAD].aiAddrInfo)
      {
         freeaddrinfo(oCtrlStatus.rgoCast[TD_CAST_BROAD].aiAddrInfo);
         oCtrlStatus.rgoCast[TD_CAST_BROAD].aiAddrInfo = IFX_NULL;
      }
      if (oCtrlStatus.pProgramArg->oArgFlags.nUseIPv6)
      {
         if (0 <= oCtrlStatus.rgoCast[TD_CAST_MULTI].nSocket)
         {
            /* Close socket used for multicast */
            close(oCtrlStatus.rgoCast[TD_CAST_MULTI].nSocket);
            oCtrlStatus.rgoCast[TD_CAST_MULTI].nSocket = NO_SOCKET;
         }
         if (IFX_NULL != oCtrlStatus.rgoCast[TD_CAST_MULTI].aiAddrInfo)
         {
            freeaddrinfo(oCtrlStatus.rgoCast[TD_CAST_MULTI].aiAddrInfo);
            oCtrlStatus.rgoCast[TD_CAST_MULTI].aiAddrInfo = IFX_NULL;
         }
      }
#endif /* TD_IPV6_SUPPORT */

#ifdef EVENT_LOGGER_DEBUG
      if (g_bUseEL == IFX_TRUE)
      {
         strcpy(stReg.sName, "tapidemo");
         stReg.nType = IFX_TAPI_DEV_TYPE_VOICESUB_GW;
         stReg.nDevNum = 0;
         
         nRet = ioctl(TD_EventLoggerFd, EL_UNREGISTER, (IFX_int32_t)&stReg); 
         if (nRet != IFX_SUCCESS)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                ("Error, deregistration in Event Logger failed.\n"));
         }
         g_bUseEL = IFX_FALSE;
      }
#endif /* EVENT_LOGGER_DEBUG*/

      COM_ITM_CLEAN_UP(0);

      /* clean up after ioctl and error trace mechanism */
      TD_DEV_IoctlCleanUp();


#ifdef LINUX
      if (oCtrlStatus.pProgramArg->oArgFlags.nDaemon)
      {
         /* check if thread was started */
         if (IFX_TRUE == oCtrlStatus.oMultithreadCtrl.oThread.bLogThreadStarted)
         {
            /* Stop thread: handle log */
            Common_StopThread(&oCtrlStatus.oMultithreadCtrl.oThread.oHandleLogThreadCtrl,
                              &oCtrlStatus.oMultithreadCtrl.oPipe.oLogSynch,
                              &oCtrlStatus.oMultithreadCtrl.oLock.oLogThreadStopLock);

            /* reset started flag */
            oCtrlStatus.oMultithreadCtrl.oThread.bLogThreadStarted = IFX_FALSE;
         }
         /* Close pipe for redirect log */
         if (IFX_SUCCESS !=
             TD_OS_PipeClose(oCtrlStatus.oMultithreadCtrl.oPipe.oRedirectLog.rgFp[TD_PIPE_IN]))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, TD_OS_PipeClose IN failed. (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
         }
         if (IFX_SUCCESS !=
             TD_OS_PipeClose(oCtrlStatus.oMultithreadCtrl.oPipe.oRedirectLog.rgFp[TD_PIPE_OUT]))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                 ("Err, TD_OS_PipeClose OUT failed. (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
         }
         /* Remove pipe for redirect log */
         remove(oCtrlStatus.oMultithreadCtrl.oPipe.oRedirectLog.rgName);
      } /* Daemon */

      if (oCtrlStatus.pProgramArg->nTraceRedirection ==
          TRACE_REDIRECTION_SYSLOG_LOCAL)
      {
         closelog();
      }
      else if (oCtrlStatus.pProgramArg->nTraceRedirection ==
               TRACE_REDIRECTION_SYSLOG_REMOTE)
      {
         if (0 <= oTraceRedirection.nSyslogSocket)
         {
            TD_OS_SocketClose(oTraceRedirection.nSyslogSocket);
            oTraceRedirection.nSyslogSocket = NO_SOCKET;
         }
      }
      else if (oCtrlStatus.pProgramArg->nTraceRedirection ==
               TRACE_REDIRECTION_FILE)
      {
         if (0 <= oTraceRedirection.pFileLogFD)
         {
            TD_OS_FClose(oTraceRedirection.pFileLogFD);
            oTraceRedirection.pFileLogFD = IFX_NULL;
         }
      }

      /* End with task */
      exit(0);
   }
#endif /* LINUX */
#ifdef LINUX
   else if (nSignal == SIGPIPE)
   {
   /* The signal is generated when send() is used for the socket of type SOCK_STREAM,
      which either is shut down for writing,
      or is connection-mode and is no longer connected.
      Additionaly, send() returns -1 and errno is set to EPIPE.
      Tapidemo doesn't need to do anything here,
      because issue is handled based on the errno code in specific functions.
   */
   }
#endif
} /* TAPIDEMO_CleanUp */


/**
   Turn on services specified by user.

   \param pCtrl - program control structure
*/
IFX_return_t TAPIDEMO_RunServices(CTRL_STATUS_t* pCtrl)
{
   PROGRAM_ARG_t* pProgramArg;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->pProgramArg, IFX_ERROR);

   /* set pointer to program settings structure */
   pProgramArg = pCtrl->pProgramArg;

   TD_OS_PRN_USR_LEVEL_SET(TAPIDEMO, pProgramArg->nDbgLevel);
#ifdef EASY336
   SVIP_RM_SetTraceLevel(pProgramArg->nRmDbgLevel);
#endif

   if (pProgramArg->oArgFlags.nQos)
   {
      QOS_TurnServiceOn(pProgramArg);
   }

   if ((pProgramArg->oArgFlags.nQos) &&
       (pProgramArg->oArgFlags.nQOS_Local))
   {
      /* With local UDP redirection conference should be disabled. */
      pProgramArg->oArgFlags.nConference = 0;
   }

   if (pProgramArg->oArgFlags.nCID)
   {
      CID_SetStandard(pProgramArg->nCIDStandard, TD_CONN_ID_INIT);
   }

#ifdef FXO
   if (pProgramArg->oArgFlags.nFXO)
   {
      if (pProgramArg->oArgFlags.nPCM_Slave)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("The PCM i/f can not be configured as a SLAVE when FXO is used.\n "
             "PCM i/f will be set as MASTER\n"));
         pProgramArg->oArgFlags.nPCM_Slave = 0;
      }
      /* General FXO intialization if needed */
      pProgramArg->oArgFlags.nPCM_Master = 1;
   }
#endif /* FXO */
   return IFX_SUCCESS;
}


/**
   Displays calling sequence.

   \param pCtrl - handle to control status structure

   \return always IFX_SUCCESS
*/
static IFX_int32_t TAPIDEMO_Call_Info(CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t nCh = 0;
   IFX_int32_t j = 0;
   BOARD_t* pBoard = IFX_NULL;
   IFX_char_t* sTableSeparate = IFX_NULL;
#ifndef TAPI_VERSION4
#ifdef FXO
   FXO_t* pFXO;
   TAPIDEMO_DEVICE_FXO_t* pFxoDevice = IFX_NULL;
#endif /* FXO */
#endif /* TAPI_VERSION4 */

#ifndef TAPI_VERSION4
   sTableSeparate = "------------------------------------------------";
#else /* TAPI_VERSION4 */
   sTableSeparate = "----------------------------------------------------------";
#endif /*  TAPI_VERSION4 */

#ifndef TAPI_VERSION4
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("Dialing sequences for different call types:\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" a) Local connection: <c>\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" b) Extern connection: <0iiic>\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" c) PCM connection: <09iiic>\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" d) FXO connection: <05D>\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" e) FEATURE selection first digit must be 'STAR (*),\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("    afterthat feature number (4 digits).\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" f) CONFERENCING first digit must be 'HASH (#)',\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("    afterthat normal dialing sequence,\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" g) Extern connection with full ip:\n"
       "    <0*xxx*xxx*xxx*xxx*c> - each part separated with STAR (*),\n"));
#ifdef TD_IPV6_SUPPORT
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" h) Extern connection with using phonebook: <06PPc>.\n"));
#endif /* TD_IPV6_SUPPORT */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("\n"));

   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("Legend:\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("<c> - phone number.\n"));
#else
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" a) Local connection: <dcc>\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" b) Extern connection: <0iiidcc>\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" c) PCM connection: <09iiidcc>\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" d) FXO connection: <05Ddcc>\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" e) FEATURE selection first digit must be 'STAR (*),\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("    afterthat feature number (4 digits).\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" f) CONFERENCING first digit must be 'HASH (#)',\n"
       "    afterthat normal dialing sequence,\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      (" g) Extern connection with full ip:\n"
       "    <0*xxx*xxx*xxx*xxx*dcc> - each part separated with STAR (*).\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("\n"));

   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("Legend:\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("<d> - device number started from 1 (1 means device 0).\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("<cc> - channel number started from 1\n"
       "       (01 means channel 0, 02 means channel 1, etc.).\n"));
#endif /* TAPI_VERSION4 */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("<iii> - lowest part of external board IP address (a.b.c.<iii>).\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("<D> - FXO number, only one digit at the moment.\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("<xxx*xxx*xxx*xxx> - external board IP address (xxx.xxx.xxx.xxx).\n"));
#ifdef TD_IPV6_SUPPORT
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("<PP> - phonebook index.\n"));
#endif /* TD_IPV6_SUPPORT */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("FXS channels calling table:\n"));
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, (" %s\n", sTableSeparate));
#ifndef TAPI_VERSION4
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("| Dial |          Board          | Phone channel |\n"));
#else
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("| Dial |          Board          | Device | Phone channel  |\n"));
#endif
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("|%s|\n", sTableSeparate));
   for (j = 0; j < pCtrl->nBoardCnt; j++)
   {
      pBoard = &pCtrl->rgoBoards[j];
      for (nCh = 0; nCh < pBoard->nMaxAnalogCh; nCh++)
      {
#ifndef TAPI_VERSION4
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("|  %3d |     %15s     |      %3d      |\n",
             (int) pBoard->rgoPhones[nCh].nPhoneNumber,
             pBoard->pszBoardName, (int) nCh));
#else
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("|  %03d |     %15s     |   %2d   |      %3d       |\n",
             pBoard->rgoPhones[nCh].nPhoneNumber,
             pBoard->pszBoardName,
             pBoard->rgoPhones[nCh].nDev,
             pBoard->rgoPhones[nCh].nPhoneCh));
#endif
      }
   }
#ifdef TD_DECT
   if (!pCtrl->pProgramArg->oArgFlags.nNoDect)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, (" %s\n", sTableSeparate));
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("| Dial |          Board          |  DECT channel |\n"));
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT, (" %s\n", sTableSeparate));
   }
   /* if dect disable nothing should be printed (nMaxAnalogCh == nMaxPhones) */
   for (j = 0; j < pCtrl->nBoardCnt; j++)
   {
      pBoard = &pCtrl->rgoBoards[j];
      for (nCh = pBoard->nMaxAnalogCh; nCh < pBoard->nMaxPhones; nCh++)
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("|  %3d |     %15s     |      %3d      |\n",
             pBoard->rgoPhones[nCh].nPhoneNumber,
             pBoard->pszBoardName,
             pBoard->rgoPhones[nCh].nDectCh));
      }
   }
#endif /* TD_DECT */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, (" %s\n", sTableSeparate));
#ifndef TAPI_VERSION4
#ifdef FXO
   /* check if FXO enabled */
   if (pCtrl->pProgramArg->oArgFlags.nFXO)
   {
      /* check if FXO number is set */
      if (0 < pCtrl->nMaxFxoNumber)
      {
         sTableSeparate = "-------------------------------------------------";
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("\n"));
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("FXO channels calling table:\n"));
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            (" %s\n", sTableSeparate));
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("| Dial |            FXO Device           | FXO ch |\n"));
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("|%s|\n", sTableSeparate));
         /* get next FXO device */
         while ((IFX_SUCCESS == Common_GetNextFxoDevCtrlFd(pCtrl,
                  &pFxoDevice, IFX_FALSE)) &&
                (IFX_NULL != pFxoDevice) &&
                (pFxoDevice->nDevFD >= 0))
         {
            /* for all FXO channels */
            for (nCh=0; nCh < pFxoDevice->nNumOfCh; nCh++)
            {
               /* get FXO channel structure */
               pFXO = &pFxoDevice->rgoFXO[nCh];
               TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                  ("|  05%d | %12s No %1d (%11s) |  %3d   |\n",
                   pFXO->nFXO_Number, pFxoDevice->pFxoTypeName,
                   pFxoDevice->nDevNum,
                   FXO_SetFxoNameString(
                      Common_Enum2Name(pFxoDevice->nFxoType, TD_rgFxoDevName),
                      pFxoDevice->nDevNum, TYPE_CHANNEL, pFXO->nFxoCh),
                   pFXO->nFxoCh));
            } /* for all FXO channels */
         }
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT, (" %s\n", sTableSeparate));
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("\n"));
      } /* if (0 < pCtrl->nMaxFxoNumber) */
      else
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("No FXO channels.\n"));
      }
   } /* if (oCtrlStatus.pProgramArg->oArgFlags.nFXO) */
#endif /* FXO */
#endif /* TAPI_VERSION4 */

   return IFX_SUCCESS;
} /* TAPIDEMO_Call_Info() */

#ifdef STREAM_1_1
/**
   Clear phone buffer containing DTMF, pulse digits.

   \param nDataCh_FD  - file descriptor for data channel
*/
static IFX_void_t TAPIDEMO_ClearDigitBuff(IFX_int32_t nDataCh_FD)
{
   IFX_int32_t ready = 0;
   IFX_int32_t digit = 0;


   /* Check for DTMF digit */
   /* Check the DTMF dialing status, but on data device not control device */
   do
   {
      ready = 0;
      TD_IOCTL(nDataCh_FD, IFX_TAPI_TONE_DTMF_READY_GET, (IFX_int32_t) &ready,
         TD_DEV_NOT_SET, TD_CONN_ID_INIT);
      if (1 == ready)
      {
         /* Digit arrived or was in buffer */
         TD_IOCTL(nDataCh_FD, IFX_TAPI_TONE_DTMF_GET, (IFX_int32_t) &digit,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
      }
      else
      {
         /* No digit in the buffer, buffer is cleared */
         break;
      }
   }
   while (1 == ready);

   /* Check for Pulse digit */
   /* Check the Pulse dialing status, but on data device not control device */
   do
   {
      ready = 0;
      TD_IOCTL(nDataCh_FD, IFX_TAPI_PULSE_READY, (IFX_int32_t) &ready,
         TD_DEV_NOT_SET, TD_CONN_ID_INIT);
      if (1 == ready)
      {
         /* Digit arrived or was in buffer */
         TD_IOCTL(nDataCh_FD, IFX_TAPI_PULSE_GET, (IFX_int32_t) &digit,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
      }
      else
      {
         /* No digit in the buffer, buffer is cleared */
         break;
      }
   }
   while (1 == ready);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
        ("Buffer with DTMF, pulse digits cleared, flushed\n"));

} /* TAPIDEMO_ClearDigitBuff() */
#endif /* STREAM_1_1 */


/**
   Initialize main structures used for testing

   \param pBoard      - board handle
   \param pCtrl       - pointer to status control structure
   \param nPhoneNum   - number of phone

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
static IFX_return_t TAPIDEMO_Setup(BOARD_t* pBoard,
                                   CTRL_STATUS_t* pCtrl,
                                   IFX_int32_t* nPhoneNum)
{
   IFX_int32_t ch = 0;
#ifndef TAPI_VERSION4
   IFX_TAPI_SIG_DETECTION_t dtmfDetection;
#endif

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->pProgramArg, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* Add default ip address and demo arguments */
   TD_SOCK_AddrCpy(&pCtrl->oTapidemo_IP_Addr, &pCtrl->pProgramArg->oMy_IP_Addr);
   pCtrl->nMy_IPv4_Addr =
      TD_SOCK_AddrIPv4Get(&pCtrl->pProgramArg->oMy_IP_Addr) & 0xffffff00;

   if (pBoard->nMaxCoderCh > 0)
   {
      /* Set codec */
      if (IFX_SUCCESS != VOIP_SetCodec(pCtrl))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("VOIP_SetCodec failed.\n"));
         return IFX_ERROR;
      }

      if (IFX_SUCCESS != VOIP_Init(pCtrl->pProgramArg, pBoard))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("VOIP_Init failed.\n"));
         return IFX_ERROR;
      }
   }

   /* PCM is global and initialized in main() function at the moment */

#ifdef STREAM_1_1
   ABSTRACT_UnmapDefaults(pCtrl, pBoard, TD_CONN_ID_INIT);
#endif /* STREAM_1_1 */

   /* Do startup mapping of channels */
   ABSTRACT_DefaultMapping(pCtrl, pBoard, TD_CONN_ID_INIT);

#ifdef TD_DECT
   /* set DECT phones info and configure DECT KPI */
   if (IFX_SUCCESS != TD_DECT_SetPhonesStructureAndKpi(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("TD_DECT_SetPhonesStructureAndKpi failed\n"));
      return IFX_ERROR;
   }
#endif /* TD_DECT */
   /* Channels are already open */
   for (ch = 0; ch < pBoard->nMaxPhones; ch++)
   {
      pBoard->rgoPhones[ch].pBoard = pBoard;
      /* reset event fields */
      pBoard->rgoPhones[ch].nIntEvent_FXS = IE_NONE;
      pBoard->rgoPhones[ch].nIntEvent_Config = IE_NONE;
#ifndef TAPI_VERSION4
      pBoard->rgoPhones[ch].nPhoneNumber = *nPhoneNum;
      *nPhoneNum += 1;
#else
      pBoard->rgoPhones[ch].nPhoneNumber =
         (pBoard->rgoPhones[ch].nDev + 1) * 100 +
         pBoard->rgoPhones[ch].nPhoneCh + 1 + *nPhoneNum;
#endif
      pBoard->rgoPhones[ch].nSeqConnId = TD_CONN_ID_INIT;

      if (ch < pBoard->nMaxAnalogCh)
      {
         /* Now setup FDs and sockets, because startup mapping is done. */
         pBoard->rgoPhones[ch].nPhoneCh_FD =
            Common_GetFD_OfCh(pBoard, pBoard->rgoPhones[ch].nPhoneCh);

         if (pBoard->nMaxCoderCh > 0)
         {
            pBoard->rgoPhones[ch].nDataCh_FD =
               Common_GetFD_OfCh(pBoard, pBoard->rgoPhones[ch].nDataCh);
#ifdef TAPI_VERSION4
            if (pBoard->fUseSockets)
#endif
            {
               /* Set sockets */
               pBoard->rgoPhones[ch].nSocket =
                  VOIP_GetSocket(pBoard->rgoPhones[ch].nDataCh, pBoard,
                                 IFX_FALSE);
            }
         }
         else
         {
            pBoard->rgoPhones[ch].nDataCh_FD = TD_NOT_SET;
         }
      }

      pBoard->rgoPhones[ch].pCID_Msg =
         TD_OS_MemCalloc(1, sizeof(IFX_TAPI_CID_MSG_t));

      if (IFX_NULL != pBoard->rgoPhones[ch].pCID_Msg)
      {
         pBoard->rgoPhones[ch].pCID_Msg->message =
            TD_OS_MemCalloc(TD_CID_ELEM_COUNT,
                            sizeof(IFX_TAPI_CID_MSG_ELEMENT_t));
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, on TD_OS_MemCalloc for phone->pCID_Msg"
                "File: %s, line: %d)\n",
                 __FILE__, __LINE__));
      }

      if (pCtrl->pProgramArg->oArgFlags.nCID)
      {
#ifndef TAPI_VERSION4
         CID_SetupData(ch, &pBoard->rgoPhones[ch]);
#else
         CID_SetupData(pBoard->rgoPhones[ch].nPhoneCh, &pBoard->rgoPhones[ch]);
#endif

         /* Shows CID data */
         CID_Display(&pBoard->rgoPhones[ch], pBoard->rgoPhones[ch].pCID_Msg,
                     CID_DISPLAY_SIMPLE);

         /* Configure CID for this channel */
         if (IFX_SUCCESS != CID_ConfDriver(&pBoard->rgoPhones[ch]))
         {
#ifdef TAPI_VERSION4
            if (pBoard->rgoPhones[ch].pBoard->fSingleFD)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("CID initialization failed for device %d, channel %d. "
                      "(File: %s, line: %d)\n",
                      pBoard->rgoPhones[ch].nDev,
                      pBoard->rgoPhones[ch].nPhoneCh, __FILE__, __LINE__));
            }
            else
#endif
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("CID initialization failed for channel %d. "
                      "File: %s, line: %d)\n",
                      (int)ch, __FILE__, __LINE__));
            }
            Common_Close_FDs(pBoard);
            return IFX_ERROR;
         }
      }

      if (ch < pBoard->nMaxAnalogCh)
      {
         /* set initial states of state machines */
         pBoard->rgoPhones[ch].rgStateMachine[FXS_SM].nState = S_READY;
      }
#ifdef TD_DECT
      else if (ch < (pBoard->nMaxAnalogCh + pBoard->nMaxDectCh))
      {
         /* set initial states of state machines for DECT phones */
         pBoard->rgoPhones[ch].rgStateMachine[FXS_SM].nState =
            S_DECT_NOT_REGISTERED;
      }
#endif /* TD_DECT */
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, %s invalid phone number %d. (File: %s, line: %d)\n",
                pBoard->pszBoardName, ch, __FILE__, __LINE__));
      }
      pBoard->rgoPhones[ch].rgStateMachine[FXS_SM].pStateCombination = ST_rgFXSStates;
      pBoard->rgoPhones[ch].nCallDirection = CALL_DIRECTION_NONE;
      pBoard->rgoPhones[ch].rgStateMachine[CONFIG_SM].nState = S_READY;
      pBoard->rgoPhones[ch].rgStateMachine[CONFIG_SM].pStateCombination = ST_rgConfigStates;
   } /* for */

#ifdef TAPI_VERSION4
   if (pBoard->nMaxAnalogCh != 0 && &pBoard->rgoPhones[pBoard->nMaxAnalogCh-1]
      != IFX_NULL)
      *nPhoneNum = (pBoard->rgoPhones[pBoard->nMaxAnalogCh-1].nDev + 1) * 100;
#endif

#ifdef EASY336
   if (Common_PCM_ConfigSet(pBoard, &pcm_cfg) != IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("PCM configuration set failed "
            "(File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif

#ifndef TAPI_VERSION4
   memset(&dtmfDetection, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));
   /* Stop detection of DTMF tones from local interface */
   dtmfDetection.sig = IFX_TAPI_SIG_DTMFTX;
   if (0 < pBoard->nMaxCoderCh)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("%s: Disable DTMF detection on all data channels \n",
             pBoard->pszBoardName));
   }
   for (ch = 0; ch < pBoard->nMaxCoderCh; ch++)
   {
      if (IFX_SUCCESS != TD_IOCTL(pBoard->nCh_FD[ch],
                            IFX_TAPI_SIG_DETECT_DISABLE,
                            (IFX_int32_t) &dtmfDetection,
                            TD_DEV_NOT_SET, TD_CONN_ID_INIT))
      {
         return IFX_ERROR;
      }
   }
#endif /* TAPI_VERSION4 */
   return IFX_SUCCESS;
} /* TAPIDEMO_Setup() */

/**
   Set file descriptors set for select on device.

   \param pCtrl   - pointer to control structure
   \param pRdFds  - pointer to file descriptors set

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
static IFX_return_t TAPIDEMO_SetDeviceFdSet(CTRL_STATUS_t* pCtrl,
                                            TD_OS_devFd_set_t* pRdFds)
{
   IFX_int32_t i, j;
   BOARD_t* pBoard;
   IFX_int32_t fd_ch, fd_dev_ctrl;
#ifdef FXO
   TAPIDEMO_DEVICE_FXO_t* pFxoDev = IFX_NULL;
#endif /* FXO */

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pRdFds, IFX_ERROR);
#if FAX_MANAGER_ENABLED
   int n;
   char d;
 			//FAX pipe for the incoming call
 printf("Socket Exited \n");
 //pCtrl->nMaxFdDevice++;
 #endif

   /* reset structure */
   TD_OS_DevFdZero(pRdFds);

   /* for all boards */
   for (j = 0; j < pCtrl->nBoardCnt; j++)
   {
      /* get board */
      pBoard = &pCtrl->rgoBoards[j];

      /* Add all board file descriptor */
      fd_dev_ctrl = pBoard->nDevCtrl_FD;
      if (0 <= fd_dev_ctrl)
      {
         TD_DEV_FD_SET(fd_dev_ctrl, pRdFds, pCtrl->nMaxFdDevice);
      }
      /* some boards could not have control device file descriptor. */
      else if (Common_GetDevice_CPU(pBoard->pDevice) != IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Control device not open, %s. (File: %s, line: %d)\n",
                pBoard->pszBoardName, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* set fds responsible for communication/calls that use data channels
         e.g. voip connections */
#ifdef TAPI_VERSION4
      if (!pBoard->fSingleFD)
#endif
      {
         /* Set up file descriptor table without sockets */
         for (i = 0; i < pBoard->nMaxCoderCh; i++)
         {
            fd_ch = Common_GetFD_OfCh(pBoard, i);
            if (0 > fd_ch)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Err, File descriptor for channel %d not open. "
                      "(File: %s, line: %d)\n",
                      (int) i, __FILE__, __LINE__));
               return IFX_ERROR;
            }

#ifdef STREAM_1_1
            /* Clear DTMF buffer for each phone channel */
            TAPIDEMO_ClearDigitBuff(fd_ch);
#endif /* STREAM_1_1 */
            /* Put file descriptors into the read set */
            TD_DEV_FD_SET(fd_ch, pRdFds, pCtrl->nMaxFdDevice);
         } /* for - go through all coder channels */
      } /* if (!pBoard->fSingleFD */
   } /* for - go through all boards */
#if 1
   TD_DEV_FD_SET(LteFifoFd, pRdFds, pCtrl->nMaxFdDevice);
#endif
#ifdef FXO
   /* add FXO device control fd */
   if (pCtrl->pProgramArg->oArgFlags.nFXO)
   {
      if (0 < pCtrl->nMaxFxoNumber)
      {
         IFX_int32_t err;
         while ((IFX_SUCCESS == (err = Common_GetNextFxoDevCtrlFd(pCtrl,
                  &pFxoDev, IFX_TRUE))) &&
                (IFX_NULL != pFxoDev) &&
                (pFxoDev->nDevFD >= 0))
         {
            /* Add config fd to fd set */
            TD_DEV_FD_SET(pFxoDev->nDevFD, pRdFds, pCtrl->nMaxFdDevice);
         }

         /* check return value, see Common_GetNextFxoDevCtrlFd description
            for details */
         if ((IFX_ERROR == err) && (IFX_NULL == pFxoDev))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, Number of FXOs %d but no FXO device FD was found."
                   " (File: %s, line: %d)\n",
                   pCtrl->nMaxFxoNumber, __FILE__, __LINE__));
         }
         else if (IFX_ERROR == err)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, Geting FXO device FD failed. (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
         }
      }
   } /* if FXO */
#endif /* FXO */

#ifdef TD_TIMER
   /* check and add timer FD */
   if (0 <= pCtrl->nTimerFd)
   {
      TD_DEV_FD_SET(pCtrl->nTimerFd, pRdFds, pCtrl->nMaxFdDevice);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Invalid pCtrl->nTimerFd = %d\n(File: %s, line: %d)\n",
             pCtrl->nTimerFd, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check and add timer FIFO FD */
   if (0 <= pCtrl->nTimerMsgFd)
   {
      TD_DEV_FD_SET(pCtrl->nTimerMsgFd, pRdFds, pCtrl->nMaxFdDevice);
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Invalid pCtrl->nTimerMsgFd = %d\n(File: %s, line: %d)\n",
             pCtrl->nTimerMsgFd, __FILE__, __LINE__));
      return IFX_ERROR;
   }
#endif /* TD_TIMER */

#ifdef TD_DECT
   if (!pCtrl->pProgramArg->oArgFlags.nNoDect)
   {
      /* check and add paging key FD */
      if (0 <= pCtrl->viPagingKeyFd)
      {
         TD_DEV_FD_SET(pCtrl->viPagingKeyFd, pRdFds, pCtrl->nMaxFdDevice);
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Invalid pCtrl->viPagingKeyFd = %d\n(File: %s, line: %d)\n",
                pCtrl->viPagingKeyFd, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* if (!pCtrl->pProgramArg->oArgFlags.nNoDect) */
#endif /* TD_DECT */

   return IFX_SUCCESS;
} /* TAPIDEMO_SetDeviceFdSet */

/**
   Set file descriptors set for select on sockets and pipes.

   \param pCtrl   - pointer to control structure
   \param pRdFds  - pointer to file descriptors set

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
static IFX_return_t TAPIDEMO_SetSocketFdSet(CTRL_STATUS_t* pCtrl,
                                            TD_OS_socFd_set_t* pRdFds)
{
   IFX_int32_t i, j;
   BOARD_t* pBoard;
   IFX_int32_t nSocket;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pRdFds, IFX_ERROR);

   /* reset structure */
   TD_OS_SocFdZero(pRdFds);

   /* for all boards */
   for (j = 0; j < pCtrl->nBoardCnt; j++)
   {
      /* get board */
      pBoard = &pCtrl->rgoBoards[j];

#ifdef TAPI_VERSION4
      if (!pBoard->fSingleFD)
#endif
      {
         /* Set up file descriptor table without sockets */
         for (i = 0; i < pBoard->nMaxCoderCh; i++)
         {
#ifdef TAPI_VERSION4
            if (pBoard->fUseSockets)
#else /* TAPI_VERSION4 */
            if (!pCtrl->pProgramArg->oArgFlags.nQos)
#endif /* TAPI_VERSION4 */
            {
               /* Set possible sockets */
               nSocket = VOIP_GetSocket(i, pBoard, IFX_FALSE);
               if (NO_SOCKET != nSocket)
               {
                  TD_SOC_FD_SET(nSocket, pRdFds, pCtrl->nMaxFdSocket);
               }
#ifdef TD_IPV6_SUPPORT
               /* if IPv6 support is on */
               if (pCtrl->pProgramArg->oArgFlags.nUseIPv6)
               {
                  /* Set possible sockets */
                  nSocket = VOIP_GetSocket(i, pBoard, IFX_TRUE);
                  if (NO_SOCKET != nSocket)
                  {
                     TD_SOC_FD_SET(nSocket, pRdFds, pCtrl->nMaxFdSocket);
                  }
               } /* if IPv6 support is on */
#endif /* TD_IPV6_SUPPORT */
            } /* if !QoS */
         } /* for - go through all coder channels */
      } /* if (!pBoard->fSingleFD */
   } /* for - go through all boards */

   /* Also add administration socket for handling calls */
   if (pCtrl->nAdminSocket == NO_SOCKET)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Couldn't get administration socket. "
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      /* Without this socket we cannot make calls. */
      return IFX_ERROR;
   }
   TD_SOC_FD_SET(pCtrl->nAdminSocket, pRdFds, pCtrl->nMaxFdSocket);

   if (pCtrl->oMultithreadCtrl.oPipe.oSocketSynch.rgFd[TD_PIPE_OUT]
       == NO_SOCKET)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Couldn't get thread synch pipe.\n"
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   TD_SOC_FD_SET(pCtrl->oMultithreadCtrl.oPipe.oSocketSynch.rgFd[TD_PIPE_OUT],
             pRdFds, pCtrl->nMaxFdSocket);

#ifdef TD_IPV6_SUPPORT
   /* add broadcast socket */
   TD_SOC_FD_SET(pCtrl->rgoCast[TD_CAST_BROAD].nSocket, pRdFds,
                 pCtrl->nMaxFdSocket);

   if (pCtrl->pProgramArg->oArgFlags.nUseIPv6)
   {
      /* add multicast socket */
      TD_SOC_FD_SET(pCtrl->rgoCast[TD_CAST_MULTI].nSocket, pRdFds,
                    pCtrl->nMaxFdSocket);
      if (TD_NOT_SET != pCtrl->oIPv6_Ctrl.nSocketFd)
      {
         /* add IPv6 admin socket */
         TD_SOC_FD_SET(pCtrl->oIPv6_Ctrl.nSocketFd, pRdFds, pCtrl->nMaxFdSocket);
      }
   }
#endif /* TD_IPV6_SUPPORT */

   /* Add communication sockets for handling control PC's requests
      (used for ITM) */
   COM_SOCKET_FD_SET(pRdFds, pCtrl->nMaxFdSocket);

#ifdef TD_FAX_MODEM
#if (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /* Add communication socket for handling PC's requests */
   if(NO_SOCKET != pCtrl->oFaxTestCtrl.nComSocket)
   {
      /* check if using T.38 fax test, if not then leave function */
      TD_T38_FAX_TEST_CHECK_IF_WORKING(&pCtrl->oFaxTestCtrl);
      /* add socket */
      TD_SOC_FD_SET(pCtrl->oFaxTestCtrl.nComSocket, pRdFds, pCtrl->nMaxFdSocket);
   }
#endif /* (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
#endif /* TD_FAX_MODEM */


#ifdef TD_DECT
   if (!pCtrl->pProgramArg->oArgFlags.nNoDect)
   {
      /* add DECT communication pipe if available */
      if ((IFX_TRUE == pCtrl->oDectStackCtrl.nDectInitialized) ||
          (0 < pCtrl->nDectPipeFdOut))
      {
         TD_SOC_FD_SET(pCtrl->nDectPipeFdOut, pRdFds, pCtrl->nMaxFdSocket);
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Unable to add DECT pipe FD (%d, %d)\n(File: %s, line: %d)\n",
                pCtrl->oDectStackCtrl.nDectInitialized,
                pCtrl->nDectPipeFdOut,
                __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* check and add CLI FD */
      if (0 <= pCtrl->viFromCliFd)
      {
         TD_SOC_FD_SET(pCtrl->viFromCliFd, pRdFds, pCtrl->nMaxFdSocket);
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Invalid pCtrl->viFromCliFd = %d\n(File: %s, line: %d)\n",
                pCtrl->viFromCliFd, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* if (!pCtrl->pProgramArg->oArgFlags.nNoDect) */
#endif /* TD_DECT */

   return IFX_SUCCESS;
} /* TAPIDEMO_SetSocketFdSet */

/**
   Clear phone after connection.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  nInternalEvent - new event information
   \param  pBoard - board handle

   \return Phone is cleared.
*/
IFX_void_t TAPIDEMO_ClearPhone(CTRL_STATUS_t* pCtrl, PHONE_t* pPhone,
                               CONNECTION_t* pConn,
                               INTERNAL_EVENTS_t nInternalEvent,
                               BOARD_t* pBoard)
{
   IFX_int32_t fd_data_ch = -1;
   TD_LINE_MODE_t nMode;

   /* check input parameters */
   TD_PTR_CHECK(pCtrl,);
   TD_PTR_CHECK(pPhone,);
   TD_PTR_CHECK(pBoard,);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
         ("Phone No %d: TAPIDEMO_ClearPhone.\n",
          pPhone->nPhoneNumber));

   /* set connection pointer for FXO_CALL*/
   if ((pPhone->fFXO_Call == IFX_TRUE) ||
       /* if pointer not set and connection in progress */
       ((pPhone->nConnCnt == 1) && (pConn == IFX_NULL)))
   {
      pConn = &pPhone->rgoConn[0];
   }
   /************************
    TURN OFF SPECIAL FEATURES
    ************************/
   /* VAD works only for Data channels */
   if (0 < pBoard->nMaxCoderCh)
   {
      Common_SetVad(pPhone, pBoard, RESTORE_DEFAULT);
   }
   /* if wideband was set then change back narrowband
      if AGC was enabled then disable AGC
      AGC must be disabled before disabling encoder */
   if ((pBoard != IFX_NULL) && (pConn != IFX_NULL))
   {
#ifdef TD_DECT
      if (PHONE_TYPE_DECT == pPhone->nType)
      {
         /* do nothing here for DECT phones */
      }
      else
#endif /** TD_DECT */
      /* Set back to default features */
      if ((pPhone->fWideBand_Cfg) || (pPhone->fAGC_Cfg))
      {
         /* Set line type, coders, ... to some default values */
         if (pBoard->nMaxCoderCh > 0)
         {
            fd_data_ch = VOIP_GetFD_OfCh(pConn->nUsedCh, pBoard);
         }
         else if (pBoard->nMaxAnalogCh > 0)
         {
            fd_data_ch = Common_GetFD_OfCh(pBoard, pConn->nUsedCh);
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Board %s without coders and analog channels. "
                   "(File: %s, line: %d)\n",
                   pBoard->pszBoardName, __FILE__, __LINE__));
         }
         /* change back to NarrowBand if WideBand was enabled */
         if (pPhone->fWideBand_Cfg)
         {
            Common_ConfigureTAPI(pCtrl, pBoard, pPhone, 2000,
                                 fd_data_ch, IFX_TRUE, IFX_FALSE,
                                 pPhone->nSeqConnId);
            pPhone->fWideBand_Cfg = IFX_FALSE;
         }
         /* disable AGC if enabled */
         if (pPhone->fAGC_Cfg)
         {
            Common_ConfigureTAPI(pCtrl, pBoard, pPhone, 2010,
                                 fd_data_ch, IFX_TRUE, IFX_FALSE,
                                 pPhone->nSeqConnId);
            pPhone->fAGC_Cfg = IFX_FALSE;
         }
      } /* if ((pPhone->fWideBand_Cfg) || (pPhone->fAGC_Cfg)) */

   } /* if ((pBoard != IFX_NULL) && (pConn != IFX_NULL)) */

   /************************
      END CONNECTION
    ************************/
   if (IFX_NULL != pConn)
   {
      if (UNKNOWN_CALL_TYPE == pConn->nType)
      {
         pConn = &pPhone->rgoConn[0];
      }
/* CONFERENCE CONFERENCE CONFERENCE CONFERENCE CONFERENCE CONFERENCE */
      /* Conference clean up*/
      if (pPhone->nConfStarter)
      {
         if (IFX_ERROR == CONFERENCE_End(pCtrl, pPhone,
                             &pCtrl->rgoConferences[pPhone->nConfIdx - 1]))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("Failed to end conference properly. Could cause "
                   "mapping related problems. (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
         }
      }
#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
/* FXO CALL FXO CALL FXO CALL FXO CALL FXO CALL FXO CALL FXO CALL FXO CALL */
      else if (FXO_CALL == pConn->nType)
      {
         if (pCtrl->pProgramArg->oArgFlags.nFXO)
         {
            if (IFX_NULL != pPhone->pFXO)
            {
               /* stop FXO call */
               if (pPhone->pFXO == IFX_NULL)
               {
                  /* should not happend*/
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Err, PHONE_t:pFXO is NULL. (File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
               else if ((pPhone->pFXO->nState == S_ACTIVE) ||
                        (pPhone->pFXO->nState == S_IN_RINGING))
               {
                  FXO_EndConnection(pCtrl->pProgramArg, pPhone->pFXO,
                                    pPhone, pConn, pBoard);
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                        ("Wrong FXO state. (File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
            }
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("pPhone->fFXO_Call is not set. (File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                  ("FXO calls are not enbled. (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
         }
      }
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */
/* PCM CALLS  PCM CALLS PCM CALLS PCM CALLS PCM CALLS PCM CALLS PCM CALLS */
#ifndef EASY336
      else if (PCM_CALL == pConn->nType ||
               LOCAL_PCM_CALL == pConn->nType ||
               LOCAL_BOARD_PCM_CALL == pConn->nType)
      {
         TAPIDEMO_PORT_DATA_t oPortData;
         oPortData.nType = PORT_FXS;
         oPortData.uPort.pPhopne = pPhone;
         if (PCM_EndConnection(&oPortData, pCtrl->pProgramArg, pBoard, pConn,
                               pPhone->nSeqConnId))
         {
#if (!defined(EASY3201) && !defined(EASY3201_EVS) && !defined(XT16))
#ifdef TD_DECT
            /* DECT phone is unmapped from PCM at end of this function
               if going on-hook */
            if (PHONE_TYPE_DECT != pPhone->nType)
#endif /* TD_DECT */
            {
#ifdef EASY3111
               /* for this board type unmapping is not needed */
               if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
#endif /* EASY3111 */
               {
                  /* Unmap phone and pcm channels */
                  PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh,
                                 IFX_FALSE, pBoard, pPhone->nSeqConnId);
               }
            }
#endif /* !EASY3201 && !EASY3201_EVS && !XT16 */
         }
      }
#endif /* EASY336 */
      else
      {
/* CALL THAT USES CODECS CALL THAT USES CODECS CALL THAT USES CODECS */
         /* Stop coder if voip call or local call that uses codec*/
         if ((EXTERN_VOIP_CALL == pConn->nType) ||
             (LOCAL_BOARD_CALL == pConn->nType) ||
             ((LOCAL_CALL == pConn->nType) &&
              (pCtrl->pProgramArg->oArgFlags.nUseCodersForLocal)))
         {
            if (IFX_TRUE == pConn->bExtDialingIn)
            {
               /* Clean external connection */
               pConn->bExtDialingIn = IFX_FALSE;
               TD_TIMEOUT_STOP(pCtrl->nUseTimeoutSocket);
            }
            /* codec is started when connection is active */
            if (IFX_TRUE == pConn->fActive)
            {
#ifndef TAPI_VERSION4
               VOIP_StopCodec(pConn->nUsedCh, pConn, pBoard, pPhone);
#else
#ifdef EASY336
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                     ("pPhone->nVoipIDs: %d\n", pPhone->nVoipIDs));
               /* if conference then codec is stopped in CONFERENCE_End() */
               if ((0 < pPhone->nVoipIDs) &&
                   (IFX_FALSE == pPhone->nConfStarter))
               {
#if defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)
                  if (TD_FAX_MODE_FDP != pCtrl->nFaxMode)
#endif /* defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW) */
                  {
                     if (pBoard->fSingleFD)
                     {
                        /* check if vad settings were changed */
                        Common_SetVad(pPhone, pBoard, RESTORE_DEFAULT);
                     }
                     VOIP_StopCodec(pConn->nUsedCh, pConn,
                        ABSTRACT_GetBoard(pCtrl, IFX_TAPI_DEV_TYPE_VIN_SUPERVIP),
                        pPhone);
                  }
#if  defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)
                  else
                  {
                     VOIP_FaxStop(pCtrl, pConn,
                        ABSTRACT_GetBoard(pCtrl, IFX_TAPI_DEV_TYPE_VIN_SUPERVIP),
                        pPhone);
                  }
#endif /* defined(TD_FAX_MODEM) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW) */
               } /* if (0 < pPhone->nVoipIDs) */
#endif /* EASY336 */
#endif /* TAPI_VERSION4 */
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
/* TAPI SIGNAL SUPPORT TAPI SIGNAL SUPPORT TAPI SIGNAL SUPPORT */
               /* fax modem only avaible for voip calls */
               if (EXTERN_VOIP_CALL == pConn->nType)
               {
                  /* reset signal handlers */
                  SIGNAL_HandlerReset(pPhone, pConn);
               }
            } /* if (IFX_TRUE == pConn->fActive) */

         } /* connection that uses codecs */
#ifndef EASY336
/* LOCAL CALL LOCAL CALL LOCAL CALL LOCAL CALL LOCAL CALL LOCAL CALL */
         else if (LOCAL_CALL == pConn->nType)
         {
            /* Unmap phone channels it will be done for phone that uses
            ClearPhone first */
            Common_MapLocalPhones(pPhone, pConn->oConnPeer.oLocal.pPhone,
                                  TD_MAP_REMOVE);
         }  /* if ((LOCAL_CALL != pConn->nType) */
#endif
      }
#ifdef EASY336
      pConn->voipID = SVIP_RM_UNUSED;
#endif /* EASY336 */
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
      /* connection is no longer active */
      pConn->fActive = IFX_FALSE;
      pConn->bIPv6_Call = IFX_FALSE;
   } /* if (IFX_NULL != pConn) */
#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      if (TD_NOT_SET != pPhone->nDataCh)
      {
         TD_DECT_DataChannelRemove(pPhone);
      }
      if (TD_NOT_SET != pPhone->nPCM_Ch)
      {
         TD_DECT_PCM_ChannelRemove(pPhone);
      }
      if (IE_READY == nInternalEvent)
      {
         /* disconnect with DECT handset - "on-hook" on DECT handset */
         TD_DECT_HookOn(pPhone);
      }
   }
#endif /* TD_DECT */
   /************************
      RESET PHONE STRUCTURE
    ************************/
   if (nInternalEvent != IE_BUSY)
   {
      pPhone->nCallDirection = CALL_DIRECTION_NONE;
   }
   /* Here we should also clear connections. */
   pPhone->nDialNrCnt = 0;
   pPhone->fExtPeerCalled = IFX_FALSE;
   pPhone->fPCM_PeerCalled = IFX_FALSE;
   pPhone->nConnCnt = 0;
   pPhone->fConfTAPI_InConn = IFX_FALSE;
   pPhone->fFXO_Call = IFX_FALSE;
   pPhone->pFXO = IFX_NULL;

   /************************
   TURN OFF UNNEEDED RESOURCES
    ************************/
   /* phone goes on hook - turn off unneeded resources and change line feed */
   if (nInternalEvent == IE_READY)
   {
#ifdef TAPI_VERSION4
      if (!pBoard->fSingleFD)
#endif /* TAPI_VERSION4 */
      {
#ifndef TAPI_VERSION4
         /* disable the DTMF detection */
         Common_DTMF_SIG_Disable(pCtrl, pPhone);
         /* Disable the LEC*/
         Common_LEC_Disable(pCtrl, pPhone);
#endif /* TAPI_VERSION4 */
      }
#ifdef TD_PPD
      if (pCtrl->pProgramArg->oArgFlags.nDisablePpd)
      {
         nMode = IFX_TAPI_LINE_FEED_STANDBY;
      }
      else
      {
         nMode = IFX_TAPI_LINE_FEED_PHONE_DETECT;
      }
#else
      nMode = IFX_TAPI_LINE_FEED_STANDBY;
#endif /* TD_PPD */
      /* Set line in standby */
      if (IFX_ERROR == Common_LineFeedSet(pPhone, nMode))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
               ("Err, line feed set failed for ch %d, %s."
                "(File: %s, line: %d)\n",
                pPhone->nPhoneCh, pBoard->pszBoardName, __FILE__, __LINE__));
      } /* if (IFX_ERROR == Common_LineFeedSet()) */
   } /* if (nInternalEvent == IE_READY) */

} /* TAPIDEMO_ClearPhone() */

/**
   SetAction    : set action of the phone we are talking to.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to phone connection
   \param nAction - which action to send to other phone
   \param nSeqConnId - Seq Conn ID

   \return       IFX_SUCCESS - no error, otherwise IFX_ERROR
*/
IFX_return_t TAPIDEMO_SetAction(CTRL_STATUS_t* pCtrl,
                                PHONE_t* pPhone,
                                CONNECTION_t* pConn,
                                IFX_int32_t nAction,
                                IFX_uint32_t nSeqConnId)
{
   PHONE_t* p_dst_phone = IFX_NULL;
   IFX_int32_t ret = IFX_SUCCESS;
   TD_OS_sockAddr_t to_addr;
   TD_COMM_MSG_t msg = {0};
   CONNECTION_t* p_conn = IFX_NULL;
   IFX_char_t* psActionName = IFX_NULL;
   TD_OS_socket_t nUsedSock;

   /* check input parameter */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* set pointer string with action name */
   psActionName = Common_Enum2Name(nAction, TD_rgIE_EventsName);
   /* check if name was set, if no name is set,
      then invalid value of TD_rgIE_EventsName is set */
   TD_PTR_CHECK(psActionName, IFX_ERROR);

#ifdef TD_IPV6_SUPPORT
   if (IFX_TRUE == pConn->bIPv6_Call)
   {
      nUsedSock = pCtrl->oIPv6_Ctrl.nSocketFd;
   }
   else
#endif /* TD_IPV6_SUPPORT */
   {
      nUsedSock = pCtrl->nAdminSocket;
   }
   /* prepare message end and start */
   msg.nMarkStart = COMM_MSG_START_FLAG;
   msg.nMarkEnd = COMM_MSG_END_FLAG;
   /* set msg size and version */
   msg.nMsgLength = sizeof(msg);
   msg.nMsgVersion = TD_COMM_MSG_CURRENT_VERSION;
   /* set call type */
   if (EXTERN_VOIP_CALL == pConn->nType)
   {
      msg.fPCM = CALL_FLAG_VOIP;
   }
   else if (PCM_CALL == pConn->nType)
   {
      msg.fPCM = CALL_FLAG_PCM;
   }
   else
   {
      msg.fPCM = CALL_FLAG_UNKNOWN;
   }
   /* Set action to external phone, for example tell him that
      we are calling */
   msg.nAction = nAction;
   /* set feature ID */
   msg.nFeatureID = pConn->nFeatureID;

   /* set Seq Conn ID */
   msg.oData2.oSeqConnId.nType = TD_COMM_MSG_DATA_1_CONN_ID;
   msg.oData2.oSeqConnId.nSeqConnID = nSeqConnId;

   /* For this two actions no phone and connection are selected,
      pPhone and pConn are phony only for message send use. */
   if ((nAction == IE_EXT_CALL_PROCEEDING) ||
       (nAction == IE_EXT_CALL_WRONG_NUM))
   {
      /* Action send to external phone, that tried to call phone on this board,
         tell him that phone number is incomplete or wrong */

      /* called phone info */
      msg.nBoard_IDX = NO_BOARD_IDX;
      msg.nSenderPhoneNum = pPhone->nPhoneNumber;
      /* no real connection set port number to 0*/
      msg.nSenderPort = 0;
      /* copy board MAC address */
      memcpy(msg.MAC, pConn->oUsedMAC, ETH_ALEN);

      /* caller phone info */
      msg.nReceiverPhoneNum = pConn->oConnPeer.nPhoneNum;

      /* set peer address */
      TD_SOCK_AddrCpy(&to_addr, &pConn->oConnPeer.oRemote.oToAddr);
      TD_SOCK_PortSet(&to_addr, ADMIN_UDP_PORT);

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Sending MSG using socket %d to [%s]:%d:\n"
             " - call type %s\n"
             " - sender MAC %02X:%02X:%02X:%02X:%02X:%02X\n"
             " - sender phone number %d\n"
             " - receiver phone number %d\n"
             " - new internal event %s\n"
             " - feature id %d\n",
             (int) pCtrl->nAdminSocket,
             TD_GetStringIP(&to_addr), TD_SOCK_PortGet(&to_addr),
             (CALL_FLAG_PCM == msg.fPCM)? "PCM" :
             (CALL_FLAG_VOIP == msg.fPCM) ? "EXTERN" : "UNKNOWN",
             msg.MAC[0],
             msg.MAC[1],
             msg.MAC[2],
             msg.MAC[3],
             msg.MAC[4],
             msg.MAC[5],
             msg.nSenderPhoneNum, msg.nReceiverPhoneNum,
             psActionName, msg.nFeatureID));

      ret = TD_OS_SocketSendTo(nUsedSock, (IFX_char_t *) &msg,
                               sizeof(msg), &to_addr);
      if (sizeof(msg) != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
              ("Err, sending data (command %d byte(s)), %s. "
               "(File: %s, line: %d)\n",
               sizeof(msg), strerror(errno), __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
#ifdef TD_USE_EXT_CALL_WORKAROUND_EASY508XX
   else if (nAction == IE_EXT_CALL_WORKAROUND_EASY508xx)
   {
      /* It is a workaround, to avoid problem with first packet being dropped
         for first external/PCM connection to EASY508xx boards.

      We need to send any packet to external board ESY508xx before starting
      to establish external connection with it. If we don't do it, first packet
      during first external connection will be not detected.
      In this point, we neither know with what type of board we make connection
      nor if it is first connection, so we have to send this packet before every
      exernal connection. This packet will be just ignore by TAPIDEMO
      on secont board  */

      /* reset feature ID */
      msg.nFeatureID = 0;

      /* called phone info */
      msg.nBoard_IDX = NO_BOARD_IDX;
      msg.nSenderPhoneNum = 0;
      /* no real connection set port number to 0*/
      msg.nSenderPort = 0;
      /* copy board MAC address */
      memcpy(msg.MAC, pConn->oUsedMAC, ETH_ALEN);

      /* caller phone info */
      msg.nReceiverPhoneNum = 0;
      /* set call type */
      if (EXTERN_VOIP_CALL == pConn->nType)
      {
         /* set peer address */
         TD_SOCK_AddrCpy(&to_addr, &pConn->oConnPeer.oRemote.oToAddr);
      }
      else if (PCM_CALL == pConn->nType)
      {
         /* set peer address */
         TD_SOCK_AddrCpy(&to_addr, pConn->oConnPeer.oPCM.oToAddr);
      }
      TD_SOCK_PortSet(&to_addr, ADMIN_UDP_PORT);
      ret = TD_OS_SocketSendTo(nUsedSock, (IFX_char_t *) &msg,
                               sizeof(msg),&to_addr);
      if (sizeof(msg) != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
              ("Err, sending data (command %d byte(s)), %s. "
               "(File: %s, line: %d)\n",
               sizeof(msg), strerror(errno), __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* else if (nAction == IE_EXT_CALL_WORKAROUND_EASY508xx) */
#endif /* TD_USE_EXT_CALL_WORKAROUND_EASY508XX */
   else
   {
#ifndef TAPI_VERSION4
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Phone No %d: SetAction: ConnType %s, Action %s\n"
             "%s(PCM Ch %d, DataCh %d, PhoneCh %d)\n",
             (int) pPhone->nPhoneNumber,
             Common_Enum2Name(pConn->nType, TD_rgCallTypeName), psActionName,
             pPhone->pBoard->pIndentPhone,
             (int) pPhone->nPCM_Ch, (int) pPhone->nDataCh,(int) pPhone->nPhoneCh));
#else /* TAPI_VERSION4 */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
            ("Phone No %d: %s device %d, PhoneCh %d,\n"
             "%sSetAction: ConnType %s, Action %s\n",
             (int) pPhone->nPhoneNumber,
             pPhone->pBoard->pszBoardName, pPhone->nDev, pPhone->nPhoneCh,
             pPhone->pBoard->pIndentPhone,
             Common_Enum2Name(pConn->nType, TD_rgCallTypeName), psActionName));
#endif /* TAPI_VERSION4 */
      /* check event type */
      if (IE_RINGING == nAction &&
          ((PCM_CALL == pConn->nType) || (EXTERN_VOIP_CALL == pConn->nType)))
      {
         /* check if CID support is on */
         if (pCtrl->pProgramArg->oArgFlags.nCID)
         {
            /* get CID data */
            if (IFX_SUCCESS != CID_FromPeerMsgSet(pPhone, &msg))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                     ("Err, CID_FromPeerMsgGet failed. (File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
         } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */
      } /* if (IE_RINGING == event) */

      if ((LOCAL_CALL == pConn->nType) || (LOCAL_BOARD_CALL == pConn->nType) ||
          (LOCAL_PCM_CALL == pConn->nType) ||
          (LOCAL_BOARD_PCM_CALL == pConn->nType))
      {
         /* Set action to local phone, for example tell him that
            we are calling */
         p_dst_phone = pConn->oConnPeer.oLocal.pPhone;

         if (IFX_NULL == p_dst_phone)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Pointer to destination phone is NULL, target phone is missing!"
                   " (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
         }
#ifdef TD_DECT
         /* print info for DECT phone */
         if (PHONE_TYPE_DECT == p_dst_phone->nType)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                  ("SetAction on destination Phone No %d: DectCh %d\n",
                   p_dst_phone->nPhoneNumber, p_dst_phone->nDectCh));
         }
         else
#endif /* TD_DECT */
         {
#ifndef TAPI_VERSION4
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                  ("SetAction on destination PhoneNo %d: DataCh %d, PhoneCh %d\n",
                   (IFX_int32_t)p_dst_phone->nPhoneNumber,
                   (int) p_dst_phone->nDataCh, (int) p_dst_phone->nPhoneCh));
#else /* TAPI_VERSION4 */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                  ("SetAction on destination PhoneNo %d: %s device %d, ch %d\n",
                   (IFX_int32_t)p_dst_phone->nPhoneNumber,
                   p_dst_phone->pBoard->pszBoardName,
                   p_dst_phone->nDev, p_dst_phone->nPhoneCh));
#endif /* TAPI_VERSION4 */
         }

         /* More phones can call us */
         /* Status from where we got command. */
         /* Called phone has always only one connection with caller phone */
         p_dst_phone->nIntEvent_FXS = nAction;
         /* set internal event flag */
         pCtrl->bInternalEventGenerated = IFX_TRUE;
         p_conn = ABSTRACT_GetConnOfLocal(p_dst_phone, pPhone, pConn->nType);
         if (IFX_NULL == p_conn)
         {
            /* Could not get phone connection */
            return IFX_ERROR;
         }
         /* internal event is generated on phone */
         p_dst_phone->pConnFromLocal = p_conn;

         /* set connection data only on first messsage */
         if (IE_RINGING == nAction)
         {
            p_conn->nType = pConn->nType;
            p_conn->oConnPeer.nPhoneNum = pPhone->nPhoneNumber;
            p_conn->oConnPeer.oLocal.pPhone = pPhone;
            p_conn->nFeatureID = pConn->nFeatureID;
#ifdef TD_DECT
            /* for now set narrowband */
            p_conn->oConnPeer.oLocal.fDectWideband = IFX_FALSE;
#endif /* TD_DECT */
            if ((LOCAL_PCM_CALL == pConn->nType) ||
                (LOCAL_BOARD_PCM_CALL == pConn->nType))
            {
               /* Caller phone is sending action */
               p_conn->oConnPeer.oLocal.fSlave = IFX_TRUE;
               /* cross timeslots numbers */
               p_conn->oPCM.nTimeslot_RX = pConn->oPCM.nTimeslot_TX;
               p_conn->oPCM.nTimeslot_TX = pConn->oPCM.nTimeslot_RX;

               p_conn->nUsedCh = p_dst_phone->nPCM_Ch;
               /* phone fd and pcm fd are the same */
               p_conn->nUsedCh_FD = p_dst_phone->nPhoneCh_FD;
            } /* if ((LOCAL_PCM_CALL == pConn->nType) */
            else if (LOCAL_CALL == pConn->nType)
            {
#ifdef EASY3111
               if (TYPE_DUSLIC_XT == p_dst_phone->pBoard->nType)
               {
                  /* for conference used channel is already set */
                  if (TD_NOT_SET == p_dst_phone->oEasy3111Specific.nOnMainPCM_Ch)
                  {
                     /* for DuSLIC-xT this is set during resource allocation */
                     p_conn->nUsedCh = TD_NOT_SET;
                     p_conn->nUsedCh_FD = TD_NOT_SET;
                  }
               }
               else
#endif /* EASY3111 */
               {
                  p_conn->nUsedCh = p_dst_phone->nPhoneCh;
                  p_conn->nUsedCh_FD = p_dst_phone->nPhoneCh_FD;
               }
#ifdef TD_DECT
               TD_DECT_WideBandSet(pPhone, pConn, p_dst_phone, p_conn);
#endif /* TD_DECT */
            }
            else if (LOCAL_BOARD_CALL == pConn->nType)
            {
               p_conn->nUsedCh = p_dst_phone->nDataCh;
               p_conn->nUsedCh_FD = p_dst_phone->nDataCh_FD;
            } /* if ((LOCAL_PCM_CALL == pConn->nType) */
         } /* if (IE_RINGING == nAction) */
      } /* if ((LOCAL_CALL == pConn->nType) ... */
      else if (EXTERN_VOIP_CALL == pConn->nType)
      {
         /* message sender info */
         msg.nBoard_IDX = pPhone->pBoard->nBoard_IDX;
         msg.nSenderPhoneNum = pPhone->nPhoneNumber;
         msg.nSenderPort = TD_SOCK_PortGet(&pConn->oUsedAddr);
         memcpy(msg.MAC, pConn->oUsedMAC, ETH_ALEN);

         /* Receiver phone number */
         msg.nReceiverPhoneNum = pConn->oConnPeer.nPhoneNum;
         /* set peer address */
         TD_SOCK_AddrCpy(&to_addr, &pConn->oConnPeer.oRemote.oToAddr);
         TD_SOCK_PortSet(&to_addr, ADMIN_UDP_PORT);

         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("%s: Sending MSG using socket %d to [%s]:%d:\n"
                "%s  - call type %s\n"
                "%s  - sender UDP port %d\n"
                "%s  - sender MAC %02X:%02X:%02X:%02X:%02X:%02X\n"
                "%s  - sender phone number %d\n"
                "%s  - receiver phone number %d\n"
                "%s  - new internal event %s\n"
                "%s  - board idx %d\n"
                "%s  - feature id %d\n",
                pPhone->pCID_Msg->message[TD_CID_IDX_NAME].string.element,
                (int) pCtrl->nAdminSocket,
                TD_GetStringIP(&to_addr), TD_SOCK_PortGet(&to_addr),
                pPhone->pBoard->pIndentBoard,(CALL_FLAG_PCM == msg.fPCM)? "PCM" :
                (CALL_FLAG_VOIP == msg.fPCM) ? "EXTERN" : "UNKNOWN",
                pPhone->pBoard->pIndentBoard, msg.nSenderPort,
                pPhone->pBoard->pIndentBoard, msg.MAC[0],
                msg.MAC[1],
                msg.MAC[2],
                msg.MAC[3],
                msg.MAC[4],
                msg.MAC[5],
                pPhone->pBoard->pIndentBoard, msg.nSenderPhoneNum,
                pPhone->pBoard->pIndentBoard, msg.nReceiverPhoneNum,
                pPhone->pBoard->pIndentBoard, psActionName,
                pPhone->pBoard->pIndentBoard, (int) msg.nBoard_IDX,
                pPhone->pBoard->pIndentBoard, msg.nFeatureID));
         /* send test message */
         COM_MOD_SEND_PHONE_MAPPING(msg, pPhone, pConn, nSeqConnId);

#ifdef TD_FAX_MODEM
#if (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
         TD_T38_FAX_TEST_SET_ACTION(pCtrl, msg);
#endif /* (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
#endif /* TD_FAX_MODEM */

         ret = TD_OS_SocketSendTo(nUsedSock, (IFX_char_t *) &msg,
                                  sizeof(msg), &to_addr);
         if (sizeof(msg) != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, sending data (command %d byte(s)), %s. "
                   "(File: %s, line: %d)\n",
                   sizeof(msg), strerror(errno), __FILE__, __LINE__));
            return IFX_ERROR;
         }
      } /* else if (EXTERN_VOIP_CALL == pConn->nType) */
      else if (PCM_CALL == pConn->nType)
      {
         /* send board MAC address */
         memcpy(msg.MAC, pCtrl->oTapidemo_MAC_Addr, ETH_ALEN);
         /** \todo: check if remote side handles port in host format */
         msg.nSenderPort = TD_SOCK_PortGet(&pConn->oUsedAddr);
         msg.nBoard_IDX = pPhone->pBoard->nBoard_IDX;
         /* Get timeslot RX index */
         msg.nTimeslot_RX = pConn->oPCM.nTimeslot_RX;
         /* Get timeslot TX index */
         msg.nTimeslot_TX = pConn->oPCM.nTimeslot_TX;

         msg.nReceiverPhoneNum = pConn->oConnPeer.nPhoneNum;
         msg.nSenderPhoneNum = pPhone->nPhoneNumber;
         /* set peer address */
         TD_SOCK_AddrCpy(&to_addr, &pConn->oConnPeer.oPCM.oToAddr);
         TD_SOCK_PortSet(&to_addr, ADMIN_UDP_PORT);

#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("%s: Sending MSG using PCM ch %d, socket %d to [%s]:%d:\n"
                "%s  - call type %s\n"
                "%s  - sender phone number %d\n"
                "%s  - receiver phone number %d\n"
                "%s  - new internal event %s\n"
                "%s  - board idx %d\n"
                "%s  - timeslot RX %d\n"
                "%s  - timeslot TX %d\n",
                pPhone->pCID_Msg->message[TD_CID_IDX_NAME].string.element,
                (int) pConn->nUsedCh, (int) pCtrl->nAdminSocket,
                TD_GetStringIP(&to_addr), TD_SOCK_PortGet(&to_addr),
                pPhone->pBoard->pIndentBoard,
                (CALL_FLAG_PCM == msg.fPCM)? "PCM" :
                (CALL_FLAG_VOIP == msg.fPCM) ? "EXTERN" : "UNKNOWN",
                pPhone->pBoard->pIndentBoard, msg.nSenderPhoneNum,
                pPhone->pBoard->pIndentBoard, msg.nReceiverPhoneNum,
                pPhone->pBoard->pIndentBoard, psActionName,
                pPhone->pBoard->pIndentBoard,(int) msg.nBoard_IDX,
                pPhone->pBoard->pIndentBoard, msg.nTimeslot_RX,
                pPhone->pBoard->pIndentBoard, msg.nTimeslot_TX));
#endif /* TAPI_VERSION4 */

         ret = TD_OS_SocketSendTo(nUsedSock, (IFX_char_t *) &msg,
                                  sizeof(msg), &to_addr);

         if (sizeof(msg) != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, sending data (command %d byte), %s. "
                   "(File: %s, line: %d)\n",
                   sizeof(msg), strerror(errno), __FILE__, __LINE__));
            return IFX_ERROR;
         }
      } /* else if (PCM_CALL == pConn->nType) */
      else if (pConn->nType == FXO_CALL)
      {
         /* Nothing at the moment */
      } /* else if (pConn->nType == FXO_CALL) */
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Wrong call type, %d. (File: %s, line: %d)\n",
                (int) pConn->nType, __FILE__, __LINE__));
      }
   } /* if(nAction == IE_EXT_CALL_PROCEEDING||nAction == IE_EXT_CALL_WRONG_NUM)*/

   return IFX_SUCCESS;
} /* TAPIDEMO_SetAction() */

#ifdef TD_IPV6_SUPPORT
/**
   Initialize administration socket which will be used to manage phone
   connections.

   \return socket number or IFX_ERROR if error.
*/
IFX_return_t TAPIDEMO_InitAdminSocketIPv6(CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t nSockeFd;
   TD_OS_sockAddr_t *pAddrIPv6;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   if (TD_NOT_SET != pCtrl->oIPv6_Ctrl.nSocketFd)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Closing IPv6 socket.\n"));
      close(pCtrl->oIPv6_Ctrl.nSocketFd);
      pCtrl->oIPv6_Ctrl.nSocketFd = TD_NOT_SET;
   }
   /* socket must be set SOCK_DGRAM to make handling messages easier,
      if SOCK_DGRAM only one message will be read at a time with recvfrom() */
   if(IFX_SUCCESS != TD_SOCK_IPv6Create(TD_OS_SOC_TYPE_DGRAM, &nSockeFd))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Can't create IPv6 admin socket, %s.\n"
             "(File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return NO_SOCKET;
   }

   if (0 > nSockeFd)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, failed to open socket for IPv6 - %s. (File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pAddrIPv6 = &pCtrl->oIPv6_Ctrl.oAddrIPv6;
   TRACE_IPV6(("DEBUG: Init socket Fd %d, fam %d, [%s]:%d\n",
               nSockeFd, TD_SOCK_FamilyGet(pAddrIPv6),
               TD_GetStringIP(pAddrIPv6), ADMIN_UDP_PORT_IPV6));
   if (AF_INET6 != TD_SOCK_FamilyGet(pAddrIPv6))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Invalid address family %d. (File: %s, line: %d)\n",
             TD_SOCK_FamilyGet(pAddrIPv6), __FILE__, __LINE__));
      close(nSockeFd);
      return IFX_ERROR;
   }
   TD_SOCK_PortSet(pAddrIPv6, ADMIN_UDP_PORT);

   if (0 != (TD_OS_SocketBind(nSockeFd, pAddrIPv6)))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, failed to bind socket %d for IPv6 - %s. (File: %s, line: %d)\n",
             nSockeFd, strerror(errno), __FILE__, __LINE__));
      close(nSockeFd);
      return IFX_ERROR;
   }
#ifdef LINUX
   /* make the socket non blocking */
   fcntl(nSockeFd, F_SETFL, O_NONBLOCK);
#endif /* LINUX */
   pCtrl->oIPv6_Ctrl.nSocketFd = nSockeFd;

   return IFX_SUCCESS;
} /* */
#endif /* TD_IPV6_SUPPORT */

/**
   Initialize administration socket which will be used to manage phone
   connections.

   \return socket number or NO_SOCKET if error.
*/
TD_OS_socket_t TAPIDEMO_InitAdminSocket(IFX_void_t)
{
   TD_OS_socket_t socFd = NO_SOCKET;
   TD_OS_sockAddr_t my_addr;
   IFX_char_t ip_tos = 0x68;

   /* socket must be set SOCK_DGRAM to make handling messages easier,
      if SOCK_DGRAM only one message will be read at a time with recvfrom() */
   if(IFX_SUCCESS != TD_OS_SocketCreate(TD_OS_SOC_TYPE_DGRAM, &socFd))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Can't create admin socket, %s.\n(File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return NO_SOCKET;
   }

   /* Socket Option Level IP, IP TOS field shall be set */
   setsockopt (socFd, IPPROTO_IP, IP_TOS, &ip_tos, sizeof(ip_tos));
#if defined(EASY336) && defined(TD_IPV6_SUPPORT)
/* #warning workaround:
   added reuse for SVIP with IPv6 support, without it it didn't work */
   if (1)
   {
      IFX_int32_t tr = 1;
      IFX_int32_t sock_ret;

      sock_ret = setsockopt(socFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(tr));
   }
#endif /* defined(EASY336) && defined(TD_IPV6_SUPPORT) */

   memset(&my_addr, 0, sizeof(my_addr));

   TD_SOCK_FamilySet(&my_addr, AF_INET);
   TD_SOCK_PortSet(&my_addr, ADMIN_UDP_PORT);

   if (0 != TD_SOCK_AddrIPv4Get(&my_addr))
   {
      TD_SOCK_AddrIPv4Set(&my_addr,
         TD_SOCK_AddrIPv4Get(&oCtrlStatus.pProgramArg->oMy_IP_Addr));
   }
   else
   {
      TD_SOCK_AddrIPv4Set(&my_addr, INADDR_ANY);
   }

   if(IFX_SUCCESS != TD_OS_SocketBind(socFd, &my_addr))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, Can't bind admin socket to port, %s.\n(File: %s, line: %d)\n",
            strerror(errno), __FILE__, __LINE__));
      TD_OS_SocketClose(socFd);
      return NO_SOCKET;
   }

#ifdef LINUX
   /* make the socket non blocking */
   fcntl(socFd, F_SETFL, O_NONBLOCK);
#endif /* LINUX */

   return socFd;
} /* TAPIDEMO_InitAdminSocket() */

/**
   Handle data from ADMIN socket.

   \param pCtrl   - pointer to status control structure
   \param nSockFd - socket file descriptor

   \return IFX_SUCCESS message received and handled otherwise IFX_ERROR
*/
IFX_return_t TAPIDEMO_HandleCallSeq(CTRL_STATUS_t* pCtrl,
                                    TD_OS_socket_t nSockFd)
{
   static IFX_char_t buf[TD_MAX_COMM_MSG_SIZE];
   IFX_int32_t ret = 0;
   IFX_int32_t event = 0;
   IFX_uint8_t from_MAC[ETH_ALEN];
   IFX_int32_t from_port = 0;
   IFX_int32_t board_idx = 0;
   TD_COMM_MSG_t msg = {0};
   PHONE_t* pPhone = IFX_NULL;
   CONNECTION_t* conn = IFX_NULL;
   IFX_boolean_t bFreeConn = IFX_FALSE;
   IFX_int32_t feature_id = 0;
   /* message receiver and sender phone numbers */
   IFX_int32_t nReciverPhoneNum;
   IFX_int32_t nSenderPhoneNum;
   IFX_char_t* psEventName = IFX_NULL;
#ifdef EASY336
   SVIP_libStatus_t SVIP_libRet;
#endif
   TD_OS_sockAddr_t addr_from;
   IFX_boolean_t bIPv6 = IFX_FALSE;
   IFX_uint32_t nRecvSeqConnId = TD_CONN_ID_MSG;

   /* check input arguments */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* Get data from socket */
   ret = TD_OS_SocketRecvFrom(nSockFd, buf, sizeof(buf), &addr_from);

   if (0 > ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
            ("Err, No data read from ADMIN socket, %s. (File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else if (0 == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
         ("No data read from ADMIN socket.\n"));
      return IFX_SUCCESS;
   }

   /* set and check message structure with received data */
   if (IFX_SUCCESS != Common_CommMsgSet(&msg, buf, ret))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
            ("Err, Invalid data on ADMIN socket %d. (File: %s, line: %d)\n",
             (int) nSockFd, __FILE__, __LINE__));
      return IFX_ERROR;

   }

   /* get conn id if set in message */
   if (TD_COMM_MSG_VER_CONN_ID <= msg.nMsgVersion &&
       TD_COMM_MSG_DATA_1_CONN_ID == msg.oData2.oSeqConnId.nType)
   {
      nRecvSeqConnId = msg.oData2.oSeqConnId.nSeqConnID;
   }

   if (AF_INET6 == TD_SOCK_FamilyGet(&addr_from))
   {
      bIPv6 = IFX_TRUE;
   }
   else if (AF_INET != TD_SOCK_FamilyGet(&addr_from))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
            ("Err, Invalid address family for received family %d. "
             "(File: %s, line: %d)\n",
             TD_SOCK_FamilyGet(&addr_from), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* change peer address if needed for ITM */
   COM_ITM_SET_IP(&addr_from);

   /* event to send to phone */
   event = msg.nAction;

   /* set pointer string with action name */
   psEventName = Common_Enum2Name(event, TD_rgIE_EventsName);

   /* check if name was set, if name was not set, then invalid value
      of TD_rgIE_EventsName is set */
   TD_PTR_CHECK(psEventName, IFX_ERROR);

   /* check if workaround */
   if(event == IE_EXT_CALL_WORKAROUND_EASY508xx)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
            ("INFO: ignoring message with type %s.\n",
             Common_Enum2Name(event, TD_rgIE_EventsName)));
      /* It is workaround to avoid problem with first external/PCM
         connection with EASY508xx. TAPIDEMO ignores this type of messages */
      return IFX_SUCCESS;
   }

   /* Print message data */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
         ("Received MSG from [%s]:%d action %s:\n",
          TD_GetStringIP(&addr_from), TD_SOCK_PortGet(&addr_from),
          psEventName));

#ifdef TD_IPV6_SUPPORT
   if ((event == IE_PHONEBOOK_UPDATE) ||
       (event == IE_PHONEBOOK_UPDATE_RESPONSE))
   {
      if ( IFX_SUCCESS != Common_PhonebookHandleEvents(pCtrl, &addr_from,
                             event, msg.oData1.aReserved))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
               ("Err, Common_PhonebookHandleEvents() failed."
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }
      return IFX_SUCCESS;
   }
#endif /* TD_IPV6_SUPPORT */

   /* Print message data */
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
         (" - call type %s\n",
          (CALL_FLAG_PCM == msg.fPCM)? "PCM" : (CALL_FLAG_VOIP == msg.fPCM) ?
          "EXTERN" : "UNKNOWN"));

   /* messages currently are received only for PCM_CALL and EXTERN_VOIP_CALL,
      UDP port is only sent during EXTERN_VOIP_CALL, but if some error ocurres
      e.g. wrong call type, then print this data also. */
   if (CALL_FLAG_PCM != msg.fPCM)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
            (" - sender UDP port %d\n", msg.nSenderPort));

   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
         (" - sender MAC %02X:%02X:%02X:%02X:%02X:%02X\n"
          " - message sender phone number %d\n"
          " - message receiver phone number %d\n"
          " - board idx %d\n"
          " - feature id %d\n",
          msg.MAC[0],
          msg.MAC[1],
          msg.MAC[2],
          msg.MAC[3],
          msg.MAC[4],
          msg.MAC[5],
          msg.nSenderPhoneNum, msg.nReceiverPhoneNum,
          msg.nBoard_IDX,
          msg.nFeatureID));
   /* messages currently are received only for PCM_CALL and EXTERN_VOIP_CALL,
      timeslots are only sent during PCM_CALL, but if some error ocurres
      e.g. wrong call type, then print this data also. */
   if (CALL_FLAG_VOIP != msg.fPCM)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
            (" - timeslot RX %d\n"
             " - timeslot TX %d\n",
             msg.nTimeslot_RX, msg.nTimeslot_TX));
   }

   /* get feature id */
   feature_id = msg.nFeatureID;

   /* Caller stuff */
   /* get board index (used to get board from pCtrl->rgoBoards[]) */
   board_idx = msg.nBoard_IDX;
   /* get message sending phone number */
   nSenderPhoneNum = msg.nSenderPhoneNum;
   memcpy(from_MAC, msg.MAC, ETH_ALEN);
   from_port = msg.nSenderPort;

   /* Called stuff */
   /* get message receiving phone number */
   nReciverPhoneNum = msg.nReceiverPhoneNum;

   /* Check if phone number is avaible */
   pPhone = ABSTRACT_GetPHONE_OfCalledNum(pCtrl, msg.nReceiverPhoneNum,
                                          nRecvSeqConnId);

   /* caller is dialing number but, didn't finish dialing full number */
   if (IFX_NULL == pPhone)
   {
      /* If receive call setup messsage, send answer */
      if (IE_EXT_CALL_SETUP == event)
      {
         /* phoney phone and connection to allow usage of TAPIDEMO_SetAction() */
         CONNECTION_t oConnTmp;
         PHONE_t oPhoneTmp;
         /* Received called number is incomplete or incorrect */
         /* Allocate temporary connection structure and send msg to caller */
         bFreeConn = IFX_TRUE;
         conn = &oConnTmp;
         memset(conn, 1, sizeof(CONNECTION_t));

         /* Allocate temporary phone structure and send msg to caller */
         pPhone = &oPhoneTmp;
         memset(pPhone, 1, sizeof(PHONE_t));

         pPhone->nSeqConnId = nRecvSeqConnId;
         if (CALL_FLAG_VOIP == msg.fPCM)
         {
            conn->nType = EXTERN_VOIP_CALL;
            conn->bIPv6_Call = IFX_FALSE;
         }

         /* PCM calls are also supported between boards with different
            count of digits in dial number */

         if (CALL_FLAG_PCM == msg.fPCM)
         {
            conn->nType = PCM_CALL;
         }
         memcpy(conn->oConnPeer.oRemote.MAC, from_MAC, ETH_ALEN);

         TD_SOCK_AddrCpy(&conn->oConnPeer.oRemote.oToAddr, &addr_from);
#ifdef TD_IPV6_SUPPORT
         if (IFX_TRUE == bIPv6)
         {
            conn->bIPv6_Call = IFX_TRUE;

            TD_SOCK_AddrCpy(&conn->oUsedAddr, &pCtrl->oIPv6_Ctrl.oAddrIPv6);
         }
         else
#endif /* TD_IPV6_SUPPORT */
         {
            TD_SOCK_AddrCpy(&conn->oUsedAddr, &pCtrl->oTapidemo_IP_Addr);
         }
         conn->oConnPeer.nPhoneNum = msg.nSenderPhoneNum;

#ifndef EASY336
         memcpy(conn->oUsedMAC, pCtrl->oTapidemo_MAC_Addr, ETH_ALEN);
#else /* EASY336 */
#if defined(TD_IPV6_SUPPORT) && defined(CONFIG_LTQ_SVIP_NAT_PT)
         if (AF_INET6 == TD_SOCK_FamilyGet(&addr_from))
         {
            SVIP_libRet = SVIP_libMAC_AddressForVoIPv6Get(TD_SOCK_GetAddrIn(&addr_from),
                                                          conn->oUsedMAC);
         }
         else
#endif /* defined(TD_IPV6_SUPPORT) && defined(CONFIG_LTQ_SVIP_NAT_PT) */
         {
            SVIP_libRet = SVIP_libMAC_AddressForVoIPGet(TD_SOCK_AddrIPv4Get(&addr_from),
                                                        conn->oUsedMAC);
         }

         if (SVIP_libRet != SVIP_libStatusOk)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
               ("Err, SVIP_libMAC_AddressForVoIPGet returned error %d "
                "(File: %s, line: %d)\n",
                SVIP_libRet, __FILE__, __LINE__));
            return IFX_ERROR;
         }
#endif /* EASY336 */

         /* We don't know phone number, so we don't know below values */
         conn->nUsedSocket = NO_SOCKET;
         conn->nUsedCh_FD = NO_CHANNEL_FD;
         conn->nUsedCh = NO_CHANNEL;
         TD_SOCK_PortSet(&conn->oUsedAddr, ADMIN_UDP_PORT);
         conn->nFeatureID = 0;

         /* We need to send correct called phone number, it's needed in
         function ABSTRACT_GetConnOfPhone for second phone */
         pPhone->nPhoneNumber = nReciverPhoneNum;

#ifdef TD_USE_EXT_CALL_WORKAROUND_EASY508XX
         /* It is a workaround, to avoid problem with first packet being dropped
            for first external/PCM connection to EASY508xx boards */
         if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, conn,
                             IE_EXT_CALL_WORKAROUND_EASY508xx, nRecvSeqConnId))
         {
            return IFX_ERROR;
         }
#endif /* TD_USE_EXT_CALL_WORKAROUND_EASY508XX */
         if (MIN_PHONE_NUM <= msg.nReceiverPhoneNum)
         {
            /* Received phone number is not correct, if another digit will be dialed, then dialed phone number ,
            inform about it phone on second board */
            if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, conn,
                                IE_EXT_CALL_WRONG_NUM, nRecvSeqConnId))
            {
               return IFX_ERROR;
            }
         }
         else
         {
            /* Received phone number is not completed,
            so inform about it second board */
            if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, conn,
                                IE_EXT_CALL_PROCEEDING, nRecvSeqConnId))
            {
               return IFX_ERROR;
            }
         } /* if (pCtrl->nSumPhone < msg.nCalledPhoneNum) */
         return IFX_SUCCESS;
      }
      else
      {
         /* For rest of the cases print error */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
               ("Err, failed to get phone. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      } /* if (IE_EXT_CALL_SETUP == event) */
   } /* if (IFX_NULL == phone) */
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
            ("%s: Received event %s on ADMIN socket\n",
             pPhone->pCID_Msg->message[TD_CID_IDX_NAME].string.element,
             psEventName));

      if (CALL_FLAG_PCM == msg.fPCM)
      {
         /* check event type */
         if (IE_RINGING == event)
         {
            if (S_READY == pPhone->rgStateMachine[FXS_SM].nState &&
                TD_CONN_ID_MSG != nRecvSeqConnId)
            {
               ABSTRACT_SeqConnID_Set(pPhone, IFX_NULL, nRecvSeqConnId);
            }
         }
         /** \todo: this function should be reviewed */
         conn = ABSTRACT_GetConnOfPhone(pPhone, PCM_CALL,
                                        nReciverPhoneNum, nSenderPhoneNum,
                                        &addr_from, &bFreeConn, nRecvSeqConnId);

         if (conn == IFX_NULL)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
                  ("Err, get connection. (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
         } /* if (conn == IFX_NULL) */

         if (bFreeConn)
         {
            conn->nType = PCM_CALL;
            conn->bIPv6_Call = IFX_FALSE;
            TD_SOCK_AddrCpy(&conn->oConnPeer.oPCM.oToAddr, &addr_from);
            TD_SOCK_PortSet(&conn->oConnPeer.oPCM.oToAddr, from_port);
            conn->oConnPeer.nPhoneNum = msg.nSenderPhoneNum;
#ifdef TD_IPV6_SUPPORT
            if (IFX_TRUE == bIPv6)
            {
               conn->bIPv6_Call = IFX_TRUE;

               TD_SOCK_AddrCpy(&conn->oUsedAddr, &pCtrl->oIPv6_Ctrl.oAddrIPv6);
            }
            else
#endif /* TD_IPV6_SUPPORT */
            {
               TD_SOCK_AddrCpy(&conn->oUsedAddr, &pCtrl->oTapidemo_IP_Addr);
            }

            if (pPhone->pBoard->nMaxPCM_Ch > 0)
            {
#ifdef EASY3111
               /* for EASY 3111 (with vmmc on main board) PCM channel is mapped
               and allocated during state transition (READY->RINGING) */
               if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
#endif /* EASY3111 */
               {
                  conn->nUsedCh_FD = PCM_GetFD_OfCh(pPhone->nPCM_Ch, pPhone->pBoard);
                  conn->nUsedCh = pPhone->nPCM_Ch;
               }
            }
            /* for PCM channel this socket isn't used */
            conn->nUsedSocket = pPhone->nSocket;
            conn->nFeatureID = feature_id;
         } /* if (free_conn) */

         if (pPhone->pBoard->nMaxPCM_Ch <= 0)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                     ("Warning, Phone No %d: %s: PCM not supported, "
                      "incoming call rejected.\n",
                      pPhone->nPhoneNumber, pPhone->pBoard->pszBoardName));
            if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, conn,
                                                IE_BUSY, nRecvSeqConnId))
            {
               return IFX_ERROR;
            }
            return IFX_SUCCESS;
         }

         /* Set new event for phone */
         pPhone->nIntEvent_FXS = event;

         if (!pCtrl->pProgramArg->oArgFlags.nPCM_Master)
         {
            if (IE_RINGING == event || IE_RINGBACK == event)
            {
               /* Set PCM timeslots */
               conn->oPCM.nTimeslot_RX = msg.nTimeslot_TX;
               conn->oPCM.nTimeslot_TX = msg.nTimeslot_RX;
            }
         }
         /* check event type */
         if (IE_RINGING == event)
         {
            /* check if CID support is on */
            if (pCtrl->pProgramArg->oArgFlags.nCID)
            {
               /* get CID data */
               if (IFX_SUCCESS != CID_FromPeerMsgGet(pPhone, &msg))
               {

                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
                        ("Err, CID_FromPeerMsgGet failed. "
                         "(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
            } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */
         } /* if (IE_RINGING == event) */

#ifndef TAPI_VERSION4
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
               ("%s: PCM connection on channel %d:\n"
                "%s  - used socket: %d\n"
                "%s  - timeslot RX %d TX %d\n"
                "%s  - to: [%s]:%d\n",
                pPhone->pCID_Msg->message[TD_CID_IDX_NAME].string.element,
                (int) conn->nUsedCh,
                pPhone->pBoard->pIndentBoard,
                (int) conn->nUsedSocket,
                pPhone->pBoard->pIndentBoard,
                conn->oPCM.nTimeslot_RX, conn->oPCM.nTimeslot_TX,
                pPhone->pBoard->pIndentBoard,
                TD_GetStringIP(&conn->oUsedAddr),
                TD_SOCK_PortGet(&conn->oUsedAddr)));
#endif /* TAPI_VERSION4 */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
               ("%s  - from [%s]:%d\n",
                pPhone->pBoard->pIndentBoard,
                TD_GetStringIP(&conn->oConnPeer.oPCM.oToAddr),
                TD_SOCK_PortGet(&conn->oConnPeer.oPCM.oToAddr)));

         if (IE_EXT_CALL_SETUP == event)
         {
#ifdef TD_USE_EXT_CALL_WORKAROUND_EASY508XX
            /* It is a workaround, to avoid problem with first packet being
               dropped for first external/PCM connection to EASY508xx boards */
            if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, conn,
                                IE_EXT_CALL_WORKAROUND_EASY508xx, nRecvSeqConnId))
            {
               return IFX_ERROR;
            }
#endif /* TD_USE_EXT_CALL_WORKAROUND_EASY508XX */
            /* Received phone number is correct,
               so inform about it second phone */
            if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, conn,
                                IE_EXT_CALL_ESTABLISHED, nRecvSeqConnId))
            {
               return IFX_ERROR;
            }
            return IFX_SUCCESS;
         } /* if (IE_EXT_CALL_SETUP == event) */
         /* save port number, caller sends it with IE_RINGING,
            called phone sends it with IE_ACTIVE action */
         else if (IE_ACTIVE == event || IE_RINGING == event)
         {
            TD_SOCK_PortSet(&conn->oConnPeer.oRemote.oToAddr, from_port);
         }
         memcpy(conn->oConnPeer.oRemote.MAC, from_MAC, ETH_ALEN);
         conn->nFeatureID = feature_id;

         /* Set new event for phone */
         pPhone->nIntEvent_FXS = event;

         if ((pPhone->rgStateMachine[CONFIG_SM].nState != S_READY) ||
             (event == IE_CONFIG_PEER))
         {
            pPhone->nIntEvent_Config = event;
         }
      } /* if (CALL_FLAG_PCM == msg.fPCM) */
      else if (CALL_FLAG_VOIP == msg.fPCM)
      {
         /* check event type */
         if (IE_RINGING == event)
         {
            if (S_READY == pPhone->rgStateMachine[FXS_SM].nState &&
                TD_CONN_ID_MSG != nRecvSeqConnId)
            {
               ABSTRACT_SeqConnID_Set(pPhone, IFX_NULL, nRecvSeqConnId);
            }
         }
         /* note: this function should be reviewed */
         conn = ABSTRACT_GetConnOfPhone(pPhone, EXTERN_VOIP_CALL,
                                        nReciverPhoneNum, nSenderPhoneNum,
                                        &addr_from, &bFreeConn, nRecvSeqConnId);
         if (IFX_NULL == conn)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
                  ("Err, get connection. (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
         } /* if (IFX_NULL == conn) */

         if (bFreeConn)
         {
            conn->nType = EXTERN_VOIP_CALL;
            conn->bIPv6_Call = IFX_FALSE;

            memcpy(conn->oConnPeer.oRemote.MAC, from_MAC, ETH_ALEN);
            TD_SOCK_AddrCpy(&conn->oConnPeer.oRemote.oToAddr, &addr_from);

            conn->oConnPeer.nPhoneNum = msg.nSenderPhoneNum;
#ifdef TD_DECT
            /* if DECT phone is available then add data channel */
            if (PHONE_TYPE_DECT == pPhone->nType &&
                TD_NOT_SET == pPhone->nDataCh &&
                S_READY == pPhone->rgStateMachine[FXS_SM].nState &&
                IE_RINGING == event)
            {
               /* this is unlikely to happend but if next message is not received
                  then this data channel will not be freed untill this phone
                  goes off-hook and then on-hook */
               if (IFX_SUCCESS != TD_DECT_DataChannelAdd(pPhone))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
                        ("Err, TD_DECT_DataChannelAdd failed. (File: %s, line: %d)\n",
                         __FILE__, __LINE__));
                  return IFX_ERROR;
               }
            } /* if DECT phone */
#endif /* TD_DECT */
#ifndef EASY336
            memcpy(conn->oUsedMAC, pCtrl->oTapidemo_MAC_Addr, ETH_ALEN);
#else
#if defined(TD_IPV6_SUPPORT) && defined(CONFIG_LTQ_SVIP_NAT_PT)
            if (AF_INET6 == TD_SOCK_FamilyGet(&addr_from))
            {
               SVIP_libRet = SVIP_libMAC_AddressForVoIPv6Get(TD_SOCK_GetAddrIn(&addr_from),
                                                             conn->oUsedMAC);
            }
            else
#endif /* defined(TD_IPV6_SUPPORT) && defined(CONFIG_LTQ_SVIP_NAT_PT) */
            {
               SVIP_libRet = SVIP_libMAC_AddressForVoIPGet(TD_SOCK_AddrIPv4Get(&addr_from),
                                                           conn->oUsedMAC);
            }

            if (SVIP_libRet != SVIP_libStatusOk)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
                  ("Err, SVIP_libMAC_AddressForVoIPGet returned error %d "
                   "(File: %s, line: %d)\n",
                   SVIP_libRet, __FILE__, __LINE__));
               return IFX_ERROR;
            }
#endif
#ifdef TD_IPV6_SUPPORT
            if (IFX_TRUE == bIPv6)
            {
               conn->bIPv6_Call = IFX_TRUE;
               TD_SOCK_AddrCpy(&conn->oUsedAddr, &pCtrl->oIPv6_Ctrl.oAddrIPv6);
            }
            else
#endif /* TD_IPV6_SUPPORT */
            {
               TD_SOCK_AddrCpy(&conn->oUsedAddr, &pCtrl->oTapidemo_IP_Addr);
            }
#ifdef EASY3111
            /* for EASY 3111 data channel is acquired during state transition */
            if (TYPE_DUSLIC_XT != pPhone->pBoard->nType)
#endif /* EASY3111 */
            {
#ifdef TD_DECT
               if (TD_NOT_SET != pPhone->nDataCh)
#endif /* TD_DECT */
#ifdef TAPI_VERSION4
               if (!pPhone->pBoard->fSingleFD)
#endif
               {
                  conn->nUsedCh_FD = VOIP_GetFD_OfCh(pPhone->nDataCh,
                                                     pPhone->pBoard);
               }
               conn->nUsedCh = pPhone->nDataCh;
#ifdef TAPI_VERSION4
               if (pPhone->pBoard->fUseSockets)
#endif /* TAPI_VERSION4 */
               {
                  /* get socket */
                  conn->nUsedSocket = VOIP_GetSocket(conn->nUsedCh,
                                         pPhone->pBoard, conn->bIPv6_Call);
                  if (!pCtrl->pProgramArg->oArgFlags.nQos ||
                      pCtrl->pProgramArg->oArgFlags.nQosSocketStart)
                  {
                     TD_SOCK_PortSet(&conn->oUsedAddr,
                        VOIP_GetSocketPort(conn->nUsedSocket, pCtrl));
                  }
               }
            } /* if (TYPE_DUSLIC_XT != phone->pBoard->nType) */
         } /* if (free_conn) */

#ifdef TD_FAX_MODEM
#if (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
         if (IE_RINGBACK == event || IE_RINGING == event)
         {
            TD_T38_FAX_TEST_HANDLE_CALL_SEQUENCE(pCtrl, msg, pPhone);
         }
#endif /* (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
#endif /* TD_FAX_MODEM */
         if (IE_EXT_CALL_SETUP == event)
         {
#ifdef TD_USE_EXT_CALL_WORKAROUND_EASY508XX
            /* It is a workaround, to avoid problem with first packet being
               dropped for first external/PCM connection to EASY508xx boards */
            if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, conn,
                                IE_EXT_CALL_WORKAROUND_EASY508xx, nRecvSeqConnId))
            {
               return IFX_ERROR;
            }
#endif /* TD_USE_EXT_CALL_WORKAROUND_EASY508XX */
            /* Received phone number is correct,
               so inform about it second phone */
            if (IFX_ERROR == TAPIDEMO_SetAction(pCtrl, pPhone, conn,
                                IE_EXT_CALL_ESTABLISHED, nRecvSeqConnId))
            {
               return IFX_ERROR;
            }
            return IFX_SUCCESS;
         } /* if (IE_EXT_CALL_SETUP == event) */
         /* save port number, caller sends it with IE_RINGING,
            called phone sends it with IE_ACTIVE action */
         else if (IE_ACTIVE == event || IE_RINGING == event)
         {
            TD_SOCK_PortSet(&conn->oConnPeer.oRemote.oToAddr, from_port);
         }
         /* check event type */
         if (IE_RINGING == event)
         {
            /* check if CID support is on */
            if (pCtrl->pProgramArg->oArgFlags.nCID)
            {
               /* get CID data */
               if (IFX_SUCCESS != CID_FromPeerMsgGet(pPhone, &msg))
               {

                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
                        ("Err, CID_FromPeerMsgGet failed. "
                         "(File: %s, line: %d)\n",
                         __FILE__, __LINE__));
               }
            } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */
         } /* if (IE_RINGING == event) */

         memcpy(conn->oConnPeer.oRemote.MAC, from_MAC, ETH_ALEN);
         conn->nFeatureID = feature_id;

         /* Set new event for phone */
         pPhone->nIntEvent_FXS = event;

         if ((pPhone->rgStateMachine[CONFIG_SM].nState != S_READY) ||
             (event == IE_CONFIG_PEER))
         {
            pPhone->nIntEvent_Config = event;
         }

         if (IE_ACTIVE == event)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
                  ("%s: VOIP connection on channel %d:\n",
                   pPhone->pCID_Msg->message[TD_CID_IDX_NAME].string.element,
                   pPhone->nPhoneCh));
#ifndef TAPI_VERSION4
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
                  ("%s - used socket: %d\n",
                   pPhone->pBoard->pIndentBoard,
                   conn->nUsedSocket));
#else /* TAPI_VERSION4 */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
                  ("%s - device: %d\n",
                   pPhone->pBoard->pIndentBoard,
                   pPhone->nDev));
#endif /* TAPI_VERSION4 */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
                  ("%s - to: [%s]:%d\n",
                   pPhone->pBoard->pIndentBoard,
                   TD_GetStringIP(&conn->oUsedAddr),
                   TD_SOCK_PortGet(&conn->oUsedAddr)));
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nRecvSeqConnId,
                  ("%s - from: [%s]:%d\n",
                   pPhone->pBoard->pIndentBoard,
                   TD_GetStringIP(&conn->oConnPeer.oRemote.oToAddr),
                   TD_SOCK_PortGet(&conn->oConnPeer.oRemote.oToAddr)));
         }
      } /* if (CALL_FLAG_PCM == msg.fPCM) */
      else
      {
         /* If we came here then wrong message was received */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nRecvSeqConnId,
            ("Err, Received Message with UNKNOWN call flag. "
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      }

      /* Action from other phone received, handle it. */
      while ( pPhone->nIntEvent_Config != IE_NONE )
      {
         ST_HandleState_Config(pCtrl, pPhone, conn);
      }

      while ( pPhone->nIntEvent_FXS!= IE_NONE)
      {
         ST_HandleState_FXS(pCtrl, pPhone, conn);
      }

      return IFX_SUCCESS;
   } /* if (IFX_NULL == phone) */

} /* ADMIN_HandleCallSeq() */

#ifdef LINUX
/**
   Initialize socket for syslog.

   \return socket number or NO_SOCKET if error.
*/
IFX_int32_t TAPIDEMO_InitSyslogSocket(IFX_void_t)
{
   TD_OS_socket_t socFd = NO_SOCKET;
   TD_OS_sockAddr_t my_addr;

   if(IFX_SUCCESS != TD_OS_SocketCreate(TD_OS_SOC_TYPE_DGRAM, &socFd))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("InitSyslogSocket: Can't create UDP socket, %s. "
             "(File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return NO_SOCKET;
   }

   memset(&my_addr, 0, sizeof(my_addr));
   TD_SOCK_FamilySet(&my_addr, AF_INET);
   TD_SOCK_AddrIPv4Set(&my_addr, INADDR_ANY);
   TD_SOCK_PortSet(&my_addr, 0);

   if(IFX_SUCCESS != TD_OS_SocketBind(socFd, &my_addr))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("InitSyslogSocket: Can't bind to port, %s. (File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      TD_OS_SocketClose(socFd);
      return NO_SOCKET;
   }

   return socFd;
} /* TAPIDEMO_InitSyslogSocket() */

/**
   Send data to remote syslog

   \param pBuffer - pointer to message
   \param nSyslogSocket - socket used for communication with remote Syslog

   \return  IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
IFX_return_t TAPIDEMO_SendLogRemote(IFX_char_t* pBuffer,
                                    TD_OS_socket_t nSyslogSocket)
{
   IFX_int32_t ret = 0;
   IFX_int32_t nLength;

   /* It is used to build external syslog message */
   IFX_char_t pExtSyslogMsg[MAX_EXT_SYSLOG_MSG_LEN] = {0};

   TD_OS_sockAddr_t server_addr;

   TD_PTR_CHECK(pBuffer, IFX_ERROR);

   memset(&server_addr, 0, sizeof(server_addr));
   TD_SOCK_FamilySet(&server_addr, AF_INET);
   TD_SOCK_AddrIPv4Set(&server_addr,
                       TD_SOCK_AddrIPv4Get(&oTraceRedirection.oSyslog_IP_Addr));
   TD_SOCK_PortSet(&server_addr,
                   TD_SOCK_PortGet(&oTraceRedirection.oSyslog_IP_Addr));

   /* Correct syslog message has The Priority part (PRI) at the beginning.*/
   /* The PRI part MUST have three, four, or five characters and will be */
   /* bound with angle brackets as the first and last characters. */
   /* The code set used in this part MUST be seven-bit ASCII in an eight-bit field. */
   /* The number contained within these angle brackets is known as */
   /* the Priority value and represents both the Facility and */
   /* Severity. The Priority value is calculated by first multiplying the Facility */
   /* number by 8 and then adding the numerical value of the Severity. */
   /* Source: RFC3164 */

   /* In TAPIDEMO, Facility is set on 1 (user-level messages) and */
   /* Severity is set on 7 (Debug: debug-level messages): 1*8+7 = 15 */
   /* We need buffer with lenght + PRI' length + '\n' */

   /* Add PRI part */
   strcpy(pExtSyslogMsg, "<15>");
   /* Add message part */
   strcat(pExtSyslogMsg, pBuffer);
   nLength = strlen(pExtSyslogMsg);

   ret = TD_OS_SocketSendTo(nSyslogSocket, pExtSyslogMsg, nLength, &server_addr);
   if (ret != nLength)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, calling sendto(), %s, socket %d"
             " (File: %s, line: %d)\n",
             strerror(errno), (IFX_int32_t) nSyslogSocket,
             __FILE__, __LINE__));

      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

/**
   Split data on smaller packets and send them.
   Data are splitted according to end of line. Every line is separate packet.

   \param pBuffer - pointer to message

   \return  IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
IFX_return_t TAPIDEMO_SplitAndSendLog(IFX_char_t* pBuffer)
{
   IFX_return_t ret;
   IFX_char_t* pPresentCh;
   IFX_char_t* pPreviusCh;
   IFX_char_t* pTmpStr;

   /* It is used to keep split data */
   IFX_char_t pTmpBuffer[MAX_LOG_LEN] = {0};

   TD_PTR_CHECK(pBuffer, IFX_ERROR);
   TD_PARAMETER_CHECK((MAX_LOG_LEN <= strlen(pBuffer)),
                      strlen(pBuffer), IFX_ERROR);

   /* We need to check if the last char is EOL.
      Otherwise, some data can be missed */
   if (pBuffer[strlen(pBuffer)] != TD_EOL)
   {
      /* Get last EOL  in string */
      pTmpStr = strrchr(pBuffer, TD_EOL);
      /* Check if find any EOL */
      if (IFX_NULL != pTmpStr)
      {
         /* Check if found EOL is the last char */
         if ( strlen(pTmpStr) > 1 )
         {
            /* EOL is not the least char, so add EOL at the end of the string */
            pBuffer[strlen(pBuffer)] = TD_EOL;
         }
      }
      else
      {
         /* Can't find any EOL. So add one. */
         pBuffer[strlen(pBuffer)] = TD_EOL;
      }
   }

   /* Locate first occurrence of character '\n' */
   pPresentCh = strchr(pBuffer, TD_EOL);
   pPreviusCh = pBuffer;

   while (pPresentCh != NULL)
   {
      if (pPresentCh - pPreviusCh + 1 < MAX_LOG_LEN)
      {
         strncpy(pTmpBuffer, pPreviusCh, pPresentCh - pPreviusCh + 1);
         pTmpBuffer[pPresentCh - pPreviusCh + 1] = '\0';

         if (oCtrlStatus.pProgramArg->nTraceRedirection ==
             TRACE_REDIRECTION_SYSLOG_LOCAL)
         {
            /* Redirect log to local syslog */
            syslog(LOG_DEBUG, pTmpBuffer);
         } /* TRACE_REDIRECTION_SYSLOG_LOCAL */
         else if (oCtrlStatus.pProgramArg->nTraceRedirection ==
                  TRACE_REDIRECTION_SYSLOG_REMOTE)
         {
            /* Redirect log to remote syslog */
            ret = TAPIDEMO_SendLogRemote(pTmpBuffer,
                                         oTraceRedirection.nSyslogSocket);
            if (ret == IFX_ERROR)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                     ("Err, sending log to remote syslog. "
                      "(File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               return IFX_ERROR;
            }
         } /* TRACE_REDIRECTION_SYSLOG_REMOTE */
         else if (oCtrlStatus.pProgramArg->nTraceRedirection ==
                  TRACE_REDIRECTION_FILE)
         {
            ret = TD_OS_FWrite(pTmpBuffer, strlen(pTmpBuffer), 1,
                               oTraceRedirection.pFileLogFD);
            if (0 > ret)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                     ("Err, writing log to file, %s. (File: %s, line: %d)\n",
                      strerror(errno), __FILE__, __LINE__));
               return IFX_ERROR;
            }
         } /* TRACE_REDIRECTION_FILE */
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                  ("Err, Unknow type of log redirection (File: %s, line: %d)\n",
                   __FILE__, __LINE__));
            return IFX_ERROR;
         }
         /* Clean tmp buffer */
         memset(pTmpBuffer, 0, MAX_LOG_LEN);
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
               ("Err, Message too long. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
      }

      pPreviusCh = pPresentCh+1;
      /* Locate next occurrence of character '\n' */
      pPresentCh = strchr(pPreviusCh, TD_EOL);
   } /* while */

   return IFX_SUCCESS;
}
/**
   Prepare logs' redirection, e.g. set up syslog, open file.

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
IFX_return_t TAPIDEMO_PreapareLogRedirection(IFX_void_t)
{
#ifdef USE_FILESYSTEM
   IFX_char_t full_filename[MAX_FULL_FILE_LEN] = {0};
#endif

   if (oCtrlStatus.pProgramArg->nTraceRedirection ==
       TRACE_REDIRECTION_SYSLOG_LOCAL)
   {
#ifdef USE_FILESYSTEM
      /* Start redirecting logs to local syslog */
      openlog("Tapidemo", LOG_PID, LOG_USER);

      oTraceRedirection.fStartRedirectLogs = IFX_TRUE;

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Start redirecting logs to local syslog\n"));
#else
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Can not redirect log to local syslog without filesystem support."
             "File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
#endif
   }
   else if (oCtrlStatus.pProgramArg->nTraceRedirection ==
            TRACE_REDIRECTION_SYSLOG_REMOTE)
   {
      /* Start redirecting logs to remote syslog */
      oTraceRedirection.nSyslogSocket = TAPIDEMO_InitSyslogSocket();
      if (oTraceRedirection.nSyslogSocket == NO_SOCKET)
      {
         oTraceRedirection.fStartRedirectLogs = IFX_TRUE;

         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Can not redirect logs to remote syslog on address [%s]:%d\n",
                TD_GetStringIP(&oTraceRedirection.oSyslog_IP_Addr),
                TD_SOCK_PortGet(&oTraceRedirection.oSyslog_IP_Addr)));
         return IFX_ERROR;
      }
      else
      {
         oTraceRedirection.fStartRedirectLogs = IFX_TRUE;

         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
               ("Start redirecting logs to remote syslog on address [%s]:%d\n",
                TD_GetStringIP(&oTraceRedirection.oSyslog_IP_Addr),
                TD_SOCK_PortGet(&oTraceRedirection.oSyslog_IP_Addr)));
      }
   }
   else if (oCtrlStatus.pProgramArg->nTraceRedirection ==
            TRACE_REDIRECTION_FILE)
   {
#ifdef USE_FILESYSTEM
      /* Start redirecting logs to file */
      strncpy(full_filename, oTraceRedirection.sPathToLogFiles,
              MAX_FULL_FILE_LEN);
      oTraceRedirection.pFileLogFD = TD_OS_FOpen(full_filename, "a");
      if (0 > oTraceRedirection.pFileLogFD)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Can not create / open log file, %s\n"
                "File: %s, line: %d)\n",
                 strerror(errno), __FILE__, __LINE__));
         return IFX_ERROR;
      }

      oTraceRedirection.fStartRedirectLogs = IFX_TRUE;

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Start redirecting logs to file %s ",
             full_filename));
#else
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Can not redirect log to file without filesystem support."
             "File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
#endif
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, on setting redirection. "
             "File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Run Tapidemo as daemon

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
IFX_return_t TAPIDEMO_CreateDaemon(CTRL_STATUS_t* pCtrl)
{
   pid_t pid1, sid;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /*******************************************************************
         Create daemon
    *******************************************************************/
   /* Create new process - TAPIDEMO */
   pid1  = fork();
   if (pid1 < 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Can not create daemon. Error on fork %s\n"
             "File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));

      return IFX_ERROR;
   }
   else if (pid1 != 0)
   {
      /* Close parent process */
      exit(0);
   }

   /*******************************************************************
         Prepare pipe for redirect log
    *******************************************************************/
   strcpy(pCtrl->oMultithreadCtrl.oPipe.oRedirectLog.rgName,
          REDIRECT_LOG_PIPE);
   if (IFX_SUCCESS !=
       Common_PreparePipe(&pCtrl->oMultithreadCtrl.oPipe.oRedirectLog))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to prepare pipe for redirect log.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /********************************************************
         Start thread for handle log redirection
    ********************************************************/
   if (IFX_SUCCESS !=
       Common_StartThread(&pCtrl->oMultithreadCtrl.oThread.oHandleLogThreadCtrl,
                          TAPIDEMO_HandleLogThread,
                          "HandleLog",
                          &pCtrl->oMultithreadCtrl.oPipe.oLogSynch,
                          &pCtrl->oMultithreadCtrl.oLock.oLogThreadStopLock))
   {
       TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
             ("Err, Failed to start thread for handle log redirection\n."
              "(File: %s, line: %d)\n", __FILE__, __LINE__));
       return IFX_ERROR;
   }
   /* set started flag */
   pCtrl->oMultithreadCtrl.oThread.bLogThreadStarted = IFX_TRUE;

   /********************************************************
         Redirect standard output and error output stream
    ********************************************************/
   /* Redirect stdout to logger via pipe */
   if (-1 == dup2(pCtrl->oMultithreadCtrl.oPipe.oRedirectLog.rgFd[TD_PIPE_IN],
                  STDOUT_FILENO))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Can not redirect standard output. Error on dup2 %s.\n"
             "File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));

      return IFX_ERROR;
   }
   /* Redirect stderr to logger via pipe */
   if (-1 == dup2(pCtrl->oMultithreadCtrl.oPipe.oRedirectLog.rgFd[TD_PIPE_IN],
                  STDERR_FILENO))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Can not redirect standard error output. Error on dup2 %s.\n"
             "File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));

      return IFX_ERROR;
   }
   /* Start new session */
   sid = setsid();
   umask(0);

   return IFX_SUCCESS;
}
#endif /* defined(LINUX) */

/* ============================= */
/* Global function definition    */
/* ============================= */

#ifdef LINUX
/**
   Get and save network interface name in program arg structure.

   \param pProgramArg - pointer to program arguments structure
   \param pIfName - pointer to interface name
   \param nNameIsSet - IFX_TRUE if name is already set

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
IFX_return_t TAPIDEMO_SetNetInterfaceName(PROGRAM_ARG_t* pProgramArg,
                                          IFX_char_t* pIfName,
                                          IFX_boolean_t nNameIsSet)
{
   IFX_char_t aBuf[TD_MAX_SIOCGIFCONF_BUF] = {0};
   struct ifconf oIfc = {0};
   struct ifreq *pIfr = NULL;
   IFX_int32_t nInterfacesMax = 0;
   IFX_int32_t i = 0;
   struct ifreq *pItem;
   TD_OS_socket_t nSock;

   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   /* check if name was set */
   if (IFX_TRUE != nNameIsSet)
   {
      /* TD_OS_SocketCreate() can't be used because, we need third argument */
      nSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
      if (nSock == -1)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, socket(), %s. (File: %s, line: %d)\n",
                strerror(errno), __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* Query available interfaces. */
      oIfc.ifc_len = sizeof(aBuf);
      oIfc.ifc_buf = aBuf;
      if(ioctl(nSock, SIOCGIFCONF, &oIfc) < 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, ioctl(SIOCGIFCONF) failed %s. (File: %s, line: %d)\n",
                strerror(errno), __FILE__, __LINE__));
         close(nSock);
         return IFX_ERROR;
      }

      /* Iterate through the list of interfaces in search of interface
         with specific IP address. */
      pIfr = oIfc.ifc_req;
      nInterfacesMax = oIfc.ifc_len / sizeof(struct ifreq);
      for(i = 0; i < nInterfacesMax; i++)
      {
         pItem = &pIfr[i];
         /* check if this is searched IP address */
         if (IFX_TRUE ==  TD_SOCK_AddrCompare((TD_OS_sockAddr_t*)&pItem->ifr_addr,
                                              &pProgramArg->oMy_IP_Addr,
                                              IFX_FALSE))
         {
            /* copy name */
            strcpy(pIfName, pItem->ifr_name);
            nNameIsSet = IFX_TRUE;
            break;
         }
      }
      /* check if name was found */
      if (IFX_TRUE != nNameIsSet)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Failed to get network interface name with addr %s.\n"
                "(File: %s, line: %d)\n",
                TD_GetStringIP(&pProgramArg->oMy_IP_Addr),
                __FILE__, __LINE__));
         close(nSock);
         return IFX_ERROR;
      }
      close(nSock);
   }
   /* save interface name */
   strcpy(pProgramArg->aNetInterfaceName, pIfName);

   return IFX_SUCCESS;
}
#endif /* LINUX */

#ifdef TD_IPV6_SUPPORT
/**
   Get ip v6 address of board and set it.

   \param pCtrl - pointer to control structure

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
IFX_return_t TAPIDEMO_SetDefaultAddrIPv6(CTRL_STATUS_t* pCtrl)
{
   IFX_char_t acBuffer1[TD_ADDRSTRLEN];
   IFX_char_t acBuffer2[TD_ADDRSTRLEN];
   struct ifaddrs *pIfAddr, *pIfAddrSingle;
   IFX_int32_t nRet;
   IFX_boolean_t bIP_AddressAcquired = IFX_FALSE;
   struct sockaddr_in6 *pIPv6_Addr;
   IFX_int32_t i = 0;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);
   TD_PTR_CHECK(pCtrl->pProgramArg, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= strlen(pCtrl->pProgramArg->aNetInterfaceName)),
                      strlen(pCtrl->pProgramArg->aNetInterfaceName), IFX_ERROR);

   if (0 != getifaddrs(&pIfAddr))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, getifaddrs failed - %s.\n(File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (pCtrl->pProgramArg->oArgFlags.nAddrIsSetIPv6)
   {
      pCtrl->oIPv6_Ctrl.nAddrIsSet = IFX_TRUE;
      TD_SOCK_AddrCpy(&pCtrl->oIPv6_Ctrl.oAddrIPv6,
                      &pCtrl->pProgramArg->oMy_IPv6_Addr);
   }

   TRACE_IPV6(("DEBUG: look through interfaces.\n"));
   /* for all allocated structures */
   for (pIfAddrSingle = pIfAddr;
        NULL != pIfAddrSingle;
        pIfAddrSingle = pIfAddrSingle->ifa_next, i++)
   {
      if (NULL == pIfAddrSingle->ifa_name)
      {
         TRACE_IPV6(("DEBUG: %d: name not set.\n", i));
         continue;
      }
      if (NULL == pIfAddrSingle->ifa_addr)
      {
         TRACE_IPV6(("DEBUG: %d: ifa_addr not set.\n", i));
         continue;
      }
      TRACE_IPV6(("DEBUG: %d: sa_family = %d, AF_INET %d, AF_INET6 %d\n",
                  i, pIfAddrSingle->ifa_addr->sa_family, AF_INET, AF_INET6));
      if (AF_INET != pIfAddrSingle->ifa_addr->sa_family &&
          AF_INET6 != pIfAddrSingle->ifa_addr->sa_family)
      {
         TRACE_IPV6(("DEBUG: unhandled AF_INET %d\n",
                     pIfAddrSingle->ifa_addr->sa_family));
         continue;
      }
      nRet = getnameinfo(pIfAddrSingle->ifa_addr,
                         ((AF_INET == pIfAddrSingle->ifa_addr->sa_family) ?
                          sizeof(struct sockaddr_in) :
                          sizeof(struct sockaddr_in6)),
                         acBuffer1, sizeof(acBuffer1),
                         acBuffer2, sizeof(acBuffer2), 0);
      if (0 != nRet)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, getnameinfo failed - %s.\n(File: %s, line: %d)\n",
                gai_strerror(nRet), __FILE__, __LINE__));
         continue;
      }
      TRACE_IPV6(("DEBUG: hello if_name %s(%d) - buf1 %s - buf2 %s - string %s - scope %d.\n",
                  pIfAddrSingle->ifa_name, if_nametoindex(pIfAddrSingle->ifa_name),
                  acBuffer1, acBuffer2,
                  TD_GetStringIP((TD_OS_sockAddr_t*)pIfAddrSingle->ifa_addr),
                  TD_SOCK_IPv6ScopeGet((TD_OS_sockAddr_t*)pIfAddrSingle->ifa_addr)));
      if (IFX_TRUE != bIP_AddressAcquired &&
          AF_INET6 == pIfAddrSingle->ifa_addr->sa_family)
      {
         pIPv6_Addr = (struct sockaddr_in6 *) pIfAddrSingle->ifa_addr;
         if (IFX_TRUE == pCtrl->oIPv6_Ctrl.nAddrIsSet)
         {
            if (IFX_TRUE == TD_SOCK_AddrCompare(&pCtrl->oIPv6_Ctrl.oAddrIPv6,
                               (TD_OS_sockAddr_t *) pIPv6_Addr, IFX_FALSE))
            {
               TD_SOCK_AddrCpy(&pCtrl->oIPv6_Ctrl.oAddrIPv6,
                               (TD_OS_sockAddr_t *) pIPv6_Addr);
               bIP_AddressAcquired = IFX_TRUE;
               TRACE_IPV6(("DEBUG: %d: copy IP address set by -i.\n", i));
            }
         }
         else if (0 == strcmp(pCtrl->pProgramArg->aNetInterfaceName,
                              pIfAddrSingle->ifa_name))
         {
            /* check if local link address */
            if (pIPv6_Addr->sin6_addr.s6_addr[0] != 0xFE) continue;
            if (pIPv6_Addr->sin6_addr.s6_addr[1] < 0x80) continue;
            if (pIPv6_Addr->sin6_addr.s6_addr[1] > 0xBF) continue;

            TD_SOCK_AddrCpy(&pCtrl->oIPv6_Ctrl.oAddrIPv6,
                            (TD_OS_sockAddr_t *) pIPv6_Addr);
            TRACE_IPV6(("DEBUG: %d: copy IP address.\n", i));
            pCtrl->oIPv6_Ctrl.nAddrIsSet = IFX_TRUE;
            bIP_AddressAcquired = IFX_TRUE;
         }
      }
   }
   TRACE_IPV6(("DEBUG: Checking interfaces ended.\n"));
   freeifaddrs(pIfAddr);

   if (IFX_TRUE != bIP_AddressAcquired)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, failed to get IPv6 address, "
             "please check if IPv6 support is on.\n"
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* TAPIDEMO_SetDefaultAddrIPv6() */
#endif /* TD_IPV6_SUPPORT */

#ifndef EASY336
/**
   Get ip address of board and set it.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
IFX_return_t TAPIDEMO_SetDefaultAddr(PROGRAM_ARG_t* pProgramArg)
{
   /* In man page of hostname is written, that host names are limited
      to 255 bytes. */
   IFX_char_t buffer[256];
#ifdef LINUX
   IFX_char_t aIfName[TD_MAX_NAME] = TD_DEFAULT_NET_IF_NAME;
   /** Value for local ip address */
   const IFX_char_t* LOCAL_IP_ADDRESS = "127.0.0.1";
   struct hostent* he = IFX_NULL;
   TD_OS_socket_t sockt_fd = NO_SOCKET;
   struct ifreq ifr;
   IFX_boolean_t bAskSocket = IFX_FALSE;
#else /* LINUX */
   IFX_int32_t addr = 0;
#endif /* LINUX */

   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   /* set IPv4 address */
   TD_SOCK_FamilySet(&pProgramArg->oMy_IP_Addr, AF_INET);

   /* check if address is already set e.g. by command line */
   if (TD_SOCK_AddrIPv4Get(&pProgramArg->oMy_IP_Addr) == 0)
   {
      if (gethostname(buffer, sizeof(buffer)) != 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, gethostname(), %s. (File: %s, line: %d)\n",
                strerror(errno), __FILE__, __LINE__));
         return IFX_ERROR;
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT, ("gethostname() returned %s.\n", buffer));
   }

#ifdef LINUX
   /* TD_OS_SocketCreate() can't be used because, we need third argument */
   sockt_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
   if (sockt_fd == -1)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, socket(), %s. (File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check if address is already set e.g. by command line */
   if (TD_SOCK_AddrIPv4Get(&pProgramArg->oMy_IP_Addr) == 0)
   {
      he = gethostbyname(buffer);

      if (IFX_NULL == he)
      {
         bAskSocket = IFX_TRUE;
      }
      else
      {
         if (he->h_addr_list[0] != IFX_NULL)
         {
            /* save IP address */
            TD_SOCK_AddrIPv4Set(&pProgramArg->oMy_IP_Addr,
                                *(IFX_uint32_t *)he->h_addr_list[0]);

            if (strcmp(TD_GetStringIP(&pProgramArg->oMy_IP_Addr),
                       LOCAL_IP_ADDRESS) == 0)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
                     ("Local address (127.0.0.1) read, "
                      "will now try to check eth0 for right address.\n"
                      "On error local address will be used.\n"));
               bAskSocket = IFX_TRUE;
            }
         }
      }
      /* if failed to get IP address then try to get it with ioctl() */
      if (bAskSocket)
      {
         /* get IP address for specific network interface */
         memset(&ifr, 0, sizeof (struct ifreq));
         strcpy (ifr.ifr_name, aIfName);
         ifr.ifr_addr.sa_family = AF_INET;
         if (ioctl(sockt_fd, SIOCGIFADDR, &ifr) == -1)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, with ioctl cmd SIOCGIFADDR %s. "
                   "(File: %s, line: %d)\n",
                   strerror(errno), __FILE__, __LINE__));
            close(sockt_fd);
            return IFX_ERROR;
         }
         /* save IP address */
         TD_SOCK_AddrIPv4Set(&pProgramArg->oMy_IP_Addr,
            TD_SOCK_AddrIPv4Get((TD_OS_sockAddr_t *) &ifr.ifr_addr));
      } /* if (bAskSocket) */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Default IP Address for host %s is %s\n",
             buffer, TD_GetStringIP(&pProgramArg->oMy_IP_Addr)));
   }
   /* save IP address */
   TAPIDEMO_SetNetInterfaceName(pProgramArg, aIfName, bAskSocket);

   /* get MAC address */
   memset(&ifr, 0, sizeof (struct ifreq));
   strcpy (ifr.ifr_name, aIfName);
   ifr.ifr_addr.sa_family = AF_INET;
   if (ioctl(sockt_fd, SIOCGIFHWADDR, &ifr) == -1)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, with ioctl cmd SIOCGIFHWADDR %s. "
             "(File: %s, line: %d)\n",
             strerror(errno), __FILE__, __LINE__));
      close(sockt_fd);
      return IFX_ERROR;
   }
   /* save MAC address */
   memcpy(oCtrlStatus.oTapidemo_MAC_Addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

   close(sockt_fd);
#elif VXWORKS /* LINUX */

   /* There is also possibility to use resolvGetHostByName(), but
      in file configAll.h support for library resolvLib,
      #define INCLUDE_DNS_RESOLVER must be added and new image
      prepared */
   addr = hostGetByName(buffer);

   TD_SOCK_AddrIPv4Set(&pProgramArg->oMy_IP_Addr, addr);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("Default IP Address for host %s is %s\n",
          buffer, TD_GetStringIP(&pProgramArg->oMy_IP_Addr)));
#else
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("Unsupported function TAPIDEMO_SetDefaultAddr()\n"));
#endif /* LINUX */


   return IFX_SUCCESS;
} /* TAPIDEMO_SetDefaultAddr() */

#else /* EASY336 */

/**
   Get ip address of board and set it.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
IFX_return_t TAPIDEMO_SetDefaultAddr(PROGRAM_ARG_t* pProgramArg)
{
   IFX_char_t aIfName[TD_MAX_NAME] = TD_DEFAULT_NET_IF_NAME;
   SVIP_libStatus_t SVIP_libRet;
   IFX_char_t pIP[INET_ADDR_LEN];

   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   TD_SOCK_FamilySet(&pProgramArg->oMy_IP_Addr, AF_INET);

   if (TD_SOCK_AddrIPv4Get(&pProgramArg->oMy_IP_Addr) == 0)
   {
      SVIP_libRet = SVIP_libGetAddress(aIfName, pIP,
                                       oCtrlStatus.oTapidemo_MAC_Addr);
      if (SVIP_libRet != SVIP_libStatusOk)
         return IFX_ERROR;

      if (IFX_SUCCESS != TD_OS_SocketAton(pIP,
#ifdef TD_IPV6_SUPPORT
   /* for IPv6 oVeth0_IP_Addr has different type */
                                          (IFXOS_sockAddr_t*)
#endif /* TD_IPV6_SUPPORT */
                                          &pProgramArg->oMy_IP_Addr))
         return IFX_ERROR;

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("Default IP-Address is %s\n", pIP));
   }

   /* get address */
   SVIP_libRet = SVIP_libGetAddress(aIfName, IFX_NULL,
                                    oCtrlStatus.oTapidemo_MAC_Addr);
   if (SVIP_libRet != SVIP_libStatusOk)
      return IFX_ERROR;

#ifdef LINUX
   TAPIDEMO_SetNetInterfaceName(pProgramArg, aIfName, IFX_FALSE);
#endif /* LINUX */

   return IFX_SUCCESS;
} /* TAPIDEMO_SetDefaultAddr() */
#endif /* EASY336 */

#if (defined EASY336 || defined XT16)
/**
   Get IP and MAC addresses of veth0 interface of board and set it.

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
static IFX_return_t TAPIDEMO_SetVeth0Addr()
{
#ifdef EASY336
   SVIP_libStatus_t SVIP_libRet;
   IFX_char_t pIP[INET_ADDR_LEN];

   SVIP_libRet = SVIP_libGetAddress("veth0", pIP,
      oCtrlStatus.oVeth0_MAC_Addr);
   if (SVIP_libRet != SVIP_libStatusOk)
   {
      return IFX_ERROR;
   }

   if (IFX_SUCCESS != TD_OS_SocketAton(pIP,
#ifdef TD_IPV6_SUPPORT
   /* for IPv6 oVeth0_IP_Addr has different type */
                                       (IFXOS_sockAddr_t*)
#endif /* TD_IPV6_SUPPORT */
                                       &oCtrlStatus.oVeth0_IP_Addr))
   {
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
        ("veth0 IP-Address: %s, MAC %02X:%02X:%02X:%02X:%02X:%02X\n", pIP,
         oCtrlStatus.oVeth0_MAC_Addr[0],
         oCtrlStatus.oVeth0_MAC_Addr[1],
         oCtrlStatus.oVeth0_MAC_Addr[2],
         oCtrlStatus.oVeth0_MAC_Addr[3],
         oCtrlStatus.oVeth0_MAC_Addr[4],
         oCtrlStatus.oVeth0_MAC_Addr[5]));
#endif /* EASY336 */

   return IFX_SUCCESS;
} /* TAPIDEMO_SetVeth0Addr() */
#endif

/**
   Set default program settings.

   \param pProgramArg - program arguments

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t TAPIDEMO_SetDefaultProgramArgs(PROGRAM_ARG_t* pProgramArg)
{
   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   /* Trace level is set to high by default */
   pProgramArg->nDbgLevel = DBG_LEVEL_HIGH;
#ifdef EASY336
   /* Trace RM level is set to high by default */
   pProgramArg->nRmDbgLevel = DBG_LEVEL_HIGH;
#endif /* EASY336 */
   /* TAPI Trace level is set to high by default */
   pProgramArg->nTapiDbgLevel = IFX_TAPI_DEBUG_REPORT_SET_HIGH;
   /* Trace redirection is disabled by default. See also function
      TAPIDEMO_SetDefault(). */
   pProgramArg->nTraceRedirection = TRACE_REDIRECTION_NONE;
#ifdef TD_DECT
   /* default DECT debug level */
   pProgramArg->nDectDbgLevel = DBG_LEVEL_OFF;
#endif /* TD_DECT */

   pProgramArg->nBoardComb = NO_EXT_BOARD;

   /* Voice Activity Detection */
   pProgramArg->nVadCfg = DEFAULT_VAD_CFG;

   /* LEC */
   pProgramArg->nLecCfg = DEFAULT_LEC_CFG;

#ifdef TD_DECT
   /* EC for DECT */
   pProgramArg->nDectEcCfg = DEFAULT_DECT_EC_CFG;
#endif /* TD_DECT */

   /* CID is turned on by default and set to ETSI FSK */
   pProgramArg->oArgFlags.nCID = 1;

   pProgramArg->nCIDStandard = CID_GetStandard();

   /* set default options */
#if (!defined XT16)
   /* Turn conferencing on */
   pProgramArg->oArgFlags.nConference = 1;
#endif /* XT16 */

#ifdef EASY336
   /* enable PCM slave */
   pProgramArg->oArgFlags.nPCM_Master = 0;
   pProgramArg->oArgFlags.nPCM_Slave = 1;
#else
   /* enable PCM master */
   pProgramArg->oArgFlags.nPCM_Master = 1;
   pProgramArg->oArgFlags.nPCM_Slave = 0;
#endif

#if defined (EASY3201) || defined (EASY3201_EVS) || defined (XT16)
   /* EASY3201 and XT16(without resource mannager and EASY336)
      by default uses PCM local loop */
   pProgramArg->oArgFlags.nPCM_Loop = 1;
#else
   pProgramArg->oArgFlags.nPCM_Loop = 0;
#endif /* DXT */

#ifdef TD_DECT
   /* dect support is disabled by default */
   pProgramArg->oArgFlags.nNoDect = 1;
#endif /* TD_DECT */
   /* Default path to download files */
   strncpy(pProgramArg->sPathToDwnldFiles, DEF_DOWNLOAD_PATH, MAX_PATH_LEN - 1);

#ifdef USE_FILESYSTEM
   /* Default FW, DRAM and BBD filenames */
   strncpy(pProgramArg->sFwFileName, "", MAX_PATH_LEN - 1);
   strncpy(pProgramArg->sDramFileName, "", MAX_PATH_LEN - 1);
   strncpy(pProgramArg->sBbdFileName, "", MAX_PATH_LEN - 1);
#endif /* USE_FILESYSTEM */

   /* default encoder and packetisation time */
   pProgramArg->nEnCoderType = TD_DEFAULT_CODER_TYPE;
   pProgramArg->nPacketisationTime = TD_DEFAULT_PACKETISATION_TIME;

   return IFX_SUCCESS;
} /* TAPIDEMO_SetDefaultProgramArgs() */

/**
   Set default program settings.

   \param  pCtrl   - pointer to status control structure
   \param pProgramArg - program arguments

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t TAPIDEMO_SetDefault(CTRL_STATUS_t* pCtrl)
{
   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* Trace level is set to high by default */
   pCtrl->pProgramArg->nDbgLevel = DBG_LEVEL_HIGH;
   TD_OS_PRN_USR_LEVEL_SET(TAPIDEMO, pCtrl->pProgramArg->nDbgLevel);


   /* configure program arguments */
   if (IFX_ERROR == TAPIDEMO_SetDefaultProgramArgs(pCtrl->pProgramArg))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Setting program options to default values failed. "
             "File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* save default setting before reading program command line arguments */
   if (IFX_ERROR == TAPIDEMO_SetDefaultProgramArgs(
                       &pCtrl->oITM_Ctrl.oDefaultProgramArg))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Setting program options to default values failed. "
             "File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Trace redirection is disabled by default. See also program
      arguments (function TAPIDEMO_SetDefaultProgramArgs()). */
   oTraceRedirection.fStartRedirectLogs = IFX_FALSE;
   oTraceRedirection.pFileLogFD = IFX_NULL;
   oTraceRedirection.nSyslogSocket = NO_SOCKET;

#ifdef TD_FAX_MODEM
   pCtrl->nFaxMode = TD_FAX_MODE_DEAFAULT;
#endif /* TD_FAX_MODEM */

#ifdef TD_IPV6_SUPPORT
   pCtrl->oIPv6_Ctrl.nSocketFd = TD_NOT_SET;
#endif /* TD_IPV6_SUPPORT */

   return IFX_SUCCESS;
} /* TAPIDEMO_SetDefault() */


/**
   Set and register the main board.

   The main board is set based on compilation flag '--enable-boardname'.
   The main board's ID is always 1.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t TAPIDEMO_SetMainBoard(BOARD_t* pBoard)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);

#if (defined(EASY3332) && defined(HAVE_DRV_EASY3332_HEADERS))
   pBoard->nType = TYPE_VINETIC;
   pBoard->nID = 1;
#elif (defined(DANUBE) && defined(HAVE_DRV_VMMC_HEADERS))
   pBoard->nType = TYPE_DANUBE;
   pBoard->nID = 1;
#elif (defined(EASY3201) && defined(HAVE_DRV_DUSLICXT_HEADERS))
   pBoard->nType = TYPE_DUSLIC_XT;
   pBoard->nID = 1;
#elif (defined(EASY3201_EVS) && defined(HAVE_DRV_DUSLICXT_HEADERS))
   pBoard->nType = TYPE_DUSLIC_XT;
   pBoard->nID = 1;
#elif (defined(VINAX) && defined(HAVE_DRV_VMMC_HEADERS))
   pBoard->nType = TYPE_VINAX;
   pBoard->nID = 1;
#elif (defined(AR9) && defined(HAVE_DRV_VMMC_HEADERS))
   pBoard->nType = TYPE_AR9;
   pBoard->nID = 1;
#elif (defined(TD_XWAY_XRX300) && defined(HAVE_DRV_VMMC_HEADERS))
   pBoard->nType = TYPE_XWAY_XRX300;
   pBoard->nID = 1;
#elif (defined(VR9) && defined(HAVE_DRV_VMMC_HEADERS))
   pBoard->nType = TYPE_VR9;
   pBoard->nID = 1;
#elif EASY336
   pBoard->nType = TYPE_SVIP ;
   pBoard->nID = 1;
#elif XT16
   pBoard->nType = TYPE_XT16;
   pBoard->nID = 1;
#else
   /* Unknown board type or miss driver's headers.*/
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Unknown board type. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
   return IFX_ERROR;
#endif /* EASY3332 */

   if (IFX_SUCCESS != Common_RegisterBoard(pBoard))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, The main board registration failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Increase number of boards */
   oCtrlStatus.nBoardCnt++;

   return IFX_SUCCESS;
} /* TAPIDEMO_SetMainBoard */

/**
   Check board combination.

   This function is used only by obsolete argument '-b'. It checks if user's
   board combination is supported based on compilation flags.

   \param pBoard - pointer to extension board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t TAPIDEMO_CheckBoardCombination(BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_ERROR;
   /* Set PCM mode must be checked */
   IFX_boolean_t hCheckPcmIfMode = IFX_FALSE;
   IFX_int32_t nID = -1;

   TD_PTR_CHECK(oCtrlStatus.pProgramArg, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   switch (oCtrlStatus.pProgramArg->nBoardComb)
   {
      case ONE_CPE:
      {
         /* TYPE_VINETIC and NO_EXT_BOARD has got the same value '0'.
            Tapidemo'll never enter this section. However, it's correct because,
            ONE_CPE means single board, so Tapidemo doesn't check for extension board */
         if (TYPE_VINETIC == oCtrlStatus.rgoBoards[0].nType)
         {
            return IFX_SUCCESS;
         }
      }
      case ONE_VMMC:
         {
         if (TYPE_DANUBE == oCtrlStatus.rgoBoards[0].nType)
            {
            return IFX_SUCCESS;
         }
         break;
            }
      case ONE_VMMC_ONE_CPE:
         {
            if (TYPE_DANUBE == oCtrlStatus.rgoBoards[0].nType)
            {
#if (defined(HAVE_DRV_VINETIC_HEADERS) && !defined(EASY3332))
               nID = BOARD_Easy50510_DetectNode(&oCtrlStatus);
               if (NO_EXT_BOARD_DETECT != nID)
               {
                  /* An extesion board has been detected. */
                  pBoard->nType = TYPE_VINETIC;
                  pBoard->nID = nID;
                  hCheckPcmIfMode = IFX_TRUE;
                  ret = IFX_SUCCESS;
               }
               else
               {
                  TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                     ("Err, Missing device node for board EASY50510.\n"
                      "     Please check if driver is loaded.\n"));
               }
#else
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Err, During compilation, require driver "
                "     VINETIC-CPE wasn't found.\n"
                "     Please recompile with missing driver.\n"));
#endif /* HAVE_DRV_VINETIC_HEADERS */
            }
            break;
         }
      case ONE_DXT:
         {
            if (TYPE_DUSLIC_XT == oCtrlStatus.rgoBoards[0].nType)
            {
               return IFX_SUCCESS;
            }
         }
      case TWO_DXTS:
         {
            if (TYPE_DUSLIC_XT == oCtrlStatus.rgoBoards[0].nType)
            {
#ifdef DXT
               nID = BOARD_Easy3111_DetectNode(&oCtrlStatus);
               if (NO_EXT_BOARD_DETECT != nID)
               {
                  /* An extesion board has been detected. */
                  pBoard->nType = TYPE_DUSLIC_XT;
                  pBoard->nID = nID;
                  ret = IFX_SUCCESS;
               }
               else
               {
                  TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                     ("Err, Missing device node for board EASY3111.\n"));
               }
#endif /* DXT */
            }
            break;
         }
      case ONE_VMMC_VINAX:
         {
            if (TYPE_VINAX == oCtrlStatus.rgoBoards[0].nType)
            {
               return IFX_SUCCESS;
            }
            break;
         }
      case ONE_VMMC_EASY508XX:
         {
            if (TYPE_AR9 == oCtrlStatus.rgoBoards[0].nType)
            {
               return IFX_SUCCESS;
            }
            break;
         }
      case ONE_EASY80910:
         {
            if (TYPE_VR9 == oCtrlStatus.rgoBoards[0].nType)
            {
               return IFX_SUCCESS;
            }
            break;
         }
      case X_SVIP:
         {
            if (TYPE_SVIP == oCtrlStatus.rgoBoards[0].nType)
            {
               return IFX_SUCCESS;
            }
            break;
         }
      case X_SVIP_X_XT16:
         {
            if (TYPE_SVIP == oCtrlStatus.rgoBoards[0].nType)
            {
               /* Extension board XT16 is detected by svipbox.
                  Check if TAPIDEMO was compiled with support for that board */
#ifdef WITH_VXT
               /* An extesion board has been detected. */
               pBoard->nType = TYPE_XT16;
               pBoard->nID = 1;
               ret = IFX_SUCCESS;
#else
               TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                  ("Err, TAPIDEMO is compiled without support for XT16.\n"
                   "       Please use compilation flag --with-vxt.\n"));
#endif /* WITH_VXT */
            }
            break;
         }
      case X_XT16:
         {
            if (TYPE_XT16 == oCtrlStatus.rgoBoards[0].nType)
            {
               return IFX_SUCCESS;
            }
            break;
         }
      case BOARD_COMB_RESERVED:
      default:
         {
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Unknown board combination.\n(File: %s, line: %d)\n",
                __FILE__, __LINE__));
            return IFX_ERROR;
          }
   } /* switch */

   /* To prevent compilation warning */
   nID = -1;

   if (IFX_SUCCESS != ret)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("User board combination: %s is unsupported.\n"
          "Compilation for: %s.\nFile: %s, line: %d)\n",
          BOARD_COMB_NAMES[oCtrlStatus.pProgramArg->nBoardComb],
          oCtrlStatus.rgoBoards[0].pszBoardName,
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else
   {
      /* Register extension board */
      if (IFX_SUCCESS != Common_RegisterBoard(pBoard))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, The extension board registration failed.\n"
                "(File: %s, line: %d)\n", __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Increase number of boards */
      oCtrlStatus.nBoardCnt++;

      /* Set PCM mode */
      if (hCheckPcmIfMode)
      {
         if (oCtrlStatus.pProgramArg->oArgFlags.nPCM_Slave)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Main board - the PCM i/f can not be configured as a SLAVE when \n"
                "extension board is used.\n"
                "PCM i/f will be set as MASTER\n"));
            oCtrlStatus.pProgramArg->oArgFlags.nPCM_Slave = 0;
         }
         oCtrlStatus.pProgramArg->oArgFlags.nPCM_Master = 1;
      }  /* if */
   }

   return IFX_SUCCESS;
} /* TAPIDEMO_CheckBoardCombination() */

/**
   Set and register extension board.

   Check which extension boards are avaiable. It's done by checking
   if required device node exists (e.g. for EASY50510 needs vin1x)
   in directory /dev/. Device node is created during board's start-up,
   regardless if extension board is connected to main board or not.

   \param pBoard - pointer to board

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t TAPIDEMO_SetExtBoard(BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_ERROR;
   /* Set PCM mode must be checked */
   IFX_boolean_t hCheckPcmIfMode = IFX_FALSE;
   IFX_int32_t nID = -1;

   TD_PTR_CHECK(pBoard, IFX_ERROR);

   do {
#ifdef DXT
      nID = BOARD_Easy3111_DetectNode(&oCtrlStatus);
      if (NO_EXT_BOARD_DETECT != nID)
      {
         /* An extesion board has been detected.
            Check if Tapidemo doesn't detect the same board twice time. */
         if (IFX_TRUE != TAPIDEMO_ExtBoardCheckForDuplicate(TYPE_DUSLIC_XT, nID))
         {
            pBoard->nType = TYPE_DUSLIC_XT;
            pBoard->nID = nID;
            ret = IFX_SUCCESS;
            break;
         }
      }
#endif /* DXT */

#if (defined(HAVE_DRV_VINETIC_HEADERS) && !defined(EASY3332))
      nID = BOARD_Easy50510_DetectNode(&oCtrlStatus);
      if (NO_EXT_BOARD_DETECT != nID)
      {
         /* An extesion board has been detected.
            Check if Tapidemo doesn't detect the same board twice time. */
         if (IFX_TRUE != TAPIDEMO_ExtBoardCheckForDuplicate(TYPE_VINETIC, nID))
         {
            hCheckPcmIfMode = IFX_TRUE;
            pBoard->nType = TYPE_VINETIC;
            pBoard->nID = nID;
            ret = IFX_SUCCESS;
            break;
         }
      }
#endif /* HAVE_DRV_VINETIC_HEADERS */

#ifdef WITH_VXT
      /* Extension board XT16 is detected by svipbox.
         Check if TAPIDEMO was compiled with support for that board */
      if (IFX_TRUE != TAPIDEMO_ExtBoardCheckForDuplicate(TYPE_XT16, 1))
      {
         pBoard->nType = TYPE_XT16;
         pBoard->nID = 1;
         ret = IFX_SUCCESS;
         break;
      }
#endif /* WITH_VXT */
   } while (0);

   /* To prevent compilation warning */
   nID = -1;

   if (IFX_SUCCESS == ret)
   {

      if (IFX_SUCCESS != Common_RegisterBoard(pBoard))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Board registration failed. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Increase number of boards */
      oCtrlStatus.nBoardCnt++;

      /* Set PCM mode */
      if (hCheckPcmIfMode)
      {
         if (oCtrlStatus.pProgramArg->oArgFlags.nPCM_Slave)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Main board - the PCM i/f can not be configured as a SLAVE when \n"
                "extension board is used.\n"
                "PCM i/f will be set as MASTER\n"));
            oCtrlStatus.pProgramArg->oArgFlags.nPCM_Slave = 0;
         }
         oCtrlStatus.pProgramArg->oArgFlags.nPCM_Master = 1;
      }  /* hCheckPcmIfMode */
   } /* ret */

   return ret;
} /* TAPIDEMO_SetExtBoard */

/**
   Check if extension board has been detected before.

   Search all registered extension boards for detected board by comparing
   board type and ID.

   \param nType - type
   \param nType - ID

   \return IFX_TRUE if board is already detected, otherwise IFX_FALSE.
*/
IFX_boolean_t TAPIDEMO_ExtBoardCheckForDuplicate(IFX_int32_t nType,
                                                 IFX_int32_t nID)
{
   IFX_int32_t i;
   /* Check if detected board has been alread set */
   for(i = 1; i < oCtrlStatus.nBoardCnt; i++)
   {
      if ((nType == oCtrlStatus.rgoBoards[i].nType) &&
          (nID == oCtrlStatus.rgoBoards[i].nID))
      {
         /* This board has been already set */
         return IFX_TRUE;
      }
   } /* for */
   return IFX_FALSE;
} /* TAPIDEMO_CheckIfExtBoardDetected */

/**
   Start board.

   Initilize all registered board. Firstly the main board is processed.
   Next, all extension board are processed. If extension board isn't
   connected to the main board, its initialization is stopped and trace is printed.
   Tapidemo shouldn't stop working, when extension board's initialization failed.

   \param  nIndex - index of board which is started

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t TAPIDEMO_StartBoard(IFX_int32_t nIndex)
{
   BOARD_t* pBoard = IFX_NULL;

   TD_PARAMETER_CHECK((0 > nIndex), nIndex, IFX_ERROR);

   pBoard = &oCtrlStatus.rgoBoards[nIndex];

   pBoard->nBoard_IDX = nIndex;
   pBoard->pCtrl = &oCtrlStatus;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("----- Start board %s initialization. -----\n",
          pBoard->pszBoardName));

   /* Init board */
   if (IFX_SUCCESS != pBoard->Init(pBoard, &oProgramArg))
   {
      if (nIndex == 0)
      {
         /* The main board is always initialized firstly. */
         /* Main board initialization failed. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, The main board initialization failed (Idx %d).\n(File: %s, line: %d)\n",
            pBoard->nBoard_IDX, __FILE__, __LINE__));
         /* Cleanup previous stuff */
#ifdef VXWORKS
         abort();
#else /* VXWORKS */
         TAPIDEMO_CleanUp(SIGABRT);
#endif /* VXWORKS */
         return IFX_ERROR;
      }
      else
      {
         /* The extension board initialization failed. It might happen when
         extension board's driver is installed and nodes are created but device
         is not connected to main board. */

         TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
               ("-----      Board %s not initialized.     -----\n",
                pBoard->pszBoardName));

         /* Make clean in board array */
         if (IFX_SUCCESS != TAPIDEMO_RebuildBoardArray(nIndex))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Rebuilding board array failed.\n(File: %s, line: %d)\n",
                __FILE__, __LINE__));
            /* Cleanup previous stuff */
#ifdef VXWORKS
            abort();
#else /* VXWORKS */
            TAPIDEMO_CleanUp(SIGABRT);
#endif /* VXWORKS */
            return IFX_ERROR;
         }
         return IFX_SUCCESS;
      } /*  if */
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("-----      Board %s initialized.     -----\n",
          pBoard->pszBoardName));

   /* Save sum of all channels */
   oCtrlStatus.nSumCoderCh += pBoard->nMaxCoderCh;
   oCtrlStatus.nSumPhones += pBoard->nMaxPhones;
   oCtrlStatus.nSumPCM_Ch += pBoard->nMaxPCM_Ch;

   return IFX_SUCCESS;
} /* TAPIDEMO_StartBoard */

/**
   Rebuild board array.

   Initialization of extension board failed. All registered boards
   are kept in one array. Tapidemo must rebuild this array.

   \param  nIndex - index of board which is failed

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t TAPIDEMO_RebuildBoardArray(IFX_int32_t nIndex)
{
   /* Pointer on board */
   BOARD_t* pBoard = IFX_NULL;
   BOARD_t* pNextBoard = IFX_NULL;
   IFX_int32_t i;

   TD_PARAMETER_CHECK((0 >= nIndex), nIndex, IFX_ERROR);

   /* Remove board which initilization failed */

   /* Get extension board which initialization failed */
   pBoard = &oCtrlStatus.rgoBoards[nIndex];
   /* Release board and free memory. */
   pBoard->RemoveBoard(pBoard);
   memset(pBoard, 0, sizeof (BOARD_t));
   pBoard = IFX_NULL;

   /* If board was on last position in array, don't do anything more */
   if (nIndex == (oCtrlStatus.nBoardCnt-1))
   {
      oCtrlStatus.nBoardCnt--;
      return IFX_SUCCESS;
   }

   /* If board wasn't on last position in array, we need to rebuild array */

   /* Build new array of boards without board which initilization failed */
   for (i=nIndex; i < oCtrlStatus.nBoardCnt-1; i++)
   {
      /* Pointer on new position in array */
      pBoard = &oCtrlStatus.rgoBoards[i];
      /* Pointer on old position in array */
      pNextBoard = &oCtrlStatus.rgoBoards[i+1];

      /* Copy elements to new position */
      pBoard->nType = pNextBoard->nType;
      pBoard->nID = pNextBoard->nID;
      pBoard->pszBoardName = pNextBoard->pszBoardName;
      pBoard->Init = pNextBoard->Init;
      pBoard->RemoveBoard = pNextBoard->RemoveBoard;
   } /* for */

   /* Remove last position in array, because it has been copied to new position */
   pBoard = &oCtrlStatus.rgoBoards[oCtrlStatus.nBoardCnt-1];
   /* Release board and free memory. */
   memset(pBoard, 0, sizeof (BOARD_t));
   pBoard = IFX_NULL;
   oCtrlStatus.nBoardCnt--;

   return IFX_SUCCESS;
} /* TAPIDEMO_RebuildBoardArray */

#ifdef FXO
/**
   Check timeouts for FXO call

   Check if timeout for FXO call elapsed. If yes, perform the appropriate action.
   If not, adjust timeout value.

   \param  pCtrl - pointer to status control structure
   \param pTimeOut   - pointer to current timeout for select

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TAPIDEMO_CheckTimeoutFxoCall(CTRL_STATUS_t* pCtrl,
                                          IFX_time_t* pTimeOut)
{
   BOARD_t* pBoard = IFX_NULL;
   PHONE_t* pPhone = IFX_NULL;
   IFX_int32_t nBoard;
   FXO_t* pFXO = IFX_NULL;
   IFX_int32_t nPhone;
   IFX_int32_t nFxoNum = 0;
   IFX_boolean_t fFxoEnd = IFX_FALSE;
   IFX_time_t nDiffMSec;
   IFX_time_t time_new;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   pBoard = TD_GET_MAIN_BOARD(pCtrl);
   /* check if FXO is enabled */
   if (!pCtrl->pProgramArg->oArgFlags.nFXO)
   {
      return IFX_SUCCESS;
   }
   /* for all FXOs (counted from 1) */
   for (nFxoNum = 1; nFxoNum <= pCtrl->nMaxFxoNumber; nFxoNum++)
   {
      /* get fxo of fxo number */
      pFXO = ABSTRACT_GetFxo(nFxoNum, pCtrl);
      if (pFXO == IFX_NULL)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
               ("Err, FXO not found for fxo channel %d. (File: %s, line: %d)\n",
                nFxoNum, __FILE__, __LINE__));
         break;
      }
      /* if ringing on FXO channel */
      if (S_IN_RINGING == pFXO->nState)
      {
         /* Check if timeout for ringing event - it means
            caller phone went ONHOOK and ringing must be stopped on
            all the called phones. */;
         nDiffMSec = TD_OS_ElapsedTimeMSecGet(pFXO->timeRingEventTimeout);
         /* check timeout */
         if (nDiffMSec >= TIMEOUT_FOR_ONHOOK)
         {
            /* if timeout occured then FXO call has ended */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW,
               (TD_CONN_ID_INIT == pFXO->nSeqConnId) ?
               TD_CONN_ID_NOT_SET : pFXO->nSeqConnId,
               ("Timeout diff %lu msec for ringing elapsed on fxo No %d\n"
                "        stop ringing on all called phones.\n",
                nDiffMSec, nFxoNum));
            /* will be set to true if fxo connection is ended during
               phone state transition */
            fFxoEnd = IFX_FALSE;
            /* for all boards */
            for (nBoard = 0; nBoard < pCtrl->nBoardCnt; nBoard++)
            {
               pBoard = &pCtrl->rgoBoards[nBoard];
               /* if main board or EASY 3111 */
               if ((0 == nBoard) || (TYPE_DUSLIC_XT == pBoard->nType))
               {
                  /* for all phones on ring board */
                  for (nPhone=0; nPhone < pBoard->nMaxPhones; nPhone++)
                  {
                     /* get phone */
                     pPhone = &pBoard->rgoPhones[nPhone];
                     /* check if phone ringing caused by fxo call */
                     if ((pPhone->rgoConn[0].nType == FXO_CALL) &&
                         (pPhone->rgStateMachine[FXS_SM].nState == S_IN_RINGING))
                     {
                        fFxoEnd = IFX_TRUE;
                        /* fxo call ended - stop ringing */
                        pPhone->nIntEvent_FXS = IE_READY;

                        while (pPhone->nIntEvent_FXS != IE_NONE)
                        {
                           ST_HandleState_FXS(pCtrl, pPhone,
                                              &pPhone->rgoConn[0]);
                        }
                     } /* if ringing uring FXO call*/
                  } /*for all phones on ring board */
               } /* if main board or EASY 3111 */
            } /* for all boards */
            /* check if any phone was ringing */
            if (IFX_TRUE != fFxoEnd)
            {
               FXO_EndConnection(pCtrl->pProgramArg, pFXO,
                                 IFX_NULL, IFX_NULL,
                                 TD_GET_MAIN_BOARD(pCtrl));
            }
         } /* if (time_diff > TIMEOUT_FOR_ONHOOK) */
         else
         {
            /* Check if timeout should be smaller than it is currently set */
            time_new = TIMEOUT_FOR_ONHOOK - nDiffMSec;
            if (time_new < *pTimeOut)
            {
               *pTimeOut = time_new;
            }
         }  /* if (time_diff > TIMEOUT_FOR_ONHOOK) */
      } /* if ringing on FXO channel */
   } /* for all FXOs on board */
   return IFX_SUCCESS;
} /* TAPIDEMO_CheckTimeoutFxoCall */
#endif /* FXO */

/**
   Check timeouts for external call

   Check if timeout for external call elapsed.
   If yes, perform the appropriate action.
   If not, adjust timeout value.

   \param  pCtrl   - pointer to status control structure
   \param pTimeOut   - pointer to current timeout for select

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TAPIDEMO_CheckTimeoutExtCall(CTRL_STATUS_t* pCtrl,
                                          IFX_time_t* pTimeOut)
{
   BOARD_t* pBoard = IFX_NULL;
   PHONE_t* pPhone = IFX_NULL;
   CONNECTION_t* pConn = IFX_NULL;
   IFX_int32_t nBoard, nPhone, nConn;
   IFX_time_t nDiffMSec;
   IFX_time_t time_new;

   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /* for all boards */
   for(nBoard = 0; nBoard < pCtrl->nBoardCnt; nBoard++)
   {
      /* get board */
      pBoard = &pCtrl->rgoBoards[nBoard];
      /* for all phones on board */
      for (nPhone=0; nPhone < pBoard->nMaxPhones; nPhone++)
      {
         pPhone = &pBoard->rgoPhones[nPhone];
         /* for all connections */
         for (nConn=0; nConn < pPhone->nConnCnt; nConn++)
         {
            pConn = &pPhone->rgoConn[nConn];
            /* check connection (must be external call in dialing) */
            if ((pConn->nType == EXTERN_VOIP_CALL ||
                 pConn->nType == PCM_CALL) &&
                (pConn->bExtDialingIn == IFX_TRUE))
            {
               nDiffMSec = TD_OS_ElapsedTimeMSecGet(pConn->timeStartExtCalling);
               if (nDiffMSec >= TIMEOUT_FOR_EXTERNAL_CALL)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pPhone->nSeqConnId,
                     ("Phone No %d: Timeout diff %lu msec for external connection\n"
                      "%selapsed, start to play busy tone.\n",
                      pPhone->nPhoneNumber, nDiffMSec,
                      pPhone->pBoard->pIndentPhone));

                  /* check phone state */
                  if ((pPhone->rgStateMachine[FXS_SM].nState == S_OUT_DIALING) ||
                      (pPhone->rgStateMachine[FXS_SM].nState == S_CF_DIALING))
                  {
                     pPhone->nIntEvent_FXS = IE_EXT_CALL_NO_ANSWER;

                     while (pPhone->nIntEvent_FXS != IE_NONE)
                     {
                        ST_HandleState_FXS(pCtrl, pPhone, pConn);
                     }
                  } /* if */
                  else
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pPhone->nSeqConnId,
                           ("Err, Phone No %d: Timeout for incorrect state: %s\n",
                            (IFX_int32_t) pPhone->nPhoneNumber,
                            Common_Enum2Name(pPhone->rgStateMachine[FXS_SM].nState,
                                             TD_rgStateName)));
                     pConn->bExtDialingIn = IFX_FALSE;
                  } /* check phone state */
               } /* check time difference */
               else
               {
                  /* Check if timeout should be smaller than it is currently set */
                  time_new = TIMEOUT_FOR_EXTERNAL_CALL - nDiffMSec;
                  if (time_new < *pTimeOut)
                  {
                     *pTimeOut = time_new;
                  }
               }
            } /* check connection - if (pConn->... == ...) */
         }/* for all connections */
      }/* for all phones on board */
   } /* for all boards */
   return IFX_SUCCESS;
} /* TAPIDEMO_CheckTimeoutExtCall */

/**
   State machine for PBX - handle events from devices

   \param  pCtrl - pointer to status control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
static IFX_int32_t TAPIDEMO_HandleDeviceThread(CTRL_STATUS_t* pCtrl)
{
   IFX_return_t ret = IFX_SUCCESS;

   BOARD_t* pBoard = IFX_NULL;
   PHONE_t* pPhone = IFX_NULL;
   CONNECTION_t* pConn = IFX_NULL;
   IFX_int32_t nBoard;
   FXO_t* pFXO = IFX_NULL;
   IFX_boolean_t bIsFxo = IFX_FALSE;
   TD_OS_devFd_set_t in_fds, out_fds;
   /* Wait forever by default */
   IFX_time_t nTimeOut = TD_OS_WAIT_FOREVER;
   IFX_int32_t VoiceCallRejectTimer=0;

#if (!defined EASY336 && !defined TAPI_VERSION4)
   IFX_int32_t nDataCh;
   IFX_int32_t nDataChFd;
#endif /* (!defined EASY336 && !defined TAPI_VERSION4) */
#if FAX_MANAGER_ENABLED
   char d;
   int n;
   int i;
   unsigned char data[20];

#endif

   /* check input parameters */
   TD_PTR_CHECK(pCtrl, IFX_ERROR);

   /*****************************
      Prepare structures
    *****************************/
   pCtrl->bInternalEventGenerated = IFX_FALSE;
   /* do not use timeout untill it is needed */
   TD_TIMEOUT_RESET(pCtrl->nUseTimeoutDevice);

   /********************************************************
         Sets the set of file descriptors for select
    ********************************************************/
   if (IFX_ERROR == TAPIDEMO_SetDeviceFdSet(pCtrl, &in_fds))
   {
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to set fd_set for select.on devices.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }


   fcntl(LteFifoFd,F_SETFL,O_NONBLOCK);
   /********************************************************
         Wait for events and handle them
    ********************************************************/
   TD_OS_MutexGet(&pCtrl->oMultithreadCtrl.oMutex.mutexStateMachine);
   while (1)
   {
      /* Check if timeout should be used */
      if (TD_TIMEOUT_CHECK(pCtrl->nUseTimeoutSocket))
      {
         /* There are two threads which are waiting for events from diffrent
            type of sources. By default, they are waiting indefinitely on events.
            It might happen that select on device may receive event which
            requires to activate timeout for select on sockets (e.g. during dialing
            external call). In such situation, the second thread must be waked-up.
            It is done by sending message via pipe. */
         TD_OS_PipePrintf(pCtrl->oMultithreadCtrl.oPipe.oSocketSynch.rgFp[TD_PIPE_IN],
                          "%d", TD_SYNCH_THREAD_MSG_SET_TIMEOUT);
      }

      if (TD_TIMEOUT_CHECK(pCtrl->nUseTimeoutDevice))
      {
         /* Time out used for FXO calls */
         if ((nTimeOut == TD_OS_NO_WAIT) || (nTimeOut == TD_OS_WAIT_FOREVER))
         {
            nTimeOut = SELECT_TIMEOUT_MSEC;
         }
         else
         {
#ifdef FXO
/* start                check timeout for FXO ringing                   start */
            TAPIDEMO_CheckTimeoutFxoCall(pCtrl, &nTimeOut);
/*   end                check timeout for FXO ringing                   end   */
#endif /* FXO */
         }
      }
      else
      {
         /* Select should wait without timeout */
         nTimeOut = TD_OS_WAIT_FOREVER;
      }
      /* Release mutex and wait on new events */
      TD_OS_MutexRelease(&pCtrl->oMultithreadCtrl.oMutex.mutexStateMachine);
      //TD_DEV_FD_SET(LteFifoFd, &in_fds,pCtrl->nMaxFdDevice);
      /* Wait for event or timeout */
      ret = TD_OS_DeviceSelect(LteFifoFd/*pCtrl->nMaxFdDevice*/+1, &in_fds, &out_fds, nTimeOut);

     // printf("Select Exited tapidemo\n");
      TD_OS_MutexGet(&pCtrl->oMultithreadCtrl.oMutex.mutexStateMachine);
      #if 0
      if(TD_OS_DevFdIsSet(LteFifoFd, &in_fds))
      {
    	  printf("Data Came Wooooooooow\n\n\n");
    	  do {
    		  n=read(LteFifoFd,&d,1);
    		  printf("in %c",d);
    	  }while(n>0);

      }
#else
    //  FD_ZERO(&rdset);
      if(TD_OS_DevFdIsSet(LteFifoFd, &in_fds))
       {
    	 // TD_DEV_FD_SET(LteFifoFd, &in_fds, pCtrl->nMaxFdDevice);
    	  //TD_OS_DevFdZero(&in_fds);
    	  memset(data,0,20);
    	  i=0;
    	//  printf("before do \n");
    	  do {
    		  n=read(LteFifoFd,&data[i++],1);
    		  if(i>15)
    			  break;				//Some Errror
    	  }while(n>0);
    	  if(data[0]=='R'&&data[1]=='I'&&data[2]=='N'&&data[3]=='G')
    	  {
        	  incoming_call_flag=1;
        	  printf("Incoming Call Coming\n");
    	  }
    	  else
    	  {
    		  incoming_call_flag=0;
    	  }
       }
      //FD_SET(FaxInfo->modem_fifo_fd, &rdset);
#endif
      /*Got Some event on any of the fds*/
      /*Getting The mutex here*/
     // pCtrl->nBoardCnt=2;
      /* Check if select waked up by event */
      if (0 < ret||incoming_call_flag==1)
      {
         /* for all boards */
         for(nBoard = 0; nBoard < pCtrl->nBoardCnt; nBoard++)
         {
            /* get board */
            pBoard = &pCtrl->rgoBoards[nBoard];
/* start             Handling tapi events on FXS                        start */
            /* if pBoard->nDevCtrl_FD not set skip this part without error */
            if(incoming_call_flag!=1)
            {
            	if (0 > pBoard->nDevCtrl_FD)
            		continue;
            }
            if(incoming_call_flag!=1)
            {
            	/* check device file descriptor */
            	if (!TD_OS_DevFdIsSet(pBoard->nDevCtrl_FD, &out_fds))
            		continue;
            }

            /* get event */
            if (IFX_ERROR == EVENT_Check(pBoard->nDevCtrl_FD, pBoard))
            {
               /* no event for this device */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                     ("Err, Board %s failed to get event. "
                      "(File: %s, line: %d)\n",
                      pBoard->pszBoardName, __FILE__, __LINE__));
              // continue;
            }
            /* Get event owner. Checking if event is dedicated for phone
               or for fxo. There are some situations that data channel
               is used to detect CPT for fxo. In such situation event
               is dedicated for fxo */
            if (IFX_ERROR == EVENT_GetOwnerOfEvent(pBoard, pBoard->nDevCtrl_FD,
                                &pPhone, &pConn, &bIsFxo, &pFXO))
            {
               /* failed to get owner or event doesn't need handling */
               /*TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                     ("Err, Failed to get event owner."
                      " (File: %s, line: %d)\n",
                      __FILE__, __LINE__));*/
                //break;
            }
            if(incoming_call_flag==1)
             {
             	pPhone->nIntEvent_FXS=IE_RINGING;
             	pPhone->rgoConn[0].nType=FXO_CALL;
             	incoming_call_flag=0;
             }

                        /* event for phone channel */
            if (IFX_FALSE == bIsFxo)
            {
               /* check if pointers are set */
               if ((IFX_NULL == pPhone) || (IFX_NULL == pConn))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                        ("Err, Invalid data: phone(%d) and connection(%d)."
                         " (File: %s, line: %d)\n",
                         (IFX_int32_t)pPhone,
                         (IFX_int32_t)pConn, __FILE__, __LINE__));
                  break;
               }
               else
               {
                  /* handle detected event */
                  EVENT_Handle(pPhone, pCtrl, pBoard);
                  /* if internal event generated then handle it
                     (both config and FXS state machine ) */
                  while (IE_NONE != pPhone->nIntEvent_Config)
                  {
                     ST_HandleState_Config(pCtrl, pPhone, pConn);
                  }
                  while (IE_NONE != pPhone->nIntEvent_FXS)
                  {
#if FAX_MANAGER_ENABLED
                	  /*We Need only This Event*/
                	 // printf("In Phone FXS Event\n");
#endif
                     ST_HandleState_FXS(pCtrl, pPhone, pConn);
                  }
               }
            } /* IFX_FALSE == bIsFxo */
            else if (IFX_TRUE == bIsFxo)
            {
               if (IFX_NULL == pFXO)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                        ("Err, Failed to get fxo."
                         " (File: %s, line: %d)\n", __FILE__, __LINE__));
                  break;
               }
#ifdef FXO
#ifdef SLIC121_FXO
               if ((pFXO->pFxoDevice->oTapiEventOnFxo.id &
                    IFX_TAPI_EVENT_TYPE_MASK) ==
                    IFX_TAPI_EVENT_TYPE_FXO)
               {
                  /* fxo event detected on fxo line in SLIC121. */
                  EVENT_FXO_Handle(pFXO, pCtrl, pBoard);
               }
               else
#endif /* SLIC121_FXO */
#endif /* FXO */
               {
                  /* handle event on data channel connected to FXO */
                  EVENT_Handle_DataCH(pFXO->nDataCh, IFX_NULL,
                                      pBoard, pFXO);
               }
            } /* IFX_TRUE == bIsFxo */
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                     ("Err, Unpredicted behaviour of EVENT_GetOwnerOfEvent()."
                      " (File: %s, line: %d)\n", __FILE__, __LINE__));
            }
            /* check if internal event generated */
            if (IFX_TRUE == pCtrl->bInternalEventGenerated)
            {
               EVENT_InternalEventHandle(pCtrl);
            }
         }/* for all boards */
/*   end             Handling tapi events on FXS                        end   */
/* start             Handling tapi events on FXO                        start */
#if !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO)
         /* FXO events handling - check if FXO enabled */
         if (pCtrl->pProgramArg->oArgFlags.nFXO)
         {
            /* get first board */
            pBoard = TD_GET_MAIN_BOARD(pCtrl);
            /* check number of FXOs */
            if (0 < pCtrl->nMaxFxoNumber)
            {
               IFX_int32_t err;
               TAPIDEMO_DEVICE_FXO_t* pFxoDevice = IFX_NULL;

               while ((IFX_SUCCESS == (err = Common_GetNextFxoDevCtrlFd(pCtrl,
                        &pFxoDevice, IFX_TRUE))) &&
                      (IFX_NULL != pFxoDevice) &&
                      (pFxoDevice->nDevFD >= 0))
               {
                  /* check if fd is set */
                  if (!TD_OS_DevFdIsSet(pFxoDevice->nDevFD, &out_fds))
                     continue;

                  /* get FXO event */
                  ret = EVENT_Check(pFxoDevice->nDevFD, pBoard);
                  if (IFX_ERROR == ret)
                  {
                     /* no event for this device */
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                           ("Err, Board %s failed to get event. "
                            "(File: %s, line: %d)\n",
                            pBoard->pszBoardName, __FILE__, __LINE__));
                     continue;
                  }
                  /* get owner of event, here can be only fxo */
                  if (IFX_ERROR == EVENT_GetOwnerOfEvent(pBoard,
                     pFxoDevice->nDevFD, &pPhone, &pConn, &bIsFxo, &pFXO))
                  {
                     /* failed to get fxo or event doesn't need handling */
                     /* TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                           ("Err, Failed to get event owner."
                            " (File: %s, line: %d)\n",
                            __FILE__, __LINE__));*/
                     continue;
                  }
                  if (IFX_TRUE != bIsFxo)
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                           ("Err, Not a FXO event. (File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                     continue;
                  }
                  if (IFX_NULL == pFXO)
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                           ("Err, Failed to Get FXO. (File: %s, line: %d)\n",
                            __FILE__, __LINE__));
                     continue;
                  }
                  /* handle FXO event */
                  if (IFX_SUCCESS == EVENT_FXO_Handle(pFXO, pCtrl, pBoard))
                  {
                     /* EVENT_FXO_Handle() can generate internal events on
                        phones, check if such events are generated
                        and handle them */
                     if (IFX_TRUE == pCtrl->bInternalEventGenerated)
                     {
                        EVENT_InternalEventHandle(pCtrl);
                     }
                  } /* EVENT_FXO_Handle */
               };
               /* check return value, see Common_GetNextFxoDevCtrlFd description
                  for details */
               if (IFX_ERROR == err && IFX_NULL == pFxoDevice)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                     ("Err, Number of FXOs %d but no FXO device FD was found."
                      " (File: %s, line: %d)\n",
                      pCtrl->nMaxFxoNumber, __FILE__, __LINE__));
               }
               else if (IFX_ERROR == err)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_EVT,
                     ("Err, Geting FXO device FD failed. (File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               }
            } /* if (0 < pCtrl->nMaxFxoNumber) */
         } /* if (oCtrlStatus.pProgramArg->oArgFlags.nFXO) */
#endif /* !defined(STREAM_1_1) && !defined(VXWORKS) && defined(FXO) */
/*   end             Handling tapi events on FXO                        end   */

#ifdef TD_TIMER
        /* check for timer events */
        if (TD_OS_DevFdIsSet(pCtrl->nTimerFd, &out_fds))
        {
           IFX_TLIB_CurrTimerExpiry(0);
        }
        /* check FIFO with timer messages */
        if (TD_OS_DevFdIsSet(pCtrl->nTimerMsgFd, &out_fds))
        {
           IFX_TIM_TimerMsgReceived();
        }
#endif /* TD_TIMER */

#ifdef TD_DECT
/* start               DECT event handling                              start */
         /* check if DECT support is on */
         if (!pCtrl->pProgramArg->oArgFlags.nNoDect)
         {
            /* check for paging key events */
            if (TD_OS_DevFdIsSet(pCtrl->viPagingKeyFd, &out_fds))
            {
               TD_DECT_HandlePaging(pCtrl->viPagingKeyFd);
            }
         } /* if (!pCtrl->pProgramArg->oArgFlags.nNoDect) */
/*   end               DECT event handling                              end   */
#endif /* TD_DECT */
/* start      HANDLING voip packets on data channels                    start */
/* packet handling must after event handling otherwise there are errors
   from drivers eg. INFO: Buffer pool growth limit reached */
         /* for all boards */
         for(nBoard = 0; nBoard < pCtrl->nBoardCnt; nBoard++)
         {
            /* get board */
            pBoard = &pCtrl->rgoBoards[nBoard];
#ifdef TAPI_VERSION4
            /* check if single fd is used */
            if (pBoard->fSingleFD)
               continue;
#endif
#if (!defined EASY336 && !defined TAPI_VERSION4)
            /* for all data channels */
            for (nDataCh = 0; nDataCh < pBoard->nMaxCoderCh; nDataCh++)
            {
               /* HANDLING PACKETS FROM DATA CHANNELS */
               /* get data channel fd to check for voice packets */
               nDataChFd = VOIP_GetFD_OfCh(nDataCh, pBoard);
               /* check if valid data chennel fd was returned */
               if (NO_DEVICE_FD == nDataChFd)
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                        ("Err, Board %s Failed to get fd for "
                         "data channel %d. (File: %s, line: %d)\n",
                         pBoard->pszBoardName, nDataCh,
                         __FILE__, __LINE__));
                  break;
               }

               /* check if packets can be read from data channel  */
               if (!TD_OS_DevFdIsSet(nDataChFd, &out_fds))
                  continue;

               /* get connection that uses this data channel */
               if (IFX_SUCCESS == ABSTRACT_GetConnOfDataCh(&pPhone, &pConn,
                                                           pBoard, nDataCh))
               {
                  /* We received data from phone */
                  VOIP_HandleData(pCtrl, pPhone, nDataChFd, pConn,
                                  QOS_HandleService(pCtrl, pConn,
                                                    pPhone->nSeqConnId));
               }
               else
               {
                /*  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                        ("Err, Unable to handle packets on Data channel"
                         "%d - discard data. (File: %s, line: %d)\n",
                         nDataCh, __FILE__, __LINE__)); */
                  /* because connection is not stopped at the same time on
                     both phones, packet from ended connection can
                     be still received and must be discarded */
                  VOIP_DiscardData(nDataChFd);
               }
            } /* for all data channels */
#endif /* (!defined EASY336 && !defined TAPI_VERSION4) */
/*   end      HANDLING voip packets on data channels and sockets        end   */
         }/* for all boards */
      } /* if (0 < ret)*/
      /* check if select waked up by timeot */
      else if (0 == ret)
      {
#ifdef FXO
         TAPIDEMO_CheckTimeoutFxoCall(pCtrl, &nTimeOut);
#endif /* FXO */
      } /* ret of select() */
      /* if ret < 0 select returned error */
      else if (0 > ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
              ("Err, in select for devices. (File: %s, line: %d)\n",
              __FILE__, __LINE__));
         //break;
      }
   } /* while */
   /*Releasing the Mutex here*/
   /* We leave loop, so we don't need keep mutex anymore */
   TD_OS_MutexRelease(&pCtrl->oMultithreadCtrl.oMutex.mutexStateMachine);

   return IFX_SUCCESS;
} /* TAPIDEMO_HandleDeviceThread() */

/**
   State machine for PBX - handle events from sockets

   \param  pThread - pointer to thread control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
static IFX_int32_t TAPIDEMO_HandleSocketThread(TD_OS_ThreadParams_t *pThread)
{
   IFX_return_t ret = IFX_SUCCESS;
   BOARD_t* pBoard = IFX_NULL;
   CTRL_STATUS_t* pCtrl;
   IFX_int32_t nBoard;
   TD_OS_socFd_set_t rfds, trfds;
   /* Wait forever by default */
   IFX_time_t nTimeOut = TD_OS_SOC_WAIT_FOREVER;
   /* Read data from thread synch pipe */
   IFX_char_t pBuffer[2] = {0};
   TD_SYNCH_THREAD_MSG_t oSychThreadMsg;

#if (!defined EASY336 && !defined TAPI_VERSION4)
   CONNECTION_t* pConn = IFX_NULL;
   PHONE_t* pPhone = IFX_NULL;
   IFX_int32_t nDataCh;
   IFX_int32_t nSocket = NO_SOCKET;
#endif /* (!defined EASY336 && !defined TAPI_VERSION4) */

   pCtrl = &oCtrlStatus;
   /* Block all signals on this thread */
   if (IFX_SUCCESS != TAPIDEMO_BlockSignals())
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Can't block signals for  socket thread.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /*****************************
      Prepare structures
    *****************************/
   pCtrl->bInternalEventGenerated = IFX_FALSE;

   /********************************************************
         Sets the set of file descriptors for select
    ********************************************************/
   if (IFX_SUCCESS != TAPIDEMO_SetSocketFdSet(pCtrl, &rfds))
   {
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to set fd_set for select on sockets.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* This lock is used for synchronization with the main thread and it's used
      when Tapidemo is closed down.
      When lock is released it means thread stops working. */
   TD_OS_LockGet(&pCtrl->oMultithreadCtrl.oLock.oSocketThreadStopLock);

   /********************************************************
         Wait for events and handle them
    ********************************************************/
   TD_OS_MutexGet(&pCtrl->oMultithreadCtrl.oMutex.mutexStateMachine);
   while (1)
   {
      /* check if timeout should be used */
      if (TD_TIMEOUT_CHECK(pCtrl->nUseTimeoutSocket))
      {
         /* set time out used for external call dialing (for dialing timeout). */
         if ((nTimeOut == TD_OS_SOC_NO_WAIT) ||
             (nTimeOut == TD_OS_SOC_WAIT_FOREVER))
         {
            /* Timeout is not set. Set default one */
            nTimeOut = SELECT_TIMEOUT_MSEC;
         }
         else
         {
            /* Check if timeout is set correctly */
            TAPIDEMO_CheckTimeoutExtCall(pCtrl, &nTimeOut);
         }
#ifdef TD_DECT
         if (!pCtrl->pProgramArg->oArgFlags.nNoDect)
         {
            TD_DECT_DialtoneTimeoutSelectSet(pCtrl, &nTimeOut);
         } /* if (!pCtrl->pProgramArg->oArgFlags.nNoDect) */
#endif /* TD_DECT */
      }
      else
      {
         /* select should wait without timeout */
         nTimeOut = TD_OS_SOC_WAIT_FOREVER;
      }

      /* if request to change IP address via test scripts */
      COM_IF_COMMUNICATION_SOCKETS_RESET(pCtrl, rfds);

      /* Release mutex and wait on new events */
      TD_OS_MutexRelease(&pCtrl->oMultithreadCtrl.oMutex.mutexStateMachine);

      /* Update the local file descriptor by the copy in the task parameter */
      memcpy((void *) &trfds, (void*) &rfds, sizeof(trfds));
      /* Wait for event or timeout */




      ret = TD_OS_SocketSelect( pCtrl->nMaxFdSocket + 1, &trfds, IFX_NULL,
                               IFX_NULL, nTimeOut);
#
      TD_OS_MutexGet(&pCtrl->oMultithreadCtrl.oMutex.mutexStateMachine);
      /* check if select waked up by event */
      if (0 < ret)
      {
/* start         HANDLING events from administration sockets            start */
         /* Check socket for handling calls between boards */
         if (TD_OS_SocFdIsSet(pCtrl->nAdminSocket, &trfds))
         {
            /* Got something on admin socket
               (actions sent by TAPIDEMO_SetAction()) */
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_MSG,
               ("New action on ADMIN socket\n"));
            TAPIDEMO_HandleCallSeq(pCtrl, pCtrl->nAdminSocket);
            /* check if internal event generated */
            if (IFX_TRUE == pCtrl->bInternalEventGenerated)
            {
               EVENT_InternalEventHandle(pCtrl);
            }
         }
#ifdef TD_IPV6_SUPPORT
         if (oCtrlStatus.pProgramArg->oArgFlags.nUseIPv6)
         {
            /* Check socket for handling calls between boards */
            if (TD_OS_SocFdIsSet(pCtrl->oIPv6_Ctrl.nSocketFd, &trfds))
            {
               /* Got something on admin socket
                  (actions sent by TAPIDEMO_SetAction()) */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_MSG,
                  ("New action on IPv6 ADMIN socket\n"));
               TAPIDEMO_HandleCallSeq(pCtrl, pCtrl->oIPv6_Ctrl.nSocketFd);
               /* check if internal event generated */
               if (IFX_TRUE == pCtrl->bInternalEventGenerated)
               {
                  EVENT_InternalEventHandle(pCtrl);
               }
            }
         }
#endif /* TD_IPV6_SUPPORT */

#ifdef TD_IPV6_SUPPORT
         /* Check broadcast socket used for phone book */
         if (TD_OS_SocFdIsSet(pCtrl->rgoCast[TD_CAST_BROAD].nSocket, &trfds))
         {
            TAPIDEMO_HandleCallSeq(pCtrl,
                                   pCtrl->rgoCast[TD_CAST_BROAD].nSocket);
         }
         /* check if IPv6 support is on */
         if (pCtrl->pProgramArg->oArgFlags.nUseIPv6)
         {
            /* Check broadcast socket used for phone book */
            if (TD_OS_SocFdIsSet(pCtrl->rgoCast[TD_CAST_MULTI].nSocket, &trfds))
            {
               TAPIDEMO_HandleCallSeq(pCtrl,
                                      pCtrl->rgoCast[TD_CAST_MULTI].nSocket);
            }
         }
#endif /* TD_IPV6_SUPPORT */
/*   end         HANDLING events from administration socket             end   */
/* start         HANDLING events from thread synch pipe                 start */
         if (TD_OS_SocFdIsSet(
            pCtrl->oMultithreadCtrl.oPipe.oSocketSynch.rgFd[TD_PIPE_OUT],
            &trfds))
         {
            /* Clean up buffer */
            memset(&pBuffer, 0, sizeof(pBuffer));
            ret = TD_OS_PipeRead(pBuffer, sizeof(pBuffer)-1, 1,
                                 pCtrl->oMultithreadCtrl.oPipe.oSocketSynch.rgFp[TD_PIPE_OUT]);
            if (Common_IsNumber(pBuffer))
            {
               oSychThreadMsg = atoi(pBuffer);
               if (TD_SYNCH_THREAD_MSG_SET_TIMEOUT == oSychThreadMsg)
               {
                  /* In order to set new timeout for this thread,
                     we need to wake-up thread. It is done by receiving message from
                     another thread. The thread is wake-up, so we don't need to do
                     anything more */
               }
               else if (TD_SYNCH_THREAD_MSG_STOP == oSychThreadMsg)
               {
                  /* Receive msg from another thread to stop working. Exit thread */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DBG,
                        ("Receive message from synchronization pipe: stop thread.\n"));
                  break;
               }
               else
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DBG,
                        ("Receive unknown message from synchronization pipe: %s\n",
                        pBuffer));
               }
            }
            else
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_DBG,
                     ("Receive unknown message from synchronization pipe: %s\n",
                     pBuffer));
            }
         }
/*   end         HANDLING events from thread synch pipe                 end   */

/* start       HANDLING events from ITM communication socket            start */
         /* ITM message handling e.g. choosing test */
         if (IFX_TRUE == g_pITM_Ctrl->fITM_Enabled)
         {
            /* check if socket available */
            if(NO_SOCKET != g_pITM_Ctrl->nComInSocket)
            {
               /* Check if there is a command pending on communication socket */
               if (TD_OS_SocFdIsSet(g_pITM_Ctrl->nComInSocket, &trfds))
               {
                  /* Receive command */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                        ("ITM: Command on communication socket.\n"));
                  COM_RECEIVE_COMMAND(pCtrl, IFX_FALSE, ret);
               }
            } /* if(NO_SOCKET != pCtrl->nComInSocket) */

            /* check if socket available */
            if(NO_SOCKET != g_pITM_Ctrl->oBroadCast.nSocket)
            {
               /* Check if there is a command pending on broadcast socket */
               if (TD_OS_SocFdIsSet(g_pITM_Ctrl->oBroadCast.nSocket, &trfds))
               {
                  /* Receive command */
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_ITM,
                        ("ITM: Command on broadcast socket.\n"));
                  COM_RECEIVE_COMMAND(pCtrl, IFX_TRUE, ret);
               }
            } /* if(NO_SOCKET != g_pITM_Ctrl->oBroadCast.nSocket) */
         } /* if (IFX_TRUE == g_pITM_Ctrl->fITM_Enabled) */
/*   end       HANDLING events from ITM communication socket            end   */
#ifdef TD_FAX_MODEM
#if (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
         /* Add communication socket for handling PC's requests */
         if(NO_SOCKET != pCtrl->oFaxTestCtrl.nComSocket)
         {
            /* Check if there is a command pending on communication socket */
            if (TD_OS_SocFdIsSet(pCtrl->oFaxTestCtrl.nComSocket, &trfds))
            {
               /* Receive command */
               /*TRACE(TAPIDEMO, DBG_LEVEL_LOW,
                     ("Command on fax test communication socket.\n"));*/
               TD_T38_FAX_TEST_HandleSocketData(pCtrl);
            }
         }
#endif /* (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
#endif /* TD_FAX_MODEM */

/* start      HANDLING voip packets on sockets        start */
         /* for all boards */
         for(nBoard = 0; nBoard < pCtrl->nBoardCnt; nBoard++)
         {
            /* get board */
            pBoard = &pCtrl->rgoBoards[nBoard];
#ifdef TAPI_VERSION4
            /* check if single fd is used */
            if (!pBoard->fSingleFD)
#endif
            {
#if (!defined EASY336 && !defined TAPI_VERSION4)
               /* for all data channels */
               for (nDataCh = 0; nDataCh < pBoard->nMaxCoderCh; nDataCh++)
               {
                  /* HANDLING PACKETS FROM SOCKETS */
                  /* check if QoS is used */
                  if (!pCtrl->pProgramArg->oArgFlags.nQos)
                  {
                     /* get socket */
                     nSocket = VOIP_GetSocket(nDataCh, pBoard, IFX_FALSE);
                     /* check if valid data chennel fd was returned */
                     if (NO_SOCKET == nSocket)
                     {
                        TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                              ("Err, Board %s Failed to get socket for "
                               "data channel %d. (File: %s, line: %d)\n",
                               pBoard->pszBoardName, nDataCh,
                               __FILE__, __LINE__));
                     }
                     /* check if packets can be read from socket */
                     if (TD_OS_SocFdIsSet(nSocket, &trfds))
                     {
                        pPhone = ABSTRACT_GetPHONE_OfSocket(nSocket, &pConn, pBoard);
                        if (IFX_NULL == pPhone || IFX_NULL == pConn)
                        {
                         /*  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                                 ("Err, %s not found for socket %d. "
                                  "(File: %s, line: %d)\n",
                                  IFX_NULL == pPhone ? "Phone" : "Connection",
                                  nSocket, __FILE__, __LINE__));*/
                           /* because connection is not stopped at the same time
                              on both phones, packet from ended connection can
                              be still received and must be discarded */
                           VOIP_DiscardData(nSocket);
                        }
                        else
                        {
                           /* We received data over socket, external phone */
                           VOIP_HandleSocketData(pPhone, pConn,
                                                 QOS_HandleService(pCtrl, pConn,
                                                    pPhone->nSeqConnId));
                        }
                     } /* check if socket is readable*/
#ifdef TD_IPV6_SUPPORT
                     if (pCtrl->pProgramArg->oArgFlags.nUseIPv6)
                     {
                        /* get socket */
                        nSocket = VOIP_GetSocket(nDataCh, pBoard, IFX_TRUE);
                        /* check if valid data chennel fd was returned */
                        if (NO_SOCKET == nSocket)
                        {
                           TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                                 ("Err, Board %s Failed to get socket for "
                                  "data channel %d. (File: %s, line: %d)\n",
                                  pBoard->pszBoardName, nDataCh,
                                  __FILE__, __LINE__));
                        }
                        /* check if packets can be read from socket */
                        if (TD_OS_SocFdIsSet(nSocket, &trfds))
                        {
                           pPhone = ABSTRACT_GetPHONE_OfSocket(nSocket, &pConn, pBoard);
                           if (IFX_NULL == pPhone || IFX_NULL == pConn)
                           {
                            /*  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                                    ("Err, %s not found for socket %d. "
                                     "(File: %s, line: %d)\n",
                                     IFX_NULL == pPhone ? "Phone" : "Connection",
                                     nSocket, __FILE__, __LINE__));*/
                              /* because connection is not stopped at the same time
                                 on both phones, packet from ended connection can
                                 be still received and must be discarded */
                              VOIP_DiscardData(nSocket);
                           }
                           else
                           {
                              /* We received data over socket, external phone */
                              VOIP_HandleSocketData(pPhone, pConn,
                                                    QOS_HandleService(pCtrl, pConn,
                                                       pPhone->nSeqConnId));
                           }
                        } /* check if socket is readable*/
                     }
#endif /* TD_IPV6_SUPPORT */
                  } /* check if QoS is used */
               } /* for all data channels */
#endif /* (!defined EASY336 && !defined TAPI_VERSION4) */
            } /* if (!pBoard->fSingleFD) */
         } /* for all boards */
/*   end      HANDLING voip packets on data channels and sockets        end   */

#ifdef TD_DECT
/* start               DECT event handling                              start */
         /* check if DECT support is on */
         if (!pCtrl->pProgramArg->oArgFlags.nNoDect)
         {
            /* check if message received */
            if (TD_OS_SocFdIsSet(pCtrl->nDectPipeFdOut, &trfds))
            {
               pPhone = IFX_NULL;
               pConn = IFX_NULL;
               /* check if there is an event to handle */
               while (IFX_SUCCESS == TD_DECT_EventHandle(
                       pCtrl->oMultithreadCtrl.oPipe.oDect.rgFp[TD_PIPE_OUT],
                       &pCtrl->rgoBoards[0], &pPhone, &pConn))
               {
                  while (IE_NONE != pPhone->nIntEvent_Config)
                  {
                     ST_HandleState_Config(pCtrl, pPhone, pConn);
                  }
                  while (IE_NONE != pPhone->nIntEvent_FXS)
                  {
                     ST_HandleState_FXS(pCtrl, pPhone, pConn);
                  }
                  if (IFX_TRUE == pCtrl->bInternalEventGenerated)
                  {
                     EVENT_InternalEventHandle(pCtrl);
                  }
               }
            }
            /* check for message on DECT debug module socket */
            if (TD_OS_SocFdIsSet(pCtrl->viFromCliFd, &trfds))
            {
               TD_DECT_HandleCliCmd(pCtrl->viFromCliFd, pCtrl);
            }
         } /* if (!pCtrl->pProgramArg->oArgFlags.nNoDect) */
/*   end               DECT event handling                              end   */
#endif /* TD_DECT */
      } /* if (0 < ret)*/
      /* check if select waked up by timeot */
      else if (0 == ret)
      {
#ifdef TD_IPV6_SUPPORT
         if(!pCtrl->bPhonebookShow)
         {
            /* Tapidemo doesn't print phonebook until the first timeout occurs */
            pCtrl->bPhonebookShow = IFX_TRUE;
            Common_PhonebookShow(pCtrl->rgoPhonebook);
            TD_TIMEOUT_STOP(pCtrl->nUseTimeoutSocket);
         } /* if(!pCtrl->bPhonebookShow) */
#endif /* TD_IPV6_SUPPORT */
/* start            check timeout for external call dialing             start */
         TAPIDEMO_CheckTimeoutExtCall(pCtrl, &nTimeOut);
/* stop             check timeout for external call dialing              stop */
#ifdef TD_DECT
/* start                check timeout for DECT dialtone                 start */
      /* check if DECT is enabled */
      if (!pCtrl->pProgramArg->oArgFlags.nNoDect)
      {
         /* check for timeout for DECT phones and handle it if needed */
         if (IFX_SUCCESS !=
             TD_DECT_DialtoneTimeoutSelectSet(pCtrl, &nTimeOut))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, TD_DECT_DialtoneTimeoutSelectSet() failed.\n"
                   "(File: %s, line: %d)\n",
                   __FILE__, __LINE__));
         }
      } /* if (!pCtrl->pProgramArg->oArgFlags.nNoDect) */
/* end                  check timeout for DECT dialtone                   end */
#endif /* TD_DECT */
      }
      /* if ret < 0 select returned error */
      else if (0 > ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
              ("Err, in select for sockets. (File: %s, line: %d)\n",
              __FILE__, __LINE__));
         break;
      }
   } /* while(1) */

   /* We leave loop, so we don't need keep mutex anymore */
   TD_OS_MutexRelease(&pCtrl->oMultithreadCtrl.oMutex.mutexStateMachine);

   /* Release lock, which means this thread stops working */
   TD_OS_LockRelease(&pCtrl->oMultithreadCtrl.oLock.oSocketThreadStopLock);

   return IFX_SUCCESS;
} /* TAPIDEMO_HandleSocketThread */


#ifdef LINUX
/**
   Run logs' redirection - read data from pipe and redirect them.

   \param  pThread - pointer to thread control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
static IFX_int32_t TAPIDEMO_HandleLogThread(TD_OS_ThreadParams_t *pThread)
{
   /* Read data from pipe and redirect them. */
   IFX_char_t pBuffer[MAX_LOG_LEN] = {0};
   IFX_int32_t nMaxFdSocket = 0;

   /* Read data from thread synch pipe */
   TD_SYNCH_THREAD_MSG_t oSychThreadMsg;

   CTRL_STATUS_t* pCtrl;
   IFX_return_t ret = IFX_SUCCESS;
   fd_set rfds, trfds;

   pCtrl = &oCtrlStatus;

   /* Block all signals on this thread */
   if (IFX_SUCCESS != TAPIDEMO_BlockSignals())
   {
       TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
             ("Err, Can't block signals for redirect log thread.\n"
              "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* This lock is used for synchronization with the main thread and it's used
      when Tapidemo is closed down.
      When lock is released it means thread stops working. */
   TD_OS_LockGet(&pCtrl->oMultithreadCtrl.oLock.oLogThreadStopLock);

   /********************************************************
         Sets the set of file descriptors for select
    ********************************************************/
   TD_OS_SocFdZero(&rfds);
   if (NO_PIPE == pCtrl->oMultithreadCtrl.oPipe.oLogSynch.rgFd[TD_PIPE_OUT])
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Couldn't get thread synch pipe.\n"
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));

      /* Release lock, which means this thread stops working */
      TD_OS_LockRelease(&pCtrl->oMultithreadCtrl.oLock.oLogThreadStopLock);
      return IFX_ERROR;
   }
   TD_SOC_FD_SET(pCtrl->oMultithreadCtrl.oPipe.oLogSynch.rgFd[TD_PIPE_OUT],
      &rfds, nMaxFdSocket);

   if (NO_PIPE == pCtrl->oMultithreadCtrl.oPipe.oRedirectLog.rgFd[TD_PIPE_OUT])
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Couldn't get redirect log pipe.\n"
             "(File: %s, line: %d)\n",
             __FILE__, __LINE__));

      /* Release lock, which means this thread stops working */
      TD_OS_LockRelease(&pCtrl->oMultithreadCtrl.oLock.oLogThreadStopLock);
      return IFX_ERROR;
   }
   TD_SOC_FD_SET(pCtrl->oMultithreadCtrl.oPipe.oRedirectLog.rgFd[TD_PIPE_OUT],
      &rfds, nMaxFdSocket);

   /********************************************************
         Wait for logs and handle them
    ********************************************************/
   while(1)
   {
      /* Update the local file descriptor by the copy in the task parameter */
      memcpy((void *) &trfds, (void*) &rfds, sizeof(trfds));
      ret = TD_OS_SocketSelect(nMaxFdSocket+1, &trfds, IFX_NULL, IFX_NULL,
                               TD_OS_WAIT_FOREVER);
      if (ret > 0)
      {
/* start         HANDLING events from thread synch pipe                 start */
         if (TD_OS_SocFdIsSet(
            pCtrl->oMultithreadCtrl.oPipe.oLogSynch.rgFd[TD_PIPE_OUT],
            &trfds))
         {
            TD_OS_PipeRead(pBuffer, MAX_LOG_LEN - 1, 1,
                           pCtrl->oMultithreadCtrl.oPipe.oLogSynch.rgFp[TD_PIPE_OUT]);
            if (Common_IsNumber(pBuffer))
            {
               oSychThreadMsg = atoi(pBuffer);
               if (TD_SYNCH_THREAD_MSG_SET_TIMEOUT == oSychThreadMsg)
               {
                  /* In order to set new timeout for this thread,
                     we need to wake-up thread. It is done by receiving message from
                     another thread. The thread is wake-up, so we don't need to do
                     anything more */
               }
               else if (TD_SYNCH_THREAD_MSG_STOP == oSychThreadMsg)
               {
                  /* Receive msg from another thread to stop working. Exit thread */
                  break;
               }
            }
            else
            {
               /* Receive unknown message from synchronization pipe */
               break;
            }
         }
/*   end         HANDLING events from thread synch pipe                 end   */
/* start                       HANDLING logs                            start */
         if (TD_OS_SocFdIsSet(
            pCtrl->oMultithreadCtrl.oPipe.oRedirectLog.rgFd[TD_PIPE_OUT],
            &trfds))
         {
            TD_OS_PipeRead(pBuffer, MAX_LOG_LEN - 1, 1,
                           pCtrl->oMultithreadCtrl.oPipe.oRedirectLog.rgFp[TD_PIPE_OUT]);
            /* Check how many bytes were read */
            if (0 >= strlen(pBuffer))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                     ("Message read error. (File: %s, line: %d)\n",
                      __FILE__, __LINE__));
               break;
            }
            else if (MAX_LOG_LEN < strlen(pBuffer))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
                     ("Message too long. (File: %s, line: %d)\n",
                      __FILE__, __LINE__));
            }
            else
            {
               /* Split and send log to new destination */
               TAPIDEMO_SplitAndSendLog(pBuffer);
            } /* if (0 > bytes_read) */
         } /* if(TD_OS_SocFdIsSet(nPipeFd, &fds)) */
         /* Clean up buffer */
         memset(&pBuffer, 0, MAX_LOG_LEN);
/* stop                        HANDLING logs                            stop */
      }
      else if (0 == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
              ("Timeout on select for logs (File: %s, line: %d)\n",
              __FILE__, __LINE__));
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
               ("Err, in select for logs. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         break;
      } /* if (ret > 0) */
   } /* while(1) */

   /* Release lock, which means this thread stops working */
   TD_OS_LockRelease(&pCtrl->oMultithreadCtrl.oLock.oLogThreadStopLock);

   /* End with task */
   return IFX_SUCCESS;
} /* TAPIDEMO_HandleLogThread */
#endif /* LINUX */

#if 1 /* VVDN_TEST */
IFX_return_t VVDN_PCM_Tone_Define(int fd) /* edk */
{
   IFX_TAPI_TONE_t tone;
   /* Open tapi for SVIP */
   memset(&tone, 0, sizeof(IFX_TAPI_TONE_t));
   /* define a simple tone for tone table index 71 */
   tone.simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
   /* tone table index where the tone is to be inserted */
   tone.simple.index = 71;
   /* using two frequencies */
   /* 0 <= till < 4000 Hz */
   tone.simple.freqA = 480;
   tone.simple.freqB = 620;
   /* tone level for freqA */
   /* -300 < till < 0 */
   tone.simple.levelA = -15;
   /* tone level for freqB */
   tone.simple.levelB = -20;
   /* program first cadences (on time) */
   tone.simple.cadence[0] = 2000;
   /* program second cadences (off time) */
   tone.simple.cadence[1] = 2000;
   /* in the first cadence, both frequencies must be played */
   tone.simple.frequencies[0] = IFX_TAPI_TONE_FREQA | IFX_TAPI_TONE_FREQB;
   /* in the second cadence, all frequencies are off */
   tone.simple.frequencies[1] = IFX_TAPI_TONE_FREQNONE;
   /* the tone is to be played two hundred times (200 loops) */
   tone.simple.loop = 200;
   /* at the end of each loop there is a pause */
   tone.simple.pause = 100;
   /* update the tone table with the simple tone */
   if (TD_IOCTL(fd, IFX_TAPI_TONE_TABLE_CFG_SET,
         (IFX_int32_t) &tone, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
       printf("\n IFX_TAPI_TONE_TABLE_CFG_SET:  FAILED to add tone index 71 \n");
       return IFX_ERROR;
   }else
       printf("\n Successfully added tone index 71 \n");
   /* now the simple tone is in the tone table at index 71 */

}

IFX_return_t VVDN_PCM_Playtone(fd) 
{
   IFX_TAPI_TONE_PLAY_t tone;
   memset (&tone, 0, sizeof (IFX_TAPI_TONE_PLAY_t));
   /* Start playing one simple tone, defined with tone table index 71 */
   /* Play the tone on the ALM module with direction "external" towards the
   analog line. */
   tone.dev = 0;
   tone.ch = 0;
   tone.external = 1;
   tone.internal = 1;
   tone.module = IFX_TAPI_MODULE_TYPE_PCM;
   tone.index = 71; /* simple tone */
   if (TD_IOCTL(fd, IFX_TAPI_TONE_PLAY,
         (IFX_int32_t) &tone, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      printf("\n *** ERROR !!! Playing tone out to PCM channel 0 \n");
      return IFX_ERROR;
   }else
      printf("\n *** Playing tone out to PCM channel 0 \n");

#if 0
/* play also on FXS port */
   sleep(3);
   tone.ch = 0;
   tone.module = IFX_TAPI_MODULE_TYPE_ALM;
   tone.index = 71; /* simple tone */
   if (VOS_Ioctl_Device (fd, IFX_TAPI_TONE_PLAY, (IFX_int32_t) &tone) != IFX_SUCCESS)
   {
      printf("\n *** ERROR !!! Playing tone out to ALM channel 0 \n");
      return IFX_ERROR;
   }else
      printf("\n *** Playing tone out to ALM channel 0 \n");
#endif
   return IFX_SUCCESS;
}

#endif
/**
   Main function.

   \param argc - number of arguments
   \param argv - array of arguments

   \return 0 or err code
*/
#if !defined (TAPIDEMO_LIBRARY)
#if defined(LINUX)

int main(int argc, char *argv [])
{

#else /* defined(LINUX) */

IFX_int32_t tapidemo(char* cmd_args)
{
   IFX_int32_t argc = 0;
   IFX_char_t** argv = IFX_NULL;

#endif /* defined(LINUX) */
#else /* defined (TAPIDEMO_LIBRARY) */

int tapidemo(int argc, char *argv [])
{

#endif /* defined (TAPIDEMO_LIBRARY) */
#if FAX_MANAGER_ENABLED
	FAXEvent_t FaxEvent;
#endif
	IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   IFX_int32_t j = 0;
   IFX_int32_t phone_num = 0;
   IFX_int32_t tmp_coder_sum = 0;
   IFX_boolean_t check_arg = IFX_FALSE;
#ifdef TAPI_VERSION4
   IFX_boolean_t fUseSockets;
#endif /* TAPI_VERSION4 */
   BOARD_t* pBoard = IFX_NULL;
   IFX_int32_t nPreviousBoardCnt;
#ifdef EVENT_LOGGER_DEBUG
   EL_IoctlRegister_t stReg;
   IFX_int32_t nRet;
#ifdef LINUX
   IFX_char_t *pCmd ="echo \"tapidemo\" > /proc/driver/el/logs/user_str";
   IFX_char_t rgCmdOutput[MAX_CMD_LEN];
#endif /* LINUX */
#endif /* EVENT_LOGGER_DEBUG */

   /* Reset parameters */
   memset((IFX_uint8_t *) &oProgramArg, 0, sizeof(PROGRAM_ARG_t));
   /* Reset control status */
   memset(&oCtrlStatus, 0, sizeof(CTRL_STATUS_t));
   /* Set all variables and all used structures to zero. */
   memset(&oTraceRedirection, 0, sizeof(TRACE_REDIRECTION_t));

   /* set pointer to program arguments structure */
   oCtrlStatus.pProgramArg = &oProgramArg;

   /* set ITM default */
   COM_SET_DEFAULT(&oCtrlStatus.oITM_Ctrl, argc, argv,
                  TAPIDEMO_VERSION, ITM_VERSION);

#if defined(VXWORKS)
   /* Set application priority level */
   taskPrioritySet((int) IFX_NULL, (int) 70);

   /* Get TID of application */
   nTapiDemo_TID = taskIdSelf();

   /* Read arguments and setup application accordingly */
   if ((cmd_args != IFX_NULL) && (strlen(cmd_args) > 0))
   {
      argc = GetArgCnt(cmd_args, GetCmdArgsArr());
      argv = GetCmdArgsArr();
      check_arg = IFX_TRUE;
   }
#elif LINUX
   if (argc > 1)
   {
      check_arg = IFX_TRUE;
   }
#endif /* VXWORKS */

   /* Set some default arguments */
   if (IFX_SUCCESS != TAPIDEMO_SetDefault(&oCtrlStatus))
   {
      /* Can't set default configuration */
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("Setting default configuration failed.\nFile: %s, line: %d\n",
          __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (check_arg)
   {
      if (ReadOptions(argc, argv, oCtrlStatus.pProgramArg) != IFX_SUCCESS)
      {
         /* Wrong arguments */
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            (" Wrong command line argument(s).\n"));
         return IFX_ERROR;
      }
   }

#ifdef EVENT_LOGGER_DEBUG
   if (g_bUseEL == IFX_TRUE)
   {
      TD_EventLoggerFd = Common_Open(EL_DEVICE_NAME, IFX_NULL);
      if (TD_EventLoggerFd < 0)
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("Error, Open of %s device failed. No trace redirection to "
             "Event Logger.\nFile: %s, line: %d\n",
             EL_DEVICE_NAME, __FILE__, __LINE__));
         g_bUseEL = IFX_FALSE;
      }
      else
      {
         strcpy(stReg.sName, "tapidemo");
         stReg.nType = IFX_TAPI_DEV_TYPE_VOICESUB_GW;
         stReg.nDevNum = 0;
         
         nRet = ioctl(TD_EventLoggerFd, EL_REGISTER, (IFX_int32_t)&stReg); 
         if (nRet != IFX_SUCCESS)
         {
            TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
               ("Error, Registration in Event Logger failed. No trace "
                "redirection to Event Logger \n"
                "File: %s, line: %d\n",
                __FILE__, __LINE__));
            g_bUseEL = IFX_FALSE;
         }
      }
   }
#ifdef LINUX
   /* enable 'user_str' log type in event logger. */
   if (g_bUseEL == IFX_TRUE)
   {
      if (IFX_SUCCESS == COM_ExecuteShellCommand(pCmd, rgCmdOutput))
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("Called %s.\n", pCmd));
         /* delay necessary - without it Event Logger will lose few first logs.*/
         TD_OS_MSecSleep(200);
      }
      else
      {
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            ("Err, COM_ExecuteShellCommand(%s) failed\n"
             "(File: %s, line: %d)\n",
             pCmd, __FILE__, __LINE__));
      }
   }
#endif /* LINUX */
#endif /* EVENT_LOGGER_DEBUG */

/* ------ Start Prepare trace redirection and run application as a daemon ------- */
#ifdef LINUX
   if (oProgramArg.nTraceRedirection != TRACE_REDIRECTION_NONE)
   {
      /* Set up log redirection - e.g. sockets, syslog or open file */
      if (IFX_SUCCESS != TAPIDEMO_PreapareLogRedirection())
      {
         /* Can't start redirecting logs */
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            (" Can not start redirecting logs.\n"));
         return IFX_ERROR;
      }
   } /* trace redirection */

   if (oProgramArg.oArgFlags.nDaemon)
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
         ("....Tapidemo runs as a daemon....\n\n"));
      /* Create process responsible for log redirection and TAPIDEMO daemon */
      if (TAPIDEMO_CreateDaemon(&oCtrlStatus) != IFX_SUCCESS)
      {
         /* Can't create daemon */
         TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
            (" Can not run Tapidemo as a daemon.\n"));
         return IFX_ERROR;
      }
   } /* daemon */
#endif /* defined(LINUX) */
/* ------ Stop Prepare trace redirection and run application as a daemon ------- */

/* ------ Start Handle program argument --help, --version --configure ------- */
   if (oCtrlStatus.pProgramArg->oArgFlags.nHelp)
   {
      return ProgramHelp();
   } /* program help */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("TAPIDEMO version %s\n", TAPIDEMO_VERSION));

   if (oCtrlStatus.pProgramArg->oArgFlags.nVersion)
   {
      /* end program after printing version of application */
      return IFX_SUCCESS;
   } /* program version */

#ifdef LINUX
   if (oCtrlStatus.pProgramArg->oArgFlags.nConfigure)
   {
      /* TAPIDEMO_CONFIGURE_STR - can be quite long > 2000.
         Hers printf is used instead of TD_TRACE,
         as TD_TRACE macro uses buffers with limited size */
      TD_PRINTF("TAPIDEMO configuration:\n %s\n", TAPIDEMO_CONFIGURE_STR);
      return IFX_SUCCESS;
   } /* program configuration */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
      ("ITM version %s.\n", ITM_VERSION));
#endif /* LINUX */

#ifdef TD_FAX_MODEM
#if (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /* FAX_TEST version */
   TD_T38_FAX_TEST_PrintVersion();
#endif /* (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
#endif /* TD_FAX_MODEM */

/* ------ Stop Handle program argument --help, --version --configure -------- */

/* ----- Start Detect and register the main board and extension board(s) ---- */

   /* Set main board */
   if (IFX_SUCCESS != TAPIDEMO_SetMainBoard(&oCtrlStatus.rgoBoards[0]))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Set main board. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Check if Tapidemo should detect any extension board */
   if (oCtrlStatus.pProgramArg->oArgFlags.nUseCombinationBoard)
   {
      /* Program argument '-b' is used. It might be used in two situation:
         - Run Tapidemo only for the main board. In this case,
           disable detection of extension boards. Use: '-b 0'
         - Sets board combination - obsolete feature. In this case,
           don't autodetect all extension boards. Detect specific
           extension board based on board combination. Use: -b num */

      if (NO_EXT_BOARD != oCtrlStatus.pProgramArg->nBoardComb)
      {
         /* Second situation: '-b num' (where num > 0) is used
            to set board combination */
         /* Check backward compatibility for flag -b */
         if (IFX_SUCCESS != TAPIDEMO_CheckBoardCombination(&oCtrlStatus.rgoBoards[1]))
         {
            return IFX_ERROR;
         }
      } /* if */
   }
   else
   {
      /* Autodetect all extension boards. */
      /* Max number of extension board is limited by constant MAX_BOARDS */
      for (i=1; i < MAX_BOARDS; i++)
      {
         if (IFX_SUCCESS != TAPIDEMO_SetExtBoard(&oCtrlStatus.rgoBoards[i]))
         {
            break;
         }
      } /* for */
   } /* if */
/* ----- Stop Detect and register the main board and extension board(s) ----- */

/* ---------- Start Print information about used compilation flag ----------- */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("Compiled for %s, with:\n",
                                     oCtrlStatus.rgoBoards[0].pszBoardName));
#ifdef FXO
   /* print: "with FXO(device_list)" */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("   - FXO ("));

#ifdef DUSLIC_FXO
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("%s", DUSLIC_FXO_NAME));
#endif /* DUSLIC_FXO */
#if defined(DUSLIC_FXO) && defined(TERIDIAN_FXO)
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, (" and "));
#endif /* defined(DUSLIC_FXO) && defined(TERIDIAN_FXO) */
#ifdef TERIDIAN_FXO
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("%s", TERIDIAN_FXO_NAME));
#endif /* TERIDIAN_FXO */
#if defined(SLIC121_FXO) && defined(TERIDIAN_FXO)
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, (" and "));
#endif /* defined(SLIC121_FXO) && defined(TERIDIAN_FXO) */
#ifdef SLIC121_FXO
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("%s", SLIC121_FXO_NAME));
#endif /* SLIC121_FXO */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, (")\n"));
#endif /* FXO */

#ifdef TD_DECT
   /* dect support */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("   - DECT\n"));
#endif /* TD_DECT */

#ifdef TD_PPD
   /* phone plug detection support */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("   - FXS Phone Plug Detection\n"));
#endif /* TD_DECT */

#ifdef EVENT_LOGGER_DEBUG
   /* Event Logger support */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("   - Event Logger support\n"));
#endif /* EVENT_LOGGER_DEBUG */
/* ----------- Stop Print information about used compilation flag ----------- */

   /* Set default IP-Address  */
   TAPIDEMO_SetDefaultAddr(oCtrlStatus.pProgramArg);
#ifdef TD_IPV6_SUPPORT
   if (oCtrlStatus.pProgramArg->oArgFlags.nUseIPv6)
   {
      TAPIDEMO_SetDefaultAddrIPv6(&oCtrlStatus);
   }
#endif /* TD_IPV6_SUPPORT */

   /* set conn ID */
   ABSTRACT_SeqConnID_Init(&oCtrlStatus);

   /* Register signal handler */
   if (IFX_SUCCESS != TAPIDEMO_RegisterSigHandler())
   {
      /* Could not register signal handler */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Could not register signal handler. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* initialize arrary with pointers to codecs names */
   VOIP_SetCodecNames();

   /* Turn on services */
   TAPIDEMO_RunServices(&oCtrlStatus);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
         ("Using network interface: %s, %02X:%02X:%02X:%02X:%02X:%02X.\n",
          TD_GetStringIP(&oProgramArg.oMy_IP_Addr),
          oCtrlStatus.oTapidemo_MAC_Addr[0],
          oCtrlStatus.oTapidemo_MAC_Addr[1],
          oCtrlStatus.oTapidemo_MAC_Addr[2],
          oCtrlStatus.oTapidemo_MAC_Addr[3],
          oCtrlStatus.oTapidemo_MAC_Addr[4],
          oCtrlStatus.oTapidemo_MAC_Addr[5]));


#ifdef TD_IPV6_SUPPORT
   if (oCtrlStatus.pProgramArg->oArgFlags.nUseIPv6)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Using interface %s.\n", oProgramArg.aNetInterfaceName));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Using IPv6 address: %s, scope %d.\n",
             TD_GetStringIP(&oCtrlStatus.oIPv6_Ctrl.oAddrIPv6),
             TD_SOCK_IPv6ScopeGet(&oCtrlStatus.oIPv6_Ctrl.oAddrIPv6)));
   }
#endif /* TD_IPV6_SUPPORT */

#if (defined EASY336 || defined XT16)
   /* get IP and MAC addresses of veth0 interface */
   TAPIDEMO_SetVeth0Addr();
#endif /* (defined EASY336 || defined XT16) */

   COM_COMMUNICATION_INIT(oProgramArg.aNetInterfaceName,
                          oProgramArg.oMy_IP_Addr, TD_CONN_ID_INIT);

   COM_RCV_FILES_ON_STARTUP(&oCtrlStatus);

#ifdef TD_FAX_MODEM
#if (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW))
   /* Initialize FAX T.38 test */
   TD_T38_FAX_TEST_Init(&oCtrlStatus);
#endif /* (defined(TD_T38_FAX_TEST) && defined(TD_FAX_T38) && defined(HAVE_T38_IN_FW)) */
#endif /* TD_FAX_MODEM */

#ifdef TD_TIMER
   /* initialize timer */
   if (IFX_SUCCESS != IFX_TIM_Init(TD_TIMER_FIFO_NAME,
                                   (IFX_uint32_t*)&oCtrlStatus.nTimerFd,
                                   (IFX_uint32_t*)&oCtrlStatus.nTimerMsgFd))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, IFX_TIM_Init() failed\n(File: %s, line: %d)\n",
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("Timer successfully initialized.\n"));
#endif /* TD_TIMER */

   /* Start all boards */
   for (i = 0; i < oCtrlStatus.nBoardCnt; i++)
   {
      /* Keep number of boards before initialization */
      nPreviousBoardCnt = oCtrlStatus.nBoardCnt;
      if ( IFX_SUCCESS != TAPIDEMO_StartBoard(i))
      {
         return IFX_ERROR;
      }
      /* Check if any board was removed from array.
         Number of board is decreased when extension board initialization failed.
         E.g. when extension board isn't connected to the main board. */
      if (nPreviousBoardCnt > oCtrlStatus.nBoardCnt)
      {
         /* One of the board has been removed from array, so we need to fix iteration */
         i--;
      }
   } /* for */

/* -------------- Start Print information about using settings -------------- */
   TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("Using board %s with:\n",
                   oCtrlStatus.rgoBoards[0].pszBoardName));
   if (oCtrlStatus.nBoardCnt > 1)
   {
     for (i=1; i < oCtrlStatus.nBoardCnt; i++)
     {
        TAPIDEMO_PRINTF(TD_CONN_ID_INIT, ("- Extension board: %s.\n",
                         oCtrlStatus.rgoBoards[i].pszBoardName));
     }
   }
   PrintUsedOptions(&oProgramArg);
/* --------------- Stop Print information about using settings -------------- */

   /* end verify system initialization test */
   COM_MOD_VERIFY_SYSTEM_INIT_SEND_DONE(0);

   /* Prepare socket for making/ending calls. */
   oCtrlStatus.nAdminSocket = TAPIDEMO_InitAdminSocket();
#ifdef TD_IPV6_SUPPORT
   if (oCtrlStatus.pProgramArg->oArgFlags.nUseIPv6)
   {
      TAPIDEMO_InitAdminSocketIPv6(&oCtrlStatus);
   }
#endif /* TD_IPV6_SUPPORT */
   if (oCtrlStatus.nSumCoderCh > 0)
   {
#ifdef TAPI_VERSION4
      fUseSockets = IFX_FALSE;
      /* If no board uses socket, we don't need to alocate memory for sockets */
      for (i = 0; i < oCtrlStatus.nBoardCnt; i++)
      {
         if (oCtrlStatus.rgoBoards[i].fUseSockets)
         {
            fUseSockets = IFX_TRUE;
         }
      }

      /* If we don't use socket, we don't need alocate memmory for it */
      if(fUseSockets)
#endif /* TAPI_VERSION4 */
      {
         /* Prepare UDP sockets for VoIP external calls */
         oCtrlStatus.rgnSockets =
            TD_OS_MemCalloc(oCtrlStatus.nSumCoderCh, sizeof(SOCKET_STAT_t));

         if (oCtrlStatus.rgnSockets == IFX_NULL)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("ERR allocate memory for UDP sockets. (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
            /* Cleanup previous stuff */
#ifdef VXWORKS
            abort();
#else /* VXWORKS */
            TAPIDEMO_CleanUp(SIGABRT);
#endif /* VXWORKS */
         }
#ifdef TD_IPV6_SUPPORT
         if (oCtrlStatus.pProgramArg->oArgFlags.nUseIPv6)
         {
            /* Prepare UDP sockets for VoIP external calls */
            oCtrlStatus.rgnSocketsIPv6 =
               TD_OS_MemCalloc(oCtrlStatus.nSumCoderCh, sizeof(SOCKET_STAT_t));

            if (oCtrlStatus.rgnSocketsIPv6 == IFX_NULL)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("ERR allocate memory for UDP sockets. (File: %s, line: %d)\n",
                     __FILE__, __LINE__));
               /* Cleanup previous stuff */
#ifdef VXWORKS
               abort();
#else /* VXWORKS */
               TAPIDEMO_CleanUp(SIGABRT);
#endif /* VXWORKS */
            }
         }
#endif /* TD_IPV6_SUPPORT */
      }

      if (!oCtrlStatus.pProgramArg->oArgFlags.nQos ||
          oCtrlStatus.pProgramArg->oArgFlags.nQosSocketStart)
      {
         IFX_int32_t k = 0;

         tmp_coder_sum = 0;
         j = 0;
         pBoard = &oCtrlStatus.rgoBoards[j];
         tmp_coder_sum = pBoard->nMaxCoderCh;
         for (i = 0; i < oCtrlStatus.nSumCoderCh; i++, k++)
         {
            /* Also initialize sockets */
#ifdef TAPI_VERSION4
            if (fUseSockets)
#endif /* TAPI_VERSION4 */
            {
               oCtrlStatus.rgnSockets[i].nSocket =
                  VOIP_InitUdpSocket(oCtrlStatus.pProgramArg, i,
                                     &oCtrlStatus.rgnSockets[i].nPort);
               if (NO_SOCKET == oCtrlStatus.rgnSockets[i].nSocket)
               {
                  /* Error initializing VoIP sockets */
                  /* Cleanup previous stuff */
   #ifdef VXWORKS
                  abort();
   #else /* VXWORKS */
                  TAPIDEMO_CleanUp(SIGABRT);
   #endif /* VXWORKS */
                  return IFX_ERROR;
               }

                /* Connect this sockets for each board, data ch of this board is
                  index in socket array. */
               if (tmp_coder_sum <= i)
               {
                  if (j < MAX_BOARDS)
                  {
                     j++;
                     pBoard = &oCtrlStatus.rgoBoards[j];
                     tmp_coder_sum += pBoard->nMaxCoderCh;
                     k = 0;
                  }
                  else
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                        ("Err, Exceeded the maximum number of boards."
                         "(File: %s, line: %d)\n", __FILE__, __LINE__));
                     /* Cleanup previous stuff */
#ifdef VXWORKS
                     abort();
#else /* VXWORKS */
                     TAPIDEMO_CleanUp(SIGABRT);
#endif /* VXWORKS */
                     return IFX_ERROR;
                  }

               }
               pBoard->rgnSockets[k] = oCtrlStatus.rgnSockets[i].nSocket;
#ifdef TD_IPV6_SUPPORT
               if (oCtrlStatus.pProgramArg->oArgFlags.nUseIPv6)
               {
                  oCtrlStatus.rgnSocketsIPv6[i].nSocket =
                     VOIP_InitUdpSocketIPv6(&oCtrlStatus, oCtrlStatus.pProgramArg, i,
                                            &oCtrlStatus.rgnSocketsIPv6[i].nPort);
                  if (NO_SOCKET == oCtrlStatus.rgnSocketsIPv6[i].nSocket)
                  {
                     /* Error initializing VoIP sockets */
                     /* Cleanup previous stuff */
#ifdef VXWORKS
                     abort();
#else /* VXWORKS */
                     TAPIDEMO_CleanUp(SIGABRT);
#endif /* VXWORKS */
                     return IFX_ERROR;
                  }
                  pBoard->rgnSocketsIPv6[k] = oCtrlStatus.rgnSocketsIPv6[i].nSocket;
               }
#endif /* TD_IPV6_SUPPORT */
            } /* if (pBoard->fUseSockets) */
         } /* for */
      } /* if (!oCtrlStatus.pProgramArg->oArgFlags.nQos) */
   } /* if (oCtrlStatus.nSumCoderCh > 0) */

   /* Holds called phone number, which will be increased in *_Setup() */
#ifndef TAPI_VERSION4
   phone_num = 1;
#endif

   /* If support for multiple boards then initialize all boards. */
   for (i = 0; i < oCtrlStatus.nBoardCnt; i++)
   {
      /* Prepare objects, structure */
      if (IFX_ERROR == TAPIDEMO_Setup(&oCtrlStatus.rgoBoards[i],
                                      &oCtrlStatus, &phone_num))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("TAPIDEMO_Setup() failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         /* Cleanup previous stuff */
#ifdef VXWORKS
         abort();
#else /* VXWORKS */
         TAPIDEMO_CleanUp(SIGABRT);
#endif /* VXWORKS */
         return IFX_ERROR;
      }
      /* If support for multiple boards then initialize all boards. */
   } /* for all boards*/

#ifndef EASY336
   if (oCtrlStatus.nSumPCM_Ch > 0)
   {
      if (PCM_Init(&oProgramArg) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, PCM_Init() failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));

         /* Cleanup previous stuff */
#ifdef VXWORKS
         abort();
#else /* VXWORKS */
         TAPIDEMO_CleanUp(SIGABRT);
#endif /* VXWORKS */
         return IFX_ERROR;
      } /* if */
   } /* if (oCtrlStatus.nSumPCM_Ch > 0) */
#endif /* EASY336 */

   /* configure indication tones */
   Common_ToneIndicationCfg(&oCtrlStatus);

#ifdef EASY336
   if (IFX_ERROR == BOARD_Easy336_Init_RM(&oCtrlStatus, &pcm_cfg))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("DEVICE_SVIP_Init_RM() failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      /* Cleanup previous stuff */
#ifdef VXWORKS
      abort();
#else /* VXWORKS */
      TAPIDEMO_CleanUp(SIGABRT);
#endif /* VXWORKS */
      return IFX_ERROR;
   }
#endif /* EASY336 */

   TAPIDEMO_Call_Info(&oCtrlStatus);

   /* do not use timeout untill it is needed */
   TD_TIMEOUT_RESET(oCtrlStatus.nUseTimeoutSocket);

#ifdef TD_IPV6_SUPPORT
   if (IFX_ERROR == Common_PhonebookInit(&oCtrlStatus))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Common_PhonebookInit() failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
      /* Cleanup previous stuff */
      TAPIDEMO_CleanUp(SIGABRT);
      return IFX_ERROR;
   }
#endif /* TD_IPV6_SUPPORT */

   /* Actions are handled by two threads. The first thread is responsible
      for handling actions from sockets. The second one is resposible for
      handling actions from devices. Two separate threads are needed because of
      library IFX OS, which has diffrent functions for supporting sockets and
      devices.
      Actions might be requested from different sources: devices or sockets.
      The synchronization between threads is using mutex (protect access to a
      shared resources) and synch pipe (used for closing thread and refresh
      timeout).*/

   /********************************************************
         Init mutex using for lock state machine
    ********************************************************/
   if (IFX_SUCCESS !=
       TD_OS_MutexInit(&oCtrlStatus.oMultithreadCtrl.oMutex.mutexStateMachine))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to init mutex using for lock state machine.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
   }
#define FLIP_FLOP 0
#if FLIP_FLOP				//Enable this to c\check the working
   IFX_int32_t fd_ch0;
#if 1 /* VVDN_TEST */
{
   IFX_int32_t  fd_dev_ctrl;
   IFX_TAPI_MAP_PHONE_t phonemap;
   IFX_TAPI_LINE_FEED_t lineMode;
   IFX_TAPI_PCM_LOOP_CFG_t lbpkparam;

   fd_ch0 = Common_GetFD_OfCh(pBoard, 0);
   fd_dev_ctrl = pBoard->nDevCtrl_FD;

#if 1  /* manually configure PCM channel and activate in case "-p m" option does not work */
{
   IFX_TAPI_PCM_CFG_t pcmConf;
   IFX_TAPI_PCM_IF_CFG_t param;
   memset (&param, 0, sizeof (param));
   param.nOpMode       = IFX_TAPI_PCM_IF_MODE_MASTER;
   param.nDCLFreq      = IFX_TAPI_PCM_IF_DCLFREQ_2048;
   param.nDoubleClk    = IFX_DISABLE;
   param.nSlopeTX      = IFX_TAPI_PCM_IF_SLOPE_RISE;
   param.nSlopeRX      = IFX_TAPI_PCM_IF_SLOPE_FALL;
   param.nOffsetTX     = IFX_TAPI_PCM_IF_OFFSET_NONE;
   param.nOffsetRX     = IFX_TAPI_PCM_IF_OFFSET_NONE;
   param.nDrive        = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   param.nShift        = IFX_DISABLE;
   param.nMCTS         = 0x00;
   param.nHighway = 0;
   if (TD_IOCTL(fd_ch0, IFX_TAPI_PCM_IF_CFG_SET,
         (IFX_int32_t) &param, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      printf("\n >>> ERROR configuring PCM IF Master highway 0***\n");
      return IFX_ERROR;
   }else
      printf("\n >>> Configured PCM IF  Master on highway 0 ***\n");

   memset(&pcmConf, 0, sizeof (IFX_TAPI_PCM_CFG_t));
   /* Use PCM highway number 0 */
   pcmConf.nHighway = 0;
   /* Program the second PCM channel */
   /* Use Alaw coding for the RX/TX time slots */
   /* Note: only one time slot is required in RX and TX */
   pcmConf.nResolution = IFX_TAPI_PCM_RES_WB_LINEAR_16BIT;//IFX_TAPI_PCM_RES_NB_ALAW_8BIT;
   /* Configure used RX and TX time slots */
   pcmConf.nTimeslotRX = 1;
   pcmConf.nTimeslotTX = 1;
   if (TD_IOCTL(fd_ch0, IFX_TAPI_PCM_CFG_SET,
         (IFX_int32_t) &pcmConf, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      printf("\n ERROR configuring timeslots on PCM channel  0 ***\n");
      return IFX_ERROR;
   }else
      printf("\n Configured timeslots on PCM channel  0 ***\n");

#if 0 /* edk  PCM loopback */
   memset (&lbpkparam, 0, sizeof (lbpkparam));
   // configure PCM D-channel
   lbpkparam.nTimeslot1  = 3;
   lbpkparam.nTimeslot2  = 1;
   lbpkparam.nEnable     = IFX_ENABLE;
  if (TD_IOCTL(fd_ch0, IFX_TAPI_PCM_LOOP_CFG_SET,
         (IFX_int32_t) &lbpkparam, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
            printf("\nIFX_TAPI_PCM_LOOP_CFG_SET:  FAILED!!!  \n");
        else
            printf("\nIFX_TAPI_PCM_LOOP_CFG_SET:  SUCCESS  \n");

#else  /* PCM loopback does not need activation */
   /* PCM channel has been configured, however the communication is not
active yet */
   if (TD_IOCTL(fd_ch0, IFX_TAPI_PCM_ACTIVATION_SET,
         (IFX_int32_t) 1, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      printf("\n ERROR activating PCM channel 0 ***\n");
      return IFX_ERROR;
   }else
      printf("\n Activated PCM channel 0 ***\n");
#endif
}
#endif
#if 0
   /* set linefeed on Analog phone to Standby mode */
  lineMode = IFX_TAPI_LINE_FEED_STANDBY;
  if (TD_IOCTL(fd_ch0, IFX_TAPI_LINE_FEED_SET,
         (IFX_int32_t) lineMode, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
    printf("\nCh 0. IFX_TAPI_LINE_FEED_SET: Standby FAILED ioctl !! \n");
  else
    printf("\nCh 0. IFX_TAPI_LINE_FEED_SET: Standby Succcess \n");

   /* set linefeed on Analog phone to Standby mode */
  lineMode = IFX_TAPI_LINE_FEED_ACTIVE;
  if (TD_IOCTL(fd_ch0, IFX_TAPI_LINE_FEED_SET,
         (IFX_int32_t) lineMode, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
    printf("\nCh 0. IFX_TAPI_LINE_FEED_SET: ACTIVE FAILED ioctl !! \n");
  else
    printf("\nCh 0. IFX_TAPI_LINE_FEED_SET: ACTIVE Succcess \n");

#endif
  /* Map Analog phone 0 to PCM channel 0 */
   memset(&phonemap, 0, sizeof(IFX_TAPI_MAP_PHONE_t));
   phonemap.dev = 0;
   phonemap.ch = 0;
   phonemap.nPhoneCh = 0;
   phonemap.nChType = IFX_TAPI_MAP_TYPE_PCM;
  if (TD_IOCTL(fd_ch0, IFX_TAPI_MAP_PHONE_ADD,
         (IFX_int32_t) &phonemap, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
            printf("\nIFX_TAPI_MAP_PHONE_ADD:  FAILED!!!  \n");
        else
            printf("\nIFX_TAPI_MAP_PHONE_ADD:  SUCCESS  \n");
	system("mem -s 0x1F103034 -w 0x04000020 -u");			
  	//open_LTE_Pipe();
    //FaxEvent.FAXEventType=eFAX_EVENT_INIT_SUCCESS;
    //printf("printing the size of the structure %d\n",sizeof(FaxEvent));
  //  sendEventToPipe("Hello I am communicating from the Tapidemo application\n");
    //sendEventToPipe(FaxEvent);
#if 1	/*FAX Related Settings*/
	IFX_TAPI_JB_CFG_t 	param1;
	memset (&param1, 0, sizeof (param1));
	 // Reconfigure JB for fax/modem communications
	param1.nJbType = IFX_TAPI_JB_TYPE_FIXED;
	param1.nPckAdpt = IFX_TAPI_JB_PKT_ADAPT_DATA;
	// The JB size are strictly application dependent
	// Initial JB size 90 ms
	param1.nInitialSize = 0x02D0;
	// Minimum JB size 10 ms
	param1.nMinSize = 0x50;
	// Maximum JB size 180 ms
	param1.nMaxSize = 0x5A0;
	if (ioctl (fd_ch0, IFX_TAPI_JB_CFG_SET, (IFX_uintptr_t) &param1) != IFX_SUCCESS)
		return IFX_ERROR;
	/* This example can be used on PCM channels by replacing */
	/* IFX_TAPI_WLEC_PHONE_CFG_SET with ioctl IFX_TAPI_WLEC_PCM_CFG_SET */
	IFX_TAPI_WLEC_CFG_t lecConf;
	memset(&lecConf, 0, sizeof (IFX_TAPI_WLEC_CFG_t));
	/* Now echo cancellation is not required anymore */
	/* Disable WLEC to save processing power and reduce power consumption */
	lecConf.nType = IFX_TAPI_WLEC_TYPE_OFF;
	lecConf.bNlp= IFX_TAPI_WLEC_NLP_OFF;
	if (TD_IOCTL(fd_ch0, IFX_TAPI_WLEC_PHONE_CFG_SET,
	       (IFX_int32_t) &lecConf, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
		return IFX_ERROR;
	// if (ioctl(fd_ch0, IFX_TAPI_WLEC_PHONE_CFG_SET, (IFX_int32_t) &lecConf) != IFX_SUCCESS)
	//	  return IFX_ERROR;

	/* Now echo cancellation is not required anymore */
	/* Disable WLEC to save processing power and reduce power consumption */
	lecConf.nType = IFX_TAPI_WLEC_TYPE_OFF;
	lecConf.bNlp= IFX_TAPI_WLEC_NLP_OFF;
	//  if (ioctl(fd_ch0, IFX_TAPI_WLEC_PCM_CFG_SET, (IFX_int32_t) &lecConf) != IFX_SUCCESS)
	//	  return IFX_ERROR;
	if (TD_IOCTL(fd_ch0, IFX_TAPI_WLEC_PCM_CFG_SET,
	       (IFX_int32_t) &lecConf, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
		return IFX_ERROR;

#if 0
		IFX_TAPI_LINE_VOLUME_t 	param;
		memset (&param, 0, sizeof (param));
		param.nGainTx = 0;
		param.nGainRx = 0;
		if (TD_IOCTL(fd_ch0, IFX_TAPI_PHONE_VOLUME_SET,
		       (IFX_uintptr_t) &param, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
		return IFX_ERROR;
		//IFX_TAPI_LINE_VOLUME_t 	param;
		//memset (&param, 0, sizeof (param));
		param.nGainTx = 0;
		param.nGainRx = 0;
		if (TD_IOCTL(fd_ch0, IFX_TAPI_PCM_VOLUME_SET,
		       (IFX_uintptr_t) &param, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
		return IFX_ERROR;
#endif
		/* Disable AGC */
			if (TD_IOCTL(fd_ch0, IFX_TAPI_ENC_AGC_ENABLE,
			       (int) IFX_TAPI_ENC_AGC_MODE_DISABLE, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
			return IFX_ERROR;




#endif/*End of FAX Related Settings*/
   /* play a tone from PCM and listen on Analog phone */
#if 0/* edk */
   VVDN_PCM_Tone_Define(fd_ch0);
   VVDN_PCM_Playtone(fd_ch0);
#endif
 //  while (1);

}
#endif
#else
IFX_int32_t fd_ch0;
{
   IFX_int32_t  fd_dev_ctrl;
   IFX_TAPI_MAP_PHONE_t phonemap;
   IFX_TAPI_LINE_FEED_t lineMode;
   IFX_TAPI_PCM_LOOP_CFG_t lbpkparam;
   fd_ch0 = Common_GetFD_OfCh(pBoard, 0);
   fd_dev_ctrl = pBoard->nDevCtrl_FD;
  {
   IFX_TAPI_PCM_CFG_t pcmConf;
   IFX_TAPI_PCM_IF_CFG_t param;
   memset (&param, 0, sizeof (param));
   param.nOpMode       = IFX_TAPI_PCM_IF_MODE_SLAVE;
   param.nDCLFreq      = IFX_TAPI_PCM_IF_DCLFREQ_2048;
   param.nDoubleClk    = IFX_DISABLE;
   param.nSlopeTX      = IFX_TAPI_PCM_IF_SLOPE_RISE;
   param.nSlopeRX      = IFX_TAPI_PCM_IF_SLOPE_FALL;
   param.nOffsetTX     = IFX_TAPI_PCM_IF_OFFSET_1;
   param.nOffsetRX     = IFX_TAPI_PCM_IF_OFFSET_2;
   param.nDrive        = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   param.nShift        = IFX_DISABLE;
   param.nMCTS         = 0x00;
   param.nHighway = 0;
   if (TD_IOCTL(fd_ch0, IFX_TAPI_PCM_IF_CFG_SET,
         (IFX_int32_t) &param, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      printf("\n >>> ERROR configuring PCM IF SLAVE highway 0***\n");
      return IFX_ERROR;
   }
   memset(&pcmConf, 0, sizeof (IFX_TAPI_PCM_CFG_t));
   /* Use PCM highway number 0 */
   pcmConf.nHighway = 0;
   pcmConf.nResolution = IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT;
  /* Configure used RX and TX time slots */
   pcmConf.nTimeslotRX = 0;
   pcmConf.nTimeslotTX = 0;
   if (TD_IOCTL(fd_ch0, IFX_TAPI_PCM_CFG_SET,
        (IFX_int32_t) &pcmConf, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      printf("\n ERROR configuring timeslots on PCM channel  0 ***\n");
      return IFX_ERROR;
   }
  if (TD_IOCTL(fd_ch0, IFX_TAPI_PCM_ACTIVATION_SET,
         (IFX_int32_t) 1, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
   {
      printf("\n ERROR activating PCM channel 0 ***\n");
      return IFX_ERROR;
   }
}
  /* set linefeed on Analog phone to Standby mode */
  lineMode = IFX_TAPI_LINE_FEED_STANDBY;
  if (TD_IOCTL(fd_ch0, IFX_TAPI_LINE_FEED_SET,
        (IFX_int32_t) lineMode, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
    printf("\nCh 0. IFX_TAPI_LINE_FEED_SET: Standby FAILED ioctl !! \n");
  else
    printf("\nCh 0. IFX_TAPI_LINE_FEED_SET: Standby Success \n");
   /* set linefeed on Analog phone to Standby mode */
  lineMode = IFX_TAPI_LINE_FEED_ACTIVE;
  if (TD_IOCTL(fd_ch0, IFX_TAPI_LINE_FEED_SET,
         (IFX_int32_t) lineMode, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
    printf("\nCh 0. IFX_TAPI_LINE_FEED_SET: ACTIVE FAILED ioctl !! \n");
  else

    printf("\nCh 0. IFX_TAPI_LINE_FEED_SET: ACTIVE Success \n");
  /* Map Analog phone 0 to PCM channel 0 */
  memset(&phonemap, 0, sizeof(IFX_TAPI_MAP_PHONE_t));
  phonemap.dev = 0;
  phonemap.ch = 0;
  phonemap.nPhoneCh = 0;
  phonemap.nChType = IFX_TAPI_MAP_TYPE_PCM;
  if (TD_IOCTL(fd_ch0, IFX_TAPI_MAP_PHONE_ADD,

         (IFX_int32_t) &phonemap, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)

            printf("\nIFX_TAPI_MAP_PHONE_ADD:  FAILED!!!  \n");
	 printf("\nFAX Manager Configuration For Flip-Flop Mode.\n");

	 printf("Disabling the Echo Cancellation Algorithms \n");



	/* This example can be used on PCM channels by replacing */
	/* IFX_TAPI_WLEC_PHONE_CFG_SET with ioctl IFX_TAPI_WLEC_PCM_CFG_SET */
	IFX_TAPI_WLEC_CFG_t lecConf;
	memset(&lecConf, 0, sizeof (IFX_TAPI_WLEC_CFG_t));
	/* Now echo cancellation is not required anymore */
	/* Disable WLEC to save processing power and reduce power consumption */
	lecConf.nType = IFX_TAPI_WLEC_TYPE_OFF;
	lecConf.bNlp= IFX_TAPI_WLEC_NLP_OFF;
	if (TD_IOCTL(fd_ch0, IFX_TAPI_WLEC_PHONE_CFG_SET,
	       (IFX_int32_t) &lecConf, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
		return IFX_ERROR;
	// if (ioctl(fd_ch0, IFX_TAPI_WLEC_PHONE_CFG_SET, (IFX_int32_t) &lecConf) != IFX_SUCCESS)
	//	  return IFX_ERROR;

	/* Now echo cancellation is not required anymore */
	/* Disable WLEC to save processing power and reduce power consumption */
	lecConf.nType = IFX_TAPI_WLEC_TYPE_OFF;
	lecConf.bNlp= IFX_TAPI_WLEC_NLP_OFF;
	//  if (ioctl(fd_ch0, IFX_TAPI_WLEC_PCM_CFG_SET, (IFX_int32_t) &lecConf) != IFX_SUCCESS)
	//	  return IFX_ERROR;
	if (TD_IOCTL(fd_ch0, IFX_TAPI_WLEC_PCM_CFG_SET,
	       (IFX_int32_t) &lecConf, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
		return IFX_ERROR;
	
	/* Disable AGC */
	if (TD_IOCTL(fd_ch0, IFX_TAPI_ENC_AGC_ENABLE,
	      (int) IFX_TAPI_ENC_AGC_MODE_DISABLE, TD_DEV_NOT_SET, TD_CONN_ID_INIT) != IFX_SUCCESS)
	return IFX_ERROR;
 
	 // printf("Playing Tone\n");
  // VVDN_PCM_Tone_Define(fd_ch0);
  // VVDN_PCM_Playtone(fd_ch0);	
//while (1);
}
//IFX_int32_t fd_ch0;
#endif		//flipflop
#if FAX_MANAGER_ENABLED
#if 1			/*Enable or Disable Voice Call Rejection Feature*/
/*Voice Call Rejection Feature*/
	IFX_TAPI_SIG_DETECTION_t startSig;
	IFX_TAPI_SIG_DETECTION_t stopSig;
	memset(&startSig, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));
	memset(&stopSig, 0, sizeof (IFX_TAPI_SIG_DETECTION_t));
	   //IFX_TAPI_SIG_CNGFAXTX- For detecting the out FAX CNG tone (OUTGoing)
	   //IFX_TAPI_SIG_CEDRX -	For detecting the response of the remote FAX machine (OutGoing)
	   //IFX_TAPI_SIG_CNGFAXRX-	For detecting the CNG tone from remote FAX machine(Incoming Call)
	   //IFX_TAPI_SIG_CEDTX  -	For detecting the Called tone from the host FAX Machine(Incoming Call)
	/* Example: start detection of Fax DIS and CNG for Fax and Modem */
	startSig.sig = IFX_TAPI_SIG_CED|IFX_TAPI_SIG_CNGFAXTX |IFX_TAPI_SIG_CNGMODRX|IFX_TAPI_SIG_DISRX|IFX_TAPI_SIG_CEDRX|IFX_TAPI_SIG_CNGFAXRX |IFX_TAPI_SIG_CEDTX;
	if (ioctl(fd_ch0, IFX_TAPI_SIG_DETECT_ENABLE, (IFX_int32_t) &startSig) != IFX_SUCCESS)
		return IFX_ERROR;
	/*For Disabling We need to do*/
	/* Disable detection of the signals */
//	if (ioctl(fd_ch0, IFX_TAPI_SIG_DETECT_DISABLE, (IFX_int32_t) &startSig) != IFX_SUCCESS)
//		return IFX_ERROR;



#endif
	/*End of Voice Call Rejection Feature*/
	open_FAX_Pipe();
	open_LTE_Pipe();
	FaxEvent.FAXEventType=eFAX_EVENT_INIT_SUCCESS;
	printf("printing the size of the structure %d\n",sizeof(FaxEvent));
	//  sendEventToPipe("Hello I am communicating from the Tapidemo application\n");
	sendEventToPipe(FaxEvent);
//	system("mem -s 0x1F103034 -w 0x04000020 -u");			
#endif
   /********************************************************
         Start thread for handle sockets
    ********************************************************/
   if (IFX_SUCCESS !=
       Common_StartThread(&oCtrlStatus.oMultithreadCtrl.oThread.oHandleSocketThreadCtrl,
                          TAPIDEMO_HandleSocketThread,
                          "HandleSocketThread",
                          &oCtrlStatus.oMultithreadCtrl.oPipe.oSocketSynch,
                          &oCtrlStatus.oMultithreadCtrl.oLock.oSocketThreadStopLock))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to start thread for handle sockets.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* set thread flag */
   //oCtrlStatus.oMultithreadCtrl.oThread.bSocketThreadStarted = IFX_TRUE;
#ifdef TAPI_VERSION4
   EVENT_DetectionInit(&oCtrlStatus);
#endif /* TAPI_VERSION4 */
   /********************************************************
         Start handle devices
    ********************************************************/
   TAPIDEMO_HandleDeviceThread(&oCtrlStatus);

   /* This part never executes. */

   return ret;
} /* main() */

