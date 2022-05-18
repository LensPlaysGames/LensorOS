#include <unistd.h>

#include <sys/syscalls.h>

int open(const char *path, int flags, int mode) {
    (void)flags;
    (void)mode;
    return syscall(SYS_open, path);
}

int open(int fd) {
    return syscall(SYS_close, fd);
}

ssize_t read(int fd, const void* buffer, size_t count) {
    return syscall(SYS_read, fd, buffer, count);
}

ssize_t write(int fd, const void* buffer, size_t count) {
    return syscall(SYS_write, fd, buffer, count);
}
