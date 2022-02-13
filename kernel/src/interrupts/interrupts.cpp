#include "interrupts.h"

#include "../basic_renderer.h"
#include "../cstr.h"
#include "../io.h"
#include "../keyboard.h"
#include "../mouse.h"
#include "../panic.h"
#include "../pit.h"
#include "../rtc.h"
#include "../uart.h"

inline void end_of_interrupt(u8 IRQx) {
    if (IRQx >= 8)
        outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void cause_div_by_zero(u8 one) {
    one /= one - 1;
}

void cause_page_not_present() {
    u8* badAddr = (u8*)0xdeadc0de;
    u8 faultHere = *badAddr;
    (void)faultHere;
}

void cause_general_protection() {
    u8* badAddr = (u8*)0xdeadbeefb00bface;
    u8 faultHere = *badAddr;
    (void)faultHere;
}

// HARDWARE INTERRUPT HANDLERS (IRQs)
/// IRQ0: SYSTEM TIMER
__attribute__((interrupt)) void system_timer_handler(InterruptFrame* frame) {
    gPIT.Ticks += 1;
    end_of_interrupt(0);
}

/// IRQ1: PS/2 KEYBOARD
__attribute__((interrupt)) void keyboard_handler(InterruptFrame* frame) {
    Keyboard::gText.handle_scancode(inb(0x60));
    end_of_interrupt(1);
}

__attribute__((interrupt)) void uart_com1_handler(InterruptFrame* frame) {
    u8 data = srl->readb();
    end_of_interrupt(4);
    Keyboard::gText.handle_character(data);
}

/// IRQ8: PERIODIC (REAL TIME CLOCK)
/// NOTE: If register 'C' is not read from inside this handler,
///         no further interrupts of this type will be sent.
/// Status Register `C`:
///   Bits 0-3: Reserved (do not touch)
///          4: Update-ended interrupt
///          5: Alarm interrupt
///          6: Periodic Interrupt
///          7: Interrupt Request (IRQ)
__attribute__((interrupt)) void rtc_periodic_handler(InterruptFrame* frame) {
    u8 statusC = gRTC.read_register(0x0C);
    if (statusC & 0b01000000)
        gRTC.Ticks += 1;
    end_of_interrupt(8);
}

/// IRQ12: PS/2 MOUSE
__attribute__((interrupt)) void mouse_handler(InterruptFrame* frame) {
    u8 data = inb(0x60);
    handle_ps2_mouse_interrupt(data);
    // End interrupt
    end_of_interrupt(12);
}

// FAULT INTERRUPT HANDLERS

__attribute__((interrupt)) void divide_by_zero_handler(InterruptFrame* frame) {
    panic(frame, "Divide by zero detected!");
    while (true) {
        asm ("hlt");
    }
}

__attribute__((interrupt)) void page_fault_handler(InterruptFrame* frame, u64 err) {
    // POP ERROR CODE FROM STACK
    u64 address;
    asm volatile ("mov %%cr2, %0" : "=r" (address));
    // If bit 0 == 0, page not present
    if ((err & 0b1) == 0)
        panic(frame, "Page fault detected (page not present)");
    // If bit 1 == 1, caused by page write access
    else if (((err & 0b10) >> 1) == 1)
        panic(frame, "Page fault detected (Invalid page write access)");
    // If bit 5 == 1, caused by a protection key violation.
    else if (((err & 0b10000) >> 4) == 1)
        panic(frame, "Page fault detected (Protection-key violation)");
    // If bit 6 == 1, caused by a shadow stack access.
    else if (((err & 0b100000) >> 5) == 1)
        panic(frame, "Page fault detected (Shadow stack access)");
    else panic(frame, "Page fault detected");
    srl->writestr("  Faulty Address: 0x");
    srl->writestr(to_hexstring(address));
    srl->writestr("\r\n");
    gRend.puts("Faulty Address: 0x", 0x00000000);
    gRend.puts(to_hexstring(address), 0x00000000);
    gRend.swap();
    while (true) {
        asm ("hlt");
    }
}

__attribute__((interrupt)) void double_fault_handler(InterruptFrame* frame, u64 err) {
    panic(frame, "Double fault detected!");
    while (true) {
        asm ("hlt");
    }
}

__attribute__((interrupt)) void stack_segment_fault_handler(InterruptFrame* frame, u64 err) {
    if (err == 0)
        panic(frame, "Stack segment fault detected (0)");
    else panic(frame, "Stack segment fault detected (selector)!");
    srl->writestr("  Error Code: 0x");
    srl->writestr(to_hexstring(err));
    srl->writestr("\r\n");
    if (err & 0b1)
        srl->writestr("  External\r\n");
    
    u8 table = (err & 0b110) >> 1;
    if (table == 0b00)
        srl->writestr("  GDT");
    else if (table == 0b01 || table == 0b11)
        srl->writestr("  IDT");
    else if (table == 0b01)
        srl->writestr("  LDT");
    
    srl->writestr(" Selector Index: ");
    srl->writestr(to_hexstring(((err & 0b1111111111111000) >> 3)));
    srl->writestr("\r\n");

    gRend.puts("Err: 0x", 0x00000000);
    gRend.puts(to_hexstring(err), 0x00000000);
    gRend.crlf();
    gRend.swap();
}

__attribute__((interrupt)) void general_protection_fault_handler(InterruptFrame* frame, u64 err) {
    if (err == 0)
        panic(frame, "General protection fault detected (0)!");
    else panic(frame, "General protection fault detected (selector)!");

    srl->writestr("  Error Code: 0x");
    srl->writestr(to_hexstring(err));
    srl->writestr("\r\n");
    if (err & 0b1)
        srl->writestr("  External\r\n");
    
    u8 table = (err & 0b110) >> 1;
    if (table == 0b00)
        srl->writestr("  GDT");
    else if (table == 0b01 || table == 0b11)
        srl->writestr("  IDT");
    else if (table == 0b01)
        srl->writestr("  LDT");
    
    srl->writestr(" Selector Index: ");
    srl->writestr(to_hexstring(((err & 0b1111111111111000) >> 3)));
    srl->writestr("\r\n");
    gRend.puts("Err: 0x", 0x00000000);
    gRend.puts(to_hexstring(err), 0x00000000);
    gRend.crlf();
    gRend.swap();
    while (true) {
        asm ("hlt");
    }
}

void remap_pic() {
    // SAVE INTERRUPT MASKS.
    u8 masterMasks;
    u8 slaveMasks;
    masterMasks = inb(PIC1_DATA);
    io_wait();
    slaveMasks = inb(PIC2_DATA);
    io_wait();
    // INITIALIZE BOTH CHIPS IN CASCADE MODE.
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    // SET VECTOR OFFSET OF MASTER PIC.
    //   This allows software to throw low interrupts as normal (0-32)
    //     without triggering an IRQ that would normally be attributed to hardware.
    outb(PIC1_DATA, PIC_IRQ_VECTOR_OFFSET);
    io_wait();
    // SET VECTOR OFFSET OF SLAVE PIC.
    outb(PIC2_DATA, PIC_IRQ_VECTOR_OFFSET + 8);
    io_wait();
    // TELL MASTER THERE IS A SLAVE ON IRQ2.
    outb(PIC1_DATA, 4);
    io_wait();
    // TELL SLAVE IT'S CASCADE IDENTITY.
    outb(PIC2_DATA, 2);
    io_wait();
    // NOT QUITE SURE WHAT THIS DOES YET.
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    // LOAD INTERRUPT MASKS.
    outb(PIC1_DATA, masterMasks);
    io_wait();
    outb(PIC2_DATA, slaveMasks);
    io_wait();
}
