/*
 ============================================================================
 Name        : TestAllPeripherals.c
 Author      : Sanju Varghese
 Version     :
 Copyright   : Your copyright notice
 Description : Test Application for All ciou_flte peripherals
 ============================================================================
 */
#include<stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>


typedef enum
{
	ACTION_READ,
	ACTION_WRITE,
	ACTION_TOGGLE,
	ACTION_READ_ALL,
	ACTION_NULL,
}action_t;



typedef struct
{
	unsigned char cmd_data;					//Device I2C Address
	unsigned char regAddress;						//Address to be written
	unsigned char regRddata;						//Data to be read
	unsigned char regWrdata;						//Data to be written
}I2C_Param_t;


#define I2C_WRITE _IOWR('q', 1, I2C_Param_t *)
#define I2C_READ _IOWR('q', 3, I2C_Param_t *)
#define I2C_SET_ADDRESS _IOWR('q', 2, I2C_Param_t *)			/*Setting Device Address to the driver*/
#define I2C_SET_FREQUENCY _IOWR('q',4,I2C_Param_t *)


int stringtoint(char *str)
{
	int i,len,num=0,multiplier=1;
	len=strlen(str);
	for(i=0;i<len;i++)
	{
		num+=(str[(len-1)-i]-0x30)*multiplier;
		multiplier*=10;
	}
	return num;
}

int stringtohex(unsigned char *str)
{
	int i,len,num=0,multiplier=1;
	len=strlen(str);
	for(i=0;i<len;i++)
	{
		if((str[(len-1)-i])>='0'&&(str[(len-1)-i])<='9')
		{
			num+=(str[(len-1)-i]-0x30)*multiplier;
		}
		else if(((str[(len-1)-i])>='a'&&(str[(len-1)-i])<='f')||(((str[(len-1)-i])>='A'&&(str[(len-1)-i])<='F')))
		{
			switch(str[(len-1)-i])
			{
				case 'a':
				case 'A':
					num+=10*multiplier;
					break;
				case 'b':
				case 'B':
					num+=11*multiplier;
					break;
				case 'c':
				case 'C':
					num+=12*multiplier;
					break;
				case 'd':
				case 'D':
					num+=13*multiplier;
					break;
				case 'e':
				case 'E':
					num+=14*multiplier;
					break;
				case 'f':
				case 'F':

					num+=15*multiplier;
					break;
			}
		}

		multiplier*=16;
	}
	return num;
}


#define CODEC_A_ADDRESS 48					/*codec A address in decimal*/
#define CODEC_B_ADDRESS 50					/*Codec B Address In decimal*/
#define CODEC_A_TEST_REGISTER 8				/*Dummy Register to Read/Write*/
#define CODEC_B_TEST_REGISTER 8				/*Dummy Regiser to Read/Write*/
#define CODEC_DUMMY_WRITE_VALUE 52			/*Dummy Write Value for the Codec*/


// Address "W" "10" "value"
int main(int argc, char *argv [])
{
	int fd,i;
	I2C_Param_t I2CParam;
	action_t action=ACTION_NULL;
#if 0
	printf("Testing Codec \n");
	fd=open("/dev/codec", O_RDWR |O_NONBLOCK);
	if(fd<0)
	{
		printf("No Codec Found. Please Correct It... Application Will not start if Codec is not ready  Rester needed\n");
		while(1);
	}
	/*A Section*/
	I2CParam.cmd_data=CODEC_A_ADDRESS;				//Codec
	I2CParam.regAddress=CODEC_A_TEST_REGISTER;
	I2CParam.regWrdata=CODEC_DUMMY_WRITE_VALUE;
	ioctl(fd,I2C_SET_ADDRESS,&I2CParam);	/*Setting Codec Address*/
	ioctl(fd,I2C_WRITE,&I2CParam);			/*Writing Value To the address*/
	I2CParam.regRddata=0;					/*Clearing the Value before reading*/
	ioctl(fd,I2C_SET_ADDRESS,&I2CParam);
	ioctl(fd,I2C_READ,&I2CParam);
	if(I2CParam.regRddata!=CODEC_DUMMY_WRITE_VALUE)					/*Checking Both values*/
	{
		printf("Codec A Seems not responding... Application Will not start if Codec is not ready.  Restart needed \n");
		while(1);
	}
	else
	{
		printf("Codec A Test Succesfull\n");
	}
	/*B Section*/
	I2CParam.cmd_data=CODEC_B_ADDRESS;				//Codec
	I2CParam.regAddress=CODEC_B_TEST_REGISTER;
	I2CParam.regWrdata=CODEC_DUMMY_WRITE_VALUE;
	ioctl(fd,I2C_SET_ADDRESS,&I2CParam);	/*Setting Codec Address*/
	ioctl(fd,I2C_WRITE,&I2CParam);			/*Writing Value To the address*/
	I2CParam.regRddata=0;					/*Clearing the Value before reading*/
	ioctl(fd,I2C_SET_ADDRESS,&I2CParam);
	ioctl(fd,I2C_READ,&I2CParam);
	if(I2CParam.regRddata!=CODEC_DUMMY_WRITE_VALUE)					/*Checking Both values*/
	{
		printf("Codec B Seems not responding... Application Will not start if Codec is not ready.  Restart needed \n");
		while(1);
	}
	else
	{
		printf("Codec B Test Succesfull\n");
	}
	close(fd);
#endif
	printf("Testing 4G Modem \n");
	fd=open("/dev/ttyUSB3",O_RDWR | O_NOCTTY|O_NDELAY);
	if(fd<0)
	{
		printf("Telit 4G Modem Connection Failed... Application Will not start if modem is not ready.  Restart needed \n");
		while(1);
	}
	else
	{
		printf("4G Modem test Passed\n");
	}
	/*Closing The LTE Modem Tests Passed*/
	close(fd);
	return 0;
}
