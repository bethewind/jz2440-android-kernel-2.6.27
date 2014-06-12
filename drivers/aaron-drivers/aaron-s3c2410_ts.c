/* 
 * aaron.yj test touchscreen
*/

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm-arm/plat-s3c/regs-adc.h>
#include <mach/regs-gpio.h>

#include <linux/miscdevice.h>
#include <asm/uaccess.h> 
#define S3C2410TSVERSION       0x0101

#define WAIT4INT(x)  (((x)<<8) | \
		     S3C2410_ADCTSC_YM_SEN | S3C2410_ADCTSC_YP_SEN | S3C2410_ADCTSC_XP_SEN | \
		     S3C2410_ADCTSC_XY_PST(3))

#define AUTOPST	     (S3C2410_ADCTSC_YM_SEN | S3C2410_ADCTSC_YP_SEN | S3C2410_ADCTSC_XP_SEN | \
		     S3C2410_ADCTSC_AUTO_PST | S3C2410_ADCTSC_XY_PST(0))



static struct input_dev * dev;
struct clk * adc_clk = NULL;
static void __iomem *base_addr;
static char *s3c2410ts_name = "s3c2410 TouchScreen";
static int count = 0;
static int x = 0,y = 0;
struct s3c_ts_regs {
	unsigned long adccon;
	unsigned long adctsc;
	unsigned long adcdly;
	unsigned long adcdat0;
	unsigned long adcdat1;
	unsigned long adcupdn;
};

static volatile struct s3c_ts_regs *s3c_ts_regs;



static void touch_timer_fire1(void)
{
	int down;
	unsigned int data0,data1;

	printk("---------touch_timer_fire\n");

	data0 = ioread32(base_addr + S3C2410_ADCDAT0);
	data1 = ioread32(base_addr + S3C2410_ADCDAT1);
	down = !(data0 & S3C2410_ADCDAT0_UPDOWN) && !(data0 & S3C2410_ADCDAT0_UPDOWN);

	if (down) {
		if (count != 0) {
			x = x/4;
			y = y/4;
			printk("--------------------------------X:%d,Y:%d", x, y);
			input_report_abs(dev,ABS_X,x);
			input_report_abs(dev,ABS_Y,y);
			input_report_abs(dev,ABS_PRESSURE,1);
			input_report_key(dev,BTN_TOUCH,1);
			input_sync(dev);
		}

		count = 0;
		iowrite32(S3C2410_ADCTSC_PULL_UP_DISABLE | AUTOPST, base_addr+S3C2410_ADCTSC);
 		iowrite32(ioread32(base_addr+S3C2410_ADCCON) | S3C2410_ADCCON_ENABLE_START, base_addr+S3C2410_ADCCON);
		
	} else {
			input_report_abs(dev,ABS_PRESSURE,0);
			input_report_key(dev,BTN_TOUCH,0);
			input_sync(dev);
			iowrite32(WAIT4INT(0), base_addr+S3C2410_ADCTSC);
	}
}

static struct timer_list touch_timer =
		TIMER_INITIALIZER(touch_timer_fire1, 0, 0);


irqreturn_t updown_irq(int irq, void * dev_id)
{
	int down;
	unsigned int data0,data1;

	printk("---------updown_irq\n");

	data0 = ioread32(base_addr + S3C2410_ADCDAT0);
	data1 = ioread32(base_addr + S3C2410_ADCDAT1);
	down = !(data0 & S3C2410_ADCDAT0_UPDOWN) && !(data0 & S3C2410_ADCDAT0_UPDOWN);

	if (down)
		touch_timer_fire1();
	
	
	return IRQ_HANDLED;
}

irqreturn_t adc_irq(int irq, void * dev_id)
{
	unsigned int data0,data1;

	printk("---------adc_irq\n");

	data0 = ioread32(base_addr + S3C2410_ADCDAT0);
	data1 = ioread32(base_addr + S3C2410_ADCDAT1);

	x += data0 & 0x3ff;
	y += data1 & 0x3ff;
	count++;
	if (count < 4) {
		iowrite32(S3C2410_ADCTSC_PULL_UP_DISABLE | AUTOPST, base_addr+S3C2410_ADCTSC);
 		iowrite32(ioread32(base_addr+S3C2410_ADCCON) | S3C2410_ADCCON_ENABLE_START, base_addr+S3C2410_ADCCON);
	} else {
		mod_timer(&touch_timer, jiffies + 1);
		iowrite32(WAIT4INT(1), base_addr+S3C2410_ADCTSC);
	}

	
	return IRQ_HANDLED;
}


static int __init s3c2410ts_init(void)
{
	printk("---------s3c2410ts_init\n");
	dev = input_allocate_device();
	if (!dev) {
		printk("---------s3c2410 could not allocate device\n");
		return -ENOMEM;
	}
	set_bit(EV_KEY,dev->evbit);
	set_bit(EV_ABS,dev->evbit);

	set_bit(BTN_TOUCH,dev->keybit);

	input_set_abs_params(dev, ABS_X, 0, 0x3FF, 0, 0);
	input_set_abs_params(dev, ABS_Y, 0, 0x3FF, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 1, 0, 0);

	dev->name = s3c2410ts_name;
	dev->phys = "ts0";
	dev->id.bustype = BUS_RS232;
	dev->id.vendor = 0xDEAD;
	dev->id.product = 0xBEEF;
	 dev->id.version = S3C2410TSVERSION;
	
	dev->uniq = "jz2440";

	
	adc_clk = clk_get(NULL,"adc");
	if (!adc_clk) {
		printk("---------clk_get failed\n");
		return -ENOENT;;
	}
	clk_enable(adc_clk);

	base_addr = ioremap(S3C2410_PA_ADC,0x20);
	if (base_addr == NULL) {
		printk("---------ioremap failed\n");
		return  -ENOMEM;
	}

	iowrite32(S3C2410_ADCCON_PRSCEN | S3C2410_ADCCON_PRSCVL(0xFF),\
		     base_addr+S3C2410_ADCCON);
	iowrite32(0xffff,  base_addr+S3C2410_ADCDLY);
	iowrite32(WAIT4INT(0), base_addr+S3C2410_ADCTSC);

	if (request_irq(IRQ_ADC,adc_irq,IRQF_SHARED|IRQF_SAMPLE_RANDOM,"s3c2410 action",dev)) {
		printk("-------request irq failed\n");
	return -EIO;
	}
	if (request_irq(IRQ_TC,updown_irq,IRQF_SAMPLE_RANDOM,"s3c2410 action",dev)) {
		printk("-------request irq failed\n");
		return -EIO;
	}

	input_register_device(dev);

	return 0;
}

static void __exit s3c2410ts_exit(void)
{
	printk("---------s3c2410ts_exit\n");
	if (adc_clk) {
		clk_disable(adc_clk);
		clk_put(adc_clk);
		adc_clk = NULL;
	}
	free_irq(IRQ_TC, dev);
	free_irq(IRQ_ADC, dev);
	iounmap(base_addr);
	
	input_unregister_device(dev);
}

module_init(s3c2410ts_init);
module_exit(s3c2410ts_exit);
MODULE_LICENSE("GPL");



