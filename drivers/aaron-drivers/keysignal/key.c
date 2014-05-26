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
static struct fasync_struct * button_fasync;
static int can_open = 1;

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static DECLARE_MUTEX(button_lock);

static struct timer_list button_timer;


static irqreturn_t button_handler(int data, void * dev_id) 
{
	printk("1111\n");
  mod_timer(&button_timer, jiffies + HZ/100);
  printk("2222\n");

  return IRQ_RETVAL(IRQ_HANDLED);
}



int key_open (struct inode * inode, struct file * file)
{	 
/*
	if (!atomic_dec_and_test(&can_open)) {
		atomic_inc(&can_open);
		return -EBUSY;
	} */
	printk("3333\n");

    if (file->f_flags & O_NONBLOCK) {
		if (down_trylock(&button_lock))
			return -EBUSY;
    } else {
    	down(&button_lock);
    }
		
    request_irq(IRQ_EINT0, button_handler, IRQF_TRIGGER_LOW, "s1", NULL);
	printk("4444\n");

	return 0;
}

ssize_t key_write (struct file * file, const char __user * buffer, size_t count, loff_t * fpos)
{
	printk("write\n");
	int val;
	copy_from_user(&val,buffer,count);
	if (val == 1)
		*gpfdat &= ~(1 << 4);
	else
    *gpfdat |= (1 << 4);


	return count;
}

ssize_t key_read (struct file * file, char __user * buffer, size_t count, loff_t * fpos)
{
	printk("5555\n");
	wait_event_interruptible(button_waitq, ev_press);

	copy_to_user(buffer,&key_val,4);
	ev_press = 0;
	printk("6666\n");

	return 1;
}

int key_close (struct inode * inode, struct file * file) 
{
	free_irq(IRQ_EINT0,NULL);
	// atomic_inc(&can_open);
    up(&button_lock);
	
	return 0;
}

unsigned int key_poll (struct file * file, struct poll_table_struct * wait)
{
	unsigned int mask = 0;
	poll_wait(file,&button_waitq,wait);
	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

int key_fasync (int fd, struct file * file, int on)
{
	printk("key_fasync");
	fasync_helper(fd, file, on, &button_fasync);
}

void button_timer_function(unsigned long data)
{
	printk("7777\n");
  key_val = s3c2410_gpio_getpin(S3C2410_GPF0);
  ev_press = 1;
  wake_up_interruptible(&button_waitq);

  kill_fasync(&button_fasync,SIGIO, POLL_IN);
  printk("8888\n");
}



static struct file_operations key_operations = {
   .owner = THIS_MODULE,
   .open = key_open,
   .write = key_write,
   .read = key_read,
   .release = key_close,
   .poll = key_poll,
   .fasync = key_fasync,
};

int major;
dev_t devt;
static int __init jz2440_key_init()
{
	init_timer(&button_timer);
	button_timer.function = button_timer_function;
	add_timer(&button_timer);
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
	del_timer(&button_timer);
	unregister_chrdev(major, "key_drv");
	class_destroy(key_dev_class);
	device_destroy(key_dev_class,devt);
	iounmap(gpfcon);
}


module_init(jz2440_key_init);
module_exit(jz2440_key_exit);
MODULE_LICENSE("GPL");
