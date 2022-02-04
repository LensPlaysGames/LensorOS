#ifndef LENSOR_OS_CSTR_H
#define LENSOR_OS_CSTR_H

#include "integers.h"

char* to_string(u64);
char* to_string(u32);
char* to_string(u16);
char* to_string(u8);
char* to_string(s64);
char* to_string(s32);
char* to_string(s16);
char* to_string(s8);
char* to_string(double, u8 decimalPlaces = 2);
char* to_hexstring(u64, bool capital = false);

#endif
