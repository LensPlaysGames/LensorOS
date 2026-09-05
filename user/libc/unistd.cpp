/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>.
 */

#include "unistd.h"

#include "errno.h"
#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/syscalls.h"

extern "C" {

int open(const char* path, int flags, int mode) {
    (void)flags;
    (void)mode;
    return syscall<int>(SYS_open, path);
}

int close(int fd) {
    /// TODO: check return value and set errno.
    syscall(SYS_close, fd);
    return 0;
}

ssize_t read(int fd, const void* buffer, size_t count) {
    /// TODO: check return value and set errno.
    int rc = 0;
    while ((rc = syscall<int>(SYS_read, fd, buffer, count, LENSOROS_SYSCALL_READ_FLAG_NONE)) == -2);
    return rc;
}

ssize_t write(int fd, const void* buffer, size_t count) {
    /// TODO: check return value and set errno.
    ssize_t rc = 0;
    while ((rc = syscall<ssize_t>(SYS_write, fd, buffer, count, LENSOROS_SYSCALL_WRITE_FLAG_NONE)) == -2);
    return rc;
}

int pipe(int out[2]) {
    size_t fds[2] = {size_t(-1), size_t(-1)};

    syscall(SYS_pipe, fds);
    // TODO: Error checking

    if (fds[0] == size_t(-1) or fds[1] == size_t(-1)) {
        errno = EFAULT;
        return -1;
    }

    // TODO: How bad is this cast?
    out[0] = (int)fds[0];
    out[1] = (int)fds[1];
    return 0;
}

int dup2(int oldfd, int newfd) {
    syscall(SYS_repfd, oldfd, newfd);
    return newfd;
}

pid_t fork(void) {
    // Flush all libc file buffers. This is necessary because, as
    // you can imagine, it isn't ideal when the two processes after
    // the fork syscall flush the buffers with data from before the
    // fork; this means /two/ processes end up using the data
    // provided before the fork by one process. Basically, this
    // ensures you don't see "hello world" twice after a fork.
    fflush(NULL);

    pid_t child_pid = syscall(SYS_fork);
    // TODO: Currently, the `fork` syscall "can't" fail. As in, the
    // kernel implementation doesn't check for failures or return
    // values indicating them. If we /do/ get an error-indicating
    // return value, we should set errno to EAGAIN or ENOMEM,
    // depending on the failure case.
    return child_pid;
}

pid_t execv(const char* file, char* const argv[]) {
#ifdef __lensor__
    // LensorOS exec syscall adds executable filepath argument itself, but
    // linux style exec* functions require it to be there already.
    argv += 1;
#endif
    syscall(SYS_exec, file, argv);
    // Obviously this will never happen...
    return 0;
}

char* getcwd(char* buf, size_t size) {
    if (!size) {
        errno = EINVAL;
        return NULL;
    }
    if (!buf) return buf;
    buf[0] = '\0';
    // On failure, return NULL, and errno is set to indicate the
    // error. The contents of the array pointed to by buf are
    // undefined on error.
    if (!syscall(SYS_pwd, buf, size)) {
        // ERANGE
        //   The size argument is less than the length of the
        //   absolute pathname of the working directory, including the
        //   terminating null byte.  You need to allocate a bigger
        //   array and try again.
        errno = ERANGE;
        return NULL;
    }
    return buf;
}

char* get_current_dir_name() {
    size_t length = 128;
    char* buf = (char*)malloc(128);
    if (!buf) {
        errno = ENOMEM;
        return NULL;
    }
    buf[0] = '\0';
    while (!syscall(SYS_pwd, buf, length)) {
        length *= 2;
        if (length >= 4096) {
            free(buf);
            errno = ERANGE;
            return NULL;
        }
        char* new_buf = (char*)realloc(buf, length);
        if (!new_buf) {
            free(buf);
            errno = ENOMEM;
            return NULL;
        }
    }
    return buf;
}

}  // extern "C"
