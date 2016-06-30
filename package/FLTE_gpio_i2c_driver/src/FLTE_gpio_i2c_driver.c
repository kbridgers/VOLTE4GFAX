/**************************************************************************************************
 * Name					:		tlv320aic34.ko
 * @desc				:		gpio based i2C driver bydirect accessing the gpios.
 * 								no kernel support
 * @author				:		sanju
 * ***********************************************************************************************/
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/ioctl.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/ioctl.h>
#include <asm/io.h>
#include <asm/ifx/ifx_gpio.h>


/*I2C Driver.h contents*/
void I2Cstart(void);
void I2CWriteByte(unsigned char data );
int I2CAckWait(void);
void I2CStop(void);
int I2CGetByte(void);
void I2CWriteFullByte(unsigned char reg_addr,unsigned char data);
unsigned char I2CReadFullByte(unsigned char reg);
#define I2C_FREQUENY 	10			//It is Actually the half period Atually freq will be 2*I2C_FREQUENCY
#define I2C_BIT_WIDTH	8
/*******************************************************/
/**I2Cgpiodriver.h***/
/*Corresponding GPIO pins you wan to configure*/
#define GPIO_SDA_PIN 	9//9
#define GPIO_SCL_PIN	14//10
#define I2C_RESET_PIN	15


typedef enum {RESET,SET}eStatus_t;

typedef enum {INPUT,OUTPUT}eDirection_t;
typedef enum {LOW,HIGH}eState_t;

unsigned char I2C_GPIOInit(unsigned char moduleid);
unsigned char I2C_GPIODeInit(unsigned char moduleid);
unsigned char GpioGetValue(unsigned char pin,unsigned char moduleid);
void GpioSetDirection(unsigned char pin,eDirection_t eDirection,unsigned char moduleid);
void GpioSetValue(unsigned char pin,unsigned char value,unsigned char moduleid);
void SetSDA(void);
void SetSCL(void);
void ClearSDA(void);
void ClearSCL(void);
int GetSDA(void);
/*******************************************************/


/***********User space to kernel Space structure*************/
typedef struct
{
	unsigned char cmd_data;				//data asociated with cmd
	unsigned char regAddress;			//Address to be written
	unsigned char regRddata;			//Data to be read
	unsigned char regWrdata;			//Data to be written
}I2C_Param_t;
/*************************************************************/


/*I2C Configuration Structure need to add more if more needed*/
typedef struct
{
	unsigned char device_id;
	unsigned char frequency;					//Frequency of the i2C communication in us (1us means 1us for low and 1 us for high)
}I2CDeviceConfig_t;
/************************************************************/

/*Global I2C Configuration Structure*/
static I2CDeviceConfig_t I2CDevice;


static int device_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg);
static int device_open(struct inode *inod, struct file *fil);
static int device_release(struct inode *inod, struct file *fil);
static ssize_t device_write(struct file *fil, char *buf, size_t len, loff_t *off);
static ssize_t device_read(struct file *fil, char *buf, size_t len, loff_t *off);

struct file_operations fops =
{
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .release = device_release,
//      .unlocked_ioctl = mydriver_ioctl,
      .ioctl = device_ioctl,
};


int Major;
//#define ADDRESS_READ 0x31	//0xA5
//#define ADDRESS_WRITE 0x30 	//0xA4
#define MODULE_ID IFX_GPIO_MODULE_I2C


/*IOCTL Commands*/
#define I2C_WRITE _IOWR('q', 1, I2C_Param_t *)
//#define QUERY_CLR_VARIABLES _IO('q', 2)
#define I2C_READ _IOWR('q', 3, I2C_Param_t *)
#define I2C_SET_ADDRESS _IOWR('q', 2, I2C_Param_t *)			/*Setting Device Address to the driver*/
#define I2C_SET_FREQUENCY _IOWR('q',4,I2C_Param_t *)
#define I2C_SET_GPIOPINS	_IOWR('q',5,I2C_Param_t *)
/*End of te IOCtl Commands*/



/******************************************************************************
 * Name : Device Create. Registering the Device
 * @desc: Every insmod will do the function call
 *****************************************************************************/
int __init init_mod(void)
{
	if(0==I2C_GPIOInit(IFX_GPIO_MODULE_I2C))			/*Calling the I2C Gpio initialisation*/
	{
		printk(  "I2C Init Succesfull \n");
	}
	else
	{
		printk(  "I2C module Init Failed\n");
		return -1;
	}
	/*Registering the Device*/
	Major = register_chrdev(0,"codec",&fops);
	if(Major < 0)
			printk("Registering device unsuccesfull\n");
	else
			printk("\n\n\nRegistered device and major number is %d\n",Major);
	/*Return Success 0*/
	return 0;
}


/******************************************************************************
 * Name : Device Exit Exit From the Device
 * @desc: Every rmmod will do the function call
 *****************************************************************************/
static void __exit cleanup_mod(void)
{
    unregister_chrdev(Major,"codec");
    I2C_GPIODeInit(IFX_GPIO_MODULE_I2C);
    printk("Un-registered codec device\n");
}
/*****************************************************************************/


/******************************************************************************
 * Name : Device Open
 * @desc: Every Read/Write needs Open and Close
 *****************************************************************************/
static int device_open(struct inode *inod, struct file *fil)
{
        //printk("Device opened\n");
        return 0;
}
/*****************************************************************************/

/******************************************************************************
 * Name : Device Release
 * @desc: Releasing the Control from the device node
 *****************************************************************************/
static int device_release(struct inode *inod, struct file *fil)
{
        //printk("Device released\n");
        return 0;
}
/*****************************************************************************/

static int device_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg)
{
	I2C_Param_t q;
	    switch (cmd)
	    {
	        case I2C_READ:
	        	//Give to the User Space
	        	if (copy_from_user(&q, (I2C_Param_t *)arg, sizeof(I2C_Param_t)))
	        	{
	        		                return -EACCES;
	        	}
	        	// q.cmd_data=0x30;q.regAddress=0x08;q.regRddata=0x48;q.regWrdata=0x49;
	        	 //printk("Data given to the User is %d,%d,%d,%d\n",q.cmd_data,q.regAddress,q.regRddata,q.regWrdata);
	        	//q.regAddress=0x08;
	        	q.regRddata=I2CReadFullByte(q.regAddress);

	        	if (copy_to_user((I2C_Param_t *)arg, &q, sizeof(I2C_Param_t)))
	            {
	                return -EACCES;
	            }

	            break;
	        case I2C_WRITE:
	        	//Take the data from the user space
	            if (copy_from_user(&q, (I2C_Param_t *)arg, sizeof(I2C_Param_t)))
	            {
	                return -EACCES;
	            }
	            I2CWriteFullByte(q.regAddress,q.regWrdata);
	            //printk(  "Data came  From the User is %d,%d,%d,%d\n",q.cmd_data,q.regAddress,q.regRddata,q.regWrdata);

	            break;

	        case I2C_SET_ADDRESS:
	        	//Take the data from the user space
	            if (copy_from_user(&q, (I2C_Param_t *)arg, sizeof(I2C_Param_t)))
	            {
	                return -EACCES;
	            }
	        	I2CDevice.device_id=q.cmd_data;
	        	//printk(  "address is %d\n",I2CDevice.device_id);
	            //Setting the I2C adress here
	           // printk(  "Data came  From the User is %d,%d,%d,%d\n",q.device_address,q.regAddress,q.regRddata,q.regWrdata);

	            break;
	        case I2C_SET_FREQUENCY:
	        	//Take the data from the user space
	        	//Setting the I2C Frequency Here
	            if (copy_from_user(&q, (I2C_Param_t *)arg, sizeof(I2C_Param_t)))
	            {
	                return -EACCES;
	            }
	        	I2CDevice.frequency=q.cmd_data;

	            //printk(  "frequency is %d\n",I2CDevice.frequency);

	            break;
	        case I2C_SET_GPIOPINS:
	        	//Take the data from the user space
	        	//Setting the I2C Frequency Here
	            if (copy_from_user(&q, (I2C_Param_t *)arg, sizeof(I2C_Param_t)))
	            {
	                return -EACCES;
	            }
	            //Read pin && Write Pin
	        	I2CDevice.frequency=q.cmd_data;

	            //printk(  "frequency is %d\n",I2CDevice.frequency);

	            break;

	        default:
	            return -EINVAL;
	    }

	    return 0;
}




/******************************************************************************
 * Name : Device Write
 * @desc: I2C Devices basically Do not Need A Read And write Directly
 * 			It will Always Communicate with the ioctl commands
 *
 *****************************************************************************/
static ssize_t device_read(struct file *fil, char *buf, size_t len, loff_t *off)
{
	//int data;
    printk("Read attempted\n");
#if 0
    int data;
    I2Cstart();
     I2CWriteByte(0x30);	//1
     I2CAckWait();
     printk(  "ACK_OK 1\n");
     I2CWriteByte(0x08);			//2
     I2CAckWait();
     printk(  "ACK_OK 2\n");
     I2CWriteByte(0xC0);			//3
     udelay(1);                      //This Will be No ACk
     ClearSCL();
	 udelay(1);
     printk(  "ACK_OK 3\n");
     I2CStop();
     I2Cstart();
     I2CWriteByte(0x30);		//4
     I2CAckWait();
     printk(  "ACK_OK 4\n");
     I2CWriteByte(0x08);				//5
     I2CAckWait();
     printk(  "ACK_OK 5\n");
     I2Cstart();
     I2CWriteByte(0x31);			//6
     I2CAckWait();
     printk(  "ACK_OK 6\n");

     data=I2CGetByte();                      //7        //Getting i2c data
     I2CStop();
     printk(  "The data came is Ok  %x\n",data);
     while(1);
#endif

    return len;
}

/******************************************************************************
 * Name : Device Read
 * @desc: I2C Devices basically Do not Need A Read And write Directly
 * 			It will Always Communicate with the ioctl commands
 *
 *****************************************************************************/
 static ssize_t device_write(struct file *fil, char *buf, size_t len, loff_t *off)
{
	// SetSDA();		//not needed added for GPIO testing
	// SetSCL();
	if(buf[0]=='1')
	{
		;//printk("Writing 1\n");
	}
	else
	{
		;//printk("writing 0 \n");
	}
        return len;
}

/*Exporting the Init And Exit Functio Macros*/
module_init(init_mod);
module_exit(cleanup_mod);

MODULE_LICENSE("GPL");


//No use but added for understanding Read bit and Write bit
#define FLAG_SET_READ 	1<<0
#define FLAG_SET_WRITE 	0<<0

/*****************************
 * I2C Driver.c Source File
 */

unsigned char I2CReadFullByte(unsigned char reg)
{
	unsigned char reg_data;
	 ;//printk( "Device id trying is %x\n",(I2CDevice.device_id)|FLAG_SET_WRITE);
	 I2Cstart();
	 I2CWriteByte((I2CDevice.device_id)|FLAG_SET_WRITE);		//4
	 if(I2CAckWait()==0)
		;//printk(  "ACK_OK After Device ID\n");
	 else
		 printk(  "I2C acknowledgement Failed-1\n ");
	 I2CWriteByte(reg);				//5
	 if(I2CAckWait()==0)
		;//printk(  "ACK_OK After Reg number\n");
	 else
		 printk(  "I2C acknowledgement Failed-2\n ");
	 //printk(  "ACK_OK 5\n");
	 I2Cstart();
	 I2CWriteByte((I2CDevice.device_id)|FLAG_SET_READ);			//6
	 if(I2CAckWait()==0)
		 ;	 	 	 	 //printk(  "ACK_OK After Device ID after read address\n");
	 else
		 printk(  "I2C acknowledgement Failed-3\n ");
	//printk(  "ACK_OK 6\n");
	 reg_data=I2CGetByte();                      //7        //Getting i2c data
	 I2CStop();
	// printk(  "The data came is Ok Read--  %x\n",reg_data);

	 return reg_data;

}
void I2CWriteFullByte(unsigned char reg_addr,unsigned char data)
{
	//printk( "Device id trying is %x\n",(I2CDevice.device_id )|FLAG_SET_WRITE);
     I2Cstart();
	 I2CWriteByte((I2CDevice.device_id)|FLAG_SET_WRITE);	//1
	 if(I2CAckWait()==0)
		 ;//printk(  "ACK_OK After Device ID - Write\n");
	 else
		 printk(  "I2C acknowledgement Failed-1 -write\n ");
	 I2CWriteByte(reg_addr);			//2
	 if(I2CAckWait()==0)
		;//printk(  "ACK_OK After reg no - Write\n");
	 else
		 printk(  "I2C acknowledgement Failed-2 -write\n ");
	 //printk(  "Write ACK_OK 2\n");
	 I2CWriteByte(data);			//3
	 udelay(1);                      //This Will be No ACk
	 ClearSCL();
	 udelay(10);
	// printk(  "Write ACK_OK 3\n");
	 I2CStop();
	// printk(  "The data came is Ok  %x\n",data);
}




/*This File includes All the I2C related Functions*/
/*Thump Rule --> Do not Change the SDA state when SCL is HIGH (Except START & STOP)*/
/******************************************************************************
 * I2C Start Function
 * Start Means the SDA HIGH->LOW transition When SCL is HIGH
 * Function Still need to fine tune
 *****************************************************************************/
void I2Cstart(void)
{
    SetSDA();
	SetSCL();
    udelay(I2C_FREQUENY);
    ClearSDA();               //Pulled Low So Start
    udelay(I2C_FREQUENY);
    ClearSCL();
    udelay(I2C_FREQUENY);
}
/******************************************************************************
 * I2C Write Byte
 * Function Still need to Fine tune
 *****************************************************************************/
void I2CWriteByte(unsigned char data )
{
	int bit_cnt=I2C_BIT_WIDTH-1,i;
	for(i=7;i>=0;i--)
	{
   		if((data>>bit_cnt)&0x01)
        	{
          		SetSDA();
        	}
        	else
        	{
          		ClearSDA();
        	}
			udelay(10);
    		SetSCL();
       		udelay(I2C_FREQUENY);
            ClearSCL();
            udelay(I2C_FREQUENY);
            //bit_cnt--;
            if(bit_cnt==0)                          //All bits Over
            {
            	SetSDA();
                SetSCL();
                udelay(I2C_FREQUENY);
                return ;
            }
            bit_cnt--;
	}
}

#if 1
int I2CAckWait(void)
{
     //GpioSetDirection(GPIO_SDA_PIN,INPUT,IFX_GPIO_MODULE_I2C);
       udelay(I2C_FREQUENY);					//With in this timeout I2C should respond
       if(LOW==GetSDA())//Add timeout also
       {
    	   udelay(I2C_FREQUENY);
       	   ClearSCL();
       	   udelay(I2C_FREQUENY);
       	 //  GpioSetDirection(GPIO_SDA_PIN,OUTPUT,IFX_GPIO_MODULE_I2C);
       	   return 0;
       }
       else
       {
    	   printk(  " ACK Timeout - I2C not Responding\n");
       	   ClearSCL();
       	   udelay(I2C_FREQUENY);
       //	   GpioSetDirection(GPIO_SDA_PIN,OUTPUT,IFX_GPIO_MODULE_I2C);
    	   return -1;
       }
}
#else

int I2CAckWait(void)
{
       GpioSetDirection(GPIO_SDA_PIN,INPUT,IFX_GPIO_MODULE_I2C);
      // udelay(100);					//With in this timeout I2C should respond
      while(HIGH==GetSDA());
       //if(LOW==GetSDA())//Add timeout also
       //{
    	   udelay(I2C_FREQUENY);
       	   ClearSCL();
       	   udelay(I2C_FREQUENY);
       	   GpioSetDirection(GPIO_SDA_PIN,OUTPUT,IFX_GPIO_MODULE_I2C);
       	   return 0;
       //}
       //else
       //{
    	 //  printk(  " ACK Timeout - I2C not Responding\n");
    	  // return -1;
       //}
}

#endif

void I2CStop(void)
{
	ClearSDA();
	udelay(I2C_FREQUENY);
	SetSCL();
	udelay(I2C_FREQUENY);
	SetSDA();
	udelay(I2C_FREQUENY);
}




int I2CGetByte(void)
{
	int i=0,bit_value=0,data=0;
	//GpioSetDirection(GPIO_SDA_PIN,INPUT,IFX_GPIO_MODULE_I2C);
	for(i=I2C_BIT_WIDTH-1;i>=0;i--)
	{
		SetSCL();
		udelay(1);
		bit_value=GetSDA();
		data|=((bit_value&0x01)<<i);
		udelay(I2C_FREQUENY);
		ClearSCL();
		udelay(I2C_FREQUENY);
	}
	SetSDA();
	SetSCL();
	udelay(I2C_FREQUENY);
	if(HIGH==GetSDA())
	{
			//No Ack
	//		printk(  " Read ACK Error\n");
	}
	//GpioSetDirection(GPIO_SDA_PIN,OUTPUT,IFX_GPIO_MODULE_I2C);
	//printk(  "Read ACK Success\n");
	return data;
}




/*****************************
 * I2Cgpiodriver.c Source File
 */

/******************************************************************************
 * Initialising the GPIO I2C
 * @return: Status Success or Not
 *****************************************************************************/
unsigned char I2C_GPIOInit(unsigned char moduleid)
{
	/*SDA Pin Configurations for the I2C*/
	ifx_gpio_register(moduleid);
	ifx_gpio_pin_reserve(GPIO_SDA_PIN, moduleid);
	ifx_gpio_altsel0_clear(GPIO_SDA_PIN, moduleid);
	ifx_gpio_altsel1_clear(GPIO_SDA_PIN, moduleid);
	ifx_gpio_open_drain_clear(GPIO_SDA_PIN, moduleid);
	ifx_gpio_output_set(GPIO_SDA_PIN, moduleid);
	/*SCL Pin Configurations for the I2C*/
	ifx_gpio_dir_out_set(GPIO_SDA_PIN, moduleid);
    ifx_gpio_pin_reserve(GPIO_SCL_PIN, moduleid);
    ifx_gpio_output_set(GPIO_SCL_PIN, moduleid);
    ifx_gpio_altsel0_clear(GPIO_SCL_PIN, moduleid);
    ifx_gpio_altsel1_clear(GPIO_SCL_PIN, moduleid);
 	ifx_gpio_open_drain_clear(GPIO_SCL_PIN, moduleid);
    ifx_gpio_dir_out_set(GPIO_SCL_PIN, moduleid);

	ifx_gpio_dir_out_set(I2C_RESET_PIN, moduleid);
    ifx_gpio_pin_reserve(I2C_RESET_PIN, moduleid);
    ifx_gpio_output_set(I2C_RESET_PIN, moduleid);
    ifx_gpio_altsel0_clear(I2C_RESET_PIN, moduleid);
    ifx_gpio_altsel1_clear(I2C_RESET_PIN, moduleid);
 	ifx_gpio_open_drain_clear(I2C_RESET_PIN, moduleid);
    ifx_gpio_dir_out_set(I2C_RESET_PIN, moduleid);
    GpioSetValue(I2C_RESET_PIN,RESET,moduleid);
    udelay(1000);
    GpioSetValue(I2C_RESET_PIN,SET,moduleid);

#if 0
    ifx_gpio_puden_set(GPIO_SCL_PIN, moduleid);
    ifx_gpio_pudsel_set(GPIO_SCL_PIN, moduleid);
    ifx_gpio_puden_set(GPIO_SDA_PIN, moduleid);
     ifx_gpio_pudsel_set(GPIO_SDA_PIN, moduleid);
#endif
     return 0;
}

unsigned char I2C_GPIODeInit(unsigned char moduleid)
{
	ifx_gpio_deregister(moduleid);
	return 0;
}

;


/******************************************************************************
 * Setting the desired direction to GPIO pin
 * @args :	pin --> gpio pin number
 * 			direction --> desired direction to set
 * 			moduleid --> corresponding module it belongs
 * @return: nil
 *****************************************************************************/
void GpioSetDirection(unsigned char pin,eDirection_t eDirection,unsigned char moduleid)
{
	if(INPUT==eDirection)			/*Set 0*/
	{
		ifx_gpio_dir_in_set(pin,moduleid);
	}
	else if(OUTPUT==eDirection)						/*set HIGH if not zero*/
	{
		ifx_gpio_dir_out_set(pin,moduleid);
	}
}
/*****************************************************************************/




/******************************************************************************
 * Setting the desired value to GPIO pin
 * @args :	pin --> gpio pin number
 * 			value --> desired value to set
 * 			moduleid --> corresponding module it belongs
 * @return: nil
 *****************************************************************************/
void GpioSetValue(unsigned char pin,unsigned char value,unsigned char moduleid)
{
	if(RESET==value)			/*Set 0*/
	{
		ifx_gpio_output_clear(pin, moduleid);					//moduleid specific to the lantiq chip
	}
	else						/*set HIGH if not zero*/
	{
		ifx_gpio_output_set(pin, moduleid);
	}
}
/*****************************************************************************/

/******************************************************************************
 * Setting the desired value to GPIO pin
 * @args :	pin --> gpio pin number
 * 			value --> current value of the pin
 * 			moduleid --> corresponding module it belongs
 * @return: value of the pin
 *****************************************************************************/
unsigned char GpioGetValue(unsigned char pin,unsigned char moduleid)
{
	int value;
	ifx_gpio_input_get(pin,moduleid,&value);
	return value;
}
/*****************************************************************************/

/***********************Clearing The SDA Pin**********************************/
void SetSDA(void)
{
	GpioSetValue(GPIO_SDA_PIN,SET,IFX_GPIO_MODULE_I2C);
}
/*****************************************************************************/
/***********************Setting The SCL Pin**********************************/
void SetSCL(void)
{
	GpioSetValue(GPIO_SCL_PIN,SET,IFX_GPIO_MODULE_I2C);
}
/*****************************************************************************/
/***********************Clearing The SDA Pin**********************************/
void ClearSDA(void)
{
	GpioSetValue(GPIO_SDA_PIN,RESET,IFX_GPIO_MODULE_I2C);
}
/*****************************************************************************/
/***********************Clearing The SCL Pin**********************************/
void ClearSCL(void)
{
	GpioSetValue(GPIO_SCL_PIN,RESET,IFX_GPIO_MODULE_I2C);
}
/*****************************************************************************/

/****************Getting the Value of the SDA pin ****************************/
int GetSDA(void)
{
	return GpioGetValue(GPIO_SDA_PIN,IFX_GPIO_MODULE_I2C);
}
/*****************************************************************************/






