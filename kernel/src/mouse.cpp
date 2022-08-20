#include <mouse.h>

#include <random_lfsr.h>
#include <basic_renderer.h>
#include <cstr.h>
#include <integers.h>
#include <io.h>
#include <math.h>
#include <uart.h>

// globally accessible mouse information
u8 gMouseID;
Vector2<u64> gMousePosition = {0, 0};
Vector2<u64> gOldMousePosition = {0, 0};

void mouse_wait() {
    u64 timeout = 100000;
    while (timeout--) {
        if ((in8(0x64) & 0b10) == 0)
            return;
    }
}

void mouse_wait_input() {
    u64 timeout = 100000;
    while (timeout--) {
        if (in8(0x64) & 0b1)
            return;
    }
}

void mouse_write(u8 value) {
    mouse_wait_input();
    out8(0x64, 0xd4);
    mouse_wait();
    out8(0x60, value);
}

u8 mouse_read() {
    mouse_wait_input();
    return in8(0x60);
}

void init_ps2_mouse() {
    // Enable mouse.
    out8(0x64, 0xA8);
    mouse_wait();
    // Tell keyboard controller a mouse message is incoming.
    out8(0x64, 0x20);

    mouse_wait_input();
    u8 status = in8(0x60);
    status |= 0b10;
    mouse_wait();
    out8(0x64, 0x60);
    mouse_wait();
    out8(0x60, status);

    mouse_write(0xf6);
    mouse_read(); // ACKNOWLEDGE

    mouse_write(0xf4);
    mouse_read(); // ACK

    mouse_write(0xf2);
    mouse_read(); // ACK
    gMouseID = mouse_read();

    UART::out("[Mouse]: Successfully initialized PS2 mouse using serial port (ID: ");
    UART::out(to_string(gMouseID));
    UART::out(")\r\n");
}

u8 mouse_cycle {0};
u8 mouse_packet[4];
bool mouse_packet_ready = false;
void handle_ps2_mouse_interrupt(u8 data) {
    // Skip first mouse packet (garbage data).
    static bool skip = true;
    if (skip) {
        skip = false;
        return;
    }
    switch (mouse_cycle) {
    case 0:
        // Ensure always one bit is one.
        if ((data & 0b00001000) == 0)
            break;

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
// This is a bitmap of the cursor, lmao.
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
void draw_mouse_cursor() {
    Vector2<u64> refreshPos = gOldMousePosition;
    Vector2<u64> refreshSize = {MouseCursorSize, MouseCursorSize};

    if (gMousePosition.x > refreshPos.x) {
        refreshSize.x += gMousePosition.x - refreshPos.x;
    }
    else if (gMousePosition.x < refreshPos.x) {
        refreshSize.x += refreshPos.x - gMousePosition.x;
        refreshPos.x = gMousePosition.x;
    }
    if (gMousePosition.y > refreshPos.y) {
        refreshSize.y += gMousePosition.y - refreshPos.y;
    }
    else if (gMousePosition.y < refreshPos.y) {
        refreshSize.y += refreshPos.y - gMousePosition.y;
        refreshPos.y = gMousePosition.y;
    }

    // Skip first iteration in order to accurately read
    // what is under the cursor before it is drawn.
    static bool skip = true;
    if (skip == false) {
        gRend.drawpix(gOldMousePosition,
                      {MouseCursorSize, MouseCursorSize},
                      &pixels_under_mouse_cursor[0]);
    }
    else skip = false;

    // READ PIXELS UNDER NEW MOUSE POSITION INTO BUFFER.
    gRend.readpix(gMousePosition,
                  {MouseCursorSize, MouseCursorSize}
                  , &pixels_under_mouse_cursor[0]);
    // DRAW MOUSE CUSOR AT NEW POSITION.
    gRend.drawbmpover(gMousePosition,
                      {MouseCursorSize, MouseCursorSize}
                      , &mouse_cursor[0], 0xffffffff);
    gOldMousePosition = gMousePosition;
    gRend.swap(refreshPos, refreshSize);
}

u32 DrawColor = 0xffffffff;
void process_mouse_packet() {
    // ONLY PROCESS A PACKET THAT IS READY
    if (mouse_packet_ready == false)
        return;
    // MOUSE BUTTONS
    // Left Mouse Button (LMB)
    const Vector2<u64> DrawSize {2, 2};
    if (mouse_packet[0] & PS2_LEFT_BUTTON) {
        // Draw a rectangle of size `DrawSize` with color of `DrawColor`
        Vector2<u64> drawPos = gMousePosition - DrawSize;
        gRend.drawrect(drawPos, DrawSize, DrawColor);
        gRend.swap({DrawSize}, drawPos);
    }
    // Right Mouse Button (RMB)
    else if (mouse_packet[0] & PS2_RIGHT_BUTTON) {
        // Use pseudo-random number generator
        //   to get a new color to draw with.
        do { DrawColor = (u32)gRandomLFSR.get(); }
        while (DrawColor == 0);
    }
    // Middle Mouse Button (MMB)
    else if (mouse_packet[0] & PS2_MIDDLE_BUTTON) {}

    // MOUSE MOVEMENT
    bool isXNegative  {false};
    bool isYNegative  {false};
    bool xOverflow    {false};
    bool yOverflow    {false};
    // DECODE BIT-FLAGS FROM FIRST PACKET
    if (mouse_packet[0] & PS2_X_SIGN)
        isXNegative = true;
    if (mouse_packet[0] & PS2_Y_SIGN)
        isYNegative = true;
    if (mouse_packet[0] & PS2_X_OVERFLOW)
        xOverflow = true;
    if (mouse_packet[0] & PS2_Y_OVERFLOW)
        yOverflow = true;

    // ACCUMULATE X MOUSE POSITION FROM SECOND PACKET
    if (isXNegative) {
        mouse_packet[1] = 256 - mouse_packet[1];
        gMousePosition.x -= mouse_packet[1];
        if (xOverflow)
            gMousePosition.x -= 255;
        // CLAMP MOUSE POSITION (OVERFLOW DETECTION)
        if (gMousePosition.x > gOldMousePosition.x)
            gMousePosition.x = 0;
    }
    else {
        gMousePosition.x += mouse_packet[1];
        if (xOverflow)
            gMousePosition.x += 255;
    }
    // ACCUMULATE Y MOUSE POSITION FROM THIRD PACKET
    if (isYNegative) {
        mouse_packet[2] = 256 - mouse_packet[2];
        gMousePosition.y += mouse_packet[2];
        if (yOverflow)
            gMousePosition.y += 255;
    }
    else {
        gMousePosition.y -= mouse_packet[2];
        if (yOverflow)
            gMousePosition.y -= 255;
        // CLAMP MOUSE POSITION (OVERFLOW DETECTION)
        if (gMousePosition.y > gOldMousePosition.y)
            gMousePosition.y = 0;
    }
    // CLAMP MOUSE POSITION
    if (gMousePosition.x >= gRend.Target->PixelWidth)
        gMousePosition.x = gRend.Target->PixelWidth  - 1;
    if (gMousePosition.y >= gRend.Target->PixelHeight)
        gMousePosition.y = gRend.Target->PixelHeight - 1;
    // USE GLOBAL OUTPUT PROTOCOL RENDERER TO DRAW THE MOUSE CURSOR.
    draw_mouse_cursor();
    // PACKET USED; DISCARD READY STATE
    mouse_packet_ready = false;
}
