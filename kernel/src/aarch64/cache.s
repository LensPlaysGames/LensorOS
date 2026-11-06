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
// FUNCTION: clean_dcache_range
// ============================================================================
// Purpose: Flushes L1/L2 cache down to Point-of-Coherency (physical RAM).
//
// Arguments:
//   x0 = address that begins range (inclusive)
//   x1 = address that ends range (exclusive)
//
// ============================================================================
    .global clean_dcache_range
clean_dcache_range:
    mrs x2, ctr_el0
    ubfx x2, x2, #16, #4
    mov x3, #4
    lsl x2, x3, x2

    sub x3, x2, #1
    bic x0, x0, x3

.Ldcache_loop:
    dc cvac, x0     // clear cache line at x0
    add x0, x0, x2  // move x0 to next cache line
    cmp x0, x1      // until we are done
    b.lo .Ldcache_loop

    dsb sy
    isb
    ret
