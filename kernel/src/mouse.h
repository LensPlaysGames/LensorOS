#ifndef LENSOR_OS_MOUSE_H
#define LENSOR_OS_MOUSE_H

#define PS2LBTN      0b00000001
#define PS2RBTN      0b00000010
#define PS2MBTN      0b00000100
#define PS2XSIGN     0b00010000
#define PS2YSIGN     0b00100000
#define PS2XOVERFLOW 0b01000000
#define PS2YOVERFLOW 0b10000000

#include "integers.h"
#include "basic_renderer.h"
#include "cstr.h"
#include "io.h"
#include "math.h"
#include "uart.h"

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
extern uVector2 gMousePosition;
extern uVector2 gOldMousePosition;

#endif
