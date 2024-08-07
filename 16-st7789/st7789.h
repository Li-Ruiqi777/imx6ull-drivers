#pragma once
#include "linux/gpio.h"
#include "linux/spi/spi.h"
#include "linux/spi/spidev.h"
#include <linux/types.h>

extern uint32_t gpio_res;
extern uint32_t gpio_dc;
extern uint32_t gpio_cs;
extern uint32_t gpio_blk;

extern struct spi_device *st7789_spi_device;

void LCD_RES_Clr(void);
void LCD_RES_Set(void);

void LCD_DC_Clr(void);
void LCD_DC_Set(void);

void LCD_CS_Clr(void);
void LCD_CS_Set(void);

void LCD_BLK_Clr(void);
void LCD_BLK_Set(void);

int spi_send(struct spi_device *spi, uint8_t *buf, int len);
int spi_receive(struct spi_device *spi, uint8_t *buf, int len);

void LCD_WR_DATA8(uint8_t dat); // 写入一个字节
void LCD_WR_DATA(uint16_t dat); // 写入两个字节
void LCD_WR_REG(uint8_t dat);   // 写入一个指令

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2); // 设置坐标函数
void LCD_Init(void);                                                      // LCD初始化


void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color); //指定区域填充颜色
