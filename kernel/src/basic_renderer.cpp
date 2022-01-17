#include "basic_renderer.h"

// Define global renderer for use anywhere within the kernel.
BasicRenderer gRend;

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

void BasicRenderer::clear(unsigned int color) {
	BackgroundColor = color;
	clear();
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
	if (PixelPosition.y + Font->PSF1_Header->CharacterSize > framebuffer->PixelHeight) {
		PixelPosition.y = 0;
	}
	if (PixelPosition.x + 8 > framebuffer->PixelWidth) {
		PixelPosition.x = 0;
	}
	unsigned int* pixel_ptr = (unsigned int*)framebuffer->BaseAddress;
	char* font_ptr = (char*)Font->GlyphBuffer + (c * Font->PSF1_Header->CharacterSize);
	for (unsigned long y = PixelPosition.y; y < PixelPosition.y + Font->PSF1_Header->CharacterSize; y++) {
		for (unsigned long x = PixelPosition.x; x < PixelPosition.x + 8; x++) {
			if ((*font_ptr & (0b10000000 >> (x - PixelPosition.x))) > 0) {
				*(unsigned int*)(pixel_ptr + x + (y * framebuffer->PixelsPerScanLine)) = color;
			}
		}
		font_ptr++;
	}
	// Increment pixel position horizontally by one character.
	PixelPosition.x += 8;
	// Newline if next character would be off-screen.
	if (PixelPosition.x + 8 > framebuffer->PixelWidth) {
		crlf();
	}
}


// FIXME FIXME FIXME
// THIS FUNCTION CAUSES A PAGE FAULT
// void BasicRenderer::clearchar() {
// 	// Decrement pixel position horizontally by one character.
// 	PixelPosition.x -= 8;
// 	if (PixelPosition.x < 0) {
// 	    PixelPosition.x = framebuffer->PixelWidth - 8;
// 		PixelPosition.y -= Font->PSF1_Header->CharacterSize;
// 	}
// 	if (PixelPosition.y < 0) { PixelPosition.y = 0; }
// 	unsigned int* pixel_ptr = (unsigned int*)framebuffer->BaseAddress;
// 	for (unsigned long y = PixelPosition.y; y < PixelPosition.y + Font->PSF1_Header->CharacterSize; y++) {
// 		for (unsigned long x = PixelPosition.x; x < PixelPosition.x + 8; x++) {
// 			*(unsigned int*)(pixel_ptr + x + (y * framebuffer->PixelsPerScanLine)) = BackgroundColor;
// 		}
// 	}
// }

void BasicRenderer::putstr(const char* str, unsigned int color) {
	// Set current character to first character in string.
	char* c = (char*)str;
	// Loop over string until null-terminator.
	while (*c != 0) {
		// put current character of string at current pixel position.
		putchar(*c, color);
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
