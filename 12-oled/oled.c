#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "oled.h"
#include "linux/spi/spi.h"

struct oled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    int minor;
};

static struct oled_dev oledcdev;

static struct i2c_client *oled_i2c_client = NULL;

static int oled_write_command(uint8_t command)
{
    if (oled_i2c_client == NULL)
        return -1;
    uint8_t buffer[2];
    buffer[0] = 0x00; // Co = 0, D/C# = 0
    buffer[1] = command;
    return i2c_master_send(oled_i2c_client, buffer, 2);
}

static int oled_write_data(uint8_t *data, size_t size)
{
    if (oled_i2c_client == NULL)
        return -1;
    if (data == NULL)
    {
        pr_err("oled_write_data: data is NULL\n");
        return -EINVAL;
    }
    uint8_t buffer[129];
    buffer[0] = 0x40; // Co = 0, D/C# = 1
    memcpy(&buffer[1], data, size);
    return i2c_master_send(oled_i2c_client, buffer, size + 1);
}

void oled_init(void)
{
    u8 i;
    u8 data[] = {0xAE, 0x00, 0x10, 0x40, 0xB0, 0x81, 0xFF, 0xA1, 0xA6,
                 0xA8, 0x3F, 0xC8, 0xD3, 0x00, 0xD5, 0x80, 0xD8, 0x05,
                 0xD9, 0xF1, 0xDA, 0x12, 0xDB, 0x30, 0x8D, 0x14, 0xAF};
    for (i = 0; i < sizeof(data); i++)
    {
        oled_write_command(data[i]);
    }
}

void oled_set_pos(u8 x, u8 y)
{
    oled_write_command(0xb0 + y);
    oled_write_command(((x & 0xf0) >> 4) | 0x10);
    oled_write_command(x & 0x0f);
}

void oled_showchar(u8 x, u8 y, u8 chr)
{
    u8 c = 0, i = 0;
    // 得到当前字符偏移量
    c = chr - ' ';
    // 超出范围，另起一页显示
    if (x > Max_Column - 1)
    {
        // 另起一行
        x = 0;
        // 显示到下两页
        y = y + 2;
    }
    // 设置坐标
    oled_set_pos(x, y);
    // 依次写入每个字节
    for (i = 0; i < 8; i++)
    {
        // 前8字节数据
        oled_write_data(&F8X16[c * 16 + i], 1);
    }
    // 设置坐标(页数加1)，其实是下一页起始位置
    oled_set_pos(x, y + 1);
    for (i = 0; i < 8; i++)
    {
        // 后8字节数据
        oled_write_data(&F8X16[c * 16 + i + 8], 1);
    }
}

void oled_showstring(u8 x, u8 y, u8 *chr)
{
    unsigned char j = 0;
    while (chr[j] != '\0')
    {
        oled_showchar(x, y, chr[j]);
        x += 8;
        if (x > 120)
        {
            x = 0;
            y += 2;
        }
        j++;
    }
}

void oled_clear(void)
{
    u8 i, n;
    uint8_t data[1];
    data[0] = 0x00;

    for (i = 0; i < 8; i++)
    {
        oled_write_command(0xb0 + i); // 设置页地址（0~7）
        oled_write_command(0x00);     // 设置显示位置—列低地址
        oled_write_command(0x10);     // 设置显示位置—列高地址
        for (n = 0; n < 128; n++)
        {
            oled_write_data(&data, 1);
        }
    }
}

static int oled_open(struct inode *inode, struct file *filp)
{
    oled_init();
    oled_clear();
    return 0;
}

static ssize_t oled_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *off)
{
    oled_showstring(1, 1, "hello world!");
    return 0;
}

static struct file_operations oled_ops = {
    .owner = THIS_MODULE,
    .open = oled_open,
    .write = oled_write,
};

static int oled_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    printk("oled_probe \n");
    if (oledcdev.major)
    {
        oledcdev.devid = MKDEV(oledcdev.major, 0);
        register_chrdev_region(oledcdev.devid, oled_CNT,
                               oled_NAME);
    }
    else
    {
        alloc_chrdev_region(&oledcdev.devid, 0, oled_CNT,
                            oled_NAME);
        oledcdev.major = MAJOR(oledcdev.devid);
        oledcdev.minor = MINOR(oledcdev.devid);
    }
    printk("oled major = %d,minor = %d \r\n", oledcdev.major, oledcdev.minor);
    /* 2、注册设备 */
    cdev_init(&oledcdev.cdev, &oled_ops);
    cdev_add(&oledcdev.cdev, oledcdev.devid, oled_CNT);

    /* 3、创建类 */
    oledcdev.class = class_create(THIS_MODULE, oled_NAME);

    if (IS_ERR(oledcdev.class))
    {

        return PTR_ERR(oledcdev.class);
    }

    oledcdev.device = device_create(oledcdev.class, NULL, oledcdev.devid, NULL, oled_NAME);
    if (IS_ERR(oledcdev.device))
    {
        return PTR_ERR(oledcdev.device);
    }

    oled_i2c_client = client;

    oled_init();
    oled_clear();
    oled_showstring(1, 1, "LRQ 666!");

    return 0;
}

static int oled_i2c_remove(struct i2c_client *i2c)
{
    cdev_del(&oledcdev.cdev);
    unregister_chrdev_region(oledcdev.devid, DEV_CNT);
    device_destroy(oledcdev.class, oledcdev.devid);
    class_destroy(oledcdev.class);
    return 0;
}

static struct i2c_device_id oled_i2c_id[] = {
    {"alientek,oled", 0},
    {}};

static const struct of_device_id oled_of_match[] = {
    {.compatible = "alientek,oled"},
    {}};

static struct i2c_driver oled_i2c_driver = {
    .probe = oled_i2c_probe,
    .remove = oled_i2c_remove,
    .id_table = oled_i2c_id,
    .driver = {
        .name = "oled",
        .owner = THIS_MODULE,
        .of_match_table = oled_of_match,
    },
};

static int __init oledcdev_init(void)
{
    int ret = 0;

    ret = i2c_add_driver(&oled_i2c_driver);
    if (ret != 0)
    {
        pr_err("oledcdev I2C registration failed %d\n", ret);
        return ret;
    }

    return ret;
}

static void __exit oledcdev_exit(void)
{
    i2c_del_driver(&oled_i2c_driver);
}

module_init(oledcdev_init);
module_exit(oledcdev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LRQ");
