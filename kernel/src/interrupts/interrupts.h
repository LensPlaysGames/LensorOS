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
#include "../basic_renderer.h"
#include "../panic.h"
#include "../io.h"
/// IRQ0
#include "../pit.h"
/// IRQ1
#include "../keyboard.h"
/// IRQ12
#include "../mouse.h"


struct InterruptFrame;
// HARDWARE INTERRUPT REQUESTS (IRQs)
__attribute__((interrupt)) void system_timer_handler (InterruptFrame*);
__attribute__((interrupt)) void keyboard_handler	 (InterruptFrame*);
__attribute__((interrupt)) void rtc_periodic_handler (InterruptFrame*);
__attribute__((interrupt)) void mouse_handler		 (InterruptFrame*);
// FAULT/TRAP HANDLING
__attribute__((interrupt)) void double_fault_handler             (InterruptFrame*, u64);
__attribute__((interrupt)) void general_protection_fault_handler (InterruptFrame*, u64);
__attribute__((interrupt)) void page_fault_handler				 (InterruptFrame*, u64);

void remap_pic();

#endif
