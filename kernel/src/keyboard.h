#ifndef LENSOR_OS_KEYBOARD_H
#define LENSOR_OS_KEYBOARD_H

#include "integers.h"
#include "math.h"

namespace Keyboard {
    void newline();
    void put_char(u8);
    void clear_char();
    void handle_scancode(u8);
    extern uVector2 gTextPosition;
    extern uVector2 gCachedPos;
}

#endif
