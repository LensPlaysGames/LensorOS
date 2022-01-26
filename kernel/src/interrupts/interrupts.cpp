#include "interrupts.h"

inline void end_master_pic() {
	outb(PIC1_COMMAND, PIC_EOI);
}

inline void end_slave_pic() {
	outb(PIC2_COMMAND, PIC_EOI);
	outb(PIC1_COMMAND, PIC_EOI);
}

// HARDWARE INTERRUPT HANDLERS (IRQs)
__attribute__((interrupt)) void system_timer_handler(InterruptFrame* frame) {
	gTicks += 1;
	// End interrupt
	end_master_pic();
}

__attribute__((interrupt)) void keyboard_handler(InterruptFrame* frame) {
	u8 scancode = inb(0x60);
	handle_keyboard(scancode);
	// End interrupt	
	end_master_pic();
}

__attribute__((interrupt)) void mouse_handler(InterruptFrame* frame) {
	u8 data = inb(0x60);
	handle_ps2_mouse_interrupt(data);
	// End interrupt
	end_slave_pic();
}

// FAULT INTERRUPT HANDLERS
__attribute__((interrupt)) void page_fault_handler(InterruptFrame* frame) {
	// POP ERROR CODE FROM STACK
	u64 address;
	asm volatile ("mov %%cr2, %0" : "=r" (address));
	u32 err;
	asm volatile ("pop %%rax\n\t"
				  "mov %%eax, %0" : "=r" (err));
	// If bit 0 == 0, page not present
	if ((err & 0b1) == 0) {
		panic("Page fault detected (page not present)");
	}
	// If bit 1 == 1, caused by page write access
	else if (((err & 0b10) >> 1) == 1) {
		panic("Page fault detected (Invalid page write access)");
	}
	// If bit 5 == 1, caused by a protection key violation.
	else if (((err & 0b10000) >> 4) == 1) {
		panic("Page fault detected (Protection-key violation)");
	}
	// If bit 6 == 1, caused by a shadow stack access.
	else if (((err & 0b100000) >> 5) == 1) {
		panic("Page fault detected (Shadow stack access)");
	}
	else {
		panic("Page fault detected");
	}
	gRend.putstr(to_string(address));
	while (true) {
		asm ("hlt");
	}
}

__attribute__((interrupt)) void double_fault_handler(InterruptFrame* frame) {
	// ERROR CODE FROM DOUBLE FAULT ALWAYS ZERO (never useful).
	asm volatile ("pop %rax");
	panic("Double fault detected!");
	while (true) {
		asm ("hlt");
	}
}

__attribute__((interrupt)) void general_protection_fault_handler(InterruptFrame* frame) {
	// POP ERROR CODE FROM STACK (segment selector if segment related fault)
	asm volatile ("pop %rax");
	panic("General protection fault detected!");
	while (true) {
		asm ("hlt");
	}
}

void remap_pic() {
	// SAVE INTERRUPT MASKS
	u8 masterMasks;
	u8 slaveMasks;
	masterMasks = inb(PIC1_DATA);
	io_wait();
	slaveMasks = inb(PIC2_DATA);
	io_wait();
	// START INIT IN CASCADE MODE
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	// SET VECTOR OFFSET OF MASTER PIC
	outb(PIC1_DATA, 0x20);
	io_wait();
	// SET VECTOR OFFSET OF SLAVE PIC
	//   This allows software to throw low interrupts as normal (0-32)
	//     without triggering an IRQ. This allows specific software and
	//     hardware error handling.
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
	outb(PIC1_DATA, masterMasks);
	io_wait();
	outb(PIC2_DATA, slaveMasks);
	io_wait();
}
