KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
obj-m := beep.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm ./beep_test

test:
	arm-linux-gnueabihf-gcc beep_test.c -o beep_test

install:
	install beep.ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	install beep_test /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15