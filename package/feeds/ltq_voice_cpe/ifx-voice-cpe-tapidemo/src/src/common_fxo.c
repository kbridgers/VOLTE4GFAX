/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file common_fxo.c
   \date 2006-11-28
   \brief This file contains implementation of FXO.

   It initializes FXO (FW, channels, ...) and uses PCM or channel mapping
   and timeslot usage. Basically when connection to FXO is started phone
   channel must be mapped to pcm channel and then pcm connection between this
   pcm channels and pcm channel on FXO is started.
*/
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#include "ifx_types.h"
#include "tapidemo.h"
#include "analog.h"
#include "abstract.h"

#ifdef FXO

/* ============================= */
/* Includes                      */
/* ============================= */
#include "common_fxo.h"

#ifdef DUSLIC_FXO
   #include "duslic_fxo.h"
#endif /* DUSLIC_FXO */

#ifdef TERIDIAN_FXO
   #include "teridian_fxo.h"
#endif /* TERIDIAN_FXO */

#ifdef SLIC121_FXO
   #include "td_slic121_fxo.h"
#endif /* TERIDIAN_FXO */

#ifdef HAVE_DRV_TAPI_HEADERS
    #include "drv_tapi_io.h"
#else
    #error "drv_tapi is missing"
#endif
#include "cid.h"
#include "pcm.h"
#include "voip.h"
#include "state_trans.h"

#ifdef TD_DECT
#include "td_dect.h"
#endif /* TD_DECT */

#ifdef EASY3111
   #include "board_easy3111.h"
#endif /* EASY3111 */

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global variable definition    */
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

IFX_return_t FXO_ReleaseDataCh(FXO_t* pFXO, BOARD_t* pBoard);
IFX_return_t FXO_Start_CPTD(IFX_int32_t nDataCh_FD, IFX_uint32_t nSeqConnId);
IFX_return_t FXO_Stop_CPTD(IFX_int32_t nDataCh_FD, IFX_uint32_t nSeqConnId);

/* ============================= */
/* Local function definition     */
/* ============================= */

/** table to hold device/channel file name */
IFX_char_t pszDevNameWithPath[MAX_DEV_NAME_LEN_WITH_NUMBER];

/** FXO names */
TD_ENUM_2_NAME_t TD_rgFxoName[] =
{
#ifdef DUSLIC_FXO
   {FXO_TYPE_DUSLIC, DUSLIC_FXO_NAME},
#endif /* DUSLIC_FXO */
#ifdef TERIDIAN_FXO
   {FXO_TYPE_TERIDIAN, TERIDIAN_FXO_NAME},
#endif /* TERIDIAN_FXO */
#ifdef SLIC121_FXO
   {FXO_TYPE_SLIC121, SLIC121_FXO_NAME},
#endif /* SLIC121_FXO */
   {FXO_TYPE_NONE, "no more devices"},
   {TD_MAX_ENUM_ID, "FXO_TYPE_t"}
};

/** FXO devices names */
TD_ENUM_2_NAME_t TD_rgFxoDevName[] =
{
#ifdef DUSLIC_FXO
   {FXO_TYPE_DUSLIC, "dus"},
#endif /* DUSLIC_FXO */
#ifdef TERIDIAN_FXO
   {FXO_TYPE_TERIDIAN, "ter"},
#endif /* TERIDIAN_FXO */
#ifdef SLIC121_FXO
   {FXO_TYPE_SLIC121, "vmmc"},
#endif /* SLIC121_FXO */
   {FXO_TYPE_NONE, "no more devices"},
   {TD_MAX_ENUM_ID, "FXO_TYPE_t"}
};

/**
   Close opened FXO channels.

   \param pFxoChFds - FXO channels FDs structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_CloseFxoChFds(FXO_CHANNELS_FDS_t* pFxoChFds)
{
   IFX_int32_t nChannel;

   /* check input parameters */
   TD_PTR_CHECK(pFxoChFds, IFX_ERROR);
   TD_PARAMETER_CHECK((TD_FXO_MAX_CHANNEL_NUMBER < pFxoChFds->nFxoChNum),
                      pFxoChFds->nFxoChNum, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pFxoChFds->nFxoChNum),
                      pFxoChFds->nFxoChNum, IFX_ERROR);

   /* for all channels */
   for (nChannel=0; nChannel<pFxoChFds->nFxoChNum; nChannel++)
   {
      if (0 > pFxoChFds->rgoFD[nChannel])
         continue;

      TD_OS_DeviceClose(pFxoChFds->rgoFD[nChannel]);
      pFxoChFds->rgoFD[nChannel] = -1;
   }
   return IFX_SUCCESS;
}
/**
   Remove device list from the board.

   \param pBoard - pointer to the board

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_RemoveFxoDevices(BOARD_t* pBoard)
{
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_t *pNextDevice = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   pDevice = pBoard->pDevice;
   while (pDevice != IFX_NULL)
   {
      pNextDevice = pDevice->pNext;
#ifdef FXO
      if (TAPIDEMO_DEV_FXO == pDevice->nType)
      {
         FXO_CleanUpFxoDevice(&pDevice->uDevice.stFxo);
         TD_OS_MemFree(pDevice);
         pDevice = pNextDevice;
      }
#endif /* FXO */
   }
   return IFX_SUCCESS;
}

/**
   Search for FXO devices.

   \param pFxoSetupData - pointer to FXO setup structure

   \return IFX_SUCCESS if device FD opened, otherwise IFX_ERROR
*/
IFX_return_t FXO_DevSearch(BOARD_t* pBoard,
                           FXO_SETUP_DATA_t* pFxoSetupData,
                           PROGRAM_ARG_t* pProgramArg)
{
   IFX_char_t* pDevNameWithPath;
   BOARD_t* pBoardTmp = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pFxoSetupData, IFX_ERROR);
   TD_PTR_CHECK(pFxoSetupData->pFxoType, IFX_ERROR);

   /* pFxoType table ends with ID == FXO_TYPE_NONE */
   while (FXO_TYPE_NONE != pFxoSetupData->pFxoType->nID)
   {
      /* Check if this FXO device was marked in the '-x' option, when
         Tapidemo was started. */
      switch (pFxoSetupData->pFxoType->nID)
      {
         case FXO_TYPE_DUSLIC:
         {
            if (!(pProgramArg->nFxoMask & TD_FXO_DUS))
            {
               /* This Fxo device is not choosen to be started.
                  Check next device. */
               pFxoSetupData->nFxoDevNum = NO_DEV_SPECIFIED;
               pFxoSetupData->pFxoType++;
               continue;
            }
            break;
         }

         case FXO_TYPE_TERIDIAN:
         {
            if (!(pProgramArg->nFxoMask & TD_FXO_TER))
            {
               /* This Fxo device is not choosen to be started.
                  Check next device. */
               pFxoSetupData->nFxoDevNum = NO_DEV_SPECIFIED;
               pFxoSetupData->pFxoType++;
               continue;
            }
            break;
         }

         case FXO_TYPE_SLIC121:
         {
            if (!(pProgramArg->nFxoMask & TD_FXO_SLIC121))
            {
               /* This Fxo device is not choosen to be started.
                  Check next device. */
               pFxoSetupData->nFxoDevNum = NO_DEV_SPECIFIED;
               pFxoSetupData->pFxoType++;
               continue;
            }
            if (pFxoSetupData->nFxoDevNum >= 1)
            {
               /* FXO on SLIC121 supported only on the main device. */
               pFxoSetupData->nFxoDevNum = NO_DEV_SPECIFIED;
               pFxoSetupData->pFxoType++;
               continue;
            }
            break;
         }
         default:
         break;
      } /* switch */

      /* check first use of function */
      if (NO_DEV_SPECIFIED == pFxoSetupData->nFxoDevNum)
      {
         /* first device number */
         pFxoSetupData->nFxoDevNum = 1;
      }
      else
      {
         /* check next device */
         pFxoSetupData->nFxoDevNum++;
      }
      /* last available device of this type was checked */
      if (TD_FXO_MAX_DEVICE_NUMBER < pFxoSetupData->nFxoDevNum)
      {
         pFxoSetupData->nFxoDevNum = NO_DEV_SPECIFIED;
         pFxoSetupData->pFxoType++;
         continue;
      }
      /* no more FXO device types to check */
      /* check if valid ID - should not happend  */
      if ((TD_MAX_ENUM_ID == pFxoSetupData->pFxoType->nID) ||
          (FXO_TYPE_NONE < pFxoSetupData->pFxoType->nID))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Invalid FXO ID %d. (File: %s, line: %d)\n",
             pFxoSetupData->pFxoType->nID, __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get device node file name */
      pDevNameWithPath = FXO_SetFxoNameString(
         Common_Enum2Name(pFxoSetupData->pFxoType->nID, TD_rgFxoDevName),
         pFxoSetupData->nFxoDevNum, TYPE_DEVICE, NO_CHANNEL);
      if (strcmp(pDevNameWithPath, TD_INVALID_STRING) == 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to get device string (type %d, dev %d)."
             " (File: %s, line: %d)\n",
             pFxoSetupData->pFxoType->nID, pFxoSetupData->nFxoDevNum,
             __FILE__, __LINE__));
        return IFX_ERROR;
      }
#ifdef SLIC121_FXO
      if (FXO_TYPE_SLIC121 == pFxoSetupData->pFxoType->nID)
      {
         /* for slic121 error handling from board can be used */
         pBoardTmp = pBoard;
      }
#endif /* SLIC121_FXO */
      /* open device FD */
      pFxoSetupData->nDevFd = Common_Open(pDevNameWithPath, pBoardTmp);
      /* device node exists if openable, otherwise node doesn't exist or program
         has no rights to open FD*/
      if (0 <= pFxoSetupData->nDevFd)
      {
         /* print initialization data */
         TD_FXO_TRACE_DEV_INIT(pFxoSetupData, TD_FXO_DEV_INIT_START);
         return IFX_SUCCESS;
      }
   }
   /* no more devices to check */
   return IFX_ERROR;
}

/**
   Configure PCM interface for FXO device.

   \param pFxoDevice - pointer to FXO device structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_PCMInit(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   IFX_uint32_t ret = IFX_SUCCESS;

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);

   switch (pFxoDevice->nFxoType)
   {
#ifdef DUSLIC_FXO
      case FXO_TYPE_DUSLIC:
         ret = DUSLIC_CfgPCMInterface(pFxoDevice);
      break;
#endif
#ifdef TERIDIAN_FXO
      case FXO_TYPE_TERIDIAN:
         ret = TERIDIAN_CfgPCMInterface(pFxoDevice);
      break;
#endif
#ifdef SLIC121_FXO
      case FXO_TYPE_SLIC121:
         /* Nothing to do with SLIC121. */
      break;
#endif
      default:
         /* Unknown FXO type */
         ret = IFX_ERROR;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Unknown FXO type (%d). (File: %s, line: %d)\n",
             pFxoDevice->nFxoType, __FILE__, __LINE__));
      break;

   } /* switch */

   return ret;
}

/**
   Initialize FXO device and FXO device structure.

   \param pFxoDevice - pointer to FXO device structure
   \param pFxoSetupData - pointer to FXO setup structure
   \param pBoard - pointer to board structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_DevInit(TAPIDEMO_DEVICE_FXO_t* pFxoDevice,
                         FXO_SETUP_DATA_t* pFxoSetupData,
                         BOARD_t* pBoard)
{
   IFX_uint32_t ret = IFX_SUCCESS;

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);
   TD_PTR_CHECK(pFxoSetupData, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pFxoSetupData->pFxoType, IFX_ERROR);

   /* set data according to setup data */
   pFxoDevice->nDevNum = pFxoSetupData->nFxoDevNum;
   pFxoDevice->nFxoType = pFxoSetupData->pFxoType->nID;
   pFxoDevice->pFxoTypeName = pFxoSetupData->pFxoType->rgName;
   pFxoDevice->nDevFD = pFxoSetupData->nDevFd;
   pFxoDevice->pBoard = pBoard;
   pFxoDevice->pIndent =
      Common_GetTraceIndention(strlen(pFxoDevice->pFxoTypeName) + TD_FXO_INDENT);

   switch (pFxoDevice->nFxoType)
   {
#ifdef DUSLIC_FXO
      case FXO_TYPE_DUSLIC:
         ret = DUSLIC_DevInit(pFxoDevice, pBoard);
      break;
#endif
#ifdef TERIDIAN_FXO
      case FXO_TYPE_TERIDIAN:
         ret = TERIDIAN_DevInit(pFxoDevice, pBoard);
      break;
#endif
#ifdef SLIC121_FXO
      case FXO_TYPE_SLIC121:
         ret = TD_SLIC121_FXO_DevInit(pFxoDevice, pBoard);
      break;
#endif
      default:
         /* Unknown FXO type */
         ret = IFX_ERROR;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Unknown FXO type (%d). (File: %s, line: %d)\n",
             pFxoDevice->nFxoType, __FILE__, __LINE__));
      break;

   } /* switch */

   return ret;
}

/**
   Function returns first fxo chunnel number.

   \param pBoard   - pointer to board structure
   \param nFxoType - FXO type
   \param pChan    - sets channel number

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_GetFirstChan(BOARD_t* pBoard, IFX_int32_t nFxoType,
                              IFX_int32_t *pChan)
{
   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pChan, IFX_ERROR);

   switch (nFxoType)
   {
#ifdef SLIC121_FXO
      case FXO_TYPE_SLIC121:
         /* Fxo channel starts after phone channels. */
         *pChan = pBoard->nMaxAnalogCh;
         break;
#endif /* SLIC121_FXO */

      default:
         *pChan = 0;
         break;
   }

   return IFX_SUCCESS;
}

/**
   Open channel FDs according to setup data.

   \param pBoard        - pointer to board structure
   \param pFxoSetupData - pointer to FXO setup structure
   \param pFxoChFds     - FXO channels FDs structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_GetChannelsFds(BOARD_t* pBoard,
                                FXO_SETUP_DATA_t* pFxoSetupData,
                                FXO_CHANNELS_FDS_t* pFxoChFds)
{
   IFX_int32_t nChannel;
   IFX_int32_t nFirstFxoCh;
   IFX_int32_t nNumOfFxo = 0;
   IFX_char_t* pChNameWithPath = IFX_NULL;
   BOARD_t* pBoardTmp = IFX_NULL;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pFxoSetupData, IFX_ERROR);
   TD_PTR_CHECK(pFxoChFds, IFX_ERROR);

   if (FXO_GetFirstChan(pBoard, pFxoSetupData->pFxoType->nID, &nFirstFxoCh) !=
       IFX_SUCCESS)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, Failed to get first Fxo channel number (fxo type: %d)."
          " (File: %s, line: %d)\n",
          pFxoSetupData->pFxoType->nID, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* check how many fxo channels function should expect. */
   switch (pFxoSetupData->pFxoType->nID)
   {
#ifdef SLIC121_FXO
      case FXO_TYPE_SLIC121:
         if (pBoard->nMaxFxo == 0)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("WARNING, %s, firmware does not support FXO. %d FXO channels.\n",
                pFxoSetupData->pFxoType->rgName, pBoard->nMaxFxo));
            return IFX_ERROR;
         }
         /* for SLIC121 number of fxo channels is read from FW. */
         nNumOfFxo = pBoard->nMaxFxo;
         break;
#endif /* SLIC121_FXO */

      default:
         nNumOfFxo = TD_FXO_MAX_CHANNEL_NUMBER;
         break;
   }

   if (nNumOfFxo > TD_FXO_MAX_CHANNEL_NUMBER)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, Wrong number of supported FXO channels "
          "(nNumOfFxo: %d, fxo type: %d)."
          " (File: %s, line: %d)\n",
          nNumOfFxo, pFxoSetupData->pFxoType->nID, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pFxoChFds->nFxoChNum = 0;

   /* open all fxo channel fd. */
   for (nChannel=0; nChannel < nNumOfFxo; nChannel++)
   {
      /* get file name string */
      pChNameWithPath = FXO_SetFxoNameString(
         Common_Enum2Name(pFxoSetupData->pFxoType->nID, TD_rgFxoDevName),
         pFxoSetupData->nFxoDevNum, TYPE_CHANNEL, (nChannel + nFirstFxoCh));
      if (strcmp(pChNameWithPath, TD_INVALID_STRING) == 0)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to get channel string (type %d (%s), dev %d, ch %d)."
             " (File: %s, line: %d)\n",
             pFxoSetupData->pFxoType->nID, pFxoSetupData->pFxoType->rgName,
             pFxoSetupData->nFxoDevNum,
             nChannel, __FILE__, __LINE__));
         /* close all opened FDs */
         for (;nChannel>0; nChannel--)
         {
            if (0 > pFxoChFds->rgoFD[nChannel])
               continue;

            TD_OS_DeviceClose(pFxoChFds->rgoFD[nChannel]);
            pFxoChFds->rgoFD[nChannel] = -1;
         }
         return IFX_ERROR;
      }
      /* open channel FD */
#ifdef SLIC121_FXO
      if (FXO_TYPE_SLIC121 == pFxoSetupData->pFxoType->nID)
      {
         /* for slic121 error handling from board can be used */
         pBoardTmp = pBoard;
      }
#endif /* SLIC121_FXO */
      pFxoChFds->rgoFD[nChannel] = Common_Open(pChNameWithPath, pBoardTmp);
      if (0 > pFxoChFds->rgoFD[nChannel])
      {
         /* if failed to open then there are no more channels for this device */
         break;
      }
      else
      {
         pFxoChFds->nFxoChNum++;
      }
   }
   /* check if any channel FD was opened, if no FD was open trace error.
      It can happend for SLIC121 that FW returns 0 fxo channels. So, in
      this case we also need to check pChNameWithPath. */
   if ((0 == pFxoChFds->nFxoChNum) && (pChNameWithPath != IFX_NULL))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, Failed to open FXO channel FD(%s) for device %s No %d."
          " (File: %s, line: %d)\n",
          pChNameWithPath, pFxoSetupData->pFxoType->rgName,
          pFxoSetupData->nFxoDevNum, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Initialize all channels for this FXO device and set channels structure.

   \param pFxoDevice - pointer to FXO device structure
   \param pFxoChFds - FXO channels FDs structure
   \param pBoard - pointer to board structure
   \param pFxoNum - pointer to FXO number

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_ChannelsInit(TAPIDEMO_DEVICE_FXO_t* pFxoDevice,
                              FXO_CHANNELS_FDS_t* pFxoChFds,
                              BOARD_t* pBoard, IFX_int32_t* pFxoNum)
{
   IFX_int32_t nChannel, nFxoNum;
   FXO_t* pFxo = IFX_NULL;
   IFX_return_t nRet = IFX_ERROR;
   IFX_int32_t nFirstFxoCh;

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);
   TD_PTR_CHECK(pFxoChFds, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PTR_CHECK(pFxoNum, IFX_ERROR);

   /* set number of channels */
   pFxoDevice->nNumOfCh = pFxoChFds->nFxoChNum;
   pFxoDevice->rgoFXO = TD_OS_MemCalloc(pFxoDevice->nNumOfCh, sizeof(FXO_t));
   if (IFX_NULL == pFxoDevice->rgoFXO)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
         ("Err, Failed to allocate memory (%s No %d)."
          " (File: %s, line: %d)\n",
          pFxoDevice->pFxoTypeName, pFxoDevice->nDevNum,
          __FILE__, __LINE__));
   }
   else
   {
      if (FXO_GetFirstChan(pBoard,
                           pFxoDevice->nFxoType,
                           &nFirstFxoCh) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Failed to get first Fxo channel number (fxo type: %d (%s))."
             " (File: %s, line: %d)\n",
             pFxoDevice->nFxoType, pFxoDevice->pFxoTypeName, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      nFxoNum = *pFxoNum;

      for (nChannel=0; nChannel < pFxoDevice->nNumOfCh; nChannel++)
      {
         pFxo = &pFxoDevice->rgoFXO[nChannel];
         pFxo->nFxoCh_FD = pFxoChFds->rgoFD[nChannel];
         /* reset this data in structure */
         pFxoChFds->rgoFD[nChannel] = 0;
         pFxo->nFxoCh = nChannel + nFirstFxoCh;
         pFxo->nDataCh = NO_FREE_DATA_CH;
         pFxo->nPCM_Ch = NO_FREE_PCM_CH;
         pFxo->nSeqConnId = TD_CONN_ID_INIT;
         pFxo->pFxoDevice = pFxoDevice;
         pFxo->nState = S_READY;
         pFxo->fCID_FSK_Started = IFX_FALSE;
         pFxo->fFirstStopRingEventOccured = IFX_FALSE;
         pFxo->oCID_DTMF.nDTMF_DetectionEnabled = IFX_FALSE;
         pFxo->oCID_DTMF.nCnt = 0;
         pFxo->oCID_Msg.message =
            TD_OS_MemCalloc(1, sizeof(IFX_TAPI_CID_MSG_ELEMENT_t));
         if (IFX_NULL == pFxo->oCID_Msg.message)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, Failed to allocate memory  for CID message."
                   " (File: %s, line: %d)\n", __FILE__, __LINE__));
         }
         /* set next  */
         pFxo->nFXO_Number = nFxoNum++;
         switch (pFxoDevice->nFxoType)
         {
#ifdef DUSLIC_FXO
            case FXO_TYPE_DUSLIC:
               nRet = DUSLIC_InitChannel(pFxoDevice, pFxo);
            break;
#endif
#ifdef TERIDIAN_FXO
            case FXO_TYPE_TERIDIAN:
               nRet = TERIDIAN_InitChannel(pFxoDevice,
                         pBoard->pCtrl->pProgramArg->sPathToDwnldFiles ,pFxo);
            break;
#endif
#ifdef SLIC121_FXO
            case FXO_TYPE_SLIC121:
               nRet = TD_SLIC121_FXO_InitChannel(pFxoDevice,
                         pBoard->pCtrl->pProgramArg->sPathToDwnldFiles ,pFxo);
            break;
#endif
            default:
               /* Unknown FXO type */
               nRet = IFX_ERROR;
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Unknown FXO type (%d). (File: %s, line: %d)\n",
                      pFxoDevice->nFxoType, __FILE__, __LINE__));
            break;

         } /* switch */
         if (IFX_SUCCESS != nRet)
         {
            /* quit for loop in case of error */
            FXO_CloseFxoChFds(pFxoChFds);
            break;
         }
      }
   } /* if (IFX_NULL == pFxoDevice->rgoFXO) */
   /* if failed to initialize then close all FDs and free memory */
   if (IFX_SUCCESS != nRet)
   {
      FXO_CleanUpFxoDevice(pFxoDevice);
   }
   else
   {
      /* update FXO number */
      *pFxoNum = nFxoNum;
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, TD_CONN_ID_INIT,
            ("Number of initialized FXO channels: %d.\n", nChannel));
   }

   return nRet;
}

/**
   Search or available FXO devices and their channels, configure them, set
   FXO structures and add FXO devices.

   \param pBoard - pointer to board structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_Setup(BOARD_t* pBoard)
{
   FXO_SETUP_DATA_t oFxoSetupData = {0};
   TAPIDEMO_DEVICE_t* pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_FXO_t* pFxoDevice = IFX_NULL;
   FXO_CHANNELS_FDS_t oFxoChFds = {{0}, 0};
   /* number of next FXO that will be initialized */
   IFX_int32_t nFxoNum = 1;

   /* check input parameters */
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   FXO_RESET_SETUP_DATA(oFxoSetupData);

   /* try to open device nodes */
   while (IFX_SUCCESS == FXO_DevSearch(pBoard, &oFxoSetupData,
                                       pBoard->pCtrl->pProgramArg))
   {
      if (IFX_NULL == pDevice)
      {
         /* allocate memory for device */
         pDevice = TD_OS_MemCalloc(1, sizeof(TAPIDEMO_DEVICE_t));
         if (IFX_NULL == pDevice)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, Failed to allocate memory . (File: %s, line: %d)\n",
                   __FILE__, __LINE__));

            /* propably some memory problem, clean up adn quit this function */
            if (0 <= oFxoSetupData.nDevFd)
            {
               TD_OS_DeviceClose(oFxoSetupData.nDevFd);
               oFxoSetupData.nDevFd = -1;
            }
            FXO_RemoveFxoDevices(pBoard);
            return IFX_ERROR;
         }
         /* set device type */
         pDevice->nType = TAPIDEMO_DEV_FXO;
         pFxoDevice = &pDevice->uDevice.stFxo;
         /* set default indentation */
         pDevice->uDevice.stFxo.pIndent = Common_GetTraceIndention(0);
      }
      /* open FXO channel nodes */
      if (IFX_SUCCESS != FXO_GetChannelsFds(pBoard, &oFxoSetupData, &oFxoChFds))
      {
         if (0 <= oFxoSetupData.nDevFd)
         {
            TD_OS_DeviceClose(oFxoSetupData.nDevFd);
            oFxoSetupData.nDevFd = -1;
         }
         /* print initialization data */
         TD_FXO_TRACE_DEV_INIT((&oFxoSetupData), TD_FXO_DEV_INIT_FAILED);
         continue;
      }

      /* initialize FXO device */
      if (IFX_SUCCESS != FXO_DevInit(pFxoDevice, &oFxoSetupData, pBoard))
      {
         if (0 <= pFxoDevice->nDevFD)
         {
            TD_OS_DeviceClose(pFxoDevice->nDevFD);
            pFxoDevice->nDevFD = -1;
         }
         FXO_CloseFxoChFds(&oFxoChFds);
         /* print initialization data */
         TD_FXO_TRACE_DEV_INIT((&oFxoSetupData), TD_FXO_DEV_INIT_FAILED);
         continue;
      }

      /* initialize FXO channels and set FXO number */
      if (IFX_SUCCESS != FXO_ChannelsInit(pFxoDevice, &oFxoChFds, pBoard,
                                          &nFxoNum))
      {
         if (0 <= pFxoDevice->nDevFD)
         {
            TD_OS_DeviceClose(pFxoDevice->nDevFD);
            pFxoDevice->nDevFD = -1;
         }
         /* print initialization data */
         TD_FXO_TRACE_DEV_INIT((&oFxoSetupData), TD_FXO_DEV_INIT_FAILED);
         continue;
      }
      /* print FXO device version */
      FXO_GetVersion(pFxoDevice);
      if (IFX_SUCCESS != FXO_PCMInit(pFxoDevice))
      {
         FXO_CleanUpFxoDevice(pFxoDevice);
         /* print initialization data */
         TD_FXO_TRACE_DEV_INIT((&oFxoSetupData), TD_FXO_DEV_INIT_FAILED);
         continue;
      }
      if (IFX_SUCCESS != Common_AddDevice(pBoard, pDevice))
      {
         FXO_CleanUpFxoDevice(pFxoDevice);
         if (IFX_NULL != pDevice)
         {
            TD_OS_MemFree(pDevice);
            pDevice = IFX_NULL;
         }
         /* failed to register device, propably some memory problem. Remove
            the FXO devices which were already registered. */
         FXO_RemoveFxoDevices(pBoard);
         /* print initialization data */
         TD_FXO_TRACE_DEV_INIT((&oFxoSetupData), TD_FXO_DEV_INIT_FAILED);

         return IFX_ERROR;
      }
      else
      {
         /* device is registered, set pointer to IFX_NULL because this memory
            is now used */
         pDevice = IFX_NULL;
         /* print initialization data */
         TD_FXO_TRACE_DEV_INIT((&oFxoSetupData), TD_FXO_DEV_INIT_END);
      }
   } /* while */

   /* no more FXO devices - free memory */
   if (IFX_NULL != pDevice)
   {
      TD_OS_MemFree(pDevice);
      pDevice = IFX_NULL;
   }
   /* set number of FXOs */
   pBoard->pCtrl->nMaxFxoNumber = nFxoNum - 1;
   if (0 == pBoard->pCtrl->nMaxFxoNumber)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("No FXO devices were initialized.\n"));
   }

   return IFX_SUCCESS;
}

/**
   Get string with device/channel file name (name is saved in pszDevNameWithPath
   table, every time this function is called pszDevNameWithPath is cleared).

   \param pDevName - pointer to string with device name
   \param nDevNum - device number (counting started from 1)
   \param nType - type of string to be returned (channel or device)
   \param nChNum - channel number (counting started from 0)

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR

   \remarks Function generates strings:
      device name: /dev/nameX0
      channel name: /dev/nameXY
      X - device number (counting from 1)
      Y - channel number (counting from 1),
      TAPI uses device and channel numbering starting from 0
*/
IFX_char_t* FXO_SetFxoNameString(IFX_char_t* pDevName, IFX_int32_t nDevNum,
                                 IFX_int32_t nType, IFX_int32_t nChNum)
{
   IFX_int32_t nLen;

   /* check input parameters */
   TD_PTR_CHECK(pDevName, TD_INVALID_STRING);

   /* clean up string */
   memset(pszDevNameWithPath, 0,
          sizeof(IFX_char_t) * MAX_DEV_NAME_LEN_WITH_NUMBER);

   /* check string type */
   if (TYPE_DEVICE == nType)
   {
      nChNum = 0;
   }
   else if (TYPE_CHANNEL == nType)
   {
      /* check channel range */
      if ((0 <= nChNum) && (TD_FXO_MAX_CHANNEL_NUMBER > nChNum))
      {
         /* channels are numbered from 1, (this input data is numbered from 0) */
         nChNum++;
      }
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Invalid channel number %d. (File: %s, line: %d)\n",
                nChNum, __FILE__, __LINE__));
         return TD_INVALID_STRING;
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Invalid type %d. (File: %s, line: %d)\n",
             nType, __FILE__, __LINE__));
      return TD_INVALID_STRING;
   }

   /* check device range */
   if ((0 >= nDevNum) || (nDevNum > TD_FXO_MAX_DEVICE_NUMBER))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Invalid device number %d. (File: %s, line: %d)\n",
             nDevNum, __FILE__, __LINE__));
      return TD_INVALID_STRING;
   }

   /* set string */
   nLen = snprintf(pszDevNameWithPath, MAX_DEV_NAME_LEN_WITH_NUMBER,
                   "%s%s%d%d", TD_DEV_PATH, pDevName, nDevNum, nChNum);

   /* check return value */
   if ((0 > nLen) || (MAX_DEV_NAME_LEN_WITH_NUMBER <= nLen))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Unable to create string - %d. (File: %s, line: %d)\n",
             nLen, __FILE__, __LINE__));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s-%s-%d-%d. (File: %s, line: %d)\n",
             TD_DEV_PATH, pDevName, nDevNum, nChNum , __FILE__, __LINE__));
      return TD_INVALID_STRING;
   }
   /* return file name */
   return pszDevNameWithPath;
}

/**
   Get FXO device version.

   \param pFxoDevice - pointer to FXO device structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_GetVersion(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   IFX_uint32_t ret = IFX_SUCCESS;

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);

   switch (pFxoDevice->nFxoType)
   {
#ifdef DUSLIC_FXO
      case FXO_TYPE_DUSLIC:
         ret = DUSLIC_GetVersions(pFxoDevice);
      break;
#endif
#ifdef TERIDIAN_FXO
      case FXO_TYPE_TERIDIAN:
         ret = TERIDIAN_GetVersions(pFxoDevice);
      break;
#endif
#ifdef SLIC121_FXO
      case FXO_TYPE_SLIC121:
         ret = TD_SLIC121_FXO_GetVersions(pFxoDevice);
      break;
#endif
      default:
         /* Unknown FXO type */
         ret = IFX_ERROR;
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, Unknown FXO type (%d). (File: %s, line: %d)\n",
             pFxoDevice->nFxoType, __FILE__, __LINE__));
      break;

   } /* switch */

   return ret;
}

/**
   release FXO device memory.

   \param pFxoDevice - pointer to FXO device structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_CleanUpFxoDevice(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   IFX_int32_t nChannel;
#ifdef SLIC121_FXO
   IFX_TAPI_FXO_LINE_MODE_t lineMode;
   IFX_uint32_t ret = IFX_SUCCESS;
#endif /* SLIC121_FXO */
   FXO_t* pFxo;

   /* Check input arguments */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);

   /* check if memory allocated */
   if (IFX_NULL != pFxoDevice->rgoFXO)
   {
      /* for all fxo channels for this device */
      for (nChannel=0; nChannel < pFxoDevice->nNumOfCh; nChannel++)
      {
         pFxo = &pFxoDevice->rgoFXO[nChannel];
#ifdef SLIC121_FXO
         if ((FXO_TYPE_SLIC121 == pFxoDevice->nFxoType) &&
             (0 < pFxo->nFxoCh_FD))
         {
            /* Disable the FXO line to save the power. */
            lineMode.mode = IFX_TAPI_FXO_LINE_MODE_DISABLED;
            ret = TD_IOCTL(pFxo->nFxoCh_FD, IFX_TAPI_FXO_LINE_MODE_SET,
                           &lineMode, TD_DEV_NOT_SET, TD_CONN_ID_INIT);
            if (IFX_SUCCESS == ret)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
                     ("%s ch %d - line deactivated.\n",
                      pFxoDevice->pFxoTypeName, pFxo->nFxoCh));
            }
         } /* if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType) */
#endif /* SLIC121_FXO */

         if (0 <= pFxo->nFxoCh_FD)
         {
            /* close channel FD */
            TD_OS_DeviceClose(pFxo->nFxoCh_FD);
         }
         /* free CID memory */
         if (IFX_NULL != pFxo->oCID_Msg.message)
         {
            TD_OS_MemFree(pFxo->oCID_Msg.message);
         }
      }
      /* free channel memory */
      TD_OS_MemFree(pFxoDevice->rgoFXO);
      pFxoDevice->rgoFXO = IFX_NULL;
   }
   if (0 <= pFxoDevice->nDevFD)
   {
      /* close device FD */
      TD_OS_DeviceClose(pFxoDevice->nDevFD);
      pFxoDevice->nDevFD = -1;
   }
   pFxoDevice->nNumOfCh = 0;

   return IFX_SUCCESS;
}

/**
   Unmap and release used data channel.

   \param pFXO - pointer to fxo
   \param pBoard - pointer to board

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_ReleaseDataCh(FXO_t* pFXO, BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   TAPIDEMO_DEVICE_FXO_t* pFxoDevice;

   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pFXO->pFxoDevice, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((pFXO->nDataCh == NO_FREE_DATA_CH),
                      pFXO->nDataCh, IFX_ERROR);

   /* get FXO device */
   pFxoDevice = pFXO->pFxoDevice;

   if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
   {
      /* Unmap data channel from fxo channel */
      ret = VOIP_MapPhoneToData(pFXO->nDataCh, 0, pFXO->nFxoCh, IFX_FALSE,
                                pBoard, pFXO->nSeqConnId);
      if (ret != IFX_SUCCESS)
         return IFX_ERROR;
   }
   else
   {
      /* Unmap data channel from pcm channel */
      ret = PCM_MapToData(pFXO->nPCM_Ch, pFXO->nDataCh, IFX_FALSE, pBoard,
                          pFXO->nSeqConnId);
      if (ret != IFX_SUCCESS)
      {
         /* error, could not unmap data ch from pcm ch */
         return ret;
      }
   }


   if (VOIP_FreeDataCh(pFXO->nDataCh, pBoard, pFXO->nSeqConnId) != IFX_SUCCESS)
   {
      /* ERR could not release reserved data ch */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
           ("Err, FXO No %d: %s No %d (ch %d): "
            "Couldn't release data channel %d, %s"
            "(File: %s, line: %d)\n",
            pFXO->nFXO_Number, pFxoDevice->pFxoTypeName, pFxoDevice->nDevNum,
            pFXO->nFxoCh, pFXO->nDataCh, pBoard->pszBoardName,
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
      ("FXO No %d: %s No %d (ch %d): Free data ch: %d, %s\n",
       pFXO->nFXO_Number, pFxoDevice->pFxoTypeName, pFxoDevice->nDevNum,
       pFXO->nFxoCh, pFXO->nDataCh, pBoard->pszBoardName));

   pFXO->nDataCh = NO_FREE_DATA_CH;

   return ret;
}

/**
   Get FXO structure connected to data channel.

   \param nDataCh - number of data chennel
   \param pCtrl   - pointer to control structure

   \return pointer to FXO_t structrue if connected to data channel or
      IFX_NULL if structure not found
   \remarks assumtion is made that data channels from main board are used.

*/
FXO_t* FXO_GetFxoOfDataCh(IFX_int32_t nDataCh, CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t nFxoCh;
   IFX_int32_t nBoard;
   TAPIDEMO_DEVICE_t* pDevice;

   /* check input parameters */
   TD_PARAMETER_CHECK((0 > nDataCh), nDataCh, IFX_NULL);
   TD_PTR_CHECK(pCtrl, IFX_NULL);

   /* for all boards */
   for (nBoard=0; nBoard < pCtrl->nBoardCnt; nBoard++)
   {
      /* get first device */
      pDevice = pCtrl->rgoBoards[nBoard].pDevice;
      /* untill last device is reached */
      while (IFX_NULL != pDevice)
      {
         /* if FXO device */
         if (TAPIDEMO_DEV_FXO == pDevice->nType)
         {
            /* for all FXO channels */
            for (nFxoCh=0; nFxoCh < pDevice->uDevice.stFxo.nNumOfCh; nFxoCh++)
            {
               /* check if using this data channel */
               if (nDataCh == pDevice->uDevice.stFxo.rgoFXO[nFxoCh].nDataCh)
               {
                  return &pDevice->uDevice.stFxo.rgoFXO[nFxoCh];
               }
            } /* for all FXO channels */
         } /* if FXO device */
         pDevice = pDevice->pNext;
      } /* while (IFX_NULL != pDevice) */
   } /* for all boards */
   return IFX_NULL;
}

/**
   Start Call Progress Tone Detection

   \param nDataCh_FD    - data channel file descriptor
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_Start_CPTD(IFX_int32_t nDataCh_FD, IFX_uint32_t nSeqConnId)
{
   IFX_TAPI_TONE_CPTD_t cpt;
   IFX_int32_t aTone[] = { TD_DEFAULT_CPT_TO_DETECT_ON_FXO,
#ifndef TD_3_CPTD_SUPPORT_DISABLED
      RINGBACK_TONE_IDX, DIALTONE_IDX,
#endif /* TD_3_CPTD_SUPPORT_DISABLED */
      0 };
   IFX_int32_t *pTone;
   IFX_int32_t nTone;

   /* check input parameters */
   TD_PARAMETER_CHECK((0 > nDataCh_FD), nDataCh_FD, IFX_ERROR);

   /* get begining of tone */
   pTone = aTone;

   /* until end of array */
   while (0 != *pTone)
   {
      /* use temp buffer that can be changed to custom tone */
      nTone = *pTone;

      memset(&cpt, 0, sizeof(IFX_TAPI_TONE_CPTD_t));

      /* if tone was redefined */
      COM_CHANGE_TONE_IF_CUSTOM_CPT(nTone);

      /* fill up configuration */
      cpt.tone = nTone;
      cpt.signal = IFX_TAPI_TONE_CPTD_DIRECTION_TX;

      if (IFX_SUCCESS != TD_IOCTL(nDataCh_FD, IFX_TAPI_TONE_CPTD_START,
                            (IFX_int32_t) &cpt, TD_DEV_NOT_SET, nSeqConnId))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, nSeqConnId,
               ("Err, failed to start CPT Detector for %s using fd %d. "
                "(File: %s, line: %d)\n",
                Common_Enum2Name(cpt.tone, TD_rgEnumToneIdx),
                (int) nDataCh_FD, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      pTone++;
   } /* until end of array */

   return IFX_SUCCESS;
}

/**
   Stop Call Progress Tone Detection

   \param nDataCh_FD    - data channel file descriptor
   \param nSeqConnId    - Seq Conn ID

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_Stop_CPTD(IFX_int32_t nDataCh_FD, IFX_uint32_t nSeqConnId)
{
   IFX_int32_t nArg = 0;
#ifndef TD_3_CPTD_SUPPORT_DISABLED
   IFX_TAPI_TONE_CPTD_t cpt;
#endif /* TD_3_CPTD_SUPPORT_DISABLED */

   /* check input parameters */
   TD_PARAMETER_CHECK((0 > nDataCh_FD), nDataCh_FD, IFX_ERROR);

#ifndef TD_3_CPTD_SUPPORT_DISABLED
   memset(&cpt, 0, sizeof(IFX_TAPI_TONE_CPTD_t));

   cpt.signal = IFX_TAPI_TONE_CPTD_DIRECTION_TX;

   /* disable tone detection for all tones */
   cpt.tone = 0;

   nArg = (IFX_int32_t) &cpt;
#endif /* TD_3_CPTD_SUPPORT_DISABLED */

   if ( IFX_ERROR == TD_IOCTL(nDataCh_FD, IFX_TAPI_TONE_CPTD_STOP, nArg,
                        TD_DEV_NOT_SET, nSeqConnId))
   {
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Start DTMF detection on data channel - used for CID detection on FXO

   \param pFXO    - pointer to FXO structure
   \param pBoard  - pointer to board structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_DTMF_DetectionStart(FXO_t* pFXO, BOARD_t* pBoard)
{
   IFX_TAPI_SIG_DETECTION_t dtmfDetection = {0};

   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pFXO->nDataCh), pFXO->nDataCh, IFX_ERROR);

   /* check if detection is enabled */
   if (IFX_TRUE == pFXO->oCID_DTMF.nDTMF_DetectionEnabled)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: DTMF detection already enabled."
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* reset digit count */
   pFXO->oCID_DTMF.nCnt = 0;
   memset(pFXO->oCID_DTMF.acDialed, 0, MAX_DIALED_NUM);

   /* Start detection of DTMF tones from local interface */
   dtmfDetection.sig = IFX_TAPI_SIG_DTMFTX;
   if (IFX_SUCCESS == TD_IOCTL(VOIP_GetFD_OfCh(pFXO->nDataCh, pBoard),
                         IFX_TAPI_SIG_DETECT_ENABLE,
                         (IFX_int32_t) &dtmfDetection,
                         TD_DEV_NOT_SET, pFXO->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
         ("FXO No %d: DTMF detection for CID activated.\n",
          pFXO->nFXO_Number));
      pFXO->oCID_DTMF.nDTMF_DetectionEnabled = IFX_TRUE;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: Failed to activate DTMF detection."
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   COM_ITM_TRACES(pBoard->pCtrl, IFX_NULL, IFX_NULL,
                  pFXO, TTP_CID_DTMF_DETECTION_START, pFXO->nSeqConnId);

   return IFX_SUCCESS;
}

/**
   Stop DTMF detection on data channel - used for CID detection on FXO

   \param pFXO    - pointer to FXO structure
   \param pBoard  - pointer to board structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_DTMF_DetectionStop(FXO_t* pFXO, BOARD_t* pBoard)
{
   IFX_TAPI_SIG_DETECTION_t dtmfDetection = {0};

   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= pFXO->nDataCh), pFXO->nDataCh, IFX_ERROR);

   /* check if detection is enabled */
   if (IFX_FALSE == pFXO->oCID_DTMF.nDTMF_DetectionEnabled)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: DTMF detection is not enabled."
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Stop detection of DTMF tones from local interface */
   dtmfDetection.sig = IFX_TAPI_SIG_DTMFTX | IFX_TAPI_SIG_DTMFRX;
   if (IFX_SUCCESS == TD_IOCTL(VOIP_GetFD_OfCh(pFXO->nDataCh, pBoard),
                         IFX_TAPI_SIG_DETECT_DISABLE,
                         (IFX_int32_t) &dtmfDetection,
                         TD_DEV_NOT_SET, pFXO->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
         ("FXO No %d: DTMF detection for CID deactivated.\n",
          pFXO->nFXO_Number));
      pFXO->oCID_DTMF.nDTMF_DetectionEnabled = IFX_FALSE;
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: Failed to deactivate DTMF detection."
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   COM_ITM_TRACES(pBoard->pCtrl, IFX_NULL, IFX_NULL,
                  pFXO, TTP_CID_DTMF_DETECTION_STOP, pFXO->nSeqConnId);

   return IFX_SUCCESS;
}

/**
   Make flashook on FXO channel.

   \param pFXO - pointer to fxo structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_FlashHook(FXO_t* pFXO)
{
   IFX_return_t nRet;

   TD_PTR_CHECK(pFXO, IFX_ERROR);

   nRet = TD_IOCTL(pFXO->nFxoCh_FD, IFX_TAPI_FXO_FLASH_SET, 0,
             TD_DEV_NOT_SET, pFXO->nSeqConnId);

   if (IFX_SUCCESS != nRet)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: Failed to make flash-hook.\n"
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, __FILE__, __LINE__));
   }
   return nRet;
}

/**
   Get phone that is using specified fxo number.

   \param pCtrl   - pointer to status control structure
   \param pFXO - pointer to fxo structure

   \return pointer to phone or IFX_NULL if unable to get phone
*/
PHONE_t* FXO_GetPhone(CTRL_STATUS_t* pCtrl, IFX_int32_t nFxoNum)
{
   IFX_int32_t nBoard, nPhone;
   BOARD_t *pBoard;

   TD_PTR_CHECK(pCtrl, IFX_NULL);

   /* for all boards */
   for (nBoard=0; nBoard < pCtrl->nBoardCnt; nBoard++)
   {
      /* get board pointer */
      pBoard = &pCtrl->rgoBoards[nBoard];
      /* for all phones */
      for (nPhone=0; nPhone < pBoard->nMaxPhones; nPhone++)
      {
         /* check if FXO connected to phone */
         if (IFX_NULL != pBoard->rgoPhones[nPhone].pFXO)
         {
            /* check fxo number */
            if (pBoard->rgoPhones[nPhone].pFXO->nFXO_Number == nFxoNum)
            {
               /* phone was found, return pointer */
               return &pBoard->rgoPhones[nPhone];
            }
         } /* if (IFX_NULL != pBoard->rgoPhones[nPhone].pFXO) */
      } /* for all phones */
   } /* for all board */

   /* failed to get phone */
   return IFX_NULL;
}

/**
   Send DTMF CID detected on FXO.

   \param pFXO - pointer to fxo structure
   \param pBoard  - pointer to board structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_ReceivedCID_DTMF_Send(FXO_t* pFXO, BOARD_t* pBoard)
{
   IFX_TAPI_CID_MSG_t oCID_Msg = {0};
   IFX_TAPI_CID_MSG_ELEMENT_t oCID_MsgElement[TD_CID_ELEM_COUNT];
   IFX_int32_t i;
   IFX_return_t nRet;

   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* reset structure */
   memset(oCID_MsgElement, 0 ,
          sizeof (IFX_TAPI_CID_MSG_ELEMENT_t) * TD_CID_ELEM_COUNT);

   if (0 == pFXO->oCID_DTMF.nCnt)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: No DTMF digits collected."
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (MAX_DIALED_NUM <= pFXO->oCID_DTMF.nCnt)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: Invalid number of DIGITS(%d)."
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->oCID_DTMF.nCnt, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
         ("FXO No %d: Recived CID(DTMF), Collected %d digits: %s.\n",
          pFXO->nFXO_Number, pFXO->oCID_DTMF.nCnt, pFXO->oCID_DTMF.acDialed));

   if (TD_CID_DTMF_DIGIT_CNT_MIN > pFXO->oCID_DTMF.nCnt)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: Number of collected digits is too small %d."
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->oCID_DTMF.nCnt, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* check start digit */
   if ((TD_CID_DTMF_DIGIT_START_A != pFXO->oCID_DTMF.acDialed[0]) &&
       (TD_CID_DTMF_DIGIT_START_D != pFXO->oCID_DTMF.acDialed[0]))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: Invalid start digit %c, expected %c or %c."
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, pFXO->oCID_DTMF.acDialed[0],
          TD_CID_DTMF_DIGIT_START_A, TD_CID_DTMF_DIGIT_START_D,
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   /* check stop digit */
   if (TD_CID_DTMF_DIGIT_STOP_C !=
       pFXO->oCID_DTMF.acDialed[pFXO->oCID_DTMF.nCnt - 1])
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: Invalid stop digit %c, expected %c."
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number,
          pFXO->oCID_DTMF.acDialed[pFXO->oCID_DTMF.nCnt - 1],
          TD_CID_DTMF_DIGIT_STOP_C, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   COM_ITM_TRACES(IFX_NULL, IFX_NULL, IFX_NULL,
                  pFXO, TTP_CID_MSG_RECEIVED_DTMF, pFXO->nSeqConnId);

   /* set CID data */
   oCID_Msg.message = oCID_MsgElement;
   /* set number of elements */
   oCID_Msg.nMsgElements = TD_CID_ELEM_COUNT;

   /* set element type - Date and time presentation,
      it is set right before sending CID */
   oCID_MsgElement[TD_CID_IDX_DATE].date.elementType = IFX_TAPI_CID_ST_DATE;
   /* set caller number */
   oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.elementType = IFX_TAPI_CID_ST_CLI;
   memcpy(oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.element,
          &pFXO->oCID_DTMF.acDialed[1],
          pFXO->oCID_DTMF.nCnt - TD_CID_DTMF_START_STOP_DIGIT_CNT);
   oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.len =
      strlen((IFX_char_t *)oCID_MsgElement[TD_CID_IDX_CLI_NUM].string.element);
   /* set caller name */
   oCID_MsgElement[TD_CID_IDX_NAME].string.elementType = IFX_TAPI_CID_ST_NAME;
   snprintf((IFX_char_t *)oCID_MsgElement[TD_CID_IDX_NAME].string.element,
            TD_MAX_PEER_NAME_LEN, "FXO CALL#%d", pFXO->nFXO_Number);
   /* get string length */
   oCID_MsgElement[TD_CID_IDX_NAME].string.len =
      strlen((IFX_char_t *)oCID_MsgElement[TD_CID_IDX_NAME].string.element);

   /* for all phones - before calling this function number of available phones
      must be checked */
   for (i = 0; i < pBoard->nMaxPhones; i++)
   {
      /* check phone state */
      if (S_READY ==
         pBoard->rgoPhones[i].rgStateMachine[FXS_SM].nState)
      {
         /* change phone state to S_IN_RINGING */
         pBoard->rgoPhones[i].rgoConn[0].nType = FXO_CALL;
         pBoard->rgoPhones[i].nIntEvent_FXS = IE_RINGING;
         pBoard->rgoPhones[i].pFXO = pFXO;
         pBoard->rgoPhones[i].fFXO_Call = IFX_TRUE;
         pBoard->pCtrl->bInternalEventGenerated = IFX_TRUE;
         ABSTRACT_SeqConnID_Set(&pBoard->rgoPhones[i], IFX_NULL,
                                pFXO->nSeqConnId);
         /* if phone cannot play CID (e.g. DECT handset) then ringing
            is started during state transition */
         if (0 < pBoard->nMaxCoderCh)
         {
            nRet = CID_Send(pBoard->rgoPhones[i].nDataCh_FD,
                            &pBoard->rgoPhones[i],
                            IFX_TAPI_CID_HM_ONHOOK, &oCID_Msg, IFX_NULL);
         }
         else
         {
            nRet = CID_Send(pBoard->rgoPhones[i].nPhoneCh_FD,
                            &pBoard->rgoPhones[i],
                            IFX_TAPI_CID_HM_ONHOOK, &oCID_Msg, IFX_NULL);

         } /* if (0 < pBoard->nMaxCoderCh) */
         if (IFX_SUCCESS != nRet)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d: Failed to send CID to Phone No %d. "
                   "(File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pBoard->rgoPhones[i].nPhoneNumber,
                   __FILE__, __LINE__));
         }
      } /* check phone state */
   } /* for all phones */

   return IFX_SUCCESS;
}
/**
   Set new timeout time.

   \param pFXO - pointer to fxo

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_RefreshTimeout(FXO_t* pFXO)
{
   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);

   pFXO->timeRingEventTimeout = TD_OS_ElapsedTimeMSecGet(0);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
      ("FXO No %d: %s No %d (ch %d): "
       "Ringing event occured at %lu msec.\n",
       pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
       pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
       pFXO->timeRingEventTimeout));

   return IFX_SUCCESS;
}

/**
   End FXO connection (stop pcm connection, unmap mapped channels)

   \param pProgramArg - program arguments
   \param pFXO - pointer to fxo
   \param pPhone - pointer to phone
   \param pConn - used connection for fxo
   \param pBoard - pointer of board

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_EndConnection(PROGRAM_ARG_t* pProgramArg,
                               FXO_t* pFXO,
                               PHONE_t* pPhone,
                               CONNECTION_t* pConn,
                               BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   CONNECTION_t* pUsedConn = IFX_NULL;
   TAPIDEMO_PORT_DATA_t oPortData;
   IFX_int32_t nConn, nFxoConnCnt = 0;
   /* Used for checking if WB or NB */
   IFX_boolean_t fWideBand = IFX_FALSE;

   if (TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
   }

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* check if proper FXO state */
   if (S_READY == pFXO->nState)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d invalid  state %s (File: %s, line: %d)\n",
          pFXO->nFXO_Number, Common_Enum2Name(pFXO->nState, TD_rgStateName),
          __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* check if ringing on FXO channel detected */
   if (S_IN_RINGING == pFXO->nState)
   {
      /* connection will be activated, no need for timeout check */
      TD_TIMEOUT_STOP(pBoard->pCtrl->nUseTimeoutDevice);
   }
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pBoard->nType)
   {
      pBoard = TD_GET_MAIN_BOARD(pBoard->pCtrl);
   }
#endif /* EASY3111 */
   /* check if cid enabled and in ringing state */
   if (pBoard->pCtrl->pProgramArg->oArgFlags.nCID)
   {
      /* check if CID receiving is still on */
      if (IFX_TRUE == pFXO->fCID_FSK_Started)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
            ("FXO No %d: %s No %d(ch %d): Stop CID RX FSK.\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh));
         /* Stop FSK and change fxo CID flag state */
         CID_StopFSK(pFXO, VOIP_GetFD_OfCh(pFXO->nDataCh, pBoard), pFXO->nDataCh);
         pFXO->fCID_FSK_Started = IFX_FALSE;
      } /* if (IFX_TRUE == pFXO->fCID_FSK_Started) */
      /* stop DTMF detection */
      if (IFX_FALSE != pFXO->oCID_DTMF.nDTMF_DetectionEnabled)
      {
         FXO_DTMF_DetectionStop(pFXO, pBoard);
      }
   } /* if (pCtrl->pProgramArg->oArgFlags.nCID) */

   /* check phone state, only active FXO went off-hook and started CPDT */
   if (S_ACTIVE == pFXO->nState)
   {
      /* Selected FXO goes to S_READY and we also send hook event back. */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
         ("FXO No %d: %s No %d(ch %d): Set ONHOOK.\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh));

      ret = TD_IOCTL(pFXO->nFxoCh_FD, IFX_TAPI_FXO_HOOK_SET,
               IFX_TAPI_FXO_HOOK_ONHOOK, TD_DEV_NOT_SET, pFXO->nSeqConnId);
      if (IFX_ERROR == ret)
      {
          TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
             ("Err, FXO No %d: %s No %d (ch %d): setting state to on-hook. "
              "(File: %s, line: %d)\n",
              pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
              pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
              __FILE__, __LINE__));
      }

      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:FXO_HOOK_ON::%d\n",(int) pFXO->nFxoCh));

      ret = FXO_Stop_CPTD(VOIP_GetFD_OfCh(pFXO->nDataCh, pBoard),
                          pFXO->nSeqConnId);
      if (IFX_ERROR == ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): "
             "FXO_Stop_CPTD failed on  data ch %d, %s.\n"
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
             pFXO->nDataCh, pBoard->pszBoardName, __FILE__, __LINE__));
      }

      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:CPTD_BUSY_TONE:STOP:%d\n", pFXO->nDataCh));
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:DATA_CHANNEL:RELEASE:%d\n", pFXO->nDataCh));
   } /* if (S_ACTIVE == pFXO->nState) */

   /* Unmap and release used data channel */
   ret = FXO_ReleaseDataCh(pFXO, pBoard);
   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: %s No %d (ch %d): Couldn't unmap data channel %d."
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, pFXO->nDataCh,
          __FILE__, __LINE__));
   }

   /* Check direction of FXO call */
   if ( IFX_TRUE == pFXO->fIncomingFXO )
   {
      /* Note: Incomming call - connection structure is used from FXO
               Outcomming call - connection structure is used from PHONE */
      pUsedConn = &pFXO->oConn;
   }
   else
   {
      pUsedConn = pConn;
   }
   TD_PTR_CHECK(pUsedConn, IFX_ERROR);

   /* check if teridian */
   if (FXO_TYPE_TERIDIAN == pFXO->pFxoDevice->nFxoType)
   {
      /* disable lec on call end */
      if (IFX_ERROR == Common_LEC_On_PCM_Ch(pFXO, pUsedConn->nUsedCh,
                                            IFX_DISABLE, pBoard))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, PCM ch No %d: %s Disable LEC failed. (File: %s, line: %d)\n",
             pUsedConn->nUsedCh, pBoard->pszBoardName, __FILE__, __LINE__));
      }
   }

   if (FXO_TYPE_SLIC121 != pFXO->pFxoDevice->nFxoType)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
            ("FXO No %d: %s No %d (ch %d): Deactivate PCM on ch %d\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, pFXO->nFxoCh));
      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
            ("FXO No %d: %s No %d (ch %d): Deactivate PCM on pcm ch %d %s\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
             pFXO->oConn.nUsedCh, pBoard->pszBoardName));
      /* set structure */
      oPortData.nType = PORT_FXO;
      oPortData.uPort.pFXO = pFXO;
      /* end PCM connection and free PCM timeslots */
      ret = PCM_EndConnection(&oPortData, pProgramArg, pBoard, pUsedConn,
                              pFXO->nSeqConnId);

      if (IFX_ERROR == ret)
      {
          TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
             ("Err, FXO No %d: %s No %d (ch %d): on PCM_EndConnection. "
              "(File: %s, line: %d)\n",
              pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
              pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, __FILE__, __LINE__));
      }

      /* Unset PCM activation on DUSLIC or Teridian */
      ret = PCM_SetActivation(&oPortData, pFXO->nFxoCh_FD, IFX_FALSE,
                              /* fxo uses two PCM channels on one board, for
                                 second channel timeslots must be swaped */
                              pUsedConn->oPCM.nTimeslot_TX,
                              pUsedConn->oPCM.nTimeslot_RX,
                              fWideBand, pFXO->nSeqConnId);

      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:PCM_IF_FXO_DEACTIVE:%d:%d\n",
                   pUsedConn->oPCM.nTimeslot_RX, pFXO->nFxoCh));

      if (IFX_SUCCESS != ret)
      {
          TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
             ("Err, FXO No %d: %s No %d (ch %d): PCM_SetActivation() failed."
              " (File: %s, line: %d)\n",
              pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
              pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, __FILE__, __LINE__));
      }
   } /* if (FXO_TYPE_SLIC121 != pFXO->pFxoDevice->nFxoType) */

   if (IFX_NULL != pPhone && pFXO->nState == S_ACTIVE)
   {
#ifdef TD_DECT
      if (PHONE_TYPE_DECT == pPhone->nType)
      {
         if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
         {
            /* unmap fxo and dect channels. */
            ret = TD_DECT_MapToAnalog(pPhone->nDectCh, pFXO->nFxoCh,
                                      TD_MAP_REMOVE, pBoard, pFXO->nSeqConnId);
            if (IFX_SUCCESS != ret)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d: TD_DECT_MapToAnalog failed. Dect ch: %d, "
                   "Fxo ch: %d (File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pPhone->nDectCh,
                   pFXO->nFxoCh, __FILE__, __LINE__));
               return IFX_ERROR;
            }
         }
         else
         {
            if (IFX_TRUE == pFXO->fIncomingFXO)
            {
               /* for incoming call FXO reserves PCM channel,
                  if not incoming call then DECT phone reserves PCM channel,
                  DECT PCM channel is unmapped in ClearPhone*/
               if (IFX_SUCCESS != TD_DECT_MapToPCM(pPhone->nDectCh,
                                     pUsedConn->nUsedCh, TD_MAP_REMOVE, pBoard,
                                     pFXO->nSeqConnId))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                     ("Err, Phone No %d, TD_DECT_MapToPCM failed\n"
                      "(File: %s, line: %d)\n",
                      pPhone->nPhoneNumber, __FILE__, __LINE__));
               }
            } /* if (IFX_TRUE == pFXO->fIncomingFXO) */
         }
      } /* if (PHONE_TYPE_DECT == pPhone->nType) */
      else
#endif /* TD_DECT */
      {
#ifdef EASY3111
         if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
         {
            /* PCM channel is used to connect to main board,
               do the correct mapping */
            if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
            {
               if (IFX_SUCCESS !=
                   PCM_MapToPhone(pPhone->oEasy3111Specific.nOnMainPCM_Ch,
                      pFXO->nFxoCh, IFX_FALSE, pBoard, pFXO->nSeqConnId))
               {
                  TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                     ("Err, FXO No %d: PCM_MapToPhone() failed. PCM ch: %d, "
                      "Fxo ch: %d (File: %s, line: %d)\n",
                      pFXO->nFXO_Number,
                      pPhone->oEasy3111Specific.nOnMainPCM_Ch,
                      pFXO->nFxoCh, __FILE__, __LINE__));
                  return IFX_ERROR;
               }
            }
            else if (CALL_DIRECTION_TX != pPhone->nCallDirection)
            {
               if (IFX_NULL != pConn)
               {
                  if (IFX_SUCCESS !=
                      PCM_MapToPCM(pPhone->oEasy3111Specific.nOnMainPCM_Ch,
                         pConn->nUsedCh, IFX_FALSE, pBoard, pFXO->nSeqConnId))
                  {
                     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                        ("Err, FXO No %d: PCM_MapToPCM() failed. PCM ch: %d, "
                         "PCM ch: %d (File: %s, line: %d)\n",
                         pFXO->nFXO_Number,
                         pPhone->oEasy3111Specific.nOnMainPCM_Ch,
                         pConn->nUsedCh, __FILE__, __LINE__));
                     return IFX_ERROR;
                  }
               } /* if (IFX_NULL != pConn) */
            }
         }
         else
#endif /* EASY3111 */
         if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
         {
            /* Map phone channel with fxo channel. */
            ret = ALM_MapToPhone(pPhone->nPhoneCh, pFXO->nFxoCh,
                                 TD_MAP_REMOVE, pBoard, pPhone->nSeqConnId);
            if (IFX_SUCCESS != ret)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d: ALM_MapToPhone failed. Phone ch: %d, "
                   "Fxo ch: %d (File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pPhone->nPhoneCh,
                   pFXO->nFxoCh, __FILE__, __LINE__));
               return IFX_ERROR;
            }
            COM_MOD_FXO(pFXO->nSeqConnId,
                        ("FXO_TEST:FXO2PHONE_FXO:UNMAP:%d\n",
                         pFXO->nFxoCh));
            COM_MOD_FXO(pFXO->nSeqConnId,
                        ("FXO_TEST:FXO2PHONE_PHONE:UNMAP:%d\n",
                         pPhone->nPhoneCh));
         }
         else
         {
            /* UnMap pcm channel with phone channel */
            ret = PCM_MapToPhone(pUsedConn->nUsedCh, pPhone->nPhoneCh,
                                 IFX_FALSE, pBoard, pFXO->nSeqConnId);

            COM_MOD_FXO(pFXO->nSeqConnId,
                        ("FXO_TEST:PCM2PHONE_PCM:UNMAP:%d\n",
                         pUsedConn->nUsedCh));
            COM_MOD_FXO(pFXO->nSeqConnId,
                        ("FXO_TEST:PCM2PHONE_PHONE:UNMAP:%d\n",
                         pPhone->nPhoneCh));
         }
      }
   } /* if (IFX_NULL != pPhone && pFXO->nState == S_ACTIVE) */

   if (FXO_TYPE_SLIC121 != pFXO->pFxoDevice->nFxoType)
   {
      if (pFXO->fIncomingFXO == IFX_TRUE )
      {
         /* Free the pcm channel in case of incoming call. If there is
            the outcoming call the pcm channel should NOT be free
            becasue it is assigned to the phone channel. */
         TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
            ("FXO No %d: %s No %d (ch %d): Free pcm ch %d, %s\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
             pFXO->oConn.nUsedCh, pBoard->pszBoardName));
         PCM_FreeCh(pFXO->oConn.nUsedCh, pBoard);

         COM_MOD_FXO(pFXO->nSeqConnId,
                     ("FXO_TEST:PCM_CHANNEL:RELEASE:%d\n",
                      pFXO->oConn.nUsedCh));
      }
   } /* if (FXO_TYPE_SLIC121 != pFXO->pFxoDevice->nFxoType) */

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
      ("FXO No %d: %s No %d (ch %d): State changed to S_READY \n",
       pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
       pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh));
   pFXO->nState = S_READY;

#ifdef EASY3111
   if (IFX_NULL != pPhone)
   {
      /* EASY 3111 specific actions */
      if (IFX_ERROR == Easy3111_ClearPhone(pPhone, pBoard->pCtrl,
                                           pUsedConn))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, Phone No %d: Easy3111_ClearPhone() failed.\n"
             "(File: %s, line: %d)\n",
             pPhone->nPhoneNumber, __FILE__, __LINE__));
      }
   }
#endif /* EASY3111 */
   if (IFX_NULL != pPhone)
   {
      /* check if this is the last FXO connection for this phone  */
      if (IFX_TRUE == pFXO->fIncomingFXO)
      {
         /* reset the flag */
         pPhone->fFXO_Call = IFX_FALSE;
      } /* if (IFX_TRUE == pFXO->fIncomingFXO) */
      else
      {
         /* count FXO calls for this phone */
         for (nConn = 0; nConn < pPhone->nConnCnt; nConn++)
         {
            if (FXO_CALL == pPhone->rgoConn[nConn].nType)
            {
               nFxoConnCnt++;
            }
         }
         /* if there is no more FXO connections then reset th flag */
         if (nFxoConnCnt < 2)
         {
            pPhone->fFXO_Call = IFX_FALSE;
         }
      } /* if (IFX_TRUE == pFXO->fIncomingFXO) */
   } /* if (IFX_NULL != pPhone) */
   if (IFX_NULL != pConn)
   {
      pConn->nType = UNKNOWN_CALL_TYPE;
      pConn->fActive = IFX_FALSE;
   } /* if (IFX_NULL != pConn) */
   pUsedConn->oPCM.nTimeslot_RX = TIMESLOT_NOT_SET;
   pUsedConn->oPCM.nTimeslot_TX = TIMESLOT_NOT_SET;
   pFXO->fFirstStopRingEventOccured = IFX_FALSE;
   pFXO->oCID_DTMF.nCnt = 0;
   memset(pFXO->oCID_DTMF.acDialed, 0, MAX_DIALED_NUM);
   /* reset direction marker on end of connection */
   pFXO->fIncomingFXO = IFX_FALSE;
   if (IFX_NULL != pPhone)
   {
      pPhone->pFXO = IFX_NULL;
   } /* if (IFX_NULL != pPhone) */
   ABSTRACT_SeqConnID_Reset(IFX_NULL, pFXO);

   return ret;
}

/**
   Start FXO connection (start pcm connection, map channels)

   \param pProgramArg - program arguments
   \param pFXO - pointer to fxo
   \param pPhone - pointer to phone
   \param pConn - used connection for fxo
   \param pBoard - pointer of board

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_StartConnection(PROGRAM_ARG_t* pProgramArg,
                                 FXO_t* pFXO,
                                 PHONE_t* pPhone,
                                 CONNECTION_t* pConn,
                                 BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   TAPIDEMO_PORT_DATA_t oPortData;
   /* Used for checking if WB or NB */
   IFX_boolean_t fWideBand = IFX_FALSE;

   if (TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
   }
   /* All the other phones went to S_READY, selected FXO
      goes to S_ACTIVE and we also send hook event back. */

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* check if proper FXO state */
   if (S_READY != pFXO->nState)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d invalid  state %s, should be state %s"
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number,
             Common_Enum2Name(pFXO->nState, TD_rgStateName),
             Common_Enum2Name(S_READY, TD_rgStateName),
             __FILE__, __LINE__));
      return IFX_ERROR;
   }

   ABSTRACT_SeqConnID_Set(IFX_NULL, pFXO, pPhone->nSeqConnId);
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
   {
      /* for DuSLIC-xT channels on main are used in this function */
      pBoard = TD_GET_MAIN_BOARD(pBoard->pCtrl);
   }
#endif /* EASY3111 */

   ret = TD_IOCTL(pFXO->nFxoCh_FD, IFX_TAPI_FXO_HOOK_SET,
            IFX_TAPI_FXO_HOOK_OFFHOOK, TD_DEV_NOT_SET, pFXO->nSeqConnId);
   if (IFX_ERROR == ret)
   {
       TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): setting state to off-hook.. "
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, __FILE__, __LINE__));
       return IFX_ERROR;
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
      ("FXO No %d: %s No %d (ch %d):  Set OFFHOOK.\n",
       pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
       pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh));

   COM_MOD_FXO(pFXO->nSeqConnId,
               ("FXO_TEST:FXO_CHANNEL:SET:%d\n", pFXO->nFxoCh));
   COM_MOD_FXO(pFXO->nSeqConnId,
               ("FXO_TEST:FXO_HOOK_OFF::%d\n", pFXO->nFxoCh));

   pFXO->nDataCh = VOIP_GetFreeDataCh(pBoard, pFXO->nSeqConnId);
   if (pFXO->nDataCh == NO_FREE_DATA_CH)
   {
      /* ERR could not get free data ch */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: %s No %d (ch %d): No data channel available on %s."
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, pBoard->pszBoardName,
          __FILE__, __LINE__));
      return IFX_ERROR;
   }
   if (IFX_SUCCESS !=  VOIP_ReserveDataCh(pFXO->nDataCh, pBoard,
                                          pFXO->nSeqConnId))
   {
      /* ERR could not reserve data ch */
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: %s No %d (ch %d): Couldn't reserve data ch %d, %s."
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
          pFXO->nDataCh, pBoard->pszBoardName, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   COM_MOD_FXO(pFXO->nSeqConnId,
               ("FXO_TEST:DATA_CHANNEL:SET:%d\n", pFXO->nDataCh));

   if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
   {
      /* Map fxo (analog) channel with data channel */
      ret = VOIP_MapPhoneToData(pFXO->nDataCh, 0, pFXO->nFxoCh, IFX_TRUE,
                                pBoard, pFXO->nSeqConnId);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): "
             "Couldn't map fxo channel %d to data channel %d, %s."
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
             pFXO->nFxoCh, pFXO->nDataCh, pBoard->pszBoardName,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:DATA2FXO_DATA:MAP:%d\n", pFXO->nDataCh));
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:DATA2FXO_FXO:MAP:%d\n", pFXO->nFxoCh));
   }
   else
   {
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:PCM_CHANNEL:SET:%d\n", pConn->nUsedCh));
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:PCM_RELEASE:NO:%d\n", pConn->nUsedCh));
      /* Map pcm channel with data channel */
      ret = PCM_MapToData(pConn->nUsedCh, pFXO->nDataCh, IFX_TRUE, pBoard,
                          pFXO->nSeqConnId);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): "
             "Couldn't map data channel %d to pcm channel %d, %s."
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
             pFXO->nDataCh, pConn->nUsedCh, pBoard->pszBoardName,
             __FILE__, __LINE__));
         return IFX_ERROR;
      }
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:DATA2PCM_DATA:MAP:%d\n", pFXO->nDataCh));
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:DATA2PCM_PCM:MAP:%d\n", pConn->nUsedCh));
   }

   /* Start Call Progress Tone Detection for BUSY_TONE */
   if (IFX_SUCCESS != FXO_Start_CPTD(VOIP_GetFD_OfCh(pFXO->nDataCh, pBoard),
                                     pFXO->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d: %s No %d (ch %d): "
          "Couldn't start FXO_Start_CPTD on  data ch %d, %s.\n"
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
          pFXO->nDataCh, pBoard->pszBoardName, __FILE__, __LINE__));
   }
   else
   {
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:CPTD_BUSY_TONE:START:%d\n", pFXO->nDataCh));
   }

   if (FXO_TYPE_SLIC121 != pFXO->pFxoDevice->nFxoType)
   {
      /* Get timeslots */
      pConn->oPCM.nTimeslot_RX = PCM_GetTimeslots(TS_RX, fWideBand,
                                                  pFXO->nSeqConnId);
      pConn->oPCM.nTimeslot_TX = pConn->oPCM.nTimeslot_RX;

      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:PCM_IF_ID:SET:%d\n", pConn->oPCM.nTimeslot_RX));

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
         ("FXO No %d: %s No %d (ch %d): Activate PCM on ch %d\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, pFXO->nFxoCh));

      /* set structure */
      oPortData.nType = PORT_FXO;
      oPortData.uPort.pFXO = pFXO;

      /* Set PCM activation on DUSLIC or Teridian*/
      ret = PCM_SetActivation(&oPortData, pFXO->nFxoCh_FD, IFX_TRUE,
                              /* fxo uses two PCM channels on one board, for
                              second (this) channel timeslots must be swapped */
                              pConn->oPCM.nTimeslot_TX,
                              pConn->oPCM.nTimeslot_RX,
                              fWideBand, pFXO->nSeqConnId);
      if (IFX_SUCCESS != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): on PCM_SetActivation(). "
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:PCM_IF_FXO_ACTIVE:%d:%d\n",
                   pConn->oPCM.nTimeslot_RX, pFXO->nFxoCh));

      TD_TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, pFXO->nSeqConnId,
         ("FXO No %d: %s No %d (ch %d): Activate PCM on pcm ch %d, %s\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
          pConn->nUsedCh, pBoard->pszBoardName));

      if (IFX_TRUE != PCM_StartConnection(&oPortData, pProgramArg,
                                          pBoard, pConn, pFXO->nSeqConnId))
      {
          TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
             ("Err, FXO No %d: %s No %d (ch %d): on PCM_StartConnection()."
              "(File: %s, line: %d)\n",
              pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
              pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, __FILE__, __LINE__));
          return IFX_ERROR;
      }
   }

#ifdef TERIDIAN_FXO
   /* check if teridian */
   if (FXO_TYPE_TERIDIAN == pFXO->pFxoDevice->nFxoType)
   {
      /* enable LEC on PCM channel */
      if (IFX_ERROR == Common_LEC_On_PCM_Ch(pFXO, pConn->nUsedCh,
                                            IFX_ENABLE, pBoard))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, PCM ch No %d: %s Enable LEC failed. (File: %s, line: %d)\n",
             pConn->nUsedCh, pBoard->pszBoardName, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
#endif /* TERIDIAN_FXO */

   COM_MOD_FXO(pFXO->nSeqConnId,
               ("FXO_TEST:PHONE_CHANNEL:SET:%d\n", pPhone->nPhoneCh));

#ifdef TD_DECT
   /* for DECT mapping was already done */
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      /* if connection started by pressing talk key and then dialing number,
         then DECT channel was already activated */
      if (IFX_TRUE != pPhone->bDectChActivated)
      {
         /* enable DECT connection */
         ret = TD_DECT_PhoneActivate(pPhone, pConn);
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
               ("Err, Phone No %d: TD_DECT_PhoneActivate failed. "
                "(File: %s, line: %d)\n",
                pPhone->nPhoneNumber, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
      if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
      {
         /* Map fxo channel with dect channel. */
         ret = TD_DECT_MapToAnalog(pPhone->nDectCh, pFXO->nFxoCh, TD_MAP_ADD,
                                   pBoard, pFXO->nSeqConnId);
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
               ("Err, FXO No %d: TD_DECT_MapToAnalog failed. Dect ch: %d, "
                "Fxo ch: %d (File: %s, line: %d)\n",
                pFXO->nFXO_Number, pPhone->nDectCh,
                pFXO->nFxoCh, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
   } /* if (PHONE_TYPE_DECT == pPhone->nType) */
   else
#endif /* TD_DECT */
   {
#ifdef EASY3111
      if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
      {
         /* for other FXO types mapping is already done */
         if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
         {
            if (IFX_SUCCESS != PCM_MapToPhone(pConn->nUsedCh, pFXO->nFxoCh,
                                  IFX_TRUE, TD_GET_MAIN_BOARD(pBoard->pCtrl),
                                  pFXO->nSeqConnId))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d:  PCM %d mapping to ALM %d failed. "
                   "(File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pConn->nUsedCh, pFXO->nFxoCh,
                   __FILE__, __LINE__));
               return IFX_ERROR;
            }
         }
      }
      else
#endif /* EASY3111 */
      {
         if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
         {
            /* Map phone channel with fxo channel. */
            ret = ALM_MapToPhone(pPhone->nPhoneCh, pFXO->nFxoCh,
                                 TD_MAP_ADD, pBoard, pPhone->nSeqConnId);
            if (IFX_SUCCESS != ret)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d: ALM_MapToPhone failed. Phone ch: %d, "
                   "Fxo ch: %d (File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pPhone->nPhoneCh,
                   pFXO->nFxoCh, __FILE__, __LINE__));
               return IFX_ERROR;
            }
            COM_MOD_FXO(pFXO->nSeqConnId,
                        ("FXO_TEST:FXO2PHONE_FXO:MAP:%d\n", pFXO->nFxoCh));
            COM_MOD_FXO(pFXO->nSeqConnId,
                        ("FXO_TEST:FXO2PHONE_PHONE:MAP:%d\n", pPhone->nPhoneCh));
         }
         else
         {
            /* Map pcm channel with phone channel of initiator */
            ret = PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh,
                     IFX_TRUE, pBoard, pFXO->nSeqConnId);
            if (IFX_SUCCESS != ret)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d: PCM_MapToPhone failed. PCM ch: %d, "
                   "Phone ch: %d (File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pConn->nUsedCh,
                   pPhone->nPhoneCh, __FILE__, __LINE__));
               return IFX_ERROR;
            }
            COM_MOD_FXO(pFXO->nSeqConnId,
                        ("FXO_TEST:PCM2PHONE_PCM:MAP:%d\n", pConn->nUsedCh));
            COM_MOD_FXO(pFXO->nSeqConnId,
                        ("FXO_TEST:PCM2PHONE_PHONE:MAP:%d\n", pPhone->nPhoneCh));
         }
      }
   }



   /* Tell, that we are making FXO connection */
   pPhone->fFXO_Call = IFX_TRUE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
      ("FXO No %d: %s No %d (ch %d): State changed to S_ACTIVE\n",
       pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
       pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh));
   pConn->fActive = IFX_TRUE;
   /* State FXO went S_ACTIVE. */
   pFXO->nState = S_ACTIVE;

   /* reset direction marker on end of connection */
   pFXO->fIncomingFXO = IFX_FALSE;

   pFXO->nPCM_Ch = pConn->nUsedCh;

   return ret;
}


/**
   Prepare FXO connection (start pcm connection between FXO and pcm ch,
                           map data channel)

   \param pProgramArg - program arguments
   \param pFXO - pointer to fxo
   \param pBoard - pointer of board

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_PrepareConnection(PROGRAM_ARG_t* pProgramArg,
                                   FXO_t* pFXO,
                                   BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;
   TAPIDEMO_PORT_DATA_t oPortData;
   /* Used for checking if WB or NB */
   IFX_boolean_t fWideBand = IFX_FALSE;

   if (TD_WIDEBAND == g_oPCM_Data.nBand)
   {
      fWideBand = IFX_TRUE;
   }

   /* check input parameters */
   TD_PTR_CHECK(pProgramArg, IFX_ERROR);
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* check if proper FXO state */
   if (S_READY != pFXO->nState)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
         ("Err, FXO No %d invalid  state %s, should be state %s"
          "(File: %s, line: %d)\n",
          pFXO->nFXO_Number,
          Common_Enum2Name(pFXO->nState, TD_rgStateName),
          Common_Enum2Name(S_READY, TD_rgStateName),
          __FILE__, __LINE__));
      return IFX_ERROR;
   }

   ABSTRACT_SeqConnID_Get(IFX_NULL, pFXO, pBoard->pCtrl);

   COM_MOD_FXO(pFXO->nSeqConnId,
               ("FXO_TEST:FXO_CHANNEL:SET:%d\n", pFXO->nFxoCh));

   pFXO->oConn.nType = FXO_CALL;
   /* set FXO direction marker */
   pFXO->fIncomingFXO = IFX_TRUE;
   /* Timeslots are not set */
   pFXO->oConn.oPCM.nTimeslot_RX = TIMESLOT_NOT_SET;
   pFXO->oConn.oPCM.nTimeslot_TX = TIMESLOT_NOT_SET;

   pFXO->nDataCh = VOIP_GetFreeDataCh(pBoard, pFXO->nSeqConnId);
   if (pFXO->nDataCh == NO_FREE_DATA_CH)
   {
       /* ERR could not get free data ch */
       TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
          ("Err, FXO No %d: %s No %d (ch %d): No data channel available on %s."
           "(File: %s, line: %d)\n",
           pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
           pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, pBoard->pszBoardName,
           __FILE__, __LINE__));
       return IFX_ERROR;
   }

   if (IFX_SUCCESS != VOIP_ReserveDataCh(pFXO->nDataCh, pBoard,
                                         pFXO->nSeqConnId))
   {
       /* ERR could not reserve data ch */
       TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
          ("Err, FXO No %d: %s No %d (ch %d): Could not reserve data channel %d, %s."
           "(File: %s, line: %d)\n",
           pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
           pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
           pFXO->nDataCh, pBoard->pszBoardName, __FILE__, __LINE__));
       return IFX_ERROR;
    }

   COM_MOD_FXO(pFXO->nSeqConnId,
               ("FXO_TEST:DATA_CHANNEL:SET:%d\n", pFXO->nDataCh));

   if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
   {
      /* Map fxo (analog) channel with data channel */
      ret = VOIP_MapPhoneToData(pFXO->nDataCh, 0, pFXO->nFxoCh, IFX_TRUE,
                                pBoard, pFXO->nSeqConnId);
      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("FXO No %d: %s No %d (ch %d): "
             "Err, Could not map fxo channel %d to data channel %d, %s."
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
             pFXO->nFxoCh, pFXO->nDataCh, pBoard->pszBoardName,
             __FILE__, __LINE__));
         VOIP_FreeDataCh(pFXO->nDataCh, pBoard, pFXO->nSeqConnId);
         return IFX_ERROR;
      }
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:DATA2FXO_DATA:MAP:%d\n", pFXO->nDataCh));
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:DATA2FXO_FXO:MAP:%d\n", pFXO->nFxoCh));
   }
   else
   {
      /* Get free pcm channel */
      pFXO->nPCM_Ch = PCM_GetFreeCh(pBoard);
      if (pFXO->nPCM_Ch == NO_FREE_PCM_CH)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): Err, No free PCM channel on %s. "
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, pBoard->pszBoardName,
             __FILE__, __LINE__));
         VOIP_FreeDataCh(pFXO->nDataCh, pBoard, pFXO->nSeqConnId);
         return IFX_ERROR;
      }

      if (PCM_ReserveCh(pFXO->nPCM_Ch, pBoard, pFXO->nSeqConnId) != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): Err, Could not reserve PCM ch %d, %s. "
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
             pFXO->nPCM_Ch, pBoard->pszBoardName, __FILE__, __LINE__));
         VOIP_FreeDataCh(pFXO->nDataCh, pBoard, pFXO->nSeqConnId);
         return IFX_ERROR;
      }

      pFXO->oConn.nUsedCh = pFXO->nPCM_Ch;
      pFXO->oConn.nUsedCh_FD = PCM_GetFD_OfCh(pFXO->nPCM_Ch, pBoard);

      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:PCM_CHANNEL:SET:%d\n", pFXO->oConn.nUsedCh));
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:PCM_RELEASE:YES:%d\n", pFXO->oConn.nUsedCh));

      /* Map pcm channel with data channel */
      ret = PCM_MapToData(pFXO->oConn.nUsedCh, pFXO->nDataCh, IFX_TRUE, pBoard,
                          pFXO->nSeqConnId);

      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): "
             "Could not map data channel %d to pcm channel %d, %s."
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
             pFXO->nDataCh, pFXO->oConn.nUsedCh, pBoard->pszBoardName,
             __FILE__, __LINE__));
         PCM_FreeCh(pFXO->oConn.nUsedCh, pBoard);
         VOIP_FreeDataCh(pFXO->nDataCh, pBoard, pFXO->nSeqConnId);
         return IFX_ERROR;
      }
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:DATA2PCM_DATA:MAP:%d\n", pFXO->nDataCh));
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:DATA2PCM_PCM:MAP:%d\n", pFXO->oConn.nUsedCh));
   } /* if (FXO_TYPE_SLIC121 != pFXO->pFxoDevice->nFxoType) */

   if (FXO_TYPE_SLIC121 != pFXO->pFxoDevice->nFxoType)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
         ("FXO No %d: %s No %d (ch %d): Activate PCM on ch %d\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
          pFXO->nFxoCh));

      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
         ("FXO No %d: %s No %d (ch %d): Activate PCM on pcm ch %d, %s\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
          pFXO->nPCM_Ch, pBoard->pszBoardName));

      /* Get timeslots */
      pFXO->oConn.oPCM.nTimeslot_RX = PCM_GetTimeslots(TS_RX, fWideBand,
                                                       pFXO->nSeqConnId);
      pFXO->oConn.oPCM.nTimeslot_TX = pFXO->oConn.oPCM.nTimeslot_RX;

      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:PCM_IF_ID:SET:%d\n",
                   pFXO->oConn.oPCM.nTimeslot_RX));

      /* set structure */
      oPortData.nType = PORT_FXO;
      oPortData.uPort.pFXO = pFXO;
      /* Set PCM activation on FXO */
      ret = PCM_SetActivation(&oPortData, pFXO->nFxoCh_FD, IFX_TRUE,
                              /* fxo uses two PCM channels on one board, for
                               second channel timeslots must be swaped */
                              pFXO->oConn.oPCM.nTimeslot_TX,
                              pFXO->oConn.oPCM.nTimeslot_RX,
                              fWideBand, pFXO->nSeqConnId);

      if (ret != IFX_SUCCESS)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, PCM_SetActivation() failed. (File: %s, line: %d)\n",
             __FILE__, __LINE__));
         /* error, could not activate pcm */
         return IFX_ERROR;
      }
      /* set PCM active flag */
      pFXO->oConn.fPCM_Active = IFX_TRUE;

      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:PCM_IF_FXO_ACTIVE:%d:%d\n",
                   pFXO->oConn.oPCM.nTimeslot_RX, pFXO->nFxoCh));

      if (IFX_TRUE != PCM_StartConnection(&oPortData, pProgramArg, pBoard,
                                          &pFXO->oConn, pFXO->nSeqConnId))
      {
         ret = IFX_ERROR;
      }
   } /* if (FXO_TYPE_SLIC121 != pFXO->pFxoDevice->nFxoType) */

   /* check if teridian */
   if (FXO_TYPE_TERIDIAN == pFXO->pFxoDevice->nFxoType)
   {
      /* enable LEC on PCM channel */
      if (IFX_ERROR == Common_LEC_On_PCM_Ch(pFXO, pFXO->oConn.nUsedCh,
                                            IFX_ENABLE, pBoard))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, PCM ch No %d: %s Enable LEC failed. (File: %s, line: %d)\n",
             pFXO->oConn.nUsedCh, pBoard->pszBoardName, __FILE__, __LINE__));
      }
   }

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
      ("FXO No %d: %s No %d (ch %d): State changed to S_IN_RINGING\n",
       pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
       pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh));
   pFXO->nState = S_IN_RINGING;

   /* use timeout to check if ringing timeout occured */
   TD_TIMEOUT_START(pBoard->pCtrl->nUseTimeoutDevice);

   return ret;
}

/**
   Activate FXO connection (map phone channel to it)

   \param pFXO - pointer to fxo
   \param pPhone - pointer to phone
   \param pConn - used connection for fxo
   \param pBoard - pointer of board

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t FXO_ActivateConnection(FXO_t* pFXO,
                                    PHONE_t* pPhone,
                                    CONNECTION_t* pConn,
                                    BOARD_t* pBoard)
{
   IFX_return_t ret = IFX_SUCCESS;

   /* check input parameters */
   TD_PTR_CHECK(pFXO, IFX_ERROR);
   TD_PTR_CHECK(pPhone, IFX_ERROR);
   TD_PTR_CHECK(pConn, IFX_ERROR);
   TD_PTR_CHECK(pBoard, IFX_ERROR);

   /* check if proper FXO state */
   if (S_IN_RINGING != pFXO->nState)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d invalid  state %s, should be state %s"
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number,
             Common_Enum2Name(pFXO->nState, TD_rgStateName),
             Common_Enum2Name(S_IN_RINGING, TD_rgStateName),
             __FILE__, __LINE__));
      return IFX_ERROR;
   }
#ifdef EASY3111
   if (TYPE_DUSLIC_XT == pBoard->nType)
   {
      /* for DuSLIC-xT channels from main board are used in this function */
      pBoard = TD_GET_MAIN_BOARD(pBoard->pCtrl);
   }
#endif /* EASY3111 */
   /* timeout for FXO cahnnel no longer needed */
   TD_TIMEOUT_STOP(pBoard->pCtrl->nUseTimeoutDevice);

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
         ("FXO No %d: %s No %d (ch %d): "
          "Set OFFHOOK.\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh));

   ret = TD_IOCTL(pFXO->nFxoCh_FD, IFX_TAPI_FXO_HOOK_SET,
            IFX_TAPI_FXO_HOOK_OFFHOOK, TD_DEV_NOT_SET, pFXO->nSeqConnId);
   if (IFX_ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): setting state to off-hook. "
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   COM_MOD_FXO(pFXO->nSeqConnId,
               ("FXO_TEST:FXO_HOOK_OFF::%d\n", pFXO->nFxoCh));
   COM_MOD_FXO(pFXO->nSeqConnId,
               ("FXO_TEST:PHONE_CHANNEL:SET:%d\n", pPhone->nPhoneCh));

#ifdef TD_DECT
   if (PHONE_TYPE_DECT == pPhone->nType)
   {
      if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
      {
         /* Map fxo channel with dect channel. */
         ret = TD_DECT_MapToAnalog(pPhone->nDectCh, pFXO->nFxoCh, TD_MAP_ADD,
                                   pBoard, pFXO->nSeqConnId);
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d: TD_DECT_MapToAnalog failed. Dect ch: %d, "
                   "Fxo ch: %d (File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pPhone->nDectCh,
                   pFXO->nFxoCh, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
      else
      {
         /* map DECT channel to PCM channel */
         ret = TD_DECT_MapToPCM(pPhone->nDectCh, pConn->nUsedCh,
                                TD_MAP_ADD, pBoard, pFXO->nSeqConnId);
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d: TD_DECT_MapToPCM failed. Dect ch: %d, "
                   "PCM ch: %d (File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pPhone->nDectCh,
                   pConn->nUsedCh, __FILE__, __LINE__));
            return IFX_ERROR;
         }
      }
   } /* if (PHONE_TYPE_DECT == pPhone->nType) */
   else
#endif /* TD_DECT */
   {
#ifdef EASY3111
      if (TYPE_DUSLIC_XT == pPhone->pBoard->nType)
      {
         /* do mapping with PCM connected with extension board */
         if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
         {
            if (IFX_SUCCESS !=
                PCM_MapToPhone(pPhone->oEasy3111Specific.nOnMainPCM_Ch,
                   pFXO->nFxoCh, IFX_TRUE, pBoard, pFXO->nSeqConnId))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                     ("Err, FXO No %d: PCM_MapToPhone() failed. PCM ch: %d, "
                      "Fxo ch: %d (File: %s, line: %d)\n",
                      pFXO->nFXO_Number, pPhone->oEasy3111Specific.nOnMainPCM_Ch,
                      pFXO->nFxoCh, __FILE__, __LINE__));
               return IFX_ERROR;
            }
         }
         else
         {
            if (IFX_SUCCESS !=
                PCM_MapToPCM(pPhone->oEasy3111Specific.nOnMainPCM_Ch,
                   pConn->nUsedCh, IFX_TRUE, pBoard, pFXO->nSeqConnId))
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                     ("Err, FXO No %d: PCM_MapToPCM() failed. PCM ch: %d, "
                      "PCM ch: %d (File: %s, line: %d)\n",
                      pFXO->nFXO_Number, pPhone->oEasy3111Specific.nOnMainPCM_Ch,
                      pConn->nUsedCh, __FILE__, __LINE__));
               return IFX_ERROR;
            }
         }
      }
      else
#endif /* EASY3111 */
      if (FXO_TYPE_SLIC121 == pFXO->pFxoDevice->nFxoType)
      {
         /* Map phone channel with fxo channel. */
         ret = ALM_MapToPhone(pPhone->nPhoneCh, pFXO->nFxoCh,
                              TD_MAP_ADD, pBoard, pPhone->nSeqConnId);
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d: ALM_MapToPhone failed. Phone ch: %d, "
                   "Fxo ch: %d (File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pPhone->nPhoneCh,
                   pFXO->nFxoCh, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         COM_MOD_FXO(pFXO->nSeqConnId,
                     ("FXO_TEST:FXO2PHONE_FXO:MAP:%d\n", pFXO->nFxoCh));
         COM_MOD_FXO(pFXO->nSeqConnId,
                     ("FXO_TEST:FXO2PHONE_PHONE:MAP:%d\n", pPhone->nPhoneCh));
      }
      else
      {
         /* Map pcm channel with phone channel of initiator */
         ret = PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh, IFX_TRUE,
                  pBoard, pFXO->nSeqConnId);
         if (IFX_SUCCESS != ret)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
                  ("Err, FXO No %d: PCM_MapToPhone failed. PCM ch: %d, "
                   "Phone ch: %d (File: %s, line: %d)\n",
                   pFXO->nFXO_Number, pConn->nUsedCh,
                   pPhone->nPhoneCh, __FILE__, __LINE__));
            return IFX_ERROR;
         }
         COM_MOD_FXO(pFXO->nSeqConnId,
                     ("FXO_TEST:PCM2PHONE_PCM:MAP:%d\n", pConn->nUsedCh));
         COM_MOD_FXO(pFXO->nSeqConnId,
                     ("FXO_TEST:PCM2PHONE_PHONE:MAP:%d\n", pPhone->nPhoneCh));
      }
   }

   /* Start Call Progress Tone Detection for BUSY_TONE */
   if (IFX_SUCCESS != FXO_Start_CPTD(VOIP_GetFD_OfCh(pFXO->nDataCh, pBoard),
                                     pFXO->nSeqConnId))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, pFXO->nSeqConnId,
            ("Err, FXO No %d: %s No %d (ch %d): "
             "Couldn't start FXO_Start_CPTD on  data ch %d, %s.\n"
             "(File: %s, line: %d)\n",
             pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
             pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh,
             pFXO->nDataCh, pBoard->pszBoardName, __FILE__, __LINE__));
   }
   else
   {
      COM_MOD_FXO(pFXO->nSeqConnId,
                  ("FXO_TEST:CPTD_BUSY_TONE:START:%d\n", pFXO->nDataCh));
   }

   /* Tell, that we are making FXO connection */
   pPhone->fFXO_Call = IFX_TRUE;

   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, pFXO->nSeqConnId,
         ("FXO No %d: %s No %d (ch %d): State changed to S_ACTIVE\n",
          pFXO->nFXO_Number, pFXO->pFxoDevice->pFxoTypeName,
          pFXO->pFxoDevice->nDevNum, pFXO->nFxoCh));
   pFXO->nState = S_ACTIVE;

   return ret;
}

#endif /* FXO */
/** ---------------------------------------------------------------------- */



