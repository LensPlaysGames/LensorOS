#include <lapic.h>
#include <memory/common.h>
#include <memory/virtual_memory_manager.h>
#include <x86_64/cpu.h>

static inline void write_msr(uint32_t msr, uintptr_t value) {
    uint32_t low = value & 0xffffffff;
    uint32_t high = (uint32_t)(value >> 32);
    // 'wrmsr' writes contents of EDX:EAX to MSR specified by ECX
    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}
static inline uintptr_t read_msr(uint32_t msr) {
    uint32_t low{0};
    uint32_t high{0};
    // 'rdmsr' reads contents of MSR specified by ECX into EDX:EAX
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

void LAPIC::write(uint16_t regoffset, uint32_t value) {
    if (x2apic) {
        write_msr(LAPIC_MSR_FROM_REGOFFSET(regoffset), value);
        return;
    }
    *(volatile uint32_t*)(((uintptr_t)Base) + regoffset) = value;
}

uint32_t LAPIC::read(uint16_t regoffset) {
    if (x2apic)
        return read_msr(LAPIC_MSR_FROM_REGOFFSET(regoffset));

    return *(volatile uint32_t*)(((uintptr_t)Base) + regoffset);
}

void IOAPIC::write_address(uint16_t regindex) {
    *(volatile uint32_t*)(((uintptr_t)Base) + IOAPIC_REGOFFSET_ADDRESS) = regindex;
}
void IOAPIC::write(uint16_t regindex, uint32_t value) {
    write_address(regindex);
    *(volatile uint32_t*)(((uintptr_t)Base) + IOAPIC_REGOFFSET_DATA) = value;
}
uint32_t IOAPIC::read(uint16_t regindex) {
    write_address(regindex);
    return *(volatile uint32_t*)(((uintptr_t)Base) + IOAPIC_REGOFFSET_DATA);
}

bool IOAPIC::init() {
    auto version = read(LAPIC_REGOFFSET_VERSION);
    MaxLVTCount = (version >> 16) & 0xff;

    return true;
};

bool LAPIC::init() {
    // For now, don't place it in x2APIC mode.
    // In the future, we could do this for performance improvements, support
    // for modern systems, ACPI power state management, etc.

    // Set Base from ACPI table, if possible. Otherwise, use the
    // standard specified by Intel in the SDM Volume 3 Chapter 13.
    // "APIC Base field, bits MAXAPICADDR-1:12 ⎯ Specifies the base address of
    // the APIC registers. This value is extended by 12 bits at the low end
    // to form the base address.This automatically aligns the address on a 4 -
    // KByte boundary. Following a power - up or reset, the field is set to
    // FEE0 0000H."
    // Base = 0xfee00000;

    auto apic_base_msr = read_msr(MSR_APIC_BASE);
    std::print("[APIC]: Raw APIC_BASE Register: {:#016x}\n", apic_base_msr);
    bootstrap = apic_base_msr & LAPIC_APIC_BASE_MSR_BOOTSTRAP_CPU;
    x2apic = apic_base_msr & LAPIC_APIC_BASE_MSR_x2APIC;
    bool enabled = apic_base_msr & LAPIC_APIC_BASE_MSR_ENABLE;
    // "When IA32_APIC_BASE[11] is 0, the processor is functionally equivalent
    // to an IA-32 processor without an on-chip APIC."
    // Basically, if this bit isn't set, we have to act like there isn't an
    // APIC at all.
    if (not enabled) return false;

    // apic base model-specific register contains address, but the bottom 12
    // bits contain flags/garbage.
    auto apic_base_from_msr = apic_base_msr & ~(uintptr_t)0xfff;

    std::print("[APIC]: Base Address from APIC_BASE Register: {:#016x}\n", apic_base_from_msr);

    // TODO: Verify address from model-specific register is trustworthy...

    Base = Memory::FROM_FRAME_POINTER(apic_base_from_msr);
    std::print("[APIC]: Mapped Base: {:#016x}\n", Base);
    Memory::map(
        (void*)Base,
        (void*)Memory::TO_FRAME_POINTER(Base),
        (u64)Memory::PageTableFlag::Present
            | (u64)Memory::PageTableFlag::ReadWrite
            | (u64)Memory::PageTableFlag::CacheDisabled
            | (u64)Memory::PageTableFlag::WriteThrough);

    // Get Local APIC ID
    Id = read(LAPIC_REGOFFSET_ID);

    auto version = read(LAPIC_REGOFFSET_VERSION);
    MaxLVTCount = (version >> 16) & 0xff;

    auto spurious_interrupt = read(LAPIC_REGOFFSET_SPURIOUS_INTERRUPT_VECTOR);
    bool software_disabled = not(spurious_interrupt & LAPIC_SPURIOUS_INT_ENABLE);

    // Clear Task Priority Register, ensuring no interrupts get masked because
    // of it.
    write(LAPIC_REGOFFSET_TASK_PRIORITY, 0);

    std::print(
        "[APIC]:\n"
        "  ID: {:#x}\n"
        "  Base: {}\n"
        "  Max LVT Count: {}\n"
        "  x2APIC: {}\n"
        "  bootstrap: {}\n"
        "  enabled: {}\n"
        "  software disabled: {}\n",
        Id,
        (void*)Base,
        MaxLVTCount,
        x2apic,
        bootstrap,
        enabled,
        software_disabled);

    return enabled;
}
