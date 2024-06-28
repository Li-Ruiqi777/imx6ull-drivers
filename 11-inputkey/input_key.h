#pragma once

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
#include <linux/input.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LEDOFF 0
#define LEDON 1

struct inputkey_desc
{
    int gpio_index;                      // 设备对应的gpio的序号
    int irq_num;                         // 中断号
    unsigned char value;                 // 按键对应的键值(KEY_0)
    char name[10];                       // 名字
    irqreturn_t (*handler)(int, void *); // 中断服务函数
};

struct inputkey_dev
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

    struct inputkey_desc keydesc; // 按键描述
    struct input_dev *inputdev;
};

void key_io_init(void);

static int key_open(struct inode *inode, struct file *file);
static ssize_t key_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);

static int key_probe(struct platform_device *dev);
static int key_remove(struct platform_device *dev);

static int key_input_init(void);
static void key_input_exit(void);

static void timer_callback(unsigned long arg);
static irqreturn_t key0_handler(int irq, void *dev_id);