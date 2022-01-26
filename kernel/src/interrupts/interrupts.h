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
#include "../keyboard.h"
#include "../mouse.h"
#include "../timer.h"

struct InterruptFrame;
// GENERAL INTERRUPTS
__attribute__((interrupt)) void system_timer_handler   			    (InterruptFrame* frame);
__attribute__((interrupt)) void keyboard_handler				    (InterruptFrame* frame);
__attribute__((interrupt)) void mouse_handler					    (InterruptFrame* frame);
// FAULT HANDLING
__attribute__((interrupt)) void double_fault_handler                (InterruptFrame* frame);
__attribute__((interrupt)) void general_protection_fault_handler    (InterruptFrame* frame);
__attribute__((interrupt)) void page_fault_handler				    (InterruptFrame* frame);

void remap_pic();
void end_master_pic();
void end_slave_pic();

#endif
