// mycat1.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    int fd = STDIN_FILENO;
    if (argc > 1) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) { perror("open file"); exit(1); }
    }
    int out_fd = STDOUT_FILENO;
    char c;
    // 单字节读取和写入
    ssize_t n;
    while ((n = read(fd, &c, 1)) == 1) {
        if (write(out_fd, &c, 1) != 1) {
            perror("write error");
            exit(1);
        }
    }
    if (n < 0) { perror("read error"); exit(1); }
    if (fd != STDIN_FILENO) close(fd);
    return 0;
}
