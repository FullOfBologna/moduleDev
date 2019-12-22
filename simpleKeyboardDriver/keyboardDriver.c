#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h> 	/* for put_user */

/*
 *	Prototypes 
 */

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "kbcount"
#define MSG_LEN 80
#define BUF_LEN 8

/*
 *	Global variables are declared as static, so are global within this file
 */

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0; 	/*Is Device open? Used to prevent multiple access
				 too device. */

static char msg[MSG_LEN];	/* The message the device will give when opened or closed. */
static char cnt[BUF_LEN];	/* The message the device will give when read. */
static char *msg_Ptr;
static char *cnt_Ptr;

static int keyboard_count = 0;	/* Tracks how many keys have been press on the keyboard. */

static struct file_operations fops = 
{
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static void convertToCharBuf(void)
{
	int byteCount = 0;

	for(byteCount = 0; byteCount < BUF_LEN; byteCount++)
	{
		cnt[byteCount] = (keyboard_count >> byteCount*8) & 0xFF;
	}		
}

irq_handler_t irq_handler(int irq, void* dev_id, struct pt_regs *regs,int* numberOfKeyPresses)
{
	static unsigned char scancode;	
	/*
	 * Read keyboard status
	 */
	scancode = inb(0x60);

	keyboard_count = keyboard_count + 1;

	convertToCharBuf();

	cnt_Ptr = cnt;

	return (irq_handler_t) IRQ_HANDLED;
}

static int __init keyboard_counter(void)
{
	int result;
	
	/*
	 *	Register device on loading of module. 
	 */

	Major = register_chrdev(0,DEVICE_NAME,&fops);

	if(Major < 0)
	{
		printk(KERN_ALERT "Registering keyboard device failed with %d\n", Major);
		return Major;
	}

	/*
	 *	Register this module with Keyboard IRQ. 
	 */

	result = request_irq(1, (irq_handler_t) irq_handler, IRQF_SHARED, "keyboard_stats_irq", (void*)(irq_handler));

	if(result)
	{
		printk(KERN_INFO "Unable to register shared interrupt");
		return result;
	}
		
	printk(KERN_INFO "Keyboard counter was assigned to major number %d. To talk to\n",Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to \n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");

	return SUCCESS;
}

/*
 *Remove the interrupt handler and unregister device. 
 */

static void __exit keyboard_counter_exit(void)
{
	free_irq(1,(void*)irq_handler); //Only free this irq_handler from irq 1

	/*
	 *	Unregister device
	 */

	unregister_chrdev(Major, DEVICE_NAME);

}

/*
 * Methods for device file operations
 */

/*
 * Called when a process closes the device file.
 */

static int device_open(struct inode *inode, struct file *file)
{

	if(Device_Open)
		return -EBUSY;

	Device_Open++;

	sprintf(msg, "Keyboard counter device opened.\n");

	msg_Ptr = msg;
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/*
 *	Called when a process closes the device file.
 */

static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;	/*We're now ready for our next caller*/

	/*
	 *	Decrement the usage count, or else once you opened the file, you'll
	 *	never get rid of the module. 
	 */

	module_put(THIS_MODULE);

	return 0;
}

/*
 *	Called when a process, which already opened the dev file, attempts to read from it. 
 */

static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer	    */
			   loff_t * offset)
{
	/*
	 * Number of bytes actually written to the buffer
	 */
	int bytes_read = 0;

	/*
	 *	If we're at the end of the message,
	 *	return 0 signifying end of the file
	 */

	if(*cnt_Ptr == 0)
		return 0;

	/*
	 *	Fill data buffer for the user
	 */

	while(length && *cnt_Ptr)
	{
		/*
		 * The buffer is in the user data segment, not the kernel
		 * segment so "*" assignment won't work. We have to use put_user which copies
		 * data from the kernel data segment to the user data segment. 
		 */

		put_user(*(cnt_Ptr++),buffer++);
	        length--;
		bytes_read++;	
	}

	/*
	 * Most read functions return the number of bytes put into the buffer.
	 */
	return bytes_read;
}

/*
 * Called when a process writes to dev file: echo "hi > /dev/hello
 */

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	printk(KERN_ALERT "sorry, this operation is not currently supported.\n");
	return -EINVAL;
}

MODULE_LICENSE("GPL");
module_init(keyboard_counter);
module_exit(keyboard_counter_exit);
