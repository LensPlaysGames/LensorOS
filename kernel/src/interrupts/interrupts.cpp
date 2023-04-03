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

/** On writing interrupt handlers for x86_64
 *
 *  MOST IMPORTANTLY:
 *  YOU WILL CAUSE BIG BAD SYSTEM BREAKING PROBLEMS IF THE HANDLER
 *  DOES NOT END UP ACKNOWLEDGING (by calling end of interrupt). DO
 *  NOT EVER WRITE `return` UNLESS YOU ABSOLUTELY KNOW WHAT YOU ARE
 *  DOING!
 *
 */

#include <interrupts/interrupts.h>

#include <basic_renderer.h>
#include <cstr.h>
#include <format>
#include <io.h>
#include <keyboard.h>
#include <keyboard_scancode_translation.h>
#include <memory/paging.h>
#include <memory/virtual_memory_manager.h>
#include <mouse.h>
#include <panic.h>
#include <pit.h>
#include <rtc.h>
#include <scheduler.h>
#include <system.h>
#include <uart.h>
#include <vfs_forward.h>

/// Use this when called from an interrupt handler.
__attribute__((no_caller_saved_registers))
u8 in8_wrap(u8 port) {
    return in8(port);
}


/// Use this when called from an interrupt handler.
#define print printerrupt
namespace std {
template <typename... _Args>
__attribute__((no_caller_saved_registers))
void printerrupt(std::format_string<_Args...> __fmt, _Args&&... __args) {
    vprint(__fmt.get(), std::forward<_Args>(__args)...);
}
}


void enable_interrupt(u8 irq) {
    if (irq > 15)
        return;

    u16 port = PIC2_DATA;
    if (irq < 8)
        port = PIC1_DATA;
    else
        irq -= 8;
    u8 value = in8(port) & ~IRQ_BIT(irq);
    out8(port, value);
}

void disable_interrupt(u8 irq) {
    if (irq > 15)
        return;

    u16 port = PIC2_DATA;
    if (irq < 8)
        port = PIC1_DATA;
    else irq -= 8;
    u8 value = in8(port) | IRQ_BIT(irq);
    out8(port, value);
}

void disable_all_interrupts() {
    out8(PIC1_DATA, 0);
    out8(PIC2_DATA, 0);
}

void end_of_interrupt(u8 IRQx) {
    if (IRQx >= 8)
        out8(PIC2_COMMAND, PIC_EOI);
    out8(PIC1_COMMAND, PIC_EOI);
}

_PushIgnoreWarning("-Wvolatile")
void cause_div_by_zero(volatile u8 one) {
    one /= one - 1;
}
_PopWarnings()

void cause_page_not_present() {
    u8* badAddr = (u8*)0xdeadc0de;
    volatile u8 faultHere = *badAddr;
    (void)faultHere;
}

void cause_nullptr_dereference() {
    u8* badAddr = (u8*)nullptr;
    volatile u8 faultHere = *badAddr;
    (void)faultHere;
}

void cause_general_protection() {
    u8* badAddr = (u8*)0xdeadbeefb00bface;
    volatile u8 faultHere = *badAddr;
    (void)faultHere;
}

// HARDWARE INTERRUPT HANDLERS (IRQs)
/// IRQ0: SYSTEM TIMER
__attribute__((interrupt))
void system_timer_handler(InterruptFrame* frame) {
    gPIT.tick();
    end_of_interrupt(0);
}

Keyboard::KeyboardState State = {0};

__attribute__((no_caller_saved_registers))
static void handle_direct_input(char input) {
    if (!input) {
        std::print("[INPUT]: Refusing null input\n");
        return;
    }

    // Send user input to userspace!
    // Write to stdin of init process.
    if (SYSTEM) {
        Process* init = SYSTEM->init_process();
        if (init) {
            auto fd = static_cast<ProcFD>(0);
            auto sysfd = init->FileDescriptors[fd];
            auto f = SYSTEM->virtual_filesystem().file(*sysfd);
            if (f) f->device_driver()->write(f.get(), 0, sizeof(char), &input);
            return;
        }
    }
    std::print("[INPUT]: No init process: cannot handle user input properly.\n");
}

__attribute__((no_caller_saved_registers))
static void handle_scancode_input(u8 scancode) {
    // Handle modifiers
    static Keyboard::KeyboardState State = {0};
    switch (scancode) {
    case LSHIFT:
        //std::print("[INPUT]: mod_press: left shift\n");
        State.LeftShift = true;
        return;
    case LSHIFT + 0x80:
        //std::print("[INPUT]: mod_release: left shift\n");
        State.LeftShift = false;
        return;
    case RSHIFT:
        //std::print("[INPUT]: mod_press: right shift\n");
        State.RightShift = true;
        return;
    case RSHIFT + 0x80:
        //std::print("[INPUT]: mod_press: right shift\n");
        State.RightShift = false;
        return;
    case CAPSLOCK:
        //std::print("[INPUT]: mod_capslock\n");
        State.CapsLock = !State.CapsLock;
        return;
    default: break;
    }

    // TODO: Support other keyboard scancode translation layouts.
    char translated = Keyboard::QWERTY::Translate
        (scancode, State.LeftShift || State.RightShift || State.CapsLock);
    if (translated) handle_direct_input(translated);
    //else std::print("Skipping scancode: {:x}\n", scancode);
}

/// IRQ1: PS/2 KEYBOARD
__attribute__((interrupt))
void keyboard_handler(InterruptFrame* frame) {
    // Read scancode from bus.
    handle_scancode_input(in8_wrap(0x60));
    end_of_interrupt(1);
}

/// IRQ4: COM1/COM3 Serial Communications Recieved
__attribute__((interrupt))
void uart_com1_handler(InterruptFrame* frame) {
    u8 data = UART::read();
    // TODO: Handle input data more betterer.
    if (data == '\n' || data == '\b' || data == '\a' || (data >= ' ' && data <= '~')) handle_direct_input(data);
    //Keyboard::gText.handle_character(data);
    end_of_interrupt(4);
}

/// IRQ8: Real Time Clock
/// NOTE: If register 'C' is not read from inside this handler,
///         no further interrupts of this type will be sent.
/// Status Register `C`:
///   Bits 0-3: Reserved (do not touch)
///          4: Update-ended interrupt
///          5: Alarm interrupt
///          6: Periodic Interrupt
///          7: Interrupt Request (IRQ)
__attribute__((interrupt))
void rtc_handler(InterruptFrame* frame) {
    u8 statusC = gRTC.read_register(0x0C);
    if (statusC & 0b01000000) gRTC.Ticks += 1;
    end_of_interrupt(8);
}

/// IRQ12: PS/2 MOUSE
__attribute__((interrupt))
void mouse_handler(InterruptFrame* frame) {
    u8 data = in8_wrap(0x60);
    // TODO: Send input event or something? Write input event to queue?
    //handle_ps2_mouse_interrupt(data);
    // End interrupt
    end_of_interrupt(12);
}

/// FAULT INTERRUPT HANDLERS

__attribute__((interrupt))
void divide_by_zero_handler(InterruptFrame* frame) {
    panic(frame, "Divide by zero detected!");
    while (true)
        asm ("hlt");
}

enum class PageFaultErrorCode {
    Present                                   = 1 << 0,
    ReadWrite                                 = 1 << 1,
    UserSuper                                 = 1 << 2,
    Reserved                                  = 1 << 3,
    InstructionFetch                          = 1 << 4,
    ProtectionKeyViolation                    = 1 << 5,
    ShadowStackAccess                         = 1 << 6,
    HypervisorManagedLinearAddressTranslation = 1 << 7,
    SoftwareGaurdExtensions                   = 1 << 15,
};

__attribute__((interrupt))
void page_fault_handler(InterruptFrameError* frame) {
    // Collect faulty address as soon as possible (it may be lost quickly).
    u64 address;
    asm volatile ("mov %%cr2, %0" : "=r" (address));

    std::print("  Faulty Address: {:#016x}\n", address);
    u64 cr3;
    asm volatile ("mov %%cr3, %0" : "=r" (cr3));
    std::print("  PageTable Address: {:#016x}\n", cr3);

    Memory::PageMapIndexer indexer(address);
    Memory::PageDirectoryEntry PDE;
    PDE = ((Memory::PageTable*)cr3)->entries[indexer.page_directory_pointer()];
    std::print("4th lvl permissions | ");
    Memory::print_pde_flags(PDE);
    std::print("\n");

    auto* PDP = (Memory::PageTable*)((u64)PDE.address() << 12);
    PDE = PDP->entries[indexer.page_directory()];
    std::print("3rd lvl permissions | ");
    Memory::print_pde_flags(PDE);
    std::print("\n");

    auto* PD = (Memory::PageTable*)((u64)PDE.address() << 12);
    PDE = PD->entries[indexer.page_table()];
    std::print("2nd lvl permissions | ");
    Memory::print_pde_flags(PDE);
    std::print("\n");

    auto* PT = (Memory::PageTable*)((u64)PDE.address() << 12);
    PDE = PT->entries[indexer.page()];
    std::print("1st lvl permissions | ");
    Memory::print_pde_flags(PDE);
    std::print("\n");

    std::print("PHYS {:#016x} at VIRT {:#016x}\n",
               u64(PDE.address() << 12),
               u64(address));

    if ((frame->error & (u64)PageFaultErrorCode::ProtectionKeyViolation) > 0)
        std::print("  Protection Key Violation\n");
    if ((frame->error & (u64)PageFaultErrorCode::HypervisorManagedLinearAddressTranslation) > 0)
        std::print("  Hypervisor Managed Linear Address Translation\n");
    if ((frame->error & (u64)PageFaultErrorCode::InstructionFetch) > 0)
        std::print("  Instruction fetch\n");
    else std::print("  Data Access\n");
    if ((frame->error & (u64)PageFaultErrorCode::ShadowStackAccess) > 0)
        std::print("  Shadow stack access\n");
    if ((frame->error & (u64)PageFaultErrorCode::HypervisorManagedLinearAddressTranslation) > 0)
        std::print("  Hypvervisor-managed linear address translation\n");
    if ((frame->error & (u64)PageFaultErrorCode::SoftwareGaurdExtensions) > 0)
        std::print("  Software gaurd extensions\n");
    if ((frame->error & (u64)PageFaultErrorCode::UserSuper) > 0)
        std::print("  From ring 3\n");
    if ((frame->error & (u64)PageFaultErrorCode::ReadWrite) > 0)
        std::print("  Write\n");
    else std::print("  Read\n");
    if ((frame->error & (u64)PageFaultErrorCode::Reserved) > 0)
        std::print("  Reserved\n");

    std::print("CurrentProcess->ProcessID == {}\n", u64(Scheduler::CurrentProcess->value()->ProcessID));
    if (frame->error & (u64)PageFaultErrorCode::UserSuper)
        Memory::print_page_map((Memory::PageTable*)cr3, Memory::PageTableFlag::UserSuper);
    else Memory::print_page_map((Memory::PageTable*)cr3);

    /* US RW P - Description
     * 0  0  0 - Supervisory process tried to read a non-present page entry
     * 0  0  1 - Supervisory process tried to read a page and caused a protection fault
     * 0  1  0 - Supervisory process tried to write to a non-present page entry
     * 0  1  1 - Supervisory process tried to write a page and caused a protection fault
     * 1  0  0 - User process tried to read a non-present page entry
     * 1  0  1 - User process tried to read a page and caused a protection fault
     * 1  1  0 - User process tried to write to a non-present page entry
     * 1  1  1 - User process tried to write a page and caused a protection fault
     */
    bool notPresent = (frame->error & (u64)PageFaultErrorCode::Present) == 0;
    if ((frame->error & (u64)PageFaultErrorCode::UserSuper) > 0) {
        if ((frame->error & (u64)PageFaultErrorCode::ReadWrite) > 0) {
            if (notPresent)
                panic(frame, "#PF: User process attempted to write to a page that is not present");
            else panic(frame, "#PF: User process attempted to write to a page and caused a protection fault");
        }
        else {
            if (notPresent)
                panic(frame, "#PF: User process attempted to read from a page that is not present");
            else panic(frame, "#PF: User process attempted to read from a page and caused a protection fault");
        }
    }
    else {
        if ((frame->error & (u64)PageFaultErrorCode::ReadWrite) > 0) {
            if (notPresent)
                panic(frame, "#PF: Supervisor process attempted to write to a page that is not present");
            else panic(frame, "#PF: Supervisor process attempted to write to a page and caused a protection fault");
        }
        else {
            if (notPresent)
                panic(frame, "#PF: Supervisor process attempted to read from a page that is not present");
            else panic(frame, "#PF: Supervisor process attempted to read from a page and caused a protection fault");
        }
    }

    Vector2<u64> drawPosition = { PanicStartX, PanicStartY };
    gRend.puts(drawPosition, std::format("Faulty Address: {:#016x}", address), 0x00000000);
    gRend.swap({ PanicStartX, PanicStartY }, { 1024, 128 } );
    while (true)
        asm volatile ("hlt");
}

__attribute__((interrupt))
void double_fault_handler(InterruptFrameError* frame) {
    panic(frame, "Double fault detected!");
    while (true) {
        asm ("hlt");
    }
}

__attribute__((interrupt))
void stack_segment_fault_handler(InterruptFrameError* frame) {
    if (frame->error == 0)
        panic(frame, "Stack segment fault detected (0)");
    else panic(frame, "Stack segment fault detected (selector)!");

    if (frame->error & 0b1)
        std::print("  External\n");

    u8 table = (frame->error & 0b110) >> 1;
    if (table == 0b00)
        std::print("  GDT");
    else if (table == 0b01 || table == 0b11)
        std::print("  IDT");
    else if (table == 0b10)
        std::print("  LDT");

    std::print(" Selector Index: {:x}\n", (frame->error & 0b1111'1111'1111'1000) >> 3);
}

__attribute__((interrupt))
void general_protection_fault_handler(InterruptFrameError* frame) {
    if (frame->error == 0)
        panic(frame, "General protection fault detected (0)!");
    else panic(frame, "General protection fault detected (selector)!");

    if (frame->error & 0b1)
        std::print("  External\n");

    u8 table = (frame->error & 0b110) >> 1;
    if (table == 0b00)
        std::print("  GDT");
    else if (table == 0b01 || table == 0b11)
        std::print("  IDT");
    else if (table == 0b10)
        std::print("  LDT");

    std::print(" Selector Index: {:x}\n", (frame->error & 0b1111'1111'1111'1000) >> 3);
    while (true)
        asm ("hlt");
}

__attribute__((interrupt))
void simd_exception_handler(InterruptFrame* frame) {
    /* NOTE: Data about why exception occurred can be found in MXCSR register.
     * MXCSR Register breakdown:
     * 0b00000000
     *          =   -- invalid operation flag
     *         =    -- denormal flag
     *        =     -- divide-by-zero flag
     *       =      -- overflow flag
     *      =       -- underflow flag
     *     =        -- precision flag
     *    =         -- denormals are zeros flag
     */
    u32 mxcsr { 0 };
    asm volatile ("ldmxcsr %0"::"m"(mxcsr));
    if (mxcsr & 0b00000001)
        panic(frame, "SIMD fault detected (Invalid Operation)!");
    else if (mxcsr & 0b00000010)
        panic(frame, "SIMD fault detected (Denormal)!");
    else if (mxcsr & 0b00000100)
        panic(frame, "SIMD fault detected (Divide by Zero)!");
    else if (mxcsr & 0b00001000)
        panic(frame, "SIMD fault detected (Overflow)!");
    else if (mxcsr & 0b00010000)
        panic(frame, "SIMD fault detected (Underflow)!");
    else if (mxcsr & 0b00100000)
        panic(frame, "SIMD fault detected (Precision)!");
    else if (mxcsr & 0b01000000)
        panic(frame, "SIMD fault detected (Denormals are Zero)!");
    else panic(frame, "Unknown SIMD fault");
    while (true)
        asm ("hlt");
}

void remap_pic() {
    // SAVE INTERRUPT MASKS.
    u8 parentMasks;
    u8 childMasks;
    parentMasks = in8(PIC1_DATA);
    io_wait();
    childMasks = in8(PIC2_DATA);
    io_wait();
    // INITIALIZE BOTH CHIPS IN CASCADE MODE.
    out8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    // SET VECTOR OFFSET OF MASTER PIC.
    //   This allows software to throw low interrupts as normal (0-32)
    //     without triggering an IRQ that would normally be attributed to hardware.
    out8(PIC1_DATA, PIC_IRQ_VECTOR_OFFSET);
    io_wait();
    // SET VECTOR OFFSET OF SLAVE PIC.
    out8(PIC2_DATA, PIC_IRQ_VECTOR_OFFSET + 8);
    io_wait();
    // TELL MASTER THERE IS A SLAVE ON IRQ2.
    out8(PIC1_DATA, 4);
    io_wait();
    // TELL SLAVE IT'S CASCADE IDENTITY.
    out8(PIC2_DATA, 2);
    io_wait();
    // NOT QUITE SURE WHAT THIS DOES YET.
    out8(PIC1_DATA, ICW4_8086);
    io_wait();
    out8(PIC2_DATA, ICW4_8086);
    io_wait();
    // LOAD INTERRUPT MASKS.
    out8(PIC1_DATA, parentMasks);
    io_wait();
    out8(PIC2_DATA, childMasks);
    io_wait();
}

#include <scheduler.h>
__attribute__((no_caller_saved_registers))
void scheduler_switch(CPUState* cpu) {
    Scheduler::switch_process(cpu);
};
