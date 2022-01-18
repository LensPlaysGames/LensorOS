#ifndef LENSOR_OS_BASIC_RENDERER_H
#define LENSOR_OS_BASIC_RENDERER_H

#include <stddef.h>
#include <stdint.h>
#include "math.h"
#include "cstr.h"

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
	Vector2      DrawPos         {0, 0};
	// I = ignore                 0xIIRRGGBB
	unsigned int BackgroundColor {0x00000000};

	BasicRenderer() {}
	BasicRenderer(Framebuffer* fbuffer, PSF1_FONT* f) {
		Target = fbuffer;
		Font = f;
	}

	// Change every pixel in the framebuffer to BackgroundColor.
	void clear() {
		// Draw background color to every pixel.
		unsigned int* pixel_ptr = (unsigned int*)Target->BaseAddress;
		for (unsigned long y = 0; y < Target->PixelHeight; y++) {
			for (unsigned long x = 0; x < Target->PixelWidth; x++) {
				*(unsigned int*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = BackgroundColor;
			}
		}
		// Re-initialize draw position.
		DrawPos = {0, 0};
	}
	// Update BackgroundColor to given color, then clear screen.
	void clear(unsigned int color) {
		BackgroundColor = color;
		clear();
	}
	// Remove a single character behind DrawPos.
	void clearchar();
	
	// '\r'
	void cret();
	// '\n'
	void newl();
	// '\r' + '\n'
	void crlf();
	// '\r' then add offset + '\n'
	void crlf(unsigned int offset);

	// Draw `size` of rectangle as `color`.
	void drawrect(Vector2 size, unsigned int color = 0xffffffff);
	// Draw `size` of `bitmap` as `color`.
	void drawbmp(Vector2 size, uint8_t* bitmap, unsigned int color = 0xffffffff);
	// Use PSF1 bitmap font to draw a character to the screen (don't advance).
	void drawchar(char c, unsigned int color = 0xffffffff);
	
	// Use font to put a character to the screen (advance draw position).
	void putchar(char c, unsigned int color = 0xffffffff);
	// Put a null-terminated string of characters to the screen, wrapping if necessary.
	void putstr(const char* str, unsigned int color = 0xffffffff);
};

extern BasicRenderer gRend;

#endif
