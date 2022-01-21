#include "rtc.h"

RTC gRTC;

void RTC::get_rtc_data(RTCData& data) {
	data.second  = read_register(0x00);
	data.minute  = read_register(0x02);
	data.hour    = read_register(0x04);
	data.weekday = read_register(0x06);
	data.date    = read_register(0x07);
	data.month   = read_register(0x08);
	data.year    = read_register(0x09);
	if (CENTURY_REGISTER != 0) {
		data.century = read_register(0x32);
	}
}
	
void RTC::get_date_time() {
	// SPIN UNTIL RTC IS NOT UPDATING
	//   This may be up to one second.
	while (is_rtc_updating() != 0);
	get_rtc_data(time);
	RTCData new_time;
	// SPIN UNTIL RTC IS NOT UPDATING AGAIN
	//   This paired with the following comparison
	//     will ensure a time is only returned when
	//     it has not been updated throughout this
	//     function's execution.
	while (is_rtc_updating() != 0);
	do {
		get_rtc_data(new_time);
		// Wait for update to read again.
		while (is_rtc_updating() != 0);
		get_rtc_data(time);
	}
	while ((new_time.second  != time.second)
		   || (new_time.minute  != time.minute)
		   || (new_time.hour    != time.hour)
		   || (new_time.weekday != time.weekday)
		   || (new_time.date    != time.date)
		   || (new_time.month   != time.month)
		   || (new_time.year    != time.year)
		   || (new_time.century != time.century));

	uint8_t statusB = read_register(0x0b);

	// BCD -> BINARY
	if (!(statusB & 0b00000100)) {
		time.second = (time.second & 0x0f)
			+ ((time.second / 16)
			   * 10);
		time.minute = (time.minute & 0x0f)
			+ ((time.minute / 16)
			   * 10);
		time.hour = ((time.hour & 0x0f)
					  + (((time.hour & 0x70) / 16)
					  * 10))
			          | (time.hour & 0x80);
		time.weekday = (time.weekday & 0x0f) + ((time.weekday / 16) * 10);
		time.date = (time.date & 0x0f) + ((time.date / 16) * 10);
		time.month = (time.month & 0x0f) + ((time.month / 16) * 10);
		time.year = (time.year & 0x0f) + ((time.year / 16) * 10);
		if (CENTURY_REGISTER != 0) {
			time.century = (time.century & 0x0f) + ((time.century / 16) * 10);
		}
	}

	// 12 hr -> 24 hr
	if ((statusB & 0b00000010) && (time.hour & 0x80)) {
		time.hour = ((time.hour & 0x7f) + 12) % 24;
	}

	// CCYY -> YYYY
	if (CENTURY_REGISTER != 0) {
		time.year += time.century * 100;
	}
	else {
		time.year += (CURRENT_YEAR / 100) * 100;
		if (time.year < CURRENT_YEAR) {
			// FIX THE 1900s
			time.year += 100;
		}
	}
}
