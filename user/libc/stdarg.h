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


#ifndef _STDARG_H
#define _STDARG_H

#include "bits/decls.h"

__BEGIN_DECLS__

typedef __builtin_va_list va_list;

#define va_start(__ap,__last)   __builtin_va_start(__ap, __last)
#define va_end(__ap)            __builtin_va_end(__ap)
#define va_arg(__ap,__type)     __builtin_va_arg(__ap,__type)
#define va_copy(__dest, __src)  __builtin_va_copy(__dest,__src)

__END_DECLS__

#endif /* _STDARG_H */
