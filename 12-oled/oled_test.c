#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int ret = 0;
    int fd;
    unsigned short databuf[3];
    char *filename;
    if (argc != 2)
    {
        printf("Error happpend!\r\n");
        return -1;
    }
    filename = argv[1];
    fd = open(filename, O_RDWR);
    if (fd < 0)
    {
        printf("jklcan't open file %s\r\n", filename);
        return -1;
    }

    ret = write(fd, databuf, sizeof(databuf));

    close(fd);
    return 0;
}
