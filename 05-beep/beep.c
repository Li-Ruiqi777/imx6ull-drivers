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
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

struct beep_dev
{
    dev_t devid;            // 设备号
    int major;              // 主设备号
    int minor;              // 次设备号
    struct cdev cdev;       // 字符设备
    struct class *class;    // 设备类
    struct device *device;  // 设备实例
    struct device_node *nd; // 设备数中的节点
    int gpio_index;           // led对应gpio的序号
};

struct beep_dev gpio_beep;

#define LEDOFF 0 /* 关灯 */
#define LEDON 1  /* 开灯 */

void beep_init(void);
static int beep_open(struct inode *inode, struct file *file);
static int beep_release(struct inode *inode, struct file *file);
static ssize_t beep_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t beep_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static int beep_module_init(void);
static void beep_module_exit(void);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .release = beep_release,
    .read = beep_read,
    .write = beep_write,
};

static int beep_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "LED opened\n");
    file->private_data = &gpio_beep;
    return 0;
}

static int beep_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "LED closed\n");
    return 0;
}

static ssize_t beep_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}

static ssize_t beep_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    if (buf == NULL)
    {
        return -EINVAL;
    }

    int ret = 0;
    uint8_t databuf[1];
    ret = __copy_from_user(databuf, buf, count);
    struct beep_dev *dev = file->private_data;

    if (ret < 0)
    {
        printk(KERN_INFO "write LED status failed! \n");
        return EFAULT;
    }

    if (databuf[0] == LEDON)
    {
        gpio_set_value(dev->gpio_index, 0);
    }
    else
    {
        gpio_set_value(dev->gpio_index, 1);
    }
}

// 初始化LED
void beep_init(void)
{
    u32 val = 0;
    int ret;
    u32 regdata[14];
    const char *str;
    struct property *proper;

    /* 获取设备树中的属性数据 */
    /* 1、获取设备节点 */
    gpio_beep.nd = of_find_node_by_path("/beep");
    if (gpio_beep.nd == NULL)
    {
        printk("beep node nost find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("beep node find!\r\n");
    }

    /* 2、获取compatible属性内容 */
    proper = of_find_property(gpio_beep.nd, "compatible", NULL);
    if (proper == NULL)
    {
        printk("compatible property find failed\r\n");
    }
    else
    {
        printk("compatible = %s\r\n", (char *)proper->value);
    }

    /* 3、获取status属性内容 */
    ret = of_property_read_string(gpio_beep.nd, "status", &str);
    if (ret < 0)
    {
        printk("status read failed!\r\n");
    }
    else
    {
        printk("status = %s\r\n", str);
    }

    /* 4、获取设备树的gpio子系统节点的属性,得到led的所使用的gpio编号 */
    gpio_beep.gpio_index = of_get_named_gpio(gpio_beep.nd, "beep-gpio", 0);
    if (gpio_beep.gpio_index < 0)
    {
        printk("can't get beep-gpio! \r\n");
        return -EINVAL;
    }
    printk("beep-gpio num = %d\r\n", gpio_beep.gpio_index);

    /* 5.设置该GPIO为输出,默认高电平 */
    ret = gpio_direction_output(gpio_beep.gpio_index, 1);
    if (ret < 0)
    {
        printk("can't set beep! \r\n");
    }
}

static int beep_module_init(void)
{
    int result;
    //* 通过寄存器从硬件上初始化LED
    beep_init();

    //* 动态分配设备号
    if (gpio_beep.major)
    {
        gpio_beep.devid = MKDEV(gpio_beep.major, 0);
        register_chrdev_region(gpio_beep.devid, 1, "beep");
    }
    else
    {                                                          /* 没有定义设备号 */
        alloc_chrdev_region(&gpio_beep.devid, 0, 1, "beep"); /* 申请设备号 */
        gpio_beep.major = MAJOR(gpio_beep.devid);                  /* 获取分配号的主设备号 */
        gpio_beep.minor = MINOR(gpio_beep.devid);                  /* 获取分配号的次设备号 */
    }
    printk("gpio_beep major=%d,minor=%d\r\n", gpio_beep.major, gpio_beep.minor);

    //* 注册此类字符设备
    gpio_beep.cdev.owner = THIS_MODULE;
    cdev_init(&gpio_beep.cdev, &fops);
    result = cdev_add(&gpio_beep.cdev, gpio_beep.devid, 1);
    if (result)
    {
        printk(KERN_NOTICE "Error %d adding beep", result);
        unregister_chrdev_region(gpio_beep.devid, 1);
        return result;
    }

    //* 创建类
    gpio_beep.class = class_create(THIS_MODULE, "beep");
    if (IS_ERR(gpio_beep.class))
    {
        return PTR_ERR(gpio_beep.class);
    }

    //* 创建字符设备实例
    gpio_beep.device = device_create(gpio_beep.class, NULL, gpio_beep.devid, NULL, "beep");
    if (IS_ERR(gpio_beep.device))
    {
        return PTR_ERR(gpio_beep.device);
    }

    printk(KERN_INFO "beep module loaded \n");
    return 0;
}

static void beep_module_exit(void)
{
    device_destroy(gpio_beep.class, gpio_beep.devid);
    class_destroy(gpio_beep.class);
    cdev_del(&gpio_beep.cdev);
    unregister_chrdev_region(gpio_beep.devid, 1);
    printk(KERN_INFO "beep module unloaded \n");
}

module_init(beep_module_init);
module_exit(beep_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A Simple Character Device Driver");
MODULE_VERSION("0.1");
