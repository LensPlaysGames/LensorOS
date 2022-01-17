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
	memset(PML4, 0, 0x1000);
	// I don't know why I need this line, but I do otherwise QEMU does WEIRD things.
	gAlloc.LockPages(PML4, 1000);
    PTM = PageTableManager(PML4);
	kInfo.PTM = &PTM;
	// Map EFI memory map into Page Map Level Four
	for (uint64_t t = 0; t < GetMemorySize(bInfo->map, bInfo->mapSize / bInfo->mapDescSize, bInfo->mapDescSize); t+=0x1000) {
		PTM.MapMemory((void*)t, (void*)t);
	}
	// Map framebuffer memory.
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
void PrepareInterrupts() {
	idtr.Limit = 0x0FFF;
	idtr.Offset = (uint64_t)gAlloc.RequestPage();

	IDTDescEntry* PageFault = (IDTDescEntry*)(idtr.Offset + 0xE * sizeof(IDTDescEntry));
	PageFault->SetOffset((uint64_t)PageFaultHandler);
	PageFault->type_attr = IDT_TA_InterruptGate;
	PageFault->selector = 0x08;

	IDTDescEntry* DoubleFault = (IDTDescEntry*)(idtr.Offset + 0x8 * sizeof(IDTDescEntry));
	DoubleFault->SetOffset((uint64_t)DoubleFaultHandler);
	DoubleFault->type_attr = IDT_TA_InterruptGate;
	DoubleFault->selector = 0x08;

	IDTDescEntry* GeneralProtectionFault = (IDTDescEntry*)(idtr.Offset + 0xD * sizeof(IDTDescEntry));
	GeneralProtectionFault->SetOffset((uint64_t)GeneralProtectionFaultHandler);
	GeneralProtectionFault->type_attr = IDT_TA_InterruptGate;
	GeneralProtectionFault->selector = 0x08;

	IDTDescEntry* Keyboard = (IDTDescEntry*)(idtr.Offset + 0x21 * sizeof(IDTDescEntry));
	Keyboard->SetOffset((uint64_t)KeyboardHandler);
	Keyboard->type_attr = IDT_TA_InterruptGate;
	Keyboard->selector = 0x08;

	asm ("lidt %0" :: "m" (idtr));

	RemapPIC();

	outb(PIC1_DATA, 0b11111101);
	outb(PIC2_DATA, 0b11111111);

	asm ("sti");
}


KernelInfo InitializeKernel(BootInfo* bInfo) {
	// SETUP GDT DESCRIPTOR
	GDTDescriptor GDTD = GDTDescriptor();
	GDTD.Size = sizeof(GDT) - 1;
	GDTD.Offset = (uint64_t)&gGDT;
	LoadGDT(&GDTD);
	// SETUP GOP RENDERER
	gRend = BasicRenderer(bInfo->framebuffer,bInfo->font);
	// Initialize screen to background color.
	gRend.clear();
	// GPLv3 LICENSE REQUIREMENT (interactive terminal must print cpy notice).
	gRend.putstr("<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>");
	gRend.crlf();
	// PREPARE MEMORY
	PrepareMemory(bInfo);
	gRend.putstr("Memory prepared successfully");
	gRend.crlf();
	gAlloc.PrintMemoryInfo(&gRend);

	PrepareInterrupts();
	
	return kInfo;
}
