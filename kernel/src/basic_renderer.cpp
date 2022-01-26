#include "basic_renderer.h"

// Define global renderer for use anywhere within the kernel.
BasicRenderer gRend;

inline void BasicRenderer::ValidateDrawPos() {
	if (DrawPos.x < 0) { DrawPos.x = 0; }
	else if (DrawPos.x > Target->PixelWidth) { DrawPos.x = Target->PixelWidth; }
	if (DrawPos.y < 0) { DrawPos.y = 0; }
	else if (DrawPos.y > Target->PixelHeight) { DrawPos.y = Target->PixelHeight; }
}

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

// Carriage return offset by given argument value pixels, then newline.
void BasicRenderer::crlf(u32 offset) {
	DrawPos = {
		offset,
		DrawPos.y + Font->PSF1_Header->CharacterSize
	};
}

void BasicRenderer::drawrect(Vector2 size, u32 color) {
    ValidateDrawPos();
	u32 diffX = Target->PixelWidth - DrawPos.x;
	u32 diffY = Target->PixelHeight - DrawPos.y;
	if (diffX < size.x) { size.x = diffX; }
	if (diffY < size.y) { size.y = diffY; }
	u32* pixel_ptr = (u32*)Target->BaseAddress;
	for (u64 y = DrawPos.y; y < DrawPos.y + size.y; y++) {
		for (u64 x = DrawPos.x; x < DrawPos.x + size.x; x++) {
			*(u32*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = color;
		}
	}
}

void BasicRenderer::readpix(Vector2 size, u32* buffer) {
	if (buffer == nullptr) { return; }
	ValidateDrawPos();
	u32 initX = size.x;
	u32 diffX = Target->PixelWidth - DrawPos.x;
	u32 diffY = Target->PixelHeight - DrawPos.y;
	if (diffX < size.x) { size.x = diffX; }
	if (diffY < size.y) { size.y = diffY; }
	u32* pixel_ptr = (u32*)Target->BaseAddress;
	for (u64 y = DrawPos.y; y < DrawPos.y + size.y; y++) {
		for (u64 x = DrawPos.x; x < DrawPos.x + size.x; x++) {
			*(u32*)(buffer + (x - DrawPos.x) + ((y - DrawPos.y) * initX))
				= *(u32*)(pixel_ptr + x + (y * Target->PixelsPerScanLine));
		}
	}
}

void BasicRenderer::drawpix(Vector2 size, u32* pixels) {
	if (pixels == nullptr) { return; }
    ValidateDrawPos();
	u32 initX = size.x;
	u32 diffX = Target->PixelWidth - DrawPos.x;
	u32 diffY = Target->PixelHeight - DrawPos.y;
	if (diffX < size.x) { size.x = diffX; }
	if (diffY < size.y) { size.y = diffY; }
	u32* pixel_ptr = (u32*)Target->BaseAddress;
	for (u64 y = DrawPos.y; y < DrawPos.y + size.y; y++) {
		for (u64 x = DrawPos.x; x < DrawPos.x + size.x; x++) {
			*(u32*)(pixel_ptr + x + (y * Target->PixelsPerScanLine))
				= *(u32*)(pixels + (x - DrawPos.x) + ((y - DrawPos.y) * initX));
		}
	}
}

void BasicRenderer::drawbmp(Vector2 size, u8* bitmap, u32 color) {
	if (bitmap == nullptr) { return; }
	ValidateDrawPos();
	u32 initX = size.x;
	u32 diffX = Target->PixelWidth - DrawPos.x;
	u32 diffY = Target->PixelHeight - DrawPos.y;
	if (diffX < size.x) { size.x = diffX; }
	if (diffY < size.y) { size.y = diffY; }
	u32* pixel_ptr = (u32*)Target->BaseAddress;
	for (u64 y = DrawPos.y; y < DrawPos.y + size.y; y++) {
		for (u64 x = DrawPos.x; x < DrawPos.x + size.x; x++) {
			s32 byte = ((x - DrawPos.x) + ((y - DrawPos.y) * initX)) / 8;
			if ((bitmap[byte] & (0b10000000 >> ((x - DrawPos.x) % 8))) > 0) {
			    *(u32*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = color;
			}
			else {
				*(u32*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = BackgroundColor;
			}
		}
	}
}

void BasicRenderer::drawbmpover(Vector2 size, u8* bitmap, u32 color) {
	if (bitmap == nullptr) { return; }
	ValidateDrawPos();
	u32 initX = size.x;
	u32 diffX = Target->PixelWidth - DrawPos.x;
	u32 diffY = Target->PixelHeight - DrawPos.y;
	if (diffX < size.x) { size.x = diffX; }
	if (diffY < size.y) { size.y = diffY; }
	u32* pixel_ptr = (u32*)Target->BaseAddress;
	for (u64 y = DrawPos.y; y < DrawPos.y + size.y; y++) {
		for (u64 x = DrawPos.x; x < DrawPos.x + size.x; x++) {
			s32 byte = ((x - DrawPos.x) + ((y - DrawPos.y) * initX)) / 8;
			if ((bitmap[byte] & (0b10000000 >> ((x - DrawPos.x) % 8))) > 0) {
			    *(u32*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = color;
			}
		}
	}
}

void BasicRenderer::drawchar(char c, u32 color) {
	// Draw character.
	drawbmp({8, Font->PSF1_Header->CharacterSize},
			(u8*)Font->GlyphBuffer + (c * Font->PSF1_Header->CharacterSize),
			color);
}

void BasicRenderer::putchar(char c, u32 color)
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

void BasicRenderer::putstr(const char* str, u32 color) {
	// Set current character to first character in string.
	char* c = (char*)str;
	// Loop over string until null-terminator.
	while (*c != 0) {
		// put current character of string at current pixel position.
		putchar(*c, color);
		c++;
	}
}
