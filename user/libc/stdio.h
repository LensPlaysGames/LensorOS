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


#ifndef _STDIO_H
#define _STDIO_H

#include "bits/decls.h"
#include "sys/types.h"

__BEGIN_DECLS__

#define EOF (-1)
#define FILENAME_MAX 11

/// Opaque type representing a file.
typedef struct _IO_File FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

/// The standard requires these to be macros.
#define stdin stdin
#define stdout stdout
#define stderr stderr

/// File operations
int remove(const char* pathname);
int rename(const char* oldpath, const char* newpath);

#define TMP_MAX 25 // Minimum is twenty five
FILE* tmpfile(void);

#define L_tmpnam 4
char* tmpnam(char*);

/// File access
int fclose(FILE*);
int fflush(FILE*);


#define FOPEN_MAX 7  // Minimum is seven
FILE* fopen(const char* pathname, const char* mode);
FILE* freopen(const char* pathname, const char* mode, FILE*);

#define BUFSIZ 1024
void setbuf(FILE*, char* buf);

/// These MUST be 0, 1, 2 because of how we handle them internally.
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

int setvbuf(FILE*, char* buf, int mode, size_t);

/// Formatted I/O
int fprintf(FILE*, const char* fmt, ...) __attribute__((__format__(__printf__, 2, 3)));
int fscanf(FILE*, const char* fmt, ...) __attribute__((__format__(__scanf__, 2, 3)));
int printf(const char* fmt, ...) __attribute__((__format__(__printf__, 1, 2)));
int scanf(const char* fmt, ...) __attribute__((__format__(__scanf__, 1, 2)));
int snprintf(char* buffer, size_t, const char* fmt, ...) __attribute__((__format__(__printf__, 3, 4)));

__attribute__((__format__( printf, 2, 3),
__deprecated__("sprintf() is deprecated as it can cause buffer overflows. Use snprintf() instead.")))
int sprintf(char* buffer, const char* fmt, ...);

int sscanf(const char* str, const char* fmt, ...) __attribute__((__format__(__scanf__, 2, 3)));
int vfprintf(FILE*, const char* fmt, __builtin_va_list) __attribute__((__format__(__printf__, 2, 0)));
int vfscanf(FILE*, const char*, __builtin_va_list) __attribute__((__format__(__scanf__, 2, 0)));
int vprintf(const char* fmt, __builtin_va_list) __attribute__((__format__(__printf__, 1, 0)));
int vscanf(const char*, __builtin_va_list) __attribute__((__format__(__scanf__, 1, 0)));
int vsnprintf(char* buffer, size_t, const char* fmt, __builtin_va_list) __attribute__((__format__(__printf__, 3, 0)));

__attribute__((__format__(__printf__, 2, 0)),
__deprecated__("vsprintf() is deprecated as it can cause buffer overflows. Use vsnprintf() instead."))
int vsprintf(char* buffer, const char* fmt, __builtin_va_list);

int vsscanf(const char*, const char*, __builtin_va_list) __attribute__((__format__(__scanf__, 2, 0)));
int fgetc(FILE*);
char* fgets(char* buffer, int size, FILE*);
int fputc(int ch, FILE*);
int fputs(const char*, FILE*);
int getc(FILE*);
int getchar(void);
int putc(int ch, FILE*);
int putchar(int ch);
int puts(const char*);
int ungetc(int c, FILE*);

/// Direct I/O
size_t fread(void* ptr, size_t size, size_t nmemb, FILE*);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE*);

/// File positioning
int fgetpos(FILE*, fpos_t*);

#define SEEK_CUR 0
#define SEEK_END 1
#define SEEK_SET 2
int fseek(FILE*, long offset, int whence);

int fsetpos(FILE*, const fpos_t*);
long ftell(FILE*);
void rewind(FILE*);

/// Error handling
void clearerr(FILE*);
int feof(FILE*);
int ferror(FILE*);
void perror(const char*);

/// POSIX extensions.
#define L_ctermid 1
char *ctermid(char *s);
int dprintf(int fd, const char * __restrict__ fmt, ...) __attribute__((__format__(__printf__, 2, 3)));
FILE* fdopen(int fd, const char* mode);
void flockfile(FILE* stream);
int ftrylockfile(FILE* stream);
void funlockfile(FILE* stream);
FILE* fmemopen(void* __restrict__ buf, size_t size, const char* __restrict__ mode);
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *);
int getc_unlocked(FILE *stream);
int getchar_unlocked(void);
int putc_unlocked(int c, FILE *stream);
int putchar_unlocked(int c);

__END_DECLS__

#endif /* _STDIO_H */
