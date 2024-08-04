#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/types.h>

#include <asm/io.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>

#include "asm/memory.h"
#include "linux/gfp.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "oled.h"

static struct i2c_client *oled_i2c_client;
static struct fb_info *oled_fb_info = NULL;
static struct task_struct *oled_kthread;

static unsigned char *data_buf;
unsigned long attrs = DMA_ATTR_WRITE_COMBINE; // 属性标志
static unsigned int pseudo_palette[16];

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
    u8 data[] = {0xAE, 0x00, 0x10, 0x40, 0xB0, 0x81, 0xFF, 0xA1, 0xA6, 0xA8, 0x3F, 0xC8, 0xD3, 0x00,
                 0xD5, 0x80, 0xD8, 0x05, 0xD9, 0xF1, 0xDA, 0x12, 0xDB, 0x30, 0x8D, 0x14, 0xAF};
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

static int get_pixel(int x, int y)
{
    unsigned char byte = *(oled_fb_info->screen_base + y * oled_fb_info->fix.line_length + (x >> 3));
    int bit = x & 0x7;
    if (byte & (1 << bit))
        return 1;
    else
        return 0;
}

static void convert_fb_to_oled(void)
{
    unsigned char data;
    int i = 0;
    int x, page;
    int bit;

    for (page = 0; page < 8; page++)
    {
        for (x = 0; x < 128; x++)
        {
            data = 0;
            for (bit = 0; bit < 8; bit++)
            {
                data |= (get_pixel(x, page * 8 + bit) << bit);
            }
            data_buf[i++] = data;
        }
    }
}

static int oled_thread(void *data)
{
    unsigned char y;
    while (1)
    {
        if (oled_fb_info->screen_base == NULL || oled_fb_info == NULL || data_buf == NULL)
        {
            printk("ERROR! \n");
            return -1;
        }
        /* 把Framebuffer的数据刷到OLED上去 */
        convert_fb_to_oled();
        // memcpy(data_buf, oled_fb_info->screen_base, 1024);
        for (y = 0; y < 8; y++)
        {
            oled_set_pos(0, y);
            oled_write_data(&data_buf[y * 128], 128);
            // oled_write_datas(oled_fb_info->screen_base+y*128, 128);
        }

        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(HZ);

        if (kthread_should_stop())
        {
            set_current_state(TASK_RUNNING);
            break;
        }
    }
    return 0;
}

static int mylcd_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue, unsigned transp,
                           struct fb_info *info)
{
    return 1; /* unkown type */
}

static struct fb_ops myfb_ops = {
    .owner = THIS_MODULE,
    .fb_setcolreg = mylcd_setcolreg,
    .fb_fillrect = cfb_fillrect,
    .fb_copyarea = cfb_copyarea,
    .fb_imageblit = cfb_imageblit,
};

static int oled_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    printk("oled_probe \n");

    oled_i2c_client = client;
    if (client == NULL)
    {
        printk(KERN_ERR "oled_i2c_probe: failed to allocate memory for client\n");
        return -ENOMEM;
    }

    /* 1.1 分配 fb_info */
    oled_fb_info = framebuffer_alloc(0, &client->dev);

    /* 1.2 设置fb_info */
    /* a. var : LCD分辨率、颜色格式 */
    oled_fb_info->var.xres_virtual = oled_fb_info->var.xres = 128; // 宽度
    oled_fb_info->var.yres_virtual = oled_fb_info->var.yres = 64;  // 高度

    oled_fb_info->var.bits_per_pixel = 1; // 1位表示一个像素

    /* b. fix */
    strcpy(oled_fb_info->fix.id, "pgg_oled");
    oled_fb_info->fix.smem_len = oled_fb_info->var.xres * oled_fb_info->var.yres * oled_fb_info->var.bits_per_pixel /
                                 8; // 显存占据的地址大小，要除以8变成字节数

    /* c. 分配显存 */
    // oled_fb_info->screen_base = dmam_alloc_coherent(&client->dev,
    // oled_fb_info->fix.smem_len, &phy_addr_fb, GFP_KERNEL);

    // oled_fb_info->screen_base = devm_kzalloc(&client->dev, oled_fb_info->fix.smem_len, GFP_KERNEL);
    oled_fb_info->screen_base = kzalloc(oled_fb_info->fix.smem_len, GFP_DMA);
    if (oled_fb_info->screen_base == NULL)
    {
        printk(KERN_ERR "oled_i2c_probe: failed to allocate memory for "
                        "oled_fb_info->screen_base\n");
        return -ENOMEM;
    }

    oled_fb_info->fix.smem_start = virt_to_phys(oled_fb_info->screen_base); // fb的物理地址

    oled_fb_info->fix.type = FB_TYPE_PACKED_PIXELS; // 表示像素类型
    oled_fb_info->fix.visual = FB_VISUAL_MONO10;    // 表示单色屏幕

    oled_fb_info->fix.line_length = oled_fb_info->var.xres * oled_fb_info->var.bits_per_pixel / 8; // 每行的长度

    // data_buf = devm_kzalloc(&client->dev, oled_fb_info->fix.smem_len, GFP_KERNEL);
    data_buf = kzalloc(oled_fb_info->fix.smem_len, GFP_KERNEL);
    if (data_buf == NULL)
    {
        printk(KERN_ERR "oled_i2c_probe: failed to allocate memory for data_buf\n");
        return -ENOMEM;
    }

    /* d. fbops */
    oled_fb_info->fbops = &myfb_ops;
    oled_fb_info->pseudo_palette = pseudo_palette;
    register_framebuffer(oled_fb_info);

    /* 硬件初始化 */
    oled_init();
    oled_clear();
    oled_showstring(1, 1, "LRQ 666!");

    /* 创建1个内核线程,用来把Framebuffer的数据通过I2C控制器发送给OLED */
    oled_kthread = kthread_create(oled_thread, NULL, "ssd1306");
    wake_up_process(oled_kthread);

    return 0;
}

static int oled_i2c_remove(struct i2c_client *i2c)
{
    kzfree(oled_fb_info->screen_base);
    kzfree(data_buf);
    return 0;
}

static struct i2c_device_id oled_i2c_id[] = {{"alientek,oled", 0}, {}};

static const struct of_device_id oled_of_match[] = {{.compatible = "alientek,oled"}, {}};

static struct i2c_driver oled_i2c_driver = {
    .probe = oled_i2c_probe,
    .remove = oled_i2c_remove,
    .id_table = oled_i2c_id,
    .driver =
        {
            .name = "oled",
            .owner = THIS_MODULE,
            .of_match_table = oled_of_match,
        },
};

module_i2c_driver(oled_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LRQ");
