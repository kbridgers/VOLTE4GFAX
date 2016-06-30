/*
 * monitorprocess.c
 *
 *  Created on: 10-Feb-2015
 *      Author: vvdnlt230
 *
 *      Des: All the Main monitoring process
 *
 *
 */
#include "monitorprocess.h"
#include <stdio.h>

int Monitor_process()
{
#if EN_DEBUG_MSGS
	printf("I am in parent process\n");
#endif
	while(1);
	return 0;
}
