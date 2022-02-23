#include "kUtility.h"

#include "acpi.h"
#include "basic_renderer.h"
#include "bitmap.h"
#include "cstr.h"
#include "efi_memory.h"
#include "fat_definitions.h"
#include "fat_driver.h"
#include "gdt.h"
#include "heap.h"
#include "hpet.h"
#include "interrupts/idt.h"
#include "interrupts/interrupts.h"
#include "interrupts/syscalls.h"
#include "io.h"
#include "keyboard.h"
#include "memory.h"
#include "mouse.h"
#include "paging/paging.h"
#include "paging/page_frame_allocator.h"
#include "paging/page_table_manager.h"
#include "pci.h"
#include "pit.h"
#include "random_lcg.h"
#include "random_lfsr.h"
#include "rtc.h"
#include "scheduler.h"
#include "tss.h"
#include "uart.h"

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
    // Map all physical RAM addresses to virtual addresses 1:1, store them in the PML4.
    // This means that virtual addresses will be equal to physical addresses.
    u64 memSize = get_memory_size(bInfo->map, bInfo->mapSize / bInfo->mapDescSize, bInfo->mapDescSize);
    for (u64 t = 0; t < memSize; t+=0x1000)
        gPTM.map_memory((void*)t, (void*)t);

    /* x86: Control Register 3 = Address of the page directory in physical form.
     *   A page directory is a multi-level page map; a four-level map is standard
     *     on x86_64 CPUs, so that's what I use.
     *   Modern CPUs have the ability to use a five-level map, but for now there
     *     is no absolutely no need; it would be easy to add this ability in the future.
     *
     *   The Transition Lookaside Buffer (Virtual Address -> Physical Address HashMap)
     *     is hardware based in x86; by writing to CR3, it is flushed (reset to empty).
     *   A single TLB entry may be invalidated using the `INVLPG <addr>` instruction.
     *
     *   Most processes share most memory; this means (parts of) the page table 
     *     may be shared until a thread tries to write to the memory. 
     *   This will cause a copy to occur, and that thread will now have it's
     *       own object, ensuring thread-safety.
     *   This technique is called Copy On Write (COW).
     */
    asm ("mov %0, %%cr3" : : "r" (PML4));
}

void prepare_interrupts() {
    // REMAP PIC CHIP IRQs OUT OF THE WAY OF GENERAL SOFTWARE EXCEPTIONS.
    remap_pic();
    // CREATE INTERRUPT DESCRIPTOR TABLE.
    gIDT = IDTR(0x0fff, (u64)gAlloc.request_page());
    // POPULATE TABLE.
    gIDT.install_handler((u64)irq0_handler,                     PIC_IRQ0);
    gIDT.install_handler((u64)keyboard_handler,                 PIC_IRQ1);
    gIDT.install_handler((u64)uart_com1_handler,                PIC_IRQ4);
    gIDT.install_handler((u64)rtc_periodic_handler,             PIC_IRQ8);
    gIDT.install_handler((u64)mouse_handler,                    PIC_IRQ12);
    gIDT.install_handler((u64)divide_by_zero_handler,           0x00);
    gIDT.install_handler((u64)double_fault_handler,             0x08);
    gIDT.install_handler((u64)stack_segment_fault_handler,      0x0c);
    gIDT.install_handler((u64)general_protection_fault_handler, 0x0d);
    gIDT.install_handler((u64)page_fault_handler,               0x0e);
    gIDT.install_handler((u64)system_call_handler_asm,          0x80, IDT_TA_UserInterruptGate);
    gIDT.flush();
}

void prepare_pci() {
    // Memory-mapped ConFiguration Table
    ACPI::MCFGHeader* mcfg = (ACPI::MCFGHeader*)ACPI::find_table("MCFG");
    if (mcfg) {
        srl->writestr("[kUtil]: Found Memory-mapped Configuration Space\r\n");
        PCI::enumerate_pci(mcfg);
        srl->writestr("[kUtil]: \033[32mPCI Prepared\033[0m\r\n");
    }
}

TSSEntry tss_entry;
// 'tss' USED IN 'src/userswitch.asm' AS EXTERNAL SYMBOL.
void* tss;

void kernel_init(BootInfo* bInfo) {
    // Disable interrupts (with no IDT, not much was happening anyway).
    asm ("cli");
    // Prepare Global Descriptor Table Descriptor.
    GDTDescriptor GDTD = GDTDescriptor(sizeof(GDT) - 1, (u64)&gGDT);
    LoadGDT(&GDTD);
    // Parse memory map passed by bootloader.
    prepare_memory(bInfo);
    // Setup dynamic memory allocation.
    init_heap((void*)0x700000000000, 1);
    // Setup serial input/output.
    srl = new UARTDriver;
    srl->writestr("\r\n!===--- You are now booting into \033[1;33mLensorOS\033[0m ---===!\r\n\r\n");
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
    // Create basic framebuffer renderer.
    srl->writestr("[kUtil]: Setting up Graphics Output Protocol Renderer\r\n");
    gRend = BasicRenderer(bInfo->framebuffer, bInfo->font);
    srl->writestr("    \033[32msetup successful\033[0m\r\n");
    gRend.puts("<<>><<<!===--- You are now booting into LensorOS ---===!>>><<>>");
    gRend.crlf();
    gRend.swap();
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
    srl->writestr("[kUtil]: \033[32mFilesystem drivers created successfully.\033[0m\r\n");
    // INITIALIZE ADVANCED CONFIGURATION AND POWER MANAGEMENT INTERFACE.
    ACPI::initialize(bInfo->rsdp);
    srl->writestr("[kUtil]: ACPI prepared.\r\n");
    // PCI ENUMERATION.
    prepare_pci();
    srl->writestr("[kUtil]: PCI prepared.\r\n");
    // INITIALIZE HIGH PRECISION EVENT TIMER.
    gHPET.initialize();
    // PREPARE PS/2 MOUSE.
    init_ps2_mouse();
    // SETUP TASK STATE SEGMENT ENTRY FOR EVENTUAL SWITCH TO USERLAND.
    memset((void*)&tss_entry, 0, sizeof(TSSEntry));
    u32 limit = sizeof(TSSEntry) - 1;
    u64 base = (u64)&tss_entry;
    gGDT.TSS.Entry.Limit0 = limit;
    u8 flags = gGDT.TSS.Entry.Limit1_Flags;
    gGDT.TSS.Entry.Limit1_Flags = limit >> 16;
    gGDT.TSS.Entry.Limit1_Flags |= flags;
    gGDT.TSS.Entry.Base0 = base;
    gGDT.TSS.Entry.Base1 = base >> 16;
    gGDT.TSS.Entry.Base2 = base >> 24;
    gGDT.TSS.Base3 = base >> 32;
    tss = (void*)&tss_entry;
    // BASIC KEYBOARD HANDLER
    Keyboard::gText = Keyboard::BasicTextRenderer();
    // SETUP RANDOM NUMBER GENERATOR(S)
    gRandomLCG = LCG();
    gRandomLFSR = LFSR();
    // USE KERNEL PROCESS SWITCHING
    Scheduler::Initialize(gPTM.PML4);
    /// INTERRUPT MASKS (IRQs).
    /// 0 = UNMASKED, ALLOWED TO HAPPEN
    /// System Timer, PS/2 Keyboard, Slave PIC enabled, UART
    out8(PIC1_DATA, 0b11101000);
    io_wait();
    /// Real time clock, PS/2 Mouse
    out8(PIC2_DATA, 0b11101110);
    io_wait();
    // ENABLE INTERRUPTS.
    srl->writestr("[kUtil]: Interrupt masks sent, enabling interrupts.\r\n");
    asm ("sti");
    srl->writestr("    \033[32mInterrupts enabled.\033[0m\r\n");
}
