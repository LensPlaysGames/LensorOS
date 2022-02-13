#ifndef LENSOR_OS_MATH_H
#define LENSOR_OS_MATH_H

/// Vector with a fixed width of two unsigned 64-bit integer numbers.
struct uVector2 {
    u64 x;
    u64 y;

    uVector2() {
        x = 0;
        y = 0;
    }

    uVector2(u64 _x, u64 _y) {
        x = _x;
        y = _y;
    }

    friend inline bool operator == (const uVector2& lhs, const uVector2& rhs) {
        return (lhs.x == rhs.x && lhs.y == rhs.y);
    }
    friend inline bool operator != (const uVector2& lhs, const uVector2& rhs) {
        return !(lhs == rhs);
    }
    friend inline uVector2 operator + (const uVector2& lhs, const uVector2& rhs) {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }
    friend inline uVector2 operator - (const uVector2& lhs, const uVector2& rhs) {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }
    friend inline uVector2 operator * (const uVector2& lhs, const uVector2& rhs) {
        return {lhs.x * rhs.x, lhs.y * rhs.y};
    }
    friend inline uVector2 operator / (const uVector2& lhs, const uVector2& rhs) {
        return {lhs.x / rhs.x, lhs.y / rhs.y};
    }
};

#endif
