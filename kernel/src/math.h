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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#ifndef LENSOR_OS_MATH_H
#define LENSOR_OS_MATH_H

#include <integers.h>

template<typename T>
struct Vector2 {
    T x;
    T y;
};

template<>
struct Vector2<u64> {
    u64 x;
    u64 y;

    Vector2() {
        x = 0;
        y = 0;
    }

    Vector2(u64 _x, u64 _y) {
        x = _x;
        y = _y;
    }

    friend inline bool operator == (const Vector2& lhs, const Vector2& rhs) {
        return (lhs.x == rhs.x && lhs.y == rhs.y);
    }
    friend inline bool operator != (const Vector2& lhs, const Vector2& rhs) {
        return !(lhs == rhs);
    }
    friend inline Vector2 operator + (const Vector2& lhs, const Vector2& rhs) {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }
    friend inline Vector2 operator - (const Vector2& lhs, const Vector2& rhs) {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }
    friend inline Vector2 operator * (const Vector2& lhs, const Vector2& rhs) {
        return {lhs.x * rhs.x, lhs.y * rhs.y};
    }
    friend inline Vector2 operator / (const Vector2& lhs, const Vector2& rhs) {
        return {lhs.x / rhs.x, lhs.y / rhs.y};
    }
};

#endif
