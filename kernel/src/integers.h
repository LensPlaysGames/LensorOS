#ifndef LENSOR_OS_INTEGERS_H
#define LENSOR_OS_INTEGERS_H

// Included with compiler, not std library
//   (which makes the name very confusing).
#include <cstddef>
#include <cstdint>

/// Unsigned Integer Alias Declaration
using uint = unsigned int;

/// Fixed-Width Unsigned Integer Alias Declarations
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
/// Fixed-Width Signed Integer Alias Declarations
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

#endif
