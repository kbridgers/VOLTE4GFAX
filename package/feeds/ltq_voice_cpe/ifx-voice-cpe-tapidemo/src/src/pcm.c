

/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file pcm.c
   \date 2006-03-29
   \brief Implementation of PCM module for application.

   PCM is initialized, handling with channel resources, defining timeslot for
   connection, mapping pcm channels, ...
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "pcm.h"
#include "cid.h"

#ifdef EASY336
#include "lib_svip_rm.h"
#endif /* EASY336 */

#include "voip.h"

#ifdef TD_HAVE_DRV_BOARD_HEADERS
    #include "drv_board_io.h"
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

/* ============================= */
/* Global Structures             */
/* ============================= */

/** Array of timeslots with their status */
typedef struct _TIMESLOTS_t
{
   /** IFX_TRUE - On PCM master board this one is used for TX, else is RX
       On slave mode is opposite. For now variable is only set never checked. */
   IFX_boolean_t fUsedForTX;
   /** timeslot is reserved */
   IFX_boolean_t fReserved;
   /** how many more timeslots are reserved for one direction. */
   IFX_uint32_t nNumOfTimeslots;
} TIMESLOTS_t;

#ifdef TD_HAVE_DRV_BOARD_HEADERS
/** structure used to decode enum to number */
typedef struct Freq2Number_t_ {
   /** frequency enum from board driver */
   IFX_int32_t nFreqEnum;
   /** frequency number in kHz */
   IFX_int32_t nFreqNumber;
} Freq2Number_t;
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Table of all PCM timeslots */
static TIMESLOTS_t* pTimeslots = IFX_NULL;

/** Number of PCM highways on board */
#ifndef EASY336
static IFX_int32_t nPCM_Highway = 0;
#else
static IFX_int32_t nPCM_Highway = RM_SVIP_NUM_HIGHWAYS;
#endif

const IFX_int32_t TAPIDEMO_PCM_FREQ_MAX_TIMESLOT_NUMBER[] =
{
   /* IFX_TAPI_PCM_IF_DCLFREQ_512 */
   8,
   /* IFX_TAPI_PCM_IF_DCLFREQ_1024 */
   16,
   /* IFX_TAPI_PCM_IF_DCLFREQ_1536 */
   24,
   /* IFX_TAPI_PCM_IF_DCLFREQ_2048 */
   32,
   /* IFX_TAPI_PCM_IF_DCLFREQ_4096 */
   64,
   /* IFX_TAPI_PCM_IF_DCLFREQ_8192 */
   128,
   /* IFX_TAPI_PCM_IF_DCLFREQ_16384 */
   256
};

/** Marks the end of coder type array */
#define TD_PCM_RES_END TD_MAX_ENUM_ID

/** PCM data initialized with default parameters */
TAPIDEMO_PCM_DATA_t g_oPCM_Data = {
   /* Sample rate */
   0,
   /* Max number of timeslots */
   0,
   /* Band setting */
   TD_NARROWBAND,
   /* Default WB codec */
   TD_DEFAULT_PCM_WB_RESOLUTION,
   /* Default NB codec */
   TD_DEFAULT_PCM_NB_RESOLUTION
};

/** holds codec names and corresponding to them number of
 *  timeslots and band type */
static TD_CODEC_TIMESLOT_t TD_Codec_Timeslot[] =
{
   {IFX_TAPI_PCM_RES_NB_ALAW_8BIT,    1, TD_NARROWBAND},
   {IFX_TAPI_PCM_RES_NB_ULAW_8BIT,    1, TD_NARROWBAND},
   {IFX_TAPI_PCM_RES_NB_LINEAR_16BIT, 2, TD_NARROWBAND},
   {IFX_TAPI_PCM_RES_WB_ALAW_8BIT,    2, TD_WIDEBAND},
   {IFX_TAPI_PCM_RES_WB_ULAW_8BIT,    2, TD_WIDEBAND},
   {IFX_TAPI_PCM_RES_WB_LINEAR_16BIT, 4, TD_WIDEBAND},
   {IFX_TAPI_PCM_RES_WB_G722,         1, TD_WIDEBAND},
   {IFX_TAPI_PCM_RES_NB_G726_16,      1, TD_NARROWBAND},
   {IFX_TAPI_PCM_RES_NB_G726_24,      1, TD_NARROWBAND},
   {IFX_TAPI_PCM_RES_NB_G726_32,      1, TD_NARROWBAND},
   {IFX_TAPI_PCM_RES_NB_G726_40,      1, TD_NARROWBAND},
   {IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT,  4, TD_WIDEBAND},
   {TD_PCM_RES_END,                   0, 0}
};

/** PCM codec names */
TD_ENUM_2_NAME_t TD_rgEnumPCM_CodecName[] =
{
   {IFX_TAPI_PCM_RES_NB_ALAW_8BIT,    "G.711 A-law, 8 bits NB"},
   {IFX_TAPI_PCM_RES_NB_ULAW_8BIT,    "G.711 u-law, 8 bits NB"},
   {IFX_TAPI_PCM_RES_NB_LINEAR_16BIT, "Linear 16 bits NB"},
   {IFX_TAPI_PCM_RES_WB_ALAW_8BIT,    "G.711 A-law, 8 bits WB"},
   {IFX_TAPI_PCM_RES_WB_ULAW_8BIT,    "G.711 u-law, 8 bits WB"},
   {IFX_TAPI_PCM_RES_WB_LINEAR_16BIT, "Linear 16 bits WB"},
   {IFX_TAPI_PCM_RES_WB_G722,         "G.722 16 bits WB"},
   {IFX_TAPI_PCM_RES_NB_G726_16,      "G.726 16 kbit/s NB"},
   {IFX_TAPI_PCM_RES_NB_G726_24,      "G.726 24 kbit/s NB"},
   {IFX_TAPI_PCM_RES_NB_G726_32,      "G.726 32 kbit/s NB"},
   {IFX_TAPI_PCM_RES_NB_G726_40,      "G.726 40 kbit/s NB"},
   {IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT,  "Linear 16 bits, split time slots, WB"},
   {TD_MAX_ENUM_ID,                   "IFX_TAPI_PCM_RES_t"}
};

/** max length of string with "Phone No X" or "FXO No X" */
#define MAX_STRING_LEN  30
/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

#ifdef TD_HAVE_DRV_BOARD_HEADERS
/**
   Configure PCM with board driver.

   \param fMaster - flag indicating if this board will be master (IFX_TRUE) or
                    slave (IFX_FALSE)
   \param pBoard - pointer to board
   \param fLocalLoop - IFX_TRUE if PCM local loop is used,
                       otherwise nothing (IFX_FALSE).

   \return IFX_SUCCESS if successful, otherwise IFX_ERROR.
*/
IFX_return_t PCM_ConfigBoardDrv(IFX_boolean_t fMaster, BOARD_t* pBoard,
                                IFX_boolean_t fLocalLoop)
{
   BOARD_PCM_CLK_CONF_t pcm_clk_freq = {0};
   BOARD_PCM_MASTER_CONF_t pcm_master = {0};
   BOARD_PCM_LOOP_t pcm_loop = {0};
   BOARD_PCM_HWCONN_t hw_conn = {0};
   Freq2Number_t *pFreq;
   Freq2Number_t aGetFreq[] =
   {
      {BOARD_PCLK_FREQ_4096_KHZ,    4096},
      {BOARD_PCLK_FREQ_2048_KHZ,    2048},
      {BOARD_PCLK_FREQ_1024_KHZ,    1024},
      {BOARD_PCLK_FREQ_512_KHZ,     512},
      {BOARD_PCLK_FREQ_8192_KHZ,    8192},
      {BOARD_PCLK_FREQ_16384_KHZ,   16384},
      {-1,                          0}
   };

   /* configure PCM clock */
   /* #warning g_oPCM_Data.nPCM_Rate = pcm_if.nDCLFreq;
      must be updated when value bellow is changed */
   pcm_clk_freq.pcm_clk_freq = BOARD_PCLK_FREQ_2048_KHZ;

   pFreq = aGetFreq;
   while (pFreq->nFreqEnum != -1)
   {
      if (pFreq->nFreqEnum == pcm_clk_freq.pcm_clk_freq)
      {
         break;
      }
      pFreq++;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
      ("PCM configuration with board driver: %s,%sloop %dkHz.\n",
       ((IFX_TRUE == fMaster) ? "master" : "slave"),
       ((IFX_TRUE == fLocalLoop) ? " " : " no "), pFreq->nFreqNumber));

   /* configure PCM clock */
   if (TD_IOCTL(pBoard->nSystem_FD, FIO_BOARD_PCM_CLK_CONFIG,
               (IFX_int32_t) &pcm_clk_freq, TD_DEV_NOT_SET, TD_CONN_ID_INIT)
       != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }
   pcm_master.hw = BOARD_PCM_HW_0;
   if (IFX_TRUE == fMaster)
   {
      pcm_master.master = BOARD_PCM_CLK_MASTER_INT;
   }
   else
   {
      pcm_master.master = BOARD_PCM_CLK_MASTER_EXT;
   }
   if (TD_IOCTL(pBoard->nSystem_FD, FIO_BOARD_PCM_MASTER_CONFIG,
               (IFX_int32_t) &pcm_master, TD_DEV_NOT_SET, TD_CONN_ID_INIT)
       != IFX_SUCCESS)
   {
      return IFX_ERROR;
   }
   if (IFX_FALSE == fLocalLoop)
   {
      /* if not local loop then connect PCM interface to BNC connectors */
      if (IFX_TRUE != fMaster)
      {
         /* without it slave configuration cannot be used
            if previous configuration was master */
         pcm_master.hw = BOARD_PCM_HW_EXT;
         pcm_master.master = BOARD_PCM_CLK_MASTER_DISABLED;
         if (TD_IOCTL(pBoard->nSystem_FD, FIO_BOARD_PCM_MASTER_CONFIG,
                     (IFX_int32_t) &pcm_master, TD_DEV_NOT_SET, TD_CONN_ID_INIT)
             != IFX_SUCCESS)
         {
            return IFX_ERROR;
         }
      }

      pcm_master.hw = BOARD_PCM_HW_EXT;
      if (IFX_TRUE == fMaster)
      {
         pcm_master.master = BOARD_PCM_CLK_MASTER_INT;
      }
      else
      {
         pcm_master.master = BOARD_PCM_CLK_MASTER_EXT;
      }
      if (TD_IOCTL(pBoard->nSystem_FD, FIO_BOARD_PCM_MASTER_CONFIG,
                  (IFX_int32_t) &pcm_master, TD_DEV_NOT_SET, TD_CONN_ID_INIT)
          != IFX_SUCCESS)
      {
         return IFX_ERROR;
      }
      /* configures the PCM highway connection */
      hw_conn.hw = BOARD_PCM_HW_0;
      hw_conn.conn = BOARD_PCM_CONNECTED_HWEXT;
      if (TD_IOCTL(pBoard->nSystem_FD, FIO_BOARD_PCM_HWCONN_CONFIG,
                  (IFX_int32_t) &hw_conn, TD_DEV_NOT_SET, TD_CONN_ID_INIT)
          != IFX_SUCCESS)
      {
         return IFX_ERROR;
      }
      hw_conn.hw = BOARD_PCM_HW_EXT;
      hw_conn.conn = BOARD_PCM_CONNECTED_HW0;
      if (TD_IOCTL(pBoard->nSystem_FD, FIO_BOARD_PCM_HWCONN_CONFIG,
                  (IFX_int32_t) &hw_conn, TD_DEV_NOT_SET, TD_CONN_ID_INIT)
          != IFX_SUCCESS)
      {
         return IFX_ERROR;
      }
   }
   /* loop setting */
   pcm_loop.hw = BOARD_PCM_HW_0;
   if (fLocalLoop == IFX_TRUE)
   {
      pcm_loop.bLoop = IFX_TRUE; /* activate loop */
   }
   else
   {
      pcm_loop.bLoop = IFX_FALSE; /* deactivate loop */
   }
   if (IFX_SUCCESS != TD_IOCTL(pBoard->nSystem_FD, FIO_BOARD_PCM_HWLOOP_CONFIG,
                         (IFX_int32_t) &pcm_loop, TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, PCM loop configuration failed. (File: %s, line: %d)\n",
         __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}
#endif /* TD_HAVE_DRV_BOARD_HEADERS */

/**
   Prepares table with timeslots. This table is global, not done for each board.

   \return IFX_SUCCESS if table was created, otherwise IFX_ERROR
*/
IFX_return_t PCM_PrepareTimeslotTable()
{
   IFX_return_t ret = IFX_SUCCESS;

   if (pTimeslots != IFX_NULL)
   {
      /* Table with timeslots is already prepared. */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
           ("Table with timeslots already initialized. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_SUCCESS;
   }
   /* allocate memory */
   pTimeslots = TD_OS_MemCalloc(g_oPCM_Data.nMaxTimeslotNum, sizeof(TIMESLOTS_t));

   if (pTimeslots == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, allocate memory for timeslot pairs. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return ret;
}


/**
   Release table with timeslots from memory.
*/
IFX_void_t PCM_ReleaseTimeslotTable()
{
   if (pTimeslots == IFX_NULL)
   {
      /* Table already released from memory */
      return;
   }

   if (pTimeslots != IFX_NULL)
   {
      TD_OS_MemFree(pTimeslots);
      pTimeslots = IFX_NULL;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Timeslot table released from memory\n"));
}

/**
   Returns number of timeslot needed for PCM call in one direction.

   \param nPCM_Codec - values of PCM codec in enum
                       IFX_TAPI_PCM_RES_t

   \return Number of timeslots needed, if invalid codec is
           passed, returns IFX_ERROR
*/
IFX_int32_t PCM_GetNumberOfTimeslots(IFX_int32_t nPCM_Codec)
{
   TD_CODEC_TIMESLOT_t* pTD_Codec_Timeslot = TD_Codec_Timeslot;
   /* Find appropriate coder type in array */
   do
   {
      if (nPCM_Codec == pTD_Codec_Timeslot->nCodecType)
      {
         /* Return codec band */
         return pTD_Codec_Timeslot->nTimeslotsNum;
      }
      pTD_Codec_Timeslot++;
   } while (TD_PCM_RES_END != pTD_Codec_Timeslot->nCodecType);
   /* No codec found */
   return IFX_ERROR;
} /* PCM_GetNumberOfTimeslots() */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   PCM Initialization.

   \param pProgramArg  - pointer to program arguments

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t PCM_Init(PROGRAM_ARG_t* pProgramArg)
{

   TD_PTR_CHECK(pProgramArg, IFX_ERROR);

   /* get number of timeslots */
   g_oPCM_Data.nMaxTimeslotNum =
      TAPIDEMO_PCM_FREQ_MAX_TIMESLOT_NUMBER[g_oPCM_Data.nPCM_Rate];
   if (0 >= g_oPCM_Data.nMaxTimeslotNum)
   {
      /* Could not prepare timeslot table */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, PCM: Invalid number of timeslots(%d). (File: %s, line: %d)\n",
          g_oPCM_Data.nMaxTimeslotNum, __FILE__, __LINE__));

      return IFX_ERROR;
   }

   if (PCM_PrepareTimeslotTable() != IFX_SUCCESS)
   {
      /* Could not prepare timeslot table */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, PCM_PrepareTimeslotTable(). (File: %s, line: %d)\n",
          __FILE__, __LINE__));

      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* PCM_Init() */


/**
   PCM release.

   \param pProgramArg  - pointer to program arguments
   \param pBoard       - board handle

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t PCM_Release(PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard)
{
   PCM_ReleaseTimeslotTable();

   return IFX_SUCCESS;
} /* PCM_Release() */

/**
   Returns timeslot number and reserves it.

   \param nTS_direction - direction for witch timeslots are needed
   \param fWideBand     - if IFX_TRUE WideBand is used
   \param nSeqConnId    - Seq Conn ID

   \return timeslot or TIMESLOT_NOT_SET if none was found
*/
IFX_int32_t PCM_GetTimeslots(TS_DIRECTION_t nTS_direction,
                             IFX_boolean_t fWideBand, IFX_uint32_t nSeqConnId)
{
   IFX_int32_t nTimeslot, j;
   IFX_int32_t nNumOfTimeslots;
   IFX_boolean_t fTimeslotFound = IFX_FALSE;

   TD_PARAMETER_CHECK((TS_RX != nTS_direction && TS_TX != nTS_direction),
                      (nTS_direction + nTS_direction), IFX_FALSE);

   if(fWideBand)
   {
      /* get number of timeslots */
      nNumOfTimeslots = PCM_GetNumberOfTimeslots(g_oPCM_Data.ePCM_WB_Resolution);
   }
   else
   {
      /* get number of timeslots */
      nNumOfTimeslots = PCM_GetNumberOfTimeslots(g_oPCM_Data.ePCM_NB_Resolution);
   }

   /* for all timeslots */
   for(nTimeslot = 0; nTimeslot < g_oPCM_Data.nMaxTimeslotNum; nTimeslot++)
   {
      /* check if timeslot avaible */
      if (pTimeslots[nTimeslot].fReserved == IFX_FALSE)
      {
         /* first free timeslot found */
         fTimeslotFound = IFX_TRUE;
         /* check if nNumberOfTimeslots is free */
         for (j = 1; j<nNumOfTimeslots; j++)
         {
            if ( (g_oPCM_Data.nMaxTimeslotNum <= (nTimeslot + j)) ||
                 (IFX_TRUE == pTimeslots[nTimeslot + j].fReserved) )
            {
               /* timeslot is not valid */
               fTimeslotFound = IFX_FALSE;
               break;
            }
         }
         if (IFX_TRUE == fTimeslotFound)
         {
            /* reserve all needed timeslots */
            for (j = 0; j<nNumOfTimeslots; j++)
            {
               pTimeslots[nTimeslot + j].fReserved = IFX_TRUE;
               /* set direction */
               if (TS_RX == nTS_direction)
               {
                  pTimeslots[nTimeslot + j].fUsedForTX = IFX_FALSE;
               }
               else
               {
                  pTimeslots[nTimeslot + j].fUsedForTX = IFX_TRUE;
               }
               /* set number of reserved timeslots after this one */
               pTimeslots[nTimeslot + j].nNumOfTimeslots = nNumOfTimeslots - j - 1;
            }
            TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
                  ("PCM: Timeslot %d reserved (number of timeslots %d).\n",
                   nTimeslot, nNumOfTimeslots));
            return nTimeslot;
         } /* if timeslot is valid */
      } /* if timeslot noot reserved */
   } /* for all timeslots */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
         ("PCM: Unable to get timeslots (number of timeslots %d). "
          "(File: %s, line: %d)\n",
          nNumOfTimeslots,__FILE__, __LINE__));

   return TIMESLOT_NOT_SET;
} /* PCM_GetTimeslots() */

/**
   Free timeslots from timeslots table.

   \param nTimeslot  - number of timeslot to free
   \param nSeqConnId - Seq Conn ID

   \return timeslot or -1 if none was found
*/
IFX_return_t PCM_FreeTimeslots(IFX_int32_t nTimeslot, IFX_uint32_t nSeqConnId)
{
   IFX_int32_t i = 0;
   IFX_int32_t nNumOfTimeslots;

   if (0 > nTimeslot || g_oPCM_Data.nMaxTimeslotNum <= nTimeslot)
   {
      /* Wrong input arguments */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Invalid timeslot number %d(%d). (File: %s, line: %d)\n",
             nTimeslot, nTimeslot + i,__FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* get number of timeslots to free */
   nNumOfTimeslots = pTimeslots[nTimeslot].nNumOfTimeslots;

   do
   {
      /* check if timeslot is reserved */
      if (IFX_FALSE == pTimeslots[nTimeslot + i].fReserved)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, PCM: Timeslot %d(%d) is not reserved. (File: %s, line: %d)\n",
             nTimeslot, nTimeslot + i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* reset timslot */
      pTimeslots[nTimeslot + i].fReserved = IFX_FALSE;
      pTimeslots[nTimeslot + i].fUsedForTX = IFX_FALSE;

      /* check if last reserved timeslot is reached */
      if (0 != pTimeslots[nTimeslot + i].nNumOfTimeslots)
      {
         /* reset number of timeslots */
         pTimeslots[nTimeslot + i].nNumOfTimeslots = 0;
         /* go to next timeslot */
         i++;
         /* check if valid timeslot number */
         if ((0 > (nTimeslot + i)) ||
             (g_oPCM_Data.nMaxTimeslotNum < (nTimeslot + i)))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, PCM: Invalid timeslot number %d(%d). "
                   "(File: %s, line: %d)\n",
                   nTimeslot, nTimeslot + i +1, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         /* check if valid number of timeslots */
         if (pTimeslots[nTimeslot + i].nNumOfTimeslots != (nNumOfTimeslots - i))
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
                  ("Err, PCM: Invalid number of timeslots %d(%d). "
                   "(File: %s, line: %d)\n",
                   nNumOfTimeslots, nTimeslot + i, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
      else
      {
         /* reset number of timeslots */
         pTimeslots[nTimeslot + i].nNumOfTimeslots = 0;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
               ("PCM: Timeslot %d released (number of timeslots %d).\n",
                nTimeslot, nNumOfTimeslots + 1));
         /* timeslots are free to use */
         return IFX_SUCCESS;
      } /* if (0 != pTimeslots[nTimeslot].nNumOfTimeslots) */
   }
   while(i <= nNumOfTimeslots);

   return IFX_ERROR;
} /* PCM_FreeTimeslots() */

/**
   Activate PCM

   \param pPhone        - phone for which PCM activation is done
   \param nPCM_Ch_FD    - PCM channel file descriptor
   \param fActivate     - IFX_TRUE - activate pcm, IFX_FALSE - deactivate pcm
   \param nTimeslot_RX  - number of receiving timeslot
   \param nTimeslot_TX  - number of transmiting timeslot
   \param fWideBand     - IFX_TRUE use WideBand, otherwise use NarrowBand
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS all ok, otherwise IFX_ERROR
   \remarks pPhone should be only used with TAPI_VERSION4, when making FXO calls
      pPhone is IFX_NULL so using it could cause program crash
*/
IFX_return_t PCM_SetActivation(TAPIDEMO_PORT_DATA_t* pPortData,
                            IFX_int32_t nPCM_Ch_FD, IFX_boolean_t fActivate,
                            IFX_int32_t nTimeslot_RX, IFX_int32_t nTimeslot_TX,
                            IFX_boolean_t fWideBand, IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_PCM_CFG_t pcmConf;
   IFX_int32_t nNumOfTimeslots;
   IFX_int32_t nChannelNum;
   IFX_return_t ret = IFX_SUCCESS;
#ifdef TAPI_VERSION4
   IFX_TAPI_PCM_ACTIVATION_t PCM_Activation;
#endif/* TAPI_VERSION4 */
   IFX_char_t aName[MAX_STRING_LEN];
   IFX_char_t* pTraceIndention;
   IFX_uint16_t nDevTmp = TD_DEV_NOT_SET;
   IFX_uint32_t nIoctlArg = TD_NOT_SET;

   TD_PARAMETER_CHECK((0 > nPCM_Ch_FD), nPCM_Ch_FD, IFX_ERROR);
   TD_PTR_CHECK(pPortData, IFX_ERROR);
   if (IFX_TRUE == fActivate)
   {
      TD_PARAMETER_CHECK((0 > nTimeslot_RX), nTimeslot_RX, IFX_ERROR);
      TD_PARAMETER_CHECK((g_oPCM_Data.nMaxTimeslotNum <= nTimeslot_RX),
                         nTimeslot_RX, IFX_ERROR);
      TD_PARAMETER_CHECK((0 > nTimeslot_TX), nTimeslot_TX, IFX_ERROR);
      TD_PARAMETER_CHECK((g_oPCM_Data.nMaxTimeslotNum <= nTimeslot_TX),
                         nTimeslot_TX, IFX_ERROR);
   }

#ifndef TAPI_VERSION4
   if (PORT_FXO == pPortData->nType)
   {
      /* no FXO support for FXO in tapidemo for TAPI_VERSION4 */
      TD_PTR_CHECK(pPortData->uPort.pFXO, IFX_ERROR);
      snprintf(aName, (int) MAX_STRING_LEN, "FXO No %d:",
               pPortData->uPort.pFXO->nFXO_Number);
      nChannelNum = pPortData->uPort.pFXO->nFxoCh;
   }
   else
#endif /* TAPI_VERSION4 */
   if (PORT_FXS == pPortData->nType)
   {
      TD_PTR_CHECK(pPortData->uPort.pPhopne, IFX_ERROR);
      snprintf(aName, (int) MAX_STRING_LEN, "Phone No %d:",
               pPortData->uPort.pPhopne->nPhoneNumber);
      nChannelNum = pPortData->uPort.pPhopne->nPhoneCh;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
         ("Err, Invalid port type %d. (File: %s, line: %d)\n",
          pPortData->nType, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&pcmConf, 0, sizeof (IFX_TAPI_PCM_CFG_t));

   if (fWideBand)
   {
      pcmConf.nResolution = g_oPCM_Data.ePCM_WB_Resolution;
   }
   else
   {
      pcmConf.nResolution = g_oPCM_Data.ePCM_NB_Resolution;
   }

   pcmConf.nHighway = nPCM_Highway;

   /* get number of timeslots */
   nNumOfTimeslots = PCM_GetNumberOfTimeslots(pcmConf.nResolution);

#ifdef STREAM_1_1
   pcmConf.nRate = g_oPCM_Data.nPCM_Rate;
#endif /* STREAM_1_1 */
   /* set timeslots numbers */
   pcmConf.nTimeslotRX = nTimeslot_RX;
   pcmConf.nTimeslotTX = nTimeslot_TX;

   /* Every time, we need to count indention size because it can be different,
      It depends on type of connection */
   pTraceIndention = Common_GetTraceIndention(strlen(aName)+1);
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("%s %s PCM on ch %d (%s)\n"
          "%s- codec %s\n"
          "%s- highway %d, %s kHz\n"
          "%s- timeslot pair (RX %u, TX %u)\n",
          aName, fActivate ? "Activate" : "Deactivate",
          nChannelNum, TD_DEV_IoctlGetNameOfFd(nPCM_Ch_FD, nSeqConnId),
          pTraceIndention,
          Common_Enum2Name(pcmConf.nResolution, TD_rgEnumPCM_CodecName),
          pTraceIndention,
          nPCM_Highway, TAPIDEMO_PCM_FREQ_STR[g_oPCM_Data.nPCM_Rate],
          pTraceIndention,
          pcmConf.nTimeslotRX, pcmConf.nTimeslotTX));

   if (((pcmConf.nTimeslotRX + nNumOfTimeslots) > g_oPCM_Data.nMaxTimeslotNum) ||
       ((pcmConf.nTimeslotTX + nNumOfTimeslots) > g_oPCM_Data.nMaxTimeslotNum))
   {
      /* Wrong timeslot idx */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("%s Invalid timeslots: RX %u, TX %u, max. %d. "
             "(File: %s, line: %d)\n",
             aName, pcmConf.nTimeslotRX, pcmConf.nTimeslotTX,
             g_oPCM_Data.nMaxTimeslotNum, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (IFX_TRUE == fActivate)
   {
#ifdef TAPI_VERSION4
      if (pPortData->uPort.pPhopne->pBoard->fSingleFD)
      {
         pcmConf.dev = pPortData->uPort.pPhopne->nDev;
         pcmConf.ch = pPortData->uPort.pPhopne->nPCM_Ch;
         nDevTmp = pcmConf.dev;
      } /* if (pPhone->pBoard->fSingleFD) */
#endif /* TAPI_VERSION4 */
      ret = TD_IOCTL(nPCM_Ch_FD, IFX_TAPI_PCM_CFG_SET,
               (IFX_int32_t) &pcmConf, nDevTmp, nSeqConnId);

      if (IFX_ERROR == ret)
      {
         return IFX_ERROR;
      }
   }
   /* set ret to IFX_ERROR so if ioctl is somehow skipped error is printed */
   ret = IFX_ERROR;
   /* PCM channel has been configured, however the communication
      is not active yet */
   if (IFX_TRUE == fActivate)
   {
#ifdef TAPI_VERSION4
      if ((PORT_FXS == pPortData->nType) &&
          (pPortData->uPort.pPhopne->pBoard->fSingleFD))
      {
         PCM_Activation.mode = IFX_ENABLE;
      } /* if (pPhone->pBoard->fSingleFD) */
      else
#endif /* TAPI_VERSION4 */
      {
         nIoctlArg = IFX_ENABLE;
      } /* if (pPhone->pBoard->fSingleFD) */
   }
   else
   {
#ifdef TAPI_VERSION4
      if ((PORT_FXS == pPortData->nType) &&
          (pPortData->uPort.pPhopne->pBoard->fSingleFD))
      {
         PCM_Activation.mode = IFX_DISABLE;
      } /* if (pPhone->pBoard->fSingleFD) */
      else
#endif /* TAPI_VERSION4 */
      {
         nIoctlArg = IFX_DISABLE;
      }  /* if (pPhone->pBoard->fSingleFD) */
   }

#ifdef TAPI_VERSION4
   if ((PORT_FXS == pPortData->nType) &&
       (pPortData->uPort.pPhopne->pBoard->fSingleFD))
   {
      PCM_Activation.dev = pPortData->uPort.pPhopne->nDev;
      PCM_Activation.ch = pPortData->uPort.pPhopne->nPCM_Ch;
      nIoctlArg = (IFX_uint32_t) &PCM_Activation;
      nDevTmp = PCM_Activation.dev;
   }
#endif /* TAPI_VERSION4 */

   ret = TD_IOCTL(nPCM_Ch_FD, IFX_TAPI_PCM_ACTIVATION_SET, nIoctlArg,
            nDevTmp, nSeqConnId);

   if (IFX_ERROR == ret)
   {
      return IFX_ERROR;
   }

   /* Now PCM communication is started - test it */
   return IFX_SUCCESS;
} /* PCM_Activate() */

/**
   Map pcm channel to phone channel.

   \param nDstCh     - target PCM channel
   \param nAddCh     - which phone channel to add
   \param fDoMapping - IFX_TRUE do mapping, IFX_FALSE do unmapping
   \param pBoard     - board handle
   \param nSeqConnId - Seq Conn ID

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t PCM_MapToPhone(IFX_int32_t nDstCh, IFX_int32_t nAddCh,
                            IFX_boolean_t fDoMapping,
                            BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_PHONE_t phonemap;
   IFX_int32_t fd_ch = -1;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nDstCh), nDstCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxPCM_Ch < nDstCh), nDstCh, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nAddCh), nAddCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxAnalogCh < nAddCh), nAddCh, IFX_ERROR);

   memset(&phonemap, 0, sizeof(IFX_TAPI_MAP_PHONE_t));

   fd_ch = Common_GetFD_OfCh(pBoard, nAddCh);

   phonemap.nPhoneCh = nDstCh;
   phonemap.nChType = IFX_TAPI_MAP_TYPE_PCM;
   if (IFX_TRUE == fDoMapping)
   {
      /* Map pcm channel to phone channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Map phone ch %d to pcm ch %d\n",
          (int)nAddCh, (int)nDstCh));

      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_PHONE_ADD, (IFX_int32_t) &phonemap,
               TD_DEV_NOT_SET, nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, mapping phone ch %d to pcm ch %d using fd %d. "
             "(File: %s, line: %d)\n",
             (int) nAddCh, (int) nDstCh, (int) fd_ch,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   else
   {
      /* Unmap pcm channel from phone channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Unmap phone ch %d from pcm ch %d\n",
          (int)nAddCh, (int)nDstCh));

      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_PHONE_REMOVE,
               (IFX_int32_t) &phonemap, TD_DEV_NOT_SET, nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, unmapping phone ch %d from pcm ch %d using fd %d. "
             "(File: %s, line: %d)\n",
             (int) nAddCh, (int) nDstCh, (int) fd_ch,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* PCM_MapToPhone() */


/**
   Map pcm channel to pcm channel.

   \param nDstCh     - target channel
   \param nAddCh     - which channel to add
   \param fDoMapping - IFX_TRUE do mapping, IFX_FALSE do unmapping
   \param pBoard     - board handle
   \param nSeqConnId - Seq Conn ID

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t PCM_MapToPCM(IFX_int32_t nDstCh, IFX_int32_t nAddCh,
                          IFX_boolean_t fDoMapping,
                          BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_PCM_t pcmmap;
   IFX_int32_t fd_ch = -1;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nDstCh), nDstCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxPCM_Ch < nDstCh), nDstCh, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nAddCh), nAddCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxPCM_Ch < nAddCh), nAddCh, IFX_ERROR);

   memset(&pcmmap, 0, sizeof(IFX_TAPI_MAP_PCM_t));

   fd_ch = Common_GetFD_OfCh(pBoard, nAddCh);

   pcmmap.nDstCh = nDstCh;
   pcmmap.nChType = IFX_TAPI_MAP_TYPE_PCM;
   if (IFX_TRUE == fDoMapping)
   {
      /* Map pcm channel to pcm channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Map PCM ch %d to PCM ch %d\n",
          (int)nAddCh, (int)nDstCh));

      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_PCM_ADD, (IFX_int32_t) &pcmmap,
               TD_DEV_NOT_SET, nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, mapping PCM ch %d to PCM ch %d using fd %d. "
             "(File: %s, line: %d)\n",
             (int) nAddCh, (int) nDstCh, (int) fd_ch,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   else
   {
      /* Unmap pcm channel from pcm channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Unmap PCM ch %d from PCM ch %d\n",
          (int)nAddCh, (int)nDstCh));

      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_PCM_REMOVE, (IFX_int32_t) &pcmmap,
               TD_DEV_NOT_SET, nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, unmapping PCM ch %d to PCM ch %d using fd %d. "
             "(File: %s, line: %d)\n",
             (int) nAddCh, (int) nDstCh, (int) fd_ch,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* PCM_MapToPCM() */


/**
   Map PCM channel to data channel.

   \param nDstCh     - target PCM channel
   \param nAddCh     - which data channel to add
   \param fDoMapping - IFX_TRUE do mapping, IFX_FALSE do unmapping
   \param pBoard     - board handle
   \param nSeqConnId - Seq Conn ID

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t PCM_MapToData(IFX_int32_t nDstCh, IFX_int32_t nAddCh,
                           IFX_boolean_t fDoMapping,
                           BOARD_t* pBoard, IFX_uint32_t nSeqConnId)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_int32_t fd_ch = -1;

   /* check input arguments */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nDstCh), nDstCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxPCM_Ch < nDstCh), nDstCh, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nAddCh), nAddCh, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxCoderCh < nAddCh), nAddCh, IFX_ERROR);

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

   fd_ch = VOIP_GetFD_OfCh(nAddCh, pBoard);

   datamap.nDstCh = nDstCh;
   datamap.nChType = IFX_TAPI_MAP_TYPE_PCM;
   if (IFX_TRUE == fDoMapping)
   {
      /* Map PCM channel to data channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Map data ch %d to PCM ch %d\n", (int)nAddCh, (int) nDstCh));

      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_DATA_ADD, (IFX_int32_t) &datamap,
               TD_DEV_NOT_SET, nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, mapping data ch %d to PCM ch %d using fd %d. "
             "(File: %s, line: %d)\n",
             (int) nAddCh, (int) nDstCh, (int) fd_ch,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   else
   {
      /* Unmap pcm channel from data channel */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, nSeqConnId,
         ("Unmap data ch %d from PCM ch %d\n",
          (int)nAddCh, (int) nDstCh));

      ret = TD_IOCTL(fd_ch, IFX_TAPI_MAP_DATA_REMOVE, (IFX_int32_t) &datamap,
               TD_DEV_NOT_SET, nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, unmapping data ch %d to PCM ch %d using fd %d. "
             "(File: %s, line: %d)\n",
             (int) nAddCh, (int) nDstCh, (int) fd_ch,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* PCM_MapToData() */


/**
   Return free PCM channel.

   \param pBoard - pointer to board

   \return free PCM channel or NO_FREE_PCM_CH
*/
IFX_int32_t PCM_GetFreeCh(BOARD_t* pBoard)
{
   IFX_int32_t i = 0;
   IFX_int32_t free_ch = NO_FREE_PCM_CH;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pBoard->fPCM_ChFree, IFX_ERROR);

   for (i = 0; i < pBoard->nMaxPCM_Ch; i++)
   {
      if (IFX_TRUE == pBoard->fPCM_ChFree[i])
      {
         free_ch = i;
         break;
      }
   }

   return free_ch;
} /* PCM_GetFreeCh() */


/**
   Free mapped PCM channel.

   \param nPCM_Ch - pcm channel number
   \param pBoard - pointer to board

   \return IFX_SUCCESS - pcm channel is freed, otherwise IFX_ERROR
*/
IFX_return_t PCM_FreeCh(IFX_int32_t nPCM_Ch, BOARD_t* pBoard)
{
   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pBoard->fPCM_ChFree, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nPCM_Ch), nPCM_Ch, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxPCM_Ch < nPCM_Ch), nPCM_Ch, IFX_ERROR);

   pBoard->fPCM_ChFree[nPCM_Ch] = IFX_TRUE;

   return IFX_SUCCESS;
} /* PCM_FreeCh() */


/**
   Reserve pcm channel as mapped.

   \param nPCM_Ch - pcm channel number
   \param pBoard - pointer to board
   \param nSeqConnId  - Seq Conn ID

   \return IFX_SUCCESS - pcm channel is reserved, otherwise IFX_ERROR
*/
IFX_return_t PCM_ReserveCh(IFX_int32_t nPCM_Ch, BOARD_t* pBoard,
                           IFX_uint32_t nSeqConnId)
{
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 > nPCM_Ch), nPCM_Ch, IFX_ERROR);
   TD_PARAMETER_CHECK((pBoard->nMaxPCM_Ch < nPCM_Ch), nPCM_Ch, IFX_ERROR);
   TD_PTR_CHECK(pBoard->fPCM_ChFree, IFX_ERROR);

   if (IFX_FALSE == pBoard->fPCM_ChFree[nPCM_Ch])
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
            ("Err, PCM ch %d, already reserved. (File: %s, line: %d)\n",
             (int) nPCM_Ch, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   pBoard->fPCM_ChFree[nPCM_Ch] = IFX_FALSE;

   return IFX_SUCCESS;
} /* PCM_ReserveCh() */


/**
   Start connection on PCM, but basically activate PCM.

   \param pPhone - pointer to phone
   \param pProgramArg - pointer to program arguments
   \param nPCM_Ch - pcm channel number
   \param pBoard - pointer to board
   \param nSeqConnId  - Seq Conn ID

   \return IFX_TRUE if activated, otherwise IFX_FALSE
*/
IFX_boolean_t PCM_StartConnection(TAPIDEMO_PORT_DATA_t* pPortData,
                                  PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard,
                                  CONNECTION_t* pConn, IFX_uint32_t nSeqConnId)
{
   IFX_boolean_t fWideBand = IFX_FALSE;

   /* check input parameters */
   TD_PTR_CHECK(pPortData, IFX_ERROR);
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);

   /* Set appropriate band at connection start */
   if (TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
      /* Set only for FXS connection */
      if(PORT_FXO != pPortData->nType)
      {
         pPortData->uPort.pPhopne->fWideBand_PCM_Cfg = IFX_TRUE;
      }
   }

   if ((pProgramArg->oArgFlags.nPCM_Master)
       || (pProgramArg->oArgFlags.nPCM_Slave))
   {
      /* check call type */
      if (pConn->nType == PCM_CALL ||
          pConn->nType == LOCAL_PCM_CALL ||
          pConn->nType == LOCAL_BOARD_PCM_CALL ||
          pConn->nType == FXO_CALL)
      {
         PCM_SetActivation(pPortData, pConn->nUsedCh_FD, IFX_TRUE,
                           pConn->oPCM.nTimeslot_RX, pConn->oPCM.nTimeslot_TX,
                           fWideBand, nSeqConnId);
         /* print ITM data */
         if (pConn->nType == FXO_CALL)
         {
            /* ITM trace */
            COM_MOD_FXO(nSeqConnId,
                        ("FXO_TEST:PCM_IF_PCM_ACTIVE:%d:%d\n",
                         (int) pConn->oPCM.nTimeslot_RX, pConn->nUsedCh));
         }
      } /* if (pConn->nType == PCM_CALL || ...) */
      else
      {
         /* unsupported connection type */
         return IFX_FALSE;
      }
      /* PCM is activated for this connection */
      pConn->fPCM_Active = IFX_TRUE;
      return IFX_TRUE;
   } /* if */

   return IFX_FALSE;
} /* PCM_StartConnection() */


/**
   Stop connection on PCM, but basically deactivate PCM.

   \param pPhone        - pointer to phone
   \param pProgramArg   - pointer to program arguments
   \param pBoard        - pointer to board
   \param pConn         - pointer to connection
   \param nSeqConnId    - Seq Conn ID

   \return IFX_TRUE if deactivated, otherwise IFX_FALSE
*/
IFX_boolean_t PCM_EndConnection(TAPIDEMO_PORT_DATA_t* pPortData,
                                PROGRAM_ARG_t* pProgramArg, BOARD_t* pBoard,
                                CONNECTION_t* pConn, IFX_uint32_t nSeqConnId)
{
   IFX_boolean_t fWideBand = IFX_FALSE;

   TD_PTR_CHECK(pBoard, IFX_FALSE);
   TD_PTR_CHECK(pProgramArg, IFX_FALSE);
   TD_PTR_CHECK(pConn, IFX_FALSE);
   TD_PTR_CHECK(pPortData, IFX_FALSE);

   /* for FXO calls pPhone is not used and is equal IFX_NULL */
   if (FXO_CALL != pConn->nType)
   {
      TD_PARAMETER_CHECK((PORT_FXS != pPortData->nType), pPortData->nType,
                         IFX_FALSE)
      TD_PTR_CHECK(pPortData->uPort.pPhopne, IFX_FALSE);

      if (IFX_TRUE == pPortData->uPort.pPhopne->fWideBand_PCM_Cfg)
      {
         fWideBand = IFX_TRUE;
      }
   }
   else
   {
      /* For FXO calls g_oPCM_Data holds actual state
         Currently FXO calls doesn't support WB */
      if (TD_WIDEBAND == g_oPCM_Data.nBand)
      {
         fWideBand = IFX_TRUE;
      }
   }

   if ((pProgramArg->oArgFlags.nPCM_Master) ||
       (pProgramArg->oArgFlags.nPCM_Slave))
   {
      if (pConn->nType == PCM_CALL ||
          pConn->nType == LOCAL_PCM_CALL ||
          pConn->nType == LOCAL_BOARD_PCM_CALL ||
          pConn->nType == FXO_CALL)
      {
         /* check if PCM is active */
         if (IFX_FALSE != pConn->fPCM_Active)
         {
            PCM_SetActivation(pPortData, pConn->nUsedCh_FD, IFX_FALSE,
                              pConn->oPCM.nTimeslot_RX,
                              pConn->oPCM.nTimeslot_TX,
                              fWideBand, nSeqConnId);
            /* check call type for ITM trace */
            if (pConn->nType == FXO_CALL)
            {
               COM_MOD_FXO(nSeqConnId,
                           ("FXO_TEST:PCM_IF_PCM_DEACTIVE:%d:%d\n",
                            (int) pConn->oPCM.nTimeslot_RX,
                            (int) pConn->nUsedCh));
            }
         }
      }
      else
      {
         /* unsupported connection type */
         return IFX_FALSE;
      }
      /* PCM is deactivated for this connection */
      pConn->fPCM_Active = IFX_FALSE;
      /* if at least one timeslot is set */
      if (0 <= pConn->oPCM.nTimeslot_RX && 0 <= pConn->oPCM.nTimeslot_TX)
      {
         /* only master can free timeslots */
         if (pProgramArg->oArgFlags.nPCM_Master)
         {
            if (PCM_CALL == pConn->nType ||
                FXO_CALL == pConn->nType)
            {
               PCM_FreeTimeslots(pConn->oPCM.nTimeslot_RX, nSeqConnId);
            }
            /* for PCM_LOCAL_CALL and LOCAL_BOARD_PCM_CALL only master peer
               can free timeslots */
            else if ((LOCAL_PCM_CALL == pConn->nType ||
                      LOCAL_BOARD_PCM_CALL == pConn->nType) &&
                     (IFX_TRUE != pConn->oConnPeer.oLocal.fSlave))
            {
               PCM_FreeTimeslots(pConn->oPCM.nTimeslot_RX, nSeqConnId);
               PCM_FreeTimeslots(pConn->oPCM.nTimeslot_TX, nSeqConnId);
            }
         } /* PCM master */
      } /* if at least one timeslot is set */
      if (FXO_CALL != pConn->nType)
      {
         pPortData->uPort.pPhopne->fWideBand_PCM_Cfg = IFX_FALSE;
         /* for FXO timeslot number is reseted in FXO_EndConnection() */
         pConn->oPCM.nTimeslot_RX = TIMESLOT_NOT_SET;
         pConn->oPCM.nTimeslot_TX = TIMESLOT_NOT_SET;
      }
      return IFX_TRUE;
   } /* if */

   return IFX_FALSE;
} /* PCM_EndConnection() */


/**
   Get file descriptor of device for pcm channel.

   \param nPCM_Ch - pcm channel
   \param pBoard  - board handle

   \return device connected to this channel or NO_DEVICE_FD if none
*/
IFX_int32_t PCM_GetFD_OfCh(IFX_int32_t nPCM_Ch, BOARD_t* pBoard)
{
   TD_PTR_CHECK(pBoard, NO_DEVICE_FD);
   TD_PARAMETER_CHECK((0 > nPCM_Ch), nPCM_Ch, NO_DEVICE_FD);
   TD_PARAMETER_CHECK((pBoard->nMaxPCM_Ch < nPCM_Ch), nPCM_Ch, NO_DEVICE_FD);

   return Common_GetFD_OfCh(pBoard, nPCM_Ch);
} /* PCM_GetFD_OfCh() */

/**
   Set PCM codec type received from DTMF code.

   \param nPCM_Codec    - PCM codec

   \return IFX_ERROR if codec wasn't set succesfuly, otherwise
           IFX_SUCCESS
*/
IFX_return_t PCM_CodecSet(IFX_int32_t nPCM_Codec)
{
   /* Receive coder value from DTMF code */
   switch (nPCM_Codec)
   {
      case 0:
         /* DTMF *1900 */
         g_oPCM_Data.ePCM_NB_Resolution = IFX_TAPI_PCM_RES_NB_ALAW_8BIT;
         break;
      case 1:
         /* DTMF *1901 */
         g_oPCM_Data.ePCM_NB_Resolution = IFX_TAPI_PCM_RES_NB_ULAW_8BIT;
         break;
      case 2:
         /* DTMF *1902 */
         g_oPCM_Data.ePCM_NB_Resolution = IFX_TAPI_PCM_RES_NB_LINEAR_16BIT;
         break;
      case 3:
         /* DTMF *1903 */
         g_oPCM_Data.ePCM_WB_Resolution = IFX_TAPI_PCM_RES_WB_ALAW_8BIT;
         break;
      case 4:
         /* DTMF *1904 */
         g_oPCM_Data.ePCM_WB_Resolution = IFX_TAPI_PCM_RES_WB_ULAW_8BIT;
         break;
      case 5:
         /* DTMF *1905 */
         g_oPCM_Data.ePCM_WB_Resolution = IFX_TAPI_PCM_RES_WB_LINEAR_16BIT;
         break;
      case 6:
         /* DTMF *1906 */
         g_oPCM_Data.ePCM_WB_Resolution = IFX_TAPI_PCM_RES_WB_G722;
         break;
      case 7:
         /* DTMF *1907 */
         g_oPCM_Data.ePCM_NB_Resolution = IFX_TAPI_PCM_RES_NB_G726_16;
         break;
      case 8:
         /* DTMF *1908 */
         g_oPCM_Data.ePCM_NB_Resolution = IFX_TAPI_PCM_RES_NB_G726_24;
         break;
      case 9:
         /* DTMF *1909 */
         g_oPCM_Data.ePCM_NB_Resolution = IFX_TAPI_PCM_RES_NB_G726_32;
         break;
      case 10:
         /* DTMF *1910 */
         g_oPCM_Data.ePCM_NB_Resolution = IFX_TAPI_PCM_RES_NB_G726_40;
         break;
      case 11:
         /* DTMF *1911 */
         g_oPCM_Data.ePCM_WB_Resolution = IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT;
         break;
      default:
         /* Unrecognized codec */
         return IFX_ERROR;
   }
   /* Set main codec according to band type */
   if (TD_NARROWBAND == PCM_CodecBand(nPCM_Codec))
   {
      g_oPCM_Data.nBand = TD_NARROWBAND;
   }
   else if (TD_WIDEBAND == PCM_CodecBand(nPCM_Codec))
   {
      g_oPCM_Data.nBand = TD_WIDEBAND;
   }
   /* Everything went well */
   return IFX_SUCCESS;
}
/**
   Returns type of coder type used.

   \param nPCM_Codec - codec value

   \return Band of codec as described in TD_PCM_BAND_t
*/
IFX_int32_t PCM_CodecBand(IFX_int32_t nPCM_Codec)
{
   TD_CODEC_TIMESLOT_t* pTD_Codec_Timeslot = TD_Codec_Timeslot;
   /* Find appropriate coder type in array */
   do
   {
      if (nPCM_Codec == pTD_Codec_Timeslot->nCodecType)
      {
         /* Return codec band */
         return pTD_Codec_Timeslot->nBand;
      }
      pTD_Codec_Timeslot++;
   } while (TD_PCM_RES_END != pTD_Codec_Timeslot->nCodecType);
   /* No codec found */
   return IFX_ERROR;
}
