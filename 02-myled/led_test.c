#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>

#define LEDOFF 0
#define LEDON 1


int main(int argc, char *argv[])
{
	int fd, retvalue;
	char *filename;
    uint8_t led_status = 0;

	if (argc != 3)
	{
		printf("Error Usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开led驱动 */
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("file %s open failed!\r\n", argv[1]);
		return -1;
	}

	led_status = atoi(argv[2]); /* 要执行的操作：打开或关闭 */

	/* 向/dev/led文件写入数据 */
	retvalue = write(fd, &led_status, sizeof(led_status));

	if (retvalue < 0)
	{
		printf("LED Control Failed!\r\n");
		close(fd);
		return -1;
	}

	retvalue = close(fd); /* 关闭文件 */
	if (retvalue < 0)
	{
		printf("file %s close failed!\r\n", argv[1]);
		return -1;
	}

	return 0;
}
