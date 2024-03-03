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

void append_codepoint_as_utf8(std::string &appendee, u32 codepoint) {
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
        } else {
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
