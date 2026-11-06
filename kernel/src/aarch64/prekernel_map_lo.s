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
// FUNCTION: map_kernel_identity
// ============================================================================
// Purpose: Maps a variable-sized region into the identity translation
//          table (TTBR0) dynamically expanding L1/L2 structures using
//          an allocation pool.
//
// Arguments:
//   x0 = Physical base address of region to map
//   x1 = Size of region in bytes
//   x2 = Base address of the root Level 0 page table
//   x3 = Base address of the table allocation pool
//
// Returns:
//   x0 = Updated next-available free pointer in the allocation pool
// ============================================================================
.global map_kernel_identity
.type map_kernel_identity, %function
map_kernel_identity:
    // Stack Frame & Register Preservation (Callee-saved registers used inside)
    stp     x29, x30, [sp, #-48]!       // Save frame pointer and link register
    mov     x29, sp
    stp     x19, x20, [sp, #16]         // Save x19, x20
    stp     x21, x22, [sp, #32]         // Save x21, x22

    // Unaligned Safeguard: Calculate the misalignment offset
    mov     x6, #0xffff
    movk    x6, #0x1f, lsl #16          // x6 = 0x1fffff
    and     x7, x0, x6                  // x7 = misalignment offset in bytes

    // Align base address down and expand size to capture the full region
    bic     x19, x0, x6                 // x19 = Aligned Physical Base Step
    add     x1, x1, x7                  // Expand size by the offset amount

    mov     x20, x2                     // x20 = Root Level 0 Table pointer
    mov     x21, x3                     // x21 = Next available page pointer in pool

    // Compute loop termination threshold boundary
    add     x22, x19, x1                // x22 = True raw physical end address
    add     x22, x22, x6                // Ceiling padding
    bic     x22, x22, x6                // x22 = Aligned boundary check value

.L_func_map_loop:
    // --- LEVEL 0 PROCESSING (512GiB Boundaries) ---
    lsr     x13, x19, #39               // Get L0 index bits [47:39]
    and     x13, x13, #0x1ff
    ldr     x10, [x20, x13, lsl #3]     // Read current L0 entry
    cbnz    x10, .L_func_l0_linked      // If table exists, proceed to track it

    // Allocate new L1 table page from pool
    orr     x10, x21, #0x3              // Table descriptor flag + pointer
    str     x10, [x20, x13, lsl #3]     // Commit entry to parent table
    add     x21, x21, #4096             // Step pool allocator forward 4KB
.L_func_l0_linked:
    bic     x10, x10, #0x3              // Clear attribute bits to get raw address

    // --- LEVEL 1 PROCESSING (1GiB Boundaries) ---
    lsr     x13, x19, #30               // Get L1 index bits [38:30]
    and     x13, x13, #0x1ff
    ldr     x11, [x10, x13, lsl #3]     // Read current L1 entry
    cbnz    x11, .L_func_l1_linked

    orr     x11, x21, #0x3
    str     x11, [x10, x13, lsl #3]
    add     x21, x21, #4096
.L_func_l1_linked:
    bic     x11, x11, #0x3

    // --- LEVEL 2 PROCESSING (2MiB Block Allocation) ---
    lsr     x13, x19, #21               // Get L2 index bits [29:21]
    and     x13, x13, #0x1ff
    add     x15, x11, x13, lsl #3       // Compute specific destination index slot

    mov     x4, #0x0705
    orr     x5, x19, x4
    str     x5, [x15]

    // --- LOOP MANAGEMENT ---
    mov     x6, #1
    lsl     x6, x6, #21                 // x6 = 2MiB step size (0x200000)
    add     x19, x19, x6                // Step current physical address tracker forward

    cmp     x19, x22
    b.lo    .L_func_map_loop

    // Return value configuration & Stack Clean-up
    mov     x0, x21                     // Return the updated pool pointer back in x0
    ldp     x21, x22, [sp, #32]
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #48         // Restore frame pointer + link register
    ret
