#include "hpet.h"

#include "cstr.h"
#include "integers.h"
#include "io.h"
#include "memory.h"
#include "memory/virtual_memory_manager.h"
#include "uart.h"

HPET gHPET;

/* Software doesn't know about hardware changing it's mmio 
 *   registers, so they must be marked as volatile! 
 */
void HPET::writel(u16 offset, u32 value) {
    volatile_write(&value, (volatile void*)(Header->Address.Address + (offset)), 4);
}

u32 HPET::readl(u16 offset) {
    u32 ret { 0 };
    volatile_read((const volatile void*)(Header->Address.Address + (offset)), &ret, 4);
    return ret;
}

void hpet_init_failed(const char* msg) {
    UART::out("[HPET]: \033[31mFailed to initialize:\033[0m ");
    UART::out(msg);
    UART::out("\r\n");
}

bool HPET::initialize() {
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
        Memory::map((void*)Header->Address.Address, (void*)Header->Address.Address);
    }
    else {
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
    config &= ~2;
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
     *   unexpected results in current testing environments (QEMU).
     */
    NumberOfComparators = (Header->ID & 0b11111) + 1;
    if (NumberOfComparators < HPET_MIN_COMPARATORS
        || NumberOfComparators > HPET_MAX_COMPARATORS)
    {
        UART::out("  Number of Comparators: ");
        UART::out(to_string(NumberOfComparators));
        UART::out("\r\n");
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

double HPET::get_seconds() {
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

    UART::out("[HPET]: \033[32mInitialized\033[0m\r\n");
    UART::out("  Revision ID: 0x");
    UART::out(to_hexstring(Header->RevisionID));
    UART::out("\r\n");
    UART::out("  ID: 0x");
    UART::out(to_hexstring(Header->ID));
    UART::out("\r\n");
    UART::out("  PCI Vendor ID: 0x");
    UART::out(to_hexstring(Header->PCIvendorID));
    UART::out("\r\n");
    UART::out("  Main Counter Enabled: ");
    UART::out(to_string(readl(HPET_REG_GENERAL_CONFIGURATION) & 1));
    UART::out("\r\n");
    UART::out("  Supports 64-bit Main Counter: ");
    UART::out(to_string(LargeCounterSupport));
    UART::out("\r\n");
    UART::out("  Supports Legacy Interrupt Mapping: ");
    UART::out(to_string(LegacyInterruptSupport));
    UART::out("\r\n");
    UART::out("  Base Address: 0x");
    UART::out(to_hexstring(Header->Address.Address));
    UART::out("\r\n");
    UART::out("  Address Space ID: ");
    UART::out(to_string(Header->Address.AddressSpaceID));
    UART::out("\r\n");
    UART::out("  Sequence Number: ");
    UART::out(to_string(Header->Number));
    UART::out("\r\n");
    UART::out("  Minimum Tick: ");
    UART::out(to_string(Header->MinimumTick));
    UART::out("\r\n");
    UART::out("  Period: ");
    UART::out(to_string(Period));
    UART::out("\r\n");
    UART::out("  Frequency: ");
    UART::out(to_string(Frequency));
    UART::out("\r\n");
    UART::out("  Number of Comparators: ");
    UART::out(to_string(NumberOfComparators));
    UART::out("\r\n");
    UART::out("  Page Protection: ");
    UART::out(to_string(Header->PageProtection));
    UART::out("\r\n");
}
