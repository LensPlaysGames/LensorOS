#ifndef LENSOR_OS_KEYBOARD_H
#define LENSOR_OS_KEYBOARD_H

#include <integers.h>
#include <math.h>
#include <basic_renderer.h>

namespace Keyboard {
    const u8 KBCursorSizeX = 8;
    const u8 KBCursorSizeY = 2;

    extern u8 KeyboardCursor[KBCursorSizeX * KBCursorSizeY];
    extern u32 PixelsUnderKBCursor[KBCursorSizeX * KBCursorSizeY + 1];

    struct KeyboardState {
        bool LeftShift  { false };
        bool RightShift { false };
        bool CapsLock   { false };
    };

    class BasicTextRenderer {
    public:
        Vector2<u64> SizeInCharacters;
        Vector2<u64> DrawPosition;
        Vector2<u64> CachedDrawPosition;
        Vector2<u64> CursorPosition;
        Vector2<u64> LastCursorPosition;
        KeyboardState State;
        bool GotE0 { false };

        BasicTextRenderer() {
            SizeInCharacters.x = gRend.Target->PixelWidth / 8;
            SizeInCharacters.y = gRend.Target->PixelHeight / gRend.Font->PSF1_Header->CharacterSize;
        }

        void newline() {
            CursorPosition.x = 0;
            cursor_down();
        }

        void cursor_up(u64 amt = 1) {
            if (amt > CursorPosition.y)
                CursorPosition.y = 0;
            else CursorPosition.y -= amt;
        }

        void cursor_down(u64 amt = 1) {
            if (SizeInCharacters.y - amt < CursorPosition.y)
                CursorPosition.y = SizeInCharacters.y - 1;
            else CursorPosition.y += amt;
        }

        void cursor_left(u64 amt = 1) {
            if (amt > CursorPosition.x)
                CursorPosition.x = 0;
            else CursorPosition.x -= amt;
        }

        void cursor_right(u64 amt = 1) {
            if (SizeInCharacters.x - amt <= CursorPosition.x)
                newline();
            else CursorPosition.x += amt;
        }

        void set_cursor_from_pixel_position(Vector2<u64> pos) {
            CursorPosition.x = pos.x / 8;
            CursorPosition.y = pos.y / gRend.Font->PSF1_Header->CharacterSize;
        }

        void update_draw_position() {
            DrawPosition.x = CursorPosition.x * 8;
            DrawPosition.y = CursorPosition.y * gRend.Font->PSF1_Header->CharacterSize;
        }

        void handle_scancode(u8 code) {
            parse_scancode(code);
            draw_cursor();
        }

        void handle_character(char c) {
            parse_character(c);
            draw_cursor();
        }

        void putc(char);
        void backspace();

        void draw_cursor();

        void parse_character(char c);
        void parse_scancode(u8);
    };

    extern BasicTextRenderer gText;
}

#endif
