
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_slic121_fxo.c
   \date 2010-07-20
   \brief This file contain implementation of SLIC121 FXO.

   It initializes the FXO channels on SLIC121.
*/

#include "ifx_types.h"
#ifdef LINUX
#include "tapidemo_config.h"
#endif

#ifdef FXO
#ifdef SLIC121_FXO
/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef HAVE_DRV_TAPI_HEADERS
   #include "drv_tapi_io.h"
#else
    #error "drv_tapi is missing"
#endif /* HAVE_DRV_TAPI_HEADERS */

#ifdef HAVE_DRV_VMMC_HEADERS
    #include "vmmc_io.h"
#else
    #error "drv_vmmc is missing"
#endif

#include "tapidemo.h"

#include "td_slic121_fxo.h"

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local defies                  */
/* ============================= */

/** BBD file ending when it is fxs specific */
#define TD_BBD_SUFIX_FXS    "fxs.bin"
/** BBD file ending when it is fxo specific */
#define TD_BBD_SUFIX_FXO    "fxo.bin"

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

/**
   Initialize the FXO channel in SLIC121

   \param pFxoDevice - pointer to FXO device structure
   \param psDownloadPath - string with download path
   \param pFxo - pointer to single FXO channel structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_SLIC121_FXO_InitChannel(TAPIDEMO_DEVICE_FXO_t *pFxoDevice,
                                        IFX_char_t* psDownloadPath,
                                        FXO_t* pFxo)
{
   IFX_uint32_t ret = IFX_SUCCESS;
   IFX_TAPI_FXO_LINE_MODE_t lineMode;
   TAPIDEMO_DEVICE_t *pDevice = IFX_NULL;
   TAPIDEMO_DEVICE_CPU_t  *pCpuDevice = IFX_NULL;
#ifdef USE_FILESYSTEM
   VMMC_DWLD_t oBBD_Data;
#endif
   IFX_char_t pszFullFilename[MAX_PATH_LEN] = {0};
   IFX_char_t pszFullFilenameTmp[MAX_PATH_LEN] = {0};
   IFX_uint32_t nFullFilenameLen;
   IFX_int32_t nSufixSize, nSufixPosition;
   IFX_uint8_t* pBBD_Buf = IFX_NULL;
   IFX_size_t nBufSize;

   /* check input arguments */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);
   TD_PTR_CHECK(psDownloadPath, IFX_ERROR);
   TD_PTR_CHECK(pFxo, IFX_ERROR);

   if ((pDevice = Common_GetDevice_CPU(pFxoDevice->pBoard->pDevice)) == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
           ("Err, CPU device not found. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
   pCpuDevice = &pDevice->uDevice.stCpu;
#ifdef USE_FILESYSTEM

   /* Set tapi init structure */
   memset(&oBBD_Data, 0, sizeof(VMMC_DWLD_t));

   COM_RCV_FILES_GET(pFxoDevice->pBoard->nType, ITM_RCV_FILE_BBD_FXO,
                     &oBBD_Data.buf, (IFX_size_t*) &oBBD_Data.size);

   if ((IFX_FALSE == g_bBBD_Dwld) && (IFX_NULL == oBBD_Data.buf))
   {
      oBBD_Data.buf = IFX_NULL;
      oBBD_Data.size = 0;
   }
   if (IFX_NULL == oBBD_Data.buf)
   {
      /* get BBD file name used by FXS */
      if (IFX_TRUE != Common_FindBBD_CRAM(pCpuDevice, psDownloadPath,
                                          pszFullFilename))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, Unknown BBD file for CPU device. (File: %s, line: %d)\n",
                __FILE__, __LINE__));
         return IFX_ERROR;
      }
      /* get length and possition of suffix */
      nFullFilenameLen = strlen(pszFullFilename);
      nSufixSize = strlen(TD_BBD_SUFIX_FXS);
      nSufixPosition = nFullFilenameLen - nSufixSize;

      /* check if positive value */
      if ((0 > nSufixPosition) || (nFullFilenameLen <= nSufixPosition))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
               ("Err, File name %s is longer than sufix %s.\n "
                "(File: %s, line: %d)\n",
                pszFullFilename, TD_BBD_SUFIX_FXS, __FILE__, __LINE__));
      }
      /* check if file has FXS suffix,
         if not then common for FXS and FXS file is used */
      else if (0 == strncmp(&pszFullFilename[nSufixPosition],
                            TD_BBD_SUFIX_FXS, nSufixSize))
      {
         /* check if sufixes have the same length */
         if (nSufixSize == strlen(TD_BBD_SUFIX_FXO))
         {
            /* change to FXO sufix */
            strcpy(pszFullFilenameTmp, pszFullFilename);
            strncpy(&pszFullFilenameTmp[nSufixPosition],
                    TD_BBD_SUFIX_FXO, nSufixSize);
            /* check if fxo file is available */
            if (IFX_SUCCESS == Common_CheckFileExists(pszFullFilenameTmp))
            {
               strcpy(pszFullFilename, pszFullFilenameTmp);
            }
            else
            {
               /* file with fxs sufix will be used */
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                     ("Err, File %s is not available using alternate BBD file.\n",
                      pszFullFilenameTmp));
            }
         }
         else
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
                  ("Err, Different length of sufixes %s and %s,"
                   " using alternate BBD file.\n"
                   " (File: %s, line: %d)\n",
                   TD_BBD_SUFIX_FXO, TD_BBD_SUFIX_FXS, __FILE__, __LINE__));
         }
      }
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("%s ch %d - BBD file used (%s).\n",
             pFxoDevice->pFxoTypeName, pFxo->nFxoCh, pszFullFilename));

      /* get binary data and allocate memory */
      ret = TD_OS_FileLoad(pszFullFilename, &pBBD_Buf, &nBufSize);
      if (IFX_SUCCESS != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("Err, %s: Failed to read BBD file %s.\n"
               "(File: %s, line: %d)\n",
               pFxoDevice->pFxoTypeName, pszFullFilename, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      oBBD_Data.buf = pBBD_Buf;
      oBBD_Data.size = nBufSize;
   }

   if (IFX_NULL != oBBD_Data.buf)
   {
      ret = TD_IOCTL(pFxo->nFxoCh_FD, FIO_BBD_DOWNLOAD, &oBBD_Data,
               TD_DEV_NOT_SET, TD_CONN_ID_INIT);
      /* binary data no longer needed */
      TD_OS_MemFree(pBBD_Buf);
      pBBD_Buf = IFX_NULL;
      oBBD_Data.buf = IFX_NULL;

      if (IFX_SUCCESS != ret)
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
              ("WARNING, %s, FIO_BBD_DOWNLOAD failed for ch %d.\n"
               "   It is possible that device node is created, "
               "but device itself isn't connected.\n"
               "   (File: %s, line: %d)\n",
               pFxoDevice->pFxoTypeName, pFxo->nFxoCh,
               __FILE__, __LINE__));
         return IFX_ERROR;
      }
   
      TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
               ("%s ch %d initialized.\n",
                pFxoDevice->pFxoTypeName, pFxo->nFxoCh));
   }
#else
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
            ("%s: Can't init without filesystem support.\n",
             pFxoDevice->pFxoTypeName, pFxo->nFxoCh));
   return IFX_ERROR;
#endif /* USE_FILESYSTEM */


   memset (&lineMode, 0, sizeof(IFX_TAPI_FXO_LINE_MODE_t));
   lineMode.mode = IFX_TAPI_FXO_LINE_MODE_ACTIVE;
   /* Set line mode */
   ret = TD_IOCTL(pFxo->nFxoCh_FD, IFX_TAPI_FXO_LINE_MODE_SET, &lineMode,
            TD_DEV_NOT_SET, TD_CONN_ID_INIT);
   if (ret == IFX_ERROR)
   {
      return (IFX_ERROR);
   }
   TD_TRACE(TAPIDEMO, DBG_LEVEL_LOW, TD_CONN_ID_INIT,
         ("%s ch %d - line type set to FXO.\n",
          pFxoDevice->pFxoTypeName, pFxo->nFxoCh));

   /* Line is activated by default. */

   pCpuDevice->ReleaseFwPtr(pCpuDevice->pDevDependStuff);
   return ret;
}

/**
   Display SLIC121 FXO version

   \param  pFxoDevice - pointer to FXO device structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TD_SLIC121_FXO_GetVersions(TAPIDEMO_DEVICE_FXO_t* pFxoDevice)
{
   /* value 80 is the same as in drivers code and example */
   IFX_char_t Version[80];

   /* check input parameters */
   TD_PTR_CHECK(pFxoDevice, IFX_ERROR);

   if (IFX_SUCCESS == TD_IOCTL(pFxoDevice->nDevFD, IFX_TAPI_VERSION_GET,
                         (IFX_char_t*) &Version[0],
                         TD_DEV_NOT_SET, TD_CONN_ID_INIT))
   {
      TAPIDEMO_PRINTF(TD_CONN_ID_INIT,
                      ("%s Version: %s\n",
                       pFxoDevice->pFxoTypeName, Version));
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_INIT,
            ("Err, %s version could not be read. (File: %s, line: %d)\n",
             pFxoDevice->pFxoTypeName, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}

/**
   Initialize SLIC121 FXO device.

   \param pFxoDevice - pointer to FXO device structure
   \param pBoard - pointer to board structure

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t TD_SLIC121_FXO_DevInit(TAPIDEMO_DEVICE_FXO_t* pFxoDevice,
                                    BOARD_t* pBoard)
{
   /* nothing to do. */
   return IFX_SUCCESS;
}
#endif /* SLIC121_FXO */
#endif /* FXO */

/** ---------------------------------------------------------------------- */




