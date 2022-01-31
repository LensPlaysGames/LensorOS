#include "basic_renderer.h"

// Define global renderer for use anywhere within the kernel.
BasicRenderer gRend;

inline void BasicRenderer::clamp_draw_position() {
    if (DrawPos.x > Target->PixelWidth)  { DrawPos.x = Target->PixelWidth;  }
    if (DrawPos.y > Target->PixelHeight) { DrawPos.y = Target->PixelHeight; }
}

void BasicRenderer::swap(uVector2 position, uVector2 size) {
    // Only swap what is within the bounds of the framebuffer.
    if (position.x > Target->PixelWidth
        || position.y > Target->PixelHeight) { return; }
    // Ensure size doesn't over-run edge of framebuffer.
    u64 diffX = Target->PixelWidth - position.x;
    u64 diffY = Target->PixelHeight - position.y;
    if (diffX < size.x) { size.x = diffX; }
    if (diffY < size.y) { size.y = diffY; }
    // Calculate addresses.
    u64 offset = ((BytesPerPixel * position.x)
                  + (BytesPerPixel * position.y * Target->PixelsPerScanLine));
    u32* targetBaseAddress = (u32*)((u64)Target->BaseAddress + offset);
    u32* renderBaseAddress = (u32*)((u64)Render->BaseAddress + offset);
    // Copy rectangle line-by-line.
    u64 bytesPerLine = BytesPerPixel * size.x;
    for (u64 y = 0; y < size.y; ++y) {
        memcpy(targetBaseAddress, renderBaseAddress, bytesPerLine);
        targetBaseAddress += Target->PixelsPerScanLine;
        renderBaseAddress += Render->PixelsPerScanLine;
    }
}

// Carriage return ('\r')
void BasicRenderer::cret() {
    DrawPos = {
        0,
        DrawPos.y
    };
}
/// Newline ('\n') or LineFeed (LF)
void BasicRenderer::newl() {
    DrawPos = {
        DrawPos.x,
        DrawPos.y + Font->PSF1_Header->CharacterSize
    };
}
/// Carriage return line feed; CRLF ('\r' + '\n')
void BasicRenderer::crlf() {
    DrawPos = {
        0,
        DrawPos.y + Font->PSF1_Header->CharacterSize
    };
}

/// Carriage return, offset by given argument value pixels, then newline.
void BasicRenderer::crlf(u32 offset) {
    DrawPos = {
        offset,
        DrawPos.y + Font->PSF1_Header->CharacterSize
    };
}

void BasicRenderer::drawrect(uVector2 size, u32 color) {
    clamp_draw_position();
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

/// Read `size` of pixel framebuffer starting at `DrawPos` into `buffer`.
void BasicRenderer::readpix(uVector2 size, u32* buffer) {
    if (buffer == nullptr) { return; }
    clamp_draw_position();
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

/// Draw `size` of `pixels` linear buffer starting at `DrawPos`.
void BasicRenderer::drawpix(uVector2 size, u32* pixels) {
    if (pixels == nullptr) { return; }
    clamp_draw_position();
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

/// Draw `size` of a bitmap `bitmap`, using passed color `color`
///   where bitmap is `1` and `BackgroundColor` where it is `0`.
void BasicRenderer::drawbmp(uVector2 size, u8* bitmap, u32 color) {
    if (bitmap == nullptr) { return; }
    clamp_draw_position();
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

/// Draw `size` of a bitmap `bitmap`, using passed color `color` where bitmap is `1`.
void BasicRenderer::drawbmpover(uVector2 size, u8* bitmap, u32 color) {
    if (bitmap == nullptr) { return; }
    clamp_draw_position();
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

/// Draw a character at the current `DrawPos` using the renderer's bitmap font.
void BasicRenderer::drawchar(char c, u32 color) {
    // Draw character.
    drawbmp({8, Font->PSF1_Header->CharacterSize},
            (u8*)Font->GlyphBuffer + (c * Font->PSF1_Header->CharacterSize),
            color);
}

/// Draw a character using the renderer's bitmap font, then increment `DrawPos`
///   as such that another character would not overlap with the previous (ie. typing).
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

void BasicRenderer::clear(uVector2 position, uVector2 size) {
    // Only clear what is within the bounds of the framebuffer.
    if (position.x > Target->PixelWidth
        || position.y > Target->PixelHeight) { return; }
    // Ensure size doesn't over-run edge of framebuffer.
    u64 diffX = Target->PixelWidth - position.x;
    u64 diffY = Target->PixelHeight - position.y;
    if (diffX < size.x) { size.x = diffX; }
    if (diffY < size.y) { size.y = diffY; }
    // Calculate addresses.
    u32* renderBaseAddress = (u32*)((u64)Render->BaseAddress
                                    + (BytesPerPixel * position.x)
                                    + (BytesPerPixel * position.y * Render->PixelsPerScanLine));
    // Copy rectangle line-by-line.
    for (u64 y = 0; y < size.y; ++y) {
        for (u64 x = 0; x < size.x; ++x) {
            *(renderBaseAddress + x) = BackgroundColor;
    	}
		renderBaseAddress += Render->PixelsPerScanLine;
	}
}
void BasicRenderer::clear(uVector2 position, uVector2 size, u32 color) {
	BackgroundColor = color;
	clear(position, size);
}

/// Clear a single character to the background color behind the current `DrawPos`.
/// Effectively 'backspace'.
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

/// Put a string of characters `str` (must be null terminated) to the screen with color `color`.
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
