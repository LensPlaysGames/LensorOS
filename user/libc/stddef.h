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


#ifndef _STDDEF_H
#define _STDDEF_H

#include "bits/decls.h"

#ifdef __cplusplus
#   define NULL nullptr
typedef decltype(nullptr) nullptr_t;
#else
#   define NULL ((void*)0)
    typedef __WCHAR_TYPE__ wchar_t;
#endif

#define offsetof(type, member) __builtin_offsetof(type, member)

__BEGIN_DECLS__

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __SIZE_TYPE__ size_t;
typedef __WINT_TYPE__ wint_t;

typedef struct {
    long long __ll __attribute__((__aligned__(__alignof__(long long))));
    long double __ld __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;

__END_DECLS__

#endif /* _STDDEF_H */
