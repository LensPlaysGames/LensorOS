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

#ifndef _LENSOR_OS_LIBC_POSIX_STRINGS_H
#define _LENSOR_OS_LIBC_POSIX_STRINGS_H

#include <bits/decls.h>
#include <locale.h>
#include <stddef.h>

__BEGIN_DECLS__

int ffs(int);
int strcasecmp(const char*, const char*);
int strcasecmp_l(const char*, const char*, locale_t);
int strncasecmp(const char*, const char*, size_t);
int strncasecmp_l(const char*, const char*, size_t, locale_t);

__END_DECLS__

#endif
