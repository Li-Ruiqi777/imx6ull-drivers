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

struct key_dev
{
    dev_t devid;            // 设备号
    int major;              // 主设备号
    int minor;              // 次设备号
    struct cdev cdev;       // 字符设备
    struct class *class;    // 设备类
    struct device *device;  // 设备实例
    struct device_node *nd; // 设备数中的节点
    int gpio_index;         // 设备对应gpio的序号
};

struct key_dev gpio_key;

#define KEY0VALUE 0XF0 // 按键值
#define INVAKEY 0X00   // 无效按键

void key_init(void);
static int key_open(struct inode *inode, struct file *file);
static int key_release(struct inode *inode, struct file *file);
static ssize_t key_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t key_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static int key_module_init(void);
static void key_module_exit(void);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .release = key_release,
    .read = key_read,
    .write = key_write,
};

static int key_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "KEY opened\n");
    file->private_data = &gpio_key;
    return 0;
}

static int key_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "KEY closed\n");
    return 0;
}

static ssize_t key_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct key_dev *dev = file->private_data;

    int val = gpio_get_value(dev->gpio_index);
    int ret = __copy_to_user(buf, &val, sizeof(val));
    return ret;
}

static ssize_t key_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}

// 初始化KEY
void key_init(void)
{
    u32 val = 0;
    int ret;
    u32 regdata[14];
    const char *str;
    struct property *proper;

    /* 获取设备树中的属性数据 */
    /* 1、获取设备节点 */
    gpio_key.nd = of_find_node_by_path("/key");
    if (gpio_key.nd == NULL)
    {
        printk("key node nost find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("key node find!\r\n");
    }

    /* 2、获取compatible属性内容 */
    proper = of_find_property(gpio_key.nd, "compatible", NULL);
    if (proper == NULL)
    {
        printk("compatible property find faiKEY\r\n");
    }
    else
    {
        printk("compatible = %s\r\n", (char *)proper->value);
    }

    /* 3、获取status属性内容 */
    ret = of_property_read_string(gpio_key.nd, "status", &str);
    if (ret < 0)
    {
        printk("status read faiKEY!\r\n");
    }
    else
    {
        printk("status = %s\r\n", str);
    }

    /* 4、获取设备树的gpio子系统节点的属性,得到KEY的所使用的gpio编号 */
    gpio_key.gpio_index = of_get_named_gpio(gpio_key.nd, "key-gpio", 0);
    if (gpio_key.gpio_index < 0)
    {
        printk("can't get key-gpio! \r\n");
        return -EINVAL;
    }
    printk("key-gpio num = %d\r\n", gpio_key.gpio_index);

    /* 5.设置该GPIO为输入 */
    ret = gpio_request(gpio_key.gpio_index, "key0");
    if (ret < 0)
    {
        printk("can't request beep! \r\n");
    }
    gpio_direction_input(gpio_key.gpio_index);
}

static int key_module_init(void)
{
    int result;
    //* 通过寄存器从硬件上初始化KEY
    key_init();

    //* 动态分配设备号
    if (gpio_key.major)
    {
        gpio_key.devid = MKDEV(gpio_key.major, 0);
        register_chrdev_region(gpio_key.devid, 1, "key");
    }
    else
    {                                                      /* 没有定义设备号 */
        alloc_chrdev_region(&gpio_key.devid, 0, 1, "key"); /* 申请设备号 */
        gpio_key.major = MAJOR(gpio_key.devid);            /* 获取分配号的主设备号 */
        gpio_key.minor = MINOR(gpio_key.devid);            /* 获取分配号的次设备号 */
    }
    printk("gpio_key major=%d,minor=%d\r\n", gpio_key.major, gpio_key.minor);

    //* 注册此类字符设备
    gpio_key.cdev.owner = THIS_MODULE;
    cdev_init(&gpio_key.cdev, &fops);
    result = cdev_add(&gpio_key.cdev, gpio_key.devid, 1);
    if (result)
    {
        printk(KERN_NOTICE "Error %d adding key", result);
        unregister_chrdev_region(gpio_key.devid, 1);
        return result;
    }

    //* 创建类
    gpio_key.class = class_create(THIS_MODULE, "key");
    if (IS_ERR(gpio_key.class))
    {
        return PTR_ERR(gpio_key.class);
    }

    //* 创建字符设备实例
    gpio_key.device = device_create(gpio_key.class, NULL, gpio_key.devid, NULL, "key");
    if (IS_ERR(gpio_key.device))
    {
        return PTR_ERR(gpio_key.device);
    }

    printk(KERN_INFO "key module loaded \n");
    return 0;
}

static void key_module_exit(void)
{
    device_destroy(gpio_key.class, gpio_key.devid);
    class_destroy(gpio_key.class);
    cdev_del(&gpio_key.cdev);
    unregister_chrdev_region(gpio_key.devid, 1);
    printk(KERN_INFO "key module unloaded \n");
}

module_init(key_module_init);
module_exit(key_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A Simple Character Device Driver");
MODULE_VERSION("0.1");
