#include "unistd.h"

#include "sys/syscalls.h"

extern "C" {
    int open(const char *path, int flags, int mode) {
        (void)flags;
        (void)mode;
        return syscall(SYS_open, path);
    }

    void close(int fd) {
        syscall(SYS_close, fd);
    }

    ssize_t read(int fd, const void* buffer, size_t count) {
        return syscall(SYS_read, fd, buffer, count);
    }

    ssize_t write(int fd, const void* buffer, size_t count) {
        return syscall(SYS_write, fd, buffer, count);
    }

    __attribute__((noreturn)) void exit(int status) {
        write(STDOUT_FILENO, "EXITING!\r\n", 10);
        syscall(SYS_exit, status);
        while (1);
    }
}
