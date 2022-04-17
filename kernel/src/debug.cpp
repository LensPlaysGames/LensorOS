#include <debug.h>

#include <basic_renderer.h>
#include <cstr.h>
#include <integers.h>
#include <uart.h>
#include <va_list.h>

/* Supported formats:
 *
 *  %s     -- null terminated C-style string
 *
 *  %hhu   -- 8 bit unsigned integer
 *  %hu    -- 16 bit unsigned integer
 *  %u     -- native bit width unsigned integer
 *  %ul    -- 32 bit unsigned integer
 *  %ull   -- 64 bit unsigned integer
 *
 *  %hhi   -- 8 bit signed integer
 *  %hi    -- 16 bit signed integer
 *  %i     -- native bit width signed integer
 *  %il    -- 32 bit signed integer
 *  %ill   -- 64 bit signed integer
 *
 *  %f     -- double, 2 digits of precision
 *  %x     -- 16 digit 64 bit unsigned integer (hexadecimal)
 *  %c     -- Character
 */
void dbgmsg_v(const u8* fmt, va_list args) {
    const u8* current = fmt;
    for(; *current != '\0'; ++current) {
        if (*current == '%') {
            current++;
            switch (*current) {
            case '\0':
                // Handle '%' at end of format string.
                dbgmsg(static_cast<char>('%'));
                return;
            case 's':
                // Found %s -- string
                dbgmsg(va_arg(args, const char*));
                break;
            case 'h':
                // %h
                current++;
                // %h? <- ? is *current
                if (*current != 'i'
                    && *current != 'u'
                    && *current != 'h')
                {
                    // Found %h -- nothing
                    current--;
                    break;
                }
                if (*current == 'i') {
                    // Found %hi -- 16 bit signed integer
                    dbgmsg(static_cast<s16>(va_arg(args, int)));
                }
                if (*current == 'u') {
                    // Found %hu -- 16 bit unsigned integer
                    dbgmsg(static_cast<u16>(va_arg(args, int)));
                }
                if (*current == 'h') {
                    current++;
                    if (*current != 'i'
                        && *current != 'u')
                    {
                        // Found %hh -- nothing
                        current--;
                        break;
                    }
                    if (*current == 'i') {
                        // Found %hhi -- 8 bit signed integer
                        dbgmsg(static_cast<s8>(va_arg(args, int)));
                        break;
                    }
                    if (*current == 'u') {
                        // Found %hhu -- 8 bit unsigned integer
                        dbgmsg(static_cast<u8>(va_arg(args, int)));
                        break;
                    }
                }
                break;
            case 'u':
                current++;
                if (*current != 'l') {
                    // Found %u -- native bit unsigned integer
                    dbgmsg(va_arg(args, unsigned));
                    current--;
                    break;
                }
                current++;
                if (*current != 'l') {
                    // Found %ul -- 32 bit unsigned integer
                    dbgmsg(va_arg(args, u32));
                    current--;
                    break;
                }
                // Found %ull -- 64 bit unsigned integer
                dbgmsg(va_arg(args, u64));
                break;
            case 'd':
            case 'i':
                current++;
                if (*current != 'l') {
                    // Found %u -- native bit signed integer
                    dbgmsg(va_arg(args, signed));
                    current--;
                    break;
                }
                current++;
                if (*current != 'l') {
                    // Found %ul -- 32 bit unsigned integer
                    dbgmsg(va_arg(args, s32));
                    current--;
                    break;
                }
                // Found %ull -- 64 bit unsigned integer
                dbgmsg(va_arg(args, s64));
                break;
            case 'f':
                dbgmsg((va_arg(args, double)));
                break;
            case 'x':
                dbgmsg_s("0x");
                dbgmsg_s(to_hexstring(va_arg(args, u64)));
                break;
            case 'c':
                dbgmsg(static_cast<char>(va_arg(args, int)));
                break;
            default:
                dbgmsg(static_cast<char>(*current));
                break;
            }
        }
        else dbgmsg(static_cast<char>(*current));
    }
}

void dbgmsg_s(const char* str) {
    UART::out(str);
}

void dbgmsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    dbgmsg_v(reinterpret_cast<const u8*>(fmt), args);
    va_end(args);
}

void dbgmsg(char character, ShouldNewline nl) {
    UART::outc(character);
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}

void dbgmsg(u8* buffer, u64 byteCount, ShouldNewline nl) {
    UART::out(buffer, byteCount);
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}

void dbgmsg(double number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}

void dbgmsg(s64 number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}

void dbgmsg(s32 number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}

void dbgmsg(s16 number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}

void dbgmsg(s8 number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}

void dbgmsg(u64 number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}
void dbgmsg(u32 number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}
void dbgmsg(u16 number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}
void dbgmsg(u8 number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        UART::out("\r\n");
}
