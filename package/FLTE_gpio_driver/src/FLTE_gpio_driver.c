#include <linux/module.h>
#include <linux/fs.h>
#include <asm/delay.h>
#include <asm/ifx/ifx_gpio.h>
#include <linux/delay.h>

#define GPIO38       38  //GPIO_LTE_ON_OFF
#define GPIO6        6   //LTE_HW_SHD
#define GPIO15       15  //CODEC_RESET
#define gpio_module_id    IFX_GPIO_MODULE_I2C

#define CodecRst GPIO15
#define LTE_OnOff GPIO38
#define LTE_SHD GPIO6

typedef enum {RESET,SET}eStatus_t;

typedef enum {INPUT,OUTPUT}eDirection_t;
typedef enum {LOW,HIGH}eState_t;

unsigned char gpio_init(void);
unsigned char gpio_remove(void);
void GpioSetValue(unsigned char pin,unsigned char value,unsigned char gpio_module_id);
static int device_open(struct inode *inod, struct file *fil);
static int device_release(struct inode *inod, struct file *fil);
static ssize_t device_write(struct file *fil, const char *buf, size_t len, loff_t *off);

int strcmpr(char * str1, char * str2)
{
int i;
for(i=0;str1[i]!='\0' && str2[i]!='\0';i++)
{
	if(str1[i]==str2[i])
	{
		continue;
	}
	else
	{
		return 0;
	}
}
if(str1[i]=='\0' && str2[i]=='\0')
return 1;
else
return 0;
}

struct file_operations fops =
{
        .write = device_write,
        .open = device_open,
        .release = device_release,
};


int Major;

/******************************************************************************
 * Name : Device Create. Registering the Device
 * @desc: Every insmod will do the function call
 *****************************************************************************/
int __init init_mod(void)
{
	if(0==gpio_init())			/*Calling the I2C Gpio initialisation*/
	{
		printk( KERN_ERR "flte gpios Init Succesfull \n");
	}
	else
	{
		printk( KERN_ERR "flte gpios Init Failed\n");
		return -1;
	}
	/*Registering the Device*/
	Major = register_chrdev(0,"gpio_flte",&fops);
	if(Major < 0)
			printk("Registering gpio_flte device failed\n");
	else
			printk("Registered device gpio_flte with major number %d\n",Major);
	/*Return Success 0*/
	return 0;
}


/******************************************************************************
 * Name : Device Exit Exit From the Device
 * @desc: Every rmmod will do the function call
 *****************************************************************************/
static void __exit cleanup_mod(void)
{
    unregister_chrdev(Major,"gpio_flte");
    gpio_remove();
   // printk(KERN_ERR"Un-registered device\n");
}
/*****************************************************************************/


/******************************************************************************
 * Name : Device Open
 * @desc: Every Read/Write needs Open and Close
 *****************************************************************************/
static int device_open(struct inode *inod, struct file *fil)
{
 //       printk(KERN_ERR"Device opened\n");
        return 0;
}
/*****************************************************************************/

/******************************************************************************
 * Name : Device Release
 * @desc: Releasing the Control from the device node
 *****************************************************************************/
static int device_release(struct inode *inod, struct file *fil)
{
//        printk(KERN_ERR"Device released\n");
        return 0;
}
/*****************************************************************************/

static ssize_t device_write(struct file *fil, const char *buf, size_t len, loff_t *off)
{
	char gpio_buff[30];
        if(copy_from_user(gpio_buff,buf,len)<0)
           return -1;
	gpio_buff[len-1]='\0';
	if(strcmpr(gpio_buff,"CoRst"))
	{
		GpioSetValue(CodecRst,SET,gpio_module_id);
		GpioSetValue(CodecRst,RESET,gpio_module_id);
		mdelay(1500);
		GpioSetValue(CodecRst,SET,gpio_module_id);
	}
	else if(strcmpr(gpio_buff,"CoRstH"))
	{
		GpioSetValue(CodecRst,SET,gpio_module_id);
	}
	else if(strcmpr(gpio_buff,"CoRstL"))
	{
		GpioSetValue(CodecRst,RESET,gpio_module_id);
	}
	else if(strcmpr(gpio_buff,"LTEon"))
        {
		GpioSetValue(LTE_SHD,RESET,gpio_module_id);
                GpioSetValue(LTE_OnOff,SET,gpio_module_id);
		mdelay(5000);
		GpioSetValue(LTE_OnOff,RESET,gpio_module_id);
        }
	else if(strcmpr(gpio_buff,"LTEoff"))
        {
		GpioSetValue(LTE_OnOff,SET,gpio_module_id);
		GpioSetValue(LTE_SHD,RESET,gpio_module_id);
		mdelay(1000);                
		GpioSetValue(LTE_SHD,SET,gpio_module_id);
        }
	else
	{
		printk(KERN_ERR"gpio_flte Driver: value not supported\n");
	}
	return len;
}

/*Exporting the Init And Exit Functio Macros*/
module_init(init_mod);
module_exit(cleanup_mod);

MODULE_LICENSE("GPL");


/******************************************************************************
 * Initialising the GPIO I2C
 * @return: Status Success or Not
 *****************************************************************************/
unsigned char gpio_init()
{
	ifx_gpio_register(gpio_module_id);
	//GPIO_LTE_ON_OFF
	ifx_gpio_pin_reserve(LTE_OnOff, gpio_module_id);
	ifx_gpio_altsel0_clear(LTE_OnOff, gpio_module_id);//GPIO mode
	ifx_gpio_altsel1_clear(LTE_OnOff, gpio_module_id);//GPIO mode
	ifx_gpio_open_drain_set(LTE_OnOff, gpio_module_id);//normal mode
	ifx_gpio_dir_out_set(LTE_OnOff, gpio_module_id);//output direction
	ifx_gpio_pudsel_set(LTE_OnOff, gpio_module_id);//pull up selected
	ifx_gpio_puden_set(LTE_OnOff, gpio_module_id);//pull up/down enable
	 printk(KERN_ERR"Pin value Now is High\n");
	ifx_gpio_output_set(LTE_OnOff, gpio_module_id);//default HIGH (initially OFF)
	//LTE_HW_SHD
//       ifx_gpio_pin_reserve(LTE_SHD, gpio_module_id);
//       ifx_gpio_altsel0_clear(LTE_SHD, gpio_module_id);
//        ifx_gpio_altsel1_clear(LTE_SHD, gpio_module_id);
//        ifx_gpio_open_drain_set(LTE_SHD, gpio_module_id);
//       ifx_gpio_dir_out_set(LTE_SHD, gpio_module_id);
//        ifx_gpio_pudsel_set(LTE_SHD, gpio_module_id);
//        ifx_gpio_puden_set(LTE_SHD, gpio_module_id);
//        ifx_gpio_output_clear(LTE_SHD, gpio_module_id);//defualt HIGH 
	//CODEC_RESET
        ifx_gpio_pin_reserve(CodecRst, gpio_module_id);
    	ifx_gpio_altsel0_clear(CodecRst, gpio_module_id);
     	ifx_gpio_altsel1_clear(CodecRst, gpio_module_id);
 	ifx_gpio_open_drain_clear(CodecRst, gpio_module_id);//open drain mode
    	ifx_gpio_dir_out_set(CodecRst, gpio_module_id);
	ifx_gpio_pudsel_set(CodecRst, gpio_module_id);
	ifx_gpio_puden_set(CodecRst, gpio_module_id);
	ifx_gpio_output_set(CodecRst, gpio_module_id);//default LOW (hold in reset state)
	

     	return 0;
}

unsigned char gpio_remove()
{
	ifx_gpio_deregister(gpio_module_id);
	return 0;
}

;


/******************************************************************************
 * Setting the desired value to GPIO pin
 * @args :	pin --> gpio pin number
 * 			value --> desired value to set
 * 			gpio_module_id --> corresponding module it belongs
 * @return: nil
 *****************************************************************************/
void GpioSetValue(unsigned char pin,unsigned char value,unsigned char gpio_module_id)
{
	if(RESET==value)
	{
		ifx_gpio_output_clear(pin, gpio_module_id);
	}
	else
	{
		ifx_gpio_output_set(pin, gpio_module_id);
	}
}






