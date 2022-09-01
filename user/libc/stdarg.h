/* Copyright 2022, Contributors To LensorOS.
All rights reserved.

This file is part of LensorOS.

LensorOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LensorOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LensorOS. If not, see <https://www.gnu.org/licenses/>. */


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
