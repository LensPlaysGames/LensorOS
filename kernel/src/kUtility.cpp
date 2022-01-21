#include "kUtility.h"

KernelInfo kInfo;
PageTableManager PTM = NULL;

void prepare_memory(BootInfo* bInfo) {
	// Setup global page frame allocator
	gAlloc = PageFrameAllocator();
	// Setup memory map
	gAlloc.read_efi_memory_map(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
	// _KernelStart and _KernelEnd defined in linker script "../kernel.ld"
	uint64_t kernelSize = (uint64_t)&_KernelEnd - (uint64_t)&_KernelStart;
	uint64_t kernelPagesNeeded = (uint64_t)kernelSize / 4096 + 1;
	gAlloc.lock_pages(&_KernelStart, kernelPagesNeeded);
	// PAGE MAP LEVEL FOUR (see paging.h).
    PageTable* PML4 = (PageTable*)gAlloc.request_pages(0x100);
	// PAGE TABLE MANAGER
    PTM = PageTableManager(PML4);
	kInfo.PTM = &PTM;
	// Map all physical RAM addresses to virtual addresses, store them in the PML4.
	for (uint64_t t = 0;
		 t < GetMemorySize(bInfo->map, bInfo->mapSize / bInfo->mapDescSize, bInfo->mapDescSize);
		 t+=0x1000)
	{
		PTM.MapMemory((void*)t, (void*)t);
	}
    // Value of Control Register 3 = Address of the page directory in physical form.
	asm ("mov %0, %%cr3" : : "r" (PML4));
}

IDTR idtr;
void set_idt_gate(void* handler, uint8_t entryOffset, uint8_t type_attr, uint8_t selector) {
	IDTDescEntry* interrupt = (IDTDescEntry*)(idtr.Offset + entryOffset * sizeof(IDTDescEntry));
	interrupt->SetOffset((uint64_t)handler);
	interrupt->type_attr = type_attr;
	interrupt->selector = selector;
}

void prepare_interrupts() {
	idtr.Limit = 0x0FFF;
	idtr.Offset = (uint64_t)gAlloc.request_page();
	// SET CALLBACK TO HANDLER BASED ON INTERRUPT ENTRY OFFSET.
	// SYSTEM TIMER (PIT CHIP on IRQ0)
	set_idt_gate((void*)system_timer_handler,				0x20, IDT_TA_InterruptGate, 0x08);
	// PS/2 KEYBOARD (SERIAL)
	set_idt_gate((void*)keyboard_handler,					0x21, IDT_TA_InterruptGate, 0x08);
	// PS/2 MOUSE    (SERIAL)
	set_idt_gate((void*)mouse_handler,						0x2c, IDT_TA_InterruptGate, 0x08);
	// FAULTS (POSSIBLE TO RECOVER)
	set_idt_gate((void*)page_fault_handler,					0x0E, IDT_TA_InterruptGate, 0x08);
	set_idt_gate((void*)double_fault_handler,				0x08, IDT_TA_InterruptGate, 0x08);
	set_idt_gate((void*)general_protection_fault_handler,	0x0D, IDT_TA_InterruptGate, 0x08);
	// LOAD INTERRUPT DESCRIPTOR TABLE.
	asm ("lidt %0" :: "m" (idtr));
	// REMAP PIC CHIP IRQs OUT OF THE WAY OF GENERAL SOFTWARE EXCEPTIONS.
	// IRQs now start at 0x20 (what was `int 0` is now `int 32`).
	remap_pic();
}

Framebuffer target;
KernelInfo kernel_init(BootInfo* bInfo) {
	// DISABLE INTERRUPTS.
	asm ("cli");
	// SETUP GDT DESCRIPTOR.
	GDTDescriptor GDTD = GDTDescriptor();
	GDTD.Size = sizeof(GDT) - 1;
	GDTD.Offset = (uint64_t)&gGDT;
	// Call assembly `lgdt`.
	LoadGDT(&GDTD);
	// PREPARE MEMORY.
	prepare_memory(bInfo);
	// SETUP GOP RENDERER.
	target = *bInfo->framebuffer;
	// GOP = Graphics Output Protocol.
	uint64_t fbBase = (uint64_t)bInfo->framebuffer->BaseAddress;
	uint64_t fbSize = (uint64_t)bInfo->framebuffer->BufferSize + 0x1000;
	uint64_t fbPages = fbSize / 0x1000 + 1;
	// ALLOCATE PAGES IN BITMAP FOR ACTIVE FRAMEBUFFER (CURRENT DISPLAY MEMORY).
	gAlloc.lock_pages(bInfo->framebuffer->BaseAddress, fbPages);
	// Map active framebuffer physical address to virtual addresses 1:1.
	for (uint64_t t = fbBase; t < fbBase + fbSize; t += 0x1000) {
		PTM.MapMemory((void*)t, (void*)t);
	}
	// ALLOCATE PAGES IN BITMAP FOR TARGET FRAMEBUFFER (WRITABLE).
	target.BaseAddress = gAlloc.request_pages(fbPages);
    fbBase = (uint64_t)target.BaseAddress;
	for (uint64_t t = fbBase; t < fbBase + fbSize; t += 0x1000) {
		PTM.MapMemory((void*)t, (void*)t);
	}
	// CREATE GLOBAL RENDERER
	gRend = BasicRenderer(bInfo->framebuffer, &target, bInfo->font);
	// PREPARE HARDWARE INTERRUPTS (IDT).
	// IDT = INTERRUPT DESCRIPTOR TABLE.
	// Call assembly `lidt`.
	prepare_interrupts();
	// SYSTEM TIMER.
	initialize_timer(1000);
	// PREPARE PS/2 MOUSE.
	init_ps2_mouse();
	// CREATE GLOBAL DATE/TIME (RTC INIT)
	gRTC = RTC();
	// INTERRUPT MASKS.
	// 0 = UNMASKED, ALLOWED TO HAPPEN
	outb(PIC1_DATA, 0b11111000);
	outb(PIC2_DATA, 0b11101111);
	// ENABLE INTERRUPTS.
	asm ("sti");
	return kInfo;
}
