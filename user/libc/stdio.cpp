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

#include "bits/cdtors.h"
#include "bits/file_struct.h"
#include "bits/stub.h"
#include "errno.h"
#include "extensions"
#include "stdarg.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/syscalls.h"

#include <algorithm>
#include <atomic>
#include <extensions>
#include <mutex>
#include <new>
#include <string>
#include <utility>

#define LOCK(stream) std::unique_lock _CAT(__lock_, __COUNTER__)(stream->__mutex)

/// ===========================================================================
///  Internal definitions and state
/// ===========================================================================
std::sparse_vector<_IO_File*, nullptr> _IO_File::open_files;
std::recursive_mutex _IO_File::big_file_lock;

enum : size_t { BUFFERED_READ_THRESHOLD = 64 };

/// ===========================================================================
///  _IO_File — CDtors.
/// ===========================================================================
_IO_File::_IO_File(_IO_fd_t fd, Buffering buffering_mode) {
    __f_buffering = 0;
    __f_error = 0;
    __f_eof = 0;
    __f_has_ungotten = 0;

    /// Write buffer.
    ///
    /// We use `malloc()` instead of `new` for the buffers because we might
    /// need to `realloc()` them later on.
    /// TODO: Only allocate a write buffer if the stream is open for writing.
    __wbuf.__buf = static_cast<char*>(malloc(BUFSIZ));
    __wbuf.__cap = BUFSIZ;
    __wbuf.__offs = 0;

    /// Read buffer.
    /// TODO: Only allocate a read buffer if the stream is open for reading.
    __rdbuf.__buf = static_cast<char*>(malloc(BUFSIZ));
    __rdbuf.__cap = BUFSIZ;
    __rdbuf.__start = 0;
    __rdbuf.__offs = 0;

    /// File descriptor.
    __fd = fd;

    /// Flags.
    _PushIgnoreWarning("-Wconversion")
    __f_buffering = buffering_mode;
    _PopWarnings()
}

_IO_File::~_IO_File() {
    /// Flush and close the stream.
    close();

    /// Delete the buffers.
    free(__wbuf.__buf);
    free(__rdbuf.__buf);
}

/// ===========================================================================
///  _IO_File — Flushing, closing, and reassociating.
/// ===========================================================================
void _IO_File::clear_buffers() {
    __wbuf.__offs = 0;
    __rdbuf.__start = 0;
    __rdbuf.__offs = 0;
}

void _IO_File::close() {
    /// Flush and close the stream.
    if (__fd != -1) {
        flush();
        ::close(__fd);
    }

    /// Clear the file descriptor.
    __fd = -1;

    /// Reset the stream.
    clear_buffers();
    clear_flags();
}

void _IO_File::erase() {
    /// Close the file.
    close();

    /// Remove it from the list of open files.
    std::unique_lock lock(big_file_lock);

#ifdef __lensor__
    /// Because the file list mirrors the file descriptor table in the kernel,
    /// there's a good chance that we might be able to index into the list of
    /// open files using the file descriptor.
    ///
    /// If we can't, we'll have to search the list for the file.
    size_t index = __fd;
    auto element = open_files[index];
    if (element && *element == this) { open_files.erase(index); }
    else
#endif
        open_files.erase(std::find(open_files, this));
}

bool _IO_File::flush() {
    if (__wbuf.__offs == 0) { return true; }

    /// Write the data.
    ssize_t written = ::write(__fd, __wbuf.__buf, __wbuf.__offs);

    /// Check for errors.
    if (written < 0 || size_t(written) != __wbuf.__offs) {
        __f_error = true;
        return false;
    }

    /// Clear the buffer.
    __wbuf.__offs = 0;
    return true;
}

void _IO_File::reassociate(_IO_fd_t fd) {
    close();
    __fd = fd;
}

/// ===========================================================================
///  _IO_File (static) — Flushing, closing, and reassociating.
/// ===========================================================================

void _IO_File::close_all() {
    std::unique_lock lock(big_file_lock);
    for (auto file : open_files) {
         file->erase();
    }
}

/// Open a new file
_IO_File* _IO_File::create(_IO_fd_t fd, Buffering buffering_mode) {
    /// Create the file.
    auto file = new _IO_File(fd, buffering_mode);

    /// Add it to the list of open files.
    std::unique_lock lock(big_file_lock);
    open_files.push_back(file);
    return file;
}

/// ===========================================================================
///  _IO_File — File manipulation.
/// ===========================================================================
int _IO_File::unget(char c) {
    /// TODO: unget() isn't possible if the stream isn't open for reading.
    if (!may_unget()) { return EOF; }

    __f_has_ungotten = true;
    __ungotten = c;

    return c;
}

int _IO_File::getpos(fpos_t& pos) const {
    /// TODO: Implement.
    (void)pos;
    return -1;
}

int _IO_File::seek(_IO_off_t offset, int whence) {
    __f_has_ungotten = false;
    __f_eof = false;

    /// Flush the stream.
    if (!flush()) return -1;

    // TODO: seek syscall
    return syscall(SYS_seek, __fd, offset, whence);
}

int _IO_File::setpos(const fpos_t& pos) {
    /// TODO: Implement.
    /// TODO: This has to undo unget()s.
    (void)pos;
    return -1;
}

auto _IO_File::tell(_IO_off_t& offset) const -> _IO_off_t{
    /// TODO: Implement.
    (void)offset;
    return -1;
}

/// ===========================================================================
///  _IO_File – Reading.
/// ===========================================================================
ssize_t _IO_File::read(char* __restrict__ buf, const size_t size) {
    /// TODO: Check if the stream is readable.
    /// If we have an ungotten character, store that in the buffer first.
    size_t rest = size;
    if (has_ungotten()) {
        buf[0] = __ungotten;
        __f_has_ungotten = false;
        buf++;
        rest--;
    }

    /// Copy data from the read buffer.
    if (__rdbuf.__offs > __rdbuf.__start) {
        auto copied = copy_from_read_buffer(buf, rest);

        /// If we've copied all the data, return.
        if (copied == rest) { return size; }
    }

    /// Return if we're at end of file.
    if (at_eof()) { return EOF; }

    /// Read from the file descriptor.
    ///
    /// If we ever get here, then the read buffer is empty.
    ///
    /// If the rest of the data that we need to read is small, read a block into
    /// the read buffer and copy the data.
    ///
    /// Otherwise, read it directly into the destination buffer.
    if (rest < BUFFERED_READ_THRESHOLD) {
        ssize_t n_read = ::read(__fd, __rdbuf.__buf, __rdbuf.__cap);
        if (n_read == -1) {
            __f_error = true;
            return EOF;
        }

        /// If we've reached end of file, set the flag.
        if (n_read == 0) { __f_eof = true; }
        __rdbuf.__offs = n_read;

        /// Copy the data.
        auto copied = copy_from_read_buffer(buf, rest);

        /// We've copied all the data.
        if (copied == rest) { return ssize_t(size); }

        /// We've reached end of file.
        return ssize_t(size - rest + copied);
    } else {
        auto n_read = ::read(__fd, buf, rest);
        if (n_read == -1) {
            __f_error = true;
            return EOF;
        }

        /// If we've reached end of file, set the flag.
        if (n_read == 0) { __f_eof = true; }
        return ssize_t(size - rest + n_read);
    }
}

bool _IO_File::read_until(char* __restrict__ buf, const size_t size, char until) {
/// TODO: Check if the stream is readable.
    /// If we have an ungotten character, store that in the buffer first.
    size_t rest = size;
    if (has_ungotten()) {
        buf[0] = __ungotten;
        __f_has_ungotten = false;
        buf++;
        rest--;
        if (__ungotten == until) { return true; }
    }

    /// Copy data from the read buffer.
    if (__rdbuf.__offs > __rdbuf.__start) {
        auto [copied, done] = copy_until_from_read_buffer(buf, rest, until);
        if (done || at_eof()) return true;
        rest -= copied;
    }

    /// Read from the file descriptor.
    /// If we ever get here, then the read buffer is empty.
    while (!at_eof()) {
        ssize_t n_read = ::read(__fd, __rdbuf.__buf, __rdbuf.__cap);
        if (n_read == -1) {
            __f_error = true;
            return false;
        }

        /// If we've reached end of file, set the flag.
        if (n_read == 0) { __f_eof = true; }
        __rdbuf.__offs = n_read;

        /// Copy the data.
        auto [copied, done] = copy_until_from_read_buffer(buf, rest, until);
        if (done) return true;
    }

    return false;
}

size_t _IO_File::copy_from_read_buffer(char* __restrict__ buf, size_t size) {
    /// We can copy at most __offs - __start many bytes.
    size_t stream_rem = __rdbuf.__offs - __rdbuf.__start;

    /// Copy the data.
    size_t to_copy = std::min(size, stream_rem);
    memcpy(buf, __rdbuf.__buf, to_copy);

    /// If we've copied all the data, reset the buffer.
    if (stream_rem == to_copy) {
        __rdbuf.__start = 0;
        __rdbuf.__offs = 0;
    }
    /// Otherwise, move the start pointer.
    else __rdbuf.__start += to_copy;

    /// Return the number of bytes copied.
    return to_copy;
}

std::pair<size_t, bool> _IO_File::copy_until_from_read_buffer(
    char* __restrict__ buf,
    const size_t rest,
    char until
) {
    /// Find the first occurrence of `until` in the read buffer.
    auto* first = (char*) memchr(
        __rdbuf.__buf + __rdbuf.__start,
        until,
        __rdbuf.__offs - __rdbuf.__start
    );

    /// Copy everything up to and including `until`, or the size of the buffer
    /// if `until` was not found.
    auto max_copy = first ? size_t(first - __rdbuf.__start + 1) : __rdbuf.__offs;
    auto to_copy = std::min(rest, max_copy);

    /// Copy the data.
    auto copied = copy_from_read_buffer(buf, to_copy);

    /// If we've filled the buffer or found `until`, we're done.
    if (first || copied == rest) { return {copied, true}; }
    return {copied, false};
}

/// ===========================================================================
///  _IO_File – Writing.
/// ===========================================================================
bool _IO_File::setbuf(Buffering buffering, size_t size) {
    /// Flush the stream.
    if (!flush()) return false;

    /// Set the buffer.
    switch (buffering) {
        case Unbuffered:
            /// We don't free the buffer here because the read buffer may be
            /// non-empty, and we don't want to lose the data.
            __f_buffering = Unbuffered;
            return true;

        case LineBuffered:
        case FullyBuffered:
            _PushIgnoreWarning("-Wconversion")
            __f_buffering = buffering;
            _PopWarnings()

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
            if (__wbuf.__cap != size) {
                __wbuf.__cap = size;
                __wbuf.__buf = (char*) __mextend(__wbuf.__buf, size);
            }

            /// Realloc the read buffer. Make sure we don't lose any data.
            if (__rdbuf.__cap != size) {
                /// Buffer is empty; just extend it.
                if (!__rdbuf.__offs) {
                    __rdbuf.__cap = size;
                    __rdbuf.__buf = (char*) __mextend(__rdbuf.__buf, size);
                }

                /// Buffer has data; make sure we don't lose any of it.
                else {
                    __rdbuf.__cap = std::max(__rdbuf.__offs, size);
                    __rdbuf.__buf = (char*) realloc(__rdbuf.__buf, __rdbuf.__cap);
                }
            }

            return true;

        /// Invalid buffering mode.
        default: return false;
    }
}

ssize_t _IO_File::write(const char* __restrict__ str, size_t sz) {
    /// TODO: Check if the stream is open for writing.
    switch (buffering()) {
        case Unbuffered: {
            auto written = ::write(__fd, str, sz);
            if (written < 0 || size_t(written) != sz) {
                __f_error = true;
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
                auto n_put = write_internal(str, nl_sz);
                if (n_put != nl_sz) return written;

                /// Discard the data up to and including the newline.
                str += nl_sz;
                sz -= nl_sz;

                /// Flush the stream.
                if (!flush()) return written;
                written += n_put;
            }

            /// Write the rest.
            written += write_internal(str, sz);
            return written;
        }

        case FullyBuffered: return write_internal(str, sz);

        /// Invalid buffering mode. Should be unreachable.
        default: return EOF;
    }
}

bool _IO_File::write(char c) { return write(&c, 1) == 1; }

ssize_t _IO_File::write_internal(const char* __restrict__ buffer, size_t count) {
    /// TODO: Check if the stream is open for writing.
    /// Flush the buffer if this operation would overflow it.
    if (__wbuf.__offs + count > __wbuf.__cap && !flush()) { return EOF; }

    /// Write data directly to the stream if it doesn't fit in the buffer.
    if (count > __wbuf.__cap) {
        auto written = ::write(__fd, buffer, count);
        if (written < 0 || size_t(written) != count) {
            __f_error = true;
            return EOF;
        }

        return ssize_t(count);
    }

    /// Write data to the buffer.
    memcpy(__wbuf.__buf + __wbuf.__offs, buffer, count);
    __wbuf.__offs += count;
    return ssize_t(count);
}

/// ===========================================================================
///  C Interface.
/// ===========================================================================
__BEGIN_DECLS__

FILE* stdin;
FILE* stdout;
FILE* stderr;

_PushIgnoreWarning("-Wprio-ctor-dtor")

/// Initialise the standard streams.
[[gnu::constructor(_CDTOR_STDIO)]] void __stdio_init() {
    stdin = FILE::create(STDIN_FILENO);
    stdout = FILE::create(STDOUT_FILENO);
    stderr = FILE::create(STDERR_FILENO, Unbuffered);
    __stdio_destructed = false;
}

/// Flush the streams on exit.
[[gnu::destructor(_CDTOR_STDIO)]] void __stdio_fini() {
    FILE::close_all();
    __stdio_destructed = true;
}

_PopWarnings()

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
    stream->erase();
    return 0;
}

int fflush(FILE* stream) {
    /// Flush all streams if `stream` is nullptr.
    if (!stream) {
        std::unique_lock file_list_lock{FILE::big_file_lock};
        for (auto f : FILE::open_files) {
            LOCK(f);
            if (!f->flush()) return EOF;
        }
        return 0;
    }

    LOCK(stream);
    if (!stream->flush()) return EOF;

    return 0;
}

FILE* fopen(const char* __restrict__ filename, const char* __restrict__ mode) {
    /// TODO: Parse mode.
    (void)mode;

    /// TODO: Allocate a new file and buffer.
    /// TODO: Parse mode and set the right flags.
    auto fd = open(filename, 0, 0);
    if (fd < 0) return nullptr;
    return FILE::create(fd);
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
    LOCK(stream);
    stream->reassociate(fd);
    return stream;
}

void setbuf(FILE* __restrict__ stream, char* __restrict__ buffer) {
    setvbuf(stream, buffer, buffer ? _IOFBF : _IONBF, BUFSIZ);
}

int setvbuf(FILE* __restrict__ stream, char* __restrict__, int mode, size_t size) {
    /// It is illegal to call setvbuf() after any I/O operation has been performed on the stream.
    LOCK(stream);
    if (stream->__rdbuf.__offs != 0
        || stream->__wbuf.__offs != 0
        || stream->__f_error
        || stream->__f_eof) { return -1; }

    /// Set the buffer.
    switch (mode) {
        case _IONBF:
            if (!stream->setbuf(Unbuffered, 0)) return -1;
            break;
        case _IOLBF:
            if (!stream->setbuf(LineBuffered, size)) return -1;
            break;
        case _IOFBF:
            if (!stream->setbuf(FullyBuffered, size)) return -1;
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
    _PushIgnoreWarning("-Wdeprecated-declarations")
    auto ret = vsprintf(str, format, args);
    _PopWarnings()
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
    if (!stream || !format) {
        return -1;
    }

    for (const char *fmt = format; *fmt; ++fmt) {
        if (*fmt == '%') {
            ++fmt;
            switch (*fmt) {

            // TODO: Support more format specifiers

            case 's': {
                const char *str = va_arg(args, const char *);
                fputs(str, stream);
            } continue;

            case 'c': {
                int c = va_arg(args, int);
                fputc(c, stream);
            } continue;

            case 'd': {
                int val = va_arg(args, int);
                bool negative = false;

                if (val < 0) {
                    negative = true;
                    val = -val;
                }

                // TODO: Abstract + parameterise number printing
                constexpr const size_t radix = 10;
                static_assert(radix != 0, "Can not print zero-base numbers");

                constexpr const size_t max_digits = 32;
                static_assert(max_digits != 0, "Can not print into zero-length buffer");
                char digits[max_digits] = {0};

                size_t i = 0;
                for (size_t tmp_val = val; tmp_val && i < max_digits; tmp_val /= radix, ++i) {
                    // TODO: better value -> digit conversion
                    digits[i] = '0' + (tmp_val % radix);
                }

                if (i == 0) digits[0] = '0';
                else if (i != 1) {
                    // Reverse digits
                    size_t j = 0;
                    while (j < i) {
                        --i;
                        char tmp = digits[i];
                        digits[i] = digits[j];
                        digits[j] = tmp;
                        ++j;
                    }
                }

                if (negative) fputc('-', stream);
                for (const char *it = &digits[0]; it < &digits[0] + max_digits && *it; ++it)
                    fputc(*it, stream);

            } continue;

            case '\0':
                fputc('%', stream);
                return 0;

            default:
                fputc('%', stream);
                break;
            }
        }
        fputc(*fmt, stream);
    }
    return 0;
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
    LOCK(stream);
    char c;
    if (stream->read(&c, 1) != 1) return EOF;
    return c;
}

char* fgets(char* __restrict__ str, int size, FILE* __restrict__ stream) {
    if (size < 0) return nullptr;
    if (size == 0) return str;

    LOCK(stream);
    return stream->read_until(str, size_t(size), '\n') ? str : nullptr;
}

int fputc(int c, FILE* stream) {
    /// Lock the file.
    LOCK(stream);

    /// Perform the write.
    auto ch = static_cast<char>(c);
    if (!stream->write(ch)) return EOF;
    return ch;
}

int fputs(const char* __restrict__ str, FILE* __restrict__ stream) {
    /// Lock the file.
    LOCK(stream);

    /// Perform the write.
    if (stream->write(str, strlen(str)) == EOF) return EOF;
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
    LOCK(stream);
    return stream->unget(static_cast<char>(c));
}

/// ===========================================================================
///  7.21.8 Direct input/output functions.
/// ===========================================================================
size_t fread(void* __restrict__ ptr, size_t size, size_t nmemb, FILE* __restrict__ stream) {
    if (size == 0 || nmemb == 0) return 0;

    /// Lock the file.
    LOCK(stream);

    /// Read the data. We assume that we don't overflow here because it would be
    /// physically impossible to allocate a buffer of that size anyway.
    auto ret = stream->read(static_cast<char*>(ptr), size * nmemb);
    return ret / size;
}

size_t fwrite(const void* __restrict__ ptr, size_t size, size_t nmemb, FILE* __restrict__ stream) {
    if (size == 0 || nmemb == 0) return 0;

    /// Lock the file.
    LOCK(stream);

    /// Write the data. We assume that we don't overflow here because it would be
    /// physically impossible to allocate a buffer of that size anyway.
    auto ret = stream->write(reinterpret_cast<const char* __restrict__>(ptr), size * nmemb);
    return ret == EOF ? 0 : nmemb;
}

/// ===========================================================================
///  7.21.9 File positioning functions.
/// ===========================================================================
int fgetpos(FILE* __restrict__ stream, fpos_t* __restrict__ pos) {
    /// Lock the file.
    LOCK(stream);

    /// Get the position.
    *pos = stream->getpos(*pos);
    return 0;
}

int fseek(FILE* stream, long offset, int whence) {
    /// Lock the file.
    LOCK(stream);

    /// Seek to the position.
    return stream->seek(offset, whence) == 0 ? 0 : -1;
}

int fsetpos(FILE* stream, const fpos_t* pos) {
    /// Lock the file.
    LOCK(stream);

    /// Set the position.
    return stream->setpos(*pos) ? 0 : -1;
}

long ftell(FILE* stream) {
    /// Lock the file.
    LOCK(stream);

    /// Get the position.
    long pos;
    [[maybe_unused]] auto res = stream->tell(pos);
    return pos;
}

void rewind(FILE* stream) {
    /// Lock the file.
    LOCK(stream);

    /// Rewind the file.
    stream->seek(0, SEEK_SET);
    stream->__f_error = 0;
    stream->__f_eof = 0;
    stream->__f_has_ungotten = 0;
}

/// ===========================================================================
///  7.21.9 Error-handling functions.
/// ===========================================================================
void clearerr(FILE* stream) {
    /// Lock the file.
    LOCK(stream);

    /// Clear the error flags.
    stream->__f_error = false;
    stream->__f_eof = false;
}

int feof(FILE* stream) {
    /// Lock the file.
    LOCK(stream);

    /// We're logically at EOF if we're physically at EOF and there is no ungotten
    /// character and the read buffer is empty.
    return stream->at_eof() && !stream->has_ungotten() && stream->__rdbuf.__offs == 0;
}

int ferror(FILE* stream) {
    LOCK(stream);
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
_PushIgnoreWarning("-Wunused-parameter")
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
    if (stream->read(&c, 1) != 1) return EOF;
    return c;
}

int getchar_unlocked() {
    return getc_unlocked(stdin);
}

int putc_unlocked(int c, FILE *stream) {
    auto ch = static_cast<char>(c);
    if (!stream->write(ch)) return EOF;
    return ch;
}

int putchar_unlocked(int c) {
    return putc_unlocked(c, stdout);
}
_PopWarnings()
/// ===========================================================================
///  Internal
/// ===========================================================================
void __write(const char* str) { write(2, str, strlen(str)); }
void __write_ptr(void* ptr) {
    auto s = std::to_string(reinterpret_cast<uintptr_t>(ptr));
    __write("0x");
    write(2, s.data(), s.size());
}

__END_DECLS__
