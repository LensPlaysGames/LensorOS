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

#ifndef _LENSOROS_LIBC_FILE_STRUCT_H
#define _LENSOROS_LIBC_FILE_STRUCT_H

#ifndef __cplusplus
#    error "This header is C++ only."
#endif

#include <bits/decls.h>
#include <stddef.h>
#include <stdint.h>
#include <mutex>

/// ===========================================================================
///  File struct.
/// ===========================================================================
__BEGIN_DECLS__
typedef int _IO_fd_t;
typedef size_t _IO_size_t;
typedef uint16_t _IO_flags_t;

/// Recursive mutex because flockfile() is a thing.
typedef std::recursive_mutex _IO_lock_t;

typedef struct _IO_writebuf_t {
    char* __buf;
    _IO_size_t __offs;
    _IO_size_t __cap;
} _IO_writebuf_t;

typedef struct _IO_readbuf_t {
    char* __buf;
    _IO_size_t __start;
    _IO_size_t __offs;
    _IO_size_t __cap;
} _IO_readbuf_t;

struct _IO_File {
    _IO_writebuf_t __wbuf{};
    _IO_readbuf_t __rdbuf{};
    _IO_File* __next{};
    _IO_File* __prev{};
    _IO_lock_t __mutex{};
    _IO_fd_t __fd = -1;

    _IO_flags_t __f_buffering : 2;    /// Buffering mode.
    _IO_flags_t __f_error : 1;        /// Error indicator.
    _IO_flags_t __f_eof : 1;          /// End-of-file indicator.
    _IO_flags_t __f_has_ungotten : 1; /// Whether ungetc() has been called.
    _IO_flags_t __f_unused : 11;      /// Unused.

    char __ungotten{}; /// The character that ungetc() should insert.

    _IO_File() {
        __f_buffering = 0;
        __f_error = 0;
        __f_eof = 0;
        __f_has_ungotten = 0;
        __f_unused = 0;
    }
};
__END_DECLS__

#endif // _LENSOROS_LIBC_FILE_STRUCT_H
