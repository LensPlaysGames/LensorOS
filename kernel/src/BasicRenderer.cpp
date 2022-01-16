#include "BasicRenderer.h"

void BasicRenderer::clear() {
	// Draw background color to every pixel.
	unsigned int* pixel_ptr = (unsigned int*)framebuffer->BaseAddress;
	for (unsigned long y = 0; y < framebuffer->PixelHeight; y++) {
		for (unsigned long x = 0; x < framebuffer->PixelWidth; x++) {
			*(unsigned int*)(pixel_ptr + x + (y * framebuffer->PixelsPerScanLine)) = BackgroundColor;
		}
	}
	// Reset pixel position to origin.
	PixelPosition = {0, 0};
}

// Carriage return ('\r')
void BasicRenderer::cret() {
	PixelPosition = {
		0,
		PixelPosition.y
	};
}
// Newline ('\n') or LineFeed (LF)
void BasicRenderer::newl() {
	PixelPosition = {
		PixelPosition.x,
		PixelPosition.y + Font->PSF1_Header->CharacterSize
	};
}
// Carriage return line feed; CRLF ('\r' + '\n')
void BasicRenderer::crlf() {
	PixelPosition = {
		0,
		PixelPosition.y + Font->PSF1_Header->CharacterSize
	};
}

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
		// put current character of string at current pixel position.
		putchar(*c, color);
		// Increment pixel position horizontally by one character.
	    PixelPosition.x += 8;
		if (PixelPosition.x + 8 > framebuffer->PixelWidth) {
			// Next character would be off-screen, wrap to start of next line
			crlf();
		}
		c++;
	}
}

void BasicRenderer::putrect(Vector2 size, unsigned int color) {
	unsigned int* pixel_ptr = (unsigned int*)framebuffer->BaseAddress;
	for (unsigned long y = PixelPosition.y; y < PixelPosition.y + size.y; y++) {
		for (unsigned long x = PixelPosition.x; x < PixelPosition.x + size.x; x++) {
			*(unsigned int*)(pixel_ptr + x + (y * framebuffer->PixelsPerScanLine)) = color;
		}
	}
}
