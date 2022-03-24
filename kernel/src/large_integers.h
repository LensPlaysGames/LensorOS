#ifndef LENSOR_OS_LARGE_INTEGERS_H
#define LENSOR_OS_LARGE_INTEGERS_H

#include "integers.h"

/// 16 BYTES
struct uint128_t {
    u64 a;
    u64 b;
} __attribute__((packed));
using u128 = uint128_t;

/// 32 BYTES
struct uint256_t {
    u64 a;
    u64 b;
    u64 c;
    u64 d;
} __attribute__((packed));
using u256 = uint256_t;

/// 64 BYTES
struct uint512_t {
    u256 a;
    u256 b;
} __attribute__((packed));
using u512 = uint512_t;


/// 128 BYTES
struct uint1024_t {
    u512 a;
    u512 b;
} __attribute__((packed));
using u1024 = uint1024_t;

/// 512 BYTES
struct uint4096_t {
    u1024 a;
    u1024 b;
    u1024 c;
    u1024 d;
} __attribute__((packed));
using u4096 = uint4096_t;

/// 2048 BYTES
struct uint16384_t {
    u4096 a;
    u4096 b;
    u4096 c;
    u4096 d;
} __attribute__((packed));
using u16384 = uint16384_t;

/// 8192 BYTES
struct uint65536_t {
    u16384 a;
    u16384 b;
    u16384 c;
    u16384 d;
} __attribute__((packed));
using u65536 = uint65536_t;

/// 32768 BYTES
struct uint262144_t {
    u65536 a;
    u65536 b;
    u65536 c;
    u65536 d;
} __attribute__((packed));
using u262144 = uint262144_t;


/// 131072 BYTES
struct uint1048576_t {
    u262144 a;
    u262144 b;
    u262144 c;
    u262144 d;
} __attribute__((packed));
using u1048576 = uint1048576_t;

#endif
