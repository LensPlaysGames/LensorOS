/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#ifndef LENSOROS_DEVICE_TREE_H
#define LENSOROS_DEVICE_TREE_H

#include <integers.h>

/// fdt -> Flattened Device Tree

typedef struct fdt_header {
    // *Big Endian* 0xd00dfeed
    u32 magic;
    u32 totalsize;
    /// Structure Block byte offset
    /// Consists of "tokens", 32-bit *Big Endian* integers (@see FDT_* macros
    /// for values). Some tokens are followed by additional binary data.
    u32 off_dt_struct;
    /// Strings Block byte offset
    /// No alignment requirements.
    /// Contiguous null-terminated strings.
    u32 off_dt_strings;
    /// Memory Reservation Block byte offset
    /// Memory Reservation Block
    /// Memory Reservation Block Entry: fdt_reserve_entry
    /// List of pairs of 64-bit *Big Endian* integers.
    /// Described regions do not overlap.
    /// Zero-terminated (both integers of pair must be zero)
    u32 off_mem_rsvmap;
    /// Device Tree Binary Specification Version
    u32 version;
    /// Minimum Device Tree Binary Specification Version Required
    u32 last_comp_version;
    u32 boot_cpuid_phys;
    /// Strings Block byte size
    u32 size_dt_strings;
    /// Structure Block byte size
    u32 size_dt_struct;
} fdt_header;

/// Memory Reservation Block Entry
typedef struct fdt_reserve_entry {
    u64 address;
    u64 size;
} fdt_reserve_entry;

typedef struct fdt_property_header {
    /// length of property in bytes, which may be zero
    u32 length;
    /// byte offset into strings block to null-terminated name string.
    u32 name_offset;
} fdt_property_header;

/// NOTE: Structure Block Entry Values
/// Structure Block is sequence of 32-bit Big Endian integers known as
/// "tokens".
/// The structure block is composed of a sequence of pieces, each beginning
/// with a token, that is, a big-endian 32-bit integer. Some tokens are
/// followed by extra data, the format of which is determined by the token
/// value. All tokens shall be aligned on a 32-bit boundary, which may
/// require padding bytes (with a value of 0x0) to be inserted after the
/// previous token’s data.
///
/// Node Layout:
/// - (optionally) any number of FDT_NOP tokens
/// - FDT_BEGIN_NODE token
/// - The node’s name as a null-terminated string
/// - [zeroed padding bytes to align to a 4-byte boundary]
/// - For each property of the node:
///   - (optionally) any number of FDT_NOP tokens
///   - FDT_PROP token
///   - property header + (optionally) binary information
///   - [zeroed padding bytes to align to a 4-byte boundary]
///   - Representations of all child nodes in this format
///   - (optionally) any number of FDT_NOP tokens
/// - FDT_END_NODE token

/// Null-terminated name string directly following
#define FDT_BEGIN_NODE 1
/// No more properties or children for this node! Must follow a preceding,
/// matching FDT_BEGIN_NODE token.
#define FDT_END_NODE 2
/// fdt_property_header directly following, possibly information described
/// by header following that.
#define FDT_PROP 3
/// Ignore it
#define FDT_NOP 4
/// Structure Block complete
#define FDT_END 9

#endif /* LENSOROS_DEVICE_TREE_H */
