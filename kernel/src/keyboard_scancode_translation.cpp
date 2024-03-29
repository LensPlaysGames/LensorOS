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

namespace Keyboard {
    namespace QWERTY {
        uint8_t map[][2] = {
            {0, 0}     ,{1, 0},
            {2,'1'}    ,{3,'2'}    ,{4,'3'}     ,{5,'4'}    ,
            {6,'5'}    ,{7,'6'}    ,{8,'7'}     ,{9,'8'}    ,
            {10,'9'}   ,{11,'0'}   ,{12,'-'}    ,{13,'='}   ,
            {14, '\b'} ,{15, 0}    ,
            {16,'q'}   ,{17,'w'}   ,{18,'e'}    ,{19,'r'}   ,
            {20,'t'}   ,{21,'y'}   ,{22,'u'}    ,{23,'i'}   ,
            {24,'o'}   ,{25,'p'}   ,{26,'['}    ,{27,']'}   ,
            {28, '\n'} ,{29, 0}    ,
            {30,'a'}   ,{31,'s'}   ,{32,'d'}    ,{33,'f'}   ,
            {34,'g'}   ,{35,'h'}   ,{36,'j'}    ,{37,'k'}   ,
            {38,'l'}   ,{39,';'}   ,{40,'\''}   ,{41,'`'}   ,
            {42, 0}    ,{43,'\\'}  ,
            {44,'z'}   ,{45,'x'}   ,{46,'c'}    ,{47,'v'}   ,
            {48,'b'}   ,{49,'n'}   ,{50,'m'}    ,{51,','}   ,
            {52,'.'}   ,{53,'/'}   ,{54, 0}     ,{55,'*'}   ,
            {56, 0}    ,{57,' '}
        };

        uint8_t cap_map[][2] = {
            {0, 0}     ,{1, 0},
            {2,'!'}    ,{3,'@'}    ,{4,'#'}     ,{5,'$'}    ,
            {6,'%'}    ,{7,'^'}    ,{8,'&'}     ,{9,'*'}    ,
            {10,'('}   ,{11,')'}   ,{12,'_'}    ,{13,'+'}   ,
            {14, 0}    ,{15, 0}    ,
            {16,'Q'}   ,{17,'W'}   ,{18,'E'}    ,{19,'R'}   ,
            {20,'T'}   ,{21,'Y'}   ,{22,'U'}    ,{23,'I'}   ,
            {24,'O'}   ,{25,'P'}   ,{26,'{'}    ,{27,'}'}   ,
            {28, 0}    ,{29, 0}    ,
            {30,'A'}   ,{31,'S'}   ,{32,'D'}    ,{33,'F'}   ,
            {34,'G'}   ,{35,'H'}   ,{36,'J'}    ,{37,'K'}   ,
            {38,'L'}   ,{39,':'}   ,{40,'"'}    ,{41,'~'}   ,
            {42, 0}    ,{43,'|'}   ,
            {44,'Z'}   ,{45,'X'}   ,{46,'C'}    ,{47,'V'}   ,
            {48,'B'}   ,{49,'N'}   ,{50,'M'}    ,{51,'<'}   ,
            {52,'>'}   ,{53,'?'}   ,{54, 0}     ,{55,'*'}   ,
            {56, 0}    ,{57,' '}
        };

        char Translate(uint8_t scancode, bool capital) {
            if (scancode > 58) { return 0; }
            if (capital) { return (char)cap_map[scancode][1]; }
            return (char)map[scancode][1];
        }
    }
}
