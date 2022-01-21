#ifndef LENSOR_OS_RTC_H
#define LENSOR_OS_RTC_H

#define CURRENT_YEAR 2022

#include <stdint.h>
#include "io.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

// RTC; Real-Time Clock
//   As close to real-time as it gets, at least. It's no atomic clock.
// REGISTER MAP
// 0x00 = Seconds [0-59]
// 0x02 = Minutes [0-59]
// 0x04 = Hours   [0-23] OR [1-12]
// 0x06 = Weekday [1-7]
// 0x07 = Date    [1-31]
// 0x08 = Month   [1-12]
// 0x09 = Year    [0-99]
// 0x32 = Century [19-20]
// 0x0a = Status A
// 0x0b = Status B

// TODO: Set century register from ACPI table.
//         For now, hard-code 0 (not supported)
#define CENTURY_REGISTER 0x00

struct RTCData {
	uint8_t  second;
	uint8_t  minute;
	uint8_t  hour;
	uint8_t  weekday;
	uint8_t  date;
	uint8_t  month;
	uint32_t year;
	uint8_t  century;

	RTCData() {}

	void operator=(const RTCData& other) {
		second  = other.second;
		minute  = other.minute;
		hour    = other.hour;
		weekday = other.weekday;
		date    = other.date;
		month   = other.month;
		year    = other.year;
		century = other.century;
	}
} ;

class RTC {
public:
	RTCData time;
	
	RTC() {
		// POPULATE `gRTC` STRUCT
	    get_date_time();
	}

private:
	inline uint8_t is_rtc_updating() {
		outb(CMOS_ADDR, 0x0a);
		return inb(CMOS_DATA) & 0x80;
	}
	
	inline uint8_t read_register(uint8_t reg) {
		outb(CMOS_ADDR, reg);
		return inb(CMOS_DATA);
	}

	void get_rtc_data(RTCData& data);
    void get_date_time();
};

extern RTC gRTC;

#endif
