#ifndef LENSOR_OS_HIGH_PRECISION_EVENT_TIMER_H
#define LENSOR_OS_HIGH_PRECISION_EVENT_TIMER_H

/* Resources & Inspiration:
 * |- https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf
 * |- https://github.com/torvalds/linux/blob/master/arch/x86/kernel/hpet.c
 * `- https://wiki.osdev.org/HPET
 */

#include "acpi.h"
#include "spinlock.h"
#include "uart.h"

#define HPET_MAX_COMPARATORS 32

#define HPET_REG_GENERAL_CAPABILITIES_AND_ID(base)        (base + 0x000)
#define HPET_REG_GENERAL_CONFIGURATION(base)              (base + 0x010)
#define HPET_REG_GENERAL_INTERRUPT_STATUS(base)           (base + 0x020)
#define HPET_REG_MAIN_COUNTER_VALUE(base)                 (base + 0x0f0)
#define HPET_REG_TIMER_N_CONFIG_AND_CAPABILITIES(base, n) (base + ((0x100 + 0x20 * n) - (0x107 + 0x20 * n)))
#define HPET_REG_TIMER_N_COMPARATOR_VALUE(base, n)        (base + ((0x108 + 0x20 * n) - (0x10f + 0x20 * n)))
#define HPET_REG_TIMER_N_FSB_INTERRUPT_ROUTE(base, n)     (base + ((0x110 + 0x20 * n) - (0x117 + 0x20 * n)))

/* HPET Registers 
 * General Capabilities and ID:
 *   Bits 0-7: Revision ID (must not be zero).
 *        8-12: Number of Timers.
 *        13: If set, main counter capable of 64-bit.
 *        14: Reserved.
 *        15: If set, HPET is capable of using legacy replacement interrupt mapping.
 *        16-31: PCI Vendor ID.
 *        32-63: Main counter tick period in femtoseconds (must not be zero).
 *        
 * General Configuration Register:
 *   Bit 0: If set, main counter enabled.
 *       1: If set, legacy replacement mapping is enabled.
 *
 * General Interrupt Status Register
 *   Bit n: If level-triggered, set when corresponding timer interrupt is active.
 *            If set, software can clear it by writing a 1 to this bit. Writes of 0 have no effect.
 *          If edge-triggered, ignored (must be zero).
 *       32-63: Reserved
 * 
 * Main Counter Value
 *   Do not write to this register unless main counter is disabled.
 *   Bits 0-63: Value of main counter.
 *
 * Timer N Configuration and Capabilities
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
 * Timer N Comparator Value Register
 *   Bits 0-63: When main counter value is equal to this value, an interrupt will be generated.
 */

struct Comparator {
    bool PeriodicCapable;
};

class HPET {
public:
    HPET() {};

    bool initialize(ACPI::HPETHeader* header) {
        /* TODO: Initialize comparators.
         *       |- 1.) Determine if timer N is periodic capable; cache.
         *       `- 2.) Determine allowed interrupt routing for current timer, allocate interrupt for it.
         */

        // AddressSpaceID = 1 = System I/O Port Address Space
        if (header->Address.AddressSpaceID != 1)
            return false;

        BaseAddress = header->Address.Address;
        MinimalTick = header->MinimumTick;

        // Frequency = 10^15 / Period
        u32 Period = in32(HPET_REG_GENERAL_CAPABILITIES_AND_ID(BaseAddress));
        Frequency = 1000000000000000 / Period;

        // Enable Main Counter
        u32 config = in32(HPET_REG_GENERAL_CONFIGURATION(BaseAddress));
        out32(HPET_REG_GENERAL_CONFIGURATION(BaseAddress), config & 1);

        srl->writestr("[HPET]: \033[33m Initialized \033[0m\r\n");
        srl->writestr("  Address: 0x");
        srl->writestr(to_hexstring((u64)BaseAddress));
        srl->writestr("\r\n");
        srl->writestr("  Frequency: ");
        srl->writestr(to_string(Frequency));
        srl->writestr("\r\n");
    }

    void start_main_counter() {
        u32 config = in32(HPET_REG_GENERAL_CONFIGURATION(BaseAddress));
        config &= 1;
        out32(HPET_REG_GENERAL_CONFIGURATION(BaseAddress), config);
    }

    void stop_main_counter() {
        u32 config = in32(HPET_REG_GENERAL_CONFIGURATION(BaseAddress));
        config &= ~1;
        out32(HPET_REG_GENERAL_CONFIGURATION(BaseAddress), config);
    }

    void set_main_counter(u64 value) {
        SpinlockLocker locker(Lock);
        stop_main_counter();
        out32(HPET_REG_MAIN_COUNTER_VALUE(BaseAddress), (u32)value);
        out32(HPET_REG_MAIN_COUNTER_VALUE(BaseAddress) + 4, (u32)(value >> 32));
        start_main_counter();
    }

    void reset_counter() {
        set_main_counter(0);
    }

    u64 get() {
        SpinlockLocker locker(Lock);
        u64 result = in32(HPET_REG_MAIN_COUNTER_VALUE(BaseAddress));
        result |= (u64)in32(HPET_REG_MAIN_COUNTER_VALUE(BaseAddress) + 4) << 32;
        return result;
    }

private:
    bool Initialized { false };
    Spinlock Lock;
    u64 BaseAddress { 0 };
    /* Minimum period the HPET can count at in femtoseconds (10^-15 seconds). */
    u64 MinimalTick { 0 };
    u64 Frequency { 0 };
    /* Number of comparators implemented; can vary from 3-32. */
    u8 NumberOfComparators { 0 };
    Comparator* Comparators[HPET_MAX_COMPARATORS];
};

#endif
