#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    int exit_status = 0;
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    
    if (argc < 2) {
        // No file arguments: use STDIN
        struct stat st;
        size_t blocksize;
        if (fstat(STDIN_FILENO, &st) == 0) {
            blocksize = st.st_blksize;
        } else {
            perror("fstat");
            blocksize = pagesize;
        }
        // Determine buffer size: start at 256 KiB, align up to page size and filesystem block size
        size_t base_size = 256 * 1024;  // 256 KiB
        size_t alignment = pagesize > blocksize ? pagesize : blocksize;
        if (base_size % alignment != 0) {
            base_size += alignment - (base_size % alignment);
        }
        size_t bufsize = base_size;
        
        void *vbuffer;
        if (posix_memalign(&vbuffer, pagesize, bufsize) != 0) {
            perror("posix_memalign");
            return 1;
        }
        char *buffer = vbuffer;
        
        // Read from STDIN and write to STDOUT
        while (1) {
            ssize_t bytes_read = read(STDIN_FILENO, buffer, bufsize);
            if (bytes_read == 0) {
                break; // EOF
            }
            if (bytes_read < 0) {
                if (errno == EINTR) continue;
                perror("read");
                free(buffer);
                return 1;
            }
            ssize_t bytes_written = 0;
            while (bytes_written < bytes_read) {
                ssize_t w = write(STDOUT_FILENO, buffer + bytes_written, bytes_read - bytes_written);
                if (w < 0) {
                    if (errno == EINTR) continue;
                    perror("write");
                    free(buffer);
                    return 1;
                }
                bytes_written += w;
            }
        }
        
        free(buffer);
    } else {
        for (int i = 1; i < argc; ++i) {
            const char *filename = argv[i];
            int fd;
            if (filename[0] == '-' && filename[1] == '\0') {
                fd = STDIN_FILENO;
            } else {
                fd = open(filename, O_RDONLY);
            }
            if (fd < 0) {
                perror(filename);
                exit_status = 1;
                continue;
            }
            
            // Get filesystem block size for the file
            struct stat st;
            size_t blocksize;
            if (fstat(fd, &st) == 0) {
                blocksize = st.st_blksize;
            } else {
                perror("fstat");
                blocksize = pagesize;
            }
            // Determine buffer size (256 KiB aligned to page size and filesystem block size)
            size_t base_size = 256 * 1024;
            size_t alignment = pagesize > blocksize ? pagesize : blocksize;
            if (base_size % alignment != 0) {
                base_size += alignment - (base_size % alignment);
            }
            size_t bufsize = base_size;
            
            void *vbuffer;
            if (posix_memalign(&vbuffer, pagesize, bufsize) != 0) {
                perror("posix_memalign");
                if (fd != STDIN_FILENO) close(fd);
                return 1;
            }
            char *buffer = vbuffer;
            
            // Read file and output to STDOUT
            while (1) {
                ssize_t bytes_read = read(fd, buffer, bufsize);
                if (bytes_read == 0) break;
                if (bytes_read < 0) {
                    if (errno == EINTR) continue;
                    perror("read");
                    exit_status = 1;
                    break;
                }
                ssize_t bytes_written = 0;
                while (bytes_written < bytes_read) {
                    ssize_t w = write(STDOUT_FILENO, buffer + bytes_written, bytes_read - bytes_written);
                    if (w < 0) {
                        if (errno == EINTR) continue;
                        perror("write");
                        free(buffer);
                        if (fd != STDIN_FILENO) close(fd);
                        return 1;
                    }
                    bytes_written += w;
                }
            }
            
            free(buffer);
            if (fd != STDIN_FILENO) {
                if (close(fd) < 0) {
                    perror("close");
                    exit_status = 1;
                }
            }
        }
    }
    
    return exit_status;
}
