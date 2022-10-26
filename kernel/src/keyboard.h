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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#ifndef LENSOR_OS_KEYBOARD_H
#define LENSOR_OS_KEYBOARD_H

#include <integers.h>
#include <math.h>
#include <basic_renderer.h>

namespace Keyboard {
    const u8 KBCursorSizeX = 8;
    const u8 KBCursorSizeY = 2;
    constexpr u16 KBCursorSize = KBCursorSizeX * KBCursorSizeY;
    extern u8 KeyboardCursor[KBCursorSize];
    extern u32 PixelsUnderKBCursor[KBCursorSize + 1];

    struct KeyboardState {
        bool LeftShift  { false };
        bool RightShift { false };
        bool CapsLock   { false };
    };

    class BasicTextRenderer {
    public:
        /// The two-dimensional size of the text renderer, in characters.
        Vector2<u64> SizeInCharacters;
        /// Two-dimensional position to draw at in pixels.
        Vector2<u64> DrawPosition;
        /// Two-dimensional position of cursor in character grid.
        Vector2<u64> CursorPosition;
        /// Two-dimensional position of cursor in character grid last render.
        Vector2<u64> LastCursorPosition;
        KeyboardState State;
        bool GotE0 { false };

        BasicTextRenderer() {
            SizeInCharacters.x = gRend.Target->PixelWidth / 8;
            SizeInCharacters.y = gRend.Target->PixelHeight / gRend.Font->PSF1_Header->CharacterSize;
        }

        BasicTextRenderer(Vector2<u64> size) {
            SizeInCharacters.x = size.x;
            SizeInCharacters.y = size.y;
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
