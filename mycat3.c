#define _POSIX_C_SOURCE 200112L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static size_t page_size(void)
{
    long ps = sysconf(_SC_PAGESIZE);
    return ps > 0 ? (size_t)ps : 4096;
}

static void *align_alloc(size_t size)
{
    void *ptr = NULL;
    if (posix_memalign(&ptr, page_size(), size) != 0) return NULL;
    return ptr;
}
static void align_free(void *p) { free(p); }

int main(int argc, char *argv[])
{
    const size_t BUF_SZ = 256 * 1024;          /* 与 cat2 相同 */
    size_t ps  = page_size();
    size_t sz  = ((BUF_SZ + ps - 1) / ps) * ps;/* 向上页对齐 */

    int fd = (argc > 1) ? open(argv[1], O_RDONLY) : STDIN_FILENO;
    if (fd < 0) { perror("open"); return 1; }

    char *buf = align_alloc(sz);
    if (!buf) { perror("align_alloc"); if (fd!=STDIN_FILENO) close(fd); return 1; }

    ssize_t n;
    while ((n = read(fd, buf, sz)) > 0) {
        char *p = buf;
        while (n) {
            ssize_t m = write(STDOUT_FILENO, p, (size_t)n);
            if (m < 0) { perror("write"); align_free(buf); if(fd!=STDIN_FILENO) close(fd); return 1; }
            p += m; n -= m;
        }
    }
    if (n < 0) perror("read");

    align_free(buf);
    if (fd != STDIN_FILENO) close(fd);
    return n < 0;
}
