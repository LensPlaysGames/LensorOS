#ifndef _LIMITS_H
#define _LIMITS_H

#if defined (__cplusplus)
extern "C" {
#endif

#define INT_MAX INT32_MAX
#define INT_MIN INT32_MIN

#define UINT_MAX UINT32_MAX

#define CHAR_BIT 8
#define SCHAR_MIN (-128)
#define SCHAR_MAX 127
#define UCHAR_MAX 255

#define SHRT_MAX 32768
#define SHRT_MIN (-SHRT_MAX - 1)

#define USHRT_MAX 65535

#define LONG_MAX 2147483647L
#define LONG_MIN (-LONG_MAX - 1L)

#define ULONG_MAX 4294967295UL

#define LONG_LONG_MAX 9223372036854775807LL
#define LONG_LONG_MIN (-LONG_LONG_MAX - 1LL)

#define LLONG_MAX LONG_LONG_MAX
#define LLONG_MIN LONG_LONG_MIN

#define ULONG_LONG_MAX 18446744073709551615ULL
#define ULLONG_MAX ULONG_LONG_MAX

#define CHAR_MIN SCHAR_MIN
#define CHAR_MAX SCHAR_MAX

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _LIMITS_H */
