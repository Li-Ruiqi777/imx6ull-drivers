#include "st7789.h"
#include "asm-generic/gpio.h"
#include "linux/delay.h"
#include "linux/init.h"
#include "linux/kern_levels.h"
#include "linux/module.h"
#include "linux/of_gpio.h"
#include "linux/spi/spi.h"

uint32_t gpio_res;
uint32_t gpio_dc;
uint32_t gpio_cs;
uint32_t gpio_blk;

struct spi_device *st7789_spi_device = NULL;

static int st7789_probe(struct spi_device *spi)
{
    printk(KERN_INFO "st7789 probe \n");

    /* 初始化用到的GPIO */
    gpio_res = of_get_named_gpio(spi->dev.of_node, "reset-gpios", 0);
    gpio_dc  = of_get_named_gpio(spi->dev.of_node, "dc-gpios", 0);
    gpio_cs  = of_get_named_gpio(spi->dev.of_node, "cs-gpios", 0);
    gpio_blk = of_get_named_gpio(spi->dev.of_node, "ledp-gpios", 0);

    /* 申请复位IO*/
    if (gpio_is_valid(gpio_res))
    {
        /* 申请IO，并且默认输出高电平 */
        int ret = devm_gpio_request_one(&spi->dev, gpio_res,
                                        GPIOF_OUT_INIT_HIGH, "st7789 reset");
        if (ret)
            return ret;
    }
    else
    {
        printk(KERN_ERR "gpio_res invalid \n");
    }

    /* 申请D/C IO*/
    if (gpio_is_valid(gpio_dc))
    {
        /* 申请IO，并且默认输出高电平 */
        int ret = devm_gpio_request_one(&spi->dev, gpio_dc, GPIOF_OUT_INIT_HIGH,
                                        "st7789 dc");
        if (ret)
            return ret;
    }
    else
    {
        printk(KERN_ERR "gpio_dc invalid \n");
    }

    /* 申请CS IO*/
    if (gpio_is_valid(gpio_cs))
    {
        /* 申请IO，并且默认输出高电平 */
        int ret = devm_gpio_request_one(&spi->dev, gpio_cs, GPIOF_OUT_INIT_HIGH,
                                        "st7789 cs");
        if (ret)
            return ret;
    }
    else
    {
        printk(KERN_ERR "gpio_cs invalid \n");
    }

    /* 申请背光IO*/
    if (gpio_is_valid(gpio_blk))
    {
        /* 申请IO，并且默认输出高电平 */
        int ret = devm_gpio_request_one(&spi->dev, gpio_blk,
                                        GPIOF_OUT_INIT_HIGH, "st7789 blk");
        if (ret)
            return ret;
    }
    else
    {
        printk(KERN_ERR "gpio_blk invalid \n");
    }

    /* 初始化SPI */
    st7789_spi_device = spi;
    spi->mode         = SPI_MODE_0;
    if (spi_setup(spi) != 0)
    {
        printk(KERN_ERR "spi_setup error \n");
        return 1;
    }

    /* 初始化LCD */
    LCD_Init();
    LCD_Fill(0, 0, 240, 280, 0x001F);
    return 0;
}

static int st7789_remove(struct spi_device *spi)
{
    printk(KERN_INFO "st7789 remove \n");
    return 0;
}

static const struct spi_device_id spi_id[] = {{"alientek,st7789", 0}, {}};

static const struct of_device_id spi_match[] = {
    {.compatible = "alientek,st7789"}, {}};

static struct spi_driver st7789_spi_driver = {
    .probe    = st7789_probe,
    .remove   = st7789_remove,
    .id_table = spi_id,
    .driver =
        {
            .name           = "st7789",
            .owner          = THIS_MODULE,
            .of_match_table = spi_match,
        },
};

static int __init ST7789_init(void)
{
    int ret = spi_register_driver(&st7789_spi_driver);
    if (ret != 0)
    {
        printk(KERN_ERR "Failed to register SPI driver\n");
        return ret;
    }
    printk(KERN_INFO "ST7789 driver initialized\n");
    return 0;
}

static void __exit ST7789_exit(void)
{
    spi_unregister_driver(&st7789_spi_driver);
    printk(KERN_INFO "ST7789 driver unloaded\n");
}

module_init(ST7789_init);
module_exit(ST7789_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LRQ");
