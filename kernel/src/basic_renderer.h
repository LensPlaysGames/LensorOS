/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#ifndef LENSOR_OS_BASIC_RENDERER_H
#define LENSOR_OS_BASIC_RENDERER_H

#include <integers.h>
#include <math.h>
#include <memory.h>

struct PSF1_HEADER {
    // Magic bytes to indicate PSF1 font type
    u8 Magic[2];
    u8 Mode;
    u8 CharacterSize;
};

struct PSF1_FONT {
    PSF1_HEADER* PSF1_Header;
    void* GlyphBuffer;
};

struct Framebuffer {
    void* BaseAddress;
    u64 BufferSize;
    u32 PixelWidth;
    u32 PixelHeight;
    u32 PixelsPerScanLine;

    Framebuffer() {
        BaseAddress = nullptr;
        BufferSize = 0;
        PixelWidth = 0;
        PixelHeight = 0;
        PixelsPerScanLine = 0;
    }
};

constexpr u32 BytesPerPixel = 4;

class BasicRenderer {
public:
    // Target = framebuffer to draw to in memory
    Framebuffer* Render          {nullptr};
    Framebuffer* Target          {nullptr};
    PSF1_FONT*   Font            {nullptr};
    u32 BackgroundColor {0x00000000};

    BasicRenderer() {}
    BasicRenderer(Framebuffer* render, PSF1_FONT* f);

    /// ENSURE DRAW POSITION IS WITHIN FRAMEBUFFER.
    /// Used by renderer internally to ensure valid arguments.
    void clamp_draw_position(Vector2<u64>& position);

    /// UPDATE MEMORY CONTENTS OF RENDER FROM TARGET
    void swap() {
        memcpy(Render->BaseAddress, Target->BaseAddress, Target->BufferSize);
    }

    /// UPDATE SIZE of MEMORY CONTENTS OF RENDER FROM TARGET AT POSITION
    void swap(Vector2<u64> position, Vector2<u64> size);

    void readpix(Vector2<u64>& position, Vector2<u64> size, u32* buffer);

    /// Change every pixel in the target framebuffer to BackgroundColor.
    void clear() {
        // TODO: Make this more efficient (don't loop over every pixel).
        // Draw background color to every pixel.
        u32* pixel_ptr = (u32*)Target->BaseAddress;
        for (u64 y = 0; y < Target->PixelHeight; y++) {
            for (u64 x = 0; x < Target->PixelWidth; x++) {
                *(u32*)(pixel_ptr + x + (y * Target->PixelsPerScanLine)) = BackgroundColor;
            }
        }
    }
    /// Update BackgroundColor to given color, then clear screen.
    void clear(u32 color) {
        BackgroundColor = color;
        clear();
    }

    void clear(Vector2<u64> position, Vector2<u64> size);
    void clear(Vector2<u64> position, Vector2<u64> size, u32 color);

    // Remove a single character behind position.
    void clearchar(Vector2<u64>& position);

    // '\r'
    void cret(Vector2<u64>& position);
    // '\n'
    void newl(Vector2<u64>& position);
    // '\r' + '\n'
    void crlf(Vector2<u64>& position);
    void crlf(Vector2<u64>& position, u32 offset);

    // Draw `size` of rectangle as `color`.
    void drawrect(Vector2<u64>& position, Vector2<u64> size, u32 color = 0xffffffff);
    // Draw `size` of `pixels` buffer into target framebuffer.
    void drawpix(Vector2<u64>& position, Vector2<u64> size, u32* pixels);
    // Draw `size` of `bitmap` as `color`.
    void drawbmp(Vector2<u64>& position, Vector2<u64> size, const u8* bitmap, u32 color = 0xffffffff);
    // Draw `size` of `bitmap` as `color`, but don't clear `0` to background color.
    // This allows the use of bitmaps acting on alpha as well as color.
    void drawbmpover(Vector2<u64>& position, Vector2<u64> size, const u8* bitmap, u32 color = 0xffffffff);
    // Use PSF1 bitmap font to draw a character to the screen (don't advance).
    void drawchar(Vector2<u64>& position, char c, u32 color = 0xffffffff);
    void drawcharover(Vector2<u64>& position, char c, u32 color = 0xffffffff);

    // Use font to put a character to the screen (advance draw position).
    void putchar(Vector2<u64>& position, char c, u32 color = 0xffffffff);
    // Put a null-terminated string of characters to the screen, wrapping if necessary.
    void puts(Vector2<u64>& position, const char* str, u32 color = 0xffffffff);
};

extern BasicRenderer gRend;

#endif
