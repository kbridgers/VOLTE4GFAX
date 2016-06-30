
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/ifx/ifx_gpio.h>

/*******************************************************/
/**I2Cgpiodriver.h***/
/*Corresponding GPIO pins you wan to configure*/
#define GPIO5		11// 5  //SYS_STATUS_LED1 
#define GPIO12      12 //SYS_STATUS_LED2
#define GPIO3       3  //LE910_LED1
#define GPIO2       2  //LE910_LED3
#define GPIO1       1  //LE910_LED2
#define led_module_id    IFX_GPIO_MODULE_I2C

#define sysL1 GPIO5
#define sysL2 GPIO12
#define sigLhigh GPIO3
#define sigLmed GPIO1
#define sigLlow GPIO2

typedef enum {RESET,SET}eStatus_t;

typedef enum {INPUT,OUTPUT}eDirection_t;
typedef enum {LOW,HIGH}eState_t;

unsigned char led_init(void);
unsigned char led_remove(void);
void GpioSetValue(unsigned char pin,unsigned char value,unsigned char led_module_id);
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
	if(0==led_init())			/*Calling the I2C Gpio initialisation*/
	{
		printk( KERN_ERR "led Init Succesfull \n");
	}
	else
	{
		printk( KERN_ERR "led module Init Failed\n");
		return -1;
	}
	/*Registering the Device*/
	Major = register_chrdev(0,"led",&fops);
	if(Major < 0)
			printk("Registering device unsuccesfull\n");
	else
			printk("Registered device and major number is %d\n",Major);
	/*Return Success 0*/
	return 0;
}


/******************************************************************************
 * Name : Device Exit Exit From the Device
 * @desc: Every rmmod will do the function call
 *****************************************************************************/
static void __exit cleanup_mod(void)
{
    unregister_chrdev(201,"led");
    led_remove();
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
	char led_buff[30];
        if(copy_from_user(led_buff,buf,len)<0)
           return -1;
	led_buff[len-1]='\0';
	if(strcmpr(led_buff,"sysL1on"))
	{
		GpioSetValue(sysL1,SET,IFX_GPIO_MODULE_I2C);
	}
	else if(strcmpr(led_buff,"sysL1off"))
	{
		GpioSetValue(sysL1,RESET,IFX_GPIO_MODULE_I2C);
	}
	else if(strcmpr(led_buff,"sysL2on"))
        {
                GpioSetValue(sysL2,SET,IFX_GPIO_MODULE_I2C);
        }
	else if(strcmpr(led_buff,"sysL2off"))
        {
                GpioSetValue(sysL2,RESET,IFX_GPIO_MODULE_I2C);
        }
	else if(strcmpr(led_buff,"sigLhighon"))
        {
                GpioSetValue(sigLhigh,RESET,IFX_GPIO_MODULE_I2C);
        }
	else if(strcmpr(led_buff,"sigLhighoff"))
        {
                GpioSetValue(sigLhigh,SET,IFX_GPIO_MODULE_I2C);
        }
	else if(strcmpr(led_buff,"sigLmedon"))
        {
                GpioSetValue(sigLmed,RESET,IFX_GPIO_MODULE_I2C);
        }
	else if(strcmpr(led_buff,"sigLmedoff"))
        {
                GpioSetValue(sigLmed,SET,IFX_GPIO_MODULE_I2C);
        }
	else if(strcmpr(led_buff,"sigLlowon"))
        {
                GpioSetValue(sigLlow,RESET,IFX_GPIO_MODULE_I2C);
        }
        else if(strcmpr(led_buff,"sigLlowoff"))
        {
                GpioSetValue(sigLlow,SET,IFX_GPIO_MODULE_I2C);
        }
	else
	{
		printk(KERN_ERR"LED Driver: value not supported\n");
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
unsigned char led_init()
{
	ifx_gpio_register(led_module_id);
	/*SYS_STATUS_LED1*/
	ifx_gpio_pin_reserve(GPIO5, led_module_id);
	ifx_gpio_altsel0_clear(GPIO5, led_module_id);//GPIO mode
	ifx_gpio_altsel1_clear(GPIO5, led_module_id);//GPIO mode
	ifx_gpio_open_drain_set(GPIO5, led_module_id);//normal mode
	ifx_gpio_dir_out_set(GPIO5, led_module_id);//output direction
	ifx_gpio_pudsel_set(GPIO5, led_module_id);//pull up selected
	ifx_gpio_puden_set(GPIO5, led_module_id);//pull up/down enable
	ifx_gpio_output_clear(GPIO5, led_module_id);//default off
	/*SYS_STATUS_LED2*/
        ifx_gpio_pin_reserve(GPIO12, led_module_id);
        ifx_gpio_altsel0_clear(GPIO12, led_module_id);
        ifx_gpio_altsel1_clear(GPIO12, led_module_id);
        ifx_gpio_open_drain_set(GPIO12, led_module_id);
        ifx_gpio_dir_out_set(GPIO12, led_module_id);
        ifx_gpio_pudsel_set(GPIO12, led_module_id);
        ifx_gpio_puden_set(GPIO12, led_module_id);
        ifx_gpio_output_set(GPIO12, led_module_id);
	/*LE910_LED2*/
        ifx_gpio_pin_reserve(GPIO1, led_module_id);
    	ifx_gpio_altsel0_clear(GPIO1, led_module_id);
     	ifx_gpio_altsel1_clear(GPIO1, led_module_id);
 	ifx_gpio_open_drain_clear(GPIO1, led_module_id);//open drain mode
    	ifx_gpio_dir_out_set(GPIO1, led_module_id);
	ifx_gpio_pudsel_set(GPIO1, led_module_id);
	ifx_gpio_puden_set(GPIO1, led_module_id);
	ifx_gpio_output_set(GPIO1, led_module_id);//default off
	/*LE910_LED3*/
   	ifx_gpio_pin_reserve(GPIO2, led_module_id);
    	ifx_gpio_altsel0_clear(GPIO2, led_module_id);
    	ifx_gpio_altsel1_clear(GPIO2, led_module_id);
 	ifx_gpio_open_drain_clear(GPIO2, led_module_id);//open drain mode
    	ifx_gpio_dir_out_set(GPIO2, led_module_id);//output direction
	ifx_gpio_pudsel_set(GPIO2, led_module_id);//pull up selected
	ifx_gpio_puden_set(GPIO2, led_module_id);//pull up enabled
	ifx_gpio_output_set(GPIO2, led_module_id);//default off
	/*LE910_LED1*/
    	ifx_gpio_pin_reserve(GPIO3, led_module_id);
   	ifx_gpio_altsel0_clear(GPIO3, led_module_id);//GPIO mode
    	ifx_gpio_altsel1_clear(GPIO3, led_module_id);//GPIO mode
 	ifx_gpio_open_drain_clear(GPIO3, led_module_id);//open drain mode
    	ifx_gpio_dir_out_set(GPIO3, led_module_id);//output direction
	ifx_gpio_pudsel_set(GPIO3, led_module_id);//pull up selected
	ifx_gpio_puden_set(GPIO3, led_module_id);//pull up enabled
	ifx_gpio_output_set(GPIO3, led_module_id);//default off

     	return 0;
}

unsigned char led_remove()
{
	ifx_gpio_deregister(led_module_id);
	return 0;
}

;


/******************************************************************************
 * Setting the desired value to GPIO pin
 * @args :	pin --> gpio pin number
 * 			value --> desired value to set
 * 			led_module_id --> corresponding module it belongs
 * @return: nil
 *****************************************************************************/
void GpioSetValue(unsigned char pin,unsigned char value,unsigned char led_module_id)
{
	if(RESET==value)
	{
		ifx_gpio_output_clear(pin, led_module_id);
	}
	else
	{
		ifx_gpio_output_set(pin, led_module_id);
	}
}






