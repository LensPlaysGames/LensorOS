#include "mouse.h"

Vector2 gMousePosition = {0, 0};

void MouseWait() {
	uint64_t timeout = 100000;
	while (timeout--){
		if ((inb(0x64) & 0b10) == 0) {
			return;
		}
	}
}

void MouseWaitInput() {
	uint64_t timeout = 100000;
	while (timeout--){
		if (inb(0x64) & 0b1) {
			return;
		}
	}
}

void MouseWrite(uint8_t value) {
	MouseWaitInput();
	outb(0x64, 0xD4);
	MouseWait();
	outb(0x60, value);
}

uint8_t MouseRead() {
	MouseWaitInput();
	return inb(0x60);
}

void InitPS2Mouse() {
	// Enable mouse.
	outb(0x64, 0xA8);
	MouseWait();
	// Tell keyboard controller a mouse message is incoming.
	outb(0x64, 0x20);
	
	MouseWaitInput();
	uint8_t status = inb(0x60);
	status |= 0b10;
	MouseWait();
	outb(0x64, 0x60);
	MouseWait();
	outb(0x60, status);

	MouseWrite(0xF6);
	MouseRead();

	MouseWrite(0xF4);
	MouseRead();
}

uint8_t mouse_cycle {0};
uint8_t mouse_packet[4];
bool mouse_packet_ready = false;
void HandlePS2Mouse(uint8_t data) {
	ProcessMousePacket();
	static bool skip = true;
	if (skip) {
		skip = false;
		return;
	}
	
	switch (mouse_cycle) {
	case 0:
		// Ensure always one bit is one.
		if (data & 0b00001000 == 0) { break; }
		mouse_packet[0] = data;
		mouse_cycle++;
		break;
	case 1:
		mouse_packet[1] = data;
		mouse_cycle++;
		break;
	case 2:
		mouse_packet[2] = data;
		mouse_packet_ready = true;
		mouse_cycle = 0;
		break;
	}
}


void ProcessMousePacket() {
	if (mouse_packet_ready == false) {
		return;
	}

	bool isXNegative  {false};
	bool isYNegative  {false};
	bool xOverflow    {false};
	bool yOverflow    {false};

	if (mouse_packet[0] & PS2XSIGN) {
		isXNegative = true;
	}
	if (mouse_packet[0] & PS2YSIGN) {
		isYNegative = true;
	}
	if (mouse_packet[0] & PS2XOVERFLOW) {
		xOverflow = true;
	}
	if (mouse_packet[0] & PS2YOVERFLOW) {
		yOverflow = true;
	}

	if (isXNegative) {
		mouse_packet[1] = 0x100 - mouse_packet[1];
		gMousePosition.x -= mouse_packet[1];
		if (xOverflow) {
			gMousePosition.x -= 255;
		}
	}
	else {
		gMousePosition.x += mouse_packet[1];
		if (xOverflow) {
			gMousePosition.x += 255;
		}
	}

	if (isYNegative) {
		mouse_packet[2] = 0x100 - mouse_packet[2];
		gMousePosition.y += mouse_packet[2];
		if (yOverflow) {
			gMousePosition.y += 255;
		}
	}
	else {
		gMousePosition.y -= mouse_packet[2];
		if (yOverflow) {
			gMousePosition.y -= 255;
		}
	}

	if (gMousePosition.x < 0) { gMousePosition.x = 0; }
	else if (gMousePosition.x > gRend.framebuffer->PixelWidth-1) {
		gMousePosition.x = gRend.framebuffer->PixelWidth-1;
	}

	if (gMousePosition.y < 0) { gMousePosition.y = 0; }
	else if (gMousePosition.y > gRend.framebuffer->PixelHeight-1) {
		gMousePosition.y = gRend.framebuffer->PixelHeight-1;
	}
	
	gRend.PixelPosition = gMousePosition;
	gRend.putchar('A');
}
