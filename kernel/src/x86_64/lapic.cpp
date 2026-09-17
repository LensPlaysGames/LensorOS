#include <interrupts/interrupts.h>
#include <memory/common.h>
#include <memory/virtual_memory_manager.h>
#include <x86_64/cpu.h>
#include <x86_64/lapic.h>

IOAPIC gIOAPIC;
LAPIC gLAPIC;

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

void IOAPIC::process_source_override(uint8_t irq_source, uint8_t global_system_interrupt) {
    if (irq_source > sizeof(irq_redirects)) {
        std::print(
            "[IOAPIC]: Cannot process interrupt source override {} -> {}\n",
            global_system_interrupt,
            irq_source);
        return;
    }
    irq_redirects[irq_source] = global_system_interrupt;
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

void IOAPIC::enable_irq(uint8_t irq) {
    if (irq >= 16) {
        // panic()? hang()?
        std::print("[IOAPIC]: invalid irq: {}\n", irq);
        return;
    }

    std::print(
        "[IOAPIC]: Enabling IRQ{} on pin {} via IOAPIC at {}\n",
        irq,
        irq_redirects[irq],
        (void*)Base);

    redirect_gsi_to_idt_vector(
        irq_redirects[irq],
        PIC_IRQ_VECTOR_OFFSET + irq);
}

void IOAPIC::redirect_gsi_to_idt_vector(uint8_t global_interrupt, uint8_t vector) {
    if (global_interrupt < MinimumGlobalInterrupt) {
        // panic()?
        std::print(
            "[IOAPIC]: requested GSI({}) is less than minimum GSI({}) of this IOAPIC (at {})\n",
            global_interrupt,
            MinimumGlobalInterrupt,
            (void*)Base);
        return;
    }

    uint32_t low = vector;
    uint32_t high = 0;

    low |= IOAPIC_REDIRECTION_LOW_DELIVERY_MODE(0);
    high |= IOAPIC_REDIRECTION_HIGH_DESTINATION(0);

    write(IOAPIC_REGINDEX_REDIRECTION_LOW(global_interrupt), low);
    write(IOAPIC_REGINDEX_REDIRECTION_HIGH(global_interrupt), high);
}

void IOAPIC::print_debug() {
    std::print(
        "[IOAPIC]:\n"
        "  ID: {:#x}\n"
        "  Max LVT Count: {}\n",
        Id,
        MaxLVTCount);

    std::print(
        "Pin | Vector | Delivery Mode | Dest. Mode | Delivery Status | Polarity | Request | Trigger | Mask | Destination\n");
    for (size_t i = 0; i < MaxLVTCount; ++i) {
        // Redirection entries are 64 bits wide, split across two 32-bit registers
        uint32_t low = read(IOAPIC_REGINDEX_REDIRECTION_LOW(i));
        uint32_t high = read(IOAPIC_REGINDEX_REDIRECTION_HIGH(i));

        uint8_t vector = low & 0xff;
        uint8_t delivery_mode = (low >> 8) & 0b111;
        bool destination_mode = (low >> 11) & 1;
        bool delivery_status = (low >> 12) & 1;
        bool polarity = (low >> 13) & 1;
        bool interrupt_request_routine = (low >> 14) & 1;
        bool trigger = (low >> 15) & 1;
        bool masked = (low >> 16) & 1;
        // Should hold physical APIC ID of CPU we want to interrupt
        uint8_t destination = (high >> 24) & 0xff;

        std::print(
            "{:02} | {:#02x} | {} | {} | {} | {} | {} | {} | {} | {:#02x}\n",
            i,
            vector,
            delivery_mode,
            destination_mode ? "Logical" : "Physical",
            delivery_status ? "Pending" : "No Status",
            polarity ? "Active Low (PCI)" : "Active High (ISA)",
            interrupt_request_routine,
            trigger ? "Level" : "Edge ",
            masked ? "Masked" : "Mapped",
            destination);
    }
}

bool IOAPIC::init(uintptr_t bootstrap_cpu_id) {
    if (not Base) return false;

    Base = Memory::FROM_FRAME_POINTER(Base);
    Memory::map(
        (void*)Base,
        (void*)Memory::TO_FRAME_POINTER(Base),
        (u64)Memory::PageTableFlag::Present
            | (u64)Memory::PageTableFlag::ReadWrite
            | (u64)Memory::PageTableFlag::CacheDisabled
            | (u64)Memory::PageTableFlag::WriteThrough);
    std::print("[IOAPIC]: Mapped Base: {:#016x}\n", Base);

    auto id_raw = read(IOAPIC_REGINDEX_ID);
    Id = (id_raw >> 24) & 0xf;

    auto version_raw = read(IOAPIC_REGINDEX_VERSION);
    MaxLVTCount = ((version_raw >> 16) & 0xff) + 1;

    print_debug();

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
    Memory::map(
        (void*)Base,
        (void*)Memory::TO_FRAME_POINTER(Base),
        (u64)Memory::PageTableFlag::Present
            | (u64)Memory::PageTableFlag::ReadWrite
            | (u64)Memory::PageTableFlag::CacheDisabled
            | (u64)Memory::PageTableFlag::WriteThrough);
    std::print("[APIC]: Mapped Base: {:#016x}\n", Base);

    // Get Local APIC ID
    auto id_raw = read(LAPIC_REGOFFSET_ID);
    Id = (id_raw >> 24) & 0xf;

    auto version = read(LAPIC_REGOFFSET_VERSION);
    MaxLVTCount = ((version >> 16) & 0xff) + 1;

    auto spurious_interrupt = read(LAPIC_REGOFFSET_SPURIOUS_INTERRUPT_VECTOR);
    // Configure spurious interrupt vector as 0xff.
    // Matches IDT vector
    constexpr uint8_t spurious_vector = 0xff;
    spurious_interrupt |= spurious_vector;
    // Ensure software enable bit is set.
    spurious_interrupt |= LAPIC_SPURIOUS_INT_ENABLE;
    // Write back spurious interrupt register.
    write(
        LAPIC_REGOFFSET_SPURIOUS_INTERRUPT_VECTOR,
        spurious_interrupt);

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
        "  spurious vector: {:#x}",
        Id,
        (void*)Base,
        MaxLVTCount,
        x2apic,
        bootstrap,
        enabled,
        spurious_vector);

    return enabled;
}

void LAPIC::eoi() {
    write(LAPIC_REGOFFSET_EOI, 0);
}
