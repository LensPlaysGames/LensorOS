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

#ifndef LENSOR_OS_IDT_H
#define LENSOR_OS_IDT_H

#include <integers.h>

/// IA-32e System Descriptor Types
#define SYSTEM_DESCRIPTOR_TYPE_LDT              0b0010
#define SYSTEM_DESCRIPTOR_TYPE_TSS_AVAILABLE    0b1001
#define SYSTEM_DESCRIPTOR_TYPE_TSS_BUSY         0b1011
#define SYSTEM_DESCRIPTOR_TYPE_CALL_GATE        0b1100
#define SYSTEM_DESCRIPTOR_TYPE_INTERRUPT_GATE   0b1110
#define SYSTEM_DESCRIPTOR_TYPE_TRAP_GATE        0b1111

/// Interrupt Descriptor Table Type Attributes
#define IDT_TA_RING0 0b10000000
#define IDT_TA_RING3 0b11100000
#define IDT_TA_InterruptGate       IDT_TA_RING0 | SYSTEM_DESCRIPTOR_TYPE_INTERRUPT_GATE
#define IDT_TA_TrapGate            IDT_TA_RING0 | SYSTEM_DESCRIPTOR_TYPE_TRAP_GATE
#define IDT_TA_UserInterruptGate   IDT_TA_RING3 | SYSTEM_DESCRIPTOR_TYPE_INTERRUPT_GATE
#define IDT_TA_UserTrapGate        IDT_TA_RING3 | SYSTEM_DESCRIPTOR_TYPE_TRAP_GATE

/* Interrupt Descriptor Table Descriptor Entry
 *   Offset          --  The address of the interrupt handler function in memory.
 *   Selector        --  Selector for entry in Global Descriptor Table.
 *   IST             --  Interrupt Stack Table
 *   Type Attribute  --  Specifies how the interrupt will be handled by the CPU.
 *
 * Information taken from Intel Software Developer's Manual Volume 3A:
 *   Chapter 6.14 Exception and Interrupt Handling in 64-bit Mode:
 *   Figure 6-8 64-bit IDT Gate Descriptors
 *
 * For more info on System Descriptor Type, see:
 *   Intel Software Developer;s Manual Volume 3A:
 *   Chapter 3.5 System Descriptor Types:
 *   Table 3-2 System-Segment and Gate-Descriptor Types (IA-32e Mode column)
 */
struct IDTEntry {
    u16 Offset0;
    u16 Selector;
    /// 0b00000011
    ///         ==   IST
    ///   ======     Zero
    u8  IST;
    /// 0b00000000
    ///       ====   System Descriptor Type
    ///      =       Zero
    ///    ==        Descriptor Privilege Level (DPL)
    ///   =          Segment Present Flag
    u8  TypeAttribute;
    u16 Offset1;
    u32 Offset2;
    u32 Ignore;

    void SetOffset(u64 offset);
    u64 GetOffset();
} __attribute__((packed));;

/* Interrupt Descriptor Table Register
 *   Limit   --  The size of the descriptor table in bytes minus one.
 *   Offset  --  The address of the table in memory.
 */
struct IDTR {
    u16 Limit  { 0 };
    u64 Offset { 0 };

    IDTR() {}
    IDTR(u16 limit, u64 offset);

    void install_handler(u64 handler_address
                         , u8 entryOffset
                         , u8 typeAttribute = IDT_TA_InterruptGate
                         , u8 selector = 0x08
                         );

    void flush() {
        asm volatile ("lidt %0" :: "m"(*this));
    }
} __attribute__((packed));

extern IDTR gIDT;

#endif
