#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

static size_t get_bufsize(size_t pagesize, size_t blocksize) {
    // 从环境变量读取乘数 MULT，默认 1
    const char *env = getenv("CAT5_MULT");
    unsigned long mult = 1;
    if (env) {
        char *end;
        unsigned long v = strtoul(env, &end, 10);
        if (end != env && v > 0) {
            mult = v;
        }
    }

    // 基础缓冲区大小：256 KiB × MULT
    size_t base_size = 256UL * 1024UL * mult;

    // 取 pagesize 和 filesystem blocksize 的较大者作为对齐单位
    size_t alignment = pagesize > blocksize ? pagesize : blocksize;

    // 向上对齐到 alignment 的整数倍
    if (base_size % alignment != 0) {
        base_size += alignment - (base_size % alignment);
    }

    return base_size;
}

static int cat_fd(int fd_in, size_t pagesize) {
    int exit_status = 0;
    struct stat st;
    size_t blocksize;

    if (fstat(fd_in, &st) == 0) {
        blocksize = st.st_blksize;
    } else {
        perror("fstat");
        blocksize = pagesize;
    }

    size_t bufsize = get_bufsize(pagesize, blocksize);

    void *vbuffer;
    if (posix_memalign(&vbuffer, pagesize, bufsize) != 0) {
        perror("posix_memalign");
        return 1;
    }
    char *buffer = vbuffer;

    while (1) {
        ssize_t bytes_read = read(fd_in, buffer, bufsize);
        if (bytes_read == 0) break;  // EOF
        if (bytes_read < 0) {
            if (errno == EINTR) continue;
            perror("read");
            exit_status = 1;
            break;
        }
        ssize_t bytes_written = 0;
        while (bytes_written < bytes_read) {
            ssize_t w = write(STDOUT_FILENO,
                              buffer + bytes_written,
                              bytes_read - bytes_written);
            if (w < 0) {
                if (errno == EINTR) continue;
                perror("write");
                exit_status = 1;
                break;
            }
            bytes_written += w;
        }
        if (exit_status) break;
    }

    free(buffer);
    return exit_status;
}

int main(int argc, char *argv[]) {
    int exit_status = 0;
    size_t pagesize = sysconf(_SC_PAGE_SIZE);

    if (argc < 2) {
        // 没有参数，默认为读取 stdin
        exit_status = cat_fd(STDIN_FILENO, pagesize);
    } else {
        for (int i = 1; i < argc; ++i) {
            int fd;
            if (argv[i][0] == '-' && argv[i][1] == '\0') {
                fd = STDIN_FILENO;
            } else {
                fd = open(argv[i], O_RDONLY);
                if (fd < 0) {
                    perror(argv[i]);
                    exit_status = 1;
                    continue;
                }
            }
            int st = cat_fd(fd, pagesize);
            if (st) exit_status = st;
            if (fd != STDIN_FILENO && close(fd) < 0) {
                perror("close");
                exit_status = 1;
            }
        }
    }

    return exit_status;
}
