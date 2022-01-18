#include "keyboard_scancode_translation.h"

namespace QWERTY {
    const char ASCII[] = {
         0 ,  0 , '1', '2',
        '3', '4', '5', '6',
        '7', '8', '9', '0',
        '-', '=',  0 ,  0 ,
        'q', 'w', 'e', 'r',
        't', 'y', 'u', 'i',
        'o', 'p', '[', ']',
         0 ,  0 , 'a', 's',
        'd', 'f', 'g', 'h',
        'j', 'k', 'l', ';',
        '\'','`',  0 , '\\',
        'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',',
        '.', '/',  0 , '*',
         0 , ' '
    };

	bool isalnum(uint8_t scancode) {
		return (scancode >= 16 && scancode <= 25)
			|| (scancode >= 30 && scancode <= 38)
			|| (scancode >= 44 && scancode <= 50);
	}

	char Translate(uint8_t scancode, bool capital) {
		if (scancode > 58) return 0;
		if (capital && isalnum(scancode)) {
			return ASCII[scancode] - 32;
		}
		return ASCII[scancode];
	}
}
