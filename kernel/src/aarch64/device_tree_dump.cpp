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
        const auto length = strlen(current);
        offset += length + 1;
        return std::string_view(out, length);
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
        , name_length(strlen(name))
        , data_byte_size(data_size) {};

    /// <u32> property type
    u32 as_u32() {
        return data_byte_size == sizeof(u32) ? big_endian_to_cpu(*(u32*)data) : 0;
    }
    u32 as_phandle() { return as_u32(); }
    /// <u64> property type
    u32 as_u64() {
        return data_byte_size == sizeof(u64) ? big_endian_to_cpu(*(u64*)data) : 0;
    }
    /// <string> property type
    std::string_view as_string() {
        return std::string_view((char*)data, data_byte_size);
    }
    /// <stringlist> property type
    fdt_stringlist as_stringlist() {
        return fdt_stringlist(data, data_byte_size);
    }
    /// <prop-encoded-array> property type
    /// data format is specific to property
    std::span<u8> as_prop_encoded_array() {
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
        if (offset >= structure_byte_size) {
            current.type = FDTTokenType::END;
            return;
        }

        auto token = big_endian_to_cpu(*(u32*)(structure + offset));
        offset += sizeof(u32);  // eat token

        switch (token) {
            default:
            case FDT_END:
                current.type = FDTTokenType::END;
                return;

            case FDT_NOP:
                next_token();
                return;

            case FDT_BEGIN_NODE: {
                current.type = FDTTokenType::BEGIN_NODE;
                current.name = big_endian_to_cpu((char*)(structure + offset));
                offset += strlen(current.name) + 1;
                // 4-byte align
                offset = (offset + 3) & ~usz(3);
            } break;

            case FDT_END_NODE:
                current.type = FDTTokenType::END_NODE;
                break;

            case FDT_PROP: {
                current.type = FDTTokenType::PROPERTY;
                auto* property
                    = (fdt_property_header*)(structure + offset);
                offset += sizeof(fdt_property_header);
                auto* value = structure + offset;
                auto property_length = big_endian_to_cpu(property->length);
                offset += property_length;
                // 4-byte align
                offset = (offset + 3) & ~usz(3);
                auto* name
                    = (const char*)(strings + big_endian_to_cpu(property->name_offset));

                current.property = fdt_property_value(
                    property_length,
                    name,
                    value);
            } break;
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
    auto* base = (u8*)dt;
    auto* strings = base + big_endian_to_cpu(dt->off_dt_strings);
    auto* structure = base + big_endian_to_cpu(dt->off_dt_struct);
    auto structure_byte_size = big_endian_to_cpu(dt->size_dt_struct);
    auto strings_byte_size = big_endian_to_cpu(dt->size_dt_strings);

    hang();

    auto parse_context = fdt_structure_parse_context(
        structure,
        strings,
        structure_byte_size,
        strings_byte_size);

    hang();

    // we get a "reg", we record the address in a stack.
    // whenever we want to actually know what a particular "reg" refers to, we
    // can walk the stack to calculate address ranges appropriately.
    // For example, the aobus is @c8100000, and the UART is @4c0 within it.
    // If we just tried to use the "reg" from the UART, we'd be writing to the
    // WRONG memory address, by like a lot.
    struct stackframe {
        uintptr_t address{0};
        uintptr_t size{0};
        // for decoding next "reg" entries
        uint16_t address_cells{2};  // 4-byte cells
        uint16_t size_cells{2};     // 4-byte cells
    };
    stackframe fdtstack[24]{};
    size_t current_fdtstack_index{0};

    bool within_uart{false};
    while ((int)parse_context.current.type != FDT_END) {
        if ((int)parse_context.current.type == FDT_BEGIN_NODE) {
            fdtstack[current_fdtstack_index++] = {};
            within_uart = false;
        }

        else if ((int)parse_context.current.type == FDT_END_NODE) {
            if (within_uart) {
                uintptr_t uart_base_address{};
                for (size_t i = current_fdtstack_index; i; --i)
                    uart_base_address += fdtstack[i].address;

                UART::Base = uart_base_address;
                UART::initialize();
                UART::out("\n\ndid it work?\n\n");
            }

            --current_fdtstack_index;
        }

        else if ((int)parse_context.current.type == FDT_PROP) {
            constexpr std::string_view reg_string = "reg";
            constexpr std::string_view reg_address_cells_string = "#address-cells";
            constexpr std::string_view reg_size_cells_string = "#size-cells";
            constexpr std::string_view compat_string = "compatible";
            if (parse_context.current.property.name == compat_string) {
                auto s_list = parse_context.current.property.as_stringlist();
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
            else if (parse_context.current.property.name == reg_string) {
                stackframe& newframe = fdtstack[current_fdtstack_index];

                size_t shift{0};
                auto* regdata = (uint32_t*)parse_context.current.property.data;
                for (uint16_t i = 0; i < newframe.address_cells; ++i) {
                    newframe.address |= ((uintptr_t)big_endian_to_cpu(regdata[i])) << shift;
                    shift += 32;
                }
                regdata += newframe.address_cells;
                for (uint16_t i = 0; i < newframe.size_cells; ++i) {
                    newframe.size |= ((uintptr_t)big_endian_to_cpu(regdata[i])) << shift;
                    shift += 32;
                }
            }
            else if (parse_context.current.property.name == reg_address_cells_string) {
                fdtstack[current_fdtstack_index].address_cells = parse_context.current.property.as_u32();
            }
            else if (parse_context.current.property.name == reg_size_cells_string) {
                fdtstack[current_fdtstack_index].size_cells = parse_context.current.property.as_u32();
            }
        }

        parse_context.next_token();
    }
}

void dump_device_tree(fdt_header* dt) {
    auto* base = (u8*)dt;
    auto* strings = base + big_endian_to_cpu(dt->off_dt_strings);
    auto* structure = base + big_endian_to_cpu(dt->off_dt_struct);
    auto structure_byte_size = big_endian_to_cpu(dt->size_dt_struct);

    std::print("[DevTree]: dumping...\n");

    std::string indent{};
    indent.reserve(64);

    bool got_end{false};
    for (
        auto structure_offset = usz(0);
        structure_offset < structure_byte_size and (not got_end);
        /** (!) No Increment (!) **/) {
        // handle padding possibly inserted for 4-byte token alignment.
        structure_offset += 4 - (structure_offset % 4);

        auto token = *(u32*)(structure + structure_offset);
        structure_offset += sizeof(u32);  // eat token

        switch (token) {
            case FDT_BEGIN_NODE: {
                std::print("{}node\n", indent);
                indent += "  ";
                auto* name = (const char*)(structure + structure_offset);
                structure_offset += strlen(name) + 1;
                // 4-byte align
                structure_offset = (structure_offset + 3) & ~usz(3);
                std::print("{}name=\"{}\"\n", indent, name);
            } break;
            case FDT_END_NODE: {
                if (indent.size() >= 2)
                    indent.resize(indent.size() - 2);
            } break;

            case FDT_PROP: {
                std::print("{}property\n", indent);
                indent += "  ";

                auto* property
                    = (fdt_property_header*)(structure + structure_offset);
                structure_offset += sizeof(fdt_property_header);
                auto* value = structure + structure_offset;
                auto property_length = big_endian_to_cpu(property->length);
                structure_offset += property_length;
                // 4-byte align
                structure_offset = (structure_offset + 3) & ~usz(3);

                auto* name
                    = (const char*)(strings + big_endian_to_cpu(property->name_offset));

                std::print("{}name=\"{}\"\n", indent, name);
                std::print("{}value=\"", indent);
                for (auto i = usz(0); i < property_length; ++i)
                    std::print("  {:02x}", *(value + i));
                std::print("\"\n");

                indent.resize(indent.size() - 2);
            } break;

            case FDT_END: {
                got_end = true;
            } break;

            case FDT_NOP:
                break;

            default:
                std::print("{}[DevTree]:ERROR: unknown token type {:#x}\n", indent, token);
                return;
        }
    }
}
