#ifndef LENSOR_OS_MOUSE_H
#define LENSOR_OS_MOUSE_H

#define PS2LBTN      0b00000001
#define PS2RBTN      0b00000010
#define PS2MBTN      0b00000100
#define PS2XSIGN     0b00010000
#define PS2YSIGN     0b00100000
#define PS2XOVERFLOW 0b01000000
#define PS2YOVERFLOW 0b10000000

#include "io.h"
#include "math.h"
#include "cstr.h"
#include "basic_renderer.h"

void InitPS2Mouse();
void HandlePS2Mouse(uint8_t data);
void ProcessMousePacket();

extern Vector2 gMousePosition;

#endif
