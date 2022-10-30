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

#include "stdio.h"

#include "bits/std/algorithm"
#include "bits/std/atomic"
#include "bits/std/mutex"
#include "bits/std/new"
#include "bits/std/utility"
#include "bits/stub.h"
#include "errno.h"
#include "stdarg.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

enum : size_t {
    BUFFERED_READ_THRESHOLD = 64,
};

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

/// ===========================================================================
///  File I/O Implementation.
/// ===========================================================================
/// The functions and data structures in this namespace implement stdio file streams.
///
/// In order to make sure that there are no deadlocks or data races in multithreaded
/// code, the following invariants must hold at all times with respect to the stream
/// lock in each I/O stream:
///
///   - Except for stream_delete(), no function in this namespace may lock a stream.
///   - No function in this namespace may call a C stdio API function.
///   - Any C stdio API function that calls a function in this namespace on a stream
///     must lock the stream before calling any function in this namespace, except for
///     stream_delete().
namespace {
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

/// ===========================================================================
///  Initialisation.
/// ===========================================================================
_IO_File* open_files;
std::mutex big_file_lock;

/// Forward decls for init()/fini().
FILE* stream_create(_IO_fd_t fd);
void stream_create(FILE* stream, _IO_fd_t fd);
void stream_delete(FILE&& stream);

/// Initialise the standard streams.
[[gnu::constructor(1)]] void init() {
    /// TODO: stdin and stdout are fully buffered at program startup; stderr is not fully buffered.
    stdout = stream_create(STDOUT_FILENO);

    /// FIXME: These should be separate streams eventually.
    open_files = stdin = stderr = stdout;
}

/// Flush the streams on exit.
[[gnu::destructor(1)]] void fini() {
    /// Flush all open streams and close them.
    while (open_files) {
        auto f = open_files;
        open_files = open_files->__next;
        stream_delete(std::move(*f));
    }
}

/// ===========================================================================
///  Implementation.
/// ===========================================================================
/// RAII helper to lock a stream.
struct [[nodiscard]] Lock {
    std::unique_lock<_IO_lock_t> lock;
    explicit Lock(FILE& stream) : lock(stream.__mutex) {}
};

/// Check how a stream is buffered.
Buffering stream_buffering(FILE& stream) {
    return Buffering(stream.__f_buffering);
}

/// Check if a stream has reached EOF. This only checks if we're actually at
/// the end of the file. There might be a character that is left in the stream
/// because the user called ungetc(), so make sure to check that too to determine
/// whether the stream is *actually* at EOF.
bool stream_at_eof(FILE& stream) {
    return stream.__f_eof;
}

/// Check if unget() may be called on a stream.
bool stream_may_unget(FILE& stream) {
    return !stream.__f_has_ungotten;
}

/// Check if unget() has been called on a stream.
bool stream_has_ungotten(FILE& stream) {
    return stream.__f_has_ungotten;
}

/// ===========================================================================
///  Stream creation and destruction.
/// ===========================================================================
/// Create a new stream.
void stream_create(FILE* stream, _IO_fd_t fd) {
    /// Write buffer.
    ///
    /// We use `malloc()` instead of `new` for the buffers because we might
    /// need to `realloc()` them later on.
    /// TODO: Only allocate a write buffer if the stream is open for writing.
    stream->__wbuf.__buf = static_cast<char*>(malloc(BUFSIZ));
    stream->__wbuf.__cap = BUFSIZ;
    stream->__wbuf.__offs = 0;

    /// Read buffer.
    /// TODO: Only allocate a read buffer if the stream is open for reading.
    stream->__rdbuf.__buf = static_cast<char*>(malloc(BUFSIZ));
    stream->__rdbuf.__cap = BUFSIZ;
    stream->__rdbuf.__start = 0;
    stream->__rdbuf.__offs = 0;

    /// File descriptor.
    stream->__fd = fd;

    /// Flags.
    stream->__f_buffering = FullyBuffered;

    /// Add to the list of open files.
    std::unique_lock file_list_lock{big_file_lock};
    auto old_head = open_files;
    stream->__next = old_head;
    open_files = stream;

    /// If there was an old head, update its previous pointer.
    if (old_head) {
        Lock lock{*old_head};
        old_head->__prev = stream;
    }
}

/// Create a new stream.
FILE* stream_create(_IO_fd_t fd) {
    auto stream = new FILE;
    stream_create(stream, fd);
    return stream;
}

/// Flush an output stream.
bool stream_flush(FILE& stream);

/// Close and delete.
void stream_delete(FILE&& stream) {
    /// Remove from the list of open files.
    std::unique_lock file_list_lock{big_file_lock};
    Lock lock{stream};
    if (stream.__prev) {
        Lock lock2{*stream.__prev};
        stream.__prev->__next = stream.__next;
    }
    if (stream.__next) {
        Lock lock2{*stream.__next};
        stream.__next->__prev = stream.__prev;
    }
    if (open_files == &stream) {
        open_files = stream.__next;
    }

    /// Flush the stream.
    stream_flush(stream);

    /// Close the file.
    close(stream.__fd);

    /// Delete the buffers.
    free(stream.__wbuf.__buf);
    free(stream.__rdbuf.__buf);

    /// Delete the stream.
    delete &stream;
}

/// Clear the buffers of a stream.
void stream_clear_buffers(FILE& stream) {
    stream.__wbuf.__offs = 0;
    stream.__rdbuf.__start = 0;
    stream.__rdbuf.__offs = 0;
}

/// Clear the flags of a stream.
void stream_clear_flags(FILE& stream) {
    stream.__f_eof = false;
    stream.__f_error = false;
    stream.__f_has_ungotten = false;
}

/// Close the stream and clear the buffers, but don't delete the stream.
void stream_close(FILE& stream) {
    /// Flush and close the stream.
    if (stream.__fd != -1) {
        stream_flush(stream);
        close(stream.__fd);
    }

    /// Clear the file descriptor.
    stream.__fd = -1;

    /// Reset the stream.
    stream_clear_buffers(stream);
    stream_clear_flags(stream);
}

/// Set the file descriptor for a stream.
void stream_reassociate(FILE& stream, _IO_fd_t fd) {
    stream_close(stream);
    stream.__fd = fd;
}

/// ===========================================================================
///  File manipulation.
/// ===========================================================================
int stream_unget(FILE& stream, char c) {
    /// TODO: unget() isn't possible if the stream isn't open for reading.
    if (!stream_may_unget(stream)) { return EOF; }

    stream.__f_has_ungotten = true;
    stream.__ungotten = c;

    return c;
}

int stream_getpos(FILE& stream, fpos_t& pos) {
    /// TODO: Implement.
    (void)stream;
    (void)pos;
    return -1;
}

int stream_seek(FILE& stream, long offset, int whence) {
    /// TODO: Implement.
    /// TODO: This has to undo unget()s.
    (void)stream;
    (void)offset;
    (void)whence;
    return -1;
}

int stream_setpos(FILE& stream, const fpos_t& pos) {
    /// TODO: Implement.
    /// TODO: This has to undo unget()s.
    (void)stream;
    (void)pos;
    return -1;
}

long stream_tell(FILE& stream, long& offset) {
    /// TODO: Implement.
    (void)stream;
    (void)offset;
    return -1;
}

/// ===========================================================================
///  Reading.
/// ===========================================================================
/// Copy data into a buffer from a stream’s read buffer.
size_t stream_copy_from_read_buffer(FILE& __restrict__ stream, char* __restrict__ buf, size_t size) {
    /// We can copy at most __offs - __start many bytes.
    size_t stream_rem = stream.__rdbuf.__offs - stream.__rdbuf.__start;

    /// Copy the data.
    size_t to_copy = std::min(size, stream_rem);
    memcpy(buf, stream.__rdbuf.__buf, to_copy);

    /// If we've copied all the data, reset the buffer.
    if (stream_rem == to_copy) { stream.__rdbuf.__start = stream.__rdbuf.__offs = 0; }

    /// Otherwise, move the start pointer.
    else { stream.__rdbuf.__start += to_copy; }

    /// Return the number of bytes copied.
    return to_copy;
}

/// Read up to `size` bytes from `stream` into `buf`.
/// TODO: Check if the stream is readable.
/// \return The number of bytes read or EOF on error.
ssize_t stream_read(FILE& __restrict__ stream, char* __restrict__ buf, const size_t size) {
    /// If we have an ungotten character, store that in the buffer first.
    size_t rest = size;
    if (stream_has_ungotten(stream)) {
        buf[0] = stream.__ungotten;
        stream.__f_has_ungotten = false;
        buf++;
        rest--;
    }

    /// Copy data from the read buffer.
    if (stream.__rdbuf.__offs > stream.__rdbuf.__start) {
        auto copied = stream_copy_from_read_buffer(stream, buf, rest);

        /// If we've copied all the data, return.
        if (copied == rest) { return size; }
    }

    /// Return if we're at end of file.
    if (stream_at_eof(stream)) { return EOF; }

    /// Read from the file descriptor.
    ///
    /// If we ever get here, then the read buffer is empty.
    ///
    /// If the rest of the data that we need to read is small, read a block into
    /// the read buffer and copy the data.
    ///
    /// Otherwise, read it directly into the destination buffer.
    if (rest < BUFFERED_READ_THRESHOLD) {
        ssize_t n_read = read(stream.__fd, stream.__rdbuf.__buf, stream.__rdbuf.__cap);
        if (n_read == -1) {
            stream.__f_error = true;
            return EOF;
        }

        /// If we've reached end of file, set the flag.
        if (n_read == 0) { stream.__f_eof = true; }
        stream.__rdbuf.__offs = n_read;

        /// Copy the data.
        auto copied = stream_copy_from_read_buffer(stream, buf, rest);

        /// We've copied all the data.
        if (copied == rest) { return ssize_t(size); }

        /// We've reached end of file.
        return ssize_t(size - rest + copied);
    } else {
        auto n_read = read(stream.__fd, buf, rest);
        if (n_read == -1) {
            stream.__f_error = true;
            return EOF;
        }

        /// If we've reached end of file, set the flag.
        if (n_read == 0) { stream.__f_eof = true; }
        return ssize_t(size - rest + n_read);
    }
}

/// Copy data into a buffer from a stream’s read buffer up to and including `until` is found
std::pair<size_t, bool> stream_copy_until_from_read_buffer(
    FILE& __restrict__ stream,
    char* __restrict__ buf,
    const size_t rest,
    char until
) {
    /// Find the first occurrence of `until` in the read buffer.
    auto* first = (char*) memchr(
        stream.__rdbuf.__buf + stream.__rdbuf.__start,
        until,
        stream.__rdbuf.__offs - stream.__rdbuf.__start
    );

    /// Copy everything up to and including `until`, or the size of the buffer
    /// if `until` was not found.
    auto max_copy = first ? size_t(first - stream.__rdbuf.__start + 1) : stream.__rdbuf.__offs;
    auto to_copy = std::min(rest, max_copy);

    /// Copy the data.
    auto copied = stream_copy_from_read_buffer(stream, buf, to_copy);

    /// If we've filled the buffer or found `until`, we're done.
    if (first || copied == rest) { return {copied, true}; }
    return {copied, false};
}

/// Read up to `size` bytes from `stream` into `buf`. Stop if `until` is encountered.
/// TODO: Check if the stream is readable.
/// \return True if `until` was found or EOF was reached, false if there was an error
///         or if the stream is at EOF.
bool stream_read_until(FILE& __restrict__ stream, char* __restrict__ buf, const size_t size, char until) {
    /// If we have an ungotten character, store that in the buffer first.
    size_t rest = size;
    if (stream_has_ungotten(stream)) {
        buf[0] = stream.__ungotten;
        stream.__f_has_ungotten = false;
        buf++;
        rest--;
        if (stream.__ungotten == until) { return true; }
    }

    /// Copy data from the read buffer.
    if (stream.__rdbuf.__offs > stream.__rdbuf.__start) {
        auto [copied, done] = stream_copy_until_from_read_buffer(stream, buf, rest, until);
        if (done || stream_at_eof(stream)) return true;
        rest -= copied;
    }

    /// Read from the file descriptor.
    /// If we ever get here, then the read buffer is empty.
    while (!stream_at_eof(stream)) {
        ssize_t n_read = read(stream.__fd, stream.__rdbuf.__buf, stream.__rdbuf.__cap);
        if (n_read == -1) {
            stream.__f_error = true;
            return false;
        }

        /// If we've reached end of file, set the flag.
        if (n_read == 0) { stream.__f_eof = true; }
        stream.__rdbuf.__offs = n_read;

        /// Copy the data.
        auto [copied, done] = stream_copy_until_from_read_buffer(stream, buf, rest, until);
        if (done) return true;
    }

    return false;
}

/// ===========================================================================
///  Writing.
/// ===========================================================================
/// Flush a stream.
bool stream_flush(FILE& stream) {
    if (stream.__wbuf.__offs == 0) { return true; }

    /// Write the data.
    ssize_t written = write(stream.__fd, stream.__wbuf.__buf, stream.__wbuf.__offs);

    /// Check for errors.
    if (written < 0 || size_t(written) != stream.__wbuf.__offs) {
        stream.__f_error = true;
        return false;
    }

    /// Clear the buffer.
    stream.__wbuf.__offs = 0;
    return true;
}

/// Set the buffering mode of a stream.
bool stream_set_buffering(FILE& stream, Buffering buffering, char*, size_t size) {
    /// Flush the stream.
    if (!stream_flush(stream)) return false;

    /// Set the buffer.
    switch (buffering) {
        case Unbuffered:
            /// We don't free the buffer here because the read buffer may be
            /// non-empty, and we don't want to lose the data.
            stream.__f_buffering = Unbuffered;
            return true;

        case LineBuffered:
        case FullyBuffered:
            _IgnoreWarning("-Wconversion", stream.__f_buffering = buffering);

            /// According to the standard, `size` is only supposed to be a hint
            /// if `buffer` is nullptr, so we should ignore it if it's too small.
            if (size < BUFSIZ) size = BUFSIZ;

            /// We never use the buffer that the user provided, because that would
            /// be too much of a hassle.
            ///
            /// It's legal to do so because the standard specifies that the contents
            /// of the user-supplied buffer are implementation-defined, so we can
            /// just choose to not use them. The standard doesn't say that we are
            /// not allowed to allocate memory for the buffer ourselves. ;Þ

            /// Realloc the write buffer. We've already flushed it, so there's no
            /// reason to copy its contents.
            if (stream.__wbuf.__cap != size) {
                stream.__wbuf.__cap = size;
                stream.__wbuf.__buf = (char*) __mextend(stream.__wbuf.__buf, size);
            }

            /// Realloc the read buffer. Make sure we don't lose any data.
            if (stream.__rdbuf.__cap != size) {
                /// Buffer is empty; just extend it.
                if (!stream.__rdbuf.__offs) {
                    stream.__rdbuf.__cap = size;
                    stream.__rdbuf.__buf = (char*) __mextend(stream.__rdbuf.__buf, size);
                }

                /// Buffer has data; make sure we don't lose any of it.
                else {
                    stream.__rdbuf.__cap = std::max(stream.__rdbuf.__offs, size);
                    stream.__rdbuf.__buf = (char*) realloc(stream.__rdbuf.__buf, stream.__rdbuf.__cap);
                }
            }

            return true;

        /// Invalid buffering mode.
        default: return false;
    }
}

/// Write to a stream. This function only flushes the buffer if it is full.
/// TODO: Check if the stream is open for writing.
/// \return The number of bytes written, or EOF on error.
ssize_t stream_put(FILE& __restrict__ stream, const char* __restrict__ buffer, size_t count) {
    /// Flush the buffer if this operation would overflow it.
    if (stream.__wbuf.__offs + count > stream.__wbuf.__cap && !stream_flush(stream)) { return EOF; }

    /// Write data directly to the stream if it doesn't fit in the buffer.
    if (count > stream.__wbuf.__cap) {
        auto written = write(stream.__fd, buffer, count);
        if (written < 0 || size_t(written) != count) {
            stream.__f_error = true;
            return EOF;
        }

        return ssize_t(count);
    }

    /// Write data to the buffer.
    memcpy(stream.__wbuf.__buf + stream.__wbuf.__offs, buffer, count);
    stream.__wbuf.__offs += count;
    return ssize_t(count);
}

/// Write a string to a stream. This function does not lock the stream.
/// \return The number of characters written, or EOF on error.
ssize_t stream_put_maybe_buffered(FILE& __restrict__ stream, const char* __restrict__ str, size_t sz) {
    switch (stream_buffering(stream)) {
        case Unbuffered: {
            /// TODO: Check if the stream is open for writing.
            auto written = write(stream.__fd, str, sz);
            if (written < 0 || size_t(written) != sz) {
                stream.__f_error = true;
                return EOF;
            }

            return ssize_t(sz);
        }

        case LineBuffered: {
            ssize_t written{};

            /// Write up to the next newline and flush.
            while (auto nl = static_cast<char*>(memchr(str, '\n', sz))) {
                /// Write to the stream.
                auto nl_sz = nl - str + 1;
                auto n_put = stream_put(stream, str, nl_sz);
                if (n_put != nl_sz) return written;

                /// Discard the data up to and including the newline.
                str += nl_sz;
                sz -= nl_sz;

                /// Flush the stream.
                if (!stream_flush(stream)) return written;
                written += n_put;
            }

            /// Write the rest.
            written += stream_put(stream, str, sz);
            return written;
        }

        case FullyBuffered: {
            return stream_put(stream, str, sz);
        }

        /// Invalid buffering mode. Should be unreachable.
        default: return EOF;
    }
}

/// Write a character to a stream. This function does not lock the stream.
bool stream_put_maybe_buffered(FILE& stream, char c) {
    return stream_put_maybe_buffered(stream, &c, 1) == 1;
}

} // namespace

/// ===========================================================================
///  C Interface.
/// ===========================================================================
__BEGIN_DECLS__

FILE* stdin;
FILE* stdout;
FILE* stderr;

/// ===========================================================================
///  7.21.4 Operations on files.
/// ===========================================================================
int remove(const char*) {
    _LIBC_STUB();
}

int rename(const char*, const char*) {
    _LIBC_STUB();
}

FILE* tmpfile(void) {
    _LIBC_STUB();
}

char* tmpnam(char*) {
    _LIBC_STUB();
}

/// ===========================================================================
///  7.21.5 File access functions.
/// ===========================================================================
int fclose(FILE* stream) {
    /// Do NOT lock the stream here.
    stream_delete(std::move(*stream));
    return 0;
}

int fflush(FILE* stream) {
    /// Flush all streams if `stream` is nullptr.
    if (!stream) {
        std::unique_lock file_list_lock{big_file_lock};
        auto next = open_files;
        while (next) {
            if (next->__f_buffering != Unbuffered) {
                Lock lock{*next};
                if (!stream_flush(*next)) return EOF;
            }

            next = next->__next;
        }
        return 0;
    }

    Lock lock{*stream};
    if (!stream_flush(*stream)) return EOF;

    return 0;
}

FILE* fopen(const char* __restrict__ filename, const char* __restrict__ mode) {
    /// TODO: Parse mode.
    (void)mode;

    /// TODO: Allocate a new file and buffer.
    /// TODO: Parse mode and set the right flags.
    auto fd = open(filename, 0, 0);
    if (fd < 0) return nullptr;
    return stream_create(fd);
}

FILE* freopen(const char* __restrict__ filename, const char* __restrict__ mode, FILE* __restrict__ stream) {
    /// If the filename is nullptr, the mode of the stream is changed to the mode specified by `mode`.
    /// What mode changes are possible is implementation-defined, so currently, we always fail.
    (void)mode;
    if (!filename) { return nullptr; }

    /// Open the new file.
    auto fd = open(filename, 0, 0);
    if (fd < 0) return nullptr;

    /// Reassociate the stream with the new file.
    Lock lock{*stream};
    stream_reassociate(*stream, fd);
    return stream;
}

void setbuf(FILE* __restrict__ stream, char* __restrict__ buffer) {
    setvbuf(stream, buffer, buffer ? _IOFBF : _IONBF, BUFSIZ);
}

int setvbuf(FILE* __restrict__ stream, char* __restrict__ buffer, int mode, size_t size) {
    /// It is illegal to call setvbuf() after any I/O operation has been performed on the stream.
    Lock lock{*stream};
    if (stream->__rdbuf.__offs != 0
        || stream->__wbuf.__offs != 0
        || stream->__f_error
        || stream->__f_eof) { return -1; }

    /// Set the buffer.
    switch (mode) {
        case _IONBF:
            if (!stream_set_buffering(*stream, Unbuffered, nullptr, 0)) return -1;
            break;
        case _IOLBF:
            if (!stream_set_buffering(*stream, LineBuffered, buffer, size)) return -1;
            break;
        case _IOFBF:
            if (!stream_set_buffering(*stream, FullyBuffered, buffer, size)) return -1;
            break;
        default: return -1;
    }

    return 0;
}

/// ===========================================================================
///  7.21.6 Formatted input/output.
/// ===========================================================================
int fprintf(FILE* __restrict__ stream, const char* __restrict__ format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = vfprintf(stream, format, args);
    va_end(args);
    return ret;
}

int fscanf(FILE* __restrict__ stream, const char* __restrict__ format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = vfscanf(stream, format, args);
    va_end(args);
    return ret;
}

int printf(const char* __restrict__ format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = vfprintf(stdout, format, args);
    va_end(args);
    return ret;
}

int scanf(const char* __restrict__ format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = vfscanf(stdin, format, args);
    va_end(args);
    return ret;
}

int snprintf(char* __restrict__ str, size_t size, const char* __restrict__ format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = vsnprintf(str, size, format, args);
    va_end(args);
    return ret;
}

int sprintf(char* __restrict__ str, const char* __restrict__ format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = _IgnoreWarning("-Wdeprecated-declarations", vsprintf(str, format, args));
    va_end(args);
    return ret;
}

int sscanf(const char* __restrict__ str, const char* __restrict__ format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = vsscanf(str, format, args);
    va_end(args);
    return ret;
}

int vfprintf(FILE* __restrict__ stream, const char* __restrict__ format, va_list args) {
    /// FIXME: Stub.
    (void)stream;
    (void)format;
    (void)args;
    _LIBC_STUB();
    return -1;
}

int vfscanf(FILE* __restrict__ stream, const char* __restrict__ format, va_list args) {
    /// FIXME: Stub.
    (void)stream;
    (void)format;
    (void)args;
    _LIBC_STUB();
    return EOF;
}

int vprintf(const char* __restrict__ format, va_list args) {
    return vfprintf(stdout, format, args);
}

int vscanf(const char* __restrict__ format, va_list args) {
    return vfscanf(stdin, format, args);
}

int vsnprintf(char* __restrict__ str, size_t size, const char* __restrict__ format, va_list args) {
    /// FIXME: Stub.
    (void)str;
    (void)size;
    (void)format;
    (void)args;
    _LIBC_STUB();
    return -1;
}

int vsprintf(char* __restrict__ str, const char* __restrict__ format, va_list args) {
    /// FIXME: Stub.
    (void)str;
    (void)format;
    (void)args;
    _LIBC_STUB();
    return -1;
}

int vsscanf(const char* __restrict__ str, const char* __restrict__ format, va_list args) {
    /// FIXME: Stub.
    (void)str;
    (void)format;
    (void)args;
    _LIBC_STUB();
    return EOF;
}

/// ===========================================================================
///  7.21.7 Character input/output functions.
/// ===========================================================================
int fgetc(FILE* stream) {
    Lock lock{*stream};
    char c;
    if (stream_read(*stream, &c, 1) != 1) return EOF;
    return c;
}

char* fgets(char* __restrict__ str, int size, FILE* __restrict__ stream) {
    if (size < 0) return nullptr;
    if (size == 0) return str;

    Lock lock{*stream};
    return stream_read_until(*stream, str, size_t(size), '\n') ? str : nullptr;
}

int fputc(int c, FILE* stream) {
    /// Lock the file.
    Lock lock{*stream};

    /// Perform the write.
    auto ch = static_cast<char>(c);
    if (!stream_put_maybe_buffered(*stream, ch)) return EOF;
    return ch;
}

int fputs(const char* __restrict__ str, FILE* __restrict__ stream) {
    /// Lock the file.
    Lock lock{*stream};

    /// Perform the write.
    if (stream_put_maybe_buffered(*stream, str, strlen(str)) == EOF) return EOF;
    return 0;
}

int getc(FILE* stream) { return fgetc(stream); }

int getchar() { return fgetc(stdin); }

int putc(int c, FILE* stream) { return fputc(c, stream); }

int putchar(int c) { return fputc(c, stdout); }

int puts(const char* str) {
    if (fputs(str, stdout) == EOF) return EOF;
    if (fputc('\n', stdout) == EOF) return EOF;
    return 0;
}

int ungetc(int c, FILE* stream) {
    /// Lock the file.
    Lock lock{*stream};

    return stream_unget(*stream, static_cast<char>(c));
}

/// ===========================================================================
///  7.21.8 Direct input/output functions.
/// ===========================================================================
size_t fread(void* __restrict__ ptr, size_t size, size_t nmemb, FILE* __restrict__ stream) {
    if (size == 0 || nmemb == 0) return 0;

    /// Lock the file.
    Lock lock{*stream};

    /// Read the data. We assume that we don't overflow here because it would be
    /// physically impossible to allocate a buffer of that size anyway.
    auto ret = stream_read(*stream, static_cast<char*>(ptr), size * nmemb);
    return ret / size;
}

size_t fwrite(const void* __restrict__ ptr, size_t size, size_t nmemb, FILE* __restrict__ stream) {
    if (size == 0 || nmemb == 0) return 0;

    /// Lock the file.
    Lock lock{*stream};

    /// Write the data. We assume that we don't overflow here because it would be
    /// physically impossible to allocate a buffer of that size anyway.
    auto ret = stream_put_maybe_buffered(*stream, reinterpret_cast<const char* __restrict__>(ptr), size * nmemb);
    return ret == EOF ? 0 : nmemb;
}

/// ===========================================================================
///  7.21.9 File positioning functions.
/// ===========================================================================
int fgetpos(FILE* __restrict__ stream, fpos_t* __restrict__ pos) {
    /// Lock the file.
    Lock lock{*stream};

    /// Get the position.
    *pos = stream_getpos(*stream, *pos);
    return 0;
}

int fseek(FILE* stream, long offset, int whence) {
    /// Lock the file.
    Lock lock{*stream};

    /// Seek to the position.
    return stream_seek(*stream, offset, whence) ? 0 : -1;
}

int fsetpos(FILE* stream, const fpos_t* pos) {
    /// Lock the file.
    Lock lock{*stream};

    /// Set the position.
    return stream_setpos(*stream, *pos) ? 0 : -1;
}

long ftell(FILE* stream) {
    /// Lock the file.
    Lock lock{*stream};

    /// Get the position.
    long pos;
    [[maybe_unused]] auto res = stream_tell(*stream, pos);
    return pos;
}

void rewind(FILE* stream) {
    /// Lock the file.
    Lock lock{*stream};

    /// Rewind the file.
    stream_seek(*stream, 0, SEEK_SET);
    stream->__f_error = false;
}

/// ===========================================================================
///  7.21.9 Error-handling functions.
/// ===========================================================================
void clearerr(FILE* stream) {
    /// Lock the file.
    Lock lock{*stream};

    /// Clear the error flags.
    stream->__f_error = false;
    stream->__f_eof = false;
}

int feof(FILE* stream) {
    /// Lock the file.
    Lock lock{*stream};

    /// We're logically at EOF if we're physically at EOF and there is no ungotten
    /// character and the read buffer is empty.
    return stream_at_eof(*stream) && !stream_has_ungotten(*stream) && stream->__rdbuf.__offs == 0;
}

int ferror(FILE* stream) {
    /// Lock the file.
    Lock lock{*stream};

    return stream->__f_error;
}

void perror(const char* str) {
    if (str && *str) {
        fputs(str, stderr);
        fputs(": ", stderr);
    }

    fputs(strerror(errno), stderr);
    fputc('\n', stderr);
}

/// ===========================================================================
///  POSIX extensions.
/// ===========================================================================
char *ctermid(char *s) {
    static char empty_string = 0;
    if (!s) return &empty_string;
    *s = 0;
    return s;
}

int dprintf(int fd, const char * __restrict__ fmt, ...) {
    /// TODO: Like fprintf, but it accepts a file descriptor rather than a FILE.
    _LIBC_STUB();
}

FILE* fdopen(int fd, const char* mode) {
    /// TODO: Parse mode.
    /// TODO: Stub.
    /// NOTE: Semantics of 'w' are different.
    _LIBC_STUB();
    return nullptr;
}

void flockfile(FILE* stream) { stream->__mutex.lock(); }
int ftrylockfile(FILE* stream) { return stream->__mutex.try_lock() ? 0 : -1; }
void funlockfile(FILE* stream) { stream->__mutex.unlock(); }

FILE* fmemopen(void* __restrict__ buf, size_t size, const char* __restrict__ mode) {
    /// Screw you, I'm not implementing this; thank you very much.
    _LIBC_STUB();
    return nullptr;
}

int fseeko(FILE* stream, off_t offset, int whence) {
    return fseek(stream, long(offset), whence);
}

off_t ftello(FILE* stream) {
    return ftell(stream);
}

int getc_unlocked(FILE *stream) {
    char c;
    if (stream_read(*stream, &c, 1) != 1) return EOF;
    return c;
}

int getchar_unlocked() {
    return getc_unlocked(stdin);
}

int putc_unlocked(int c, FILE *stream) {
    auto ch = static_cast<char>(c);
    if (!stream_put_maybe_buffered(*stream, ch)) return EOF;
    return ch;
}

int putchar_unlocked(int c) {
    return putc_unlocked(c, stdout);
}

__END_DECLS__