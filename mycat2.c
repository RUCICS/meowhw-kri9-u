#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUF_SZ (256 * 1024)   /* 256 KiB */

int main(int argc, char *argv[]) {
    int fd = STDIN_FILENO;
    if (argc > 1) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) { perror("open"); return 1; }
    }

    char *buf = malloc(BUF_SZ);
    if (!buf) { perror("malloc"); return 1; }

    ssize_t n;
    while ((n = read(fd, buf, BUF_SZ)) > 0) {
        if (write(STDOUT_FILENO, buf, n) != n) {
            perror("write"); return 1;
        }
    }
    if (n < 0) perror("read");

    free(buf);
    if (fd != STDIN_FILENO) close(fd);
    return 0;
}
