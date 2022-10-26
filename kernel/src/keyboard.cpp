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

#include <keyboard.h>

#include <basic_renderer.h>
#include <integers.h>
#include <keyboard_scancode_translation.h>
#include <math.h>

namespace Keyboard {
    BasicTextRenderer gText;
    u8 KeyboardCursor[] = {
        0b11111111,
        0b11111111
    };
    u32 PixelsUnderKBCursor[KBCursorSizeX * KBCursorSizeY + 1];

    void BasicTextRenderer::draw_cursor() {
        // Calculate rectangle that needs to be updated (in characters).
        Vector2<u64> RefreshPosition = Vector2<u64>(0, 1) + LastCursorPosition;
        Vector2<u64> RefreshSize = Vector2<u64>(1, 1);
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
        RefreshPosition = RefreshPosition * Vector2<u64>(8, gRend.Font->PSF1_Header->CharacterSize);
        RefreshSize = RefreshSize * Vector2<u64>(8, gRend.Font->PSF1_Header->CharacterSize);

        // Skip first iteration in order to accurately read what is under the cursor before it is drawn.
        static bool skip = true;
        if (skip == false) {
            DrawPosition = {
                LastCursorPosition.x * 8,
                (LastCursorPosition.y + 1)
                * gRend.Font->PSF1_Header->CharacterSize
            };
            gRend.drawpix(DrawPosition, {KBCursorSizeX, KBCursorSizeY}, &PixelsUnderKBCursor[0]);
        }
        else skip = false;

        update_draw_position();
        DrawPosition.y = DrawPosition.y + gRend.Font->PSF1_Header->CharacterSize;
        // READ PIXELS UNDER NEW POSITION INTO BUFFER.
        gRend.readpix(DrawPosition, {KBCursorSizeX, KBCursorSizeY}, &PixelsUnderKBCursor[0]);
        // DRAW CURSOR AT NEW POSITION.
        gRend.drawbmpover(DrawPosition, {KBCursorSizeX, KBCursorSizeY}, &KeyboardCursor[0], 0xffffffff);
        gRend.swap(RefreshPosition, RefreshSize);

        LastCursorPosition = CursorPosition;
    }

    void BasicTextRenderer::backspace() {
        update_draw_position();
        gRend.clearchar(DrawPosition);
        gRend.swap(DrawPosition, {8, gRend.Font->PSF1_Header->CharacterSize});
        cursor_left();
    }

    void BasicTextRenderer::putc(char character) {
        update_draw_position();
        gRend.putchar(DrawPosition, character);
        gRend.swap(DrawPosition, {8, gRend.Font->PSF1_Header->CharacterSize});
        cursor_right();
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
