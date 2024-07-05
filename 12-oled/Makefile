KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
obj-m := oled.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	# rm ./oled_test

test:
	arm-linux-gnueabihf-gcc oled_test.c -o oled_test

install:
	install oled.ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	# install oled_test /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	make clean
