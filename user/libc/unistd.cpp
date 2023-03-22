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
#include "stdlib.h"
#include "sys/syscalls.h"

extern "C" {
    int open(const char *path, int flags, int mode) {
        (void)flags;
        (void)mode;
        return syscall<int>(SYS_open, path);
    }

    /// FIXME: close() should return an int.
    void close(int fd) {
        /// TODO: check return value and set errno.
        syscall(SYS_close, fd);
    }

    ssize_t read(int fd, const void* buffer, size_t count) {
        /// TODO: check return value and set errno.
        int rc = 0;
        while ((rc = syscall<int>(SYS_read, fd, buffer, count)) == -2);
        return rc;
    }

    ssize_t write(int fd, const void* buffer, size_t count) {
        /// TODO: check return value and set errno.
        return syscall<ssize_t>(SYS_write, fd, buffer, count);
    }

    char *getcwd(char *buf, size_t size) {
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

    char *get_current_dir_name() {
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
}
