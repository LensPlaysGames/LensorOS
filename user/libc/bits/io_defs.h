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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#ifndef LENSOROS_IO_DEFS_H
#define LENSOROS_IO_DEFS_H

#include <bits/decls.h>
#include <stddef.h>
#include <stdint.h>

__BEGIN_DECLS__

/// These MUST be 0, 1, 2 because of how we handle them internally.
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

typedef int _IO_fd_t;
typedef size_t _IO_size_t;
typedef uint16_t _IO_flags_t;
typedef long _IO_off_t;

__END_DECLS__

#ifdef __cplusplus
#include <mutex>

/// Recursive mutex because flockfile() is a thing.
typedef std::recursive_mutex _IO_lock_t;

/// Buffering mode.
enum Buffering : _IO_flags_t {
    /// No buffering. Write the character(s) immediately.
    Unbuffered = _IONBF,

    /// Line buffering. Write the character(s) immediately if a newline is
    /// encountered. Otherwise, write to a buffer and only flush when the
    /// buffer is full or a newline is encountered.
    LineBuffered = _IOLBF,

    /// Full buffering. Write the character(s) to a buffer and only flush when
    /// the buffer is full.
    FullyBuffered = _IOFBF,
};

#endif

#endif  // LENSOROS_IO_DEFS_H
