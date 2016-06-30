/*
 * config_device.h
 *
 *  Created on: 14-Mar-2015
 *      Author: vvdnlt230
 */

#ifndef CONFIG_DEVICE_H_
#define CONFIG_DEVICE_H_


#define MODEMDEVICE 				"/dev/ttyUSB3"
#define BAUDRATE 					B115200
#define CODEC_NAME 					"/dev/codec"
//This Will test the I2C Read/Write before attempting to the Initialization
#define ENABLE_I2C_RDWR_TEST 				1		//Enables only the Read Write Test
#define ENABLE_CODEC 						0

typedef enum
{
	RESET,SET
};



#define RESP_INDEX_0	0
#define RESP_INDEX_1	1
#define RESP_INDEX_2	2
#define RESP_INDEX_3	3
#define RESP_INDEX_4	4
#define RESP_INDEX_5	5
#define RESP_INDEX_6	6
#define RESP_INDEX_7	7
#define RESP_INDEX_8	8
#define RESP_INDEX_9	9
#define RESP_INDEX_10	10
#define RESP_INDEX_11	11
#define RESP_INDEX_12	12
#define RESP_INDEX_13	13
#define RESP_INDEX_14	14
#define END_OF_STRING 0


#endif /* CONFIG_DEVICE_H_ */
