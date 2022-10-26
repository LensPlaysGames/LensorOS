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


#ifndef _UNISTD_H
#define _UNISTD_H

#include "sys/types.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define STDOUT_FILENO 0

    int open(const char* path, int flags, int mode);
    void close(int fd);

    ssize_t read(int fd, const void* buffer, size_t count);
    ssize_t write(int fd, const void* buffer, size_t count);

  __attribute__((noreturn)) void exit(int status);

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _UNISTD_H */
