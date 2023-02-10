;; Copyright 2022, Contributors To LensorOS.
;; All rights reserved.
;;
;; This file is part of LensorOS.
;;
;; LensorOS is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; LensorOS is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with LensorOS. If not, see <https://www.gnu.org/licens

[BITS 64]
;; External symbols provided in `scheduler.h` and `scheduler.cpp`
;; A pointer to task switching handler function.
extern scheduler_switch_process
;; A pointer to a function that increments timer ticks by one.
extern timer_tick
do_swapgs:
    cmp QWORD [rsp + 0x8], 0x8
    je skip_swap
    swapgs
skip_swap:
    ret

GLOBAL irq0_handler
irq0_handler:
;; `iretq` arguments already on the stack:
;; |-- Data Segment Selector
;; |-- Old Stack Pointer (RSP)
;; |-- Flags Register (RFLAGS)
;; |-- Code Segment Selector
;; `-- Instruction Pointer (RIP)
;;; SAVE CPU STATE ON STACK
    call do_swapgs
    push rax
    push gs
    push fs
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rsp
;;; INCREMENT SYSTEM TIMER TICKS
    call [rel timer_tick]
;;; CALL C++ FUNCTION; ARGUMENT IN `rdi`
;;#; FIXME: Does this only work because RSP is what's at the stack pointer? Should this be `lea` instead?
    mov rdi, rsp
    call [rel scheduler_switch_process]
;;; END INTERRUPT
    mov ax, 0x20                ; 0x20 = PIC_EOI
    out 0x20, al                ; 0x20 = PIC1_COMMAND port
yield_asm_impl:
;;; RESTORE CPU STATE FROM STACK
    add rsp, 8                  ; Eat `rsp` off of stack.
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop fs
    pop gs
    pop rax
    call do_swapgs
    iretq

GLOBAL yield_asm
yield_asm:
    mov rsp, rdi
    jmp yield_asm_impl
