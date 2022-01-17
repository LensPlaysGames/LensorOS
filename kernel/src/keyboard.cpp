#include "keyboard.h"

bool isLSHIFT;
bool isRSHIFT;

void HandleKeyboard(uint8_t scancode) {
	// TODO: Check what type of keyboard it is.
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
		gRend.crlf();
		return;
	// case BACKSPACE:
	// 	gRend.clearchar();
	// 	return;
	case SPACE:
		gRend.putchar(' ');
		return;
	}

	char ascii = QWERTY::Translate(scancode, isLSHIFT | isRSHIFT);
	if (ascii != 0) {
		gRend.putchar(ascii);
	}
}
