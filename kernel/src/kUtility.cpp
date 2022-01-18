#include "kUtility.h"

KernelInfo kInfo;
PageTableManager PTM = NULL;

void PrepareMemory(BootInfo* bInfo) {
	// Setup global page frame allocator
	gAlloc = PageFrameAllocator();
	// Setup memory map
	gAlloc.ReadEfiMemoryMap(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
	uint64_t kernelSize = (uint64_t)&_KernelEnd - (uint64_t)&_KernelStart;
	uint64_t kernelPagesNeeded = (uint64_t)kernelSize / 4096 + 1;
	gAlloc.LockPages(&_KernelStart, kernelPagesNeeded);
	// PAGE MAP LEVEL FOUR (see paging.h).
    PageTable* PML4 = (PageTable*)gAlloc.RequestPage();
	// Init PML4 page to 0.
	memset(PML4, 0, 0x1000);
	// This line is necessary to fix a bug in QEMU regarding a rainbow pattern at the top of the screen.
	// I don't know why this is needed; I tried every value starting at 1 until it worked.
	gAlloc.LockPages(PML4, 0x100);
    PTM = PageTableManager(PML4);
	kInfo.PTM = &PTM;
	// Map all physical RAM addresses to virtual addresses, store them in the PML4.
	// This is done so that it can be used (free memory).
	// The whole point of paging is that more pages could be created than the size of the physical memory.
	for (uint64_t t = 0; t < GetMemorySize(bInfo->map, bInfo->mapSize / bInfo->mapDescSize, bInfo->mapDescSize); t+=0x1000) {
		PTM.MapMemory((void*)t, (void*)t);
	}
	// Active framebuffer is passed to us through EFI.
	// Map physical address of active framebuffer to virtual addresses within the PML4.
	// This will prevent any other processes/allocations from over-writing this memory we are using.
	uint64_t fbBase = (uint64_t)bInfo->framebuffer->BaseAddress;
	uint64_t fbSize = (uint64_t)bInfo->framebuffer->BufferSize + 0x1000;
	gAlloc.LockPages((void*)fbBase, fbSize / 0x1000 + 1);
	for (uint64_t t = fbBase; t < fbBase + fbSize; t += 0x1000) {
		PTM.MapMemory((void*)t, (void*)t);
	}
	// Load PML4 into correct register (use new mapped framebuffer).
	asm ("mov %0, %%cr3" : : "r" (PML4));
}

IDTR idtr;
void SetIDTGate(void* handler, uint8_t entryOffset, uint8_t type_attr, uint8_t selector) {
	IDTDescEntry* interrupt = (IDTDescEntry*)(idtr.Offset + entryOffset * sizeof(IDTDescEntry));
	interrupt->SetOffset((uint64_t)handler);
	interrupt->type_attr = type_attr;
	interrupt->selector = selector;
}

void PrepareInterrupts() {
	idtr.Limit = 0x0FFF;
	idtr.Offset = (uint64_t)gAlloc.RequestPage();
	// SET CALLBACK TO HANDLER BASED ON INTERRUPT ENTRY OFFSET.
	SetIDTGate((void*)PageFaultHandler,				 0x0E, IDT_TA_InterruptGate, 0x08);
	SetIDTGate((void*)DoubleFaultHandler,			 0x08, IDT_TA_InterruptGate, 0x08);
	SetIDTGate((void*)GeneralProtectionFaultHandler, 0x0D, IDT_TA_InterruptGate, 0x08);
	// PS/2 KEYBOARD (SERIAL)
	SetIDTGate((void*)KeyboardHandler,				 0x21, IDT_TA_InterruptGate, 0x08);
	// PS/2 MOUSE    (SERIAL)
	SetIDTGate((void*)MouseHandler,					 0x2c, IDT_TA_InterruptGate, 0x08);
	// LOAD INTERRUPT DESCRIPTOR TABLE.
	asm ("lidt %0" :: "m" (idtr));
	// REMAP PIC CHIP TO ENSURE IT DOESN'T JUMBLE-UP INTERRUPTS.
	RemapPIC();
}

KernelInfo InitializeKernel(BootInfo* bInfo) {
	// SETUP GDT DESCRIPTOR.
	GDTDescriptor GDTD = GDTDescriptor();
	GDTD.Size = sizeof(GDT) - 1;
	GDTD.Offset = (uint64_t)&gGDT;
	// Call assembly `lgdt`.
	LoadGDT(&GDTD);
	// SETUP GOP RENDERER.
	// GOP = Graphics Output Protocol.
	gRend = BasicRenderer(bInfo->framebuffer, bInfo->font);
	// Initialize screen to background color.
	gRend.clear();
	// GPLv3 LICENSE REQUIREMENT (interactive terminal must print cpy notice).
	gRend.putstr("<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>");
	gRend.crlf();
	// END GPLv3 LICENSE REQUIREMENT.
	// PREPARE MEMORY.
	PrepareMemory(bInfo);
	// PRINT MEMORY INFORMATION.
	// FREE, USED, RESERVED MEMORY.
	gRend.putstr("Memory prepared successfully");
	gRend.crlf();
	gAlloc.PrintMemoryInfo();
	// PREPARE HARDWARE INTERRUPTS (IDT).
	// IDT = INTERRUPT DESCRIPTOR TABLE.
	// Call assembly `lidt`.
	PrepareInterrupts();
	// PREPARE PS/2 MOUSE (INTERRUPT BASED).
	InitPS2Mouse();
	// USE I/O BUS TO SEND BITMASKS.
	outb(PIC1_DATA, 0b11111001);
	outb(PIC2_DATA, 0b11101111);
	// SET INTERRUPT FLAG (ENABLE MASKABLE INTERRUPTS).
	asm ("sti");
	return kInfo;
}
