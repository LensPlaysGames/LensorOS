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

#ifndef _LENSOROS_LIBC_CTYPE_H
#define _LENSOROS_LIBC_CTYPE_H

#include <bits/decls.h>

__BEGIN_DECLS__

__constexpr inline int isdigit(int c) { return c >= '0' && c <= '9'; }
__constexpr inline int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
__constexpr inline int isalnum(int c) { return isalpha(c) || isdigit(c); }
__constexpr inline int isxdigit(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
__constexpr inline int islower(int c) { return c >= 'a' && c <= 'z'; }
__constexpr inline int isupper(int c) { return c >= 'A' && c <= 'Z'; }

__END_DECLS__

#endif // _LENSOROS_LIBC_CTYPE_H
