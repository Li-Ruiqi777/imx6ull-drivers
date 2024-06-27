#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>

static dev_t dev_number;
static struct cdev my_cdev;
static struct class *my_class;

static char readbuf[100];  /* 读缓冲区 */
static char writebuf[100]; /* 写缓冲区 */
static char temp_data[] = {"LRQ 6666"};

static int my_open(struct inode *inode, struct file *file)
{
  printk(KERN_INFO "Device opened\n");
  return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
  printk(KERN_INFO "Device closed\n");
  return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
  memcpy(readbuf, temp_data, sizeof(temp_data));
  int ret = 0;
  ret = __copy_to_user(buf, readbuf, count);
  if (ret)
  {
    printk("read from kernel FAILED!\r\n");
  }
  else
  {
    printk("read from kernel OK!\r\n");
  }

  // 实现读取逻辑
  return 0;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
  int ret = 0;
  ret = __copy_from_user(writebuf, buf, count);
  if (ret)
  {
    printk("write to kernel FAILED!\r\n");
  }
  else
  {
    printk("write to kernel OK!\r\n");
    printk("Kernel receive data : %s\n", writebuf);
  }
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init my_module_init(void)
{
  int result;

  result = alloc_chrdev_region(&dev_number, 0, 1, "my_device");
  if (result < 0)
  {
    printk(KERN_WARNING "Can't get major number\n");
    return result;
  }

  cdev_init(&my_cdev, &fops);
  my_cdev.owner = THIS_MODULE;
  result = cdev_add(&my_cdev, dev_number, 1);
  if (result)
  {
    printk(KERN_NOTICE "Error %d adding my_device", result);
    unregister_chrdev_region(dev_number, 1);
    return result;
  }

  my_class = class_create(THIS_MODULE, "my_class");
  if (IS_ERR(my_class))
  {
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_number, 1);
    return PTR_ERR(my_class);
  }
  device_create(my_class, NULL, dev_number, NULL, "my_device");

  printk(KERN_INFO "my_device module loaded\n");
  return 0;
}

static void __exit my_module_exit(void)
{
  device_destroy(my_class, dev_number);
  class_destroy(my_class);
  cdev_del(&my_cdev);
  unregister_chrdev_region(dev_number, 1);
  printk(KERN_INFO "my_device module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A Simple Character Device Driver");
MODULE_VERSION("0.1");
