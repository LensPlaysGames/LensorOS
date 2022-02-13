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

    void BasicTextRenderer::draw_cursor() {
        CachedDrawPosition = gRend.DrawPos;

        // Calculate rectangle that needs to be updated (in characters).
        uVector2 RefreshPosition = LastCursorPosition + uVector2(0, 1);
        uVector2 RefreshSize { 1, 1 };
        if (CursorPosition.x > RefreshPosition.x)
            RefreshSize.x += CursorPosition.x - RefreshPosition.x;
        else if (CursorPosition.x < RefreshPosition.x) {
            RefreshSize.x += RefreshPosition.x - CursorPosition.x;
            RefreshPosition.x = CursorPosition.x;
        }
        if (CursorPosition.y > RefreshPosition.y)
            RefreshSize.y += CursorPosition.y - RefreshPosition.y;
        else if (CursorPosition.y < RefreshPosition.y) {
            RefreshSize.y += RefreshPosition.y - CursorPosition.y;
            RefreshPosition.y = CursorPosition.y;
        }
        // Convert characters to pixels.
        RefreshPosition = RefreshPosition * uVector2(8, gRend.Font->PSF1_Header->CharacterSize);
        RefreshSize = RefreshSize * uVector2(8, gRend.Font->PSF1_Header->CharacterSize);

        // Skip first iteration in order to accurately read what is under the cursor before it is drawn.
        static bool skip = true;
        if (skip == false) {
            gRend.DrawPos = uVector2(LastCursorPosition.x * 8, LastCursorPosition.y * gRend.Font->PSF1_Header->CharacterSize);
            gRend.DrawPos.y = gRend.DrawPos.y + gRend.Font->PSF1_Header->CharacterSize;
            gRend.drawpix({KBCursorSizeX, KBCursorSizeY}, &PixelsUnderKBCursor[0]);
        }
        else skip = false;

        update_draw_position();
        DrawPosition.y = DrawPosition.y + gRend.Font->PSF1_Header->CharacterSize;
        gRend.DrawPos = DrawPosition;
        // READ PIXELS UNDER NEW POSITION INTO BUFFER.
        gRend.readpix({KBCursorSizeX, KBCursorSizeY}, &PixelsUnderKBCursor[0]);
        // DRAW CURSOR AT NEW POSITION.
        gRend.drawbmpover({KBCursorSizeX, KBCursorSizeY}, &KeyboardCursor[0], 0xffffffff);
        gRend.swap(RefreshPosition, RefreshSize);
        // RETURN GLOBAL DRAW POSITION.
        gRend.DrawPos = CachedDrawPosition;

        LastCursorPosition = CursorPosition;
    }

    void BasicTextRenderer::backspace() {
        CachedDrawPosition = gRend.DrawPos;
        update_draw_position();
        gRend.DrawPos = DrawPosition;
        gRend.clearchar();
        gRend.swap(gRend.DrawPos, {8, gRend.Font->PSF1_Header->CharacterSize});
        cursor_left();
        gRend.DrawPos = CachedDrawPosition;
    }

    void BasicTextRenderer::putc(char character) {
        CachedDrawPosition = gRend.DrawPos;
        update_draw_position();
        gRend.DrawPos = DrawPosition;
        gRend.putchar(character);
        gRend.swap(DrawPosition, {8, gRend.Font->PSF1_Header->CharacterSize});
        cursor_right();
        gRend.DrawPos = CachedDrawPosition;
    }

    void BasicTextRenderer::parse_character(char c) {
        switch (c) {
        case 0x8:
            // BS
            backspace();
            return;
        case 0xd:
            // CR
            newline();
            return;
        default:
            break;
        }

        // Non-printable characters.
        if (c < 32)
            return;

        putc(c);
    }

    void BasicTextRenderer::parse_scancode(u8 code) {
        if (GotE0) {
            switch (code) {
            case ARROW_UP:
                cursor_up();
                break;
            case ARROW_DOWN:
                cursor_down();
                break;
            case ARROW_LEFT:
                cursor_left();
                break;
            case ARROW_RIGHT:
                cursor_right();
                break;
            default:
                break;
            }
            
            GotE0 = false;
        }
        else {
            switch (code) {
            case 0xe0:
                GotE0 = true;
                return;
            case LSHIFT:
                State.LeftShift = true;
                return;
            case LSHIFT + 0x80:
                State.LeftShift = false;
                return;
            case RSHIFT:
                State.RightShift = true;
                return;
            case RSHIFT + 0x80:
                State.RightShift = false;
                return;
            case CAPSLOCK:
                State.CapsLock = !State.CapsLock;
                return;
            case ENTER:
                newline();
                return;
            case BACKSPACE:
                backspace();
                return;
            default:
                break;
            }
            handle_character(QWERTY::Translate(code, (State.LeftShift || State.RightShift || State.CapsLock)));
        }
    }
}
