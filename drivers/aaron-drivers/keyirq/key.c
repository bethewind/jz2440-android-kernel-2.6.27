/*
	aaron.yj
*/

#include <linux/device.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>
#include <linux/interrupt.h>
#include <linux/poll.h>



volatile unsigned long* gpfcon = NULL;
volatile unsigned long* gpfdat = NULL;
static struct class * key_dev_class;
#define GPF4_MASK (3 << (2 * 4))
#define GPF0_MASK (3 << (2 * 0))

#define GPF4_CON (1 << (2 * 4)) // out
#define GPF0_CON (0 << (2 * 0)) // in
static volatile int ev_press = 0;
static int key_val;

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static irqreturn_t button_handler(int data, void * dev_id) 
{
	printk("1111");
  key_val = s3c2410_gpio_getpin(S3C2410_GPF0);
  ev_press = 1;
  wake_up_interruptible(&button_waitq);
  printk("2222");

  return IRQ_RETVAL(IRQ_HANDLED);
}



int key_open (struct inode * inode, struct file * file)
{
    request_irq(IRQ_EINT0, button_handler, IRQF_TRIGGER_LOW , "s1", NULL);

	return 0;
}

ssize_t key_write (struct file * file, const char __user * buffer, size_t count, loff_t * fpos)
{
	int val;
	copy_from_user(&val,buffer,count);
	if (val == 1)
		*gpfdat |= (1 << 4);
	else
		*gpfdat &= ~(1 << 4);

	return count;
}

ssize_t key_read (struct file * file, char __user * buffer, size_t count, loff_t * fpos)
{
	printk("3333");
	wait_event_interruptible(button_waitq, ev_press);

	copy_to_user(buffer,&key_val,4);
	ev_press = 0;
	printk("4444");

	return 1;
}

int key_close (struct inode * inode, struct file * file) 
{
	free_irq(IRQ_EINT0,NULL);
	return 0;
}

unsigned int key_poll (struct file * file, struct poll_table_struct * wait)
{
	printk("5555");
	unsigned int mask = 0;
	poll_wait(file,&button_waitq,wait);
	if (ev_press)
		mask |= POLLIN | POLLRDNORM;
	printk("6666");

	return mask;
}




static struct file_operations key_operations = {
   .owner = THIS_MODULE,
   .open = key_open,
   .write = key_write,
   .read = key_read,
   .release = key_close,
   .poll = key_poll,
};

int major;
dev_t devt;
static int __init jz2440_key_init()
{
	major = register_chrdev(0,"key_drv",&key_operations);
	devt = MKDEV(major,0);
	key_dev_class = class_create(THIS_MODULE,"key_drv");
	device_create(key_dev_class,NULL,devt,NULL,"key0");
	gpfcon = (volatile unsigned long*)ioremap(0x56000050, 16);
	gpfdat = gpfcon + 1;

	return 0;
}
static void __exit jz2440_key_exit()
{
	unregister_chrdev(major, "key_drv");
	class_destroy(key_dev_class);
	device_destroy(key_dev_class,devt);
	iounmap(gpfcon);
}


module_init(jz2440_key_init);
module_exit(jz2440_key_exit);
MODULE_LICENSE("GPL");
