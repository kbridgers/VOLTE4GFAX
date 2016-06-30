/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_ifxos_map.c
   \date 2010-12-14
   \brief  This file contains the functions specific to the OS.

   This file provides functions specific to the OS, which either are not
   implemented or has incorrect implementation in library IFX OS.
*/

#include "common.h"
#include "td_ifxos_map.h"
#include "./itm/com_client.h"

#ifdef VXWORKS
#include <pipeDrv.h>

#define PIPE_DIR_PATH "/pipe/"

/**
   Create a pipe.

\param
   pName - pipe name

\return
   - IFX_SUCCESS on success
   - IFX_ERROR on failure
*/
IFX_int32_t TD_IFXOS_MAP_PipeCreate(IFX_char_t *pName)
{
   STATUS ret;
   IFX_int32_t nMaxMsgs = 10;
   IFX_int32_t nMaxLength = 4;
   IFX_char_t *pPipePath = NULL;

   /* setup pipe path */
   pPipePath = malloc(strlen(PIPE_DIR_PATH) + strlen(pName) + 1);
   if ( !pPipePath )
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
           ("Err, Allocation memory for pipe path failed.\n"
            "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return IFX_ERROR;
   }
   strcpy(pPipePath, PIPE_DIR_PATH);
   strcat(pPipePath, pName);

   ret = pipeDevCreate(pPipePath, nMaxMsgs, nMaxLength);
   if (ERROR == ret)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
           ("Err, pipeDevCreate() for %s failed.\n"
            "(File: %s, line: %d)\n", pPipePath, __FILE__, __LINE__));
     free(pPipePath);
     return IFX_ERROR;
   }

   free(pPipePath);
   return IFX_SUCCESS;
} /* TD_IFXOS_MAP_PipeCreate */

/**
   Open a pipe.

\param
   pName    - pipe name.
\param
   reading  - if set, open the pipe for read.
\param
   blocking - if set, open the pipe in blocking mode.

\return
   - pointer to IFXOS_Pipe_t structure
   - in case of error the return value is NULL
*/
TD_OS_Pipe_t* TD_IFXOS_MAP_PipeOpen(IFX_char_t *pName,
                                    IFX_boolean_t reading,
                                    IFX_boolean_t blocking)
{
   IFX_int32_t fd;
   IFX_int32_t flags;
   IFX_int32_t mode;
   TD_OS_Pipe_t *pPipe = IFX_NULL;
   IFX_char_t pPipePath[256];

   snprintf(pPipePath, sizeof(pPipePath), "%s%s", PIPE_DIR_PATH, pName);

   if (reading == IFX_TRUE)
   {
      flags = O_RDONLY;
      mode = 0444;
   }
   else
   {
      flags = O_WRONLY;
      mode = 0222;
   }

   if (!blocking)
      flags |= O_NONBLOCK;

   fd = open(pPipePath, flags, mode);
   if (fd <= 0)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
           ("Err, Can't open pipe %s.\n"
            "(File: %s, line: %d)\n", pPipePath, __FILE__, __LINE__));
      return IFX_NULL;
   }

   if (reading == IFX_TRUE)
      pPipe = fdopen(fd, "r");
   else
      pPipe = fdopen(fd, "w");

   if (pPipe == IFX_NULL)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
           ("Err, Can't open pipe %s.\n"
            "(File: %s, line: %d)\n", pPipePath, __FILE__, __LINE__));
      return IFX_NULL;
   }
   return pPipe;
} /* TD_IFXOS_MAP_PipeOpen */

/**
   Close a pipe.

\param
   pPipe    - handle of the pipe stream

\return
   - IFX_SUCCESS on success
   - IFX_ERROR on failure
*/
IFX_int32_t TD_IFXOS_MAP_PipeClose(TD_OS_Pipe_t *pPipe)
{
   fflush(pPipe);
   return (fclose(pPipe)==0) ? IFX_SUCCESS : IFX_ERROR;
} /* TD_IFXOS_MAP_PipeClose */

/**
   Print to a pipe.

\param
   streamPipe  - handle of the pipe stream.
\param
   format      - points to the printf format string.

\return
   For success - Number of written bytes.
   For error   - negative value.

*/
IFX_int32_t TD_IFXOS_MAP_PipePrintf(TD_OS_Pipe_t *streamPipe,
                                    const IFX_char_t  *format, ...)
{
   va_list ap;
   IFX_int32_t retVal = 0;
   IFX_char_t buf[100] = {0};
   IFX_int32_t nPipeFd = NO_PIPE;

   nPipeFd = fileno(streamPipe);
   if (NO_PIPE == nPipeFd)
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, Failed to get fd.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return -1;
   }

   va_start(ap, format);
   retVal = vsprintf(buf, format, ap);
   va_end(ap);

   retVal = write(nPipeFd, buf, strlen(buf));

   return retVal;
} /* TD_IFXOS_MAP_PipePrintf */

/**
   Read from pipe .

\param
   pDataBuf          - Points to the buffer used for get the data. [o]
\param
   elementSize_byte  - Element size of one element to read [byte]
\param
   elementCount      - Number of elements to read
\param
   pPipe             - handle of the pipe stream.

\return
   Number of read elements

*/
IFX_int32_t TD_IFXOS_MAP_PipeRead(IFX_void_t *pDataBuf,
                                  IFX_uint32_t elementSize_byte,
                                  IFX_uint32_t elementCount,
                                  TD_OS_Pipe_t *pPipe)
{
   IFX_int32_t nReceivedElementsSize = 0;
   IFX_int32_t nReceivedElementCount = 0;
   IFX_int32_t nReceivedElementCountRes = 0;
   IFX_int32_t nPipeFd = NO_PIPE;

   nPipeFd = fileno(pPipe);
   if (NO_PIPE == nPipeFd)
   {
     TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
            ("Err, Failed to get fd.\n"
             "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return 0;
   }

   nReceivedElementsSize = read(nPipeFd, pDataBuf,
                                elementSize_byte*elementCount);

   nReceivedElementCount = nReceivedElementsSize / elementSize_byte;
   nReceivedElementCountRes = nReceivedElementsSize % elementSize_byte;

   if ((nReceivedElementCount != elementCount) ||
       (0 != nReceivedElementCountRes))
   {
      TD_TRACE(TAPIDEMO, DBG_LEVEL_HIGH, TD_CONN_ID_ERR,
           ("Received %d element(s) with size %d and element with size %d.\n"
            "Expect %d element with size %d.\n (File: %s, line: %d)\n",
            nReceivedElementCount, elementSize_byte, nReceivedElementCountRes,
            elementCount, elementSize_byte, __FILE__, __LINE__));
   }

   return nReceivedElementCount;
} /* TD_IFXOS_MAP_PipeRead */

#endif /* VXWORKS */


