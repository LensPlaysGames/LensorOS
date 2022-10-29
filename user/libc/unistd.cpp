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

#include "sys/syscalls.h"

extern "C" {
    int open(const char *path, int flags, int mode) {
        (void)flags;
        (void)mode;
        return syscall(SYS_open, path);
    }

    /// FIXME: close() should return an int.
    void close(int fd) {
        /// TODO: check return value and set errno.
        syscall(SYS_close, fd);
    }

    ssize_t read(int fd, const void* buffer, size_t count) {
        /// TODO: check return value and set errno.
        return syscall(SYS_read, fd, buffer, count);
    }

    ssize_t write(int fd, const void* buffer, size_t count) {
        /// TODO: check return value and set errno.
        return syscall(SYS_write, fd, buffer, count);
    }

    __attribute__((noreturn)) void exit(int status) {
        write(STDOUT_FILENO, "EXITING!\r\n", 10);
        syscall(SYS_exit, status);
        while (1);
    }
}
