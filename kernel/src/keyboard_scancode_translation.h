#ifndef LENSOR_OS_KB_SCANCODE_TRANSLATION_H
#define LENSOR_OS_KB_SCANCODE_TRANSLATION_H

#include "integers.h"

namespace Keyboard {
    namespace QWERTY {
        /// For release counter-part, add 0x80.
#define ESCAPE     0x01
#define BACKSPACE  0x0e
#define TAB        0x0f
#define ENTER      0x1c
#define LCONTROL   0x1d
#define LSHIFT     0x2a
#define RSHIFT     0x36
#define LALT       0x38
#define SPACE      0x39
#define CAPSLOCK   0x3a
#define NUMLOCK    0x45
#define SCROLLLOCK 0x46

        /// Preceded by 'e0' byte.
#define ARROW_UP    0x48
#define ARROW_DOWN  0x50
#define ARROW_LEFT  0x4b
#define ARROW_RIGHT 0x4d
  
        extern const char ASCII[];
        char Translate(u8 scancode, bool capital);
    }
}
#endif
