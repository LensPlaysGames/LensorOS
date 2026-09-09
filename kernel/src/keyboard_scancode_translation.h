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

#ifndef LENSOR_OS_KB_SCANCODE_TRANSLATION_H
#define LENSOR_OS_KB_SCANCODE_TRANSLATION_H

#include <integers.h>

namespace Keyboard {
namespace QWERTY {

void HandleScancodeInput(u8 scancode);

/// @return LENSOR_KEY_* value corresponding to the given scancode.
uint16_t TranslateScancode(uint8_t scancode);

/// @return LENSOR_KEY_* value corresponding to the given scancode *as if
/// it were prefixed with the extended set zero prefix (0xe0, usually)*.
uint32_t TranslateExtendedScancode(u8 scancode);

/// @return ASCII Text Character corresponding to the given scancode within the QWERTY namespace.
char Translate(u8 scancode, bool capital);

// Writes to stdin driver, basically. For simplified text-based operation.
void HandleDirectInput(char input);

}  // namespace QWERTY
}  // namespace Keyboard

#endif
