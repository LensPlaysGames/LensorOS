//
// Created by ae on 02/11/22.
//

#ifndef LENSOROS_IO_DEFS_H
#define LENSOROS_IO_DEFS_H

#include <stdint.h>
#include <stddef.h>
#include <bits/decls.h>

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


#endif // LENSOROS_IO_DEFS_H
