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

volatile unsigned long* gpfcon = NULL;
volatile unsigned long* gpfdat = NULL;
static struct class * key_dev_class;
#define GPF4_MASK (3 << (2 * 4))
#define GPF0_MASK (3 << (2 * 0))

#define GPF4_CON (1 << (2 * 4)) // out
#define GPF0_CON (0 << (2 * 0)) // in
int key_open (struct inode * inode, struct file * file)
{
    *gpfcon &= ~(GPF4_MASK | GPF0_MASK); //
    *gpfcon |= (GPF4_CON | GPF0_CON); //

	return 0;
}

ssize_t key_write (struct file * file, const char __user * buffer, size_t count, loff_t * fpos)
{
	printk("write b\n");
	int val;
	copy_from_user(&val,buffer,count);
	if (val == 1)
		*gpfdat |= (1 << 4);
	else
		*gpfdat &= ~(1 << 4);

	printk("write e\n");

	return count;
}

ssize_t key_read (struct file * file, char __user * buffer, size_t count, loff_t * fpos)
{
	printk("read b\n");
	unsigned long gpfDat = *gpfdat;
	int val;
	if (gpfDat & 0x00000001)
		val = 1;
	else
		val = 0;

	copy_to_user(buffer,&val,4);
	printk("read e\n");

	return 4;
}


static struct file_operations key_operations = {
   .owner = THIS_MODULE,
   .open = key_open,
   .write = key_write,
   .read = key_read,
};

int major;
dev_t devt;
static int __init jz2440_key_init()
{
	printk("init b\n");
	major = register_chrdev(0,"key_drv",&key_operations);
	devt = MKDEV(major,0);
	key_dev_class = class_create(THIS_MODULE,"key_drv");
	device_create(key_dev_class,NULL,devt,NULL,"key0");
	gpfcon = (volatile unsigned long*)ioremap(0x56000050, 16);
	gpfdat = gpfcon + 1;

	printk("init e\n");

	return 0;
}
static void __exit jz2440_key_exit()
{
	printk("exit b\n");
	unregister_chrdev(major, "key_drv");
	class_destroy(key_dev_class);
	device_destroy(key_dev_class,devt);
	iounmap(gpfcon);
	printk("exit e\n");
}


module_init(jz2440_key_init);
module_exit(jz2440_key_exit);
MODULE_LICENSE("GPL");
