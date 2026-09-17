/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#ifndef LENSOR_OS_LAPIC_H
#define LENSOR_OS_LAPIC_H

/* Advanced Programmable Interrupt Controller
 * This serves as a driver for the I/O APIC (redirection for hardware
 * interrupts) as well as Local APIC configuration.
 *
 * See Intel Software Developer's Manual, Volume 3, Chapter 13
 *
 * x2APIC -> accessed through rdmsr, wrmsr instructions.
 *   must be placed into x2APIC mode.
 *   backwards-compatible with "regular", MMIO xAPIC, by default.
 *
 * xAPIC -> MMIO interface for configuring APIC.
 */

#include <stddef.h>
#include <stdint.h>

#define LAPIC_APIC_BASE_MSR_BOOTSTRAP_CPU (((uint32_t)1) << 8)
#define LAPIC_APIC_BASE_MSR_x2APIC (((uint32_t)1) << 10)
#define LAPIC_APIC_BASE_MSR_ENABLE (((uint32_t)1) << 11)

#define LAPIC_MSR_FROM_REGOFFSET(regoffset) (0x800 + ((regoffset) >> 4))

// Read/Write
#define LAPIC_REGOFFSET_ID 0x20
// Read-only
#define LAPIC_REGOFFSET_VERSION 0x30
// Read/Write
#define LAPIC_REGOFFSET_TASK_PRIORITY 0x80
// Read-only
#define LAPIC_REGOFFSET_ARBITRATION_PRIORITY 0x90
// Read-only
#define LAPIC_REGOFFSET_PROCESSOR_PRIORITY 0xa0
// Write-only
#define LAPIC_REGOFFSET_EOI 0xb0
// Read-only
#define LAPIC_REGOFFSET_REMOTE_READ 0xc0
// Read/Write
#define LAPIC_REGOFFSET_LOGICAL_DESTINATION 0xd0
#define LAPIC_REGOFFSET_DESTINATION_FORMAT 0xe0
#define LAPIC_REGOFFSET_SPURIOUS_INTERRUPT_VECTOR 0xf0
#define LAPIC_REGOFFSET_ERROR_STATUS 0x0280

#define LAPIC_REGOFFSET_LVT_TIMER 0x0320
#define LAPIC_REGOFFSET_LVT_THERMAL_SENSOR 0x0330
#define LAPIC_REGOFFSET_LVT_PERFORMANCE_MONITORING_COUNTERS 0x0340
#define LAPIC_REGOFFSET_LVT_LINT0 0x0350
#define LAPIC_REGOFFSET_LVT_LINT1 0x0360
#define LAPIC_REGOFFSET_LVT_ERROR 0x0370
#define LAPIC_REGOFFSET_INITIAL_COUNT 0x0380
#define LAPIC_REGOFFSET_CURRENT_COUNT 0x0390
//  Read/Write
#define LAPIC_REGOFFSET_DIVIDE_CONFIG 0x03e0

#define LAPIC_MSR_ID LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_ID)
#define LAPIC_MSR_VERSION LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_VERSION)
#define LAPIC_MSR_TASK_PRIORITY LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_TASK_PRIORITY)
#define LAPIC_MSR_ARBITRATION_PRIORITY LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_ARBITRATION_PRIORITY)
#define LAPIC_MSR_PROCESSOR_PRIORITY LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_PROCESSOR_PRIORITY)
#define LAPIC_MSR_EOI LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_EOI)
#define LAPIC_MSR_REMOTE_READ LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_REMOTE_READ)
#define LAPIC_MSR_LOGICAL_DESTINATION LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_LOGICAL_DESTINATION)
#define LAPIC_MSR_DESTINATION_FORMAT LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_DESTINATION_FORMAT)
#define LAPIC_MSR_SPURIOUS_INTERRUPT_VECTOR LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_SPURIOUS_INTERRUPT_VECTOR)
#define LAPIC_MSR_ERROR_STATUS LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_ERROR_STATUS)
#define LAPIC_MSR_LVT_TIMER LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_LVT_TIMER)
#define LAPIC_MSR_LVT_THERMAL_SENSOR LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_LVT_THERMAL_SENSOR)
#define LAPIC_MSR_LVT_PERFORMANCE_MONITORING_COUNTERS LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_LVT_PERFORMANCE_MONITORING_COUNTERS)
#define LAPIC_MSR_LVT_LINT0 LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_LVT_LINT0)
#define LAPIC_MSR_LVT_LINT1 LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_LVT_LINT1)
#define LAPIC_MSR_LVT_ERROR LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_LVT_ERROR)
#define LAPIC_MSR_INITIAL_COUNT LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_INITIAL_COUNT)
#define LAPIC_MSR_CURRENT_COUNT LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_CURRENT_COUNT)
#define LAPIC_MSR_DIVIDE_CONFIG LAPIC_MSR_FROM_REGOFFSET(LAPIC_REGOFFSET_DIVIDE_CONFIG)

// Much like the RTC, I/O APIC has internal registers that you may only
// view through a window --- the data register.
// You can move the window by writing to the address register.
#define IOAPIC_REGOFFSET_ADDRESS 0x00
#define IOAPIC_REGOFFSET_DATA 0x10

#define IOAPIC_REGINDEX_ID 0x00
#define IOAPIC_REGINDEX_VERSION 0x01
#define IOAPIC_REGINDEX_ARBITRATION 0x02
#define IOAPIC_REGINDEX_REDIRECTION_LOW(n) (0x10 + 2 * (n))
#define IOAPIC_REGINDEX_REDIRECTION_HIGH(n) IOAPIC_REGINDEX_REDIRECTION_LOW(n) + 1

#define LAPIC_SPURIOUS_INT_ENABLE (((uint32_t)1) << 8)

struct IOAPIC {
    uint32_t Id{0};
    // Address to access IOAPIC_REGOFFSET_* registers at
    uintptr_t Base{0};
    // Minimum global interrupt
    uint32_t MinimumGlobalInterrupt{0};

    uint32_t MaxLVTCount{0};

    bool init();

   private:
    void write_address(uint16_t regindex);

    void write(uint16_t regindex, uint32_t value);
    uint32_t read(uint16_t regindex);
};

struct LAPIC {
    // Timer tick value
    size_t get();

    bool init();

    void set_base(uintptr_t base) { Base = base; }

   private:
    uint64_t Id{0};
    uintptr_t Base{0};
    unsigned int MaxLVTCount{0xff};
    bool bootstrap{};
    bool x2apic{};

    void write(uint16_t regoffset, uint32_t value);
    uint32_t read(uint16_t regoffset);
};

#endif  // LENSOR_OS_LAPIC_H
