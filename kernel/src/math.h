#ifndef LENSOR_OS_MATH_H
#define LENSOR_OS_MATH_H

struct Vector2 {
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

	friend inline bool operator==(const Vector2& lhs, const Vector2& rhs) {
		return (lhs.x == rhs.x && lhs.y == rhs.y);
	}
	friend inline bool operator!=(const Vector2& lhs, const Vector2& rhs) {
		return !(lhs == rhs);
	}
};

#endif
