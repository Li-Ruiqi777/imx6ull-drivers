#include "asm/gpio.h"
#include "linux/delay.h"
#include "linux/kern_levels.h"
#include "linux/printk.h"
#include "linux/spi/spi.h"
#include "linux/types.h"
#include "st7789.h"

void LCD_RES_Clr(void)
{
    gpio_set_value(gpio_res, 0);
}

void LCD_RES_Set(void)
{
    gpio_set_value(gpio_res, 1);
}

void LCD_DC_Clr(void)
{
    gpio_set_value(gpio_dc, 0);
}

void LCD_DC_Set(void)
{
    gpio_set_value(gpio_dc, 1);
}

void LCD_CS_Clr(void)
{
    gpio_set_value(gpio_cs, 0);
}

void LCD_CS_Set(void)
{
    gpio_set_value(gpio_cs, 1);
}

void LCD_BLK_Clr(void)
{
    gpio_set_value(gpio_blk, 0);
}

void LCD_BLK_Set(void)
{
    gpio_set_value(gpio_blk, 1);
}

int spi_send(struct spi_device *spi, uint8_t *buf, int len)
{
    if (spi == NULL)
    {
        printk(KERN_ERR "spi is null");
        return 0;
    }
    int                 ret;
    struct spi_message  msg;
    struct spi_transfer transfer = {
        .tx_buf = buf,
        .len    = len,
    };

    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    ret = spi_sync(spi, &msg);
    if (ret < 0)
        printk(KERN_ERR "SPI transfer failed with error: %d\n", ret);

    return ret;
}

int spi_receive(struct spi_device *spi, uint8_t *buf, int len)
{
    if (spi == NULL)
    {
        printk(KERN_ERR "spi is null");
        return 0;
    }
    int                 ret;
    struct spi_message  msg;
    struct spi_transfer transfer = {
        .rx_buf = buf,
        .len    = len,
    };

    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    ret = spi_sync(spi, &msg);
    if (ret < 0)
        printk(KERN_ERR "SPI transfer failed with error: %d\n", ret);
    return ret;
}

void LCD_WR_DATA8(uint8_t dat)
{
    LCD_CS_Clr();
    spi_send(st7789_spi_device, &dat, 1);
    LCD_CS_Set();
}

void LCD_WR_DATA(uint16_t dat)
{
    uint8_t temp[2];
    temp[0] = (dat & 0xFF00) >> 8;
    temp[1] = dat & 0x0FF;
    LCD_CS_Clr();
    spi_send(st7789_spi_device, temp, 2);
    LCD_CS_Set();
}

void LCD_WR_REG(uint8_t dat)
{
    LCD_DC_Clr(); // 写命令
    LCD_CS_Clr(); // 片选
    spi_send(st7789_spi_device, &dat, 1);
    LCD_CS_Set();
    LCD_DC_Set(); // 写数据
}

void LCD_Init(void)
{
    // 初始化ST7789
    LCD_RES_Clr(); // 复位
    mdelay(100);
    LCD_RES_Set();
    mdelay(100);
    LCD_BLK_Set(); // 打开背光
    mdelay(500);
    LCD_WR_REG(0x11);
    mdelay(120); // Delay 120ms

    LCD_WR_REG(0x36); // 扫描和刷新模式
    LCD_WR_DATA8(0x00);

    LCD_WR_REG(0x3A);   // 像素格式		65K;RGB   16位
    LCD_WR_DATA8(0x55); // 0x05

    LCD_WR_REG(0xB2); // porch设置  不知道是啥东西
    LCD_WR_DATA8(0x0B);
    LCD_WR_DATA8(0x0B);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x33);
    LCD_WR_DATA8(0x33);

    LCD_WR_REG(0xB7); // VGH和VGL不知道是啥
    LCD_WR_DATA8(0x11);

    LCD_WR_REG(0xBB); // VMOS不知道是啥
    LCD_WR_DATA8(0x2F);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0x2C);

    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x01);

    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x0D);

    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x20); // VDV, 0x20:0v

    LCD_WR_REG(0xC6);
    LCD_WR_DATA8(0x0F); // 0x13:60Hz   好像是51Hz		0F才是60Hz

    LCD_WR_REG(0xD0); // Power control
    LCD_WR_DATA8(0xA4);
    LCD_WR_DATA8(0xA1);

    LCD_WR_REG(0xD6);   // 这个手册里没写
    LCD_WR_DATA8(0xA1); // sleep in后，gate输出为GND

    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0xF0);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x13);
    LCD_WR_DATA8(0x25);
    LCD_WR_DATA8(0x33);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x34);
    LCD_WR_DATA8(0x10);
    LCD_WR_DATA8(0x10);
    LCD_WR_DATA8(0x29);
    LCD_WR_DATA8(0x32);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0xF0);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x0A);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x25);
    LCD_WR_DATA8(0x32);
    LCD_WR_DATA8(0x3B);
    LCD_WR_DATA8(0x3B);
    LCD_WR_DATA8(0x17);
    LCD_WR_DATA8(0x18);
    LCD_WR_DATA8(0x2E);
    LCD_WR_DATA8(0x37);

    LCD_WR_REG(0xE4);
    LCD_WR_DATA8(0x1d); // 使用240根gate  (N+1)*8		25
    LCD_WR_DATA8(0x00); // 设定gate起点位置
    LCD_WR_DATA8(0x00); // 当gate没有用完时，bit4(TMG)设为0

    LCD_WR_REG(0x21); // 反转

    LCD_WR_REG(
        0x29); // 0X29是使屏幕显示		0x28是使屏幕消隐，但是背光单独控制

    LCD_WR_REG(0x2A); // Column Address Set
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x00); // 0
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0xEF); // 239

    LCD_WR_REG(0x2B); // Row Address Set
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x14); // 0X14		00
    LCD_WR_DATA8(0x01); // 0X01		01
    LCD_WR_DATA8(0x2b); // 0X2B		17

    LCD_WR_REG(0x2C);

    // SPI_to16(&hspi2);
}

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    /* 法1 */
    LCD_WR_REG(0x2a); // 列地址设置

    LCD_WR_DATA(x1);
    LCD_WR_DATA(x2);

    LCD_WR_REG(0x2b); // 行地址设置

    LCD_WR_DATA(y1);
    LCD_WR_DATA(y2);

    LCD_WR_REG(0x2c); // 储存器写
}

void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend,
              uint16_t color)
{
    uint16_t i, j;
    uint16_t width, height;

    width  = xend - xsta + 1;
    height = yend - ysta + 1;

    LCD_Address_Set(xsta, ysta + 20, xend, yend + 20); // 设置显示范围

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            LCD_WR_DATA(color);
        }
    }
}