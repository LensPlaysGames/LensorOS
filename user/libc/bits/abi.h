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

#ifndef LENSOROS_ABI_H
#define LENSOROS_ABI_H

#include <bits/decls.h>

__BEGIN_DECLS__
using __dso_cb = void (*)(void*);
using __dso_cb_arg = void*;
using __dso_handle_t = void*;

extern bool __in_quick_exit;

void __cxa_atexit(__dso_cb func, __dso_cb_arg arg, __dso_handle_t dso);
__END_DECLS__

#endif // LENSOROS_ABI_H
