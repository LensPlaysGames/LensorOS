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

#ifndef LENSOR_OS_BOOT_H
#define LENSOR_OS_BOOT_H

#include <acpi.h>
#include <basic_renderer.h>
#include <efi_memory.h>
#include <integers.h>

typedef struct {
    /// GOP Pointer (UEFI Graphics Output Protocol)
    /// Linear Pixel Framebuffer
    Framebuffer* framebuffer;

    ACPI::RSDP2* rsdp; /**< Firmware device description. **/
                       /**< RSDP, Device Tree Blob, etc. **/

    usz base;        /**< Base address of loaded kernel image. **/
    PSF1_FONT* font; /**< Base address of loaded PSF font. **/

    EFI_MEMORY_DESCRIPTOR* map;
    usz mapSize;
    usz mapDescSize;

} BootInfo;

#endif /* LENSOR_OS_BOOT_H */
