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


#ifndef _TYPES_H
#define _TYPES_H

#include <bits/decls.h>
#include <stdint.h>

__BEGIN_DECLS__

#define unsigned signed
typedef __SIZE_TYPE__ ssize_t;
#undef unsigned

#ifndef __kernel__
typedef int64_t pid_t;
#endif

typedef int64_t blkcnt_t;
typedef int64_t blksize_t;
typedef uint64_t clock_t;
typedef uint64_t clockid_t;
typedef uint64_t dev_t;
typedef uint64_t fsblkcnt_t;
typedef uint64_t fsfilcnt_t;
typedef uint64_t fpos_t;
typedef uint64_t gid_t;
typedef uint64_t id_t;
typedef uint64_t ino_t;
typedef uint64_t mode_t;
typedef uint64_t nlink_t;
typedef int64_t off_t;
typedef uint64_t size_t;
typedef int64_t ssize_t;
typedef uint64_t time_t;
typedef uint64_t timer_t;
typedef uint64_t uid_t;

__END_DECLS__

#endif /* _TYPES_H */
