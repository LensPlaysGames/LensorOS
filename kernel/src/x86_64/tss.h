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

#ifndef LENSOR_OS_TSS_H
#define LENSOR_OS_TSS_H

#include <integers.h>

struct TSSEntry {
    enum class RSP {
        Zero = 0,
        One = 1,
        Two = 2,
    };

    enum class IST {
        One = 1,
        Two = 2,
        Three = 3,
        Four = 4,
        Five = 5,
        Six = 6,
        Seven = 7,
    };

    u32 Reserved0 { 0 };
    u32 l_RSP0 { 0 };
    u32 h_RSP0 { 0 };
    u32 l_RSP1 { 0 };
    u32 h_RSP1 { 0 };
    u32 l_RSP2 { 0 };
    u32 h_RSP2 { 0 };
    u64 Reserved1 { 0 };
    u32 l_IST1 { 0 };
    u32 h_IST1 { 0 };
    u32 l_IST2 { 0 };
    u32 h_IST2 { 0 };
    u32 l_IST3 { 0 };
    u32 h_IST3 { 0 };
    u32 l_IST4 { 0 };
    u32 h_IST4 { 0 };
    u32 l_IST5 { 0 };
    u32 h_IST5 { 0 };
    u32 l_IST6 { 0 };
    u32 h_IST6 { 0 };
    u32 l_IST7 { 0 };
    u32 h_IST7 { 0 };
    u64 Reserved2 { 0 };
    u16 Reserved3 { 0 };
    u16 IOMapBaseAddress { 0 };

    void set_stack(u64 rsp, RSP stackToSet = RSP::Zero) {
        switch (stackToSet) {
        default:
        case RSP::Zero:
            l_RSP0 = rsp;
            h_RSP0 = rsp >> 32;
            break;
        case RSP::One:
            l_RSP1 = rsp;
            h_RSP1 = rsp >> 32;
            break;
        case RSP::Two:
            l_RSP2 = rsp;
            h_RSP2 = rsp >> 32;
            break;
        }
    }

    void set_ist(u64 ist, IST indexToSet = IST::One) {
        switch(indexToSet) {
        default:
        case IST::One:
            l_IST1 = ist;
            h_IST1 = ist >> 32;
            break;
        case IST::Two:
            l_IST2 = ist;
            h_IST2 = ist >> 32;
            break;
        case IST::Three:
            l_IST3 = ist;
            h_IST3 = ist >> 32;
            break;
        case IST::Four:
            l_IST4 = ist;
            h_IST4 = ist >> 32;
            break;
        case IST::Five:
            l_IST5 = ist;
            h_IST5 = ist >> 32;
            break;
        case IST::Six:
            l_IST6 = ist;
            h_IST6 = ist >> 32;
            break;
        case IST::Seven:
            l_IST7 = ist;
            h_IST7 = ist >> 32;
            break;
        }
    }
} __attribute__((packed));

namespace TSS {
    void initialize();
}

extern "C" void jump_to_userland_function(void* functionAddress);

#endif
