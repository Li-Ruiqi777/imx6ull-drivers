KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
obj-m := misc_led.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean

install:
	install misc_led.ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
