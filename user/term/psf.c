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
