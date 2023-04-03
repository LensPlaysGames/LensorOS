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

#ifndef _LENSOR_OS_LIBC_STRING_INTRINSICS_H
#define _LENSOR_OS_LIBC_STRING_INTRINSICS_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-label"

#include <bits/decls.h>
#include "../sys/intrin.h"

__BEGIN_DECLS__

// TODO: Remove `&& 0` once we can actually get it compiling (I fucking
// hate that GCC uses memcpy from our libc and inlines it ffs).
#if defined(__SSE2__) && 0
#    define __have_memcpy_sse_unaligned_intrin
#endif

#ifdef __have_memcpy_sse_unaligned_intrin

/// Flag that indicates whether Enhanced REP MOVSB is supported.
extern _Bool __libc_have_erms;

/// Threshhold that indicates when copying using SSE2 is faster than using
/// AVX instructions.
#define _SSE_MEMCPY_THRESHOLD ((__SIZE_TYPE__)262144)

/// Threshhold that indicates when Enhanced REP MOVSB is faster than using
/// SSE2 instructions.
#define _ENHANCED_REP_MOVSB_THRESHOLD ((__SIZE_TYPE__)100000000)

/// Optimised memcpy() for SSE2/AVX.
///
/// Declared here in a header so that it can be inlined; this can speed up the
/// copying of small buffers whose size is known at compile time quite drastically.
///
/// The algorithm is as follows:
///   1. For small copies (< 32 bytes) we copy 8 bytes at a time and then the
///      remaining bytes individually.
///   2. For copies below 128/256 bytes, depending on whether AVX is enabled, we
///      copy 32 bytes at a time and then the remaining bytes as per step 1.
///   3. For larger copies, we copy 128/256 bytes at a time and then the remaining
///      bytes as per step 2.
///   4. For very large copies, we always use SSE2 rather than AVX because it seems
///      to be faster. The remaining bytes are copied as per step 2.
///   5. For very very large copies, we use the REP MOVSB instruction to copy all
///      bytes if Enhanced REP MOVSB is enabled, otherwise, the data is copied as
///      per step 4.
///
/// We use unaligned vector instructions for copies that involve more than 1 byte
/// to avoid having to align the data (which may not always be possible). On modern
/// x86 processors, unaligned vector instructions should be as fast their aligned
/// counterparts.
///
/// Enhanced REP MOVSB is supposedly faster when the data to copy is 16/64 byte-aligned.
/// It remains to be investigated whether there is any benefit to using it for aligned
/// data rather than always resorting to SSE2/AVX.
__inline__ void __memcpy_sse_unaligned_intrin(void* __restrict__ __dest, const void* __restrict__ __src, __SIZE_TYPE__ __n) {
    // Fewer than 32 bytes to copy.
    if (__n < 32) goto __last_32;

        // Fewer than 256/128 bytes to copy.
#ifdef __AVX__
    if (__n < 256) goto __last_256;

    // Fewer than _SSE_MEMCPY_THRESHOLD bytes to copy.
    // SSE2 is faster than AVX2 for huge copies.
    if (__n > _SSE_MEMCPY_THRESHOLD) goto __huge;
#else
    if (__n < 128) goto __last_256;
#endif

    /// Copy 256/128 bytes at a time using AVX2 if possible and SSE2 otherwise.
    for (;;) {
#ifdef __AVX__
        _mm_prefetch((const char*) __src + 128, _MM_HINT_T0);
        _mm_prefetch((const char*) __src + 192, _MM_HINT_T0);

        _mm256_storeu_si256((__m256i*) __dest + 0, _mm256_loadu_si256((const __m256i*) __src + 0));
        _mm256_storeu_si256((__m256i*) __dest + 1, _mm256_loadu_si256((const __m256i*) __src + 1));
        _mm256_storeu_si256((__m256i*) __dest + 2, _mm256_loadu_si256((const __m256i*) __src + 2));
        _mm256_storeu_si256((__m256i*) __dest + 3, _mm256_loadu_si256((const __m256i*) __src + 3));
        _mm256_storeu_si256((__m256i*) __dest + 4, _mm256_loadu_si256((const __m256i*) __src + 4));
        _mm256_storeu_si256((__m256i*) __dest + 5, _mm256_loadu_si256((const __m256i*) __src + 5));
        _mm256_storeu_si256((__m256i*) __dest + 6, _mm256_loadu_si256((const __m256i*) __src + 6));
        _mm256_storeu_si256((__m256i*) __dest + 7, _mm256_loadu_si256((const __m256i*) __src + 7));

        __src = (const char*) __src + 256;
        __dest = (char*) __dest + 256;
        __n -= 256;
        if (__n < 256) break;
#else
        _mm_prefetch((const char*) __src + 64, _MM_HINT_T0);

        _mm_storeu_si128((__m128i*) __dest + 0, _mm_loadu_si128((const __m128i*) __src + 0));
        _mm_storeu_si128((__m128i*) __dest + 1, _mm_loadu_si128((const __m128i*) __src + 1));
        _mm_storeu_si128((__m128i*) __dest + 2, _mm_loadu_si128((const __m128i*) __src + 2));
        _mm_storeu_si128((__m128i*) __dest + 3, _mm_loadu_si128((const __m128i*) __src + 3));
        _mm_storeu_si128((__m128i*) __dest + 4, _mm_loadu_si128((const __m128i*) __src + 4));
        _mm_storeu_si128((__m128i*) __dest + 5, _mm_loadu_si128((const __m128i*) __src + 5));
        _mm_storeu_si128((__m128i*) __dest + 6, _mm_loadu_si128((const __m128i*) __src + 6));
        _mm_storeu_si128((__m128i*) __dest + 7, _mm_loadu_si128((const __m128i*) __src + 7));

        __src = (const char*) __src + 128;
        __dest = (char*) __dest + 128;
        __n -= 128;
        if (__n < 128) break;
#endif
    }

    // Copy the remaining bytes (either up to 32 or up to 256)
__rest:
    if (__n <= 32) goto __last_32;

    // Last couple of bytes.
    // Copy 32 bytes at a time.
__last_256:
    _mm_prefetch((const char*) __src + 32, _MM_HINT_T0);

    while (__n >= 32) {
        _mm_storeu_si128((__m128i*) __dest, _mm_loadu_si128((const __m128i*) __src));
        _mm_storeu_si128((__m128i*) __dest + 1, _mm_loadu_si128((const __m128i*) __src + 1));
        __src = (const char*) __src + 32;
        __dest = (char*) __dest + 32;
        __n -= 32;
    }

    // Copy chunks of 8 bytes.
__last_32:
    // Copy 8 bytes at a time.
    while (__n >= 8) {
        *(__m64_u*) __dest = *(const __m64_u*) __src;
        __src = (const char*) __src + 8;
        __dest = (char*) __dest + 8;
        __n -= 8;
    }

    // Copy last bytes.
    while (__n) {
        *(char*) __dest = *(const char*) __src;
        __dest = (char*) __dest + 1;
        __src = (const char*) __src + 1;
        __n -= 1;
    }

    return;

    // For huge copies.
__huge:
    // On some CPUs, rep movsb is faster than SSE2 for absolutely humongous copies.
    if (__libc_have_erms && __n >= _ENHANCED_REP_MOVSB_THRESHOLD) {
        __asm__ __volatile__
            ("rep movsb"
             : "+S"(__src), "+D"(__dest), "+c"(__n)
             : // No inputs.
             : "memory");
        return;
    }

    // Copy 128 bytes at a time using SSE2.
    for (;;) {
        // SSE2 loop.
        _mm_prefetch((const char*) __src + 64, _MM_HINT_T0);

        _mm_storeu_si128((__m128i*) __dest + 0, _mm_loadu_si128((const __m128i*) __src + 0));
        _mm_storeu_si128((__m128i*) __dest + 1, _mm_loadu_si128((const __m128i*) __src + 1));
        _mm_storeu_si128((__m128i*) __dest + 2, _mm_loadu_si128((const __m128i*) __src + 2));
        _mm_storeu_si128((__m128i*) __dest + 3, _mm_loadu_si128((const __m128i*) __src + 3));
        _mm_storeu_si128((__m128i*) __dest + 4, _mm_loadu_si128((const __m128i*) __src + 4));
        _mm_storeu_si128((__m128i*) __dest + 5, _mm_loadu_si128((const __m128i*) __src + 5));
        _mm_storeu_si128((__m128i*) __dest + 6, _mm_loadu_si128((const __m128i*) __src + 6));
        _mm_storeu_si128((__m128i*) __dest + 7, _mm_loadu_si128((const __m128i*) __src + 7));

        __src = (const char*) __src + 128;
        __dest = (char*) __dest + 128;
        __n -= 128;
        if (__n < 128) goto __rest;
    }
}

#endif // __SSE2__

/// Fallback if the CPU doesn't support SSE2.
///
/// Declared in the header so that it can be inlined.
__inline__ void* __memcpy_naive(void* __restrict__ __dest, const void* __restrict__ __src, __SIZE_TYPE__ __n) {
    void* __original_dst = __dest;
    __asm__ __volatile__
        ("rep movsb"
         : "+D"(__dest), "+S"(__src), "+c"(__n)
         : // No inputs
         : "memory");
    return __original_dst;
}

__END_DECLS__

#pragma GCC diagnostic pop

#endif //_LENSOR_OS_LIBC_STRING_INTRINSICS_H
