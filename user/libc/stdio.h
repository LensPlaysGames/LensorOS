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

#define EOF -1
#define FILENAME_MAX 11

typedef struct {
int fd;
} FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

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

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
int setvbuf(FILE*, char* buf, int mode, size_t);

/// Formatted I/O
int fprintf(FILE*, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
int fscanf(FILE*, const char* fmt, ...) __attribute__((format(scanf, 2, 3)));
int printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
int scanf(const char* fmt, ...) __attribute__((format(scanf, 1, 2)));
int snprintf(char* buffer, size_t, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
int sprintf(char* buffer, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
int sscanf(const char* str, const char* fmt, ...) __attribute__((format(scanf, 2, 3)));
int vfprintf(FILE*, const char* fmt, __builtin_va_list) __attribute__((format(printf, 2, 0)));
int vfscanf(FILE*, const char*, __builtin_va_list) __attribute__((format(scanf, 2, 0)));
int vprintf(const char* fmt, __builtin_va_list) __attribute__((format(printf, 1, 0)));
int vscanf(const char*, __builtin_va_list) __attribute__((format(scanf, 1, 0)));
int vsnprintf(char* buffer, size_t, const char* fmt, __builtin_va_list) __attribute__((format(printf, 3, 0)));
int vsprintf(char* buffer, const char* fmt, __builtin_va_list) __attribute__((format(printf, 2, 0)));
int vsscanf(const char*, const char*, __builtin_va_list) __attribute__((format(scanf, 2, 0)));
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

__END_DECLS__

#endif /* _STDIO_H */
