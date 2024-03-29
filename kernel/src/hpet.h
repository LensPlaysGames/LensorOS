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

#ifndef LENSOR_OS_HIGH_PRECISION_EVENT_TIMER_H
#define LENSOR_OS_HIGH_PRECISION_EVENT_TIMER_H

/* High Precision Event Timer Driver
 * Resources & Inspiration:
 * |- INTEL SD's HPET SPEC Table 1
 * |    https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf
 * |- https://github.com/Meulengracht/MollenOS/blob/master/kernel/acpi/hpet.c
 * |- https://github.com/torvalds/linux/blob/master/arch/x86/kernel/hpet.c
 * `- https://wiki.osdev.org/HPET
 */

#include <acpi.h>
#include <spinlock.h>
#include <uart.h>

/* INTEL SD's HPET SPEC Table 1 */
#define HPET_MIN_COMPARATORS 3
#define HPET_MAX_COMPARATORS 32

#define HPET_REG_GENERAL_CAPABILITIES_AND_ID 0x000
#define HPET_MAIN_COUNTER_PERIOD             0x004
#define HPET_REG_GENERAL_CONFIGURATION       0x010
#define HPET_REG_GENERAL_INTERRUPT_STATUS    0x020
#define HPET_REG_MAIN_COUNTER_VALUE          0x0f0
#define HPET_REG_TIMER_N_CONFIG_AND_CAPABILITIES(n) (0x100 + 0x20 * n)
#define HPET_REG_TIMER_N_COMPARATOR_VALUE(n)        (0x108 + 0x20 * n)
#define HPET_REG_TIMER_N_FSB_INTERRUPT_ROUTE(n)     (0x110 + 0x20 * n)

/* HPET Registers
 * General Capabilities and ID (read-only):
 *   Bits 0-7: Revision ID (must not be zero).
 *        8-12: Number of Timers.
 *        13: If set, main counter capable of 64-bit.
 *        14: Reserved.
 *        15: If set, HPET is capable of using legacy replacement interrupt mapping.
 *        16-31: PCI Vendor ID.
 *        32-63: Main counter tick period in femtoseconds (must not be zero).
 *
 * General Configuration Register (read-write):
 *   Bit 0: If set, main counter enabled.
 *       1: If set, legacy replacement mapping is enabled.
 *
 * General Interrupt Status Register (read-write clear):
 *   Bit n: If level-triggered, set when corresponding timer interrupt is active.
 *            If set, software can clear it by writing a 1 to this bit. Writes of 0 have no effect.
 *          If edge-triggered, ignored (must be zero).
 *       32-63: Reserved
 * 
 * Main Counter Value (read-write):
 *   Do not write to this register unless main counter is disabled.
 *   Bits 0-63: Value of main counter.
 *
 * Timer N Configuration and Capabilities (read-write):
 *   Bit 0: Reserved.
 *       1: Interrupt Type (0=edge-triggered, 1=level-triggered).
 *       2: Interrupt Enable.
 *       3: If periodic interrupts supported, writing a 1 enables periodic timer, else ignore.
 *       4: If set, timer has periodic interrupt capability.
 *       5: If set, timer size is 64-bit, otherwise 32-bit.
 *       6: Value Set Configuration.
 *       7: Reserved.
 *       8: If set and 64-bit, force 32-bit mode.
 *       9-13: I/O APIC Routing.
 *       14: If set, enable FSB interrupt mapping.
 *       15: If set, timer is capable of FSB interrupt mapping.
 *       16-31: Reserved.
 *       32-63: If bit X is set in this field, this timer can be mapped to IRQX line of I/O APIC.
 *
 * Timer N Comparator Value Register (read-write):
 *   Bits 0-63: When main counter value is equal to this value, an interrupt will be generated.
 */

// Data cache for each HPET comparator.
struct Comparator {
    Comparator() {}
    Comparator(bool largeCounterSupport, bool periodicCapable)
        : LargeCounterSupport(largeCounterSupport)
        , PeriodicCapable(periodicCapable) {}

    /* 64-bit Comparator Width */
    bool LargeCounterSupport { false };
    /* Capable of providing a regular, repeating interrupt. */
    bool PeriodicCapable { false };
};

// High Precision Event Timer
class HPET {
public:
    HPET() {};

    bool initialize();

    void start_main_counter();
    void stop_main_counter();

    /// Get the value of the HPET main counter.
    u64 get();
    /* Get the amount of seconds passed based on main counter.
     * NOTE: This is inaccurate as counter is paused/started often.
     */
    //double seconds();

    /// Disable counting, set the main counter to the given value, then enable counting.
    void set_main_counter(u64 value);
    /// Disable counting, set main counter to zero, then enable counting.
    void reset_counter();
    /// Print the current state of this HPET (address, freq, etc) to serial out.
    void print_state();

private:
    Spinlock Lock;
    ACPI::HPETHeader* Header { nullptr };
    bool Initialized { false };
    bool LargeCounterSupport { false };
    bool LegacyInterruptSupport { false };
    u32 Period    { 0 };
    u64 Frequency { 0 };
    /* Number of comparators implemented; can vary from 3-32. */
    u8 NumberOfComparators { 0 };
    Comparator Comparators[HPET_MAX_COMPARATORS] {};
    /* NOTE: Registers may only be read from/written to at 32-byte boundaries.
     *   To extract a single byte, the caller must do it manually (mask & shift).
     */
    void writel(u16 offset, u32 value);
    u32 readl(u16 offset);
};

extern HPET gHPET;

#endif /* if not defined LENSOR_OS_HIGH_PRECISION_EVENT_TIMER_H */
