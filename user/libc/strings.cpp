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

/**
 ** POSIX <strings.h> Implementation
 **/

#include <ctype.h>
#include <strings.h>

int ffs(int i) {
#if __has_builtin(__builtin_ffs)
    return __builtin_ffs(i);
#else
    if (i == 0) return 0;

    int position = 1;
    // Keep shifting until the lowest bit is a 1
    while ((i & 1) == 0) {
        i >>= 1;
        position++;
    }

    return position;
#endif
}
int strcasecmp(const char* s1, const char* s2) {
    return strcasecmp_l(s1, s2, 0);
}
int strcasecmp_l(const char* s1, const char* s2, locale_t) {
    while (true) {
        auto c1 = tolower(*s1);
        auto c2 = tolower(*s2);
        if (c1 != c2 || c1 == '\0')
            return c1 - c2;

        ++s1;
        ++s2;
    }
    return 0;
}
int strncasecmp(const char* s1, const char* s2, size_t n) {
    return strncasecmp_l(s1, s2, n, 0);
}
int strncasecmp_l(const char* s1, const char* s2, size_t n, locale_t) {
    for (size_t i = 0; i < n; ++i) {
        auto c1 = tolower(s1[i]);
        auto c2 = tolower(s2[i]);
        if (c1 != c2 or c1 == '\0')
            return c1 - c2;
    }
    return 0;
}
