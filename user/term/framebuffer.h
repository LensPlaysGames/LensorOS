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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>

/// 4 bytes per pixel, one byte per color, unless otherwise specified.
typedef enum FramebufferFormat {
    FB_FORMAT_INVALID = 0,
    FB_FORMAT_ARGB,
    FB_FORMAT_BGRA,
    FB_FORMAT_ABGR,
    FB_FORMAT_MAX,
    FB_FORMAT_DEFAULT = FB_FORMAT_ARGB,
} FramebufferFormat;

typedef struct Framebuffer {
    void* base_address;
    uint64_t buffer_size;
    uint32_t pixel_width;
    uint32_t pixel_height;
    uint32_t pixels_per_scanline;
    FramebufferFormat format;
} Framebuffer;

/// Get a pixel given a framebuffer format
uint32_t mkpixel(const FramebufferFormat format, const unsigned char r, const unsigned char g, const unsigned char b, const unsigned char a);

/// Ensure given x and y pixel coordinates are within bounds.
void clamp_draw_position(const Framebuffer fb, size_t* x, size_t* y);

/// Write a single pixel to the framebuffer (slow).
void write_pixel(const Framebuffer fb, const uint32_t pixel, size_t x, size_t y);

/// Set each pixel within a rectangle of pixels to the given pixel value.
void fill_rect(const Framebuffer fb, const uint32_t pixel, size_t x, size_t y, size_t w, size_t h);

/// Set each pixel to given value.
void fill_color(const Framebuffer fb, const uint32_t pixel);

// TODO: read_*

#endif /* FRAMEBUFFER_H */
