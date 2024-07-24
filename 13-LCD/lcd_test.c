#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    const char *tty_device = "/dev/tty1";  // TTY1设备文件
    int tty_fd;

    // 以可读写方式打开TTY1设备
    tty_fd = open(tty_device, O_RDWR);
    printf("open!");
    if (tty_fd == -1) {
        perror("Failed to open TTY1");
        return 1;
    }

    // 发送清屏控制序列
    const char *clear_screen = "\033[2J";  // ANSI控制序列清屏
    if (write(tty_fd, clear_screen, sizeof(clear_screen) - 1) == -1) {
        perror("Failed to write to TTY1");
        close(tty_fd);
        return 1;
    }

    // 关闭TTY1设备
    close(tty_fd);

    return 0;
}
