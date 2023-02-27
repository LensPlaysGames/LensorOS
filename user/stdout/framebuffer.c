#include "framebuffer.h"

uint32_t mkpixel(FramebufferFormat format, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  switch (format) {
  default:
  case FB_FORMAT_ARGB:
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b << 0) | ((uint32_t)a << 24);
  case FB_FORMAT_BGRA:
    return ((uint32_t)r << 8) | ((uint32_t)g << 16) | ((uint32_t)b << 24) | ((uint32_t)a << 0);
  case FB_FORMAT_ABGR:
    return ((uint32_t)r << 0) | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
  }
}

void clamp_draw_position(const Framebuffer fb, size_t *x, size_t *y) {
  if (*x > fb.pixel_width)
    *x = fb.pixel_width - 1;
  if (*y > fb.pixel_height)
    *y = fb.pixel_height - 1;
}

void write_pixel(const Framebuffer fb, const uint32_t pixel, size_t x, size_t y) {
  size_t index = y * fb.pixels_per_scanline + x;
  ((uint32_t *)fb.base_address)[index] = pixel;
}

void fill_rect(const Framebuffer fb, const uint32_t pixel, size_t x, size_t y, size_t w, size_t h) {
  if (x >= fb.pixel_width || y >= fb.pixel_height) return;
  if (x + w >= fb.pixel_width) w = fb.pixel_width - x;
  if (y + h >= fb.pixel_height) h = fb.pixel_height - y;
  for (size_t posy = y; posy < y + h; ++posy) {
    for (size_t posx = x; posx < x + w; ++posx) {
      size_t index = posy * fb.pixels_per_scanline + posx;
      ((uint32_t *)fb.base_address)[index] = pixel;
    }
  }
}

void fill_color(const Framebuffer fb, const uint32_t pixel) {
  for (size_t y = 0; y < fb.pixel_height; ++y) {
    for (size_t x = 0; x < fb.pixel_width; ++x) {
      size_t index = y * fb.pixels_per_scanline + x;
      ((uint32_t *)fb.base_address)[index] = pixel;
    }
  }
}

