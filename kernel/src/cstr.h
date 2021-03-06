#ifndef LENSOR_OS_CSTR_H
#define LENSOR_OS_CSTR_H

#include <integers.h>

/* String Length
 *   Returns the number of characters including null terminator.
 */
u64 strlen(const char* a);

/* String Compare
 *   Returns `true` only if all characters within
 *   both strings up to length are exactly equal.
 */
bool strcmp(const char* a, const char* b, u64 length);

const char* to_string(bool);

constexpr u64 LENSOR_OS_TO_STRING_BUF_SZ = 20;
constexpr u64 LENSOR_OS_TO_STRING_BUF_SZ_DBL = 40;

char* to_string(u64);
char* to_string(u32);
char* to_string(u16);
char* to_string(u8);
char* to_string(s64);
char* to_string(s32);
char* to_string(s16);
char* to_string(s8);
char* to_string(double, u8 decimalPlaces = 2);
char* to_hexstring(u64 value, bool capital = false);
extern const char to_hex_not_supported[];
template <typename T>
char* to_hexstring(T value, bool capital = false) {
    // FIXME: This is really, really bad!
    u8 sz = sizeof(T);
    if (sz == 1)
        return to_hexstring((u64)value, capital);
    if (sz == 2)
        return to_hexstring((u64)value, capital);
    if (sz == 4)
        return to_hexstring((u64)value, capital);
    if (sz == 8)
        return to_hexstring((u64)value, capital);
    else return (char*)to_hex_not_supported;
}

#endif
