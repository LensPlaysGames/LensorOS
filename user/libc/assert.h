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

#ifndef _LENSOR_OS_ASSERT_H
#define _LENSOR_OS_ASSERT_H

/// Report a failed assertion.
__attribute__((__noreturn__)) void __assert_abort(
    const char *expr,
    const char *file,
    unsigned int line,
    const char *func
);

/// Re-define assert() every time this header is included.
#ifdef assert
#   undef assert
#endif

/// Run-time assertions are pretty useful, so we *don't* compile
/// out assertions in release mode. I'm sure there is *some* reading
/// of the standard that involves some loophole that allows this.
///
/// `(void) sizeof ((expr) ? 1 : 0)` is a hack also used by GCC that
/// ensures that we don't swallow any warnings while also making sure
/// that we dont' evaluate `cond` twice.
///
/// __extension__ is a GCC extension that allows us to use other GNU
/// extensions without triggering any -pedantic warnings.
#define assert(cond) ((void) sizeof ((cond) ? 1 : 0), __extension__ ({  \
    if (!(cond)) {                                                      \
        __assert_abort(#cond, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
    }                                                                   \
}))                                                                     \

#endif // _LENSOR_OS_ASSERT_H
