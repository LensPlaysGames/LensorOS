#ifndef LENSOR_OS_KB_SCANCODE_TRANSLATION_H
#define LENSOR_OS_KB_SCANCODE_TRANSLATION_H

#include <stdint.h>

namespace QWERTY {
	// For release counter-part, add 0x80.
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
  
	extern const char ASCII[];
	char Translate(uint8_t scancode, bool capital);
}

#endif
