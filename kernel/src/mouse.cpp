#include "mouse.h"

// globally accessible mouse information
u8 gMouseID;
Vector2 gMousePosition = {0, 0};
Vector2 gOldMousePosition = {0, 0};

void mouse_wait() {
	u64 timeout = 100000;
	while (timeout--){
		if ((inb(0x64) & 0b10) == 0) {
			return;
		}
	}
}

void mouse_wait_input() {
	u64 timeout = 100000;
	while (timeout--){
		if (inb(0x64) & 0b1) {
			return;
		}
	}
}

void mouse_write(u8 value) {
	mouse_wait_input();
	outb(0x64, 0xD4);
	mouse_wait();
	outb(0x60, value);
}

u8 mouse_read() {
	mouse_wait_input();
	return inb(0x60);
}

void init_ps2_mouse() {
	// Enable mouse.
	outb(0x64, 0xA8);
	mouse_wait();
	// Tell keyboard controller a mouse message is incoming.
	outb(0x64, 0x20);
	
	mouse_wait_input();
	u8 status = inb(0x60);
	status |= 0b10;
	mouse_wait();
	outb(0x64, 0x60);
	mouse_wait();
	outb(0x60, status);

	mouse_write(0xF6);
	mouse_read(); // ACKNOWLEDGE

	mouse_write(0xF4);
	mouse_read(); // ACK

	mouse_write(0xF2);
	mouse_read(); // ACK
	gMouseID = mouse_read();
	gRend.putstr("Successfully initialized PS2 mouse using serial port (ID: ");
	gRend.putstr(to_string((u64)gMouseID));
	gRend.putchar(')');
	gRend.crlf();
}

u8 mouse_cycle {0};
u8 mouse_packet[4];
bool mouse_packet_ready = false;
void handle_ps2_mouse_interrupt(u8 data) {
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
	process_mouse_packet();
}

#define MouseCursorSize 16
u8 mouse_cursor[] = {
	0b10000000, 0b00000000,
	0b11000000, 0b00000000,
	0b11100000, 0b00000000,
	0b11110000, 0b00000000,
	0b11111000, 0b00000000,
	0b11111100, 0b00000000,
	0b11111110, 0b00000000,
	0b11111111, 0b00000000,
	0b11111111, 0b10000000,
	0b11111111, 0b11000000,
	0b11111111, 0b00000000,
	0b11111100, 0b00000000,
	0b11110000, 0b00000000,
	0b11000000, 0b00000000,
	0b00000000, 0b00000000,
	0b00000000, 0b00000000
};

u32 pixels_under_mouse_cursor[MouseCursorSize * MouseCursorSize + 1];

// DRAW MOUSE CURSOR AT gMousePosition
void DrawMouseCursor() {
	Vector2 cachedPos = gRend.DrawPos;
	// Skip first iteration in order to accurately read what is under the cursor before it is drawn.
	static bool skip = true;
	if (skip == false) {
		gRend.DrawPos = gOldMousePosition;
		gRend.drawpix({MouseCursorSize, MouseCursorSize}, &pixels_under_mouse_cursor[0]);
	}
	else {
		skip = false;
	}
	gRend.DrawPos = gMousePosition;
	// READ PIXELS UNDER NEW MOUSE POSITION INTO BUFFER.
	gRend.readpix({MouseCursorSize, MouseCursorSize}, &pixels_under_mouse_cursor[0]);
	// DRAW MOUSE CUSOR AT NEW POSITION.
	gRend.drawbmpover({MouseCursorSize, MouseCursorSize}, &mouse_cursor[0], 0xffffffff);
	// UPDATE VISUALS FROM NEW DATA.
	gOldMousePosition = gMousePosition;
	// RETURN GLOBAL DRAW POSITION.
	gRend.DrawPos = cachedPos;
}

void process_mouse_packet() {
	// ONLY PROCESS A PACKET THAT IS READY
	if (mouse_packet_ready == false) {
		return;
	}
	// MOUSE BUTTONS
	if (mouse_packet[0] & PS2LBTN) {
		// LEFT CLICK
	}
	else if (mouse_packet[0] & PS2RBTN) {
		// RIGHT CLICK
	}
	else if (mouse_packet[0] & PS2MBTN) {
		// MIDDLE (SCROLL WHEEL) CLICK
	}
	// MOUSE MOVEMENT
	bool isXNegative  {false};
	bool isYNegative  {false};
	bool xOverflow    {false};
	bool yOverflow    {false};
	// DECODE BIT-FLAGS FROM FIRST PACKET
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
	// ACCUMULATE X MOUSE POSITION FROM SECOND PACKET
	if (isXNegative) {
		mouse_packet[1] = 256 - mouse_packet[1];
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
	// ACCUMULATE Y MOUSE POSITION FROM THIRD PACKET
	if (isYNegative) {
		mouse_packet[2] = 256 - mouse_packet[2];
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
	// CLAMP MOUSE POSITION
	if (gMousePosition.x < 0) { gMousePosition.x = 0; }
	else if (gMousePosition.x > gRend.Target->PixelWidth-1) {
		gMousePosition.x = gRend.Target->PixelWidth-1;
	}
	if (gMousePosition.y < 0) { gMousePosition.y = 0; }
	else if (gMousePosition.y > gRend.Target->PixelHeight-1) {
		gMousePosition.y = gRend.Target->PixelHeight-1;
	}

	DrawMouseCursor();
	
	// PACKET USED; DISCARD READY STATE
	mouse_packet_ready = false;
}
