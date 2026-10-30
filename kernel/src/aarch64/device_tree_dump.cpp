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

#include <aarch64/device_tree.h>
#include <aarch64/uart.h>
#include <cstr.h>
#include <endian.h>
#include <integers.h>
#include <memory/api.h>

#include <print>
#include <span>
#include <string>
#include <string_view>

enum class FDTTokenType {
    NONE = 0,
    BEGIN_NODE = FDT_BEGIN_NODE,
    END_NODE = FDT_END_NODE,
    PROPERTY = FDT_PROP,
    NOP = FDT_NOP,
    END = FDT_END,
};

struct fdt_stringlist {
    const char* const base;
    const usz size;

    const char* current{nullptr};
    usz offset{0};

    fdt_stringlist(const u8* base, usz size)
        : base((const char*)base), size(size) {}

    [[nodiscard]]
    std::string_view next_string() {
        if (offset >= size) return {};
        current = base + offset;
        const auto out = current;
        const auto length = cstring_length(current);
        offset += length;
        return std::string_view(out, length - 1);
    }
};

struct fdt_property_value {
    const char* name;
    const u8* data;
    u32 name_length;
    u32 data_byte_size;

    fdt_property_value() = delete;
    fdt_property_value(u32 data_size, const char* name, const u8* data)
        : name(name)
        , data(data)
        , name_length(cstring_length(name) - 1)
        , data_byte_size(data_size) {};

    std::string_view name_string() {
        return std::string_view(name, name_length);
    }

    /// <u32> property type
    u32 as_u32() const {
        return data_byte_size == sizeof(u32) ? big_endian_to_cpu(*(u32*)data) : 0;
    }
    u32 as_phandle() const { return as_u32(); }
    /// <u64> property type
    u32 as_u64() const {
        // TODO: does this have to be split into two u32 reads?
        return data_byte_size == sizeof(u64) ? big_endian_to_cpu(*(u64*)data) : 0;
    }
    /// <string> property type
    std::string_view as_string() const {
        return std::string_view((char*)data, data_byte_size);
    }
    /// <stringlist> property type
    fdt_stringlist as_stringlist() const {
        return fdt_stringlist(data, data_byte_size);
    }
    /// <prop-encoded-array> property type
    /// data format is specific to property
    std::span<u8> as_prop_encoded_array() const {
        return std::span((u8*)data, data_byte_size);
    }
};

struct fdt_token {
    FDTTokenType type{FDTTokenType::NONE};
    const char* name{nullptr};
    fdt_property_value property{0, nullptr, nullptr};
};

struct fdt_structure_parse_context {
    const u8* const structure{};
    const u8* const strings{};
    const usz structure_byte_size{};
    const usz strings_byte_size{};

    usz offset{};
    fdt_token current{};

    fdt_structure_parse_context() = delete;

    [[nodiscard]]
    fdt_structure_parse_context(u8* structure, u8* strings, usz structure_size, usz strings_size)
        : structure(structure)
        , strings(strings)
        , structure_byte_size(structure_size)
        , strings_byte_size(strings_size) {
        next_token();
    }

    void next_token() {
        current = {};

        if (offset + sizeof(u32) > structure_byte_size) {
            current.type = FDTTokenType::END;
            return;
        }

        while (true) {
            auto token = big_endian_to_cpu(*(u32*)(structure + offset));
            offset += sizeof(u32);  // eat token

            switch (token) {
                default:
                case FDT_END:
                    current.type = FDTTokenType::END;
                    return;

                case FDT_NOP:
                    continue;

                case FDT_BEGIN_NODE: {
                    current.type = FDTTokenType::BEGIN_NODE;
                    current.name = (char*)(structure + offset);
                    offset += cstring_length(current.name);
                    // 4-byte align
                    offset = (offset + 3) & ~usz(3);
                    return;
                }

                case FDT_END_NODE:
                    current.type = FDTTokenType::END_NODE;
                    return;

                case FDT_PROP: {
                    if (offset + sizeof(fdt_property_header) > structure_byte_size) {
                        current.type = FDTTokenType::END;
                        return;
                    }

                    auto* property
                        = (fdt_property_header*)(structure + offset);
                    offset += sizeof(fdt_property_header);

                    auto property_length = big_endian_to_cpu(property->length);
                    auto property_name_offset = big_endian_to_cpu(property->name_offset);

                    if (offset + property_length > structure_byte_size) {
                        current.type = FDTTokenType::END;
                        return;
                    }

                    auto* value = structure + offset;
                    offset += property_length;
                    offset = (offset + 3) & ~usz(3);  // 4-byte align

                    auto* name = "";
                    if (property_name_offset < strings_byte_size)
                        name = (const char*)(strings + property_name_offset);

                    current.type = FDTTokenType::PROPERTY;
                    current.property = fdt_property_value(
                        property_length,
                        name,
                        value);
                    return;
                }
            }
        }
    }
};

// goal is to find hardware information
// - memory nodes
// - reserved memory nodes
// - cpus node
// - aobus (always-on bus)
// - |-- uart_AO
// - `-- sysctrl_AO
// - timer
// - interrupt controller (gic)
// - network interface card (nic), "ethmac"
// (like uart device so we can setup printing, and dump the device tree)
void parse_device_tree(fdt_header* dt) {
    if (dt == nullptr) return;

    auto* base = (u8*)dt;
    auto* strings = base + big_endian_to_cpu(dt->off_dt_strings);
    auto* structure = base + big_endian_to_cpu(dt->off_dt_struct);
    auto structure_byte_size = big_endian_to_cpu(dt->size_dt_struct);
    auto strings_byte_size = big_endian_to_cpu(dt->size_dt_strings);

    auto parse_context = fdt_structure_parse_context(
        structure,
        strings,
        structure_byte_size,
        strings_byte_size);

    // every node gets a stackframe.
    //
    // when we get a "reg", we record the current node's address and size in
    // this stackframe.
    //
    // when we get a "ranges", we record the child nodes' address space
    // translation details in this stackframe.
    //
    // when we get a "#*-cells", we record the size for the child nodes' "reg"
    // decoding in this stackframe.
    //
    // whenever we want to actually know what a particular "reg" refers to, we
    // can walk the stack to calculate address ranges appropriately.
    //
    // For example, the aobus is @c8100000, and the UART is @4c0 within it.
    // If we just tried to use the "reg" from the UART, we'd be writing to the
    // WRONG memory address, by like a lot.
    //
    // By walking the frames, we would get the appropriate address @c81004c0.
    struct stackframe {
        // keep track of node's address
        uintptr_t address{0};
        uintptr_t size{0};

        // for adusting child addresses
        uintptr_t range_address{0};
        uintptr_t translate_address{0};
        uintptr_t range_size{0};

        // for decoding *child* "reg" entries
        uint address_cells{2};  // # of 4-byte cells
        uint size_cells{2};     // # of 4-byte cells

        bool has_range{false};
    };

    stackframe fdtstack[24]{};
    size_t current_fdtstack_index{0};

    bool within_uart{false};
    while ((int)parse_context.current.type != FDT_END) {
        if ((int)parse_context.current.type == FDT_BEGIN_NODE) {
            ++current_fdtstack_index;
            fdtstack[current_fdtstack_index] = {};
            within_uart = false;

            auto& new_frame = fdtstack[current_fdtstack_index];
            auto& parent_frame = fdtstack[current_fdtstack_index - 1];
            new_frame.address_cells = parent_frame.address_cells;
            new_frame.size_cells = parent_frame.size_cells;
        }

        else if ((int)parse_context.current.type == FDT_END_NODE) {
            if (within_uart) {
                uintptr_t uart_base_address = fdtstack[current_fdtstack_index].address;

                for (size_t i = current_fdtstack_index; i > 1; --i) {
                    auto& parent_frame = fdtstack[i - 1];
                    if (
                        parent_frame.has_range
                        and uart_base_address >= parent_frame.range_address
                        and uart_base_address < (parent_frame.range_address + parent_frame.range_size))
                        uart_base_address = (uart_base_address - parent_frame.translate_address) + parent_frame.range_address;
                }

                // NOTE: Right here is where we are failing. Basically, we need to map the
                // UART in virtual address space before attempting to use it...
                hang();

                // Map uart MMIO starting at
                //   uart_base_address until offset fdtstack[current_fdtstack_index].size
                Memory::memory_space_handle memory_handle{};
                asm volatile(
                    "mrs %0, ttbr1_el1\n\t"
                    : "=r"(memory_handle)
                    : /** (!) No Inputs (!) **/
                    : "memory");

                auto uart_physical = PhysicalAddress(uart_base_address);
                auto uart_virtual = PhysicalKernelAddress(uart_physical);

                Memory::map_region_mmio(
                    memory_handle,
                    uart_virtual,
                    uart_physical,
                    fdtstack[current_fdtstack_index].size);

                UART::Base = uart_virtual.address;
                UART::initialize();
                UART::out("\n\ndid it work?\n\n");
            }

            if (current_fdtstack_index > 0)
                --current_fdtstack_index;
        }

        else if ((int)parse_context.current.type == FDT_PROP) {
            const auto& property = parse_context.current.property;
            const auto property_name = parse_context.current.property.name_string();
            if (property_name == "compatible") {
                auto s_list = property.as_stringlist();
                std::string_view s;
                while ((s = s_list.next_string()).size()) {
                    if (s == "amlogic,meson-ao-uart" or s == "arm,pl011") {
                        // found amlogic uart! we need to figure out it's base address ('reg'
                        // property), and it's interrupt information.
                        // https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/amlogic/meson-gx.dtsi
                        // https://devicetree-specification.readthedocs.io/en/latest/chapter2-devicetree-basics.html#devicetree-structure-and-conventions
                        // file:///home/lens_r/Downloads/extensible-host-controler-interface-usb-xhci.pdf
                        within_uart = true;
                        break;
                    }
                }
            }
            else if (property_name == "reg") {
                stackframe& parent_frame = fdtstack[current_fdtstack_index - 1];
                stackframe& new_frame = fdtstack[current_fdtstack_index];

                auto* regdata = (uint32_t*)property.data;
                for (uint i = 0; i < parent_frame.address_cells; ++i) {
                    uint32_t cell = big_endian_to_cpu(regdata[i]);
                    new_frame.address = (((uintptr_t)new_frame.address) << 32) | cell;
                }
                regdata += parent_frame.address_cells;
                for (uint i = 0; i < parent_frame.size_cells; ++i) {
                    uint32_t cell = big_endian_to_cpu(regdata[i]);
                    new_frame.size = (((uintptr_t)new_frame.size) << 32) | cell;
                }
            }
            else if (property_name == "ranges") {
                auto& current_frame = fdtstack[current_fdtstack_index];
                current_frame.has_range = true;

                auto* rangesdata = (uint32_t*)property.data;
                for (uint i = 0; i < current_frame.address_cells; ++i) {
                    uint32_t cell = big_endian_to_cpu(rangesdata[i]);
                    current_frame.translate_address = (current_frame.translate_address << 32) | cell;
                }
                rangesdata += current_frame.address_cells;

                auto& parent_frame = fdtstack[current_fdtstack_index];
                for (uint i = 0; i < parent_frame.address_cells; ++i) {
                    uint32_t cell = big_endian_to_cpu(rangesdata[i]);
                    current_frame.range_address = (current_frame.range_address << 32) | cell;
                }
                rangesdata += parent_frame.address_cells;

                for (uint i = 0; i < current_frame.size_cells; ++i) {
                    uint32_t cell = big_endian_to_cpu(rangesdata[i]);
                    current_frame.range_size = (current_frame.range_size << 32) | cell;
                }
            }
            else if (property_name == "#address-cells") {
                fdtstack[current_fdtstack_index].address_cells = parse_context.current.property.as_u32();
            }
            else if (property_name == "#size-cells") {
                fdtstack[current_fdtstack_index].size_cells = parse_context.current.property.as_u32();
            }
        }

        parse_context.next_token();
    }
}

void dump_device_tree(fdt_header* dt) {
    if (dt == nullptr) return;

    auto* base = (u8*)dt;
    auto* strings = base + big_endian_to_cpu(dt->off_dt_strings);
    auto* structure = base + big_endian_to_cpu(dt->off_dt_struct);
    auto structure_byte_size = big_endian_to_cpu(dt->size_dt_struct);
    auto strings_byte_size = big_endian_to_cpu(dt->size_dt_strings);

    std::print("[DevTree]: dumping...\n");

    auto parse_context = fdt_structure_parse_context(
        structure,
        strings,
        structure_byte_size,
        strings_byte_size);

    std::string indent{};
    indent.reserve(64);

    while ((int)parse_context.current.type != FDT_END) {
        switch (parse_context.current.type) {
            case FDTTokenType::BEGIN_NODE: {
                std::print("{}node\n", indent);
                indent += "  ";
                std::print("{}name=\"{}\"\n", indent, parse_context.current.name);
            } break;
            case FDTTokenType::END_NODE: {
                if (indent.size() >= 2)
                    indent.resize(indent.size() - 2);
            } break;
            case FDTTokenType::PROPERTY: {
                std::print("{}property\n", indent);
                indent += "  ";

                auto& property = parse_context.current.property;
                std::print("{}name=\"{}\"\n", indent, property.name);
                std::print("{}value=\"", indent);
                for (auto i = usz(0); i < property.data_byte_size; ++i)
                    std::print("  {:02x}", *(property.data + i));
                std::print("\"\n");

                indent.resize(indent.size() - 2);
            } break;

            case FDTTokenType::NOP:
            case FDTTokenType::NONE:
            case FDTTokenType::END:
                break;
        }
        parse_context.next_token();
    }
}
