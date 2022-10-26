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

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)
SECTIONS
{
    KERNEL_PHYSICAL = 0x100000;
    KERNEL_VIRTUAL = 0xffffffff80000000;
    . = KERNEL_PHYSICAL + KERNEL_VIRTUAL;
    KERNEL_START = .;
    .text ALIGN (0x1000) : AT(ADDR(.text) - KERNEL_VIRTUAL)
    {
        TEXT_START = .;
        *(.text*)
        *(.gnu.linkonce.t*)
        TEXT_END = .;
    }

    .data ALIGN (0x1000) : AT(ADDR(.data) - KERNEL_VIRTUAL)
    {
        DATA_START = .;
        *(.data*)
        *(.gnu.linkonce.d*)
        DATA_END = .;
    }

    .rodata ALIGN (0x1000) : AT(ADDR(.rodata) - KERNEL_VIRTUAL)
    {
        READ_ONLY_DATA_START = .;
        *(.rodata*)
        *(.gnu.linkonce.r*)
        READ_ONLY_DATA_END = .;
    }

    .bss ALIGN (0x1000) : AT(ADDR(.bss) - KERNEL_VIRTUAL)
    {
        BLOCK_STARTING_SYMBOLS_START = .;
        *(COMMON)
        *(.bss*)
        *(.gnu.linkonce.b*)
        BLOCK_STARTING_SYMBOLS_END = .;
    }
    KERNEL_END = .;

    /DISCARD/ :
    {
        *(.ctors)
        *(.dtors)
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
