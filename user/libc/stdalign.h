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


#ifndef _STDALIGN_H
#define _STDALIGN_H

#if defined (__cplusplus)
extern "C" {
#endif

#ifndef __cplusplus

#if __STDC_VERSION__ < 201112L && defined(__GNUC__)
#define _Alignas(t) __attribute__((__aligned__(t)))
#define _Alignof(t) __alignof__(t)
#endif /* C11 or compiler extensions */

#define alignas _Alignas
#define alignof _Alignof

#endif /* __cplusplus */

#define __alignas_is_defined 1
#define __alignof_is_defined 1

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif /* _STDALIGN_H */
