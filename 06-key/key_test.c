#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>

#define KEY0VALUE 0XF0 // 按键值
#define INVAKEY 0X00   // 无效按键

int main()
{
	int key_fd, retvalue;
	int beep_fd;
	char *key_dev = "/dev/key";
	char *beep_dev = "/dev/beep";
	int keyvalue;

	/* 打开驱动 */
	key_fd = open(key_dev, O_RDWR);
	beep_fd = open(beep_dev, O_RDWR);

	if (key_fd < 0)
	{
		printf("file %s open failed!\r\n", key_dev);
		return -1;
	}

	while (1)
	{
		read(key_fd, &keyvalue, sizeof(keyvalue));
		if (keyvalue == 0)
		{
			printf("KEY0 Press, value = %#X\r\n", keyvalue); // 按下
			write(beep_fd, &keyvalue, sizeof(keyvalue));
		}
		else
			write(beep_fd, &keyvalue, sizeof(keyvalue));
	}

	/* 关闭文件 */
	retvalue = close(key_fd);
	retvalue = close(beep_fd);
	if (retvalue < 0)
	{
		printf("file %s close failed!\r\n", key_dev);
		return -1;
	}

	return 0;
}
