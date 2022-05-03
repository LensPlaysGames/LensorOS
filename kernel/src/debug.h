#ifndef LENSOR_OS_DEBUG_H
#define LENSOR_OS_DEBUG_H

#include <integers.h>
#include <string.h>
#include <va_list.h>

enum class ShouldNewline {
    Yes = 0,
    No = 1
};

/// Print a single character.
void dbgmsg_c(char);

/// Print a C-style null-terminated string with no formatting.
void dbgmsg_s(const char* str);

/// Print a single character with an optional newline.
void dbgmsg(char, ShouldNewline nl = ShouldNewline::No);

/// Print a number of bytes from a given buffer as characters.
void dbgmsg(u8* buffer, u64 byteCount, ShouldNewline nl = ShouldNewline::No);

/// Print the raw bytes of a string.
void dbgmsg(const String&, ShouldNewline nl = ShouldNewline::No);

void dbgmsg(double, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(s64, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(s32, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(s16, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(s8, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(u64, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(u32, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(u16, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(u8, ShouldNewline nl = ShouldNewline::No);

/// Print a formatted string.
///  Supported formats:
///   %s     -- null terminated C-style string
///   %c     -- character
///   %b     -- boolean
///   %hhu   -- 8 bit unsigned integer
///   %hu    -- 16 bit unsigned integer
///   %u     -- native bit width unsigned integer
///   %ul    -- 32 bit unsigned integer
///   %ull   -- 64 bit unsigned integer
///   %hhi   -- 8 bit signed integer
///   %hi    -- 16 bit signed integer
///   %i     -- native bit width signed integer
///   %il    -- 32 bit signed integer
///   %ill   -- 64 bit signed integer
///   %f     -- double, 2 digits of precision
///   %x,%p  -- 16 hexadecimal-digit 64 bit unsigned integer
///   %sl    -- LensorOS `String`
void dbgmsg(const char* fmt, ...);

/// Print a string with lots of colors (and no formatting)! Nyan debug :^)
void dbgrainbow(const String&, ShouldNewline nl = ShouldNewline::No);

/// Print a C-style null-terminated string in lots
/// of colors (and no formatting)! Nyan debug :^)
void dbgrainbow(const char* str, ShouldNewline nl = ShouldNewline::No);

#endif /* LENSOR_OS_DEBUG_H */
