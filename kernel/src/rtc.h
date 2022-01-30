#ifndef LENSOR_OS_RTC_H
#define LENSOR_OS_RTC_H

#define CURRENT_YEAR 2022

#include "integers.h"
#include "io.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

// RTC; Real-Time Clock
//   As close to real-time as it gets, although it's no atomic clock.
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
	u8  second;
	u8  minute;
	u8  hour;
	u8  weekday;
	u8  date;
	u8  month;
	u32 year;
	u8  century;

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
	RTC() { get_date_time(); }
	void get_date_time();
	void set_interrupts_enabled(bool);
private:
	inline u8 is_rtc_updating();
	inline u8 read_register(u8 reg);
	void get_rtc_data(RTCData&);
};

extern RTC gRTC;

#endif
