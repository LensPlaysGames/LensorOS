#include "keyboard.h"

namespace Keyboard {
    uVector2 gTextPosition {0, 0};
    uVector2 gCachedPos    {0, 0};

    void newline() {    
        gCachedPos = gRend.DrawPos;
        gRend.DrawPos = gTextPosition;
        gRend.crlf();
        gTextPosition = gRend.DrawPos;
        gRend.DrawPos = gCachedPos;
    }

    void put_char(u8 character) {
        gCachedPos = gRend.DrawPos;
        gRend.DrawPos = gTextPosition;
        gRend.putchar(character);
        gRend.swap(gTextPosition, {8, gRend.Font->PSF1_Header->CharacterSize});
        gTextPosition = gRend.DrawPos;
        gRend.DrawPos = gCachedPos;
    }

    void clear_char() {
        gCachedPos = gRend.DrawPos;
        gRend.DrawPos = gTextPosition;
        gRend.clearchar();
        gTextPosition = gRend.DrawPos;
        gRend.swap(gTextPosition, {8, gRend.Font->PSF1_Header->CharacterSize});
        gRend.DrawPos = gCachedPos;
    }

    bool isCAPS;
    bool isLSHIFT;
    bool isRSHIFT;

    void handle_scancode(uint8_t scancode) {
        switch (scancode) {
        case LSHIFT:
            isLSHIFT = true;
            return;
        case LSHIFT + 0x80:
            isLSHIFT = false;
            return;
        case RSHIFT:
            isRSHIFT = true;
            return;
        case RSHIFT + 0x80:
            isRSHIFT = false;
            return;
        case ENTER:
            newline();
            return;
        case BACKSPACE:
            clear_char();
            return;
        case SPACE:
            put_char(' ');
            return;
        case CAPSLOCK:
            isCAPS = !isCAPS;
            return;
        default:
            break;
        }

        char ascii = QWERTY::Translate(scancode, isLSHIFT | isRSHIFT | isCAPS);
        if (ascii != 0) {
            put_char(ascii);
        }
    }
}
