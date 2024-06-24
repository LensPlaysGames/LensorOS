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
#include <bits/io_defs.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <utility>
#include <extensions>

/// ===========================================================================
///  File struct.
/// ===========================================================================
struct _IO_writebuf_t {
    char* __buf;
    _IO_size_t __offs;
    _IO_size_t __cap;
};

struct _IO_readbuf_t {
    char* __buf;
    _IO_size_t __start;
    _IO_size_t __offs;
    _IO_size_t __cap;
};

struct _IO_File {
    _IO_writebuf_t __wbuf{};
    _IO_readbuf_t __rdbuf{};
    _IO_lock_t __mutex{};
    _IO_fd_t __fd = -1;

    _IO_flags_t __f_buffering : 2;    /// Buffering mode.
    _IO_flags_t __f_error : 1;        /// Error indicator.
    _IO_flags_t __f_eof : 1;          /// End-of-file indicator.
    _IO_flags_t __f_has_ungotten : 1; /// Whether ungetc() has been called.
    _IO_flags_t : 11;                 /// Unused.

    char __ungotten{}; /// The character that ungetc() should insert.

    /// ===========================================================================
    ///  CDtors.
    /// ===========================================================================
private:
    /// Create a new FILE and allocate buffers for a file descriptor.
    /// This is private so we can make sure that no-one creates a dangling FILE.
    explicit _IO_File(_IO_fd_t fd = -1, Buffering buffering_mode = FullyBuffered);

public:
    /// List of open files.
    /// TODO: The big_file_lock and the vector should be wrapped into a single struct.
    static std::sparse_vector<_IO_File*, nullptr> open_files;
    static std::recursive_mutex big_file_lock;

    /// Disallow copying a FILE.
    _IO_File(const _IO_File&) = delete;
    _IO_File& operator=(const _IO_File&) = delete;

    /// Disallow moving a FILE (because of the mutex).
    _IO_File(_IO_File&&) = delete;
    _IO_File& operator=(_IO_File&&) = delete;

    ~_IO_File();

    /// ===========================================================================
    ///  Flags.
    /// ===========================================================================
    /// Check if a stream has reached EOF. This only checks if we're actually at
    /// the end of the file. There might be a character that is left in the stream
    /// because the user called ungetc(), so make sure to check that too to determine
    /// whether the stream is *actually* at EOF.
    _Nodiscard bool at_eof() const { return __f_eof; }

    /// Get the buffering mode of the stream.
    _Nodiscard Buffering buffering() const { return static_cast<Buffering>(__f_buffering); }

    /// Reset all flags.
    void clear_flags() {
        __f_buffering = 0;
        __f_error = 0;
        __f_eof = 0;
        __f_has_ungotten = 0;
    }

    /// Check if the error indicator is set.
    _Nodiscard bool has_error() const { return __f_error; }

    /// Check if unget() has been called on a stream.
    _Nodiscard bool has_ungotten() const { return __f_has_ungotten; }

    /// Check if unget() may be called on a stream.
    _Nodiscard bool may_unget() const { return !__f_has_ungotten; }

    /// ===========================================================================
    ///  Flushing, closing, and reassociating.
    /// ===========================================================================
    /// Clear the stream buffers.
    void clear_buffers();

    /// Close the stream.
    void close();

    /// Erase this file from the list of open files.
    /// This is an optimised version of std::sparse_vector::erase().
    void erase();

    /// Flush the stream.
    bool flush();

    /// Reassociate the stream with a new file descriptor.
    void reassociate(_IO_fd_t fd);

    /// Close all files.
    static void close_all();

    /// Open a new file
    static _IO_File* create(_IO_fd_t fd, Buffering buffering_mode = FullyBuffered);

    /// ===========================================================================
    ///  File manipulation.
    /// ===========================================================================
    /// Get the current position.
    int getpos(fpos_t& pos) const;

    /// Seet to an offset.
    int seek(_IO_off_t offset, int whence);

    /// Set the current position.
    int setpos(const fpos_t& pos);

    /// Get the current offset.
    auto tell(_IO_off_t& offset) const -> _IO_off_t;

    /// Push a character back into the input stream.
    int unget(char c);

    /// ===========================================================================
    ///  Reading
    /// ===========================================================================
    /// Read up to `size` bytes from `stream` into `buf`.
    /// \return The number of bytes read or EOF on error.
    ssize_t read(char* __restrict__ buf, const size_t size);


    /// Read up to `size` bytes from `stream` into `buf`. Stop if `until` is encountered.
    /// \return True if `until` was found or EOF was reached, false if there was an error
    ///         or if the stream is at EOF.
    bool read_until(char* __restrict__ buf, const size_t size, char until);

private:
    /// Copy data into a buffer from a stream’s read buffer.
    size_t copy_from_read_buffer(char* __restrict__ buf, size_t size);

    /// Copy data into a buffer from a stream’s read buffer up to and including `until` is found
    std::pair<size_t, bool> copy_until_from_read_buffer(
        char* __restrict__ buf,
        size_t rest,
        char until
    );

public:
    /// ===========================================================================
    ///  Writing.
    /// ===========================================================================
    /// Set the buffer size.
    bool setbuf(Buffering buffering, size_t size);

    /// Write a string to the stream.
    /// \return The number of characters written, or EOF on error.
    ssize_t write(const char* __restrict__ str, size_t sz);

    /// Write a character to the stream.
    bool write(char c);

private:
    /// Write to the stream. This function only flushes the buffer if it is full.
    /// \return The number of bytes written, or EOF on error.
    ssize_t write_internal(const char* __restrict__ buffer, size_t count);
};

#endif // _LENSOROS_LIBC_FILE_STRUCT_H
