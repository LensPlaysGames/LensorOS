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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#include <utf.h>

void append_codepoint_as_utf8(std::string& appendee, u32 codepoint) {
    if (codepoint <= 0x7f) {
        appendee += (char)(codepoint);
        return;
    }
    if (codepoint <= 0x07ff) {
        appendee += (char)(0b11000000 | (codepoint >> 6));
        appendee += (char)(0b10000000 | (codepoint & 0b111111));
        return;
    }
    if (codepoint <= 0xffff) {
        appendee += (char)(0b11100000 | (codepoint >> 12));
        appendee += (char)(0b10000000 | ((codepoint >> 6) & 0b111111));
        appendee += (char)(0b10000000 | (codepoint & 0b111111));
        return;
    }
    if (codepoint <= 0x10ffff) {
        appendee += (char)(0b11110000 | (codepoint >> 18));
        appendee += (char)(0b10000000 | ((codepoint >> 12) & 0b111111));
        appendee += (char)(0b10000000 | ((codepoint >> 6) & 0b111111));
        appendee += (char)(0b10000000 | (codepoint & 0b111111));
        return;
    }
    // Error: invalid codepoint/out of range, or something
    return;
}

// utf16 -> utf8
// Fairly lax, will ignore most invalid utf16 in a best effort approach.
auto utf16_to_utf8(std::string_view utf16) -> std::string {
    std::string out{};
    // Iterate over code units (NOT codepoints) of input utf16, 2 bytes at a time.
    for (usz i = 0; i + 1 < utf16.size(); i += 2) {
        // Get lower and upper bytes and then combine them to form the utf-16 code unit.
        u8 lower = utf16.data()[i];
        u8 upper = utf16.data()[i + 1];
        u16 code_unit = u16(lower) | (u16(upper) << 8);
        // If the code unit is U+0000 to U+D7FF or U+E000 to U+FFFF, then it is
        // numerically equivalent to a codepoint.
        if (code_unit <= 0xd7ff or code_unit > 0xe000) {
            append_codepoint_as_utf8(out, code_unit);
        }
        else {
            // From U+D800 to U+DFFF is a part of a surrogate pair encoding U+100000 to U+10FFFF
            u16 high_surrogate = code_unit;
            u16 high_surrogate_translated = (high_surrogate - 0xd800) << 10;
            if (i + 3 >= utf16.size()) {
                // Weird unpaired surrogate at the end, just ignore it ig.
                return out;
            }
            u16 low_surrogate_lower = utf16.data()[i + 2];
            u16 low_surrogate_upper = utf16.data()[i + 3];
            u16 low_surrogate = u16(low_surrogate_lower) | (u16(low_surrogate_upper) << 8);
            if (low_surrogate < 0xd800 or low_surrogate > 0xdfff) {
                // Weird unpaired surrogate in the middle of things, just ignore it ig.
                continue;
            }
            u16 low_surrogate_translated = low_surrogate - 0xdc00;
            u32 codepoint = 0x100000 + high_surrogate_translated + low_surrogate_translated;
            append_codepoint_as_utf8(out, codepoint);
        }
    }
    return out;
}

auto utf8_to_utf16(std::string_view utf8) -> std::string {
    std::string out{};
    usz i = 0;

    while (i < utf8.size()) {
        u8 first_byte = utf8[i];
        u32 codepoint = 0;
        usz bytes_to_read = 0;

        // Determine the number of bytes for the current UTF-8 character
        if ((first_byte & 0x80) == 0) {
            codepoint = first_byte;
            bytes_to_read = 1;
        }
        else if ((first_byte & 0xe0) == 0xc0) {
            codepoint = first_byte & 0x1f;
            bytes_to_read = 2;
        }
        else if ((first_byte & 0xf0) == 0xe0) {
            codepoint = first_byte & 0x0f;
            bytes_to_read = 3;
        }
        else if ((first_byte & 0xf8) == 0xf0) {
            codepoint = first_byte & 0b111;
            bytes_to_read = 4;
        }
        else {
            // Invalid starting byte, skip it
            i += 1;
            continue;
        }

        // Check if the full character fits in the remaining input
        // Malformed/truncated trailing sequence
        if (i + bytes_to_read > utf8.size()) break;

        // Consume the continuation bytes
        bool valid_sequence = true;
        for (usz j = 1; j < bytes_to_read; ++j) {
            u8 next_byte = utf8[i + j];
            if ((next_byte & 0xc0) != 0x80) {
                valid_sequence = false;
                break;
            }
            codepoint = (codepoint << 6) | (next_byte & 0x3f);
        }

        // Skip the bad byte and try to recover
        if (not valid_sequence) {
            i += 1;
            continue;
        }

        i += bytes_to_read;

        // Helper lambda to append a 16-bit code unit as 2 bytes (Little-Endian)
        auto append_u16 = [&](u16 code_unit) {
            out += char(code_unit & 0xff);
            out += char((code_unit >> 8) & 0xff);
        };

        // Encode codepoint into UTF-16 code units
        if (codepoint <= 0xffff) {
            // Direct mapping
            append_u16(u16(codepoint));
        }
        else if (codepoint <= 0x10ffff) {
            // Encode as a surrogate pair
            codepoint -= 0x10000;
            auto high_surrogate = u16((codepoint >> 10) + 0xd800);
            auto low_surrogate = u16((codepoint & 0x3ff) + 0xdc00);

            append_u16(high_surrogate);
            append_u16(low_surrogate);
        }
        // Codepoints above 0x10ffff are invalid in standard UTF-8/UTF-16 and are
        // therefore ignored.
    }

    return out;
}
