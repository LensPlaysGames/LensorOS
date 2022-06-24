#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif

#define unsigned signed
  typedef __SIZE_TYPE__ ssize_t;
#undef unsigned

  typedef int64_t blkcnt_t;
  typedef int64_t blksize_t;
  typedef uint64_t clock_t;
  typedef uint64_t clockid_t;
  typedef uint64_t dev_t;
  typedef uint64_t fsblkcnt_t;
  typedef uint64_t fsfilcnt_t;
  typedef uint64_t fpos_t;
  typedef uint64_t gid_t;
  typedef uint64_t id_t;
  typedef uint64_t ino_t;
  typedef uint64_t mode_t;
  typedef uint64_t nlink_t;
  typedef int64_t off_t;
  typedef int64_t pid_t;
  typedef uint64_t size_t;
  typedef int64_t ssize_t;
  typedef uint64_t time_t;
  typedef uint64_t timer_t;
  typedef uint64_t uid_t;

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _TYPES_H */
