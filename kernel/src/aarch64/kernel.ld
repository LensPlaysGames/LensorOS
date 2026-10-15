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

OUTPUT_FORMAT(elf64-aarch64)
ENTRY(_start)
SECTIONS
{
    KERNEL_PHYSICAL = 0x40000000;
    KERNEL_VIRTUAL = 0xffffffff80000000;
    . = KERNEL_VIRTUAL;
    KERNEL_START = .;
    KERNEL_PHYSICAL_CURSOR = ALIGN(KERNEL_PHYSICAL, 0x1000);

    .text ALIGN (0x1000) : AT(KERNEL_PHYSICAL_CURSOR)
    {
        TEXT_START = .;
        *(.text*)
        *(.gnu.linkonce.t*)
        TEXT_END = .;
    }
    KERNEL_PHYSICAL_CURSOR = ALIGN(KERNEL_PHYSICAL_CURSOR + SIZEOF(.text), 0x1000);

    .data ALIGN (0x1000) : AT(KERNEL_PHYSICAL_CURSOR)
    {
        DATA_START = .;
        *(.data*)
        *(.gnu.linkonce.d*)
        DATA_END = .;
    }
    KERNEL_PHYSICAL_CURSOR = ALIGN(KERNEL_PHYSICAL_CURSOR + SIZEOF(.data), 0x1000);

    .rodata ALIGN (0x1000) : AT(KERNEL_PHYSICAL_CURSOR)
    {
        READ_ONLY_DATA_START = .;
        *(.rodata*)
        *(.gnu.linkonce.r*)
        READ_ONLY_DATA_END = .;
    }
    KERNEL_PHYSICAL_CURSOR = ALIGN(KERNEL_PHYSICAL_CURSOR + SIZEOF(.rodata), 0x1000);

    .bss ALIGN (0x1000) : AT(KERNEL_PHYSICAL_CURSOR)
    {
        BLOCK_STARTING_SYMBOLS_START = .;
        *(COMMON)
        *(.bss*)
        *(.gnu.linkonce.b*)
        BLOCK_STARTING_SYMBOLS_END = .;
    }
    KERNEL_PHYSICAL_CURSOR = ALIGN(KERNEL_PHYSICAL_CURSOR + SIZEOF(.bss), 0x1000);

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

