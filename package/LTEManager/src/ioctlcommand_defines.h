/*
 * ioctlcommand_defines.h
 *
 *  Created on: 03-May-2015
 *      Author: vvdnlt230
 */

#ifndef IOCTLCOMMAND_DEFINES_H_
#define IOCTLCOMMAND_DEFINES_H_
#include "ATCommands.h"
#include <sys/ioctl.h>


/*IOCTL Commands*/
#define READ_DEVICE_SIGNAL_STRENGTH _IOWR('w', 1, LTEDevicesignalstrength_t *)
#define READ_DEVICE_IMEI_NUMBER _IOWR('w', 2, LTEDeviceInfo_t *)
#define READ_DEVICE_IMSI_NUMBER _IOWR('w', 3, LTEDeviceInfo_t *)			/*Setting Device Address to the driver*/
#define READ_DEVICE_NUMBER _IOWR('w',4,LTEDeviceInfo_t *)
#define READ_DEVICE_QUERY_ALL _IOWR('w',5,LTEDeviceInfo_t *)

/*IOCTL Commands*/
#define WRITE_DEVICE_SIGNAL_STRENGTH _IOWR('w',6, LTEDevicesignalstrength_t *)
#define WRITE_DEVICE_IMEI_NUMBER _IOWR('w', 7, LTEDeviceInfo_t *)
#define WRITE_DEVICE_IMSI_NUMBER _IOWR('w', 8, LTEDeviceInfo_t *)			/*Setting Device Address to the driver*/
#define WRITE_DEVICE_NUMBER _IOWR('w',9,LTEDeviceInfo_t *)
#define WRITE_DEVICE_QUERY_ALL _IOWR('w',10,LTEDeviceInfo_t *)


#endif /* IOCTLCOMMAND_DEFINES_H_ */
