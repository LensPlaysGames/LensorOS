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

#include <hpet.h>
#include <format>
#include <integers.h>
#include <memory.h>
#include <memory/virtual_memory_manager.h>

HPET gHPET;

/* Software doesn't know about hardware changing it's mmio
 *   registers, so they must be marked as volatile!
 */
void HPET::writel(u16 offset, u32 value) {
    volatile_write((u32*)(Header->Address.Address + offset), value);
}

u32 HPET::readl(u16 offset) {
    return volatile_read((u32*)(Header->Address.Address + offset));
}

void hpet_init_failed(const char* msg) {
    std::print("[HPET]: \033[31mFailed to initialize:\033[0m {}\n", msg);
}

bool HPET::initialize() {
#if defined(VBOX)
    // I can not get the HPET to work in VBOX for the life of me.
    // It causes a strange crash that shuts down the virtualbox VM.
    // Luckily, I know exactly what causes it, but (unluckily) not how to fix it.
    // The first call to `writel` and therefore `volatile_write` crashes.
    hpet_init_failed("LensorOS HPET implementation is buggy on VirtualBox");
    return false;
#else
    // This shouldn't be called by multiple threads ever, but it doesn't hurt :^).
    SpinlockLocker locker(Lock);
    Header = (ACPI::HPETHeader*)ACPI::find_table("HPET");
    if (Header == nullptr) {
        hpet_init_failed("Header is NULL!");
        return false;
    }
    if (Header->RevisionID == 0) {
        hpet_init_failed("Revision ID in ACPI header is zero.");
        return false;
    }

    if (Header->Address.AddressSpaceID == 0) {
        Memory::map((void*)Header->Address.Address, (void*)Header->Address.Address,
                    (u64) Memory::PageTableFlag::Present | (u64) Memory::PageTableFlag::ReadWrite);
    } else {
        hpet_init_failed("Invalid Address Space ID");
        return false;
    }

    /* If bit 13 of general cap. & ID register is set,
     *   HPET is capable of a 64-bit main counter value.
     */
    LargeCounterSupport = static_cast<bool>(readl(HPET_REG_GENERAL_CAPABILITIES_AND_ID) & (1 << 13));

    /* If bit 15 of general cap. & ID register is set, HPET is
     *   capable of legacy replacement interrupt mapping (IRQ0, IRQ8).
     */
    LegacyInterruptSupport = static_cast<bool>(readl(HPET_REG_GENERAL_CAPABILITIES_AND_ID) & (1 << 15));

    // Disable legacy replacement interrupt routing.
    u32 config = readl(HPET_REG_GENERAL_CONFIGURATION);
    config &= (u32)~2;
    writel(HPET_REG_GENERAL_CONFIGURATION, config);

    // Calculate Frequency (f = 10^15 / Period)
    Period = readl(HPET_MAIN_COUNTER_PERIOD);
    if (Period == 0) {
        hpet_init_failed("Period must not be zero!");
        return false;
    }
    /* Frequency can not be zero if Period is a 32-bit unsigned integer.
     * As Period grows past 49-bits wide, Frequency may become zero.
     */
    Frequency = 1000000000000000 / Period;

    /* The last five bits of the Header ID field
     *   stores the last index in the list of comparators.
     * By adding one to this value, we get the total
     *   number of comparators present on the HPET.
     * Checking the hardware version (not ACPI provided) has
     *   unexpected results in current testing environments.
     */
    NumberOfComparators = (Header->ID & 0b11111) + 1;
    if (NumberOfComparators < HPET_MIN_COMPARATORS
        || NumberOfComparators > HPET_MAX_COMPARATORS)
    {
        std::print("  Number of Comparators: {}\n", NumberOfComparators);
        hpet_init_failed("Number of comparators is invalid.");
        return false;
    }

    /* TODO: Initialize comparators.
     *       |- 1.) Determine if timer N is periodic capable; cache.
     *       `- 2.) Determine allowed interrupt routing for current timer,
     *                allocate interrupt for it (or wait for late_init or something
     *                so it's only done if the HPET is actually used as main timer).
     */

    locker.unlock();

    start_main_counter();
    Initialized = true;

    print_state();
    return Initialized;
#endif /* defined VBOX */
}

void HPET::start_main_counter() {
    if (Initialized == false)
        return;

    SpinlockLocker locker(Lock);
    u32 config = readl(HPET_REG_GENERAL_CONFIGURATION);
    config |= 1;
    writel(HPET_REG_GENERAL_CONFIGURATION, config);
}


void HPET::stop_main_counter() {
    if (Initialized == false)
        return;

    SpinlockLocker locker(Lock);
    u32 config = readl(HPET_REG_GENERAL_CONFIGURATION);
    config &= ~1;
    writel(HPET_REG_GENERAL_CONFIGURATION, config);
}

u64 HPET::get() {
    if (Initialized == false)
        return 0;

    stop_main_counter();
    SpinlockLocker locker(Lock);
    u64 result { 0 };
    if (LargeCounterSupport) {
        u32 low  { 0 };
        u32 high = readl(HPET_REG_MAIN_COUNTER_VALUE + 4);
        for(;;) {
            low = readl(HPET_REG_MAIN_COUNTER_VALUE);
            u32 newHigh = readl(HPET_REG_MAIN_COUNTER_VALUE + 4);
            if (newHigh == high)
                break;
            high = newHigh;
        }
        result = ((u64)high << 32) | low;
    }
    else result = readl(HPET_REG_MAIN_COUNTER_VALUE);
    locker.unlock();
    start_main_counter();
    return result;
}

double HPET::seconds() {
    if (Initialized == false)
        return 0;

    return static_cast<double>(get()) / Frequency;
}

void HPET::set_main_counter(u64 value) {
    if (Initialized == false)
        return;

    stop_main_counter();
    // FIXME: Another thread could lock in-between `stop_main_counter()`
    //   unlocking and this spinlock locker constructor locking it.
    SpinlockLocker locker(Lock);
    writel(HPET_REG_MAIN_COUNTER_VALUE, (u32)value);
    writel(HPET_REG_MAIN_COUNTER_VALUE + 4, (u32)(value >> 32));
    locker.unlock();
    start_main_counter();
}

void HPET::reset_counter() {
    if (Initialized == false)
        return;

    set_main_counter(0);
}

void HPET::print_state() {
    if (Initialized == false)
        return;

    std::print("[HPET]: \033[32mInitialized\033[0m\n"
              "  Revision ID: {:08b}\n"
              "  ID: {:#x}\n"
              "  PCI Vendor ID: {:#x}\n"
              "  Main Counter Enabled: {}\n"
              "  Supports 64-bit Main Counter: {}\n"
              "  Supports Legacy Interrupt Mapping: {}\n"
              "  Base Address: {:#016x}\n"
              "  Address Space ID: {:#x}\n"
              "  Sequence Number: {}\n"
              "  Minimum Tick: {}\n"
              "  Period: {}\n"
              "  Frequency: {}\n"
              "  Number of Comparators: {}\n"
              "  Page Protection: {:08b}\n"
              , Header->RevisionID
              , Header->ID
              , u16(Header->PCIvendorID)
              , bool(readl(HPET_REG_GENERAL_CONFIGURATION) & 1)
              , LargeCounterSupport
              , LegacyInterruptSupport
              , u64(Header->Address.Address)
              , Header->Address.AddressSpaceID
              , Header->Number
              , u16(Header->MinimumTick)
              , Period
              , Frequency
              , NumberOfComparators
              , Header->PageProtection);
}
