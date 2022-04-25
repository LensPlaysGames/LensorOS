#include <kstage1.h>

#include <acpi.h>
#include <ahci.h>
#include <basic_renderer.h>
#include <boot.h>
#include <bitmap.h>
#include <cpu.h>
#include <cpuid.h>
#include <cstr.h>
#include <debug.h>
#include <efi_memory.h>
#include <fat_definitions.h>
#include <gdt.h>
#include <gpt.h>
#include <guid.h>
#include <memory/heap.h>
#include <hpet.h>
#include <interrupts/idt.h>
#include <interrupts/interrupts.h>
#include <interrupts/syscalls.h>
#include <io.h>
#include <keyboard.h>
#include <link_definitions.h>
#include <memory.h>
#include <memory/common.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <mouse.h>
#include <pci.h>
#include <pit.h>
#include <random_lcg.h>
#include <random_lfsr.h>
#include <rtc.h>
#include <scheduler.h>
#include <smart_pointer.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <storage/filesystem_drivers/file_allocation_table.h>
#include <storage/device_drivers/gpt_partition.h>
#include <system.h>
#include <tss.h>
#include <uart.h>

u8 idt_storage[0x1000];
void prepare_interrupts() {
    // REMAP PIC CHIP IRQs OUT OF THE WAY OF GENERAL SOFTWARE EXCEPTIONS.
    remap_pic();
    // CREATE INTERRUPT DESCRIPTOR TABLE.
    gIDT = IDTR(0x0fff, (u64)&idt_storage[0]);
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
    gIDT.install_handler((u64)system_call_handler_asm,          0x80
                         , IDT_TA_UserInterruptGate);
    gIDT.flush();
}

void draw_boot_gfx() {
    gRend.puts("<<<!===--- You are now booting into LensorOS ---===!>>>");
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

// FXSAVE/FXRSTOR instructions require a pointer to a
//   512-byte region of memory before use.
u8 fxsave_region[512] __attribute__((aligned(16)));

void kstage1(BootInfo* bInfo) {
    /* This function is monstrous, so the functionality is outlined here.
     *     - Disable interrupts (if they weren't already)
     *     - Ensure BootInfo pointer is valid (non-null)
     * x86 - Load Global Descriptor Table
     * x86 - Load Interrupt Descriptor Table
     *     - Prepare UART serial communications driver
     *     - Prepare physical/virtual memory
     *       - Initialize Physical Memory Manager
     *       - Initialize Virtual Memory Manager
     *       - Prepare the heap (`new` and `delete`)
     *     - Prepare Real Time Clock (RTC)
     *     - Setup graphical renderers  -- these will change, and soon
     *       - BasicRenderer      -- drawing pixels to linear framebuffer
     *       - BasicTextRenderer  -- draw keyboard input on screen,
     *                               keep track of text cursor, etc
     *     - Determine and cache information about CPU(s)
     *     - Initialize ACPI
     *     - Enumerate PCI
     *     - Prepare non-PCI devices
     *       - High Precision Event Timer (HPET)
     *       - PS2 Mouse
     *     - Prepare Programmable Interval Timer (PIT)
     * x86 - Setup TSS
     *     - Setup scheduler
     * x86 - Clear (IRQ) interrupt masks in PIC for used interrupts
     *     - Print information about the system to serial output
     *     - Enable interrupts
     *
     * x86 = The step is inherently x86-only (not implementation based).
     *
     * TODO:
     * `-- A lot of hardware is just assumed to be there;
     *     figure out how to better probe for their existence,
     *     and gracefully handle the case that they aren't there.
     */

    // Disable interrupts while doing sensitive
    //   operations (like setting up interrupts :^).
    asm ("cli");

    // Don't even attempt to boot unless boot info exists.
    if (bInfo == nullptr)
        while (true)
            asm ("hlt");

    /* Tell x86_64 CPU where the GDT is located by
     * populating and loading a GDT descriptor.
     * The global descriptor table contains information about
     * memory segments (like privilege level of executing code,
     * or privilege level needed to access data).
     */
    gGDTD.Size = sizeof(GDT) - 1;
    gGDTD.Offset = V2P((u64)&gGDT);
    LoadGDT((GDTDescriptor*)V2P(&gGDTD));

    // Prepare Interrupt Descriptor Table.
    prepare_interrupts();

    // Setup serial communications chip to allow for debug messages as soon as possible.
    UART::initialize();
    dbgmsg_s("\r\n"
             "!===--- You are now booting into \033[1;33mLensorOS\033[0m ---===!\r\n"
             "\r\n");

    // Setup physical memory allocator from EFI memory map.
    Memory::init_physical(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    // Setup virtual memory (map entire address space as well as kernel).
    Memory::init_virtual();
    // Setup dynamic memory allocation (`new`, `delete`).
    init_heap();

    SYSTEM = new System();

    // Initialize the Real Time Clock.
    gRTC = RTC();
    gRTC.set_periodic_int_enabled(true);
    dbgmsg("[kstage1]: \033[32mReal Time Clock (RTC) initialized\033[0m\r\n"
           "  Periodic interrupts enabled at \033[33m%fhz\033[0m\r\n"
           "\r\n\033[1;33m"
           "Now is %hhu:%hhu:%hhu on %ul-%hhu-%hhu"
           "\033[0m\r\n\r\n"
           , static_cast<double>(RTC_PERIODIC_HERTZ)
           , gRTC.Time.hour
           , gRTC.Time.minute
           , gRTC.Time.second
           , gRTC.Time.year
           , gRTC.Time.month
           , gRTC.Time.date);
    // Create basic framebuffer renderer.
    dbgmsg_s("[kstage1]: Setting up Graphics Output Protocol Renderer\r\n");
    gRend = BasicRenderer(bInfo->framebuffer, bInfo->font);
    dbgmsg_s("  \033[32mSetup Successful\033[0m\r\n\r\n");
    draw_boot_gfx();
    // Create basic text renderer for the keyboard.
    Keyboard::gText = Keyboard::BasicTextRenderer();

    // Setup random number generators.
    const RTCData& tm = gRTC.Time;
    u64 someNumber =
        tm.century + tm.year
        + tm.month   + tm.date
        + tm.weekday + tm.hour
        + tm.minute  + tm.second;
    gRandomLCG = LCG();
    gRandomLCG.seed(someNumber);
    gRandomLFSR = LFSR();
    gRandomLFSR.seed(gRandomLCG.get(), gRandomLCG.get());

    // Store feature set of CPU (capabilities).
    CPUDescription* SystemCPU = &SYSTEM->cpu();
    // Check for CPUID availability ('ID' bit in rflags register modifiable)
    bool supportCPUID = static_cast<bool>(cpuid_support());
    if (supportCPUID) {
        SystemCPU->set_cpuid_capable();
        dbgmsg_s("[kstage1]: \033[32mCPUID is supported\033[0m\r\n");
        char* cpuVendorID = cpuid_string(0);
        SystemCPU->set_vendor_id(cpuVendorID);
        dbgmsg_s("  CPU Vendor ID: ");
        dbgmsg((u8*)SystemCPU->get_vendor_id(), 12, ShouldNewline::Yes);

        CPUIDRegisters regs;
        cpuid(1, regs);

        /* TODO:
         * |-- Get logical/physical core bits from CPUID
         * |   |- 0x0000000b -- Intel
         * |   |- 0x80000008 -- AMD
         * |   `- Otherwise: bits are zero, assume single core.
         * `-- Rewrite task switching code to save/load
         *     all supported registers in CPUState.
         *
         * FIXME: My CPUID definition and implementation needs a lot of work.
         *        A system that doesn't use nested if statements would be great.
         *
         * Enable FXSAVE/FXRSTOR instructions if CPU supports it.
         * If it is not supported, don't bother trying to support
         * FPU, SSE, etc. as there would be no mechanism to
         * save/load the registers on context switch.
         *
         * Current functionality of giant `if` statemnt:
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
        if (regs.D & static_cast<u32>(CPUID_FEATURE::EDX_FXSR)) {
            SystemCPU->set_fxsr_capable();
            asm volatile ("fxsave %0" :: "m"(fxsave_region));
            SystemCPU->set_fxsr_enabled();
            // If FXSAVE/FXRSTOR is supported, setup FPU.
            if (regs.D & static_cast<u32>(CPUID_FEATURE::EDX_FPU)) {
                SystemCPU->set_fpu_capable();
                /* FPU supported, ensure it is enabled.
                 * FPU Relevant Control Register Bits
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
                SystemCPU->set_fpu_enabled();
            }
            else {
                // FPU not supported, ensure it is disabled.
                asm volatile ("mov %%cr0, %%rdx\n"
                              "or $0b1100, %%dx\n"
                              "mov %%rdx, %%cr0\n"
                              ::: "rdx");
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
                SystemCPU->set_sse_enabled();
            }
        }
        // Enable XSAVE feature set if CPU supports it.
        if (regs.C & static_cast<u32>(CPUID_FEATURE::ECX_XSAVE)) {
            SystemCPU->set_xsave_capable();
            // Enable XSAVE feature set
            // `- Set CR4.OSXSAVE bit (bit 18  -- OS provides )
            asm volatile ("mov %cr4, %rax\n"
                          "or $0b1000000000000000000, %rax\n"
                          "mov %rax, %cr4\n");
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
                SystemCPU->set_avx_enabled();
            }
        }
    }
    dbgmsg_s("\r\n");

    // TODO: Parse CPUs from ACPI MADT table.
    //       For now we only support single core.
    CPU cpu = CPU(SystemCPU);
    SystemCPU->add_cpu(cpu);
    SystemCPU->print_debug();

    // Initialize Advanced Configuration and Power Management Interface.
    ACPI::initialize(bInfo->rsdp);

    // Find Memory-mapped ConFiguration Table in order to find PCI devices.
    // Storage devices like AHCIs will be detected here.
    auto* mcfg = (ACPI::MCFGHeader*)ACPI::find_table("MCFG");
    if (mcfg) {
        dbgmsg("[kstage1]: Found Memory-mapped Configuration Space (MCFG) ACPI Table\r\n"
               "  Address: 0x%x\r\n"
               "\r\n"
               , mcfg);
        PCI::enumerate_pci(mcfg);
    }

    /* Probe storage devices
     *
     * Most storage devices handle multiple
     * storage media hardware devices; for example,
     * a single AHCI controller has multiple ports,
     * each one referring to its own device.
     *
     * TODO: Write more efficient container types.
     * FIXME: Don't use singly linked lists for everything.
     */
    SYSTEM->devices().for_each([](auto* it){
        SystemDevice& dev = it->value();
        if (dev.major() == SYSDEV_MAJOR_STORAGE
            && dev.minor() == SYSDEV_MINOR_AHCI_CONTROLLER
            && dev.flag(SYSDEV_MAJOR_STORAGE_SEARCH) != 0)
        {
            dbgmsg_s("[kstage1]: Probing AHCI Controller\r\n");
            AHCI::HBAMemory* ABAR = (AHCI::HBAMemory*)(u64)(((PCI::PCIHeader0*)dev.data2())->BAR5);
            // TODO: Better MMIO!! It should be separate from regular virtual mappings, I think.
            Memory::map(ABAR, ABAR);
            u32 ports = ABAR->PortsImplemented;
            for (u64 i = 0; i < 32; ++i) {
                if (ports & (1u << i)) {
                    AHCI::HBAPort* port = &ABAR->Ports[i];
                    AHCI::PortType type = get_port_type(port);
                    if (type != AHCI::PortType::None) {
                        auto* PortController = new AHCI::PortController(type, i, port);
                        SystemDevice ahciPort(SYSDEV_MAJOR_STORAGE
                                              , SYSDEV_MINOR_AHCI_PORT
                                              , PortController, &dev
                                              , nullptr, nullptr);
                        // Search SATA devices further for partitions and filesystems.
                        if (type == AHCI::PortType::SATA)
                            ahciPort.set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, true);

                        SYSTEM->add_device(ahciPort);
                    }
                }
            }
            // Don't search AHCI controller any further, already found all ports.
            dev.set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, false);
        }
    });

    /* Find partitions
     * A storage device may be partitioned (i.e. GUID Partition Table).
     * These partitions are to be detected and new system devices created.
     */
    SYSTEM->devices().for_each([](auto* it) {
        SystemDevice& d = it->value();
        if (d.major() == SYSDEV_MAJOR_STORAGE
            && d.minor() == SYSDEV_MINOR_AHCI_PORT
            && d.flag(SYSDEV_MAJOR_STORAGE_SEARCH) != 0)
        {
            auto* portCon = static_cast<AHCI::PortController*>((&d)->data1());
            auto* driver = static_cast<StorageDeviceDriver*>(portCon);
            dbgmsg("[kstage1]: Searching AHCI port %ull for a GPT\r\n"
                   , portCon->port_number());
            if(GPT::is_gpt_present(driver)) {
                dbgmsg("  GPT is present!\r\n");
                auto gptHeader = SmartPtr<GPT::Header>(new GPT::Header);
                auto sector = SmartPtr<u8[]>(new u8[512], 512);
                portCon->read(512, sizeof(GPT::Header), (u8*)gptHeader.get());
                for (u32 i = 0; i < gptHeader->NumberOfPartitionsTableEntries; ++i) {
                    u64 byteOffset = gptHeader->PartitionsTableEntrySize * i;
                    u32 partSector = gptHeader->PartitionsTableLBA + (byteOffset / 512);
                    byteOffset %= 512;
                    portCon->read(partSector * 512, 512, sector.get());
                    auto* part = (GPT::PartitionEntry*)((u64)sector.get() + byteOffset);
                    if (part->should_ignore())
                        continue;

                    if (part->TypeGUID == GPT::NullGUID)
                        continue;

                    if (part->EndLBA < part->StartLBA)
                        continue;

                    dbgmsg("      Partition %ul: ", i);
                    dbgmsg(part->Name, 72);
                    dbgmsg_s(":\r\n"
                             "        Type GUID: ");
                    print_guid(part->TypeGUID);
                    dbgmsg_s("\r\n"
                             "        Unique GUID: ");
                    print_guid(part->UniqueGUID);
                    dbgmsg("\r\n"
                           "        Sector Offset: %ull\r\n"
                           "        Sector Count: %ull\r\n"
                           "        Attributes: %ull\r\n"
                           , part->StartLBA
                           , part->size_in_sectors()
                           , part->Attributes);
                    auto* partDriver = new GPTPartitionDriver(driver, part->TypeGUID
                                                              , part->UniqueGUID
                                                              , part->StartLBA, 512);
                    SystemDevice gptPartition(SYSDEV_MAJOR_STORAGE
                                              , SYSDEV_MINOR_GPT_PARTITION
                                              , partDriver, nullptr
                                              , nullptr, &it->value());
                    gptPartition.set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, true);
                    // Don't touch partitions with ANY known GUIDs (for now).
                    const GUID* knownGUID = &GPT::ReservedPartitionGUIDs[0];
                    while (*knownGUID != GPT::NullGUID) {
                        if (part->TypeGUID == *knownGUID)
                            gptPartition.set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, false);

                        knownGUID++;
                    }
                    if (gptPartition.flag(SYSDEV_MAJOR_STORAGE_SEARCH))
                        SYSTEM->add_device(gptPartition);
                    else delete partDriver;
                }
                /* Don't search port any further, we figured
                 * out it's storage media that is GPT partitioned
                 * and devices have been created for those
                 * (that will themselves be searched for filesystems).
                 */
                d.set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, false);
            }
        }
    });

    // Prepare filesystem drivers.
    auto* FAT = new FileAllocationTableDriver;

    /* Detect filesystems
     * For every storage device we know how to read/write from,
     * check if a recognized filesystem resides on it.
     */
    SYSTEM->devices().for_each([FAT](auto* it) {
        SystemDevice& dev = it->value();
        if (dev.major() == SYSDEV_MAJOR_STORAGE
            && dev.flag(SYSDEV_MAJOR_STORAGE_SEARCH) != 0)
        {
            if (dev.minor() == SYSDEV_MINOR_GPT_PARTITION) {
                auto* partDriver = static_cast<GPTPartitionDriver*>(dev.data1());
                if (partDriver) {
                    dbgmsg_s("[kstage1]: GPT Partition:\r\n"
                             "  Type GUID: ");
                    print_guid(partDriver->type_guid());
                    dbgmsg_s("\r\n"
                             "  Unique GUID: ");
                    print_guid(partDriver->unique_guid());
                    dbgmsg_s("\r\n");
                    if (FAT->test(partDriver)) {
                        dbgmsg_s("  Found valid File Allocation Table filesystem\r\n");
                        SYSTEM->add_fs(Filesystem(FilesystemType::FAT, FAT, partDriver));
                        // Done searching GPT partition, found valid filesystem.
                        dev.set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, false);
                    }
                }
            }
            else if (dev.minor() == SYSDEV_MINOR_AHCI_PORT) {
                auto* portController = (AHCI::PortController*)dev.data1();
                if (portController) {
                    dbgmsg("[kstage1]: AHCI port %ull:\r\n"
                           , portController->port_number());
                    dbgmsg_s("  Checking for valid File Allocation Table filesystem\r\n");
                    if (FAT->test(portController)) {
                        dbgmsg("  Found valid File Allocation Table filesystem\r\n");
                        SYSTEM->add_fs(Filesystem(FilesystemType::FAT, FAT, portController));
                        // Done searching AHCI port, found valid filesystem.
                        dev.set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, false);
                    }
                }
            }
        }
    });

    SYSTEM->virtual_filesystem().print_debug();

    u64 i = 0;
    SYSTEM->filesystems().for_each([&i](auto* it) {
        Filesystem& fs = it->value();
        String name("/fs");
        name += to_string(i);
        SYSTEM->virtual_filesystem().mount(name.data_copy(), &fs);
        i++;
    });

    if (SYSTEM->filesystems().length() > 0) {
        const char* filePath = "/fs0";
        dbgmsg("Opening %s with VFS\r\n", filePath);
        FileDescriptor fd = SYSTEM->virtual_filesystem().open("/fs0", 0, 0);
        dbgmsg("  Got FileDescriptor %ull\r\n", fd);
        SYSTEM->virtual_filesystem().print_debug();
        dbgmsg("Closing FileDescriptor %ull\r\n", fd);
        SYSTEM->virtual_filesystem().close(fd);
        dbgmsg("FileDescriptor %ull closed\r\n", fd);
        SYSTEM->virtual_filesystem().print_debug();
    }
    
    // Initialize High Precision Event Timer.
    (void)gHPET.initialize();
    // Prepare PS2 mouse.
    init_ps2_mouse();

    // Initialize the Programmable Interval Timer.
    gPIT = PIT();
    dbgmsg("[kstage1]: \033[32mProgrammable Interval Timer Initialized\033[0m\r\n"
           "  Channel 0, H/L Bit Access\r\n"
           "  Rate Generator, BCD Disabled\r\n"
           "  Periodic interrupts at \033[33m%fhz\033[0m.\r\n"
           "\r\n"
           , static_cast<double>(PIT_FREQUENCY));

    // The Task State Segment in x86_64 is used only
    // for switches between privilege levels.
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

    //Memory::print_efi_memory_map(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    //Memory::print_efi_memory_map_summed(bInfo->map, bInfo->mapSize, bInfo->mapDescSize);
    heap_print_debug();
    Memory::print_debug();

    SYSTEM->print();

    // Allow interrupts to trigger.
    dbgmsg_s("[kstage1]: Enabling interrupts\r\n");
    asm ("sti");
    dbgmsg_s("[kstage1]: \033[32mInterrupts enabled\033[0m\r\n");
}
