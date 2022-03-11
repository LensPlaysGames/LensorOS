#ifndef _STDDEF_H
#define _STDDEF_H

#if defined (__cplusplus)
extern "C" {
#define NULL 0L
#else
#define NULL ((void*)0)
#endif

  typedef __PTRDIFF_TYPE__ ptrdiff_t;
  typedef __SIZE_TYPE__ size_t;

#if !defined (__cplusplus)
  typedef __WCHAR_TYPE__ wchar_t;
#endif

#define offsetof(type, member) __builtin_offsetof(type, member)

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _STDDEF_H */
