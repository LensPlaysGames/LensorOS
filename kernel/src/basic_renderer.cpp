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
void BasicRenderer::crlf(unsigned int offset) {
	DrawPos = {
		offset * 8,
		DrawPos.y + Font->PSF1_Header->CharacterSize
	};
}

#define threshold 0x88
#define ReadBufferSizeSmall 176401
uint8_t readbuffer[ReadBufferSizeSmall];
uint8_t* BasicRenderer::readframebuffersmall(Vector2 size) {
	// Function does not allow to read more than buffer allows (420x420 + 1 pixels)
	if (size.x > 420) { return nullptr; }
	if (size.y > 420) { return nullptr; }
	// CLEAR READ BUFFER.
	for (uint64_t i = 0; i < ReadBufferSizeSmall; i++) {
		readbuffer[i] &= 0b00000000;
	}
	// DO NOT READ BAD MEMORY.
	unsigned int diffX = Target->PixelWidth - DrawPos.x;
	unsigned int diffY = Target->PixelHeight - DrawPos.y;
	if (diffX < size.x) { size.x = diffX; }
	if (diffY < size.y) { size.y = diffY; }
	unsigned int* pixel_ptr = (unsigned int*)Target->BaseAddress;
	for (unsigned long y = DrawPos.y; y < DrawPos.y + size.y; y++) {
		for (unsigned long x = DrawPos.x; x < DrawPos.x + size.x; x++) {
			unsigned int color = *(unsigned int*)(pixel_ptr + x + (y * Target->PixelsPerScanLine));
			// color = 0xAARRGGBB
		    uint8_t brightness = *(uint8_t*)((((uint64_t)color >> 24) & 0xff));
			if (brightness > threshold) {
				readbuffer[(x + y * size.x) / 8] |= (0b10000000) >> (x % 8);
			}

		}
	}
	return &readbuffer[0];
}

void BasicRenderer::drawrect(Vector2 size, unsigned int color) {
	if (DrawPos.y < 0) {
		DrawPos.y = 0;
	}
	if (DrawPos.x < 0) {
		DrawPos.x = 0;
	}
	unsigned int diffX = Target->PixelWidth - DrawPos.x;
	unsigned int diffY = Target->PixelHeight - DrawPos.y;
	if (diffX < size.x) { size.x = diffX; }
	if (diffY < size.y) { size.y = diffY; }
	unsigned int* pixel_ptr = (unsigned int*)Target->BaseAddress;
	for (unsigned long y = DrawPos.y; y < DrawPos.y + size.y; y++) {
		for (unsigned long x = DrawPos.x; x < DrawPos.x + size.x; x++) {
			*(uint32_t*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = color;
		}
	}
}

void BasicRenderer::drawbmp(Vector2 size, uint8_t* bitmap, unsigned int color) {
	if (bitmap == nullptr) { return; }
	if (DrawPos.y < 0) {
		DrawPos.y = 0;
	}
	if (DrawPos.x < 0) {
		DrawPos.x = 0;
	}
	unsigned int initX = size.x;
	unsigned int diffX = Target->PixelWidth - DrawPos.x;
	unsigned int diffY = Target->PixelHeight - DrawPos.y;
	if (diffX < size.x) { size.x = diffX; }
	if (diffY < size.y) { size.y = diffY; }
	unsigned int* pixel_ptr = (unsigned int*)Target->BaseAddress;
	for (uint64_t y = DrawPos.y; y < DrawPos.y + size.y; y++) {
		for (uint64_t x = DrawPos.x; x < DrawPos.x + size.x; x++) {
			int byte = ((x - DrawPos.x) + ((y - DrawPos.y) * initX)) / 8;
			if ((bitmap[byte] & (0b10000000 >> ((x - DrawPos.x) % 8))) > 0) {
			    *(uint32_t*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = color;
			}
		}
	}
}

void BasicRenderer::drawchar(char c, unsigned int color) {
	if (DrawPos.x < 0
		|| DrawPos.x + 8 > Target->PixelWidth)
	{
		DrawPos.x = 0;
	}
	if (DrawPos.y < 0
		|| DrawPos.y + Font->PSF1_Header->CharacterSize > Target->PixelHeight)
	{
		DrawPos.y = 0;
	}
	drawbmp({8, Font->PSF1_Header->CharacterSize},
			(uint8_t*)Font->GlyphBuffer + (c * Font->PSF1_Header->CharacterSize),
			color);
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
	// Move up line if necessary.
	if (DrawPos.x < 8) {
		DrawPos.x = Target->PixelWidth;
		if (DrawPos.y >= Font->PSF1_Header->CharacterSize) {
			DrawPos.y -= Font->PSF1_Header->CharacterSize;
		}
		else {
		    DrawPos = {8, 0};
		}
	}
    DrawPos.x -= 8;
    drawrect({8, Font->PSF1_Header->CharacterSize}, BackgroundColor);
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
