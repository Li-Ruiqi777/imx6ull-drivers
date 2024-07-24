
CURRENT_PATH := $(shell pwd)

all:
	arm-linux-gnueabihf-gcc lcd_test.c -o lcd_test

install:
	install lcd_test /home/lrq/linux/nfs/rootfs/lib/modules/4.1.15

clean:
	rm ./lcd_test