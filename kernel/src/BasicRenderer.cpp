#include "BasicRenderer.h"

void BasicRenderer::putchar(char c, unsigned int color)
{
	unsigned int* pixel_ptr = (unsigned int*)framebuffer->BaseAddress;
	char* font_ptr = (char*)Font->GlyphBuffer + (c * Font->PSF1_Header->CharacterSize);
	// This assumes each character in font is 8x16 pixels
	for (unsigned long y = PixelPosition.y; y < PixelPosition.y + Font->PSF1_Header->CharacterSize; y++) {
		for (unsigned long x = PixelPosition.x; x < PixelPosition.x + 8; x++) {
			if ((*font_ptr & (0b10000000 >> (x - PixelPosition.x))) > 0) {
				*(unsigned int*)(pixel_ptr + x + (y * framebuffer->PixelsPerScanLine)) = color;
			}
		}
		font_ptr++;
	}
}

void BasicRenderer::putstr(const char* str, unsigned int color) {
	// Set current character to first character in string.
	char* c = (char*)str;
	// Loop over string until null-terminator.
	while (*c != 0) {
		putchar(*c, color);
	    PixelPosition.x += 8;
		if (PixelPosition.x + 8 > framebuffer->PixelWidth) {
			PixelPosition.x = 0;
			PixelPosition.y += Font->PSF1_Header->CharacterSize;
		}
		c++;
	}
}
