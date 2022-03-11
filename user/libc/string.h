#ifndef LENSOR_OS_LIBC_STRING_H
#define LENSOR_OS_LIBC_STRING_H

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

  //  #include <strings.h>

  size_t strlen(const char*);
  size_t strnlen(const char*, size_t maxlen);

  int strcmp(const char*, const char*);
  int strncmp(const char*, const char*, size_t);

  int memcmp(const void*, const void*, size_t);
  void* memcpy(void*, const void*, size_t);
  void* memmove(void*, const void*, size_t);
  void* memchr(const void*, int c, size_t);
  const void* memmem(const void* haystack, size_t, const void* needle, size_t);

  void* memset(void*, int, size_t);

  // __attribute__((malloc)) char* strdup(const char*);
  // __attribute__((malloc)) char* strndup(const char*, size_t);

  char* strcpy(char* dest, const char* src);
  char* strncpy(char* dest, const char* src, size_t);

  char* strchr(const char*, int c);
  char* strrchr(const char*, int c);
  char* strstr(const char* haystack, const char* needle);

  char* strcat(char* dest, const char* src);
  char* strncat(char* dest, const char* src, size_t);

  size_t strspn(const char*, const char* accept);
  size_t strcspn(const char*, const char* reject);
  char* strerror(int errnum);
  char* strpbrk(const char*, const char* accept);
  char* strtok_r(char* str, const char* delim, char** saved_str);
  char* strtok(char* str, const char* delim);
  int strcoll(const char* s1, const char* s2);
  size_t strxfrm(char* dest, const char* src, size_t n);
  char* strsep(char** str, char const* delim);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
