#ifndef LENSOR_OS_CSTR_H
#define LENSOR_OS_CSTR_H

#include "integers.h"

char* to_string(u64 value);
char* to_string(int64_t value);
char* to_string(double value, u8 decimalPlaces = 2);
char* to_hexstring(u64 value);

#endif
