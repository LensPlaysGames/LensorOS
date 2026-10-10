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

#include <bits/decls.h>
#include <sys/types.h>

__BEGIN_DECLS__

struct flock {
    short l_type;    // Type of lock; F_RDLCK, F_WRLCK, F_UNLCK.
    short l_whence;  // Flag for starting offset.
    off_t l_start;   //  Relative offset in bytes.
    off_t l_len;     // Size; if 0 then until EOF.
    pid_t l_pid;     // Process ID of the process holding the lock; returned with F_GETLK.
};

int creat(const char*, mode_t);

#define F_SETFD 0
#define F_GETFD 0
int fcntl(int, int, ...);

#define O_APPEND 0
#define O_RDONLY 0 /**< Read Only **/
#define O_WRONLY 0 /**< Write Only **/
#define O_ASYNC 0
#define O_CLOEXEC 0
#define O_CREAT 0 /**< Allow Creation of New File **/
#define O_DIRECT 0
#define O_DIRECTORY 0
#define O_DSYNC 0
#define O_EXCL 0
#define O_NOCTTY 0
#define O_NOFOLLOW 0
#define O_TMPFILE 0
#define O_TRUNC 0
int open(const char* path, int flags, int mode);
int openat(int, const char*, int, ...);

__END_DECLS__

#endif  // _LENSOR_OS_LIBC_FCNTL_H
