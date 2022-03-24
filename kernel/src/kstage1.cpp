#include "kstage1.h"

#include "acpi.h"
#include "basic_renderer.h"
#include "boot.h"
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
#include "memory/virtual_memory_manager.h"
#include "mouse.h"
#include "pci.h"
#include "pit.h"
#include "random_lcg.h"
#include "random_lfsr.h"
#include "rtc.h"
#include "scheduler.h"
#include "tss.h"
#include "uart.h"

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

void kstage1(BootInfo* bInfo) {
    /* This function is kind of monstrous, so the functionality is outlined here.
     *   - Load Global Descriptor Table (CPU Privilege levels, hardware task switching)
     *   - Prepare UART serial communications driver
     *   - Prepare physical/virtual memory
     *     - Initialize Physical Memory Manager (chicken/egg happens here)
     *     - Initialize Virtual Memory Manager (ensure all RAM is mapped, as well as kernel)
     *     - Prepare the heap (`new` and `delete`)
     *   - Load Interrupt Descriptor Table (Install handlers for hardware IRQs + software exceptions)
     *   - Prepare Real Time Clock (RTC)
     *   - Setup graphical renderers
     *     - BasicRenderer      -- drawing pixels to linear framebuffer
     *     - BasicTextRenderer  -- draw keyboard input on screen, keep track of text cursor, etc
     *   - Determine and cache information about CPU(s)
     *   - Prepare device drivers
     *     - FATDriver  -- Filesystem driver
     *   - Initialize ACPI (find System Descriptor Table (XSDT))
     *   - Prepare PCI devices (enumerate PCI bus and initialize recognized devices)
     *   - Prepare non-PCI devices
     *     - High Precision Event Timer (HPET)
     *     - PS2 Mouse
     *   - Prepare Programmable Interval Timer (PIT)
     *   - Setup scheduler (TSS descriptor, task switching)
     *   - Clear (IRQ) interrupt masks in PIC for used interrupts
     *   - Print information about the system after boot initialization to serial out
     */

    // Disable interrupts while doing sensitive
    //   operations (like setting up interrupts :^).
    asm ("cli");

    /* Tell x86_64 CPU where the GDT is located by populating and loading a GDT descriptor.
     * The global descriptor table contains information about
     *   memory segments (like privilege level of executing code,
     *   or privilege level needed to access data).
     */
    gGDTD.Size = sizeof(GDT) - 1;
    gGDTD.Offset = (u64)&gGDT;
    LoadGDT(&gGDTD);

    // Setup serial communications chip to allow for debug messages as soon as possible.
    UART::initialize();
    UART::out("\r\n!===--- You are now booting into \033[1;33mLensorOS\033[0m ---===!\r\n\r\n");

    // Setup physical memory allocator from EFI memory map.
    Memory::init_physical(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    // Setup virtual memory (map entire address space as well as kernel).
    Memory::init_virtual(Memory::get_active_page_map());
    // Setup dynamic memory allocation (`new`, `delete`).
    init_heap();

    // Prepare Interrupt Descriptor Table.
    prepare_interrupts();

    // Initialize the Real Time Clock.
    gRTC = RTC();
    gRTC.set_periodic_int_enabled(true);
    UART::out("[kUtil]: \033[32mReal Time Clock Initialized\033[0m\r\n");
    UART::out("  Periodic interrupts enabled at \033[33m");
    UART::out(to_string((double)RTC_PERIODIC_HERTZ));
    UART::out("hz\033[0m\r\n  \033[1;33mNow is ");
    UART::out(to_string(gRTC.Time.hour));
    UART::outc(':');
    UART::out(to_string(gRTC.Time.minute));
    UART::outc(':');
    UART::out(to_string(gRTC.Time.second));
    UART::out(" on ");
    UART::out(to_string(gRTC.Time.year));
    UART::outc('-');
    UART::out(to_string(gRTC.Time.month));
    UART::outc('-');
    UART::out(to_string(gRTC.Time.date));
    UART::out("\033[0m\r\n\r\n");

    // Create basic framebuffer renderer.
    UART::out("[kUtil]: Setting up Graphics Output Protocol Renderer\r\n");
    gRend = BasicRenderer(bInfo->framebuffer, bInfo->font);
    UART::out("  \033[32mSetup Successful\033[0m\r\n\r\n");
    draw_boot_gfx();
    // Create basic text renderer for the keyboard.
    Keyboard::gText = Keyboard::BasicTextRenderer();

    // Setup random number generators.
    gRandomLCG = LCG();
    u64 someNumber = gRTC.Time.century + gRTC.Time.year
        + gRTC.Time.month + gRTC.Time.date
        + gRTC.Time.weekday + gRTC.Time.hour
        + gRTC.Time.minute + gRTC.Time.second;
    gRandomLCG.seed(someNumber);
    gRandomLFSR = LFSR();
    gRandomLFSR.seed(gRandomLCG.get(), gRandomLCG.get());

    // Store feature set of CPU (capabilities).
    // TODO: Don't store the global system CPU descriptor on the stack, there only ever needs to be one.
    SystemCPU = new CPUDescription();
    // Check for CPUID availability ('ID' bit in rflags register modifiable)
    bool supportCPUID = static_cast<bool>(cpuid_support());
    if (supportCPUID) {
        SystemCPU->set_cpuid_capable();
        UART::out("[kUtil]: \033[32mCPUID is supported\033[0m\r\n");
        char* cpuVendorID = cpuid_string(0);
        SystemCPU->set_vendor_id(cpuVendorID);
        UART::out("  CPU Vendor ID: ");
        UART::out((u8*)SystemCPU->get_vendor_id(), 12);
        UART::out("\r\n");
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
            UART::out("  \033[32mFXSAVE/FXRSTOR Enabled\033[0m\r\n");
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
                UART::out("  \033[32mFPU Enabled\033[0m\r\n");
                SystemCPU->set_fpu_enabled();
            }
            else {
                // FPU not supported, ensure it is disabled.
                asm volatile ("mov %%cr0, %%rdx\n"
                              "or $0b1100, %%dx\n"
                              "mov %%rdx, %%cr0\n"
                              ::: "rdx");
                UART::out("  \033[31mFPU Not Supported\033[0m\r\n");
            }
            // If FXSAVE/FXRSTOR are supported and present, setup SSE.
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
                UART::out("  \033[32mSSE Enabled\033[0m\r\n");
                SystemCPU->set_sse_enabled();
            }
            else UART::out("  \033[31mSSE Not Supported\033[0m\r\n");
        }
        // Enable XSAVE feature set if CPU supports it.
        if (regs.C & static_cast<u32>(CPUID_FEATURE::ECX_XSAVE)) {
            SystemCPU->set_xsave_capable();
            // Enable XSAVE feature set
            // `- Set CR4.OSXSAVE bit (bit 18  -- OS provides )
            asm volatile ("mov %cr4, %rax\n"
                          "or $0b1000000000000000000, %rax\n"
                          "mov %rax, %cr4\n");
            UART::out("  \033[32mXSAVE Enabled\033[0m\r\n");
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
                              ::: "rax", "rcx", "rdx");
                UART::out("  \033[32mAVX Enabled\033[0m\r\n");
                SystemCPU->set_avx_enabled();
            }
            else UART::out("  \033[31mAVX Not Supported\033[0m\r\n");
        }
        else UART::out("  \033[31mXSAVE Not Supported\033[0m\r\n");
        UART::out("\r\n");
    }
    
    // TODO: Parse CPUs from ACPI MADT table. For now only support single core.
    CPU cpu = CPU(SystemCPU);
    SystemCPU->add_cpu(cpu);
    SystemCPU->print_debug();
    
    // Prepare filesystem drivers.
    gFATDriver = FATDriver();
    UART::out("[kUtil]: \033[32mFilesystem drivers prepared successfully\033[0m\r\n");
    // Initialize Advanced Configuration and Power Management Interface.
    ACPI::initialize(bInfo->rsdp);
    UART::out("[kUtil]: \033[32mACPI initialized\033[0m\r\n");

    // Find Memory-mapped ConFiguration Table in order to find PCI devices.
    ACPI::MCFGHeader* mcfg = (ACPI::MCFGHeader*)ACPI::find_table("MCFG");
    if (mcfg) {
        UART::out("[kUtil]: Found Memory-mapped Configuration Space\r\n");
        PCI::enumerate_pci(mcfg);
        UART::out("[kUtil]: \033[32mPCI Prepared\033[0m\r\n");
    }
    
    // Initialize High Precision Event Timer.
    (void)gHPET.initialize();
    // Prepare PS2 mouse.
    init_ps2_mouse();
    
    // Initialize the Programmable Interval Timer.
    gPIT = PIT();
    UART::out("[kUtil]: \033[32mProgrammable Interval Timer Initialized\033[0m\r\n");
    UART::out("  Channel 0, H/L Bit Access\r\n");
    UART::out("  Rate Generator, BCD Disabled\r\n");
    UART::out("  Periodic interrupts at \033[33m");
    UART::out(to_string(PIT_FREQUENCY));
    UART::out("hz\033[0m.\r\n\r\n");

    // The Task State Segment in x86_64 is used only for switches between privilege levels.
    TSS::initialize();
    Scheduler::initialize();
    
    // Enable IRQ interrupts that will be used.
    disable_all_interrupts();
    enable_interrupt(IRQ_SYSTEM_TIMER);
    enable_interrupt(IRQ_PS2_KEYBOARD);
    enable_interrupt(IRQ_CASCADED_PIC);
    enable_interrupt(IRQ_UART_COM1);
    enable_interrupt(IRQ_REAL_TIMER);
    enable_interrupt(IRQ_PS2_MOUSE);

    //print_efi_memory_map(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    //print_efi_memory_map_summed(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    heap_print_debug();
    Memory::print_debug();

    // Allow interrupts to trigger.
    UART::out("[kUtil]: Enabling interrupts\r\n");
    asm ("sti");
    UART::out("[kUtil]: \033[32mInterrupts enabled\033[0m\r\n");
}
