#ifndef LENSOR_OS_LARGE_INTEGERS_H
#define LENSOR_OS_LARGE_INTEGERS_H

#include "integers.h"

struct uint256_t {
	u64 a;
	u64 b;
	u64 c;
	u64 d;
} __attribute__((packed));
using u256 = uint256_t;

struct uint1024_t {
	u256 a;
	u256 b;
	u256 c;
	u256 d;
} __attribute__((packed));
using u1024 = uint1024_t;

#endif
