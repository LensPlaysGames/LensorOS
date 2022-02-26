#include "kUtility.h"

#include "acpi.h"
#include "basic_renderer.h"
#include "bitmap.h"
#include "cpu.h"
#include "cpuid.h"
#include "cstr.h"
#include "efi_memory.h"
#include "fat_definitions.h"
#include "fat_driver.h"
#include "gdt.h"
#include "memory/heap.h"
#include "hpet.h"
#include "interrupts/idt.h"
#include "interrupts/interrupts.h"
#include "interrupts/syscalls.h"
#include "io.h"
#include "keyboard.h"
#include "memory.h"
#include "memory/physical_memory_manager.h"
#include "mouse.h"
#include "paging/paging.h"
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
    /* What this function does:
     * |- Prepare physical memory manager (Memory namespace).
     * |- Lock kernel memory so it doesn't get over-written within physical memory manager.
     * `- Create, initialize, and load a page map level four (x86 virtual memory structure).
     */
    // Setup memory state from EFI memory map.
    Memory::init_efi(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    // _KernelStart and _KernelEnd defined in linker script "../kernel.ld"
    u64 kernelPagesNeeded = (((u64)&KERNEL_END - (u64)&KERNEL_START) / 4096) + 1;
    Memory::lock_pages(&KERNEL_START, kernelPagesNeeded);
    // PAGE MAP LEVEL FOUR (see paging.h).
    PageTable* PML4 = (PageTable*)Memory::request_page();
    // PAGE TABLE MANAGER
    gPTM = PageTableManager(PML4);
    // Map all physical RAM addresses to virtual addresses 1:1, store them in the PML4.
    // This means that virtual addresses will be equal to physical addresses.
    for (u64 t = 0; t < Memory::get_total_ram(); t+=0x1000)
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
     */
    asm ("mov %0, %%cr3" : : "r" (PML4));
}

void prepare_interrupts() {
    // REMAP PIC CHIP IRQs OUT OF THE WAY OF GENERAL SOFTWARE EXCEPTIONS.
    remap_pic();
    // CREATE INTERRUPT DESCRIPTOR TABLE.
    gIDT = IDTR(0x0fff, (u64)Memory::request_page());
    // POPULATE TABLE.
    // NOTE: IRQ0 uses this handler by default, but scheduler over-rides this!
    gIDT.install_handler((u64)system_timer_handler,             PIC_IRQ0);
    gIDT.install_handler((u64)keyboard_handler,                 PIC_IRQ1);
    gIDT.install_handler((u64)uart_com1_handler,                PIC_IRQ4);
    gIDT.install_handler((u64)rtc_handler,                      PIC_IRQ8);
    gIDT.install_handler((u64)mouse_handler,                    PIC_IRQ12);
    gIDT.install_handler((u64)divide_by_zero_handler,           0x00);
    gIDT.install_handler((u64)double_fault_handler,             0x08);
    gIDT.install_handler((u64)stack_segment_fault_handler,      0x0c);
    gIDT.install_handler((u64)general_protection_fault_handler, 0x0d);
    gIDT.install_handler((u64)page_fault_handler,               0x0e);
    gIDT.install_handler((u64)simd_exception_handler,           0x13);
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

void draw_boot_gfx() {
    gRend.puts("<<>><<<!===--- You are now booting into LensorOS ---===!>>><<>>");
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
    gRend.swap();
}

/* FIXME: STUFF DECLARED HERE SHOULD DEFINITELY BE MOVED :^) */

// CPUDescription = common to all cores
// CPU = a single core
CPUDescription* SystemCPU { nullptr };

// FXSAVE/FXRSTOR instructions require a pointer to a
//   512-byte region of memory before use.
u8 fxsave_region[512] __attribute__((aligned(16)));

void kernel_init(BootInfo* bInfo) {
    /* 
     *   - Prepare physical/virtual memory
     *   - Load Global Descriptor Table
     *   - Load Interrupt Descriptor Table
     *   - Prepare the heap
     *   - Setup output to the user (serial driver, graphical renderers).
     *     - UARTDriver         -- serial output
     *     - BasicRenderer      -- drawing graphics
     *     - BasicTextRenderer  -- draw keyboard input on screen, keep track of text cursor, etc
     *   - Setup basic timers
     *     - Programmable Interval Timer (PIT)
     *     - Real Time Clock (RTC)
     *   - Determine and cache information about CPU(s)
     *   - Prepare device drivers
     *     - FATDriver  -- Filesystem driver
     *   - Initialize ACPI (find System Descriptor Table (XSDT))
     *   - Enumerate PCI devices
     *   - Prepare devices
     *     - High Precision Event Timer (HPET)
     *     - PS2 Mouse
     *   - Setup scheduler (TSS descriptor, task switching)
     */
    // Disable interrupts (with no IDT, not much was happening anyway).
    asm ("cli");
    // Parse memory map passed by bootloader.
    prepare_memory(bInfo);
    // Prepare Global Descriptor Table Descriptor.
    GDTDescriptor GDTD = GDTDescriptor(sizeof(GDT) - 1, (u64)&gGDT);
    LoadGDT(&GDTD);
    // Prepare Interrupt Descriptor Table.
    prepare_interrupts();
    // Setup dynamic memory allocation.
    init_heap((void*)0x700000000000, 1);
    // Setup random number generators.
    gRandomLCG = LCG();
    gRandomLFSR = LFSR();
    // Setup serial input/output.
    srl = new UARTDriver;
    //print_efi_memory_map(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    //print_efi_memory_map_summed(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    srl->writestr("\r\n!===--- You are now booting into \033[1;33mLensorOS\033[0m ---===!\r\n\r\n");
    Memory::print_debug();
    srl->writestr("[kUtil]: Mapped physical memory from 0x");
    srl->writestr(to_hexstring<u64>(0ULL));
    srl->writestr(" thru ");
    srl->writestr(to_hexstring<u64>(Memory::get_total_ram()));
    srl->writestr("\r\n[kUtil]:\r\n  Kernel loaded from 0x");
    srl->writestr(to_hexstring<void*>(&KERNEL_START));
    srl->writestr(" to 0x");
    srl->writestr(to_hexstring<void*>(&KERNEL_END));
    srl->writestr("\r\n    .text:   0x");
    srl->writestr(to_hexstring<void*>(&TEXT_START));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring<void*>(&TEXT_END));
    srl->writestr("\r\n    .data:   0x");
    srl->writestr(to_hexstring<void*>(&DATA_START));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring<void*>(&DATA_END));
    srl->writestr("\r\n    .rodata: 0x");
    srl->writestr(to_hexstring<void*>(&READ_ONLY_DATA_START));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring<void*>(&READ_ONLY_DATA_END));
    srl->writestr("\r\n    .bss:    0x");
    srl->writestr(to_hexstring<void*>(&BLOCK_STARTING_SYMBOLS_START));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring<void*>(&BLOCK_STARTING_SYMBOLS_END));
    srl->writestr("\r\n\r\n[kUtil]: Heap mapped to 0x");
    srl->writestr(to_hexstring<void*>(sHeapStart));
    srl->writestr(" thru 0x");
    srl->writestr(to_hexstring<void*>(sHeapEnd));
    srl->writestr("\r\n");
    // Create basic framebuffer renderer.
    srl->writestr("[kUtil]: Setting up Graphics Output Protocol Renderer\r\n");
    gRend = BasicRenderer(bInfo->framebuffer, bInfo->font);
    srl->writestr("    \033[32msetup successful\033[0m\r\n");
    draw_boot_gfx();
    // Create basic text renderer for the keyboard.
    Keyboard::gText = Keyboard::BasicTextRenderer();
    // Initialize the Programmable Interval Timer.
    gPIT = PIT();
    srl->writestr("[kUtil]: Programmable Interval Timer initialized.\r\n");
    srl->writestr("  Channel 0, H/L Bit Access\r\n");
    srl->writestr("  Rate Generator, BCD Disabled\r\n");
    srl->writestr("  Periodic interrupts at ");
    srl->writestr(to_string(PIT_FREQUENCY));
    srl->writestr("hz.\r\n");
    // Initialize the Real Time Clock.
    gRTC = RTC();
    gRTC.set_periodic_int_enabled(true);
    srl->writestr("[kUtil]: Real Time Clock initialized.\r\n");
    srl->writestr("  Periodic interrupts enabled at ");
    srl->writestr(to_string((double)RTC_PERIODIC_HERTZ));
    srl->writestr("hz\r\n");
    // Print real time to serial output.
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
    SystemCPU = new CPUDescription();
    // Check for CPUID availability ('ID' bit in rflags register modifiable)
    bool supportCPUID = static_cast<bool>(cpuid_support());
    if (supportCPUID) {
        srl->writestr("[kUtil]: CPUID is supported\r\n");
        SystemCPU->set_cpuid_capable();
        char* cpuVendorID = cpuid_string(0);
        srl->writestr("  CPU Vendor ID: ");
        srl->writestr(cpuVendorID);
        srl->writestr("\r\n");
        SystemCPU->set_vendor_id(cpuVendorID);
        /* Current functionality of giant `if` statemnt:
         * |- Setup FXSAVE/FXRSTOR
         * |  |- Setup FPU
         * |  `- Setup SSE
         * `- Setup XSAVE
         *    `- Setup AVX
         *
         * If a feature is present, it's feature flag is set in SystemCPU.
         *
         * To peek further down the rabbit hole, check out the following link:
         *   https://wiki.osdev.org/Detecting_CPU_Topology_(80x86)#Using_CPUID
         */

        CPUIDRegisters regs;
        cpuid(1, regs);

        // Enable FXSAVE/FXRSTOR instructions if CPU supports it.
        // If it is not supported, don't bother trying to support FPU, SSE, etc
        //   as there would be no mechanism to save/load the registers on context switch.
        // TODO: Get logical/physical core bits from CPUID
        // |- 0x0000000b -- Intel
        // |- 0x80000008 -- AMD
        // `- Otherwise: bits are zero, assume single core.
        // TODO: Rewrite task switching code to save/load all supported registers in CPUState.
        if (regs.D & static_cast<u32>(CPUID_FEATURE::EDX_FXSR)) {
            SystemCPU->set_fxsr_capable();
            asm volatile ("fxsave %0" :: "m"(fxsave_region));
            srl->writestr("  \033[32mFXSAVE/FXRSTOR Enabled\033[0m\r\n");
            SystemCPU->set_fxsr_enabled();
            // If FXSAVE/FXRSTOR is supported, setup FPU.
            if (regs.D & static_cast<u32>(CPUID_FEATURE::EDX_FPU)) {
                SystemCPU->set_fpu_capable();
                // FPU supported, ensure it is enabled.
                /* FPU Relevant Control Register Bits
                 * |- CR0.EM (bit 02) -- If set, FPU and vector operations will cause a #UD.
                 * `- CR0.TS (bit 03) -- Task switched. If set, all FPU and vector ops will cause a #NM.
                 */
                asm volatile ("mov %%cr0, %%rdx\n"
                              "mov $0b1100, %%ax\n"
                              "not %%ax\n"
                              "and %%ax, %%dx\n"
                              "mov %%rdx, %%cr0\n"
                              "fninit\n"
                              ::: "rax", "rdx");
                srl->writestr("  \033[32mFPU Enabled\033[0m\r\n");
                SystemCPU->set_fpu_enabled();
            }
            else {
                // FPU not supported, ensure it is disabled.
                asm volatile ("mov %%cr0, %%rdx\n"
                              "or $0b1100, %%dx\n"
                              "mov %%rdx, %%cr0\n"
                              ::: "rdx");
                srl->writestr("  \033[31mFPU Not Supported\033[0m\r\n");
            }
            // If FXSAVE/FXRSTOR and FPU are supported and present, setup SSE.
            if (regs.D & static_cast<u32>(CPUID_FEATURE::EDX_SSE)) {
                SystemCPU->set_sse_capable();
                /* Enable SSE
                 * |- Clear CR0.EM bit   (bit 2  -- coprocessor emulation) 
                 * |- Set CR0.MP bit     (bit 1  -- coprocessor monitoring)
                 * |- Set CR4.OSFXSR bit (bit 9  -- OS provides FXSAVE/FXRSTOR functionality)
                 * `- Set CR4.OSXMMEXCPT (bit 10 -- OS provides #XM exception handler)
                 */
                asm volatile ("mov %%cr0, %%rax\n"
                              "and $0b1111111111110011, %%ax\n"
                              "or $0b10, %%ax\n"
                              "mov %%rax, %%cr0\n"
                              "mov %%cr4, %%rax\n"
                              "or $0b11000000000, %%rax\n"
                              "mov %%rax, %%cr4\n"
                              ::: "rax");
                srl->writestr("  \033[32mSSE Enabled\033[0m\r\n");
                SystemCPU->set_sse_enabled();
            }
            else srl->writestr("  \033[31mSSE Not Supported\033[0m\r\n");
        }
        // Enable XSAVE feature set if CPU supports it.
        if (regs.C & static_cast<u32>(CPUID_FEATURE::ECX_XSAVE)) {
            SystemCPU->set_xsave_capable();
            // Enable XSAVE feature set
            // `- Set CR4.OSXSAVE bit (bit 18  -- OS provides )
            asm volatile ("mov %cr4, %rax\n"
                          "or 0b1000000000000000000, %rax\n"
                          "mov %rax, %cr4\n");
            srl->writestr("  \033[32mXSAVE Enabled\033[0m\r\n");
            SystemCPU->set_xsave_enabled();
            // If SSE, AND XSAVE are supported, setup AVX feature set.
            if (regs.D & static_cast<u32>(CPUID_FEATURE::EDX_SSE)
                && regs.C & static_cast<u32>(CPUID_FEATURE::ECX_AVX))
            {
                SystemCPU->set_avx_capable();
                asm volatile ("xor %%rcx, %%rcx\n"
                              "xgetbv\n"
                              "or $0b111, %%eax\n"
                              "xsetbv\n"
                              ::: "rax", "rbx", "rdx");
                srl->writestr("  \033[32mAVX Enabled\033[0m\r\n");
                SystemCPU->set_avx_enabled();
            }
            else srl->writestr("  \033[31mAVX Not Supported\033[0m\r\n");
        }
        else srl->writestr("  \033[31mXSAVE Not Supported\033[0m\r\n");
    }
    // TODO: Parse CPUs from ACPI MADT table. For now only support single core.
    CPU cpu = CPU(SystemCPU);
    SystemCPU->add_cpu(cpu);
    SystemCPU->print_debug();
    // Prepare filesystem drivers.
    gFATDriver = FATDriver();
    srl->writestr("[kUtil]: \033[32mFilesystem drivers prepared successfully\033[0m\r\n");
    // Initialize Advanced Configuration and Power Management Interface.
    ACPI::initialize(bInfo->rsdp);
    srl->writestr("[kUtil]: \033[32mACPI initialized\033[0m\r\n");
    // Enumerate PCI (find hardware devices).
    prepare_pci();
    // Initialize High Precision Event Timer.
    (void)gHPET.initialize();
    // Prepare PS2 mouse.
    init_ps2_mouse();
    // Print the state of the heap just before beginning multi-threading setup.
    heap_print_debug();
    // Setup task state segment for eventual switch to user-land.
    TSS::initialize();
    // Use kernel process switching.
    Scheduler::initialize(gPTM.PML4);
    // Enable IRQ interrupts that will be used.
    disable_all_interrupts();
    enable_interrupt(IRQ_SYSTEM_TIMER);
    enable_interrupt(IRQ_PS2_KEYBOARD);
    enable_interrupt(IRQ_CASCADED_PIC);
    enable_interrupt(IRQ_UART_COM1);
    enable_interrupt(IRQ_REAL_TIMER);
    enable_interrupt(IRQ_PS2_MOUSE);
    // Allow interrupts to trigger.
    srl->writestr("[kUtil]: Interrupt masks sent, enabling interrupts.\r\n");
    asm ("sti");
    srl->writestr("    \033[32mInterrupts enabled.\033[0m\r\n");
}
