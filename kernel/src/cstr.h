#ifndef LENSOR_OS_CSTR_H
#define LENSOR_OS_CSTR_H

#include <stdint.h>

char* to_string(uint64_t value);
char* to_string(int64_t value);
char* to_string(double value, uint8_t decimalPlaces = 2);

char* to_hexstring(uint64_t value);

#endif
