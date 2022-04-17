#ifndef LENSOR_OS_DEBUG_H
#define LENSOR_OS_DEBUG_H

#include <integers.h>
#include <va_list.h>

enum class ShouldNewline {
    Yes = 0,
    No = 1
};

/// Print a C-style null-terminated string.
void dbgmsg_s(const char* str);
/// Print byteCount bytes starting at buffer.
void dbgmsg(u8* buffer, u64 byteCount
            , ShouldNewline nl = ShouldNewline::No);

void dbgmsg_v(u8* fmt, va_list);
/* Print a formatted string.
 * Supported formats:
 * `-- "%s" -- C-style null-terminated string
 */
void dbgmsg(const char* fmt, ...);


void dbgmsg(char, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(double, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(s64, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(s32, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(s16, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(s8, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(u64, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(u32, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(u16, ShouldNewline nl = ShouldNewline::No);
void dbgmsg(u8, ShouldNewline nl = ShouldNewline::No);

#endif /* LENSOR_OS_DEBUG_H */
