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

#include <keyboard_scancode_translation.h>
#include <lensor/keys.h>

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
    {40, '\''},
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
    if (scancode >= 58)
        return 0;

    return (char)lensor_map[scancode][1];
}

char Translate(uint8_t scancode, bool capital) {
    // clear "break" bit
    scancode &= ~0x80;

    // Not within recognized range
    if (scancode > 58)
        return 0;

    if (capital)
        return (char)cap_map[scancode][1];
    return (char)map[scancode][1];
}

}  // namespace QWERTY
}  // namespace Keyboard
