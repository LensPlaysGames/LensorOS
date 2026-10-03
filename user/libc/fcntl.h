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

#ifndef _LENSOR_OS_LIBC_FCNTL_H
#define _LENSOR_OS_LIBC_FCNTL_H

#include <sys/types.h>

struct flock {
    short l_type;    // Type of lock; F_RDLCK, F_WRLCK, F_UNLCK.
    short l_whence;  // Flag for starting offset.
    off_t l_start;   //  Relative offset in bytes.
    off_t l_len;     // Size; if 0 then until EOF.
    pid_t l_pid;     // Process ID of the process holding the lock; returned with F_GETLK.
};

int creat(const char*, mode_t);
int fcntl(int, int, ...);
int open(const char*, int, ...);
int openat(int, const char*, int, ...);

#endif  // _LENSOR_OS_LIBC_FCNTL_H
