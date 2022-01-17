#ifndef LENSOR_OS_BASIC_RENDERER_H
#define LENSOR_OS_BASIC_RENDERER_H

#include <stddef.h>
#include <stdint.h>
#include "math.h"

typedef struct {
	// Magic bytes to indicate PSF1 font type	
	unsigned char Magic[2];
	unsigned char Mode;
	unsigned char CharacterSize;
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

struct RenderObject {
	// pos = pixel offset within framebuffer.
	uint64_t posX  {0};
	uint64_t posY  {0};
	// size = pixel width, height.
	uint64_t sizeX {0};
	uint64_t sizeY {0};
	// pixels array = 2d array (index = x + y * sizeX)
	uint32_t* pixels;
};

class BasicRenderer {
	/* THE NEW RENDERER
	 *  I need a renderer that doesn't suck as bad.
	 *  I think I can do something about that.
	 *  First, move to list of objects that need to be drawn.
	 *  Draw them within the main kernel loop.
	 *  I guess I need a struct or something for something that can be drawn.
	 *  This could be a character, or it could be a rectangle.
	 *  I guess the only thing these things have in common is that they are an array of pixel color values.
	 *  I just need to store the size of the thing, the data of the thing, and then access data by size.
	 *  Renderer needs to ensure object doesn't exceed bounds (or if it does, it/part of it isn't drawn).
	 *  Renderer needs to have a way for kernel to easily spit out debug information.
	 *  I'm thinking one of the objects is a debug window that is always drawn on top and has functions to draw into it specifically or something?
	 */
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

	void putobj(RenderObject obj);

	// FIXME: Change from pixel position to cursor position
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
