KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
obj-m := mychardev.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm ./test

test:
	arm-linux-gnueabihf-gcc test.c -o test

install:
	install mychardev.ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	install test /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15