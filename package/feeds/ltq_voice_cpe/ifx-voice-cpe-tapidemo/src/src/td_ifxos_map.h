#ifndef _TD_IFXOS_MAP_H
#define _TD_IFXOS_MAP_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : td_ifxos_map.h
   Description : This file provides functions for supporting pipes on vxWorks.
 ****************************************************************************
   \file td_ifxos_map.h

   \remarks

   \note Changes:
*******************************************************************************/

#ifdef VXWORKS
   #include <ioLib.h>
   #include <iosLib.h>
   #include <memLib.h>

   #include <stdio.h>
   #include <stdarg.h>
   #include <errno.h>
   #include <string.h>
#endif /* VXWORKS */

#ifdef LINUX
#include "tapidemo_config.h"
#endif

#include "td_osmap.h"

#ifdef VXWORKS
IFX_int32_t TD_IFXOS_MAP_PipeCreate(IFX_char_t *pName);
TD_OS_Pipe_t *TD_IFXOS_MAP_PipeOpen(IFX_char_t *pName, IFX_boolean_t reading,
                                    IFX_boolean_t blocking);
IFX_int32_t TD_IFXOS_MAP_PipeClose(IFXOS_Pipe_t *pPipe);
IFX_int32_t TD_IFXOS_MAP_PipePrintf(TD_OS_Pipe_t *streamPipe,
                                    const IFX_char_t  *format, ...);
IFX_int32_t TD_IFXOS_MAP_PipeRead(IFX_void_t *pDataBuf,
                                  IFX_uint32_t elementSize_byte,
                                  IFX_uint32_t elementCount,
                                  TD_OS_Pipe_t *pPipe);
#endif /* VXWORKS */


#endif /* TD_IFXOS_MAP_H */


