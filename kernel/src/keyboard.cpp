#include "keyboard.h"

Vector2 gTextPosition;

bool isCAPS;
bool isLSHIFT;
bool isRSHIFT;

void HandleKeyboard(uint8_t scancode) {
	Vector2 cachedPos = gRend.DrawPos;
	gRend.DrawPos = gTextPosition;
	
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
		gTextPosition = gRend.DrawPos;
		gRend.DrawPos = cachedPos;
		return;
	case ENTER + 0x80:
		return;
	case BACKSPACE:
	 	gRend.clearchar();
		gTextPosition = gRend.DrawPos;
		gRend.DrawPos = cachedPos;
	 	return;
	case BACKSPACE + 0x80:
	 	return;
	case SPACE:
		gRend.putchar(' ');
		gTextPosition = gRend.DrawPos;
		gRend.DrawPos = cachedPos;
		return;
	case SPACE + 0x80:
		return;
	case CAPSLOCK:
		isCAPS = !isCAPS;
		return;
	case CAPSLOCK + 0x80:
		return;
	}

	char ascii = QWERTY::Translate(scancode, isLSHIFT | isRSHIFT | isCAPS);
	if (ascii != 0) {
		gRend.putchar(ascii);
	}
	
	gTextPosition = gRend.DrawPos;
	gRend.DrawPos = cachedPos;
}
