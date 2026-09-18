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

#include <psf.h>
#include <stdlib.h>

void psf1_delete(const PSF1_FONT font) {
  free(font.glyph_buffer);
}
u8 psf1_width(const PSF1_FONT font) {
  return 8;
}
u8 psf1_height(const PSF1_FONT font) {
  return font.header.character_size;
}
/// bitmap size is as follows: (8, font.header->character_size)
/// @return address of beginning of bitmap pertaining to given character.
u8* psf1_char_bitmap(const PSF1_FONT font, const u8 c) {
  return (u8*)font.glyph_buffer + (c * psf1_height(font));
}
