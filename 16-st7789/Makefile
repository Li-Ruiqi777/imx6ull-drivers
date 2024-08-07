KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
MODULE_NAME := LCD_st7789

obj-m := $(MODULE_NAME).o

$(MODULE_NAME)-objs := st7789.o st7789_init.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean

install:
	install $(MODULE_NAME).ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	make clean
