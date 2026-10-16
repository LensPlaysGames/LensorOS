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

#ifndef LENSOROS_ENDIAN_H
#define LENSOROS_ENDIAN_H

#include <stdint.h>

#include <bit>

// Helper to handle Big-Endian to Host-Endian conversion
template <typename T>
constexpr T big_endian_to_cpu(T val) {
    if constexpr (std::endian::native == std::endian::little)
        return std::byteswap(val);

    return val;
}
template <typename T>
constexpr T* big_endian_to_cpu(T* val) {
    if constexpr (std::endian::native == std::endian::little)
        return (T*)std::byteswap(uintptr_t(val));

    return val;
}

// Helper to handle Little-Endian to Host-Endian conversion
template <typename T>
constexpr T little_endian_to_cpu(T val) {
    if constexpr (std::endian::native == std::endian::big)
        return std::byteswap(val);

    return val;
}
template <typename T>
constexpr T* little_endian_to_cpu(T* val) {
    if constexpr (std::endian::native == std::endian::big)
        return (T*)std::byteswap(uintptr_t(val));

    return val;
}

#endif /* LENSOROS_ENDIAN_H */
