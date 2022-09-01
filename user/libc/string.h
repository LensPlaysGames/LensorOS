/* Copyright 2022, Contributors To LensorOS.
All rights reserved.

This file is part of LensorOS.

LensorOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LensorOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LensorOS. If not, see <https://www.gnu.org/licenses/>. */


#ifndef LENSOR_OS_LIBC_STRING_H
#define LENSOR_OS_LIBC_STRING_H

#include "stddef.h"
#include "sys/types.h"

#if defined(__cplusplus)
extern "C" {
#endif

  /// Copying
  void* memcpy(void*, const void*, size_t);
  void* memmove(void*, const void*, size_t);  
  char* strcpy(char* dest, const char* src);
  char* strncpy(char* dest, const char* src, size_t);
  
  /// Concatenation
  char* strcat(char* dest, const char* src);
  char* strncat(char* dest, const char* src, size_t);

  /// Comparison
  int memcmp(const void*, const void*, size_t);
  int strcmp(const char*, const char*);
  int strcoll(const char* s1, const char* s2);
  int strncmp(const char*, const char*, size_t);
  size_t strxfrm(char* dest, const char* src, size_t n);  

  /// Searching
  void* memchr(const void*, int c, size_t);
  char* strchr(const char*, int c);
  size_t strcspn(const char*, const char* reject);
  char* strpbrk(const char*, const char* accept);
  char* strrchr(const char*, int c);
  size_t strspn(const char*, const char* accept);
  char* strstr(const char* haystack, const char* needle);
  char* strtok(char* str, const char* delim);

  /// Other
  const void* memmem(const void* haystack, size_t haystacklen, const void* needle, size_t needlelen);
  void* memset(void*, int, size_t);
  char* strerror(int errnum);
  size_t strlen(const char*);
  size_t strnlen(const char*, size_t maxlen);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
