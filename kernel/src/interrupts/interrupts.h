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
#include "../BasicRenderer.h"
#include "../panic.h"
#include "../io.h"

struct InterruptFrame;
__attribute__((interrupt)) void PageFaultHandler(struct InterruptFrame* frame);
__attribute__((interrupt)) void DoubleFaultHandler(struct InterruptFrame* frame);
__attribute__((interrupt)) void GeneralProtectionFaultHandler(struct InterruptFrame* frame);
__attribute__((interrupt)) void KeyboardHandler(struct InterruptFrame* frame);

void RemapPIC();
void EndMasterPIC();
void EndSlavePIC();

#endif
