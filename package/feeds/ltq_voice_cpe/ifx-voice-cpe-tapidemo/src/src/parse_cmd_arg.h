#ifndef __PARSE_CMD_ARG_H_
#define __PARSE_CMD_ARG_H_
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : parse_cmd_arg.h
   Description : Parses command line arguments
 ****************************************************************************
   \file parse_cmd_arg.h

   \remarks
******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Debug interface               */
/* ============================= */

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

extern IFX_char_t** GetCmdArgsArr(IFX_void_t);
extern IFX_return_t ProgramHelp(IFX_void_t);
extern IFX_return_t ReadOptions(IFX_int32_t nArgCnt,
                                IFX_char_t* pArgv[],
                                PROGRAM_ARG_t* pProgramArg);
extern IFX_int32_t GetArgCnt(IFX_char_t* pParam,
                             IFX_char_t* pCmdArgs[]);
extern IFX_return_t PrintUsedOptions(PROGRAM_ARG_t* pProgramArg);
#endif /* __PARSE_CMD_ARG_H_ */


