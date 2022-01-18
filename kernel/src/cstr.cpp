#include "cstr.h"

char uint_to_str_buf[20];
char* to_string(uint64_t value) {
	// Get necessary string length.
	uint8_t length;
	uint64_t val_cpy = value;
	while (val_cpy / 10 > 0) {
		val_cpy /= 10;
		length++;
	}
	// Write characters to string (in reverse order starting at length).
	uint8_t index = 0;
	while (value / 10 > 0) {
		uint8_t remainder = value % 10;
		uint_to_str_buf[length - index] = remainder + '0';
		value /= 10;
		index++;
	}
	// Loop doesn't catch first digit.
    uint8_t remainder = value % 10;
	uint_to_str_buf[length - index] = remainder + '0';
	// Null terminate the string
	uint_to_str_buf[length + 1] = 0;
	return uint_to_str_buf;
}

char int_to_str_buf[20];
char* to_string(int64_t value) {
	// Add negation symbol before negative values.
	uint8_t isNegative = 0;
	if (value < 0) {
		isNegative = 1;
		value *=- 1;
		int_to_str_buf[0] = '-';
	}
	// Get necessary string length.
	uint8_t length;
	uint64_t val_cpy = value;
	while (val_cpy / 10 > 0) {
		val_cpy /= 10;
		length++;
	}
	// Write characters to string (in reverse order starting at length).
	uint8_t index = 0;
	while (value / 10 > 0) {
		uint8_t remainder = value % 10;
		int_to_str_buf[isNegative + length - index] = remainder + '0';
		value /= 10;
		index++;
	}
	// Loop doesn't catch first digit.
    uint8_t remainder = value % 10;
	int_to_str_buf[isNegative + length - index] = remainder + '0';
	// Null terminate the string
	int_to_str_buf[isNegative + length + 1] = 0;
	return int_to_str_buf;
}

char dbl_to_str_buf[40];
char* to_string(double value, uint8_t decimalPlaces) {
	// Max decimal places = 20
	if (decimalPlaces > 20) { decimalPlaces = 20; }
	char* int_ptr = (char*)to_string((int64_t)value);
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
	double newVal = value - (int)value;
	for (uint8_t i = 0; i < decimalPlaces; i++) {
		newVal *= 10;
		*double_ptr = (int)newVal + '0';
		newVal -= (int)newVal;
		double_ptr++;
	}
	// Null termination
	*double_ptr = 0;
	return dbl_to_str_buf;
}

char hex_to_string_buf[128];
char* to_hexstring(uint64_t value) {
	uint64_t* val_ptr = &value;
	uint8_t* ptr;
	uint8_t tmp;
	const uint8_t size = 15;
	for (uint8_t i = 0; i < size; i++) {
		ptr = ((uint8_t*)val_ptr + i);
		tmp = ((*ptr & 0xF0) >> 4);
		hex_to_string_buf[size - (i * 2 + 1)] = tmp + (tmp > 9 ? 55 : '0');
		tmp = ((*ptr & 0x0F));
		hex_to_string_buf[size - (i * 2)]     = tmp + (tmp > 9 ? 55 : '0');
	}
	hex_to_string_buf[size + 1] = 0;
	return hex_to_string_buf;
}
