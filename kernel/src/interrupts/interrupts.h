#ifndef LENSOR_OS_INTERRUPTS_H
#define LENSOR_OS_INTERRUPTS_H

#define PIC_IRQ_VECTOR_OFFSET 0x20
#define PIC_IRQ0  PIC_IRQ_VECTOR_OFFSET + 0
#define PIC_IRQ1  PIC_IRQ_VECTOR_OFFSET + 1
#define PIC_IRQ2  PIC_IRQ_VECTOR_OFFSET + 2
#define PIC_IRQ3  PIC_IRQ_VECTOR_OFFSET + 3
#define PIC_IRQ4  PIC_IRQ_VECTOR_OFFSET + 4
#define PIC_IRQ5  PIC_IRQ_VECTOR_OFFSET + 5
#define PIC_IRQ6  PIC_IRQ_VECTOR_OFFSET + 6
#define PIC_IRQ7  PIC_IRQ_VECTOR_OFFSET + 7
#define PIC_IRQ8  PIC_IRQ_VECTOR_OFFSET + 8
#define PIC_IRQ9  PIC_IRQ_VECTOR_OFFSET + 9
#define PIC_IRQ10 PIC_IRQ_VECTOR_OFFSET + 10
#define PIC_IRQ11 PIC_IRQ_VECTOR_OFFSET + 11
#define PIC_IRQ12 PIC_IRQ_VECTOR_OFFSET + 12
#define PIC_IRQ13 PIC_IRQ_VECTOR_OFFSET + 13
#define PIC_IRQ14 PIC_IRQ_VECTOR_OFFSET + 14
#define PIC_IRQ15 PIC_IRQ_VECTOR_OFFSET + 15

// Define PIC chip ports
#define PIC_EOI       0x20
#define PIC1_COMMAND  0x20
#define PIC1_DATA     0x21
#define PIC2_COMMAND  0xa0
#define PIC2_DATA     0xa1

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
__attribute__((interrupt)) void uart_com1_handler    (InterruptFrame*);
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
