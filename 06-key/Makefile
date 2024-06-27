KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
obj-m := key.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm ./key_test

test:
	arm-linux-gnueabihf-gcc key_test.c -o key_test

install:
	install key.ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	install key_test /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15