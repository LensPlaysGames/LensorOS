#include "basic_renderer.h"

// Define global renderer for use anywhere within the kernel.
BasicRenderer gRend;

void BasicRenderer::putobj(RenderObject obj) {
	unsigned int* pixel_ptr = (unsigned int*)Target->BaseAddress;
	for (unsigned long y = obj.posY; y < obj.posY + sizeY; y++) {
		for (unsigned long x = obj.posX; x < obj.posX + sizeX; x++) {
			*(unsigned int*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = obj.pixels[x + y * sizeX];
		}
	}
}

void BasicRenderer::clear() {
	// Draw background color to every pixel.
	unsigned int* pixel_ptr = (unsigned int*)Target->BaseAddress;
	for (unsigned long y = 0; y < Target->PixelHeight; y++) {
		for (unsigned long x = 0; x < Target->PixelWidth; x++) {
			*(unsigned int*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = BackgroundColor;
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
	if (PixelPosition.y < 0
		|| PixelPosition.y + Font->PSF1_Header->CharacterSize > Target->PixelHeight)
	{
		PixelPosition.y = 0;
	}
	if (PixelPosition.x < 0
		|| PixelPosition.x + 8 > Target->PixelWidth)
	{
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

void BasicRenderer::putcharover(char c, unsigned int color) {
	if (PixelPosition.y < 0
		|| PixelPosition.y + Font->PSF1_Header->CharacterSize > framebuffer->PixelHeight)
	{
		PixelPosition.y = 0;
	}
	if (PixelPosition.x < 0
		|| PixelPosition.x + 8 > framebuffer->PixelWidth)
	{
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
}

void BasicRenderer::clearchar() {
	// Decrement pixel position horizontally by one character.
	if (PixelPosition.x >= 8) {
		PixelPosition.x -= 8;
	}
	else {
		PixelPosition.x = framebuffer->PixelWidth;
		PixelPosition.y -= Font->PSF1_Header->CharacterSize;
	}
	if (PixelPosition.y < 0) { PixelPosition.y = 0; }
	unsigned int* pixel_ptr = (unsigned int*)framebuffer->BaseAddress;
	for (unsigned long y = PixelPosition.y; y < PixelPosition.y + Font->PSF1_Header->CharacterSize; y++) {
		for (unsigned long x = PixelPosition.x; x < PixelPosition.x + 8; x++) {
			*(unsigned int*)(pixel_ptr + x + (y * framebuffer->PixelsPerScanLine)) = BackgroundColor;
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

void BasicRenderer::putrect(Vector2 size, unsigned int color) {
	unsigned int* pixel_ptr = (unsigned int*)framebuffer->BaseAddress;
	for (unsigned long y = PixelPosition.y; y < PixelPosition.y + size.y; y++) {
		for (unsigned long x = PixelPosition.x; x < PixelPosition.x + size.x; x++) {
			*(unsigned int*)(pixel_ptr + x + (y * framebuffer->PixelsPerScanLine)) = color;
		}
	}
}
