#include "interrupts.h"

__attribute__((interrupt)) void PageFaultHandler(struct InterruptFrame* frame) {
	Panic("Page fault detected!");
	while (true) {
		asm ("hlt");
	}
}

__attribute__((interrupt)) void DoubleFaultHandler(struct InterruptFrame* frame) {
	Panic("Double fault detected!");
	while (true) {
		asm ("hlt");
	}
}

__attribute__((interrupt)) void GeneralProtectionFaultHandler(struct InterruptFrame* frame) {
	Panic("General protection fault detected!");
	while (true) {
		asm ("hlt");
	}
}

__attribute__((interrupt)) void KeyboardHandler(struct InterruptFrame* frame) {
	uint8_t scancode = inb(0x60);
	// Handle key press
	gRend.putstr("Pressed");
	
	// End interrupt	
	EndMasterPIC();
}

void EndMasterPIC() {
	outb(PIC1_COMMAND, PIC_EOI);
}
void EndSlavePIC() {
	outb(PIC2_COMMAND, PIC_EOI);
	outb(PIC1_COMMAND, PIC_EOI);
}

void RemapPIC() {
	uint8_t a1;
	uint8_t a2;

	a1 = inb(PIC1_DATA);
	io_wait();
	a2 = inb(PIC2_DATA);
	io_wait();

	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();

	outb(PIC1_DATA, 0x20);
	io_wait();
	outb(PIC2_DATA, 0x28);
	io_wait();

	outb(PIC1_DATA, 4);
	io_wait();
	outb(PIC2_DATA, 2);
	io_wait();

	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	outb(PIC1_DATA, a1);
	io_wait();
	outb(PIC2_DATA, a2);
	io_wait();
}
