KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
obj-m := irq_key.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean

install:
	install irq_key.ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	make clean
