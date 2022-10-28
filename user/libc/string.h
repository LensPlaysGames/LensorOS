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

#ifndef _LENSOR_OS_LIBC_STRING_H
#define _LENSOR_OS_LIBC_STRING_H

#include "stddef.h"
#include "sys/types.h"
#include "bits/string_intrinsics.h"

__BEGIN_DECLS__

/// Copying
__forceinline void* memcpy(void* __restrict__ __dest, const void* __restrict__ __src, size_t __n) {
#   ifdef __SSE2__
    __memcpy_sse_unaligned_intrin(__dest, __src, __n);
#   else
    __memcpy_naive(__dest, __src, __n);
#   endif
    return __dest;
}

void* memmove(void*, const void*, size_t);
char* strcpy(char* __dest, const char* __src);
char* strncpy(char* __dest, const char* __src, size_t);

/// Concatenation
char* strcat(char* __dest, const char* __src);
char* strncat(char* __dest, const char* __src, size_t);

/// Comparison
int memcmp(const void*, const void*, size_t);
int strcmp(const char*, const char*);
int strcoll(const char* __s1, const char* __s2);
int strncmp(const char*, const char*, size_t);
size_t strxfrm(char* __dest, const char* __src, size_t __n);

/// Searching
void* memchr(const void*, int __c, size_t);
char* strchr(const char*, int __c);
size_t strcspn(const char*, const char* __reject);
char* strpbrk(const char*, const char* __accept);
char* strrchr(const char*, int __c);
size_t strspn(const char*, const char* __accept);
char* strstr(const char* __haystack, const char* __needle);
char* strtok(char* __str, const char* __delim);

/// Other
const void* memmem(const void* __haystack, size_t __haystacklen, const void* __needle, size_t __needlelen);
void* memset(void*, int, size_t);
char* strerror(int __errnum);
size_t strlen(const char*);
size_t strnlen(const char*, size_t __maxlen);

__BEGIN_DECLS__

#endif
