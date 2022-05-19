#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/types.h>

#if defined (__cplusplus)
extern "C" {
#endif

    // Syscall declarations go here!

    int open(const char* path, int flags, int mode);
    void close(int fd);

    ssize_t read(int fd, const void* buffer, size_t count);
    ssize_t write(int fd, const void* buffer, size_t count);

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _UNISTD_H */
