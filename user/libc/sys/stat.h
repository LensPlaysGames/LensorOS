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

#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <bits/decls.h>
#include <sys/types.h>
#include <time.h>

__BEGIN_DECLS__

struct stat {
    dev_t st_dev;      // Device ID of device containing file.
    ino_t st_ino;      // File serial number.
    mode_t st_mode;    // Mode of file (see below).
    nlink_t st_nlink;  // Number of hard links to the file.
    uid_t st_uid;      // User ID of file.
    gid_t st_gid;      // Group ID of file.
    dev_t st_rdev;     // Device ID (if file is character or block special).

    // For regular files, the file size in bytes.
    // For symbolic links, the length in bytes of the pathname contained in the symbolic link.
    // For a shared memory object, the length in bytes.
    // For a typed memory object, the length in bytes.
    // For other file types, the use of this field is unspecified.
    off_t st_size;

    struct timespec st_atim;  // Last data access timestamp.
    struct timespec st_mtim;  // Last data modification timestamp.
    struct timespec st_ctim;  // Last file status change timestamp.

    // A file system-specific preferred I/O block size for this object. In
    // some file system types, this may vary from file to file.
    blksize_t st_blksize;
    blkcnt_t st_blocks;  // Number of blocks allocated for this object.
};

int chmod(const char*, mode_t);
int fchmod(int, mode_t);
int fchmodat(int, const char*, mode_t, int);
int fstat(int, struct stat*);
int fstatat(int, const char* _Restrict, struct stat* _Restrict, int);
int futimens(int, const struct timespec[2]);
int lstat(const char* _Restrict, struct stat* _Restrict);
int mkdir(const char*, mode_t);
int mkdirat(int, const char*, mode_t);
int mkfifo(const char*, mode_t);
int mkfifoat(int, const char*, mode_t);
int mknod(const char*, mode_t, dev_t);
int mknodat(int, const char*, mode_t, dev_t);
int stat(const char* _Restrict, struct stat* _Restrict);
mode_t umask(mode_t);
int utimensat(int, const char*, const struct timespec[2], int);

__END_DECLS__

#endif
