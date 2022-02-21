#include "hpet.h"

#include "cstr.h"
#include "integers.h"
#include "io.h"
#include "paging/page_table_manager.h"
#include "uart.h"

HPET gHPET;

void HPET::writel(u16 offset, u32 value) {
    if (Type == AddressType::MEMORY)
        *(u32*)(Header->Address.Address + offset) = value;
    else if (Type == AddressType::IO)
        out32(Header->Address.Address + offset, value);
}

u32 HPET::readl(u16 offset) {
    u32 ret { 0 };
    if (Type == AddressType::MEMORY)
        ret = *(u32*)(Header->Address.Address + offset);
    else if (Type == AddressType::IO)
        ret = in32(Header->Address.Address + offset);

    return ret;
}

void hpet_init_failed(const char* msg) {
    srl->writestr("[HPET]: \033[31mFailed to initialize:\033[0m ");
    srl->writestr(msg);
    srl->writestr("\r\n");
}

bool HPET::initialize(ACPI::HPETHeader* header) {
    // This shouldn't be called by multiple threads ever, but it doesn't hurt :^).
    SpinlockLocker locker(Lock);
    Header = header;
    if (Header == nullptr) {
        hpet_init_failed("Header is NULL!");
        return false;
    }
    if (Header->RevisionID == 0) {
        hpet_init_failed("Revision ID in ACPI header is zero.");
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

    if (Header->Address.AddressSpaceID == 0) {
        Type = AddressType::MEMORY;
        gPTM.map_memory((void*)Header->Address.Address, (void*)Header->Address.Address);
    }
    else if (Header->Address.AddressSpaceID == 1) {
        Type = AddressType::IO;
    }
    else {
        hpet_init_failed("Invalid Address Space ID");
        return false;
    }

    // Calculate Frequency (f = 10^15 / Period)
    Period = readl(HPET_MAIN_COUNTER_PERIOD);
    if (Period == 0) {
        hpet_init_failed("Period must not be zero!");
        return false;
    }
    Frequency = 1000000000000000 / Period;

    /* The last five bits of the Header ID field 
     *   stores the last index in the list of comparators.
     * By adding one to this value, we get the total
     *   number of comparators present on the HPET.  
     * Checking the hardware version (not ACPI provided) has
     *   unexpected results in current testing environments (QEMU).
     */
    NumberOfComparators = (Header->ID & 0b11111) + 1;
    if (NumberOfComparators < HPET_MIN_COMPARATORS
        || NumberOfComparators > HPET_MAX_COMPARATORS)
    {
        srl->writestr("  Number of Comparators: ");
        srl->writestr(to_string(NumberOfComparators));
        srl->writestr("\r\n");
        hpet_init_failed("Number of comparators is invalid.");
        return false;
    }

    /* TODO: Initialize comparators.
     *       |- 1.) Determine if timer N is periodic capable; cache.
     *       `- 2.) Determine allowed interrupt routing for current timer, 
     *                allocate interrupt for it (or wait for late_init or something 
     *                so it's only done if the HPET is actually used as main timer).
     */

    start_main_counter();
    Initialized = true;

    print_state();
    return true;
}

void HPET::start_main_counter() {
    SpinlockLocker locker(Lock);
    u32 config = readl(HPET_REG_GENERAL_CONFIGURATION);
    config &= 1;
    writel(HPET_REG_GENERAL_CONFIGURATION, config);
}


void HPET::stop_main_counter() {
    SpinlockLocker locker(Lock);
    u32 config = readl(HPET_REG_GENERAL_CONFIGURATION);
    config &= ~1;
    writel(HPET_REG_GENERAL_CONFIGURATION, config);
}

u64 HPET::get() {
    SpinlockLocker locker(Lock);
    u64 result = readl(HPET_REG_MAIN_COUNTER_VALUE);
    result |= (u64)readl(HPET_REG_MAIN_COUNTER_VALUE + 4) << 32;
    return result;
}

void HPET::set_main_counter(u64 value) {
    // Kind of Spinlock heavy, but it will get better with time.
    stop_main_counter();
    SpinlockLocker locker(Lock);
    writel(HPET_REG_MAIN_COUNTER_VALUE, (u32)value);
    writel(HPET_REG_MAIN_COUNTER_VALUE + 4, (u32)(value >> 32));
    locker.unlock();
    start_main_counter();
}

void HPET::reset_counter() {
    set_main_counter(0);
}

void HPET::print_state() {
    srl->writestr("[HPET]: \033[32mInitialized\033[0m\r\n");
    srl->writestr("  Revision ID: 0x");
    srl->writestr(to_hexstring(Header->RevisionID));
    srl->writestr("\r\n");
    srl->writestr("  ID: 0x");
    srl->writestr(to_hexstring(Header->ID));
    srl->writestr("\r\n");
    srl->writestr("  PCI Vendor ID: 0x");
    srl->writestr(to_hexstring(Header->PCIvendorID));
    srl->writestr("\r\n");
    srl->writestr("  64-bit Main Counter Support: ");
    srl->writestr(to_string(static_cast<u8>(LargeCounterSupport)));
    srl->writestr("\r\n");
    srl->writestr("  Legacy Interrupt Mapping Support: ");
    srl->writestr(to_string(static_cast<u8>(LegacyInterruptSupport)));
    srl->writestr("\r\n");
    srl->writestr("  Base Address: 0x");
    srl->writestr(to_hexstring(Header->Address.Address));
    srl->writestr("\r\n");
    srl->writestr("  Address Space ID: ");
    srl->writestr(to_string(Header->Address.AddressSpaceID));
    srl->writestr("\r\n");
    srl->writestr("  Sequence Number: ");
    srl->writestr(to_string(Header->Number));
    srl->writestr("\r\n");
    srl->writestr("  Minimum Tick: ");
    srl->writestr(to_string(Header->MinimumTick));
    srl->writestr("\r\n");
    srl->writestr("  Period: ");
    srl->writestr(to_string(Period));
    srl->writestr("\r\n");
    srl->writestr("  Frequency: ");
    srl->writestr(to_string(Frequency));
    srl->writestr("\r\n");
    srl->writestr("  Number of Comparators: ");
    srl->writestr(to_string(NumberOfComparators));
    srl->writestr("\r\n");
    srl->writestr("  Page Protection: ");
    srl->writestr(to_string(Header->PageProtection));
    srl->writestr("\r\n");
}
