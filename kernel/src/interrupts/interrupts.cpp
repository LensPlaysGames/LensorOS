#include "interrupts.h"

inline void end_master_pic() {
	outb(PIC1_COMMAND, PIC_EOI);
}

inline void end_slave_pic() {
	outb(PIC2_COMMAND, PIC_EOI);
	outb(PIC1_COMMAND, PIC_EOI);
}

__attribute__((interrupt)) void SystemTimerHandler(InterruptFrame* frame) {
	gTicks += 1;
	// End interrupt
	end_master_pic();
}

__attribute__((interrupt)) void KeyboardHandler(InterruptFrame* frame) {
	uint8_t scancode = inb(0x60);
	HandleKeyboard(scancode);
	// End interrupt	
	end_master_pic();
}

__attribute__((interrupt)) void MouseHandler(InterruptFrame* frame) {
	uint8_t data = inb(0x60);
	HandlePS2Mouse(data);
	// End interrupt
	end_slave_pic();
}

__attribute__((interrupt)) void PageFaultHandler(InterruptFrame* frame) {
	Panic("Page fault detected!");
	while (true) {
		asm ("hlt");
	}
}

__attribute__((interrupt)) void DoubleFaultHandler(InterruptFrame* frame) {
	Panic("Double fault detected!");
	while (true) {
		asm ("hlt");
	}
}

__attribute__((interrupt)) void GeneralProtectionFaultHandler(InterruptFrame* frame) {
	Panic("General protection fault detected!");
	while (true) {
		asm ("hlt");
	}
}

void remap_pic() {
	// SAVE INTERRUPT MASKS
	uint8_t a1;
	uint8_t a2;
	a1 = inb(PIC1_DATA);
	a2 = inb(PIC2_DATA);
	// START INIT IN CASCADE MODE
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	// SET VECTOR OFFSET OF MASTER PIC
	outb(PIC1_DATA, 0x20);
	io_wait();
	// SET VECTOR OFFSET OF SLAVE PIC
	outb(PIC2_DATA, 0x28);
	io_wait();
	// TELL MASTER THERE IS A SLAVE ON IRQ2
	outb(PIC1_DATA, 4);
	io_wait();
	// TELL SLAVE IT'S CASCADE IDENTITY
	outb(PIC2_DATA, 2);
	io_wait();
	// NOT QUITE SURE WHAT THIS DOES YET
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
	// LOAD INTERRUPT MASKS
	outb(PIC1_DATA, a1);
	outb(PIC2_DATA, a2);
}
