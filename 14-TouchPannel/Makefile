KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
obj-m := touch_pannel.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean

test:
	arm-linux-gnueabihf-gcc touch_pannel_test.c -o touch_pannel_test

install:
	install touch_pannel.ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	make clean
