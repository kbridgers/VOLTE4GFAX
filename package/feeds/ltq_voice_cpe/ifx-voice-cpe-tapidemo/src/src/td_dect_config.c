/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_dect_config.c
   \date 2010-05-27
   \brief Implementation of functions to read and write configuration file.

   This file includes methods which initialize and handle DECT module.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>

#ifdef TAPIDEMO_APP
#include "tapidemo.h"
#endif /* TAPIDEMO_APP */

#include "ifx_common_defs.h"
#include "IFX_DECT_Stack.h"
#include "td_dect_config.h"

#define IFX_CM_RC_SIZE_K 32
#define TD_DECT_CFG_MAX_STRING_LEN     128

/**
   Get configuration data from pipe or file.
   It's just macro for function TD_DECT_CONFIG_GetRcCfgData() to handle errors.

   \param pcFileName       File/Pipe name to open.
   \param pcTag            Data tag.
   \param pcData           Search Name / which Line obtained.
   \param pcRetValue       Return string.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
   \remark use to get data from file : TD_DECT_CONFIG_GetRcCfgData(
                                          FILE_RC_CONF, "tag", "param", sValue)
           use to get data from pipe : TD_DECT_CONFIG_GetRcCfgData(
                                          &command, NULL, "1", sValue)
*/
#define TD_DECT_GET_RC_CFG_DATA(pszFile, pszTag, pszSymbol, acValue, bReturn) \
   do \
   { \
      if (IFX_SUCCESS != TD_DECT_CONFIG_GetRcCfgData(pszFile, pszTag, \
                            pszSymbol, acValue)) \
      { \
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT, \
               ("Warning, Failed to get symbol %s for tag %s in file %s.\n", \
                pszTag, pszSymbol, pszFile)); \
         if (TD_RETURN == bReturn) \
         { \
            return IFX_ERROR; \
         } \
      } \
   } while (0)

#ifdef LINUX
/**
   Deletes old data from file and saves new configuration.
   It's just macro for function TD_DECT_CONFIG_SetRcCfgData() to handle errors.

   \param pFileName       File Name to open.
   \param pTag            Data tag.
   \param nDataCount      Number of pcData strings.
   \param pcData          Data content for setting. First of argument list.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
#define TD_DECT_SET_RC_CFG_DATA(pszFile, pszTag, nElementCnt, bReturn, ...) \
   do \
   { \
      if (IFX_SUCCESS != TD_DECT_CONFIG_SetRcCfgData(pszFile, pszTag, \
                            nElementCnt, __VA_ARGS__)) \
      { \
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT, \
               ("Warning, failed to set data for tag %s in file %s.\n", \
                pszTag, pszFile)); \
         if (TD_RETURN == bReturn) \
         { \
            return IFX_ERROR; \
         } \
      } \
   } while (0)
#endif

/**
   Deletes old data from file and saves new configuration.

   \param pFileName       File Name to open.
   \param pTag            Data tag.
   \param nDataCount      Number of pcData strings.
   \param pcData          Data content for setting. First of argument list.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_SetRcCfgData (const IFX_char_t * pcFileName,
                                          const IFX_char_t * pcTag,
                                          IFX_int32_t iDataCount,
                                          const IFX_char_t * pcData, ...)
{
   FILE *fdIn, *fdOut;
   IFX_char_t acInLine[IFX_CM_MAX_FILENAME_LEN];
   IFX_int32_t nLineLen;
   IFX_char_t acTempName[IFX_CM_MAX_FILENAME_LEN];
   IFX_char_t acbuf[IFX_CM_BUF_SIZE_50K];
   IFX_char_t acBeginTag[IFX_CM_MAX_TAG_LEN], acEndTag[IFX_CM_MAX_TAG_LEN];
   IFX_boolean_t cTagArea = IFX_FALSE;
   va_list args;

   /* check iput parameters */
   TD_PTR_CHECK(pcFileName, IFX_ERROR);
   TD_PTR_CHECK(pcTag, IFX_ERROR);
   TD_PARAMETER_CHECK((0 >= iDataCount), iDataCount, IFX_ERROR);

   /* First, extract data for list and store into buffer*/
   va_start (args, pcData);
   strcpy (acbuf, pcData);
   /* for all input strings */
   while (--iDataCount)
   {
      /* copy input data to one buffer */
      pcData = va_arg (args, char8 *);
      strcat (acbuf, pcData);
   }
   va_end (args);

   /* set begining/end tags */
   memset (acBeginTag, 0x00, IFX_CM_MAX_TAG_LEN);
   memset (acEndTag, 0x00, IFX_CM_MAX_TAG_LEN);
   sprintf (acBeginTag, "#<< %s", pcTag);
   sprintf (acEndTag, "#>> %s", pcTag);

   /* open input file */
   if ((fdIn = fopen (pcFileName, "r+t")) == NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, Failed to open %s. (File: %s, line: %d)\n",
             pcFileName, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   sprintf (acTempName, "%s.tmp", pcFileName);
   /* open output file */
   if ((fdOut = fopen (acTempName, "w+")) == NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, Failed to open %s. (File: %s, line: %d)\n",
             acTempName, __FILE__, __LINE__));
      fclose (fdIn);
      return IFX_ERROR;
   }
   /* get a single line */
   while (fgets (acInLine, sizeof (acInLine), fdIn))
   {
      /* get read string length */
      nLineLen = strlen (acInLine);
      /* check string length */
      if (0 == nLineLen)
      {
         /* empty line */
         continue;
      }

      /* remove '\n' and keep string only */
      if ('\n' == acInLine[nLineLen -1])
      {
         acInLine[nLineLen -1] = '\0';
      }
      /* check if tag area */
      if (IFX_FALSE == cTagArea)
      {
         /* write line to output file */
         fprintf (fdOut, "%s\n", acInLine);
         /* check if line holds begining tag */
         if (strncmp (acInLine, acBeginTag, sizeof (acBeginTag)) == 0)
         {
            /* begining tag was found - now write input data */
            fprintf (fdOut, acbuf);
            fprintf (fdOut, acEndTag);
            /* start of tag area */
            cTagArea = IFX_TRUE;
         }
      } /* if (IFX_FALSE == cTagArea) */
      else
      {
         /* check for end tag */
         if (strncmp (acInLine, acEndTag, sizeof (acEndTag)) == 0)
         {
            /* end of tag area */
            cTagArea = IFX_FALSE;
         }
      } /* if (IFX_FALSE == cTagArea) */
   } /* while (fgets (acInLine, sizeof (acInLine), fdIn)) */

   /* close files */
   fclose (fdIn);
   fclose (fdOut);

   /* remove old file */
   if (remove (pcFileName) == 0)
   {
      /* rename output file */
      if (0 != rename (acTempName, pcFileName))
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
               ("Err, Failed rename file \"%s\" to \"%s\". "
                "(File: %s, line: %d)\n",
                acTempName, pcFileName, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }
   else
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, Failed delete old file \"%s\". (File: %s, line: %d)\n",
             pcFileName, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
}
/**
   Find the value for symbol in the string. in string there should be followng
   line: symbol="value" - where symbol is pcSymbol string and value is e.g. 140

   \param pString    input data string
   \param pSymbol    symbol name to serach for value

   \return     a pointer to the beginning of value for the symbol,
               or NULL if value is not found
*/
IFX_char_t *TD_DECT_CONFIG_GetCfgDatafromString(IFX_char_t *pcString,
                                                IFX_char_t *pcSymbol)
{
   IFX_char_t  acseps[]   = "\t\n\"";

   /* chcek input parameters */
   TD_PTR_CHECK(pcString, IFX_NULL);
   TD_PTR_CHECK(pcSymbol, IFX_NULL);

   /* search for pcSymbol in pcString */
   if ((pcString = strstr(pcString, pcSymbol)) == NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, \"%s\" not found. (File: %s, line: %d)\n",
             pcSymbol, __FILE__, __LINE__));
      return IFX_NULL;
   }
   /* next strtok will search from first '=' */
   if (strtok(pcString, "=") == NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, no '=' after \"%s\". (File: %s, line: %d)\n",
             pcSymbol, __FILE__, __LINE__));
      return IFX_NULL;
   }
   /* get value in between '"' */
   if ((pcString = strtok(NULL, acseps)) == NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
            ("Err, no value for \"%s\". (File: %s, line: %d)\n",
             pcSymbol, __FILE__, __LINE__));
      return IFX_NULL;
   }
   return pcString;
}
/**
   Get configuration data from pipe or file.

   \param pcFileName       File/Pipe name to open.
   \param pcTag            Data tag.
   \param pcData           Search Name / which Line obtained.
   \param pcRetValue       Return string.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
   \remark use to get data from file : TD_DECT_CONFIG_GetRcCfgData(
                                          FILE_RC_CONF, "tag", "param", sValue)
           use to get data from pipe : TD_DECT_CONFIG_GetRcCfgData(
                                          &command, NULL, "1", sValue)
*/
IFX_return_t TD_DECT_CONFIG_GetRcCfgData(IFX_char_t *pcFileName,
                                         IFX_char_t *pcTag,
                                         IFX_char_t *pcData,
                                         IFX_char_t *pcRetValue)
{
   FILE *fd = NULL;
   IFX_char_t *pcString = IFX_NULL;
   IFX_char_t acbuffer[(IFX_CM_MAX_BUFF_LEN*IFX_CM_RC_SIZE_K)+1];
   IFX_char_t acBeginTag[IFX_CM_MAX_TAG_LEN];
   IFX_char_t acValue[IFX_CM_MAX_BUFF_LEN];
   IFX_return_t iReturn = IFX_ERROR;
   IFX_int32_t iLine = 0;
   IFX_int32_t iIndex = 1;

   /* chcek input parameters */
   TD_PTR_CHECK(pcFileName, IFX_ERROR);
   TD_PTR_CHECK(pcData, IFX_ERROR);
   TD_PTR_CHECK(pcRetValue, IFX_ERROR);

   /* check if tag is set and if points to non empty string */
   if ( pcTag && *pcTag )
   {
      /* open file to read */
      if ((fd = fopen(pcFileName, "r")) != NULL)
      {
         do
         {
            /* reset memory */
            memset(acbuffer, 0x00, IFX_CM_MAX_BUFF_LEN*IFX_CM_RC_SIZE_K);
            /* read from file */
            fread(acbuffer, sizeof(char8),
                  IFX_CM_MAX_BUFF_LEN * IFX_CM_RC_SIZE_K, fd);
            acbuffer[IFX_CM_MAX_BUFF_LEN * IFX_CM_RC_SIZE_K] = '\0';
            /* construct begining tag */
            sprintf(acBeginTag, "#<< %s", pcTag);
            /* search for begining tag */
            if ((pcString = strstr(acbuffer, acBeginTag)) == NULL)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                     ("Err, Unable to find %s in file %s. "
                      "(File: %s, line: %d)\n",
                      acBeginTag, pcFileName, __FILE__, __LINE__));
               break;
            }
            /* get value of pcData */
            pcString = TD_DECT_CONFIG_GetCfgDatafromString(pcString, pcData);
            /* check if value was found */
            if (pcString == IFX_NULL)
            {
               TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                     ("Err, Unable to find value for %s in file %s. "
                      "(File: %s, line: %d)\n",
                      acBeginTag, pcFileName, __FILE__, __LINE__));
               break;
            }
            /* copy value string */
            strcpy(pcRetValue, pcString);
            iReturn = IFX_SUCCESS;
         } while (0);
      } /* if ((fd = fopen(pcFileName, "r")) != NULL) */
      else
      {
         TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
               ("Err, Failed to open file %s. (File: %s, line: %d)\n",
                pcFileName, __FILE__, __LINE__));
      } /* if ((fd = fopen(pcFileName, "r")) != NULL) */
   } /* if ( pcTag && *pcTag ) */
   else
   {
      do
      {
         memset(acValue, 0x00, IFX_CM_MAX_BUFF_LEN);
         /* get line number */
         iLine = atoi(pcData);

         /* open pipe */
         if ((fd = popen(pcFileName, "r")) == NULL)
         {
            TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_DECT,
                  ("Err, Failed to open pipe %s. (File: %s, line: %d)\n",
                   pcFileName, __FILE__, __LINE__));
            break;
         }
         /* get next line */
         while ( fgets(acValue, IFX_CM_MAX_BUFF_LEN, fd) )
         {
            /* check searched line was read */
            if (iIndex == iLine)
            {
               if ('\n' == acValue[strlen(acValue)-1])
               {
                  acValue[strlen(acValue)-1] = 0;
               }
               strcpy(pcRetValue, acValue);
               iReturn = IFX_SUCCESS;
               break;
            }
            iIndex++;
         }
      } while (0);
   } /* if ( pcTag && *pcTag ) */

   if (fd != NULL)
   {
      /* close fd if opened */
      fclose(fd);
   }

   return iReturn;
}

/**
   Set Transmit Power Parameters.

   \param pxTPCParams     Pointer to Transmit Power Parameters structure.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcTpcSet(x_IFX_DECT_TransmitPowerParam *pxTPCParams)
{
   IFX_char_t a[IFX_CM_TAG_TPC_ELEMENT_CNT][TD_DECT_CFG_MAX_STRING_LEN];

   /* chcek input parameters */
   TD_PTR_CHECK(pxTPCParams, IFX_ERROR);

   memset(a, 0, sizeof(a));

   /* set buffers */
   sprintf(a[0], "TransPower_0_cpeId=\"%d\"\n",1);
   sprintf(a[1], "TransPower_0_pcpeId=\"%d\"\n",1);
   sprintf(a[2], "TransPower_0_TunDgtRef=\"%d\"\n",
           pxTPCParams->ucTuneDigitalRef);
   sprintf(a[3], "TransPower_0_PABiasRef=\"%d\"\n",
           pxTPCParams->ucPABiasRef);
   sprintf(a[4], "TransPower_0_Pwroffset0=\"%d\"\n",
           pxTPCParams->ucPowerOffset[0]);
   sprintf(a[5], "TransPower_0_Pwroffset1=\"%d\"\n",
           pxTPCParams->ucPowerOffset[1]);
   sprintf(a[6], "TransPower_0_Pwroffset2=\"%d\"\n",
           pxTPCParams->ucPowerOffset[2]);
   sprintf(a[7], "TransPower_0_Pwroffset3=\"%d\"\n",
           pxTPCParams->ucPowerOffset[3]);
   sprintf(a[8], "TransPower_0_Pwroffset4=\"%d\"\n",
           pxTPCParams->ucPowerOffset[4]);
   sprintf(a[9], "TransPower_0_TD1=\"%d\"\n", pxTPCParams->ucTD1);
   sprintf(a[10],"TransPower_0_TD2=\"%d\"\n", pxTPCParams->ucTD2);
   sprintf(a[11],"TransPower_0_TD3=\"%d\"\n", pxTPCParams->ucTD3);
   sprintf(a[12],"TransPower_0_PA1=\"%d\"\n", pxTPCParams->ucPA1);
   sprintf(a[13],"TransPower_0_PA2=\"%d\"\n", pxTPCParams->ucPA2);
   sprintf(a[14],"TransPower_0_PA3=\"%d\"\n", pxTPCParams->ucPA3);
   sprintf(a[15],"TransPower_0_SwPwrMde=\"%d\"\n", pxTPCParams->ucSWPowerMode);
   sprintf(a[16],"TransPower_0_Txpow0=\"%d\"\n", pxTPCParams->ucTXPOW_0);
   sprintf(a[17],"TransPower_0_Txpow1=\"%d\"\n", pxTPCParams->ucTXPOW_1);
   sprintf(a[18],"TransPower_0_Txpow2=\"%d\"\n", pxTPCParams->ucTXPOW_2);
   sprintf(a[19],"TransPower_0_Txpow3=\"%d\"\n", pxTPCParams->ucTXPOW_3);
   sprintf(a[20],"TransPower_0_Txpow4=\"%d\"\n", pxTPCParams->ucTXPOW_4);
   sprintf(a[21],"TransPower_0_Txpow5=\"%d\"\n", pxTPCParams->ucTXPOW_5);
   sprintf(a[22],"TransPower_0_Dbpow=\"%d\"\n", pxTPCParams->ucDBPOW);
   sprintf(a[23],"TransPower_0_TuneDigit=\"%d\"\n", pxTPCParams->ucTuneDigital);
   sprintf(a[24],"TransPower_0_TxBias=\"%d\"\n", pxTPCParams->ucTxBias);

   TD_DECT_SET_RC_CFG_DATA (IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                            IFX_CM_TAG_TPC_ELEMENT_CNT, TD_RETURN,
                            a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
                            a[8], a[9], a[10],a[11],a[12],a[13],a[14],a[15],
                            a[16],a[17],a[18],a[19],a[20],a[21],a[22],a[23],
                            a[24]);

   return IFX_SUCCESS;
}

/**
   Get Transmit Power Parameters.

   \param pxTPCParams     Pointer to Transmit Power Parameters structure.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcTpcGet(x_IFX_DECT_TransmitPowerParam *pxTPCParams)
{
   IFX_char_t a[TD_DECT_CFG_MAX_STRING_LEN];

   TD_PTR_CHECK(pxTPCParams, IFX_ERROR);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                           "TransPower_0_TunDgtRef", a, TD_RETURN);
   pxTPCParams->ucTuneDigitalRef = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_PABiasRef", a, TD_RETURN);
   pxTPCParams->ucPABiasRef = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Pwroffset0", a, TD_RETURN);
   pxTPCParams->ucPowerOffset[0] = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Pwroffset1", a, TD_RETURN);
   pxTPCParams->ucPowerOffset[1] = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Pwroffset2", a, TD_RETURN);
   pxTPCParams->ucPowerOffset[2] = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Pwroffset3", a, TD_RETURN);
   pxTPCParams->ucPowerOffset[3] = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Pwroffset4", a, TD_RETURN);
   pxTPCParams->ucPowerOffset[4] = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_TD1", a, TD_RETURN);
   pxTPCParams->ucTD1 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_TD2", a, TD_RETURN);
   pxTPCParams->ucTD2 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_TD3", a, TD_RETURN);
   pxTPCParams->ucTD3 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_PA1", a, TD_RETURN);
   pxTPCParams->ucPA1 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_PA2", a, TD_RETURN);
   pxTPCParams->ucPA2 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_PA3", a, TD_RETURN);
   pxTPCParams->ucPA3 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_SwPwrMde", a, TD_RETURN);
   pxTPCParams->ucSWPowerMode = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Txpow0", a, TD_RETURN);
   pxTPCParams->ucTXPOW_0 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Txpow1", a, TD_RETURN);
   pxTPCParams->ucTXPOW_1 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Txpow2", a, TD_RETURN);
   pxTPCParams->ucTXPOW_2 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Txpow3", a, TD_RETURN);
   pxTPCParams->ucTXPOW_3 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Txpow4", a, TD_RETURN);
   pxTPCParams->ucTXPOW_4 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Txpow5", a, TD_RETURN);
   pxTPCParams->ucTXPOW_5 = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_Dbpow", a, TD_RETURN);
   pxTPCParams->ucDBPOW = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_TuneDigit", a, TD_RETURN);
   pxTPCParams->ucTuneDigital = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_TPC,
                       "TransPower_0_TxBias", a, TD_RETURN);
   pxTPCParams->ucTxBias = atoi(a);

   return IFX_SUCCESS;
}

/**
   Set Burst Mode Control parameters.

   \param x_IFX_DECT_BMCRegParams     Pointer to Burst Mode Control parameters
                                      structure.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcBmcSet(x_IFX_DECT_BMCRegParams *pxBMC)
{
   IFX_char_t a[IFX_CM_TAG_BMC_ELEMENT_CNT][TD_DECT_CFG_MAX_STRING_LEN];

   TD_PTR_CHECK(pxBMC, IFX_ERROR);

   memset(a, 0, sizeof(a));

   /* Get existing values */
   sprintf(a[0],"BmcParams_0_cpeId=\"%d\"\n",1);
   sprintf(a[1],"BmcParams_0_pcpeId=\"%d\"\n",1);
   sprintf(a[2],"BmcParams_0_RssiFreeLevel=\"%d\"\n",pxBMC->ucRSSIFreeLevel);
   sprintf(a[3],"BmcParams_0_RssiBusyLevel=\"%d\"\n",pxBMC->ucRSSIBusyLevel);
   sprintf(a[4],"BmcParams_0_BearerChglimit=\"%d\"\n",pxBMC->ucBearerChgLim);
   sprintf(a[5],"BmcParams_0_DefaultAntenna=\"%d\"\n",pxBMC->ucDefaultAntenna);
   sprintf(a[6],"BmcParams_0_WOPNSF=\"%d\"\n",pxBMC->ucWOPNSF);
   sprintf(a[7],"BmcParams_0_WWSF=\"%d\"\n",pxBMC->ucWWSF);
   sprintf(a[8],"BmcParams_0_CNTUPCtrlReg=\"%d\"\n",pxBMC->ucCNTUPCtrlReg);
   sprintf(a[9],"BmcParams_0_DelayReg=\"%d\"\n",pxBMC->ucDelayReg);
   sprintf(a[10],"BmcParams_0_HandOverEvalper=\"%d\"\n",pxBMC->ucHandOverEvalper);
   sprintf(a[11],"BmcParams_0_SYNCMCtrlReg=\"%d\"\n",pxBMC->ucCNTUPCtrlReg);
   sprintf(a[12],"BmcParams_0_Reserved_0=\"%d\"\n",255);
   sprintf(a[13],"BmcParams_0_Reserved_1=\"%d\"\n",255);
   sprintf(a[14],"BmcParams_0_Reserved_2=\"%d\"\n",3);
   sprintf(a[15],"BmcParams_0_Reserved_3=\"%d\"\n",0);

   TD_DECT_SET_RC_CFG_DATA (IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                            IFX_CM_TAG_BMC_ELEMENT_CNT, TD_RETURN,
                            a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
                            a[8], a[9], a[10],a[11],a[12],a[13],a[14],a[15]);
   return IFX_SUCCESS;
}

/**
   Get Burst Mode Control parameters.

   \param x_IFX_DECT_BMCRegParams     Pointer to Burst Mode Control parameters
                                      structure.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcBmcGet(x_IFX_DECT_BMCRegParams *pxBMC)
{
   IFX_char_t a[TD_DECT_CFG_MAX_STRING_LEN];

   TD_PTR_CHECK(pxBMC, IFX_ERROR);

   /* Get existing values */
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_RSSIFREE, a, TD_RETURN);
   pxBMC->ucRSSIFreeLevel = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_RSSIBUSY, a, TD_RETURN);
   pxBMC->ucRSSIBusyLevel = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_BRLIMIT, a, TD_RETURN);
   pxBMC->ucBearerChgLim = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_DEFANT, a, TD_RETURN);
   pxBMC->ucDefaultAntenna = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_WOPNSF, a, TD_RETURN);
   pxBMC->ucWOPNSF = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_WWSF, a, TD_RETURN);
   pxBMC->ucWWSF = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_DRONCTRL, a, TD_RETURN);
   pxBMC->ucCNTUPCtrlReg = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_DVAL, a, TD_RETURN);
   pxBMC->ucDelayReg = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_HOFRAME, a, TD_RETURN);
   pxBMC->ucHandOverEvalper = atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_BMC,
                           IFX_CM_TAG_BMC_SYNCMODE, a, TD_RETURN);
   pxBMC->ucSYNCMCtrlReg = atoi(a);
   pxBMC->ucReserved_0 =255;
   pxBMC->ucReserved_1 =255;
   pxBMC->ucReserved_2 =3;
   pxBMC->ucReserved_3 =0;

   return IFX_SUCCESS;
}
/**
   Get frequency parameters.

   \param pucFreqTx     pointer to frequency for TX.
   \param pucFreqRx     pointer to frequency for RX.
   \param pucFreqRange  pointer to frequency range.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcFreqGet(IFX_uint8_t *pucFreqTx,
                                      IFX_uint8_t *pucFreqRx,
                                      IFX_uint8_t *pucFreqRange)
{
   IFX_char_t a[TD_DECT_CFG_MAX_STRING_LEN];

   /* check input parameters */
   TD_PTR_CHECK(pucFreqTx, IFX_ERROR);
   TD_PTR_CHECK(pucFreqRx, IFX_ERROR);
   TD_PTR_CHECK(pucFreqRange, IFX_ERROR);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_FREQ,
                           "CtrySet_0_Txoffset", a, TD_RETURN);
   *pucFreqTx = atoi(a);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_FREQ,
                           "CtrySet_0_Rxoffset", a, TD_RETURN);
   *pucFreqRx = atoi(a);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_FREQ,
                           "CtrySet_0_FreqRange", a, TD_RETURN);
   *pucFreqRange = atoi(a);

   return IFX_SUCCESS;
}
/**
   Set frequency parameters.

   \param ucFreqTx      frequency for TX.
   \param ucFreqRx      frequency for RX.
   \param ucFreqRange   frequency range.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcFreqSet(IFX_uint8_t ucFreqTx,
                                      IFX_uint8_t ucFreqRx,
                                      IFX_uint8_t ucFreqRange)
{
   IFX_char_t a[IFX_CM_TAG_FREQ_ELEMENT_CNT][TD_DECT_CFG_MAX_STRING_LEN];

   memset(a, 0, sizeof(a));

   /* Store  values To Rc.conf */
   sprintf(a[0],"CtrySet_0_cpeId=\"%d\"\n",1);
   sprintf(a[1],"CtrySet_0_pcpeId=\"%d\"\n",1);
   sprintf(a[2],"CtrySet_0_Txoffset=\"%d\"\n",
           ucFreqTx);
   sprintf(a[3],"CtrySet_0_Rxoffset=\"%d\"\n",
           ucFreqRx);
   sprintf(a[4],"CtrySet_0_FreqRange=\"%d\"\n",
           ucFreqRange);
   TD_DECT_SET_RC_CFG_DATA (IFX_CM_FILE_RC_CONF,  IFX_CM_TAG_FREQ,
                            IFX_CM_TAG_FREQ_ELEMENT_CNT, TD_RETURN,
                            a[0], a[1], a[2], a[3], a[4]);

   return IFX_SUCCESS;
}
/**
   Get Oscillator Trimming parameters.

   \param puiOscTrimValue     pointer to Oscillator Trimming.
   \param pucP10Status        pointer to P10 status.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcOscGet(IFX_uint16_t *puiOscTrimValue,
                                     IFX_uint8_t *pucP10Status)
{
   IFX_char_t a[TD_DECT_CFG_MAX_STRING_LEN];

   TD_PTR_CHECK(puiOscTrimValue, IFX_ERROR);
   TD_PTR_CHECK(pucP10Status, IFX_ERROR);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_OSC,
                           "Osctrim_0_Osctrimhigh", a, TD_RETURN);
   *puiOscTrimValue = atoi(a)<<8;
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_OSC,
                           "Osctrim_0_Osctrimlow", a, TD_RETURN);
   *puiOscTrimValue |= atoi(a);
   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_OSC,
                           "Osctrim_0_P10Status", a, TD_RETURN);
   *pucP10Status = atoi(a);

   return IFX_SUCCESS;
}
/**
   Set Oscillator Trimming parameters.

   \param puiOscTrimValue     Oscillator Trimming.
   \param pucP10Status        P10 status.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcOscSet(IFX_uint16_t uiOscTrimValue,
                                     IFX_uint8_t ucP10Status)
{
   IFX_char_t a[IFX_CM_TAG_OSC_ELEMENT_CNT][TD_DECT_CFG_MAX_STRING_LEN];

   memset(a, 0, sizeof(a));

   /* Store  values To Rc.conf */
   sprintf(a[0],"Osctrim_0_cpeId=\"%d\"\n",1);
   sprintf(a[1],"Osctrim_0_pcpeId=\"%d\"\n",1);
   sprintf(a[2],"Osctrim_0_Osctrimhigh=\"%d\"\n",
           uiOscTrimValue>>8);
   sprintf(a[3],"Osctrim_0_Osctrimlow=\"%d\"\n",
           uiOscTrimValue&0xFF);
   sprintf(a[4],"Osctrim_0_P10Status=\"%d\"\n",ucP10Status);

   TD_DECT_SET_RC_CFG_DATA (IFX_CM_FILE_RC_CONF, IFX_CM_TAG_OSC,
                            IFX_CM_TAG_OSC_ELEMENT_CNT, TD_RETURN,
                            a[0],a[1],a[2],a[3],a[4]);
   return IFX_SUCCESS;
}
/**
   Get Gaussian Value.

   \param puiGfskValue        pointer to Gaussian Value.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcGfskGet(IFX_uint16_t *puiGfskValue)
{
   IFX_char_t a[TD_DECT_CFG_MAX_STRING_LEN];

   TD_PTR_CHECK(puiGfskValue, IFX_ERROR);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_GFSK,
                           "Gfsk_0_Gfskhigh", a, TD_RETURN);
   *puiGfskValue= atoi(a)<<8;

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_GFSK,
                           "Gfsk_0_Gfsklow", a, TD_RETURN);
   *puiGfskValue |= atoi(a);

   return IFX_SUCCESS;
}
/**
   Set Gaussian Value.

   \param unGfskValue         Gaussian Value.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcGfskSet(IFX_uint16_t unGfskValue)
{
   IFX_char_t a[IFX_CM_TAG_GFSK_ELEMENT_CNT][TD_DECT_CFG_MAX_STRING_LEN];

   memset(a, 0, sizeof(a));

   /* Store  values To Rc.conf */
   sprintf(a[0],"Gfsk_0_cpeId=\"%d\"\n",1);
   sprintf(a[1],"Gfsk_0_pcpeId=\"%d\"\n",1);
   sprintf(a[2],"Gfsk_0_Gfskhigh=\"%d\"\n", unGfskValue>>8);
   sprintf(a[3],"Gfsk_0_Gfsklow=\"%d\"\n", unGfskValue&0xFF);

   TD_DECT_SET_RC_CFG_DATA (IFX_CM_FILE_RC_CONF, IFX_CM_TAG_GFSK,
                            IFX_CM_TAG_GFSK_ELEMENT_CNT, TD_RETURN,
                            a[0], a[1], a[2], a[3]);

   return IFX_SUCCESS;
}
/**
   Get RFPI.

   \param pcRFPI        pointer to RFPI array.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcRfpiGet(IFX_uint8_t *pcRFPI)
{
   IFX_char_t a[TD_DECT_CFG_MAX_STRING_LEN];

   TD_PTR_CHECK(pcRFPI, IFX_ERROR);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFPI,
                           "Rfpi_0_byte0", a, TD_RETURN);
   pcRFPI[0] = atoi(a);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFPI,
                           "Rfpi_0_byte1", a, TD_RETURN);
   pcRFPI[1] = atoi(a);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFPI,
                           "Rfpi_0_byte2", a, TD_RETURN);
   pcRFPI[2] = atoi(a);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFPI,
                           "Rfpi_0_byte3", a, TD_RETURN);
   pcRFPI[3] = atoi(a);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFPI,
                           "Rfpi_0_byte4", a, TD_RETURN);
   pcRFPI[4] = atoi(a);

   return IFX_SUCCESS;
}
/**
   Set RFPI.

   \param pcRFPI        pointer to RFPI array.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcRfpiSet(IFX_uint8_t *pcRFPI)
{
   IFX_char_t a[IFX_CM_TAG_RFPI_ELEMENT_CNT][TD_DECT_CFG_MAX_STRING_LEN];

   TD_PTR_CHECK(pcRFPI, IFX_ERROR);

   memset(a, 0, sizeof(a));

   /* Store  values To Rc.conf */
   sprintf(a[0],"Rfpi_0_cpeId=\"%d\"\n", 1);
   sprintf(a[1],"Rfpi_0_pcpeId=\"%d\"\n", 1);
   sprintf(a[2],"Rfpi_0_byte0=\"%d\"\n", pcRFPI[0]);
   sprintf(a[3],"Rfpi_0_byte1=\"%d\"\n", pcRFPI[1]);
   sprintf(a[4],"Rfpi_0_byte2=\"%d\"\n", pcRFPI[2]);
   sprintf(a[5],"Rfpi_0_byte3=\"%d\"\n", pcRFPI[3]);
   sprintf(a[6],"Rfpi_0_byte4=\"%d\"\n", pcRFPI[4]);
   sprintf(a[7],"Rfpi_0_byte5=\"0\"\n");

   TD_DECT_SET_RC_CFG_DATA (IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFPI,
                            IFX_CM_TAG_RFPI_ELEMENT_CNT, TD_RETURN,
                            a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);

   return IFX_SUCCESS;
}

/**
   Get RF Test Mode.

   \param pucRFMode           pointer to RF Test Mode.
   \param pucChannelNumber    pointer to channel number.
   \param pucSlotNumber       pointer to slot number.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcRfModeGet(IFX_uint8_t *pucRFMode,
                                        IFX_uint8_t *pucChannelNumber,
                                        IFX_uint8_t *pucSlotNumber)
{
   IFX_char_t a[TD_DECT_CFG_MAX_STRING_LEN];

   TD_PTR_CHECK(pucRFMode, IFX_ERROR);
   TD_PTR_CHECK(pucChannelNumber, IFX_ERROR);
   TD_PTR_CHECK(pucSlotNumber, IFX_ERROR);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFMODE,
                           "Rfmode_0_TstMode", a, TD_RETURN);
   *pucRFMode = atoi(a);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFMODE,
                           "Rfmode_0_Channel",a, TD_RETURN);
   *pucChannelNumber = atoi(a);

   TD_DECT_GET_RC_CFG_DATA(IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFMODE,
                           "Rfmode_0_Slot", a, TD_RETURN);
   *pucSlotNumber = atoi(a);

   return IFX_SUCCESS;
}
/**
   Set RF Test Mode.

   \param ucRFMode            RF Test Mode.
   \param ucChannelNumber     channel number.
   \param ucSlotNumber        slot number.

   \return IFX_SUCCESS on success, otherwise IFX_ERROR
*/
IFX_return_t TD_DECT_CONFIG_RcRfModeSet(IFX_uint8_t ucRFMode,
                                        IFX_uint8_t ucChannelNumber,
                                        IFX_uint8_t ucSlotNumber)
{
   IFX_char_t a[IFX_CM_TAG_RFMODE_ELEMENT_CNT][TD_DECT_CFG_MAX_STRING_LEN];

   memset(a, 0, sizeof(a));

   /* Store  values To Rc.conf */
   sprintf(a[0],"Rfmode_0_cpeId=\"%d\"\n",1);
   sprintf(a[1],"Rfmode_0_pcpeId=\"%d\"\n",1);
   sprintf(a[2],"Rfmode_0_TstMode=\"%d\"\n",ucRFMode);
   sprintf(a[3],"Rfmode_0_Channel=\"%d\"\n", ucChannelNumber);
   sprintf(a[4],"Rfmode_0_Slot=\"%d\"\n",ucSlotNumber);

   TD_DECT_SET_RC_CFG_DATA (IFX_CM_FILE_RC_CONF, IFX_CM_TAG_RFMODE,
                            IFX_CM_TAG_RFMODE_ELEMENT_CNT, TD_RETURN,
                            a[0], a[1], a[2], a[3], a[4]);

   return IFX_SUCCESS;
}



