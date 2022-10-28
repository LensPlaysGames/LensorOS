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

/*
* The contents of this file are partly copied from various GCC headers.
*/

#ifndef _LENSOR_OS_LIBC_INTRIN_H
#define _LENSOR_OS_LIBC_INTRIN_H

#include "../bits/decls.h"

__BEGIN_DECLS__

/// For _mm_prefetch().
enum _mm_hint
{
    _MM_HINT_ET0 = 7,
    _MM_HINT_ET1 = 6,
    _MM_HINT_T0 = 3,
    _MM_HINT_T1 = 2,
    _MM_HINT_T2 = 1,
    _MM_HINT_NTA = 0
};

typedef int __m64_u __attribute__ ((__vector_size__ (8), __may_alias__, __aligned__ (1)));

#ifdef __SSE2__
typedef long long __m128i __attribute__ ((__vector_size__ (16), __may_alias__));
typedef double __m128d __attribute__ ((__vector_size__ (16), __may_alias__));

typedef long long __m128i_u __attribute__ ((__vector_size__ (16), __may_alias__, __aligned__ (1)));
typedef double __m128d_u __attribute__ ((__vector_size__ (16), __may_alias__, __aligned__ (1)));

/// Load a 128-bit vector from memory.
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_loadu_si128 (__m128i_u const *__P)
{
    return *__P;
}

/// Store a 128-bit vector to memory.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_storeu_si128 (__m128i_u *__P, __m128i __B)
{
    *__P = __B;
}
#endif // __SSE2__

#ifdef __AVX__
typedef float __m256 __attribute__ ((__vector_size__ (32), __may_alias__));
typedef long long __m256i __attribute__ ((__vector_size__ (32), __may_alias__));
typedef double __m256d __attribute__ ((__vector_size__ (32), __may_alias__));

typedef float __m256_u __attribute__ ((__vector_size__ (32), __may_alias__, __aligned__ (1)));
typedef long long __m256i_u __attribute__ ((__vector_size__ (32), __may_alias__, __aligned__ (1)));
typedef double __m256d_u __attribute__ ((__vector_size__ (32), __may_alias__, __aligned__ (1)));

/// Load a 256-bit vector from memory.
extern __inline __m256i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm256_loadu_si256 (__m256i_u const *__P)
{
    return *__P;
}

/// Store a 256-bit vector to memory.
extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm256_storeu_si256 (__m256i_u *__P, __m256i __A)
{
    *__P = __A;
}
#endif // __AVX__

/// Prefetch memory.
///
/// Defined as a macro because the second argument to __builtin_prefetch() must
/// be a constant. __extension__ suppresses a warning about the fact that we're
/// using a GNU extension to make sure that we're not evaluating _I twice. The
/// sizeof’s are so that warnings that occur in _P and _I are not swallowed.
#define _mm_prefetch(_P, _I) (sizeof(_P), sizeof(_I), __extension__ ({ \
  const __typeof__(_P) __P = _P;                                       \
  const __typeof__(_I) __I = _I;                                       \
  __builtin_prefetch ((__P), ((__I & 0x4) >> 2), (__I & 0x3));         \
}))

__END_DECLS__

#endif // _LENSOR_OS_LIBC_INTRIN_H
