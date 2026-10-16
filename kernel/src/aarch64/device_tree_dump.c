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
#include <cstr.h>
#include <endian.h>
#include <integers.h>

#include <print>
#include <string>

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
        structure_offset < structure_byte_size and(not got_end);
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
                std::print("{}value=\"{}", indent);
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
