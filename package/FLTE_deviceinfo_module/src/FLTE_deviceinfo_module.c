#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/ioctl.h>

/**************************************************************
 *
 */
#define MAX_DEVICE_NUM_LEN  20
#define MAX_IMSI_LEN  20
#define MAX_IMEI_LEN  20

typedef struct
{
	unsigned char FLTEDeviceNo[MAX_DEVICE_NUM_LEN];				//data asociated with cmd
	unsigned char FLTEimsiNo[MAX_IMSI_LEN];						//Address to be written
	unsigned char FLTEimeiNo[MAX_IMEI_LEN];						//Data to be read
}LTEDeviceInfo_t;

typedef struct
{
	unsigned char SignalStrength;
	unsigned char SystemStatus;
}LTEDevicesignalstrength_t;;





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

static LTEDeviceInfo_t gLTEDeviceInfo;
LTEDevicesignalstrength_t LTESignalStrengthInfo;
int Major;
/*End of te IOCtl Commands*/
/******************************************************************************
 * Name : Device Create. Registering the Device
 * @desc: Every insmod will do the function call
 *****************************************************************************/
int __init init_mod(void)
{
	/*Registering the Device*/
	Major = register_chrdev(0,"LTEDeviceInfo",&fops);
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
    unregister_chrdev(Major,"LTEDeviceInfo");
    printk("Un-registered device\n");
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

void strcpy_func(char *dst,char *src)
{
	while (*dst++=*src++);
}





/*****************************************************************************/
static int device_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg)
{
	LTEDeviceInfo_t LTEDeviceInfo;
	LTEDevicesignalstrength_t DeviceSignalStrength;
	    switch (cmd)
	    {
	        case READ_DEVICE_SIGNAL_STRENGTH:
	        	DeviceSignalStrength.SignalStrength=LTESignalStrengthInfo.SignalStrength;
			//printk(KERN_INFO"Signal strength reading is  -%d\n",DeviceSignalStrength.SignalStrength);
	        	/*SCL Pin Configurations for the I2C*/
	        	if (copy_to_user((LTEDevicesignalstrength_t *)arg, &DeviceSignalStrength, sizeof(DeviceSignalStrength)))
	            {
	                return -EACCES;
	            }
	            break;
	        case READ_DEVICE_IMEI_NUMBER:
	            strcpy_func(LTEDeviceInfo.FLTEimeiNo,gLTEDeviceInfo.FLTEimeiNo);
	        	if (copy_to_user((LTEDeviceInfo_t *)arg, &LTEDeviceInfo, sizeof(LTEDeviceInfo_t)))
	            {
	                return -EACCES;
	            }
	            break;

	        case READ_DEVICE_IMSI_NUMBER:
	        	 printk(KERN_ERR"Imsi Number before copy read -%s\n",gLTEDeviceInfo.FLTEimsiNo);
	            strcpy_func(LTEDeviceInfo.FLTEimsiNo,gLTEDeviceInfo.FLTEimsiNo);
	        	 printk(KERN_ERR"Imsi Number after copy read -%s\n",LTEDeviceInfo.FLTEimsiNo);

	        	if (copy_to_user((LTEDeviceInfo_t *)arg, &LTEDeviceInfo, sizeof(LTEDeviceInfo_t)))
	            {
	                return -EACCES;
	            }
	            break;
	        case READ_DEVICE_NUMBER:
	            strcpy_func(LTEDeviceInfo.FLTEDeviceNo,gLTEDeviceInfo.FLTEDeviceNo);
	        	if (copy_to_user((LTEDeviceInfo_t *)arg, &LTEDeviceInfo, sizeof(LTEDeviceInfo_t)))
	            {
	                return -EACCES;
	            }
	        	break;
	        case WRITE_DEVICE_SIGNAL_STRENGTH:
	        	//Give to the User Space
	        	if (copy_from_user(&DeviceSignalStrength, (LTEDevicesignalstrength_t *)arg, sizeof(LTEDevicesignalstrength_t)))
	        	{
	        		 return -EACCES;
	        	}
			//printk(KERN_INFO"Signal strength writing is  -%d\n",DeviceSignalStrength.SignalStrength);
	        	LTESignalStrengthInfo.SignalStrength=DeviceSignalStrength.SignalStrength;
	            break;
	        case WRITE_DEVICE_IMEI_NUMBER:
	        	//Take the data from the user space
	            if (copy_from_user(&LTEDeviceInfo, (LTEDeviceInfo_t *)arg, sizeof(LTEDeviceInfo_t)))
	            {
	                return -EACCES;
	            }
	            /*Copying String*/
	            strcpy_func(gLTEDeviceInfo.FLTEimeiNo,LTEDeviceInfo.FLTEimeiNo);
	            break;

	        case WRITE_DEVICE_IMSI_NUMBER:
	            if (copy_from_user(&LTEDeviceInfo, (LTEDeviceInfo_t *)arg, sizeof(LTEDeviceInfo_t)))
	            {
	                return -EACCES;
	            }
	            printk(KERN_ERR"Imsi Number received -%s\n",LTEDeviceInfo.FLTEimsiNo);
	            /*Copying String*/
	            strcpy_func(gLTEDeviceInfo.FLTEimsiNo,LTEDeviceInfo.FLTEimsiNo);
	            printk(KERN_ERR"Imsi Number copied -%s\n",gLTEDeviceInfo.FLTEimsiNo);

	            break;
	        case WRITE_DEVICE_NUMBER:
	            if (copy_from_user(&LTEDeviceInfo, (LTEDeviceInfo_t *)arg, sizeof(LTEDeviceInfo_t)))
	            {
	                return -EACCES;
	            }
	            /*Copying String*/
	            strcpy_func(gLTEDeviceInfo.FLTEDeviceNo,LTEDeviceInfo.FLTEDeviceNo);
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
	if(buf[0]=='1')
	{
		printk("Writing 1\n");
	}
	else
	{
		printk("writing 0 \n");
	}
        return len;
}

/*Exporting the Init And Exit Functio Macros*/
module_init(init_mod);
module_exit(cleanup_mod);

MODULE_LICENSE("GPL");


