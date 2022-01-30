#ifndef LENSOR_OS_KEYBOARD_H
#define LENSOR_OS_KEYBOARD_H

#include "integers.h"
#include "keyboard_scancode_translation.h"
#include "basic_renderer.h"

void handle_keyboard(u8 scancode);

extern uVector2 gTextPosition;

#endif
