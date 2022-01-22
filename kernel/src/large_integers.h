#ifndef LENSOR_OS_LARGE_INTEGERS_H
#define LENSOR_OS_LARGE_INTEGERS_H

struct uint256_t {
	uint64_t a;
	uint64_t b;
	uint64_t c;
	uint64_t d;
} __attribute__((packed));

struct uint1024_t {
	uint256_t a;
	uint256_t b;
	uint256_t c;
	uint256_t d;
} __attribute__((packed));

#endif
