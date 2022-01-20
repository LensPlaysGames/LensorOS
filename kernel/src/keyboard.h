#ifndef LENSOR_OS_KEYBOARD_H
#define LENSOR_OS_KEYBOARD_H

#include <stdint.h>
#include "keyboard_scancode_translation.h"
#include "basic_renderer.h"

void handle_keyboard(uint8_t scancode);

extern Vector2 gTextPosition;

#endif
