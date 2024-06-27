KERNELDIR := /home/lrq/linux/IMX6ULL/temp
CURRENT_PATH := $(shell pwd)
obj-m := led.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm ./led_test

test:
	arm-linux-gnueabihf-gcc led_test.c -o led_test

install:
	install led.ko /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15
	install led_test /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15