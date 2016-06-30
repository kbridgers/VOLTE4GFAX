
#ifndef __TD_DECT_CLI_H__
#define __TD_DECT_CLI_H__

/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_dect_cli.h
   \date 2010-05-XX
   \brief message structure to communicate with CLI.

   This file includes methods which initialize and handle DECT module.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "ifx_common_defs.h"
#include "IFX_DECT_Stack.h"

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/** message type */
typedef enum {
   /* CLI-->Tapidemo*/
   IFX_CLI_DIAG_MODE,
   IFX_CLI_MODEM_RESET,
   IFX_CLI_SET_BMC_REQ,
   IFX_CLI_SET_OSC_REQ,
   IFX_CLI_SET_TBR6_REQ,
   IFX_CLI_SET_RFPI_REQ,
   IFX_CLI_SET_XRAM_REQ,
   IFX_CLI_SET_GFSK_REQ,
   IFX_CLI_SET_RFMODE_REQ,
   IFX_CLI_SET_FREQ_REQ,
   IFX_CLI_SET_TPC_REQ,
   IFX_CLI_GET_BMC_REQ,
   IFX_CLI_GET_XRAM_REQ,
   IFX_CLI_GET_TPC_REQ,
   /* TAPIDEMO-->CLI*/
   IFX_CLI_GET_BMC_IND,
   IFX_CLI_GET_XRAM_IND,
   IFX_CLI_GET_TPC_IND,
}e_IFX_CLI_Events;

/** structure used to communicate with CLI */
typedef struct 
{
   e_IFX_CLI_Events Event;
   union{
      uchar8 ucIsDiag;
      x_IFX_DECT_TransmitPowerParam xTPCParams;
      struct {
         uint16 uiAddr;
         char8  acBuffer[10];
         uchar8 ucLength_Maccess;
      }xRam;
      x_IFX_DECT_BMCRegParams xBMC;
      uint16 unGfskValue;
      struct {
         uchar8 ucRFMode;
         uchar8 ucChannelNumber;
         uchar8 ucSlotNumber;
      }xRFMode;
      struct {
         uint16 uiOscTrimValue;
         uchar8 ucP10Status;
      }xOsc;
      uchar8 ucIsTBR6;
      char8  acRFPI[5];
      struct {
         uchar8 ucFreqTx;
         uchar8 ucFreqRx;
         uchar8 ucFreqRange;
      }xFreq;
      }xCLI;
}x_IFX_CLI_Cmd;

/* UNUSED IN TAPIDEMO ENUMS AND FUNCTION DECLARAIONS */
typedef enum{
   IFX_CLI_DECT = 1,
   IFX_CLI_VOIP,
}e_IFX_CLI_MainMenu;

typedef enum{
   IFX_CLI_DECT_DIAG = 1,
   IFX_CLI_DECT_CONFIG, 
}e_IFX_CLI_DectMenu;

typedef enum{
   IFX_CLI_DECT_DIAG_TPC = 1,
   IFX_CLI_DECT_DIAG_BMC,
   IFX_CLI_DECT_DIAG_XRAM,
   IFX_CLI_DECT_DIAG_RFMODE,
   IFX_CLI_DECT_DIAG_OSC,
   IFX_CLI_DECT_DIAG_GFSK,                                                 
   IFX_CLI_DECT_DIAG_RFPI,
   IFX_CLI_DECT_DIAG_RESET_MODEM,
   IFX_CLI_DECT_DIAG_COUNTRY_SETTINGS,
   IFX_CLI_DECT_DIAG_TBR6,
}e_IFX_CLI_DECT_DiagMenu;


/*displayMenu
 Displays the menus upto the menus at the bottom most level

 Arguments ::
 list - list of menu items
 size - size of the list
 */
void displayMenu(char **list,int size);

/*commonMenu
 Displays the menus common to all menus   at the bottom most level

 Arguments ::
 id - helps to identify the option to start with for the common menus
 */
void commonMenu(int id);

typedef enum{
   IFX_CLI_CHAR,
   IFX_CLI_INT16,
   IFX_CLI_UNSIGNED_CHAR,
}e_IFX_CLI_DateType;

/* getInput Function
 
    Reads the input from user and validates

    Arguments ::
    type - helps in specifyin the type to validate and its an Enum "e_IFX_CLI_DataType"
 */
int getInput(e_IFX_CLI_DateType type);


#endif /* __TD_DECT_CLI_H__ */

