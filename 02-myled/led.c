#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/init.h>
#include <asm/io.h>

static dev_t dev_number;
static struct cdev led_cdev;
static struct class *led_class;

/* 寄存器物理地址 */
#define CCM_CCGR1_BASE (0X020C406C)
#define SW_MUX_GPIO1_IO03_BASE (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE (0X020E02F4)
#define GPIO1_DR_BASE (0X0209C000)
#define GPIO1_GDIR_BASE (0X0209C004)

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

#define LEDOFF 0 /* 关灯 */
#define LEDON 1  /* 开灯 */

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

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .read = led_read,
    .write = led_write,
};

// 初始化LED
void led_init(void)
{
  int val = 0;
  /* 1、寄存器地址映射 */
  IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
  SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
  SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
  GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
  GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

  /* 2、使能GPIO1时钟 */
  val = readl(IMX6U_CCM_CCGR1);
  val &= ~(3 << 26); /* 清楚以前的设置 */
  val |= (3 << 26);  /* 设置新值 */
  writel(val, IMX6U_CCM_CCGR1);

  /* 3、设置GPIO1_IO03的复用功能，将其复用为
   *    GPIO1_IO03，最后设置IO属性。
   */
  writel(5, SW_MUX_GPIO1_IO03);

  /*寄存器SW_PAD_GPIO1_IO03设置IO属性
   *bit 16:0 HYS关闭
   *bit [15:14]: 00 默认下拉
   *bit [13]: 0 kepper功能
   *bit [12]: 1 pull/keeper使能
   *bit [11]: 0 关闭开路输出
   *bit [7:6]: 10 速度100Mhz
   *bit [5:3]: 110 R0/6驱动能力
   *bit [0]: 0 低转换率
   */
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

static int __init led_module_init(void)
{
  int result;
  //* 通过寄存器从硬件上初始化LED
  led_init();

  //* 动态分配设备号
  result = alloc_chrdev_region(&dev_number, 0, 1, "led_device");
  if (result < 0)
  {
    printk(KERN_WARNING "Can't get major number\n");
    return result;
  }

  //* 注册此类字符设备
  cdev_init(&led_cdev, &fops);
  led_cdev.owner = THIS_MODULE;
  result = cdev_add(&led_cdev, dev_number, 1);
  if (result)
  {
    printk(KERN_NOTICE "Error %d adding led_device", result);
    unregister_chrdev_region(dev_number, 1);
    return result;
  }

  led_class = class_create(THIS_MODULE, "led_class");
  if (IS_ERR(led_class))
  {
    cdev_del(&led_cdev);
    unregister_chrdev_region(dev_number, 1);
    return PTR_ERR(led_class);
  }

  //* 创建字符设备实例
  device_create(led_class, NULL, dev_number, NULL, "led_device");

  printk(KERN_INFO "led_device module loaded \n");
  return 0;
}

static void __exit led_module_exit(void)
{
  device_destroy(led_class, dev_number);
  class_destroy(led_class);
  cdev_del(&led_cdev);
  unregister_chrdev_region(dev_number, 1);
  printk(KERN_INFO "led_device module unloaded \n");
}

module_init(led_module_init);
module_exit(led_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A Simple Character Device Driver");
MODULE_VERSION("0.1");
