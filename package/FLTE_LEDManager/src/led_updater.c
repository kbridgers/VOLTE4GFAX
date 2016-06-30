#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
enum led_indications
{
 LED_SIG_NO =48,//0
 LED_SIG_LOW=49,//1
 LED_SIG_MEDIUM=50,//2
 LED_SIG_HIGH=51,//3
 LED_STAT_FAILURE,//4
 LED_DATA_START,//5
 LED_DATA_END,//6
 LED_PWR_ON,//7
 LED_OTA_UPDATE,//8
 LED_OTA_UPDATE_END//9
};

typedef enum 
{
ON,
OFF
}led_state;
int main()
{
int fdFIFO, fdLEDnode;
int event;
int indicate;
led_state sysL1_state = OFF;//initial state of led
led_state sysL2_state = ON; //initial state of led
fd_set readfd;
struct timeval tv;
int num_of_bytes;
int timer_set=0;
char buff[100];
fdFIFO = open("/dev/fifo_led",O_RDWR | O_NONBLOCK);
fdLEDnode = open("/dev/led",O_RDWR | O_NONBLOCK);
while(1)
{
FD_ZERO(&readfd);
FD_SET(fdFIFO,&readfd);
event = select(fdFIFO+1,&readfd,NULL,NULL,(timer_set>0)?&tv:NULL);
if(-1 == event)
	printf("Error in select()\n");
else if(0<event)
{
	if(FD_ISSET(fdFIFO,&readfd))
	{
		num_of_bytes=read(fdFIFO,buff,sizeof(buff));
		buff[num_of_bytes-1]='\0';
		switch (buff[0])
		{
		case LED_SIG_NO:
		write(fdLEDnode,"sigLhighoff ",strlen("sigLhighoff "));
		write(fdLEDnode,"sigLmedoff ",strlen("sigLmedoff "));
		write(fdLEDnode,"sigLlowoff ",strlen("sigLlowoff "));
		break;

		case LED_SIG_HIGH:
		write(fdLEDnode,"sigLhighon ",strlen("sigLhighon "));
		write(fdLEDnode,"sigLmedoff ",strlen("sigLmedoff "));
		write(fdLEDnode,"sigLlowoff ",strlen("sigLlowoff "));
		break;

		case LED_SIG_MEDIUM:
		write(fdLEDnode,"sigLhighoff ",strlen("sigLhighoff "));
		write(fdLEDnode,"sigLmedon ",strlen("sigLmedon "));
		write(fdLEDnode,"sigLlowoff ",strlen("sigLlowoff "));
		break;

		case LED_SIG_LOW:
		write(fdLEDnode,"sigLhighoff ",strlen("sigLhighoff "));
                write(fdLEDnode,"sigLmedoff ",strlen("sigLmedoff "));
                write(fdLEDnode,"sigLlowon ",strlen("sigLlowon "));
		break;

		case LED_STAT_FAILURE:
		tv.tv_sec=1;
		tv.tv_usec=0;
		timer_set=1;
		indicate=LED_STAT_FAILURE;
		if(OFF == sysL2_state)
        	{
		write(fdLEDnode,"sysL1off ",strlen("sysL1off "));
                write(fdLEDnode,"sysL2on ",strlen("sysL2on "));
                sysL2_state=ON;
        	}
        	else
        	{
		write(fdLEDnode,"sysL1off ",strlen("sysL1off "));
                write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
                sysL2_state=OFF;
        	}
		break;

		case LED_DATA_START:
		tv.tv_sec=0;
		tv.tv_usec=100000;
		timer_set=1;
		indicate=LED_DATA_START;
		if(OFF == sysL1_state)
        	{
		write(fdLEDnode,"sysL1on ",strlen("sysL1on "));
		write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
                sysL1_state=ON;
        	}
        	else
        	{
		write(fdLEDnode,"sysL1off ",strlen("sysL1off "));
                write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
                sysL1_state=OFF;
        	}
		break;

		case LED_OTA_UPDATE_END:
		case LED_DATA_END:
		case LED_PWR_ON:
		tv.tv_sec=0;
                tv.tv_usec=0;
		timer_set=0;
		sysL1_state=ON;	
		sysL2_state=OFF;
		write(fdLEDnode,"sysL1on ",strlen("sysL1on "));
                write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
		break;

		case LED_OTA_UPDATE:
		tv.tv_sec=0;
                tv.tv_usec=500000;
		timer_set=1;
		indicate=LED_OTA_UPDATE;
		if(OFF == sysL1_state)
	        {
		write(fdLEDnode,"sysL1on ",strlen("sysL1on "));
		write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
                sysL1_state=ON;
       		}
        	else
        	{
		write(fdLEDnode,"sysL1off ",strlen("sysL1off "));
		write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
                sysL1_state=OFF;
        	}
		break;
		
		default:
		printf("Invalid LED indication\n");
		}

	}
}
else if(0==event)
{
	switch(indicate)
	{
	case LED_DATA_START:
	tv.tv_sec=0;
        tv.tv_usec=100000;
        timer_set=1;
	if(OFF == sysL1_state)
	{
		write(fdLEDnode,"sysL1on ",strlen("sysL1on "));
		write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
		sysL1_state=ON;
		sysL2_state=OFF;
	}
	else
	{	
		write(fdLEDnode,"sysL1off ",strlen("sysL1off "));
		write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
		sysL1_state=OFF;
		sysL2_state=OFF;
	}
	break;

	case LED_STAT_FAILURE:
	tv.tv_sec=1;
        tv.tv_usec=0;
        timer_set=1;
	 if(OFF == sysL2_state)
        {
		write(fdLEDnode,"sysL1off ",strlen("sysL1off "));
		write(fdLEDnode,"sysL2on ",strlen("sysL2on "));
		sysL2_state=ON;
		sysL1_state=OFF;
        }
        else
        {
		write(fdLEDnode,"sysL1off ",strlen("sysL1off "));
		write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
		sysL2_state=OFF;
		sysL1_state=OFF;
        }
        break;

	case LED_OTA_UPDATE:
	tv.tv_sec=0;
        tv.tv_usec=500000;
        timer_set=1;
	 if(OFF == sysL1_state)
        {
		write(fdLEDnode,"sysL1on ",strlen("sysL1on "));
		write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
		sysL1_state=ON;
		sysL2_state=OFF;
        }
        else
        {
		write(fdLEDnode,"sysL1off ",strlen("sysL1off "));
                write(fdLEDnode,"sysL2off ",strlen("sysL2off "));
		sysL1_state=OFF;
		sysL2_state=OFF;
        }
        break;
	}
}
}
return 1;
}
