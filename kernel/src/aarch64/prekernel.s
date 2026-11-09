// Copyright 2022, Contributors To LensorOS.
// All rights reserved.
//
// This file is part of LensorOS.
//
// LensorOS is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// LensorOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with LensorOS. If not, see <https://www.gnu.org/licenses/>

.extern map_kernel_identity
.extern map_kernel_flexible
.extern clean_dcache_range

.section .bss
.align 12
kernel_stack_bottom:
    ;;# 64 KiB stack
    .zero 0x10000
kernel_stack_top:

table_pool:
    .zero (4096 * 22)
id_l0:
    .zero 4096
k_l0:
    .zero 4096
table_pool_end:

.section .text
.align 11
exception_vectors:
    // 0x000: Synchronous from current EL with SP0
    .align 7

    // QEMU:  0x09000000
    // MESON: 0xc81004c0
//     movz x14, #0x0900, lsl #16
//     movz x14, #0x04c0
//     movk x14, #0xc810, lsl #16
// 1:
//     ldr w15, [x14, #0xc]
//     tst w15, #0x00200000
//     b.ne 1b
//     mov w15, #'Y'
//     str w15, [x14]
//     dsb sy
//     isb

    b .  // Loop forever here so QEMU halts exactly on the faulting line!

    // 0x200: Synchronous from current EL with SPx
    .align 7

    // QEMU:  0x09000000
    // MESON: 0xc81004c0
//     movz x14, #0x0900, lsl #16
//     movz x14, #0x04c0
//     movk x14, #0xc810, lsl #16
// 1:
//     ldr w15, [x14, #0xc]
//     tst w15, #0x00200000
//     b.ne 1b
//     mov w15, #'X'
//     str w15, [x14]
//     dsb sy
//     isb

    b . // spin here

// 4-byte alignment
.align 2

.global asm_meson_uart_char
asm_meson_uart_char:
    // Build frame
    sub     sp, sp, #32
    stp     x29, x30, [sp, #16]     // Store frame pointer + link register
    add     x29, sp, #16
    stp     x14, x15, [sp, #0]      // Save the scratch registers used below

    // QEMU:  0x09000000
    // MESON: 0xc81004c0
    // movz x14, #0x0900, lsl #16
    movz x14, #0x04c0
    movk x14, #0xc810, lsl #16

    // Wait until UART FIFO is available
1:
    ldr w15, [x14, #0xc]
    tst w15, #0x00200000
    b.ne 1b

    // Write to Hardware UART FIFO
    str w0, [x14]
    dsb sy
    isb

    // Restore frame
    ldp     x14, x15, [sp, #0]
    ldp     x29, x30, [sp, #16]
    add     sp, sp, #32

    ret


    .global print_reg_hex
.type print_reg_hex, %function

asm_x0_hex:
    // Build frame
    // We need to preserve x19, x20, and x21 because they are callee-saved
    // registers we use to hold state across the nested 'bl' calls.
    sub     sp, sp, #48
    stp     x29, x30, [sp, #32]     // Store FP/LR
    add     x29, sp, #32
    stp     x19, x20, [sp, #16]     // Preserve x19, x20,
    str     x21, [sp, #0]           // and x21

    // --- Function Body ---
    mov     x19, x0                 // x19 = The 64-bit value to print
    mov     x20, #60                // x20 = Bit shift amount (start at highest nibble: 60, 56, ... 0)

.Lloop_hex:
    // Extract the current 4-bit nibble
    lsr     x21, x19, x20           // Shift value right by current shift amount
    and     x21, x21, #0xF          // Mask out everything except the lowest 4 bits

    // Convert the 4-bit value to an ASCII character
    cmp     x21, #10
    blt     .Lis_digit              // If 0-9, skip to digit handling

    // Handle letters (A-F)
    add     w0, w21, #('A' - 10)    // Convert 10-15 to 'A'-'F'
    b       .Lprint_char

.Lis_digit:
    // Handle digits (0-9)
    add     w0, w21, #'0'           // Convert 0-9 to '0'-'9'

.Lprint_char:
    // Call UART print
    bl      asm_meson_uart_char

    // Move to next nibble
    subs    x20, x20, #4            // Move right 4 bits (one nibble)
    b.ge    .Lloop_hex              // Continue looping until shift amount goes below 0

    // Restore from frame
    ldr     x21, [sp, #0]           // Restore x21,
    ldp     x19, x20, [sp, #16]     // x19, and x20.
    ldp     x29, x30, [sp, #32]     // Restore FP/LR
    add     sp, sp, #48             // Clean up reserved stack space
    ret

.global _start
.type _start, %function
_start:
    // x0 -> Physical base
    // x1 -> BootInfo*

    // ------------------------------------------------------------------------
    // Disable interrupts
    // ------------------------------------------------------------------------
    msr daifset, 3

    // QEMU:  0x09000000
    // MESON: 0xc81004c0
    // movz x14, #0x0900, lsl #16
    movz x14, #0x04c0
    movk x14, #0xc810, lsl #16
1:
    ldr w15, [x14, #0xc]
    tst w15, #0x00200000
    b.ne 1b

    mrs x15, CurrentEL
    ubfx x15, x15, #2, #2    // extract 2 bits starting at bit 2
    add x15, x15, #'0'

    str w15, [x14]
    dsb sy
    isb

    mov x19, x0
    mov x20, x1

    // x21 -> difference between virtual base - physical base
    // for physical to virtual conversion: add <vaddr>, <paddr>, x21
    // for virtual to physical conversion: sub <paddr>, <vaddr>, x21
    movz    x2, #0xffff, lsl #48  // x2 = virtual base (0xffff000000000000)
    sub     x21, x2, x0            // x21 = virtual base - physical base

    // ------------------------------------------------------------------------
    // Stack setup
    // ------------------------------------------------------------------------
    // Load the raw linked (virtual) address of the symbol
    ldr x3, =kernel_stack_top      // x3 = 0xffff.... (unmapped linked address)

    // Load the Virtual Base Base (0xffff000000000000)
    movz x4, #0xffff, lsl #48        // x4 = 0xffff000000000000

    // Subtract Virtual Base to get the relative offset from the linker origin
    sub x3, x3, x4                  // x3 = Symbol Offset (Distance from virtual start)

    // Add the loaded Physical Base (x19) to get the true execution address
    add x3, x3, x19                 // x3 = True physical address of the symbol

    // x3 holds the valid physical address. assign it to SP:
    mov sp, x3                      // Stack pointer is now set in physical space

    // ------------------------------------------------------------------------
    // Stack setup: COMPLETE
    // ------------------------------------------------------------------------

    mov x0, #'\n'
    bl asm_meson_uart_char

    mov x0, x19
    bl asm_x0_hex

    // Initial null stack frame
    // Move zero into two temporary registers
    mov     x29, #0
    mov     x30, #0
    // Push them onto the stack, allocating 16 bytes.
    // The '!' makes it pre-indexed, meaning SP decreases by 16 *before* storing.
    stp     x29, x30, [sp, #-16]!
    // Point the Frame Pointer register to the base of this new frame
    mov     x29, sp

    // Copy boot info
    sub sp, sp, #64
    mov x0, sp                  // destination = x29 - 64
    // NOTE: x1 -> source = BootInfo* passed from bootloader
    mov x2, #64                 // number of bytes
    bl kmemcpy


    // Copy efi memory map to stack, update pointer in bootinfo
    // Map size in 8 bytes is at boot_info + 40
    mov x25, sp
    ldr x2, [sp, #40]           // x2 = boot_info.mapSize
    add x2, x2, #15
    and x2, x2, #~15            // Round up to nearest 16 bytes
    sub sp, sp, x2
    mov x0, sp                  // destination = x29 - boot_info.mapSize
    ldr x1, [x25, #32]          // source = boot_info.mapBase
    // NOTE: x2 -> number of bytes = boot_info.mapSize
    bl kmemcpy

    add x0, x0, x21             // x0 = P2V(x0)
    str x0, [x25, #32]          // update boot_info pointer

    // find physical offset for mapping
    // x22 -> physical offset for mapping
    mov x22, #0
    // iterate EFI memory map to find first type 7 (EfiConventionalMemory)
    // region, and use that physical address as our base.
    ldr x0, [x25, #32] // x0 -> boot_info.map
    sub x0, x0, x21    // x0 = V2P(x0)
    ldr x2, [x25, #40] // load boot_info.map_size
    ldr x3, [x25, #48] // load boot_info.descriptor_size

    add x2, x0, x2 // x2 = end address of the memory map (map_start + map_size)

.loop_mem_map:
    cmp x0, x2           // Have we reached the end of the memory map?
    b.hs .map_not_found  // If x0 >= x2, type 7 region was not found

    ldr w1, [x0]         // load efi_memory_descriptor.type (4 bytes)
    cmp w1, #7           // check if type == 7 (EfiConventionalMemory)
    b.eq .found_conventional

    add x0, x0, x3 // Advance to the next descriptor: x0 = x0 + descriptor_size
    b .loop_mem_map

.found_conventional:
    // x22 -> physical offset for mapping
    ldr x22, [x0, #8] // x22 = physical address base of the first type 7 region
    // align down to 2MiB
    lsr x22, x22, #21           // Shift right to strip unaligned bits
    lsl x22, x22, #21           // Shift back left to restore the 2MiB alignment
.map_not_found:

    mov x0, x19                 // x0 = physical address base

// ============================================================================
// PAGETABLE INITIALIZATION
// ============================================================================

// Inputs:
//   x0 = Physical address dynamically passed
init_paging:
    // ------------------------------------------------------------------------
    // Task 1: Identity Map (TTBR0)
    // ------------------------------------------------------------------------
    // Target 0: Kernel Image
    // x0 = address to identity map

    // x1 = kernel image size in bytes
    // Calculate kernel image size
    adrp    x1, KERNEL_END
    add     x1, x1, :lo12:KERNEL_END      // x1 = Full absolute KERNEL_END address
    adrp    x11, KERNEL_VIRTUAL
    add     x11, x11, :lo12:KERNEL_VIRTUAL// x11 = Full absolute KERNEL_VIRTUAL address
    sub     x1, x1, x11                   // x1 = Kernel Size in Bytes

    // x2 = page table pointer
    adrp    x2, id_l0
    add     x2, x2, :lo12:id_l0

    // x3 = pool base
    // Load common base page pool starting pointer
    adrp    x3, table_pool
    add     x3, x3, :lo12:table_pool // x3 = Address of table pool

    bl      map_kernel_identity
    // Catch updated pool pointer in x3
    mov     x3, x0 // x3 = pool base

    // Target 2: Device Tree Blob
    ldr     x0, [x25, #8]               // Load physical DTB pointer from boot info
    // x0 = pointer to map

    // Parse DTB Size from its Big-Endian Header field (offset +4 bytes)
    ldr     w1, [x0, #4]                // Load 4-byte size field (Big-Endian)
    rev     w1, w1                      // Convert Big-Endian to Little-Endian
    uxtw    x1, w1                      // Zero-extend 32-bit size into 64-bit x1
    // x1 = size in bytes to map

    adrp    x2, id_l0
    add     x2, x2, :lo12:id_l0         // x2 = Root L0 Pointer
    // x2 = table to map it in

    // x3 = Current free pool tracking pointer

    // Map the DTB region
    bl      map_kernel_identity         // Generates L1/L2 tables as needed from pool
    mov     x3, x0                      // Save updated free pool tracker back to x4

    // Target 3: UART debug address
    // movz    x0, #0x0900, lsl #16 // QEMU
    movz    x0, #0x04c0
    movk    x0, #0xc810, lsl #16

    mov     x1, #4096
    // x1 = size in bytes to map

    adrp    x2, id_l0
    add     x2, x2, :lo12:id_l0         // x2 = Root L0 Pointer
    // x2 = table to map it in

    // x3 = Current free pool tracking pointer
    bl      map_kernel_identity
    mov     x4, x0

    // ------------------------------------------------------------------------
    // Task 2: Higher Half Mapping Layout (TTBR1)
    // ------------------------------------------------------------------------
    // Target 1: Kernel Image (x0)
    // x0 = kernel virtual base
    // TODO: get from linker without literal pool
    movz    x0, #0xffff, lsl #48        // Target Base: 0xffff000000000000
    // x1 = kernel physical base
    mov     x1, x19                     // Physical base address

    // x2 = kernel image size
    adrp    x2, KERNEL_END
    add     x2, x2, :lo12:KERNEL_END       // x2 = Full absolute KERNEL_END address
    adrp    x11, KERNEL_VIRTUAL
    add     x11, x11, :lo12:KERNEL_VIRTUAL // x11 = Full absolute KERNEL_VIRTUAL address
    sub     x2, x2, x11                    // x2 = Kernel Size in Bytes

    // x3 = page table pointer
    adrp    x3, k_l0
    add     x3, x3, :lo12:k_l0          // Higher-half root pointer

    // x4 = pool base

    bl      map_kernel_flexible
    // Catch updated pool pointer in x4
    mov     x4, x0

    // ------------------------------------------------------------------------
    // Target 2: 64MiB Direct RAM Region @ 0xffff800000000000
    // ------------------------------------------------------------------------
    // x0 = virtual address
    movz    x0, #0xffff, lsl #48
    movk    x0, #0x8000, lsl #32        // x0 = 0xffff800000000000
    // x1 = physical address
    mov     x1, x22                     // Physical base = Conventional RAM base
    // x2 = size in bytes
    mov     x2, #0x40000000             // Size = 1GiB
    // x3 = page table pointer
    adrp    x3, k_l0
    add     x3, x3, :lo12:k_l0
    // x4 = pool base
    bl      map_kernel_flexible
    mov     x4, x0                      // Save updated pool pointer in x4

    // ------------------------------------------------------------------------
    // Target 3: Map Kernel into the Direct RAM Region Space
    // ------------------------------------------------------------------------
    sub     x0, x19, x22                // Offset inside physical RAM block
    movz    x1, #0xffff, lsl #48
    movk    x1, #0x8000, lsl #32
    add     x0, x1, x0                  // x0 = Target virtual location inside RAM window
    mov     x1, x19                     // Physical base = True physical kernel base
    // x2 still holds our exact kernel size from earlier calculation
    adrp    x3, k_l0
    add     x3, x3, :lo12:k_l0
    // x4 = pool base
    bl      map_kernel_flexible
    mov     x4, x0

    // ------------------------------------------------------------------------
    // Target 4: Map Device Tree Blob into Higher Half Direct RAM Region
    // ------------------------------------------------------------------------
    ldr     x0, [x25, #8]               // Load raw physical DTB pointer from boot info
    cbz     x0, .L_high_skip_dtb        // If the DTB pointer is NULL, skip high-half mapping

    // Parse the dynamic DTB size from its Big-Endian header field (offset +4 bytes)
    ldr     w1, [x0, #4]                // Load 4-byte size field (Big-Endian)
    rev     w1, w1                      // Convert Big-Endian to Little-Endian
    uxtw    x2, w1                      // x2 = Size of region in bytes (for function argument)

    // Calculate target high-half virtual address inside the Direct RAM window:
    //    Virtual Address = 0xffff800000000000 + (dtb_physical_address - conventional_ram_base)
    sub     x1, x0, x22                 // x1 = Offset of DTB inside physical RAM block
    movz    x10, #0xffff, lsl #48
    movk    x10, #0x8000, lsl #32       // x10 = Direct RAM virtual root base address
    add     x0, x10, x1                 // x0 = Target High-Half Virtual Address for DTB

    // Set up remaining parameters for the block mapper function
    ldr     x1, [x25, #8]               // x1 = True raw physical base address of DTB
    // x2 already contains the parsed DTB size from step 2
    adrp    x3, k_l0
    add     x3, x3, :lo12:k_l0          // x3 = Root PGD base table pointer
    // x4 already contains free pool pointer
    bl      map_kernel_flexible
    mov     x4, x0                      // Save the final updated free pool tracking pointer

    // Update boot info struct copy on the stack
    ldr     x1, [x25, #8]
    sub     x1, x1, x22                 // Recalculate RAM block offset
    movz    x10, #0xffff, lsl #48
    movk    x10, #0x8000, lsl #32       // x10 = Direct RAM virtual root base address
    add     x0, x10, x1                 // Reconstruct the virtual pointer
    str     x0, [x25, #8]               // Update boot_info.dtb pointer with high-half vaddr

.L_high_skip_dtb:

    // ------------------------------------------------------------------------
    // Setup Environment Registers & Activate Translation Engine
    // ------------------------------------------------------------------------
    // --- (MMU Configuration & Enable) ---
    dsb sy
    isb

    // Flush entire table pool to physical RAM
    adrp x0, table_pool
    add x0, x0, :lo12:table_pool
    adrp x1, table_pool_end
    add x1, x1, :lo12:table_pool_end
    bl clean_dcache_range

    // If at EL2, drop to EL1 so that we are actually in the environment we think we are.
    // basically, this is the transition from "UEFI Identity Maps" to "EL1 MMU Off".
    mrs x0, CurrentEL
    ubfx x0, x0, #2, #2
    cmp x0, #2
    b.eq from_el2

    cmp x0, #1
    b el1_landing_pad

1:
    wfe
    b 1b // hang if unexpected exception level

from_el2:
    mov x0, #(1 << 31)    // Set Bit 31 (RW = 1)
    msr hcr_el2, x0

    mov x0, #0x3c5
    msr spsr_el2, x0

    adrp x0, el1_landing_pad
    add x0, x0, :lo12:el1_landing_pad
    msr elr_el2, x0

    mov x0, xzr
    msr sctlr_el1, x0

    mov x0, sp
    msr sp_el1, x0

    isb
    eret

el1_landing_pad:
    // Disable MMU to change control registers safely
    mrs x3, sctlr_el1
    mov x4, #0x1005
    bic x3, x3, x4
    msr sctlr_el1, x3
    isb

    // QEMU:  0x09000000
    // MESON: 0xc81004c0
    // movz x14, #0x0900, lsl #16
    movz x14, #0x04c0
    movk x14, #0xc810, lsl #16
1:
    ldr w15, [x14, #0xc]
    tst w15, #0x00200000
    b.ne 1b

    mrs x15, CurrentEL
    ubfx x15, x15, #2, #2
    add x15, x15, #'0'

    str w15, [x14]
    dsb sy
    isb

    adrp    x0, exception_vectors
    add     x0, x0, :lo12:exception_vectors
    msr     vbar_el1, x0
    isb

    // QEMU:  0x09000000
    // MESON: 0xc81004c0
    // movz x14, #0x0900, lsl #16
    movz x14, #0x04c0
    movk x14, #0xc810, lsl #16
1:
    ldr w15, [x14, #0xc]
    tst w15, #0x00200000
    b.ne 1b
    mov w15, #'*'
    str w15, [x14]
    dsb sy
    isb

    // Configures the Memory Attribute Indirection Register.
    // The value 0xff04 defines two distinct memory attribute types in the
    // MAIR_EL1 slots:
    //
    // 0x04 (Slot 0): Device-nGnRE memory (typically used for MMIO hardware
    //   registers, strictly ordered, non-cacheable).
    //
    // 0xff (Slot 1): Normal memory (Outer/Inner Write-Back Non-transient,
    //   Read/Write Allocate, used for your standard RAM cache configurations).
    mov x3, #0xff04
    msr mair_el1, x3

    // Programs the Translation Control Register to define how address
    // translation behaves for both the lower half (TTBR0) and upper half
    // (TTBR1) of the virtual address space.
    // T0SZ / T1SZ sizing: Dictates the size of the virtual address
    // spaces (e.g., configuring how many bits are used for translation, often
    // matching a 48-bit or 39-bit address space layout).

    // Read current TCR_EL1 to preserve implementation-defined/reserved bits
    mrs     x3, tcr_el1

    // Build the clearing mask inside x4
    mov     x4, #0xffff
    movk    x4, #0xffff, lsl #16 // x4 now holds 0x00000000FFFFFFFF
    movk    x4, #0x7, lsl #32    // x4 now holds 0x00000007FFFFFFFF
    bic     x3, x3, x4     // Clear target fields in our base register

    // Build explicit configuration layout in x2
    mov     x2, #0x3f10
    movk    x2, #0xbf10, lsl #16

    // Dynamically fetch hardware physical address size (PARange) into x4
    mrs     x4, id_aa64mmfr0_el1
    and     x4, x4, #0xf        // Extract PARange [3:0]
    lsl     x4, x4, #32         // Shift to IPS position [34:32]

    // Merge explicit config and dynamic IPS into the preserved base register
    orr     x2, x2, x4  // Combine config + dynamic IPS
    orr     x3, x3, x2  // Merge it back into the cleaned TCR_EL1 base

    // Commit to system register
    msr     tcr_el1, x3
    isb                         // Synchronize context

    // Set page table roots
    adrp    x1, id_l0
    add     x1, x1, :lo12:id_l0
    msr ttbr0_el1, x1           // lower half (identity map)
    adrp    x1, k_l0
    add     x1, x1, :lo12:k_l0
    msr ttbr1_el1, x1           // upper half (kernel map)

    // Flush caches
    tlbi vmalle1is
    dsb sy
    isb

    // QEMU:  0x09000000
    // MESON: 0xc81004c0
    // movz x14, #0x0900, lsl #16
    movz x14, #0x04c0
    movk x14, #0xc810, lsl #16
1:
    ldr w15, [x14, #0xc]
    tst w15, #0x00200000
    b.ne 1b
    mov w15, #'$'
    str w15, [x14]
    dsb sy
    isb

    // Re-enable MMU
    mrs x3, sctlr_el1
    orr x3, x3, #0x1            // MMU
    orr x3, x3, #0x4            // D-Cache
    orr x3, x3, #0x1000         // I-Cache
    msr sctlr_el1, x3
    isb

    // Flush old UEFI cached translations
    tlbi vmalle1is
    dsb ish
    isb

    // QEMU:  0x09000000
    // MESON: 0xc81004c0
    // movz x14, #0x0900, lsl #16
    movz x14, #0x04c0
    movk x14, #0xc810, lsl #16
1:
    ldr w15, [x14, #0xc]
    tst w15, #0x00200000
    b.ne 1b
    mov w15, #'*'
    str w15, [x14]
    dsb sy
    isb

    // --- STACK ADDRESS FIXUP ---
    // Calculate and apply the higher-half virtual offset to the stack pointer.
    // Higher-half kernel base starts at 0xffff000000000000:
    mov x3, sp                   // Copy current physical stack pointer
    // x19 == physical base
    sub x3, x3, x19               // x3 = current stack offset relative to physical base

    movz x4, #0xffff, lsl #48    // Form the kernel virtual higher-half offset
    add x3, x3, x4               // x3 = stack offset + virtual base
    mov sp, x3                   // Update the stack pointer

    // --- THE HIGHER-HALF JUMP ---
    ldr x3, =kernel_virtual_entry  // Load the 0xffff... higher-half address
    br  x3                         // Jump to the virtual upper-half

kernel_virtual_entry:
    // --- YOU ARE NOW IN THE KERNEL, AS IT IS LINKED ---

    add x29, x29, x21 // update link register to maintain stacktrace

    // QEMU:  0x09000000
    // MESON: 0xc81004c0
    // movz x14, #0x0900, lsl #16
    movz x14, #0x04c0
    movk x14, #0xc810, lsl #16
1:
    ldr w15, [x14, #0xc]
    tst w15, #0x00200000
    b.ne 1b
    mov w15, #'$'
    str w15, [x14]
    dsb sy
    isb

    // Call the higher level kernel main function
    mov x0, x19                 // Arg0 -> physical image base
    mov x1, x25                 // Arg1 -> BootInfo*
    bl kmain

1:
    wfe
    b 1b

/*
 * void *kmemcpy(void *dest, const void *src, size_t count);
 *
 * Inputs (AAPCS64 standard):
 *   x0 = Destination address (dest)
 *   x1 = Source address (src)
 *   x2 = Number of bytes to copy (count)
 *
 * Outputs:
 *   x0 = Original destination address
 */
.global kmemcpy
.type kmemcpy, %function
kmemcpy:
    // Save the original destination pointer to return at the end
    mov     x9, x0

.Lcopy_64_loop:
    // Check if we can do a large 64-byte unrolled block copy
    cmp     x2, #64
    b.lo    .Lcopy_32_loop

    // Load 64 bytes using register pairs
    ldp     x3, x4,   [x1]
    ldp     x5, x6,   [x1, #16]
    ldp     x7, x8,   [x1, #32]
    ldp     x10, x11, [x1, #48]
    add     x1, x1, #64

    // Store 64 bytes to destination
    stp     x3, x4,   [x0]
    stp     x5, x6,   [x0, #16]
    stp     x7, x8,   [x0, #32]
    stp     x10, x11, [x0, #48]
    add     x0, x0, #64

    sub     x2, x2, #64
    b       .Lcopy_64_loop

.Lcopy_32_loop:
    // Check if we can still do a tight 32-byte block copy
    cmp     x2, #32
    b.lo    .Lcopy_tail

    ldp     x3, x4, [x1], #32
    stp     x3, x4, [x0], #32
    sub     x2, x2, #32
    b       .Lcopy_32_loop

.Lcopy_tail:
    // If count dropped exactly to 0, we are done
    cbz     x2, .Lcopy_done

.Ltail_loop:
    // Byte-by-byte copy for any remaining trailing bytes (< 32)
    ldrb    w3, [x1], #1
    strb    w3, [x0], #1
    subs    x2, x2, #1
    b.ne    .Ltail_loop

.Lcopy_done:
    // Restore original destination pointer into x0 for return value
    mov     x0, x9
    ret
