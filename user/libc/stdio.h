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

#include <bits/decls.h>
#include <bits/io_defs.h>
#include <sys/types.h>

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

/// Initialise stdio.
void __stdio_init();
void __stdio_fini();

/// For internal use only.
void __write(const char*);
void __write_ptr(void* ptr);

/// File operations
int remove(const char* __pathname);
int rename(const char* __oldpath, const char* __newpath);

#define TMP_MAX 25 // Minimum is twenty five
FILE* tmpfile(void);

#define L_tmpnam 4
char* tmpnam(char*);

/// File access
int fclose(FILE*);
int fflush(FILE*);


#define FOPEN_MAX 7  // Minimum is seven
FILE* fopen(const char* __pathname, const char* __mode);
FILE* freopen(const char* __pathname, const char* __mode, FILE*);

#define BUFSIZ 1024
void setbuf(FILE*, char* __buf);

int setvbuf(FILE*, char* __buf, int __mode, size_t);

/// Formatted I/O
int fprintf(FILE*, const char* __fmt, ...) _Format(__printf__, 2, 3);
int fscanf(FILE*, const char* __fmt, ...) _Format(__printf__, 2, 3);
int printf(const char* __fmt, ...) _Format(__printf__, 1, 2);
int scanf(const char* __fmt, ...) _Format(__scanf__, 1, 2);
int snprintf(char* __buffer, size_t, const char* __fmt, ...) _Format(__printf__, 3, 4);

_Format(__printf__, 2, 3)
_Deprecated("sprintf() is deprecated as it can cause buffer overflows. Use snprintf() instead.")
int sprintf(char* __buffer, const char* __fmt, ...);

int sscanf(const char* __str, const char* __fmt, ...) _Format(__printf__, 2, 3);
int vfprintf(FILE*, const char* __fmt, __builtin_va_list) _Format(__printf__, 2, 0);
int vfscanf(FILE*, const char*, __builtin_va_list) _Format(__scanf__, 2, 0);
int vprintf(const char* __fmt, __builtin_va_list) _Format(__printf__, 1, 0);
int vscanf(const char*, __builtin_va_list) _Format(__scanf__, 1, 0);
int vsnprintf(char* __buffer, size_t, const char* __fmt, __builtin_va_list) _Format(__printf__, 3, 0);

_Format(__printf__, 2, 0)
_Deprecated("vsprintf() is deprecated as it can cause buffer overflows. Use vsnprintf() instead.")
int vsprintf(char* __buffer, const char* __fmt, __builtin_va_list);

int vsscanf(const char*, const char*, __builtin_va_list) _Format(__scanf__, 2, 0);
int fgetc(FILE*);
char* fgets(char* __buffer, int __size, FILE*);
int fputc(int __ch, FILE*);
int fputs(const char*, FILE*);
int getc(FILE*);
int getchar(void);
int putc(int __ch, FILE*);
int putchar(int __ch);
int puts(const char*);
int ungetc(int __ch, FILE*);

/// Direct I/O
size_t fread(void* __ptr, size_t __size, size_t __nmemb, FILE*);
size_t fwrite(const void* __ptr, size_t __size, size_t __nmemb, FILE*);

/// File positioning
int fgetpos(FILE*, fpos_t*);

#define SEEK_CUR 0
#define SEEK_END 1
#define SEEK_SET 2
int fseek(FILE* __stream, long __offset, int __whence);

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
char *ctermid(char *__s);
int dprintf(int __fd, const char * __restrict__ __fmt, ...) _Format(__printf__, 2, 3);
FILE* fdopen(int __fd, const char* __mode);
void flockfile(FILE* __stream);
int ftrylockfile(FILE* __stream);
void funlockfile(FILE* __stream);
FILE* fmemopen(void* __restrict__ __buf, size_t __size, const char* __restrict__ __mode);
int fseeko(FILE *__stream, off_t __offset, int __whence);
off_t ftello(FILE *);
int getc_unlocked(FILE *__stream);
int getchar_unlocked(void);
int putc_unlocked(int __c, FILE *__stream);
int putchar_unlocked(int __c);

__END_DECLS__

#endif /* _STDIO_H */
