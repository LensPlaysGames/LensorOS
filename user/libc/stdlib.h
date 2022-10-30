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


#ifndef _STDLIB_H
#define _STDLIB_H

#include "sys/types.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

  /// String conversion
  double atof(const char*);
  int atoi(const char*);
  long atol(const char*);
  long long atoll(const char*);
  double strtod(const char*, char** endptr);
  float strtof(const char*, char** endptr);
  long strtol(const char*, char** endptr, int base);
  long double strtold(const char*, char** endptr);
  long long strtoll(const char*, char** endptr, int base);
  unsigned long strtoul(const char*, char** endptr, int base);
  unsigned long long strtoull(const char*, char** endptr, int base);

  /// Psuedo-random sequence generation
#define RAND_MAX 32767
  int rand(void);
  void srand(unsigned seed);

  /// Dynamic memory management
  __attribute__((malloc, alloc_size(1, 2))) void* calloc(size_t nitems, size_t);
  void free(void*);
  __attribute__((malloc, alloc_size(1))) void* malloc(size_t);
  __attribute__((alloc_size(2))) void* realloc(void* ptr, size_t);

  /// Like realloc, but does not copy the old data.
  __attribute__((alloc_size(2))) void* __mextend(void* ptr, size_t);

  /// Environment
  __attribute__((noreturn)) void abort(void);
  int atexit(void (*function)(void));
  int at_quick_exit(void (*function)(void));
  __attribute__((noreturn)) void exit(int status); // defined in "unistd.cpp"
  char* getenv(const char* name);
  __attribute__((noreturn)) void quick_exit(int status);
  int system(const char* command);
  __attribute__((noreturn)) void _Exit(int status);

  /// Searching and sorting
  void* bsearch(const void* key, const void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));
  void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));

  /// Integer arithmetics
  typedef struct {
    int quot;
    int rem;
  } div_t;
  typedef struct {
    long quot;
    long rem;
  } ldiv_t;
  typedef struct {
    long long quot;
    long long rem;
  } lldiv_t;

  int abs(int);
  div_t div(int, int);
  long labs(long);
  ldiv_t ldiv(long, long);
  long long int llabs(long long int);
  lldiv_t lldiv(long long, long long);

  /// Multibyte characters
#define MB_CUR_MAX 4
#define MB_LEN_MAX 16
  int mblen(char const*, size_t);
  int mbtowc(wchar_t*, const char*, size_t);
  int wctomb(char*, wchar_t);

  /// Multibyte strings
  size_t mbstowcs(wchar_t*, const char*, size_t);
  size_t wcstombs(char*, const wchar_t*, size_t);

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _STDLIB_H */
