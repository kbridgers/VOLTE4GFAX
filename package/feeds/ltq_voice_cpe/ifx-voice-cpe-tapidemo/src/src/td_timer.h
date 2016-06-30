#ifndef _TD_TIMER_H
#define _TD_TIMER_H
/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************
   Module      : td_timer.h
   Description : This file provides functions for timers.
 ****************************************************************************
   \file td_timer.h

   \remarks

   \note Changes:
*******************************************************************************/

#include "ifx_types.h"

#include <stdio.h>
#include <string.h>
/* According to POSIX 1003.1-2001 */
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Workaround - so IFX_TRUE and IFX_FALSE will not be defined for the second
   time in file ifx_types.h and ifx_common_defs.h. Workaround should be used
   until both ifx_types.h and ifx_common_defs.h must be used */
#define IFX_TRUE 
#define IFX_FALSE 
#include "ifx_common_defs.h"
#undef IFX_TRUE
#undef IFX_FALSE

#include "common.h"
#include "IFX_TLIB_Timlib.h"

/*! \brief Maximum number of timers supported by timer module */
#define IFX_MAX_TIMERS         40

/*! 
   \brief   This is the timer callback function.  This function is implemented
   by the module, requesting for services from the Timer module.  This function
   pointer needs to be passed to the Timer module when requesting for a timer 
   to be started.  This function will be called back when the timer expires.
   \param[in] uiTimerId The timer Id of the Timer which is expired.  
   \param[in] pvPrivateData Pointer to Data given by 
               the module which triggered the timer
   \return  void No Value
*/
typedef void (*pfn_IFX_TIM_TimerCallBack)(
                    IN uint32 uiTimerId,
                    IN void* pvPrivateData );

typedef struct
{
        uint16 unTimerId;               /*The Timer Id returned by the TLIB timer library*/
        void * pvPrivateData;   /*The private data*/
        uint16 unPrivateDataLen;/*Length of private data*/
        uint32 uiTimerId;               /*The Timer Id returned by timer agent*/
        pfn_IFX_TIM_TimerCallBack pfnTimerCallBack;
}x_IFX_TimerInfo;

/*!
   \brief  This function indicates the availability of a message in the Timer FIFO.
   This function must be called when there is data available for reading in 
           the fifo descriptor passed during initialization. This functions reads the fifo
           data and invokes the appropriate timer callback function.
   \param[in] iTimFd Timer Fifo Fd that is used for reading the 
           timer message.
   \return If returned IFX_FAILURE, then something wrong with timer interface module
           and caller should reinitialize this module.
*/
e_IFX_Return IFX_TIM_TimerMsgReceived();

/*!
   \brief      This function initializes the Timer Interface module.
   This function is invoked when the timer module is initialized.
   \param[in]  szFifoName Name of fifo to be used by timer interface module          
   \param[out] puiFifoFd Fifo descriptor is returned in this parameter. Caller 
               of this function should wait for this fd and when ready for reading,
               call IFX_TIM_TimerMsgReceived to process and invoke timer callback.
   \return     If timer initialization is success IFX_SUCCESS is returned, else
               IFX_FAILURE is returned.
*/
e_IFX_Return IFX_TIM_Init(char8* szFifoName,
                          uint32* puiTimerFd, 
                          uint32* puiTimerMsgFd);


/*!
   \brief      This function shuts down the timer interface module. 
   \return     void No Value
*/
void IFX_TIM_Shut(char8* szFifoName);

/*! 
    \brief       This function starts a timer.  This function is called  by
    modules to start a timer. The modules can start more than one timer. 
    \param[in]   uiTimeOut     Time out value in milliseconds.
    \param[in]   pvPrivateData Private data of the caller. Timer module does not 
                 modify this data.
    \param[in]   uiPrivateDataLen Length of the private data. If length is zero
                 no data will be copied, only pointer (pvPrivateData) value will
                 be passed into timer callback function.
    \param[in]   pfnTimerCallBack Pointer to Timer call back function
    \param[out]  puiTimerId On success, this contains the Timer Id. 
      This Id is unique and should be used to stop timer and also when timer fires.
    \return      On success returns IFX_SUCCESS otherwise IFX_FAILURE.
*/
e_IFX_Return IFX_TIM_TimerStart(
           IN uint32 uiTimeOut,
           IN void* pvPrivateData,
           IN uint16 uiPrivateDataLen,
           IN pfn_IFX_TIM_TimerCallBack pfnTimerCallBack,
           OUT uint32* puiTimerId
          );

/*! 
    \brief  This function stops a previously started timer.  
    The modules invoke this function to stop the timer that was started 
    by invoking IFX_TIM_StartTimer.  If timer had fired already or if the 
    Timer Id is wrong, this function returns failure.
    \param[in] uiTimerId  Timer Id of the timer to be stopped.
    \return On success returns IFX_SUCCESS otherwise IFX_FAILURE.
*/
e_IFX_Return IFX_TIM_TimerStop(
          IN uint32 uiTimerId
          );



#endif /* _TD_TIMER_H */

