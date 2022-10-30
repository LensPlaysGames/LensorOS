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

#ifndef _LENSOR_OS_LIBC_DECLS_H
#define _LENSOR_OS_LIBC_DECLS_H

#ifdef __cplusplus
#    define __BEGIN_DECLS__ extern "C" {
#    define __END_DECLS__ }

/// Raise a compile error.
#    define __if if constexpr
#    define __elif else if constexpr
#    define __else else
#else
#    define __BEGIN_DECLS__
#    define __END_DECLS__
#endif

#ifndef __forceinline
#    define __forceinline __inline__ __attribute__((__always_inline__))
#endif

#ifndef _Deprecated
#    define _Deprecated(_Msg) __attribute__((__deprecated__(_Msg)))
#endif

#ifndef _Format
#    define _Format(_Fmt, ...) __attribute__((__format__(_Fmt, __VA_ARGS__)))
#endif

#ifndef __Pragma
#    define __Pragma(_Str) _Pragma(#_Str)
#endif

#ifndef _IgnoreWarning
#    define _IgnoreWarning(_W, _X) __extension__({                       \
        _Pragma("GCC diagnostic push");                                  \
        __Pragma(GCC diagnostic ignored _W); \
        __typeof__(_X) __X = _X;                                         \
        _Pragma("GCC diagnostic pop");                                   \
        __X;                                                             \
    })
#endif

/// Some C++ compilers don't define _Bool.
#ifdef __cplusplus
__BEGIN_DECLS__
typedef bool _Bool;
__END_DECLS__
#endif


#endif // _LENSOR_OS_LIBC_DECLS_H
