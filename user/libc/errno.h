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

#ifndef _ERRNO_H
#define _ERRNO_H

#if defined (__cplusplus)
extern "C" {
#endif

extern int* __errno_location(void);

#define errno (*__errno_location())

#define EPERM        1  /* Not permitted */
#define ENOENT       2  /* No file (or directory) */
#define ESRCH        3  /* Process doesn't exist */
#define EINTR        4  /* System call interrupted */
#define EIO          5  /* Input/Output error */
#define ENXIO        6  /* No device (or address) */
#define E2BIG        7  /* Too many arguments */
#define ENOEXEC      8  /* `exec` syscall (format) error */
#define EBADF        9  /* Bad file */
#define ECHILD      10  /* No child processes exist */
#define EAGAIN      11  /* If this is returned, try again */
#define ENOMEM      12  /* No memory */
#define EACCES      13  /* Access violation */
#define EFAULT      14  /* Bad address */
#define ENOTBLK     15  /* Not a block device, but a block device is required */
#define EBUSY       16  /* Wait until later, thing is busy right now */
#define EEXIST      17  /* File does exist */
#define EXDEV       18  /* Device cross-link */
#define ENODEV      19  /* No device */
#define ENOTDIR     20  /* Not a directory */
#define EISDIR      21  /* Is a directory */
#define EINVAL      22  /* Invalid (argument) */
#define ENFILE      23  /* File table overflow */
#define EMFILE      24  /* Too many open files */
#define ENOTTY      25  /* No teletype (not a typewriter) */
#define ETXTBSY     26  /* Text file is busy */
#define EFBIG       27  /* File is too big */
#define ENOSPC      28  /* No space left on the device */
#define ESPIPE      29  /* Illegal seek */
#define EROFS       30  /* Read-only file system */
#define EMLINK      31  /* Too many links */
#define EPIPE       32  /* Broken pipe */
#define EDOM        33  /* Math argument out of domain of func */
#define ERANGE      34  /* Math result not representable */

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _ERRNO_H */
