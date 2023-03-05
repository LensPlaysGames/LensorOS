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

#include <bits/decls.h>
#include "sys/types.h"

__BEGIN_DECLS__

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int open(const char* path, int flags, int mode);
void close(int fd);

ssize_t read(int fd, const void* buffer, size_t count);
ssize_t write(int fd, const void* buffer, size_t count);

/// On success, `buf` will be filled with the absolute path of the
/// current process' working directory.
/// On failure, return NULL, and errno is set to indicate the
/// error. The contents of the array pointed to by buf are
/// undefined on error.
///
/// EINVAL
///   The size argument is 0.
///
/// ERANGE
///   The size argument is less than the length of the
///   absolute pathname of the working directory, including the
///   terminating null byte.  You need to allocate a bigger
///   array and try again.
char *getcwd(char *buf, size_t size);

/// On success, return a pointer to a buffer containing the current
/// process' working directory. The caller should free() the returned
/// buffer.
/// On failure, return NULL, and errno is set to indicate the
/// error.
///
/// ERANGE
///   The working directory is longer than the maximum supported by this function.
///   Likely indicates internal error.
///
/// ERANGE
///   The working directory is longer than the maximum supported by this function.
///   Likely indicates internal error.
///
/// ENOMEM
///   Memory allocation has failed (malloc()/realloc() returned NULL).
char *get_current_dir_name(void);

__END_DECLS__

#endif /* _UNISTD_H */
