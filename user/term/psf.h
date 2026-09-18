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

#ifndef PSF_H
#define PSF_H

#include <ints.h>

#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04
typedef struct PSF1_HEADER {
    // Magic bytes to indicate PSF1 font type
    u8 magic[2];
    u8 mode;
    u8 character_size;
} PSF1_HEADER;
typedef struct PSF1_FONT {
    PSF1_HEADER header;
    void* glyph_buffer;
} PSF1_FONT;

void psf1_delete(const PSF1_FONT font);
u8 psf1_width(const PSF1_FONT font);
u8 psf1_height(const PSF1_FONT font);
u8* psf1_char_bitmap(const PSF1_FONT font, const u8 c);

#endif /* PSF_H */
