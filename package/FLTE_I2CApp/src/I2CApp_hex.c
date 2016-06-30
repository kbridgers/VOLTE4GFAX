/*
 ============================================================================
 Name        : gpioapp.c
 Author      : Sanju Varghese
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
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





// Address "W" "10" "value"
int main(int argc, char *argv [])
{
	int fd,i;
	I2C_Param_t I2CParam;
	action_t action=ACTION_NULL;
	if (argc<=1)
	{
		printf("Too Few Arguments\n");
		return 1;
	}
	//printf("Hello\n");
	if(strcmp(argv[2],"r")==0)
	{
		//printf("Entered for read \n");
		action=ACTION_READ;
	}
	else if (strcmp(argv[2],"w")==0)
	{
		//printf("Entered for Write \n");
		action=ACTION_WRITE;
	}
	else if (strcmp(argv[2],"a")==0)
	{
		//printf("Entered for read all \n");
		action=ACTION_READ_ALL;
	}
	I2CParam.cmd_data=stringtoint(argv[1]);
	I2CParam.regAddress=stringtoint(argv[3]);
	if(action==ACTION_WRITE)
		I2CParam.regWrdata=stringtohex(argv[4]);			//Convert to hex


	//printf("Details are address,reg address,value%d,%d,%d\n",I2CParam.cmd_data,I2CParam.regAddress,I2CParam.regWrdata);
	fd=open("/dev/codec", O_RDWR |O_NONBLOCK);
	if(fd<0)
	{
		printf("Device not Found \n");
	}
	switch(action)
	{
		case  ACTION_READ:
			//I2CParam.cmd_data=0x30;
			//printf("Executing Read : I2c Address,Register in hex %x,%x\n",I2CParam.cmd_data,I2CParam.regAddress);

			ioctl(fd,I2C_SET_ADDRESS,&I2CParam);
			ioctl(fd,I2C_READ,&I2CParam);
			//printf("Read data is %x \n",I2CParam.regRddata);

			break;
		case  ACTION_WRITE:
			//I2CParam.cmd_data=0x30;
			//I2CParam.cmd_data=0x32;
			ioctl(fd,I2C_SET_ADDRESS,&I2CParam);
			ioctl(fd,I2C_WRITE,&I2CParam);
			break;
		case  ACTION_READ_ALL:
			//I2CParam.cmd_data=0x30;
			//I2CParam.cmd_data=0x32;
			ioctl(fd,I2C_SET_ADDRESS,&I2CParam);
			for(i=0;i<=109;i++)
			{
				I2CParam.regAddress=i;
				ioctl(fd,I2C_READ,&I2CParam);
				//printf("Register %d value %dd, 0x%x \n",i,I2CParam.regRddata,I2CParam.regRddata);
			}
			break;
		default:
			printf("In default condition\n");
			break;

	}
	close(fd);
	return 0;
}
