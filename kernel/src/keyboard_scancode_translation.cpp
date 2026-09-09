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
#include <keyboard_scancode_translation.h>
#include <lensor/keys.h>
#include <system.h>

#include <print>

namespace Keyboard {
namespace QWERTY {

uint8_t map[][2] = {
    {0, 0},
    {1, 0},
    {2, '1'},
    {3, '2'},
    {4, '3'},
    {5, '4'},
    {6, '5'},
    {7, '6'},
    {8, '7'},
    {9, '8'},
    {10, '9'},
    {11, '0'},
    {12, '-'},
    {13, '='},
    {14, '\b'},
    {15, 0},
    {16, 'q'},
    {17, 'w'},
    {18, 'e'},
    {19, 'r'},
    {20, 't'},
    {21, 'y'},
    {22, 'u'},
    {23, 'i'},
    {24, 'o'},
    {25, 'p'},
    {26, '['},
    {27, ']'},
    {28, '\n'},
    {29, 0},
    {30, 'a'},
    {31, 's'},
    {32, 'd'},
    {33, 'f'},
    {34, 'g'},
    {35, 'h'},
    {36, 'j'},
    {37, 'k'},
    {38, 'l'},
    {39, ';'},
    {40, '\''},
    {41, '`'},
    {42, 0},
    {43, '\\'},
    {44, 'z'},
    {45, 'x'},
    {46, 'c'},
    {47, 'v'},
    {48, 'b'},
    {49, 'n'},
    {50, 'm'},
    {51, ','},
    {52, '.'},
    {53, '/'},
    {54, 0},
    {55, '*'},
    {56, 0},
    {57, ' '}};

uint8_t cap_map[][2] = {
    {0, 0},
    {1, 0},
    {2, '!'},
    {3, '@'},
    {4, '#'},
    {5, '$'},
    {6, '%'},
    {7, '^'},
    {8, '&'},
    {9, '*'},
    {10, '('},
    {11, ')'},
    {12, '_'},
    {13, '+'},
    {14, 0},
    {15, 0},
    {16, 'Q'},
    {17, 'W'},
    {18, 'E'},
    {19, 'R'},
    {20, 'T'},
    {21, 'Y'},
    {22, 'U'},
    {23, 'I'},
    {24, 'O'},
    {25, 'P'},
    {26, '{'},
    {27, '}'},
    {28, 0},
    {29, 0},
    {30, 'A'},
    {31, 'S'},
    {32, 'D'},
    {33, 'F'},
    {34, 'G'},
    {35, 'H'},
    {36, 'J'},
    {37, 'K'},
    {38, 'L'},
    {39, ':'},
    {40, '"'},
    {41, '~'},
    {42, 0},
    {43, '|'},
    {44, 'Z'},
    {45, 'X'},
    {46, 'C'},
    {47, 'V'},
    {48, 'B'},
    {49, 'N'},
    {50, 'M'},
    {51, '<'},
    {52, '>'},
    {53, '?'},
    {54, 0},
    {55, '*'},
    {56, 0},
    {57, ' '}};

uint16_t lensor_map[][2] = {
    {0, LENSOR_KEY_NULL},
    {1, LENSOR_KEY_ESC},
    {2, LENSOR_KEY_DIGIT1},
    {3, LENSOR_KEY_DIGIT2},
    {4, LENSOR_KEY_DIGIT3},
    {5, LENSOR_KEY_DIGIT4},
    {6, LENSOR_KEY_DIGIT5},
    {7, LENSOR_KEY_DIGIT6},
    {8, LENSOR_KEY_DIGIT7},
    {9, LENSOR_KEY_DIGIT8},
    {10, LENSOR_KEY_DIGIT9},
    {11, LENSOR_KEY_DIGIT0},
    {12, LENSOR_KEY_MINUS},
    {13, LENSOR_KEY_EQUAL},
    {14, LENSOR_KEY_BACKSPACE},
    {15, LENSOR_KEY_TAB},
    {16, LENSOR_KEY_Q},
    {17, LENSOR_KEY_W},
    {18, LENSOR_KEY_E},
    {19, LENSOR_KEY_R},
    {20, LENSOR_KEY_T},
    {21, LENSOR_KEY_Y},
    {22, LENSOR_KEY_U},
    {23, LENSOR_KEY_I},
    {24, LENSOR_KEY_O},
    {25, LENSOR_KEY_P},
    {26, LENSOR_KEY_LEFTBRACE},
    {27, LENSOR_KEY_RIGHTBRACE},
    {28, LENSOR_KEY_ENTER},
    {29, LENSOR_KEY_LEFTCTRL},
    {30, LENSOR_KEY_A},
    {31, LENSOR_KEY_S},
    {32, LENSOR_KEY_D},
    {33, LENSOR_KEY_F},
    {34, LENSOR_KEY_G},
    {35, LENSOR_KEY_H},
    {36, LENSOR_KEY_J},
    {37, LENSOR_KEY_K},
    {38, LENSOR_KEY_L},
    {39, LENSOR_KEY_SEMICOLON},
    {40, LENSOR_KEY_BACKSLASH},
    {41, LENSOR_KEY_GRAVE},
    {42, LENSOR_KEY_LEFTSHIFT},
    {43, LENSOR_KEY_BACKSLASH},
    {44, LENSOR_KEY_Z},
    {45, LENSOR_KEY_X},
    {46, LENSOR_KEY_C},
    {47, LENSOR_KEY_V},
    {48, LENSOR_KEY_B},
    {49, LENSOR_KEY_N},
    {50, LENSOR_KEY_M},
    {51, LENSOR_KEY_COMMA},
    {52, LENSOR_KEY_DOT},
    {53, LENSOR_KEY_SLASH},
    {54, LENSOR_KEY_RIGHTSHIFT},
    {55, LENSOR_KEY_KPASTERISK},
    {56, LENSOR_KEY_LEFTALT},
    {57, LENSOR_KEY_SPACE}};

uint16_t TranslateScancode(uint8_t scancode) {
    // clear "break" bit
    scancode &= ~0x80;

    // Not within recognized range
    if (scancode >= sizeof(lensor_map) / sizeof(lensor_map[0]))
        return 0;

    return (char)lensor_map[scancode][1];
}

char Translate(uint8_t scancode, bool capital) {
    // clear "break" bit
    scancode &= ~0x80;

    // Not within recognized range
    if (scancode > sizeof(cap_map) / sizeof(cap_map[0]))
        return 0;

    if (capital)
        return (char)cap_map[scancode][1];
    return (char)map[scancode][1];
}

//[[deprecated("Kernel-level Input State Tracking Shouldn't Be Used")]]
Keyboard::KeyboardState State{};

void HandleDirectInput(char input) {
    if (!input) {
        std::print("[INPUT]: Refusing null input\n");
        return;
    }

    // Send user input to userspace!
    // Write to stdin of init process.
    if (SYSTEM and SYSTEM->init_process()) {
        Process* init = SYSTEM->init_process();
        auto fd = static_cast<ProcFD>(0);
        auto sysfd = init->FileDescriptors[fd];
        auto f = SYSTEM->virtual_filesystem().file(*sysfd);
        if (f)
            f->filesystem_driver()->write(f.get(), 0, sizeof(char), &input);

        return;
    }
    std::print("[INPUT]: No init process: cannot handle user input properly.\n");
}

uint32_t TranslateExtendedScancode(u8 scancode) {
    // std::print("[KEYB]: Translating scancode {} from extended set 0...\n", scancode);
    switch (scancode) {
        case 0x11:
            return LENSOR_KEY_RIGHTALT;
        case 0x14:
            return LENSOR_KEY_RIGHTCTRL;
        case 0x15:
            return LENSOR_KEY_PREVIOUSSONG;
        case 0x1f:
            return LENSOR_KEY_LEFTSUPER;
        case 0x21:
            return LENSOR_KEY_VOLUMEDOWN;
        case 0x23:
            return LENSOR_KEY_MUTE;
        case 0x27:
            return LENSOR_KEY_RIGHTSUPER;
        case 0x2b:
            return LENSOR_KEY_CALC;
        case 0x2f:
            return LENSOR_KEY_MENU;
        case 0x32:
            return LENSOR_KEY_VOLUMEUP;
        case 0x34:
            return LENSOR_KEY_PLAY;
        case 0x37:
            return LENSOR_KEY_POWER;
        case 0x3b:
            return LENSOR_KEY_STOP;
        case 0x3f:
            return LENSOR_KEY_SLEEP;
        case 0x40:
            return LENSOR_KEY_COMPUTER;
        case 0x48:
            return LENSOR_KEY_EMAIL;
        case 0x4a:
            return LENSOR_KEY_KPSLASH;
        case 0x4d:
            return LENSOR_KEY_NEXTSONG;
        case 0x50:
            return LENSOR_KEY_PLAYCD;
        case 0x5a:
            return LENSOR_KEY_KPENTER;
        case 0x5e:
            return LENSOR_KEY_WAKEUP;
        case 0x69:
            return LENSOR_KEY_END;
        case 0x6b:
            return LENSOR_KEY_LEFT;
        case 0x6c:
            return LENSOR_KEY_HOME;
        case 0x70:
            return LENSOR_KEY_INSERT;
        case 0x71:
            return LENSOR_KEY_DELETE;
        case 0x72:
            return LENSOR_KEY_DOWN;
        case 0x74:
            return LENSOR_KEY_RIGHT;
        case 0x75:
            return LENSOR_KEY_UP;
        case 0x7a:
            return LENSOR_KEY_PAGEDOWN;
        case 0x7d:
            return LENSOR_KEY_PAGEUP;
        default: /* no-op */
            break;
    }
    return LENSOR_KEY_NULL;
}

bool got_extended_prefix = false;
bool got_release = false;
void HandleScancodeInput(u8 scancode) {
    // Some keyboards send 0xf0 before the extended key scancode and
    // after 0xe0 prefix to signify release rather than setting bit 0x80.
    if (scancode == 0xf0) {
        // std::print("[KEYB]: Got release scancode...\n");
        got_release = true;
        return;
    }
    if (scancode == 0xe0 and not got_extended_prefix) {
        // std::print("[KEYB]: Got extended set 0 scancode...\n");
        got_extended_prefix = true;
        return;
    }

    uint32_t translated{};
    char translated_character{};

    // A release key may either have 0x80 bit set, or be prefixed with 0xf0.
    bool press = (not(scancode & 0x80)) and (not(got_release));
    // std::print("[KEYB] release bit={}, release prefix={}\n", (bool)scancode & 0x80, got_release);
    got_release = false;

    // TODO: Support other keyboard scancode translation layouts.
    if (got_extended_prefix)
        translated = TranslateExtendedScancode(scancode);
    else
        translated = TranslateScancode(scancode);

    got_extended_prefix = false;

    {
        // TODO: Support other keyboard scancode translation layouts.
        translated_character = Keyboard::QWERTY::Translate(
            scancode,
            State.LeftShift or State.RightShift or State.CapsLock);

        // std::print("[KEYB]: Got translated character {}\n", (int)translated_character);

        if (press and translated_character)
            HandleDirectInput(translated_character);
    }

    // Send Input Event
    Event e{};
    e.Type = EventType::KEYBOARD;
    auto* e_data = (EventData_KeyboardInput*)&e.Data;
    e_data->value = translated;
    e_data->press = press;
    gEvents.notify(e);

    // FIXME: Kernel Level Modifier Tracking Has Been Deprecated
    if (translated == LENSOR_KEY_LEFTSHIFT)
        State.LeftShift = press;
    if (translated == LENSOR_KEY_RIGHTSHIFT)
        State.RightShift = press;
    if (translated == LENSOR_KEY_CAPSLOCK)
        State.CapsLock ^= press;
}

}  // namespace QWERTY
}  // namespace Keyboard
