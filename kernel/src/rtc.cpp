#include "rtc.h"

RTC gRTC;

/// Set IRQ8 periodic interrupt enabled or disabled.
/// NOTE: Must be called when interrupts are disabled (in-between `cli` and `sti`)!
void RTC::set_periodic_int_enabled(bool enabled) {
    u8 statusB = read_register(0x8b);
	if (enabled) {
		// Enable periodic interrupt.
		outb(CMOS_ADDR, 0x8b);
		outb(CMOS_DATA, statusB | 0b01000000);
	}
	else {
		// Disable periodic interrupt.
		outb(CMOS_ADDR, 0x8b);
		outb(CMOS_DATA, statusB & 0b10111111);
	}
}

void RTC::update_rtc_data(RTCData& data) {
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
	update_rtc_data(Time);
	RTCData newTime;
	// SPIN UNTIL RTC IS NOT UPDATING AGAIN
	//   This paired with the following comparison
	//     will ensure a time is only returned when
	//     it has not been updated throughout this
	//     function's execution.
	while (is_rtc_updating() != 0);
	do {
		update_rtc_data(newTime);
		// Wait for update to read again.
		while (is_rtc_updating() != 0);
		update_rtc_data(Time);
	}
	while ((newTime.second     != Time.second)
		   || (newTime.minute  != Time.minute)
		   || (newTime.hour    != Time.hour)
		   || (newTime.weekday != Time.weekday)
		   || (newTime.date    != Time.date)
		   || (newTime.month   != Time.month)
		   || (newTime.year    != Time.year)
		   || (newTime.century != Time.century));

	u8 statusB = read_register(0x0b);

	// BCD -> BINARY
	if (!(statusB & 0b00000100)) {
		Time.second = (Time.second & 0x0f)
			+ ((Time.second / 16)
			   * 10);
		Time.minute = (Time.minute & 0x0f)
			+ ((Time.minute / 16)
			   * 10);
		Time.hour = ((Time.hour & 0x0f)
					  + (((Time.hour & 0x70) / 16)
					  * 10))
			          | (Time.hour & 0x80);
		Time.weekday = (Time.weekday & 0x0f) + ((Time.weekday / 16) * 10);
		Time.date = (Time.date & 0x0f) + ((Time.date / 16) * 10);
		Time.month = (Time.month & 0x0f) + ((Time.month / 16) * 10);
		Time.year = (Time.year & 0x0f) + ((Time.year / 16) * 10);
		if (CENTURY_REGISTER != 0) {
			Time.century = (Time.century & 0x0f) + ((Time.century / 16) * 10);
		}
	}

	// 12 hr -> 24 hr
	if ((statusB & 0b00000010) && (Time.hour & 0x80)) {
		Time.hour = ((Time.hour & 0x7f) + 12) % 24;
	}

	// Century + Year -> YYYY
	if (CENTURY_REGISTER != 0) {
		Time.year += Time.century * 100;
	}
	else {
		Time.year += (CURRENT_YEAR / 100) * 100;
		if (Time.year < CURRENT_YEAR) {
			// FIX THE 1900s
			Time.year += 100;
		}
	}
}
