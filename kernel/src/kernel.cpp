#include "kUtility.h"

#include "basic_renderer.h"
#include "cstr.h"
#include "interrupts/interrupts.h"
#include "keyboard.h"
#include "paging/page_frame_allocator.h"
#include "pit.h"
#include "rtc.h"
#include "tss.h"
#include "uart.h"

/* TODO:
 *   |- 1.) Kernel Process Scheduler
 *   |      `- Resources
 *   |         |- https://wiki.osdev.org/User:Mariuszp/Scheduler_Tutorial
 *   |         |- https://wiki.osdev.org/Brendan%27s_Multi-tasking_Tutorial
 *   |         `- https://wiki.osdev.org/Scheduling_Algorithms
 *   |
 *   |- Userland: How does the desktop happen?
 *   |  |- I presume that the memory for the framebuffer must be mapped in a userland process.
 *   |  |  `- This process must be supplied by the OS, of course.
 *   |  |     `- Something like ScreenManager sounds sufficient (Compositor?). 
 *   |  |        |- If we 'pass' the framebuffer to this in-house userland program,
 *   |  |        |    it would have full read/write control over the screen of the OS.
 *   |  |        |- To avoid constantly updating the screen within the kernel,
 *   |  |        |  | the WindowManager will accept signals from applications
 *   |  |        |  | that signify the window is dirty and need to be re-painted.
 *   |  |        |  `- This will call the ScreenManager to update the framebuffer memory.
 *   |  |        `- The ScreenManager will also draw OS-wide things (taskbar? clock? etc.)
 *   |  |- To actually write userland programs that can be used,
 *   |  |  | I need to figure out GCC cross compiler sysroot stuff.
 *   |  |  `- This will allow me to write userland programs that 
 *   |  |       will be installed into the OS's root directory.
 *   |  `- I'll also need a Scheduler to hijack the IRQ0, 
 *   |     | or use a hardware-abstracted timer.
 *   |     `- This pre-emptive software task switcher will allow the kernel
 *   |          to stop programs from hogging the CPU, and also allow multiple
 *   |          programs to be run at the same time :*)
 *   |
 *   |- Further modify cross compiler to make an OS specific toolchain.
 *   |  `- This would allow for customizing default include,
 *   |       and default library (aka our own libc/libk).
 *   |
 *   |- Abstract functionality from hardware where possible (currently x86_64 only).
 *   |  |- Memory Allocation (./paging/)
 *   |  |- GDT/IDT
 *   |  |- Serial Communications (UART)
 *   |
 *   |- Create Smart Pointer Class(es).
 *   |- Create Container Class(es) -- Vector, LinkedList, etc.
 *   |- Create `integers_forward.h` for `integers.h`; replace occurences.
 *   |- Make kernel less architecture specific (it's very x86_64 specific).
 *   |
 *   |- Write a bootloader in C (no longer rely on GNU-EFI bootloader).
 *   |  `- I realize this is an insanely large project,
 *   |       but so is making an OS, I guess.
 *   |
 *   |- Make read-only section of kernel read only within 
 *   |    Page Map (PML4) using Page Table Manager (PTM).
 *   |
 *   |- Think about how Task State Segment Interrupt Stack 
 *   |    Table (TSS IST) could be used.
 *   |
 *   |- Contemplate swapping Memory Management Unit (MMU) 
 *   |    Page Map (PML4) when switching to userland, and/or 
 *   |    utilizing Translation Lookaside Buffer (TLB) partial flushes.
 *   |
 *   |- Abstract `timer` class (namespace?) -> an API for things like `sleep`
 *   |- Read more of this: https://pages.cs.wisc.edu/~remzi/OSTEP/
 *   |- Save parsed PCI devices for quick lookup (device tree).
 *   |- A slab-style memory allocator.
 *   |- File System:
 *   |  |- Virtual File System that will store intermediate representation
 *   |  |    of files/folders/worlds/storage media devices.
 *   |  |- AHCI Driver Update: DMA ATA Write implementation.
 *   |  `- Another filesystem better suited for mass storage (Ext2? Proprietary?)
 *   |
 *   |- Write ASM interrupt wrapper (no longer rely on `__attribute__((interrupt))`)
 *   |  `- See: 
 *   |     |- James Molloy's tutorials for an example: 
 *   |     |    http://www.jamesmolloy.co.uk/tutorial_html/
 *   |     `- The syscall handler in NASM assembly (`src/interrupts/syscalls.asm`).
 *   |
 *   |- Implement actually useful system calls
 *   |  |- Figure out how to pass variables to system calls (it's kind of just up to me).
 *   |  `- Useful list of 'things every OS needs': https://www.gnu.org/software/coreutils/
 *   |
 *   `- Add GPLv3 license header to top of every source file (exactly as seen in LICENSE).
 */
void print_memory_info() {
    u32 startOffset = gRend.DrawPos.x;
    gRend.puts("Memory Info:");
    gRend.crlf(startOffset);
    gRend.puts("|\\");
    gRend.crlf(startOffset);
    gRend.puts("| Free RAM: ");
    gRend.puts(to_string(gAlloc.get_free_ram() / 1024));
    gRend.puts(" KiB (");
    gRend.puts(to_string(gAlloc.get_free_ram() / 1024 / 1024));
    gRend.puts(" MiB)");
    gRend.crlf(startOffset);
    gRend.puts("|\\");
    gRend.crlf(startOffset);
    gRend.puts("| Used RAM: ");
    gRend.puts(to_string(gAlloc.get_used_ram() / 1024));
    gRend.puts(" KiB (");
    gRend.puts(to_string(gAlloc.get_used_ram() / 1024 / 1024));
    gRend.puts(" MiB)");
    gRend.crlf(startOffset);
    gRend.puts(" \\");
    gRend.crlf(startOffset);
    gRend.puts("  Reserved RAM: ");
    gRend.puts(to_string(gAlloc.get_reserved_ram() / 1024));
    gRend.puts(" KiB (");
    gRend.puts(to_string(gAlloc.get_reserved_ram() / 1024 / 1024));
    gRend.puts(" MiB)");
    gRend.crlf(startOffset);
}

void print_now(u64 xOffset = 0) {
    gRend.puts("Now is ");
    gRend.puts(to_string(gRTC.Time.hour));
    gRend.putchar(':');
    gRend.puts(to_string(gRTC.Time.minute));
    gRend.putchar(':');
    gRend.puts(to_string(gRTC.Time.second));
    gRend.puts(" on ");
    gRend.puts(to_string(gRTC.Time.year));
    gRend.putchar('-');
    gRend.puts(to_string(gRTC.Time.month));
    gRend.putchar('-');
    gRend.puts(to_string(gRTC.Time.date));
    gRend.crlf(xOffset);
}

void srl_memory_info() {
    srl->writestr("\r\n");
    srl->writestr("Memory Info:");
    srl->writestr("\r\n");
    srl->writestr("|\\");
    srl->writestr("\r\n");
    srl->writestr("| Free RAM: ");
    srl->writestr(to_string(gAlloc.get_free_ram() / 1024));
    srl->writestr(" KiB (");
    srl->writestr(to_string(gAlloc.get_free_ram() / 1024 / 1024));
    srl->writestr(" MiB)");
    srl->writestr("\r\n");
    srl->writestr("|\\");
    srl->writestr("\r\n");
    srl->writestr("| Used RAM: ");
    srl->writestr(to_string(gAlloc.get_used_ram() / 1024));
    srl->writestr(" KiB (");
    srl->writestr(to_string(gAlloc.get_used_ram() / 1024 / 1024));
    srl->writestr(" MiB)");
    srl->writestr("\r\n");
    srl->writestr(" \\");
    srl->writestr("\r\n");
    srl->writestr("  Reserved RAM: ");
    srl->writestr(to_string(gAlloc.get_reserved_ram() / 1024));
    srl->writestr(" KiB (");
    srl->writestr(to_string(gAlloc.get_reserved_ram() / 1024 / 1024));
    srl->writestr(" MiB)");
    srl->writestr("\r\n");
}

void test_userland_function() {
    for (;;) {
        asm volatile ("mov $0, %rax\r\n\t"
                      "int $0x80\r\n\t");
    }
}

// 'userland_function' USED IN 'userswitch.asm' AS EXTERNAL SYMBOL.
void* userland_function;

extern "C" void _start(BootInfo* bInfo) {
    // The heavy lifting is done within the `kernel_init` function (found in `kUtility.cpp`).
    kernel_init(bInfo);
    srl->writestr("\r\n\033[30;47m!===--- You have now booted into LensorOS ---===!\033[0m\r\n");
    // Clear + swap screen (ensure known state: blank).
    gRend.clear(0x00000000);
    gRend.swap();
    /// GPLv3 LICENSE REQUIREMENT (interactive terminal must print copyright notice).
    const char* GPLv3 = "<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>";
    // TO SERIAL
    srl->writestr(GPLv3);
    srl->writestr("\r\n\r\n");
    // TO SCREEN
    gRend.BackgroundColor = 0xffffffff;
    gRend.puts(GPLv3, 0x00000000);
    gRend.BackgroundColor = 0x00000000;
    gRend.crlf();
    gRend.swap({0, 0}, {80000, gRend.Font->PSF1_Header->CharacterSize});
    /// END GPLv3 LICENSE REQUIREMENT.

    // USERLAND SWITCH TESTING
    //userland_function = (void*)test_userland_function;
    //jump_to_userland_function();

    // Start keyboard input at draw position, not origin.
    Keyboard::gText.set_cursor_from_pixel_position(gRend.DrawPos);
    u32 debugInfoX = gRend.Target->PixelWidth - 300;
    while (true) {
        gRend.DrawPos = {debugInfoX, 0};
        // PRINT REAL TIME
        gRTC.update_data();
        print_now(debugInfoX);
        gRend.crlf(debugInfoX);
        // PRINT PIT ELAPSED TIME.
        gRend.puts("PIT Elapsed: ");
        gRend.puts(to_string(gPIT.seconds_since_boot()));
        gRend.crlf(debugInfoX);
        // PRINT RTC ELAPSED TIME.
        gRend.puts("RTC Elapsed: ");
        gRend.puts(to_string(gRTC.seconds_since_boot()));
        gRend.crlf(debugInfoX);
        // PRINT MEMORY INFO.
        gRend.crlf(debugInfoX);
        print_memory_info();
        // UPDATE TOP RIGHT CORNER OF SCREEN.
        gRend.swap({debugInfoX, 0}, {80000, 400});
    }
    
    // HALT LOOP (KERNEL INACTIVE).
    while (true) {
        asm ("hlt");
    }
}
