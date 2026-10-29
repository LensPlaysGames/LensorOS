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

.section .bss
.align 12
kernel_stack_bottom:
    ;;# 64 KiB stack
    .zero 0x16000
kernel_stack_top:

// ============================================================================
// EXPLICIT STATIC TABLE ALLOCATIONS (.data)
// ============================================================================
.section .data

// LOWER HALF (TTBR0 Identity Map):
// id_l0 -> id_l1 -> id_l2
//   Maps 2MB Identity Code Block at dynamic load address
//
// UPPER HALF (TTBR1 Kernel & RAM):
//                 ,-> k_l2_code -- Maps 2MB Higher-Half Code
// k_l0  -> k_l1 --|
//                 `-> k_l2_ram  -- Maps sequential 64MB RAM window

// 4KB page alignment
.align 12
id_l0:     .zero 4096 // 4e000
id_l1:     .zero 4096 // 4f000
id_l2:     .zero 4096 // 50000
k_l0:      .zero 4096 // 51000
k_l1:      .zero 4096 // 52000
k_l2_ram:  .zero 4096 // 53000
k_l2_code: .zero 4096 // 54000
k_l3_code: .zero 4096 // 55000

.section .text
.align 11
exception_vectors:
    // 0x000: Synchronous from current EL with SP0
    .align 7
    b .  // Loop forever here so QEMU halts exactly on the faulting line!

    // 0x200: Synchronous from current EL with SPx
    .align 7
    b .  // Putting a spin-lock here stops the infinite loop

// 4-byte alignment
.align 2
.global _start
.type _start, %function
_start:
    // x0 -> Physical base
    // x1 -> BootInfo*

    // ------------------------------------------------------------------------
    // Disable interrupts
    // ------------------------------------------------------------------------
    msr daifset, 3

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

    mov x0, x19                 // x0 = physical address base

// ============================================================================
// PAGETABLE INITIALIZATION
// ============================================================================

// Inputs:
//   x0 = Physical address dynamically passed
init_paging:
    mov     x4, #0x001fffff             // Mask for lower 21 bits
    bic     x0, x0, x4                  // Clear lower 21 bits to align to 2MB

    // ------------------------------------------------------------------------
    // Discover Physical Addresses of Static Tables Dynamically
    // ------------------------------------------------------------------------
    adrp x1, id_l0
    add x1, x1, :lo12:id_l0     // x1 = id_l0
    adrp x2, id_l1
    add x2, x2, :lo12:id_l1     // x2 = id_l1
    adrp x3, id_l2
    add x3, x3, :lo12:id_l2     // x3 = id_l2

    adrp x5, k_l0
    add x5, x5, :lo12:k_l0      // x5 = k_l0
    adrp x6, k_l1
    add x6, x6, :lo12:k_l1      // x6 = k_l1
    adrp x10, k_l2_code
    add x10, x10, :lo12:k_l2_code // x10 = k_l2_code
    adrp x11, k_l3_code
    add x11, x11, :lo12:k_l3_code // x11 = k_l3_code
    adrp x12, k_l2_ram
    add x12, x12, :lo12:k_l2_ram // x12 = k_l2_ram

    // ------------------------------------------------------------------------
    // Zero Out All Static Table Memory Space
    // ------------------------------------------------------------------------
    mov     x13, #0
zero_all_tables:
    str     xzr, [x1, x13]              // id_l0
    str     xzr, [x2, x13]              // id_l1
    str     xzr, [x3, x13]              // id_l2
    str     xzr, [x5, x13]              // k_l0
    str     xzr, [x6, x13]              // k_l1
    str     xzr, [x10, x13]             // k_l2_code
    str     xzr, [x11, x13]             // k_l3_code
    str     xzr, [x12, x13]             // k_l2_ram
    add     x13, x13, #8
    cmp     x13, #4096
    b.ne    zero_all_tables

    // ------------------------------------------------------------------------
    // Configure TTBR0: 4-Level Identity Map
    // ------------------------------------------------------------------------
    // Target 1: Kernel Image (x0)
    // Link Level 0 -> Level 1
    lsr     x13, x0, #39                // Level 0 bits: [47:39]
    and     x13, x13, #0x1ff
    orr     x4, x2, #0x3                // Table descriptor flag
    str     x4, [x1, x13, lsl #3]

    // Link Level 1 -> Level 2
    lsr     x13, x0, #30                // Level 1 bits: [38:30]
    and     x13, x13, #0x1ff
    orr     x4, x3, #0x3                // Table descriptor flag
    str     x4, [x2, x13, lsl #3]

    // Drop 2MB Block Descriptor into Level 2
    lsr     x13, x0, #21                // Level 2 bits: [29:21]
    and     x13, x13, #0x1ff
    mov     x4, #0x0705                 // Normal Memory block attributes
    orr     x4, x0, x4                  // Output physical matched perfectly to virtual
    str     x4, [x3, x13, lsl #3]

    // Target 2: Device Tree Blob
    ldr     x0, [x25, #8]       // Load device tree pointer into x0

    // Align x0 down to 2MiB boundary
    bic     x0, x0, #0x1fffff   // 0x1fffff covers 21 bits

    // Link Level 0 -> Level 1
    lsr     x13, x0, #39        // Level 0 bits: [47:39]
    and     x13, x13, #0x1ff
    orr     x4, x2, #0x3        // Table descriptor flag
    str     x4, [x1, x13, lsl #3]

    // Link Level 1 -> Level 2
    lsr     x13, x0, #30        // Level 1 bits: [38:30]
    and     x13, x13, #0x1ff
    orr     x4, x3, #0x3        // Table descriptor flag
    str     x4, [x2, x13, lsl #3]

    // Drop 2MiB Block Descriptor into Level 2
    lsr     x13, x0, #21        // Level 2 bits: [29:21]
    and     x13, x13, #0x1ff
    mov     x4, #0x0705
    orr     x4, x0, x4
    str     x4, [x3, x13, lsl #3]

    // ------------------------------------------------------------------------
    // Configure TTBR1: 4-Level High-Half Mapping Layout
    // ------------------------------------------------------------------------
    // Target 1: Higher Half Kernel @ 0xffff000000000000 (2MB mapped via 4KB pages)

    orr     x4, x6, #0x3        // Link k_l0 -> k_l1
    str     x4, [x5, #0]        // Store in L0 Entry 0

    orr     x4, x10, #0x3       // Link k_l1 -> k_l2_code
    str     x4, [x6, #0]        // Store in L1 Entry 0

    // Link k_l2_code -> k_l3_code (Table descriptor, bits [1:0] = 0x3)
    orr     x4, x11, #0x3
    str     x4, [x10, #0]       // Store in k_l2_code Entry 0

    // ------------------------------------------------------------------------
    // Loop to Populate L3: Map 512 entries (2 MiB total)
    // ------------------------------------------------------------------------
    mov     x14, #0     // x14 = L3 entry index counter (0 to 511)
    mov     x15, x19    // x15 = Working copy of physical base address
    mov     x16, #0x0707 // x16 = Page attributes (Normal Memory, RW, Valid)

map_l3_loop:
    orr     x4, x15, x16 // Combine current physical address + attributes
    str     x4, [x11, x14, lsl #3] // Store into k_l3_code[x14] (Index * 8 bytes)

    add     x15, x15, #0x1000 // Move physical address to next 4 KiB page
    add     x14, x14, #1      // Increment entry index
    cmp     x14, #512         // Have we filled all 512 entries?
    b.ne    map_l3_loop       // If not, loop back


    // Target 2: 64MiB Direct RAM Region @ 0xffff800000000000
    // 0xffff800000000000 sets Level 0 Index to 256
    orr     x4, x12, #0x3 // Link k_l1 -> k_l2_ram (Our independent L2 table!)
    str     x4, [x6, #8] // Store inside k_l1 Entry 1 (Index 1 handles next 512MB slot)

    // Map 32 sequential 2MB blocks (64MiB total) into k_l2_ram page
    mov     x14, #0   // Loop counter
    mov     x7, xzr   // Physical frame pointer starting at 0x00000000
ram_map_loop:
    mov     x4, #0x0705
    orr     x4, x7, x4
    str     x4, [x12, x14, lsl #3]

    add     x7, x7, #0x00200000         // Jump base PA forward by 2MB
    add     x14, x14, #1
    cmp     x14, #32
    b.lt    ram_map_loop


    // ------------------------------------------------------------------------
    // Setup Environment Registers & Activate Translation Engine
    // ------------------------------------------------------------------------
    // --- (MMU Configuration & Enable) ---
    dsb sy
    isb

    // Disable MMU to change control registers safely
    mrs x3, sctlr_el1
    mov x4, #0x1005
    bic x3, x3, x4
    msr sctlr_el1, x3
    isb

    adrp    x0, exception_vectors
    add     x0, x0, :lo12:exception_vectors
    msr     vbar_el1, x0
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
    mov     x2, #0x3510
    movk    x2, #0xa510, lsl #16

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
    msr ttbr0_el1, x1           // lower half (identity map)
    msr ttbr1_el1, x5           // upper half (kernel map)

    // Flush old UEFI cached translations
    tlbi vmalle1is
    dsb ish
    isb

    // Re-enable MMU with your new mappings
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
    // --- YOU ARE NOW SAFELY IN THE KERNEL ---
    // You can now safely disable TTBR0 to unmap the identity table:
    // msr ttbr0_el1, xzr
    // tlbi vmalle1is
    // dsb ish
    // isb

    // Call your C/C++ kernel main function
    mov x0, x19
    mov x1, x25
    bl kmain

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
