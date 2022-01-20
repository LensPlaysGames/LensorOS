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

#include <stdint.h>
#include "../basic_renderer.h"
#include "../panic.h"
#include "../io.h"
#include "../keyboard.h"
#include "../mouse.h"
#include "../timer.h"

struct InterruptFrame;
// GENERAL INTERRUPTS
__attribute__((interrupt)) void SystemTimerHandler				(InterruptFrame* frame);
__attribute__((interrupt)) void KeyboardHandler					(InterruptFrame* frame);
__attribute__((interrupt)) void MouseHandler					(InterruptFrame* frame);
// FAULT HANDLING
__attribute__((interrupt)) void PageFaultHandler				(InterruptFrame* frame);
__attribute__((interrupt)) void DoubleFaultHandler				(InterruptFrame* frame);
__attribute__((interrupt)) void GeneralProtectionFaultHandler	(InterruptFrame* frame);


void RemapPIC();
void EndMasterPIC();
void EndSlavePIC();

#endif
