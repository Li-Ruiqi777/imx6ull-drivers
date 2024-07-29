KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
obj-m := fb_oled.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm ./fb_oled_test

test:
	arm-linux-gnueabihf-gcc fb_oled_test.c -o fb_oled_test

install:
	install fb_oled.ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	install fb_oled_test /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	make clean
