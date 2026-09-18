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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#include <interrupts/interrupts.h>
#include <memory/common.h>
#include <memory/virtual_memory_manager.h>
#include <time.h>
#include <x86_64/cpu.h>
#include <x86_64/lapic.h>
#include <x86_64/pit.h>

#include <print>

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

void IOAPIC::disable_irq(uint8_t irq) {
    irq = irq_redirects[irq];
    auto data = read(IOAPIC_REGINDEX_REDIRECTION_LOW(irq));
    data |= IOAPIC_REDIRECTION_LOW_MASKED;
    write(IOAPIC_REGINDEX_REDIRECTION_LOW(irq), data);
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
        "  spurious vector: {:#x}\n",
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

// Setup a periodic interrupt for IRQ0
void LAPIC::init_interrupt() {
    // Timer Registers:
    // Divide Configuration
    //   Sets the clock divisor (1, 2, 4, 8, 16, 32, 64, 128)
    //   The APIC clock is the CPU clock, so it can be a bit fast.
    // LVT Timer
    //   Sets the interrupt vector, masks/unmasks the timer, and sets the mode
    //   (Periodic or One-Shot).
    // Initial Count
    //   The programmable starting value for the down-counter.
    // Current Count (read-only)
    //   The current value of the down-counter.

    // Set the timer divisor to 16
    // Pattern 0x3 sets divisor to 16 on standard x86 LAPIC
    write(
        LAPIC_REGOFFSET_DIVIDE_CONFIG,
        LAPIC_DIVIDE_CONFIG_BY16);

    // Configure the LVT Timer Register
    // Set to
    //   Periodic mode,
    //   unmasked (bit 16 clear),
    //   using Interrupt Vector 32 (0x20).
    constexpr uint32_t timer_vector = PIC_IRQ0;
    // Keep the APIC timer masked (no interrupts will fire during calibration)
    write(
        LAPIC_REGOFFSET_LVT_TIMER,
        LAPIC_LVT_TIMER_PERIODIC
            | LAPIC_LVT_TIMER_MASKED
            | timer_vector);

    constexpr size_t calibration_milliseconds = 40;
    constexpr size_t trial_count = 4;
    size_t total_tick_count{0};

    for (size_t i = 0; i < trial_count; ++i) {
        // Start the timer by setting the initial count
        // Load the maximum possible value to TICR to start the countdown
        constexpr uint32_t initial_count = 0xffffffff;
        write(LAPIC_REGOFFSET_INITIAL_COUNT, initial_count);

        // Use prepared timer to sleep for some duration
        __asm__ volatile("lfence" ::: "memory");
        gPIT.wait_polling(calibration_milliseconds);
        __asm__ volatile("lfence" ::: "memory");

        // Read how many counts are left immediately after the wait
        const uint32_t current_count = read(LAPIC_REGOFFSET_CURRENT_COUNT);

        // Total elapsed ticks in our window = Initial Max Count - What's Left After Waiting
        const uint32_t ticks_per_calibration_window = initial_count - current_count;

        total_tick_count += ticks_per_calibration_window;
    }
    constexpr size_t total_milliseconds = calibration_milliseconds * trial_count;

    // Divide to get the exact value for our window
    const uint32_t ticks_per_millisecond = total_tick_count / total_milliseconds;
    const size_t frequency = ticks_per_millisecond * Time::milliseconds_per_second;
    // Actual frequency APIC would run at if divider was configured to divide
    // by one.
    const size_t full_frequency = frequency * 16;

    std::print("[LAPIC Timer]: ticks per ms={}  freq={}hz\n", ticks_per_millisecond, frequency);

    // mask system timer interrupt (PIT)
    gIOAPIC.disable_irq(0);
    // unmask timer interrupt
    write(
        LAPIC_REGOFFSET_LVT_TIMER,
        LAPIC_LVT_TIMER_PERIODIC | timer_vector);

    constexpr uint32_t milliseconds_per_timeslice = 20;
    const uint32_t ticks_per_timeslice = ticks_per_millisecond * milliseconds_per_timeslice;
    const uint32_t programmed_tick_frequency = Time::milliseconds_per_second / milliseconds_per_timeslice;
    std::print(
        "  {} ticks per {}ms time-slice  freq={}hz\n",
        ticks_per_timeslice,
        milliseconds_per_timeslice,
        programmed_tick_frequency);

    // timer_tick still updates gPIT.Ticks ... update frequency.
    gPIT.Frequency = programmed_tick_frequency;

    // time slice...
    write(
        LAPIC_REGOFFSET_INITIAL_COUNT,
        ticks_per_timeslice);

    std::print(
        "[LAPIC({}) Timer]: {Initialized}\n"
        "  Periodic, Interrupt Vector {:#x}\n"
        "  Full Timer Tick Frequency:       {}hz\n"
        "  Configured Timer Tick Frequency: {}hz\n"
        "  Legacy 8259 Programmable Interrupt Controller (PIC) Disabled -- IRQ0 Masked\n"
        "  Initial Down-counter Value: {}\n"
        "  Interrupt Frequency: {}{}hz{}\n"
        "\n",
        Id,
        __GREEN,
        timer_vector,
        full_frequency,
        frequency,
        ticks_per_timeslice,
        __YELLOW,
        programmed_tick_frequency,
        __FG_DEFAULT);
}

size_t LAPIC::get() {
    return read(LAPIC_REGOFFSET_CURRENT_COUNT);
}

void process_madt(ACPI::APICHeader* madt, IOAPIC* ioapic, LAPIC* lapic) {
    if ((not madt) or (not ioapic) or (not lapic)) return;

    std::print("[MADT]:\n");
    // Process records
    auto header_base = (uintptr_t)madt;
    auto end = header_base + madt->Length;
    std::print(
        ""
        "  records begin: {:#016x}\n"
        "  records end:   {:#016x}\n",
        header_base + sizeof(ACPI::APICHeader),
        end);

    for (uintptr_t record_base = header_base + sizeof(ACPI::APICHeader);
         record_base + sizeof(ACPI::APICHeader::Record) < end;) {
        std::print("  record at {:#016x}\n", record_base);
        auto* record = (ACPI::APICHeader::Record*)record_base;
        std::print("  - type {}, length {}\n", record->type, record->length);

        switch (record->type) {
            case 0: {
                auto* record0 = (ACPI::APICHeader::Record0*)(record_base + sizeof(ACPI::APICHeader::Record));
                std::print(
                    "  {}  CPU({:#x}), APIC({:#x})\n",
                    record0->description,
                    record0->processor_id,
                    record0->apic_id);
                // NOTE: The first LAPIC structure listed in the MADT is, by convention,
                // the bootstrap CPU.
            } break;
            case 1: {
                auto* record1 = (ACPI::APICHeader::Record1*)(record_base + sizeof(ACPI::APICHeader::Record));
                std::print(
                    "  {}\n"
                    "    ID: {}\n"
                    "    Address: {:#016x}\n"
                    "    Minimum Interrupt: {}\n",
                    record1->description,
                    record1->ioapic_id,
                    (uintptr_t)record1->ioapic_address,
                    (uint32_t)record1->global_system_interrupt_base);
                ioapic->process_ioapic(*record1);
            } break;
            case 2: {
                auto* record2 = (ACPI::APICHeader::Record2*)(record_base + sizeof(ACPI::APICHeader::Record));
                std::print(
                    "  {} BUS({:#x}), IRQ({:#x}), GSR({:#x})\n",
                    record2->description,
                    record2->bus_source,
                    record2->irq_source,
                    (uint32_t)record2->global_system_interrupt);
                ioapic->process_source_override(*record2);
            } break;
            case 3: {
                auto* record3 = (ACPI::APICHeader::Record3*)(record_base + sizeof(ACPI::APICHeader::Record));
                std::print(
                    "  {} NMI({:#x}), GSR({:#x})\n",
                    record3->description,
                    record3->nmi_source,
                    (uint32_t)record3->global_system_interrupt);
            } break;
            case 4: {
                auto* record4 = (ACPI::APICHeader::Record4*)(record_base + sizeof(ACPI::APICHeader::Record));
                std::print(
                    "  {} CPU({:#x}), INT({:#x})\n",
                    record4->description,
                    record4->processor_id,
                    record4->vector);
            } break;
            case 5: {
                auto* record5 = (ACPI::APICHeader::Record5*)(record_base + sizeof(ACPI::APICHeader::Record));
                std::print(
                    "  {}\n"
                    "  Setting LAPIC Base to {:#016x}\n",
                    record5->description,
                    (uintptr_t)record5->lapic_address);
                lapic->set_base(record5->lapic_address);
            } break;
            case 9: {
                auto* record9 = (ACPI::APICHeader::Record9*)(record_base + sizeof(ACPI::APICHeader::Record));
                std::print("  {}\n", record9->description);
            } break;
        }

        record_base += record->length;
    }
}
