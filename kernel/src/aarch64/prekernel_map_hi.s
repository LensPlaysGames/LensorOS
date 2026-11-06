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

// ============================================================================
// FUNCTION: map_kernel_flexible
// ============================================================================
// Arguments (AAPCS compliant):
//   x0 = Target Virtual Base Address (e.g., 0xffff000000000000)
//   x1 = Physical base address of region to map
//   x2 = Size of region in bytes (e.g., KERNEL_END - KERNEL_VIRTUAL)
//   x3 = Base address of the root Level 0 page table (e.g., k_l0)
//   x4 = Base address of the table allocation pool
//
// Returns:
//   x0 = Updated next-available free pointer in the allocation pool
// ============================================================================
.global map_kernel_flexible
.type map_kernel_flexible, %function
map_kernel_flexible:
    stp     x29, x30, [sp, #-64]!
    mov     x29, sp
    stp     x19, x20, [sp, #16]         // x19 = Phys Page tracker, x20 = L0 Table Base
    stp     x21, x22, [sp, #32]         // x21 = Alloc Pool,       x22 = Aligned End Address
    stp     x23, x24, [sp, #48]         // x23 = Virt Page tracker

    // Move incoming parameters into non-volatile callee-saved registers
    mov     x23, x0                     // x23 = Virtual Base Target
    mov     x19, x1                     // x19 = Physical Base
    mov     x20, x3                     // x20 = Root L0 Table Pointer
    mov     x21, x4                     // x21 = Table Allocation Pool Pointer

    // 1. Strict 4KiB Granule Safe-Alignment Check
    mov     x6, #0xfff                  // 4KiB page mask
    and     x7, x19, x6                 // Capture physical unaligned remainder
    bic     x19, x19, x6                // Force realign physical base down
    bic     x23, x23, x6                // Force realign virtual base down

    add     x2, x2, x7                  // Account for size extension due to alignment
    add     x22, x19, x2                // Compute raw end address based on physical offset
    add     x22, x22, x6                // Ceiling pad to next 4KiB boundary
    bic     x22, x22, x6                // Final aligned loop end boundary

.L_flex_loop:
    // --- LEVEL 0 PROCESSING (512GiB Blocks) ---
    lsr     x13, x23, #39
    and     x13, x13, #0x1ff
    ldr     x10, [x20, x13, lsl #3]
    cbnz    x10, .L_flex_l0_linked

    orr     x10, x21, #0x3
    str     x10, [x20, x13, lsl #3]
    add     x21, x21, #4096
.L_flex_l0_linked:
    bic     x10, x10, #0x3

    // --- LEVEL 1 PROCESSING (1GiB Blocks) ---
    lsr     x13, x23, #30
    and     x13, x13, #0x1ff
    ldr     x11, [x10, x13, lsl #3]
    cbnz    x11, .L_flex_l1_linked

    orr     x11, x21, #0x3
    str     x11, [x10, x13, lsl #3]
    add     x21, x21, #4096
.L_flex_l1_linked:
    bic     x11, x11, #0x3

    // --- LEVEL 2 SELECTION (2MiB Block vs 4KiB Page Table Check) ---
    lsr     x13, x23, #21
    and     x13, x13, #0x1ff
    add     x15, x11, x13, lsl #3       // x15 = Target Level 2 descriptor pointer
    ldr     x12, [x15]                  // Read existing entry state

    // Condition A: If Level 2 slot houses an active Level 3 Table pointer, drop down
    and     x5, x12, #0x3
    cmp     x5, #0x3                    // If bits [1:0] == 11, it's a table descriptor
    b.eq    .L_flex_force_4k

    // Condition B: Are we 2MiB aligned? (Check both Physical and Virtual trackers)
    mov     x6, #0xffff
    movk    x6, #0x1f, lsl #16          // x6 = 0x1fffff (2MiB - 1)
    and     x7, x19, x6
    cbnz    x7, .L_flex_force_4k        // Phys unaligned? Go 4KiB
    and     x7, x23, x6
    cbnz    x7, .L_flex_force_4k        // Virt unaligned? Go 4KiB

    // Condition C: Is there at least 2MiB remaining in the requested mapping range?
    sub     x7, x22, x19                // bytes left = end - current
    cmp     x7, #0x200000               // Compare against 2MiB
    b.lo    .L_flex_force_4k            // Not enough bytes remaining? Go 4KiB

    // --- COMMIT 2MiB BLOCK ENTRY ---
    mov     x4, #0x0705                 // Block flags (AF, AP=01, AttrIndx=001, Block bit=0)
    orr     x5, x19, x4
    str     x5, [x15]                   // Overwrite Level 2 entry as a 2MiB Block

    add     x19, x19, #0x200000         // Advance forward by 2MiB
    add     x23, x23, #0x200000
    b       .L_flex_loop_test

    // --- DROP TO 4KiB PAGE ENTRY ---
.L_flex_force_4k:
    cbnz    x12, .L_flex_l2_linked      // If L3 Page Table pointer already allocated, use it

    orr     x12, x21, #0x3              // Allocate a fresh clean Level 3 table
    str     x12, [x15]
    add     x21, x21, #4096
.L_flex_l2_linked:
    bic     x12, x12, #0x3              // Isolate raw pointer base address

    lsr     x13, x23, #12               // Extract index bits [20:12]
    and     x13, x13, #0x1ff
    add     x15, x12, x13, lsl #3       // Compute specific index slot inside L3 page table

    mov     x5, #0x0707                 // Page flags (AF, AP=01, AttrIndx=001, Page bit=1)
    orr     x5, x19, x5
    str     x5, [x15]                   // Commit 4KiB leaf mapping

    add     x19, x19, #4096             // Advance forward by 4KiB
    add     x23, x23, #4096

.L_flex_loop_test:
    cmp     x19, x22
    b.lo    .L_flex_loop

    // Stack Clean-up & Return allocation pool pointer back in x0
    mov     x0, x21
    ldp     x23, x24, [sp, #48]
    ldp     x21, x22, [sp, #32]
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #64
    ret
