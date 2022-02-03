#include "cstr.h"

char uint_to_str_buf[20];
char* to_string(u64 value) {
    // Get necessary string length.
    u8 length;
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

char int_to_str_buf[20];
char* to_string(s64 value) {
    // Add negation symbol before negative values.
    u8 isNegative = 0;
    if (value < 0) {
        isNegative = 1;
        value *=- 1;
        int_to_str_buf[0] = '-';
    }
    // Get necessary string length.
    u8 length;
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

char dbl_to_str_buf[40];
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

char hex_to_string_buf[128];
char* to_hexstring(u64 value) {
    u64* val_ptr = &value;
    u8* ptr;
    u8 tmp;
    const u8 size = 15;
    for (u8 i = 0; i < size; i++) {
        ptr = ((u8*)val_ptr + i);
        tmp = ((*ptr & 0xF0) >> 4);
        hex_to_string_buf[size - (i * 2 + 1)] = tmp + (tmp > 9 ? 55 : '0');
        tmp = ((*ptr & 0x0F));
        hex_to_string_buf[size - (i * 2)]     = tmp + (tmp > 9 ? 55 : '0');
    }
    hex_to_string_buf[size + 1] = 0;
    return hex_to_string_buf;
}
