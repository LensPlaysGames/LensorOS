#ifndef LENSOR_OS_RTC_H
#define LENSOR_OS_RTC_H

#define CURRENT_YEAR 2022

#include <integers.h>
#include <io.h>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

/*
RTC; Real-Time Clock
  As close to real-time as it gets, although it's no atomic clock.

Most of the data here was found at osdev:
  https://wiki.osdev.org/RTC

Any holes in the information on osdev were supplemented with this very technical, very helpful document:
  https://web.archive.org/web/20180102225942/http://www.walshcomptech.com/ohlandl/config/cmos_bank_0.html

REGISTER MAP
  0x00 = Seconds [0-59]
  0x02 = Minutes [0-59]
  0x04 = Hours   [0-23] OR [1-12]
  0x06 = Weekday [1-7]
  0x07 = Date    [1-31]
  0x08 = Month   [1-12]
  0x09 = Year    [0-99]
  0x32 = Century [19-20]
  0x0a = Status Register A
  0x0b = Status Register B
  0x0c = Status Register C

Binary Number Bit Numbering
         0b00000000
Bit 0:            ^
Bit 7:     ^

Status Register `A`:
  Bits 0-3: Rate (`rate` must be between one and fifteen.)
         4: Bank Control
       5-6: Select Divider
         7: Update-in-progress

Status Register `B`:
  Bit 0: Enable daylight saving
      1: 24-hour mode
      2: Date mode
      3: Enable Square Wave
      4: Enable update-ended interrupt
      5: Enable alarm interrupt
      6: Enable periodic interrupt
      7: Set clock

Status Register `C`:
  Bits 0-3: Reserved (do not touch)
         4: Update-ended interrupt
         5: Alarm interrupt
         6: Periodic Interrupt
         7: Interrupt Request (IRQ)

Status Register `D`:
  Bits 0-6: Reserved (do not touch)
       7: Valid RAM (CMOS RAM considered valid if this bit is set to a one)

`C` contains information about what type of periodic interrupt has occured.
  If `C` is not read from during an IRQ8, no more IRQ8s will be sent.
*/

// TODO: Set century register from ACPI table.
//         For now, hard-code 0 (not supported)
#define CENTURY_REGISTER 0x00

struct RTCData {
    u8  second  { 0 };
    u8  minute  { 0 };
    u8  hour    { 0 };
    u8  weekday { 0 };
    u8  date    { 0 };
    u8  month   { 0 };
    u32 year    { 0 };
    u8  century { 0 };

    RTCData() {}

    void operator = (const RTCData& other) {
        second  = other.second;
        minute  = other.minute;
        hour    = other.hour;
        weekday = other.weekday;
        date    = other.date;
        month   = other.month;
        year    = other.year;
        century = other.century;
    }
};

#if defined QEMU || defined VBOX || defined VMWARE
#define RTC_PERIODIC_RATE 10
#else
#define RTC_PERIODIC_RATE 6
#endif /* defined QEMU || defined VBOX || defined VMWARE */
#define RTC_PERIODIC_HERTZ (32768 >> (RTC_PERIODIC_RATE - 1))

class RTC {
public:
    RTCData Time;
    u64 Ticks { 0 };

    RTC() {
        update_data();
        /// Set divisor for a 1024hz periodic interrupt.
        u8 statusA = read_register(0x8a);
        out8(CMOS_ADDR, 0x8a);
        out8(CMOS_DATA, (statusA & 0x11110000) | RTC_PERIODIC_RATE);
    }

    /// When this returns zero, RTC is not currently updating.
    u8 is_rtc_updating() {
        out8(CMOS_ADDR, 0x0a);
        return in8(CMOS_DATA) & 0b10000000;
    }

    u8 read_register(u8 reg) {
        out8(CMOS_ADDR, reg);
        return in8(CMOS_DATA);
    }

    //double seconds_since_boot();

    void update_data();
    void set_periodic_int_enabled(bool);
private:
    void get_rtc_data(RTCData&);
};

extern RTC gRTC;

#endif
