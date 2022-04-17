#include <cstr.h>

const char to_hex_not_supported[] = "TYPE_NOT_SUPPORTED";

bool strcmp(const char* a, const char* b, u64 length) {
    for (u64 i = 0; i < length; ++i) {
        if (*a != *b)
            return false;

        ++a;
        ++b;
    }
    return true;
}

const char* t = "\033[32mTrue\033[0m";
const char* f = "\033[31mFalse\033[0m";
const char* to_string(bool b) {
    return b ? t : f;
}

char uint_to_str_buf[LENSOR_OS_TO_STRING_BUF_SZ];
char* to_string(u64 value) {
    // Get necessary string length.
    u8 length = 0;
    u64 val_cpy = value;
    while (val_cpy / 10 > 0) {
        val_cpy /= 10;
        length++;
    }
    // Write characters to string (in reverse order starting at length).
    u8 index = 0;
    while (value / 10 > 0) {
        u8 remainder = value % 10;
        uint_to_str_buf[length - index] = remainder + '0';
        value /= 10;
        index++;
    }
    // Loop doesn't catch first digit.
    u8 remainder = value % 10;
    uint_to_str_buf[length - index] = remainder + '0';
    // Null terminate the string
    uint_to_str_buf[length + 1] = 0;
    return uint_to_str_buf;
}

char* to_string(u32 value) { return to_string((u64)value); }
char* to_string(u16 value) { return to_string((u64)value); }
char* to_string(u8  value) { return to_string((u64)value); }

char int_to_str_buf[LENSOR_OS_TO_STRING_BUF_SZ];
char* to_string(s64 value) {
    // Add negation symbol before negative values.
    u8 isNegative = 0;
    if (value < 0) {
        isNegative = 1;
        value *=- 1;
        int_to_str_buf[0] = '-';
    }
    // Get necessary string length.
    u8 length = 0;
    u64 val_cpy = value;
    while (val_cpy / 10 > 0) {
        val_cpy /= 10;
        length++;
    }
    // Write characters to string (in reverse order starting at length).
    u8 index = 0;
    while (value / 10 > 0) {
        u8 remainder = value % 10;
        int_to_str_buf[isNegative + length - index] = remainder + '0';
        value /= 10;
        index++;
    }
    // Loop doesn't catch first digit.
    u8 remainder = value % 10;
    int_to_str_buf[isNegative + length - index] = remainder + '0';
    // Null terminate the string
    int_to_str_buf[isNegative + length + 1] = 0;
    return int_to_str_buf;
}

char* to_string(s32 value) { return to_string((s64)value); }
char* to_string(s16 value) { return to_string((s64)value); }
char* to_string(s8  value) { return to_string((s64)value); }

char dbl_to_str_buf[LENSOR_OS_TO_STRING_BUF_SZ_DBL];
char* to_string(double value, u8 decimalPlaces) {
    // Max decimal places = 20
    if (decimalPlaces > 20) { decimalPlaces = 20; }
    char* int_ptr = (char*)to_string((s64)value);
    char* double_ptr = dbl_to_str_buf;
    if (value < 0) {
        value *= -1;
    }
    while (*int_ptr != 0) {
        *double_ptr = *int_ptr;
        int_ptr++;
        double_ptr++;
    }
    *double_ptr = '.';
    double_ptr++;
    double newVal = value - (s32)value;
    for (u8 i = 0; i < decimalPlaces; i++) {
        newVal *= 10;
        *double_ptr = (s32)newVal + '0';
        newVal -= (s32)newVal;
        double_ptr++;
    }
    // Null termination
    *double_ptr = 0;
    return dbl_to_str_buf;
}

// Shout-out to User:Pancakes on the osdev wiki!
// https://wiki.osdev.org/User:Pancakes
const u8 size = 15;
const char* hexmap = "0123456789abcdef";
const char* hexmap_capital = "0123456789ABCDEF";
char hex_to_string_buf[LENSOR_OS_TO_STRING_BUF_SZ];
char* to_hexstring(u64 value, bool capital) {
    s8 n = (s8)size;
    u8 i {0};
    for (; n >= 0; --n) {
        u8 tmp = (value >> (n * 4)) & 0xf;
        if (capital)
            hex_to_string_buf[i] = hexmap_capital[tmp];
        else hex_to_string_buf[i] = hexmap[tmp];
        ++i;
    }
    hex_to_string_buf[i] = 0;
    return hex_to_string_buf;
}

char* to_hexstring(void* ptr, bool capital) {
    return to_hexstring((u64)ptr, capital);
}
