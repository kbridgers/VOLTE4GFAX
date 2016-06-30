/******************************************************************************

                              Copyright (c) 2012
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

  ****************************************************************************/

/**
   \file td_ppd.c
   \date 200x-xx-xx
   \brief Phone Plug Detection implementation.

   This file provides functions for Phone Plug Detections (PPD).
*/

#include "td_timer.h"

/** timer info table */
x_IFX_TimerInfo TD_vaxTimerInfo[IFX_MAX_TIMERS];
int32 TD_viTimerMsgFd;
/*  */
extern uint32 vuiTimerLibFd;
static uint8 vcTimerInit = 0;


e_IFX_Return IFX_TIM_TimerMsgReceived()
{
   x_IFX_TimerInfo xTimerInfo;  
   int32 iSz;

   /* Drain out the FIFO and copy the data in xTimerInfo*/ 
   do
   {               
      iSz = TD_OS_DeviceRead(TD_viTimerMsgFd, &xTimerInfo, 
                             sizeof(x_IFX_TimerInfo));  
      if(iSz == sizeof(x_IFX_TimerInfo))
      {
         /*Invoke the Function pointer*/
         xTimerInfo.pfnTimerCallBack(
                     (uint32)xTimerInfo.unTimerId,
                     xTimerInfo.pvPrivateData
                     );
         /*Free the data if allocated*/
         if(xTimerInfo.unPrivateDataLen)
            if(xTimerInfo.pvPrivateData)    
               TD_OS_MemFree(xTimerInfo.pvPrivateData); 
      }
      else 
      {           
          break;      
      }
   }while(1);              
   return IFX_SUCCESS;
}

/**
*/

e_IFX_Return IFX_TIM_Init(char8* szFifoName,
                          uint32* puiTimerFd, 
                          uint32* puiTimerMsgFd)
{
   if ((szFifoName == IFX_NULL) || 
       (puiTimerFd == IFX_NULL) || 
       (puiTimerMsgFd == IFX_NULL))
   {
      printf("Error, Invalid pointer (File: %s, line: %d)\n",
             __FILE__, __LINE__);
      return IFX_FAILURE;
   }

   /* Call MakeFifo */    
   if(Common_CreateFifo(szFifoName) < 0)   
   {
      printf("Error, IFX_TIM_Init - Create FIFO failed\n");
      return IFX_FAILURE;
   }
   /* Call OpenFifo in Read-Write Mode */
   if((TD_viTimerMsgFd = Common_OpenFifo(szFifoName, O_RDWR|O_NONBLOCK)) 
       < 0 )  
   {
      printf("Error, IFX_TIM_Init - Open FIFO failed\n");
      return IFX_FAILURE;
   }
   *puiTimerMsgFd = (uint32) TD_viTimerMsgFd;

   if( 0 != vcTimerInit ) 
   {
      /* Timer already initialized. */
      *puiTimerFd  = vuiTimerLibFd;
      return IFX_SUCCESS;  
   }
   /* Initialize the timers */
   if ( IFX_TLIB_SUCCESS == 
        IFX_TLIB_TimersInit(IFX_MAX_TIMERS,2) )
   {
      *puiTimerFd  = vuiTimerLibFd;
      vcTimerInit = 1;
      return IFX_SUCCESS;
   }

   return IFX_FAILURE;
}

/**
*/
void IFX_TIM_TimeoutHandler(x_IFX_TimerInfo *pxTimerInfo)
{
   x_IFX_TimerInfo xTimerInfo;
   int32 iSz;

   if (pxTimerInfo == IFX_NULL)
   {
      printf("Error, Invalid pointer (File: %s, line: %d)\n",
             __FILE__, __LINE__);
      return;
   }
   /* Pick up the Corresponnding TimerInfo and send it to FIFO */
   memcpy(&xTimerInfo, pxTimerInfo, sizeof(x_IFX_TimerInfo));
   
   /* Write the message to the TIMER FIFO */
   iSz = TD_OS_DeviceWrite(TD_viTimerMsgFd, &xTimerInfo, sizeof(x_IFX_TimerInfo)); 
   if(iSz != sizeof(x_IFX_TimerInfo))
   {   
      printf("Error, IFX_TIM_TimeoutHandler - write to FIFO failed.\n");     
      return;
   }
   /* Memset The corresponding timer info. */
   memset(pxTimerInfo, 0, sizeof(x_IFX_TimerInfo));
   return;
}

/**
This function would be calld when any one module starts timer. 
 -Populates the xTimerInfo
 -Searches for the free node in the array and attaches it in the list.
 */
e_IFX_Return IFX_TIM_TimerStart(
              uint32 uiTimeOut,
              void* pvPrivateData,
              uint16 unPrivateDataLen,
              pfn_IFX_TIM_TimerCallBack pfnTimerCallBack,
              uint32* puiTimerId
             )

{
   int32 iCnt;
   uint16 *punTimerId;
   x_IFX_TimerInfo * pxTimerInfo;
   /*Check on Input parameters*/

   /* debug trace */
   /* TD_DECT_DEBUG(TAPIDEMO, DBG_LEVEL_LOW,
                 ("DECT STACK: IFX_TIM_TimerStart\n")); */
   if((!pfnTimerCallBack) || (!uiTimeOut))
   {
      printf("Error, IFX_TIM_TimerStart, Invalid input argument\n");
      /* error handling */
      return IFX_FAILURE;
   }

   /* get the free node */
   for(iCnt=0; iCnt<IFX_MAX_TIMERS; iCnt++)
   {
      if (0 == TD_vaxTimerInfo[iCnt].unTimerId )
      {
         break;
      }
   }
   if (iCnt == IFX_MAX_TIMERS)
   {
      printf("Error, IFX_TIM_TimerStart, no free node\n");
      return IFX_FAILURE;
   }

   pxTimerInfo = &TD_vaxTimerInfo[iCnt];

   /*Populate the TimerInfo*/
   pxTimerInfo->pfnTimerCallBack = pfnTimerCallBack;
   punTimerId = &(pxTimerInfo->unTimerId);
   pxTimerInfo->uiTimerId = (uint32)pxTimerInfo; 

   if(!pvPrivateData)
   {
      /*Private data passed as Null pointer*/
      pxTimerInfo->unPrivateDataLen = 0; 
      pxTimerInfo->pvPrivateData = pvPrivateData;     
   }
   else if(0 == unPrivateDataLen)
   {
      /*Save the pointer*/
      pxTimerInfo->unPrivateDataLen = unPrivateDataLen; 
      pxTimerInfo->pvPrivateData = pvPrivateData; 
   }
   else
   {
      /*Malloc the pointer and save the data*/
      pxTimerInfo->pvPrivateData = (void *)TD_OS_MemAlloc(unPrivateDataLen); 
      if (pxTimerInfo->pvPrivateData == IFX_NULL)
      {
         printf("Error, IFX_TIM_TimerStart, memory allocation error.\n");
         return IFX_FAILURE;
      }
      pxTimerInfo->unPrivateDataLen = unPrivateDataLen; 
      memcpy(pxTimerInfo->pvPrivateData,pvPrivateData,unPrivateDataLen);
   }

   /*Convert Tmeout vale in to Microseconds.*/
   uiTimeOut *=1000;   

   /* start the timer */
   if( IFX_TLIB_FAIL ==  IFX_TLIB_StartTimer(&pxTimerInfo->unTimerId,
                            uiTimeOut, 0, (pfnVoidFunctPtr) IFX_TIM_TimeoutHandler, pxTimerInfo) )
   { /* error handling */
      printf("Error, IFX_TIM_TimerStart, Adding Timer failure\n");
      return IFX_FAILURE;
   }
   /* printf("\nTimer started (id=%d) Timeout value: %d \n",
               pxTimerInfo->unTimerId,uiTimeOut/1000);*/

   if(puiTimerId)
      *puiTimerId = (uint32)(pxTimerInfo->unTimerId);
   return IFX_SUCCESS;
}

/**
This function would be invoked when the timer is required to be stopped  
*/
e_IFX_Return IFX_TIM_TimerStop(uint32 uiTimerId )
{
   int32 iRetVal,iCnt;
   x_IFX_TimerInfo *pxTimerInfo;

   /* get the timer node */
   for(iCnt=0; iCnt<IFX_MAX_TIMERS; iCnt++)
   {
      if (uiTimerId ==(uint32)TD_vaxTimerInfo[iCnt].unTimerId)
      {
         break;
      }
   }
   if (iCnt == IFX_MAX_TIMERS)
   {
      /* Node not found */      
      printf("Error, IFX_TIM_TimerStop, no free node\n");
      return IFX_FAILURE;
   }
   pxTimerInfo = &TD_vaxTimerInfo[iCnt];
   if (!pxTimerInfo->unTimerId)
   {
      /* Node already cleared */      
      printf("Error, IFX_TIM_TimerStop, Node already cleared\n");
      return IFX_FAILURE;
   }

   /*Stop the Timer*/
   iRetVal = IFX_TLIB_StopTimer(pxTimerInfo->unTimerId);

   if(iRetVal < 0)
   {
      /* ideally this should not fail, but if it fails it is still OK */
      printf("Error, IFX_TIM_TimerStop, IFX_TLIB_StopTimer returned error.\n");
      return IFX_FAILURE;
   }

   /*Free the pointer if allocated*/
   if(pxTimerInfo->unPrivateDataLen)
      if(pxTimerInfo->pvPrivateData)
         TD_OS_MemFree(pxTimerInfo->pvPrivateData);

   /*Memset the data*/
   memset(pxTimerInfo,0,sizeof(x_IFX_TimerInfo));

   return IFX_SUCCESS;
}

/**
This function does following actions
-Remove the timer fifo
-Unregisterwith timer library
*/
void IFX_TIM_Shut(char8* szFifoName)
{
   close(TD_viTimerMsgFd);
   unlink(szFifoName);
   vcTimerInit = 0;
   return;
}

