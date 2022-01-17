#include "basic_renderer.h"

// Define global renderer for use anywhere within the kernel.
BasicRenderer gRend;

// Carriage return ('\r')
void BasicRenderer::cret() {
	DrawPos = {
		0,
		DrawPos.y
	};
}
// Newline ('\n') or LineFeed (LF)
void BasicRenderer::newl() {
	DrawPos = {
		DrawPos.x,
		DrawPos.y + Font->PSF1_Header->CharacterSize
	};
}
// Carriage return line feed; CRLF ('\r' + '\n')
void BasicRenderer::crlf() {
	DrawPos = {
		0,
		DrawPos.y + Font->PSF1_Header->CharacterSize
	};
}
// Carriage return line feed; CRLF ('\r' + '\n')
//   but move position on newline by offset characters to the right.
void BasicRenderer::crlf(uint16_t offset) {
	DrawPos = {
		offset * 8,
		DrawPos.y + Font->PSF1_Header->CharacterSize
	};
}

void BasicRenderer::drawchar(char c, unsigned int color) {
	if (DrawPos.y < 0
		|| DrawPos.y + Font->PSF1_Header->CharacterSize > Target->PixelHeight)
	{
		DrawPos.y = 0;
	}
	if (DrawPos.x < 0
		|| DrawPos.x + 8 > Target->PixelWidth)
	{
		DrawPos.x = 0;
	}
	unsigned int* pixel_ptr = (unsigned int*)Target->BaseAddress;
	char* font_ptr = (char*)Font->GlyphBuffer + (c * Font->PSF1_Header->CharacterSize);
	for (unsigned long y = DrawPos.y; y < DrawPos.y + Font->PSF1_Header->CharacterSize; y++) {
		for (unsigned long x = DrawPos.x; x < DrawPos.x + 8; x++) {
			if ((*font_ptr & (0b10000000 >> (x - DrawPos.x))) > 0) {
				*(unsigned int*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = color;
			}
		}
		font_ptr++;
	}
}

void BasicRenderer::drawrect(Vector2 size, unsigned int color) {
	unsigned int* pixel_ptr = (unsigned int*)Target->BaseAddress;
	for (unsigned long y = DrawPos.y; y < DrawPos.y + size.y; y++) {
		for (unsigned long x = DrawPos.x; x < DrawPos.x + size.x; x++) {
			*(unsigned int*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = color;
		}
	}
}

void BasicRenderer::putchar(char c, unsigned int color)
{
    gRend.drawchar(c, color);
	// Increment pixel position horizontally by one character.
	DrawPos.x += 8;
	// Newline if next character would be off-screen.
	if (DrawPos.x + 8 > Target->PixelWidth) {
		crlf();
	}
}

void BasicRenderer::clearchar() {
	// Decrement pixel position horizontally by one character.
	if (DrawPos.x >= 8) {
		DrawPos.x -= 8;
	}
	else {
		DrawPos.x = Target->PixelWidth;
		DrawPos.y -= Font->PSF1_Header->CharacterSize;
	}
	if (DrawPos.y < 0) { DrawPos.y = 0; }
	unsigned int* pixel_ptr = (unsigned int*)Target->BaseAddress;
	for (unsigned long y = DrawPos.y; y < DrawPos.y + Font->PSF1_Header->CharacterSize; y++) {
		for (unsigned long x = DrawPos.x; x < DrawPos.x + 8; x++) {
			*(unsigned int*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = BackgroundColor;
		}
	}
}

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
