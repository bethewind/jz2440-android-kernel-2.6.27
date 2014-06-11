/* 
 * aaron.yj test
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/div64.h>

#include <asm/mach/map.h>
#include <mach/regs-lcd.h>
#include <mach/regs-gpio.h>
#include <mach/fb.h>

struct lcd_regs {
	unsigned long	lcdcon1;
	unsigned long	lcdcon2;
	unsigned long	lcdcon3;
	unsigned long	lcdcon4;
	unsigned long	lcdcon5;
    unsigned long	lcdsaddr1;
    unsigned long	lcdsaddr2;
    unsigned long	lcdsaddr3;
    unsigned long	redlut;
    unsigned long	greenlut;
    unsigned long	bluelut;
    unsigned long	reserved[9];
    unsigned long	dithmode;
    unsigned long	tpal;
    unsigned long	lcdintpnd;
    unsigned long	lcdsrcpnd;
    unsigned long	lcdintmsk;
    unsigned long	lpcsel;
};


static struct fb_info* fbinfo;
static char driver_name[] = "s3c2410fb";
static u32 pseudo_pal[16];
static struct resource * io_res;
static void __iomem		*io_base;
static volatile unsigned long *gpbcon;
static volatile unsigned long *gpbdat;
static volatile unsigned long *gpccon;
static volatile unsigned long *gpdcon;
static volatile unsigned long *gpgcon;
static volatile struct lcd_regs* lcd_regs;


static struct fb_ops s3c_lcdfb_ops = {
	.owner		= THIS_MODULE,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static inline void modify_gpio(void __iomem *reg,
			       unsigned long set, unsigned long mask)
{
	unsigned long tmp;

	tmp = readl(reg) & ~mask;
	writel(tmp | set, reg);
}

//open s3c2410 LCD
static void  LCD_POWER_ON() 
{ 
        writel(readl(S3C2410_GPGUP)  | (1<<4) ,  S3C2410_GPGUP); 
        writel(readl(S3C2410_GPGCON) | (3<<8),   S3C2410_GPGCON); 
} 



static int s3c2440fb_probe(struct platform_device * pdev)
{
	struct s3c2410fb_mach_info * mach_info;
	unsigned long smem_long;
	struct resource * res;
	int size;
	u32 lcdcon1;
	void __iomem *tpal;

	res = platform_get_resource(pdev,IORESOURCE_MEM,0);
	if (res == NULL) {
		printk(KERN_ERR, "-----s3c2440fb_probe failed to platform_get_resource");
		return -ENXIO;
	}
	size = res->end - res->start + 1;
	
	io_res = request_mem_region(res->start,size,pdev->name);
	if (io_res == NULL) {
		printk(KERN_ERR, "-----s3c2440fb_probe failed to request_mem_region");
		return -ENOENT;
	}
/*
	io_base = ioremap(res->start,size);
	if (io_base == NULL) {
		printk(KERN_ERR, "-----s3c2440fb_probe failed to ioremap");
		return -ENXIO;
	}

	tpal = io_base + S3C2410_TPAL;
	writel(0x00, tpal); 

	lcdcon1 = readl(io_base + S3C2410_LCDCON1);
	writel(lcdcon1 & ~S3C2410_LCDCON1_ENVID, io_base + S3C2410_LCDCON1);*/
	
	mach_info = pdev->dev.platform_data;
	fbinfo = framebuffer_alloc(0, &pdev->dev);
	if (!fbinfo)
		return -ENOMEM;

	strcpy(fbinfo->fix.id, driver_name);

	/* byte sme_len*/
	fbinfo->fix.smem_len = (mach_info->displays->xres * mach_info->displays->yres * mach_info->displays->bpp)/8;
	fbinfo->screen_base = dma_alloc_writecombine(&pdev->dev,fbinfo->fix.smem_len,&fbinfo->fix.smem_start,GFP_KERNEL);
	if (fbinfo->screen_base)
		memset(fbinfo->screen_base,0,fbinfo->fix.smem_len);
	else {
		printk(KERN_ERR, "failed to alloc video RAM");
		return -ENOMEM;
	}
	fbinfo->screen_size = fbinfo->fix.smem_len;
	fbinfo->fbops		    = &s3c_lcdfb_ops;
	fbinfo->flags		    = FBINFO_FLAG_DEFAULT;
	fbinfo->pseudo_palette      = &pseudo_pal;
	
	fbinfo->fix.type = FB_TYPE_PACKED_PIXELS;
    fbinfo->fix.type_aux	    = 0;
	fbinfo->fix.xpanstep	    = 0;
	fbinfo->fix.ypanstep	    = 0;
	fbinfo->fix.ywrapstep	    = 0;
	fbinfo->fix.accel	    = FB_ACCEL_NONE;
	fbinfo->fix.visual = FB_VISUAL_TRUECOLOR;
	fbinfo->fix.line_length = (mach_info->displays->xres * mach_info->displays->bpp)/8;
		
		
	
	fbinfo->var.xres = mach_info->displays->xres;
	fbinfo->var.yres = mach_info->displays->yres;
	fbinfo->var.xres_virtual = mach_info->displays->xres;
	fbinfo->var.yres_virtual = mach_info->displays->yres;
	fbinfo->var.bits_per_pixel = mach_info->displays->bpp;
	/* 16 bpp, 565 format */
	fbinfo->var.red.offset		= 11;
	fbinfo->var.green.offset	= 5;
	fbinfo->var.blue.offset	= 0;
	fbinfo->var.red.length		= 5;
	fbinfo->var.green.length	= 6;
	fbinfo->var.blue.length	= 5;
	fbinfo->var.transp.offset = 0;
	fbinfo->var.transp.length = 0;
	fbinfo->var.width= mach_info->displays->width;
	fbinfo->var.height= mach_info->displays->height;
    fbinfo->var.pixclock = mach_info->displays->pixclock;
	fbinfo->var.left_margin = mach_info->displays->left_margin;
	fbinfo->var.right_margin = mach_info->displays->right_margin;
	fbinfo->var.upper_margin = mach_info->displays->upper_margin;
	fbinfo->var.lower_margin = mach_info->displays->lower_margin;
	fbinfo->var.vsync_len = mach_info->displays->vsync_len;
	fbinfo->var.hsync_len = mach_info->displays->hsync_len;
	fbinfo->var.nonstd	    = 0;
	fbinfo->var.activate	    = FB_ACTIVATE_NOW;
	fbinfo->var.accel_flags     = 0;
	fbinfo->var.vmode	    = FB_VMODE_NONINTERLACED;

	modify_gpio(S3C2410_GPCUP,  mach_info->gpcup,  mach_info->gpcup_mask);
	modify_gpio(S3C2410_GPCCON, mach_info->gpccon, mach_info->gpccon_mask);
	modify_gpio(S3C2410_GPDUP,  mach_info->gpdup,  mach_info->gpdup_mask);
	modify_gpio(S3C2410_GPDCON, mach_info->gpdcon, mach_info->gpdcon_mask);

	LCD_POWER_ON();


	s3c2410_gpio_cfgpin(S3C2410_GPB0, S3C2410_GPB0_OUTP); // back light control

	s3c2410_gpio_pullup(S3C2410_GPB0, 0); 

	s3c2410_gpio_setpin(S3C2410_GPB0, 1);	// back light control, enable
	msleep(10);	

	
	lcd_regs = ioremap(0x4D000000, sizeof(struct lcd_regs));

	lcd_regs->lcdcon1  = (4<<8) | (3<<5) | (0x0c<<1);

	lcd_regs->lcdcon2  = (1<<24) | (271<<14) | (1<<6) | (9);

	lcd_regs->lcdcon3 = (1<<19) | (479<<8) | (1);

	lcd_regs->lcdcon4 = 40;

	lcd_regs->lcdcon5 = (1<<11) | (0<<10) | (1<<9) | (1<<8) | (1<<0);
	
	
	lcd_regs->lcdsaddr1  = (fbinfo->fix.smem_start >> 1) & ~(3<<30);
	lcd_regs->lcdsaddr2  = ((fbinfo->fix.smem_start + fbinfo->fix.smem_len) >> 1) & 0x1fffff;
	lcd_regs->lcdsaddr3  = (480*16/16);
	
	lcd_regs->lcdcon1 |= (1<<0); 
	lcd_regs->lcdcon5 |= (1<<3);
	
	s3c2410_gpio_setpin(S3C2410_GPB0, 1);	// back light control

	register_framebuffer(fbinfo);
	
	return 0;

}

static void s3c2440fb_remove(struct platform_device * pdev)
{
	dma_free_writecombine(NULL, fbinfo->fix.smem_len, fbinfo->screen_base, fbinfo->fix.smem_start);
	unregister_framebuffer(fbinfo);
	framebuffer_release(fbinfo);
	lcd_regs->lcdcon1 &= ~(1<<0);
	s3c2410_gpio_setpin(S3C2410_GPB0, 0);	// back light control
	iounmap(lcd_regs);
	// release_resource(io_res);
}



static struct platform_driver s3c2440fb_platform_driver = {
  .probe = s3c2440fb_probe,
  	.remove		= s3c2440fb_remove,
  	.driver		= {
		.name	= "s3c2410-lcd",
		.owner	= THIS_MODULE,
	},
};

static __init int s3c2440fb_init()
{
	platform_driver_register(&s3c2440fb_platform_driver);

	return 0;
}

static __exit void s3c2440fb_exit()
{
	platform_driver_unregister(&s3c2440fb_platform_driver);

}

module_init(s3c2440fb_init);
module_exit(s3c2440fb_exit);
MODULE_AUTHOR("aaron.yj@qq.com");
MODULE_LICENSE("GPL");





















