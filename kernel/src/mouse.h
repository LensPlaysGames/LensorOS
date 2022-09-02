/* Copyright 2022, Contributors To LensorOS.
All rights reserved.

This file is part of LensorOS.

LensorOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LensorOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LensorOS. If not, see <https://www.gnu.org/licenses */
#ifndef LENSOR_OS_MOUSE_H
#define LENSOR_OS_MOUSE_H

#define PS2_LEFT_BUTTON   0b00000001
#define PS2_RIGHT_BUTTON  0b00000010
#define PS2_MIDDLE_BUTTON 0b00000100
#define PS2_X_SIGN        0b00010000
#define PS2_Y_SIGN        0b00100000
#define PS2_X_OVERFLOW    0b01000000
#define PS2_Y_OVERFLOW    0b10000000

#include <integers.h>
#include <math.h>

void init_ps2_mouse();
void handle_ps2_mouse_interrupt(u8 data);
void process_mouse_packet();
// DRAW MOUSE CURSOR AT MOUSE POSITION USING GLOBAL RENDERER.
void draw_mouse_cursor();

// The mouse ID determines what features the mouse supports.
//   0 = x, y, left, right, middle (3 bytes)
//   3 = 0 + scroll data (4th byte)
//   4 = 3 + 4th button, 5th button
extern u8 gMouseID;
extern Vector2<u64> gMousePosition;
extern Vector2<u64> gOldMousePosition;

#endif
