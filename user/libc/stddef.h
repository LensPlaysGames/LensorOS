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
