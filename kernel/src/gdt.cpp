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

#include <gdt.h>

GDT gGDT;
GDTDescriptor gGDTD;

void setup_gdt() {
    //                  BASE  LIMIT       ACCESS       FLAGS:LIMIT1
    gGDT.Null =      {  0,    0,          0x00,        0x00        };
    gGDT.Ring0Code = {  0,    0xffffffff, 0b10011010,  0b10110000  };
    gGDT.Ring0Data = {  0,    0xffffffff, 0b10010010,  0b10110000  };
    gGDT.Ring3Code = {  0,    0xffffffff, 0b11111010,  0b10110000  };
    gGDT.Ring3Data = {  0,    0xffffffff, 0b11110010,  0b10110000  };
    gGDT.TSS =       {{ 0,    0xffffffff, 0b10001001,  0b00100000 }};
}
