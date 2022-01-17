#ifndef LENSOR_OS_BASIC_RENDERER_H
#define LENSOR_OS_BASIC_RENDERER_H

#include <stddef.h>
#include <stdint.h>
#include "math.h"

typedef struct {
	// Magic bytes to indicate PSF1 font type	
	uint8_t Magic[2];
	uint8_t Mode;
	uint8_t CharacterSize;
} PSF1_HEADER;

typedef struct {
	PSF1_HEADER* PSF1_Header;
	void* GlyphBuffer;
} PSF1_FONT;

struct Framebuffer {
	void* BaseAddress;
	size_t BufferSize;
	unsigned int PixelWidth;
	unsigned int PixelHeight;
	unsigned int PixelsPerScanLine;
};

const unsigned int BytesPerPixel = 4;

class BasicRenderer {
public:
	Framebuffer* Target          {nullptr};
	PSF1_FONT*   Font            {nullptr};
	Vector2      PixelPosition   {0, 0};
	// I = ignore                 0xIIRRGGBB
	unsigned int BackgroundColor {0x00000000};

	BasicRenderer() {}

	BasicRenderer(Framebuffer* fbuffer, PSF1_FONT* f) {
		Target = fbuffer;
		Font = f;
	}

	// Change every pixel in the framebuffer to BackgroundColor.
	void clear();
	// Update BackgroundColor to given color, then clear screen.
	void clear(unsigned int color);

	// Remove a single character behind PixelPosition.
	void clearchar();
	
	// '\r'
	void cret();
	// '\n'
	void newl();
	// '\r' + '\n'
	void crlf();
	// Use font to put a character to the screen.
	void putchar(char c, unsigned int color = 0xffffffff);
	// Use font to put a character to the screen without advancing PixelPosition.
	void putcharover(char c, unsigned int color = 0xffffffff);
	// Put a string of characters to the screen, wrapping if necessary.
	void putstr(const char* str, unsigned int color = 0xffffffff);
	void putrect(Vector2 size, unsigned int color = 0xffffffff);
};

extern BasicRenderer gRend;

#endif
