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

#ifndef _LENSOR_OS_DECLS_H
#define _LENSOR_OS_DECLS_H

#ifdef __cplusplus
#    define __BEGIN_DECLS__ extern "C" {
#    define __END_DECLS__ }
#    define __constexpr constexpr

/// Raise a compile error.
#    define __if if constexpr
#    define __elif else if constexpr
#    define __else else
#else
#    define __BEGIN_DECLS__
#    define __END_DECLS__
#    define __constexpr
#endif

/// Stringification and concatenation.
#define __STR(_X) #_X
#define _STR(_X) __STR(_X)
#define __CAT(_X, _Y) _X##_Y
#define _CAT(_X, _Y) __CAT(_X, _Y)

/// Attributes
#ifndef __forceinline
#    define __forceinline __inline__ __attribute__((__always_inline__))
#endif

#define _Deprecated(_Msg) __attribute__((__deprecated__(_Msg)))
#define _Format(_Fmt, ...) __attribute__((__format__(_Fmt, __VA_ARGS__)))
#define _Flatten __attribute__((__flatten__))

#ifdef __cplusplus
#    define _Nodiscard [[nodiscard]]
#else
#    define _Nodiscard __attribute__((__warn_unused_result__))
#endif

/// Pragmas
#define __Pragma(_Str) _Pragma(#_Str)

#define _PushIgnoreWarning(_W)      \
    _Pragma("GCC diagnostic push") \
    __Pragma(GCC diagnostic ignored _W)

#define _PopWarnings() _Pragma("GCC diagnostic pop")

/// Some C++ compilers don't define _Bool.
#ifdef __cplusplus
__BEGIN_DECLS__
typedef bool _Bool;
__END_DECLS__
#endif

#ifdef __cplusplus
template <typename _Callable>
struct __defer_instance {
    _Callable __func;
    __defer_instance(_Callable&& __f) : __func(__f) {}
    ~__defer_instance() { __func(); }
};

struct __defer_helper {
    template <typename _Callable>
    __defer_instance<_Callable> operator%(_Callable&& __f) {
        return __defer_instance<_Callable>(static_cast<_Callable&&>(__f));
    }
};

/// __defer { ... }; executes `...` at the end of the current block.
#define __defer auto _CAT(__defer_instance_, __COUNTER__) = __defer_helper{} % [&]()
#endif

#endif // _LENSOR_OS_DECLS_H
