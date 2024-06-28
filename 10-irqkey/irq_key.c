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
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LEDOFF 0
#define LEDON 1

struct irqkey_desc
{
    int gpio_index;                      // 设备对应的gpio的序号
    int irq_num;                         // 中断号
    unsigned char value;                 // 按键对应的键值
    char name[10];                       // 名字
    irqreturn_t (*handler)(int, void *); // 中断服务函数
};

struct irqkey_dev
{
    dev_t devid;            // 设备号
    int major;              // 主设备号
    int minor;              // 次设备号
    struct cdev cdev;       // 字符设备
    struct class *class;    // 设备类
    struct device *device;  // 设备实例
    struct device_node *nd; // 设备数中的节点

    struct timer_list timer;
    int period; // 定时器周期(ms)

    struct irqkey_desc irqkeydesc; // 按键描述
};

struct irqkey_dev key_dev;

void key_io_init(void);

static int key_open(struct inode *inode, struct file *file);
static ssize_t key_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);

static int key_probe(struct platform_device *dev);
static int key_remove(struct platform_device *dev);

static int key_input_init(void);
static void key_input_exit(void);

static void timer_callback(unsigned long arg);
static irqreturn_t key0_handler(int irq, void *dev_id);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
};

static struct miscdevice key_misc_dev = {
    .minor = 144,
    .fops = &fops,
    .name = "irqkey" // dev/生成的设备节点的名字
};

static int key_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "device opened\n");
    file->private_data = &key_dev;
    return 0;
}

static ssize_t key_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct irqkey_dev *dev = file->private_data;

    u8 val = dev->irqkeydesc.value;
    int ret = __copy_to_user(buf, &val, sizeof(val));
    return ret;
}

// 初始化LED
void key_io_init(void)
{
    u32 val = 0;
    int ret;
    u32 regdata[14];
    const char *str;
    struct property *proper;

    /* 获取设备节点 */
    key_dev.nd = of_find_node_by_path("/key");
    if (key_dev.nd == NULL)
    {
        printk("device node nost find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("device node find!\r\n");
    }

    /* 获取设备树的gpio子系统节点的属性,得到led的所使用的gpio编号 */
    key_dev.irqkeydesc.gpio_index = of_get_named_gpio(key_dev.nd, "key-gpio", 0);
    if (key_dev.irqkeydesc.gpio_index < 0)
    {
        printk("can't get key-gpio! \r\n");
        return -EINVAL;
    }
    printk("key-gpio num = %d\r\n", key_dev.irqkeydesc.gpio_index);

    /* 设置该GPIO */
    gpio_request(key_dev.irqkeydesc.gpio_index, "key0");
    gpio_direction_input(key_dev.irqkeydesc.gpio_index);
    key_dev.irqkeydesc.irq_num = irq_of_parse_and_map(key_dev.nd, 0);

    /* 配置中断 */
    key_dev.irqkeydesc.handler = key0_handler;
    key_dev.irqkeydesc.value = 0;
    memset(key_dev.irqkeydesc.name, 0, sizeof(key_dev.irqkeydesc.name));
    sprintf(key_dev.irqkeydesc.name, "KEY0");
    request_irq(key_dev.irqkeydesc.irq_num,
                key_dev.irqkeydesc.handler,
                IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                key_dev.irqkeydesc.name,
                &key_dev);

    /* 初始化定时器 */
    init_timer(&key_dev.timer);
    key_dev.timer.function = timer_callback;
    add_timer(&key_dev.timer);
}

static void timer_callback(unsigned long arg)
{
    int val = gpio_get_value(key_dev.irqkeydesc.gpio_index);
    key_dev.irqkeydesc.value = val;
    printk("key = %d \n", val);
}

static irqreturn_t key0_handler(int irq, void *dev_id)
{
    printk("start tiemr! \n");
    mod_timer(&key_dev.timer,
              jiffies + msecs_to_jiffies(10));
    return IRQ_RETVAL(IRQ_HANDLED);
}

int key_probe(struct platform_device *dev)
{
    int result;
    result = misc_register(&key_misc_dev);

    printk(KERN_INFO "patform driver loaded \n");

    key_io_init();
    return 0;
}

static int key_remove(struct platform_device *dev)
{
    gpio_direction_output(key_dev.irqkeydesc.gpio_index, 1);
    gpio_free(key_dev.devid);
    misc_deregister(&key_misc_dev);
    del_timer_sync(&key_dev.timer);
    printk(KERN_INFO "patform driver unloaded \n");
}

static const struct of_device_id led_of_match[] = {
    {.compatible = "gpio-led"}, // 设备树的 .compatible 属性
    {}};

static struct platform_driver led_driver = {
    .driver = {
        .name = "imx6ul-led",
        .of_match_table = led_of_match,
    },
    .probe = key_probe,
    .remove = key_remove,
};

static int key_input_init(void)
{
    return platform_driver_register(&led_driver);
}

static void key_input_exit(void)
{
    platform_driver_unregister(&led_driver);
}

module_init(key_input_init);
module_exit(key_input_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A Simple Character Device Driver");
MODULE_VERSION("0.1");
