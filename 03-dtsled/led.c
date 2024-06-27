#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

struct dtsled_dev
{
    dev_t devid;            // 设备号
    int major;              // 主设备号
    int minor;              // 次设备号
    struct cdev cdev;       // 字符设备
    struct class *class;    // 设备类
    struct device *device;  // 设备实例
    struct device_node *nd; // 设备数中的节点
};

struct dtsled_dev dtsled;

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

#define LEDOFF 0 /* 关灯 */
#define LEDON 1  /* 开灯 */

void led_switch(u8 sta);
void led_init(void);
static int led_open(struct inode *inode, struct file *file);
static int led_release(struct inode *inode, struct file *file);
static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static int led_module_init(void);
static void led_module_exit(void);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .read = led_read,
    .write = led_write,
};

void led_switch(u8 sta)
{
    uint32_t val = 0;
    if (sta == LEDON)
    {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_DR);
    }
    else if (sta == LEDOFF)
    {
        val = readl(GPIO1_DR);
        val |= (1 << 3);
        writel(val, GPIO1_DR);
    }
}

static int led_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "LED opened\n");
    file->private_data = &dtsled;
    return 0;
}

static int led_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "LED closed\n");
    return 0;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    if (buf == NULL)
    {
        return -EINVAL;
    }

    int ret = 0;
    uint8_t databuf[1];
    ret = __copy_from_user(databuf, buf, count);
    if (ret < 0)
    {
        printk(KERN_INFO "write LED status failed! \n");
        return EFAULT;
    }

    if (databuf[0] == LEDON)
    {
        led_switch(LEDON);
    }
    else
    {
        led_switch(LEDOFF);
    }
}

// 初始化LED
void led_init(void)
{
    u32 val = 0;
    int ret;
    u32 regdata[14];
    const char *str;
    struct property *proper;

    /* 获取设备树中的属性数据 */
    /* 1、获取设备节点：alphaled */
    dtsled.nd = of_find_node_by_path("/alphaled");
    if (dtsled.nd == NULL)
    {
        printk("alphaled node nost find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("alphaled node find!\r\n");
    }

    /* 2、获取compatible属性内容 */
    proper = of_find_property(dtsled.nd, "compatible", NULL);
    if (proper == NULL)
    {
        printk("compatible property find failed\r\n");
    }
    else
    {
        printk("compatible = %s\r\n", (char *)proper->value);
    }

    /* 3、获取status属性内容 */
    ret = of_property_read_string(dtsled.nd, "status", &str);
    if (ret < 0)
    {
        printk("status read failed!\r\n");
    }
    else
    {
        printk("status = %s\r\n", str);
    }

    /* 4、获取reg属性内容 */
    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
    if (ret < 0)
    {
        printk("reg property read failed!\r\n");
    }
    else
    {
        u8 i = 0;
        printk("reg data:\r\n");
        for (i = 0; i < 10; i++)
            printk("%#X ", regdata[i]);
        printk("\r\n");
    }

    IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0);
    SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1);
    SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2);
    GPIO1_DR = of_iomap(dtsled.nd, 3);
    GPIO1_GDIR = of_iomap(dtsled.nd, 4);

    /* 2、使能GPIO1时钟 */
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26); /* 清楚以前的设置 */
    val |= (3 << 26);  /* 设置新值 */
    writel(val, IMX6U_CCM_CCGR1);

    /* 3、设置GPIO1_IO03的复用功能，将其复用为
     *    GPIO1_IO03，最后设置IO属性。
     */
    writel(5, SW_MUX_GPIO1_IO03);

    /*寄存器SW_PAD_GPIO1_IO03设置IO属性*/
    writel(0x10B0, SW_PAD_GPIO1_IO03);

    /* 4、设置GPIO1_IO03为输出功能 */
    val = readl(GPIO1_GDIR);
    val &= ~(1 << 3); /* 清除以前的设置 */
    val |= (1 << 3);  /* 设置为输出 */
    writel(val, GPIO1_GDIR);

    /* 5、默认关闭LED */
    val = readl(GPIO1_DR);
    val |= (1 << 3);
    writel(val, GPIO1_DR);
}

static int led_module_init(void)
{
    int result;
    //* 通过寄存器从硬件上初始化LED
    led_init();

    //* 动态分配设备号
    if (dtsled.major)
    {
        dtsled.devid = MKDEV(dtsled.major, 0);
        register_chrdev_region(dtsled.devid, 1, "dts_led");
    }
    else
    {                                                        /* 没有定义设备号 */
        alloc_chrdev_region(&dtsled.devid, 0, 1, "dts_led"); /* 申请设备号 */
        dtsled.major = MAJOR(dtsled.devid);                  /* 获取分配号的主设备号 */
        dtsled.minor = MINOR(dtsled.devid);                  /* 获取分配号的次设备号 */
    }
    printk("dtsled major=%d,minor=%d\r\n", dtsled.major, dtsled.minor);

    //* 注册此类字符设备
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &fops);
    result = cdev_add(&dtsled.cdev, dtsled.devid, 1);
    if (result)
    {
        printk(KERN_NOTICE "Error %d adding dts_led", result);
        unregister_chrdev_region(dtsled.devid, 1);
        return result;
    }

    //* 创建类
    dtsled.class = class_create(THIS_MODULE, "dts_led");
    if (IS_ERR(dtsled.class))
    {
        return PTR_ERR(dtsled.class);
    }

    //* 创建字符设备实例
    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, "dts_led");
    if (IS_ERR(dtsled.device))
    {
        return PTR_ERR(dtsled.device);
    }

    printk(KERN_INFO "dts_led module loaded \n");
    return 0;
}

static void led_module_exit(void)
{
    /* 取消映射 */
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);
    cdev_del(&dtsled.cdev);
    unregister_chrdev_region(dtsled.devid, 1);
    printk(KERN_INFO "dts_led module unloaded \n");
}

module_init(led_module_init);
module_exit(led_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A Simple Character Device Driver");
MODULE_VERSION("0.1");
