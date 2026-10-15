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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#define BITMASK32(LENGTH, SHIFT)                                                        \
    ({                                                                                  \
        static_assert((SHIFT) + (LENGTH) <= 32, "Length exceeded possible by bitmask"); \
        (((LENGTH) == 0)                                                                \
             ? ((u32)0)                                                                 \
             : (((LENGTH) >= 32)                                                        \
                    ? ~((u32)0)                                                         \
                    : ((~((u32)0) >> (32 - (LENGTH))) << (SHIFT))));                    \
    })
#define BITMASK64(LENGTH, SHIFT)                                                        \
    ({                                                                                  \
        static_assert((SHIFT) + (LENGTH) <= 64, "Length exceeded possible by bitmask"); \
        (((LENGTH) == 0)                                                                \
             ? ((u64)0)                                                                 \
             : (((LENGTH) >= 64)                                                        \
                    ? ~((u64)0)                                                         \
                    : ((~((u64)0) >> (64 - (LENGTH))) << (SHIFT))));                    \
    })
#define GENMASK(H, L) BITMASK64((1 + ((H) - (L))), L)
#define BIT(N) BITMASK64(1, N)
