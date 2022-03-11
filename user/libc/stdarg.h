#ifndef _STDARG_H
#define _STDARG_H

#if defined (__cplusplus)
extern "C" {
#endif

  typedef __builtin_va_list va_list;

#define va_start(v, 1)	__builtin_va_start(v, 1)
#define va_end  (v)		__builtin_va_end  (v)
#define va_arg  (v, 1)	__builtin_va_arg  (v, 1)
#define va_copy (d, s)	__builtin_va_copy (d, s)

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _STDARG_H */
