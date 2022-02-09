#ifndef LENSOR_OS_INTERRUPTS_H
#define LENSOR_OS_INTERRUPTS_H

// Define PIC chip ports
#define PIC_EOI       0x20
#define PIC1_COMMAND  0x20
#define PIC1_DATA     0x21
#define PIC2_COMMAND  0xA0
#define PIC2_DATA     0xA1

#define ICW1_INIT     0x10
#define ICW1_ICW4     0x01
#define ICW4_8086     0x01

#include "../integers.h"

struct InterruptFrame {
    // Instruction Pointer
    u64 ip;
    // Code Segment
    u64 cs;
    u64 flags;
    // Stack Pointer
    u64 sp;
    // Segment Selector
    u64 ss;
} __attribute__((packed));

// HARDWARE INTERRUPT REQUESTS (IRQs)
__attribute__((interrupt)) void system_timer_handler (InterruptFrame*);
__attribute__((interrupt)) void keyboard_handler     (InterruptFrame*);
__attribute__((interrupt)) void rtc_periodic_handler (InterruptFrame*);
__attribute__((interrupt)) void mouse_handler        (InterruptFrame*);
// FAULT/TRAP HANDLING
__attribute__((interrupt)) void divide_by_zero_handler           (InterruptFrame*);
__attribute__((interrupt)) void double_fault_handler             (InterruptFrame*, u64);
__attribute__((interrupt)) void stack_segment_fault_handler      (InterruptFrame*, u64);
__attribute__((interrupt)) void general_protection_fault_handler (InterruptFrame*, u64);
__attribute__((interrupt)) void page_fault_handler               (InterruptFrame*, u64);

// HELPER FUNCTIONS TO TRIGGER HANDLERS FOR TESTING
void cause_div_by_zero(u8 one = 1);
void cause_page_not_present();
void cause_general_protection();

void remap_pic();

#endif
