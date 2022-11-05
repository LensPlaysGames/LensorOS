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

#ifndef _LIBC_COLOURS_H
#define _LIBC_COLOURS_H

#ifdef __lensor__
#define _RR ""
#define _R ""
#define _G ""
#define _Y ""
#define _B ""
#define _M ""
#define _C ""
#define _W ""
#define _N ""
#else
#define _RR "\033[1;31m"
#define _R "\033[31m"
#define _G "\033[32m"
#define _Y "\033[33m"
#define _B "\033[34m"
#define _M "\033[35m"
#define _C "\033[36m"
#define _W "\033[37m"
#define _N "\033[m"
#endif

#endif // _LIBC_COLOURS_H
