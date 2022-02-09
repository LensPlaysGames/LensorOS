#include "kUtility.h"

// GRAPHICS
#include "basic_renderer.h"
// BITMAP ABSTRACTION
#include "bitmap.h"
// to_string, to_hexstring
#include "cstr.h"
// MEMORY MANAGEMENT
#include "efi_memory.h"
#include "memory.h"
#include "paging/paging.h"
#include "paging/page_table_manager.h"
#include "heap.h"
// GLOBAL DESCRIPTOR TABLE
#include "gdt.h"
// INTERRUPTS
#include "interrupts/idt.h"
#include "interrupts/interrupts.h"
#include "interrupts/syscalls.h"
// SERIAL BUS COMMUNICATION
#include "io.h"
// TIMING
#include "pit.h"
#include "rtc.h"
// SYSTEM TABLES
#include "acpi.h"
#include "pci.h"
// SERIAL COMMUNICATIONS
#include "uart.h"
// FILESYSTEM/DISK DRIVERS
#include "FATDriver.h"
// INPUT
#include "mouse.h"

KernelInfo kInfo;

void prepare_memory(BootInfo* bInfo) {
    // Setup global page frame allocator.
    // Page = 4kb of virtual memory.
    // Frame = 4kb of physical memory.
    gAlloc = PageFrameAllocator();
    // Setup memory state from EFI memory map.
    gAlloc.read_efi_memory_map(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    // _KernelStart and _KernelEnd defined in linker script "../kernel.ld"
    u64 kernelPagesNeeded = (((u64)&_KernelEnd - (u64)&_KernelStart) / 4096) + 1;
    gAlloc.lock_pages(&_KernelStart, kernelPagesNeeded);
    // PAGE MAP LEVEL FOUR (see paging.h).
    PageTable* PML4 = (PageTable*)gAlloc.request_page();
    // PAGE TABLE MANAGER
    gPTM = PageTableManager(PML4);
    kInfo.PTM = &gPTM;
    // Map all physical RAM addresses to virtual addresses 1:1, store them in the PML4.
    // This means that virtual addresses will be equal to physical addresses.
    u64 memSize = get_memory_size(bInfo->map, bInfo->mapSize / bInfo->mapDescSize, bInfo->mapDescSize);
    for (u64 t = 0; t < memSize; t+=0x1000)
        gPTM.map_memory((void*)t, (void*)t);

    // Value of Control Register 3 = Address of the page directory in physical form.
    asm ("mov %0, %%cr3" : : "r" (PML4));
}

IDTR idtr;
void set_idt_gate(u64 handler, u8 entryOffset, u8 type_attr = IDT_TA_InterruptGate, u8 selector = 0x08) {
    IDTDescEntry* interrupt = (IDTDescEntry*)(idtr.Offset + entryOffset * sizeof(IDTDescEntry));
    interrupt->SetOffset(handler);
    interrupt->type_attr = type_attr;
    interrupt->selector = selector;
}

void prepare_interrupts() {
    idtr.Limit = 0x0FFF;
    idtr.Offset = (u64)gAlloc.request_page();
    // SET CALLBACK TO HANDLER BASED ON INTERRUPT ENTRY OFFSET.
    // IRQ0: SYSTEM TIMER  (PIT CHIP)
    set_idt_gate((u64)system_timer_handler,             0x20);
    // IRQ1: PS/2 KEYBOARD (SERIAL)
    set_idt_gate((u64)keyboard_handler,                 0x21);
    set_idt_gate((u64)rtc_periodic_handler,             0x28);
    // IRQ12: PS/2 MOUSE   (SERIAL)
    set_idt_gate((u64)mouse_handler,                    0x2c);
    // FAULTS (CALLED BEFORE FAULTY INSTRUCTION EXECUTES)
    set_idt_gate((u64)divide_by_zero_handler,           0x00);
    set_idt_gate((u64)double_fault_handler,             0x08);
    set_idt_gate((u64)stack_segment_fault_handler,      0x0c);
    set_idt_gate((u64)general_protection_fault_handler, 0x0d);
    set_idt_gate((u64)page_fault_handler,               0x0e);
    // USER MODE SYSTEM CALLS
    set_idt_gate((u64)system_call_handler_asm,          0x80, IDT_TA_UserInterruptGate);
    // LOAD INTERRUPT DESCRIPTOR TABLE.
    asm ("lidt %0" :: "m" (idtr));
    // REMAP PIC CHIP IRQs OUT OF THE WAY OF GENERAL SOFTWARE EXCEPTIONS.
    // IRQs now start at 0x20 (what was `int 0` is now `int 32`).
    remap_pic();
}

void prepare_acpi(BootInfo* bInfo) {
    // eXtended System Descriptor Table
    ACPI::SDTHeader* xsdt = (ACPI::SDTHeader*)(bInfo->rsdp->XSDTAddress);
    // Memory-mapped ConFiguration Table
    ACPI::MCFGHeader* mcfg = (ACPI::MCFGHeader*)ACPI::find_table(xsdt, (char*)"MCFG");
    PCI::enumerate_pci(mcfg);
}

Framebuffer target;
KernelInfo kernel_init(BootInfo* bInfo) {
    // DISABLE INTERRUPTS.
    asm ("cli");
    // SETUP GDT DESCRIPTOR.
    GDTDescriptor GDTD = GDTDescriptor();
    GDTD.Size = sizeof(GDT) - 1;
    GDTD.Offset = (u64)&gGDT;
    // Call assembly `lgdt`.
    LoadGDT(&GDTD);
    // PREPARE MEMORY.
    prepare_memory(bInfo);
    // SETUP KERNEL HEAP.
    init_heap((void*)0x700000000000, 1);
    // SETUP SERIAL I/O.
    srl = new UARTDriver;
    srl->writestr("\r\n\r\n<<>><<<!===--- You are now booting into \033[1;33mLensorOS\033[0m ---===!>>><<>>\r\n\r\n");
    srl->writestr("[kUtil]: Mapped physical memory from 0x");
    srl->writestr(to_hexstring(0ULL));
    srl->writestr(" thru ");
    srl->writestr(to_hexstring((u64)get_memory_size(bInfo->map, bInfo->mapSize / bInfo->mapDescSize, bInfo->mapDescSize)));
    srl->writestr("\r\n");
    srl->writestr("[kUtil]:\r\n  Kernel loaded from 0x");
    srl->writestr(to_hexstring((u64)&_KernelStart));
    srl->writestr(" to 0x");
    srl->writestr(to_hexstring((u64)&_KernelEnd));
    srl->writestr("\r\n");
    srl->writestr("    .text:   0x");
    srl->writestr(to_hexstring((u64)&_TextStart));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring((u64)&_TextEnd));
    srl->writestr("\r\n");
    srl->writestr("    .data:   0x");
    srl->writestr(to_hexstring((u64)&_DataStart));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring((u64)&_DataEnd));
    srl->writestr("\r\n");
    srl->writestr("    .rodata: 0x");
    srl->writestr(to_hexstring((u64)&_ReadOnlyDataStart));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring((u64)&_ReadOnlyDataEnd));
    srl->writestr("\r\n");
    srl->writestr("    .bss:    0x");
    srl->writestr(to_hexstring((u64)&_BlockStartingSymbolsStart));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring((u64)&_BlockStartingSymbolsEnd));
    srl->writestr("\r\n\r\n");
    srl->writestr("[kUtil]: Heap mapped to 0x");
    srl->writestr(to_hexstring((u64)sHeapStart));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring((u64)sHeapEnd));
    srl->writestr("\r\n");
    srl->writestr("[kUtil]: Setting up Graphics Output Protocol Renderer\r\n");
    // SETUP GOP RENDERER.
    target = *bInfo->framebuffer;
    // GOP = Graphics Output Protocol.
    u64 fbBase = (u64)bInfo->framebuffer->BaseAddress;
    u64 fbSize = (u64)bInfo->framebuffer->BufferSize + 0x1000;
    u64 fbPages = fbSize / 0x1000 + 1;
    // ALLOCATE PAGES IN BITMAP FOR ACTIVE FRAMEBUFFER (CURRENT DISPLAY MEMORY).
    gAlloc.lock_pages(bInfo->framebuffer->BaseAddress, fbPages);
    // Map active framebuffer physical address to virtual addresses 1:1.
    for (u64 t = fbBase; t < fbBase + fbSize; t += 0x1000)
        gPTM.map_memory((void*)t, (void*)t);

    srl->writestr("  Active GOP framebuffer mapped to 0x");
    srl->writestr(to_hexstring(fbBase));
    srl->writestr(" thru ");
    srl->writestr(to_hexstring(fbBase + fbSize));
    srl->writestr("\r\n");
    // ALLOCATE PAGES IN BITMAP FOR TARGET FRAMEBUFFER (ARBITRARY WRITE).
    target.BaseAddress = gAlloc.request_pages(fbPages);
    fbBase = (u64)target.BaseAddress;
    for (u64 t = fbBase; t < fbBase + fbSize; t += 0x1000)
        gPTM.map_memory((void*)t, (void*)t);

    srl->writestr("  Deferred GOP framebuffer mapped to 0x");
    srl->writestr(to_hexstring(fbBase));
    srl->writestr(" thru ");
    srl->writestr(to_hexstring(fbBase + fbSize));
    srl->writestr("\r\n");

    // CREATE GLOBAL RENDERER
    gRend = BasicRenderer(bInfo->framebuffer, &target, bInfo->font);
    gRend.clear();
    gRend.puts("<<>><<<!===--- You are now booting into LensorOS ---===!>>><<>>");
    gRend.crlf();
    gRend.swap();
    srl->writestr("    \033[32mGOP Renderer setup successful\033[0m\r\n");
    // DRAW A FACE :)
    // left eye
    gRend.DrawPos = {420, 420};
    gRend.drawrect({42, 42}, 0xff00ffff);
    // left pupil
    gRend.DrawPos = {440, 440};
    gRend.drawrect({20, 20}, 0xffff0000);
    // right eye
    gRend.DrawPos = {520, 420};
    gRend.drawrect({42, 42}, 0xff00ffff);
    // right pupil
    gRend.DrawPos = {540, 440};
    gRend.drawrect({20, 20}, 0xffff0000);
    // mouth
    gRend.DrawPos = {400, 520};
    gRend.drawrect({182, 20}, 0xff00ffff);
    gRend.swap({400, 420}, {182, 120});
    // PREPARE HARDWARE INTERRUPTS (IDT).
    // IDT = INTERRUPT DESCRIPTOR TABLE.
    // Call assembly `lidt`.
    srl->writestr("[kUtil]: Preparing interrupts.\r\n");
    prepare_interrupts();
    srl->writestr("    \033[32mInterrupts prepared successfully.\033[0m\r\n");
    // SYSTEM TIMER.
    gPIT = PIT();
    gPIT.initialize_pit();
    srl->writestr("[kUtil]: Programmable Interval Timer initialized.\r\n");
    srl->writestr("  Channel 0, H/L Bit Access\r\n");
    srl->writestr("  Rate Generator, BCD Disabled\r\n");
    srl->writestr("  Periodic interrupts at ");
    srl->writestr(to_string(PIT_FREQUENCY));
    srl->writestr("hz.\r\n");
    // INITIALIZE REAL TIME CLOCK.
    gRTC = RTC();
    srl->writestr("[kUtil]: Real Time Clock initialized.\r\n");
    gRTC.set_periodic_int_enabled(true);
    srl->writestr("  Periodic interrupts at ");
    srl->writestr(to_string((double)RTC_PERIODIC_HERTZ));
    srl->writestr("hz\r\n");
    // PRINT REAL TIME TO SERIAL OUTPUT.
    srl->writestr("[kUtil]: \033[1;33mNow is ");
    srl->writestr(to_string(gRTC.Time.hour));
    srl->writeb(':');
    srl->writestr(to_string(gRTC.Time.minute));
    srl->writeb(':');
    srl->writestr(to_string(gRTC.Time.second));
    srl->writestr(" on ");
    srl->writestr(to_string(gRTC.Time.year));
    srl->writeb('-');
    srl->writestr(to_string(gRTC.Time.month));
    srl->writeb('-');
    srl->writestr(to_string(gRTC.Time.date));
    srl->writestr("\033[0m\r\n");
    // PREPARE DRIVERS.
    gFATDriver = FATDriver();
    // TODO: PREPARE DEVICE TREE.
    // SYSTEM INFORMATION IS FOUND IN ACPI TABLE
    prepare_acpi(bInfo);
    srl->writestr("[kUtil]: ACPI prepared.\r\n");
    // PREPARE PS/2 MOUSE.
    init_ps2_mouse();
    /// INTERRUPT MASKS (IRQs).
    /// 0 = UNMASKED, ALLOWED TO HAPPEN
    /// System Timer, PS/2 Keyboard, Interrupts Enabled
    outb(PIC1_DATA, 0b11111000);
    io_wait();
    /// Real time clock, PS/2 Mouse
    outb(PIC2_DATA, 0b11101110);
    io_wait();
    // ENABLE INTERRUPTS.
    srl->writestr("[kUtil]: Interrupt masks sent, enabling interrupts.\r\n");
    asm ("sti");
    srl->writestr("    \033[32mInterrupts enabled.\033[0m\r\n");
    return kInfo;
}
