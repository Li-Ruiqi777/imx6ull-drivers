#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_PATH "/dev/my_device"
#define BUFFER_SIZE 100

int main()
{
  int fd;
  char read_buf[BUFFER_SIZE];
  char write_buf[BUFFER_SIZE];
  int ret;

  // 打开设备文件
  fd = open(DEVICE_PATH, O_RDWR);
  if (fd < 0)
  {
    perror("Failed to open the device...");
    return -1;
  }

  // 读取数据
  ret = read(fd, read_buf, BUFFER_SIZE);
  if (ret < 0)
  {
    perror("Failed to read from the device...");
  }
  else
  {
    printf("Read data from device: %s \r\n", read_buf);
  }

  // 写入数据
  strcpy(write_buf, "Hello from user space!");
  ret = write(fd, write_buf, strlen(write_buf) + 1);
  if (ret < 0)
  {
    perror("Failed to write to the device...");
  }

  // 关闭设备文件
  close(fd);

  return 0;
}
