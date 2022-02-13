#include "keyboard.h"

#include "basic_renderer.h"
#include "keyboard_scancode_translation.h"

namespace Keyboard {
    BasicTextRenderer gText;
    u8 KeyboardCursor[] = {
        0b11111111,
        0b11111111
    };
    u32 PixelsUnderKBCursor[KBCursorSizeX * KBCursorSizeY + 1];
}
