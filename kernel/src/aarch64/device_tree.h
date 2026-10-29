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

/// Standard Properties
///
/// compatible  stringlist
///   This list of strings should be used by a client program for device
///   driver selection.
///   The recommended format is "manufacturer,model"
///
/// model  string
///   Manufacturer’s model number of the device.
///   The recommended format is: "manufacturer,model"
///
/// phandle  u32
///   Unique numerical identifier for node within device tree.
///
/// status  string
///   "okay"
///     Indicates the device is operational.
///   "disabled"
///     Indicates that the device is not presently operational, but it might
///     become operational in the future (for example, something is not plugged
///     in, or switched off).
///     Refer to the device binding for details on what disabled means for a
///     given device.
///   "reserved"
///     Indicates that the device is operational, but should not be used.
///     Typically this is used for devices that are controlled by another
///     software component, such as platform firmware.
///   "fail"
///     Indicates that the device is not operational. A serious error was
///     detected in the device, and it is unlikely to become operational
///     without repair.
///   "fail-sss"
///     Indicates that the device is not operational. A serious error was
///     detected in the device and it is unlikely to become operational without
///     repair. The sss portion of the value is specific to the device and
///     indicates the error condition detected.
///
/// virtual-reg  u32 or u64
///   Specifies an effective address that maps to the first physical address
///   specified in the reg property.
///
///
/// Miscellaneous:
///
/// clock-frequency  prop-encoded-array
///   Specifies the frequency of a clock in Hz.
///   Either
///   - a 32-bit integer consisting of one <u32> specifying the frequency
///   - a 64-bit integer represented as a <u64> specifying the frequency
///
/// label  string
///   Human readable string describing device.
///
/// current-speed  u32
///   Baud rate, effectively
///
///
/// For interrupt generating devices:
///
/// interrupts  prop-encoded-array
///   Defines the interrupt or interrupts that are generated by the device.
///
///   interrupts is overridden by the interrupts-extended property and
///   normally only one or the other should be used.
///
/// interrupt-controller  empty
///   Presence indicates this node defines an interrupt controller.
///
/// TODO: interrupt mapping

/// Root Node
///
/// path: "/"
///
/// #address-cells  u32
///   Number of u32 to represent address in 'reg' property in children of root.
///
/// #size-cells  u32
///   Number of u32 to represent size in 'reg' property in children of root.
///
/// model  string
///   Uniquely identifies "model of system board".
///
/// compatible  stringlist
///   Specifies list of "platform architectures" with which the platform
///   described by this device tree is compatible with
///
/// serial-number  string  *optional*
///   Device serial number.
///
/// chassis-type  string  *optional*
///   One of "desktop", "laptop", "convertible", "server", "tablet",
///   "handset", "watch", "embedded"

/// REQUIRED NODE
/// Memory Node
///
/// path: "/memory"
///
/// A memory device node is required for all devicetrees.
///
/// (!)============================================================(!)
/// When booting via UEFI, the system memory map is obtained via the
/// GetMemoryMap() UEFI boot time service, and if present, the OS must
/// ignore any /memory nodes.
/// (!)============================================================(!)
///
/// If a system has multiple ranges of memory, they may be specified via
/// multiple memory nodes or in the reg property of a single memory node.
///
/// device_type  string  Value shall be "memory"
///
/// reg prop-encoded-array
///   Consists of an arbitrary number of address and size pairs that specify
///   the physical address and size of the memory ranges.
///   This is where #address-cells and #size-cells comes into play.

/// REQUIRED NODE
/// CPUs Node
///
/// path: "/cpus"
///
/// A /cpus node is required for all devicetrees.
///
/// It does not represent a real device in the system, but acts as a
/// container for child cpu nodes which represent the systems CPUs.
///
/// May contain properties that are common across all cpu nodes.
///
/// NOTE: Hardware threads that share an MMU would generally be represented
/// under one cpu node.
///
/// CPUs and threads are numbered through a unified number-space that
/// should match as closely as possible the interrupt controller’s
/// numbering of CPUs/threads (but it is not required).
///
/// The node name for every CPU node should be "cpu".
///
/// device_type string
///   Value shall be "cpu".
///
/// reg  array
///   The value of reg is a <prop-encoded-array> that defines a unique CPU/
///   thread id for the CPU/threads represented by the CPU node.
///
///   If a CPU supports more than one thread (i.e. multiple streams of
///   execution) the reg property is an array with 1 element per thread.
///   The #address-cells on the /cpus node specifies how many cells each
///   element of the array takes. Software can determine the number of
///   threads by dividing the size of reg by the parent node’s #address-
///   cells.
///
///   If a CPU/thread can be the target of an external interrupt the reg
///   property value must be a unique CPU/thread id that is addressable by
///   the interrupt controller.
///
///   If a CPU/thread cannot be the target of an external interrupt, then reg
///   must be unique and out of bounds of the range addressed by the
///   interrupt controller
///
///   If a CPU/thread’s PIR (pending interrupt register) is modifiable, a
///   client program should modify PIR to match the reg property value. If
///   PIR cannot be modified and the PIR value is distinct from the interrupt
///   controller number space, the CPUs binding may define a binding-specific
///   representation of PIR values if desired.
///
/// clock-frequency  array
///   Specifies the current clock speed of the CPU in Hertz. The value is a <
///   prop-encoded-array> in one of two forms:
///   A 32-bit integer consisting of one <u32> specifying the frequency.
///   A 64-bit integer represented as a <u64> specifying the frequency.
///
/// timebase-frequency  array
///   Specifies the current frequency at which the timebase and decrementer
///   registers are updated (in Hertz). The value is a <prop-encoded-array>
///   in one of two forms:
///   A 32-bit integer consisting of one <u32> specifying the frequency.
///   A 64-bit integer represented as a <u64>.
///
/// status 	string
///   A standard property describing the state of a CPU. This property shall
///   be present for nodes representing CPUs in a symmetric multiprocessing
///   (SMP) configuration. For a CPU node the meaning of the "okay",
///   "disabled" and "fail" values are as follows:
///   "okay" : The CPU is running.
///   "disabled" : The CPU is in a quiescent state.
///   "fail" : The CPU is not operational or does not exist.
///
///   A quiescent CPU is in a state where it cannot interfere with the normal
///   operation of other CPUs, nor can its state be affected by the normal
///   operation of other running CPUs, except by an explicit method for
///   enabling or re-enabling the quiescent CPU (see the enable-method
///   property).
///   In particular, a running CPU shall be able to issue broadcast TLB
///   invalidates without affecting a quiescent CPU.
///   Examples: A quiescent CPU could be in a spin loop, held in reset, and
///   electrically isolated from the system bus or in another implementation
///   dependent state.
///
///   A CPU with "fail" status does not affect the system in any way. The
///   status is assigned to nodes for which no corresponding CPU exists.
///
/// enable-method  stringlist
///   Describes the method by which a CPU in a disabled state is enabled.
///   This property is required for CPUs with a status property with a value
///   of "disabled". The value consists of one or more strings that define
///   the method to release this CPU. If a client program recognizes any of
///   the methods, it may use it. The value shall be one of the following:
///   "spin-table" :
///       The CPU is enabled with the spin table method defined in the DTSpec.
///   "[vendor],[method]" :
///       Implementation dependent string that describes the method by which a
///       CPU is released from a "disabled" state. The required format is:
///       "[vendor],[method]", where vendor is a string describing the name of
///       the manufacturer and method is a string describing the vendor specific
///       mechanism.
///       Example: "fsl,MPC8572DS"
///
/// cpu-release-addr  u64
///   The cpu-release-addr property is required for cpu nodes that have an
///   enable-method property value of "spin-table". The value specifies the
///   physical address of a spin table entry that releases a secondary CPU
///   from its spin loop.
///
/// TODO: lots more cache, TLB, and Power ISA properties.

/// Reserved Memory Node
///
/// path: "/reserved-memory"
///
/// Each child of the reserved-memory node specifies one or more regions of
/// reserved memory. Each child node may either use a reg property to
/// specify a specific range of reserved memory, or a size property with
/// optional constraints to request a dynamically allocated block of
/// memory.
///
/// A reserved memory node requires either a reg property for static
/// allocations, or a size property for dynamics allocations. Dynamic
/// allocations may use alignment and alloc-ranges properties to constrain
/// where the memory is allocated from. If both reg and size are present,
/// then the region is treated as a static allocation with the reg property
/// taking precedence and size is ignored.
///
/// Reserved regions with the no-map property must be listed in the UEFI memory
/// map with type EfiReservedMemoryType. All other reserved regions must be
/// listed with type EfiBootServicesData.
///
/// reg  prop-encoded-array  *optional*
///   Consists of an arbitrary number of address and size pairs that specify
///   the physical address and size of the memory ranges.
///
/// size  prop-encoded-array  *optional*
///   Size in bytes of memory to reserve for dynamically allocated regions.
///   Size of this property is based on parent node’s #size-cells property.
///
/// alignment  prop-encoded-array  *optional*
///   Address boundary for alignment of allocation. Size of this property is
///   based on parent node’s #size-cells property.
///
/// alloc-ranges  prop-encoded-array  *optional*
///   Specifies regions of memory that are acceptable to allocate from.
///   Format is (address, length pairs) tuples in same format as for reg
///   properties.
///
/// compatible  stringlist  *optional*
///   May contain the following strings:
///   - shared-dma-pool: This indicates a region of memory meant to be used as
///     a shared pool of DMA buffers for a set of devices. It can be used by an
///     operating system to instantiate the necessary pool management subsystem
///     if necessary.
///   - vendor specific string in the form <vendor>,[<device>-]<usage>
///
/// no-map 	empty  *optional*
///   If present, indicates the operating system must not create a virtual
///   mapping of the region as part of its standard mapping of system memory,
///   nor permit speculative access to it under any circumstances other than
///   under the control of the device driver using the region.
///
/// reusable  empty  *optional*
///   The operating system can use the memory in this region with the
///   limitation that the device driver(s) owning the region need to be able
///   to reclaim it back. Typically that means that the operating system can
///   use that region to store volatile or cached data that can be otherwise
///   regenerated or migrated elsewhere.

/// Alias Node Properties
///
/// path: "/aliases"
///
/// Each child property defines an alias. The property name is the alias,
/// and the property value is the replacement. The alias will take on the
/// value of the replacement.
///
/// Aliases are limited to 1 to 31 characters in length, with only the
/// following characters being valid: 0-9, a-z (lowercase only), and '-'.

/// Chosen Node
///
/// path: "/chosen"
///
/// Does not represent a real device in the system but describes parameters
/// chosen or specified by the system firmware at run time.
///
/// bootargs  string  *optional*
///   A string that specifies the boot arguments for the client program. The
///   value could potentially be a null string if no boot arguments are
///   required.
///
/// stdout-path  string  *optional*
///   A string that specifies the full path to the node representing the
///   device to be used for boot console output. If the character “:” is
///   present in the value it terminates the path. The value may be an alias.
///   If the stdin-path property is not specified, stdout-path should be
///   assumed to define the input device.
///
/// stdin-path  string  *optional*
///   A string that specifies the full path to the node representing the
///   device to be used for boot console input. If the character “:” is
///   present in the value it terminates the path. The value may be an alias.

void parse_device_tree(fdt_header* dt);
void dump_device_tree(fdt_header* dt);

#endif /* LENSOROS_DEVICE_TREE_H */
