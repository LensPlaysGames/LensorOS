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
;; along with LensorOS. If not, see <https://www.gnu.org/licenses

;;; System Call Assembly Wrapper for C++ System Call Functions
;;; Inspiration taking from: https://www.cs.usfca.edu/~benson/cs326/pintos/pintos/src

[BITS 64]

extern syscalls             ; Table of system call functions declared in "syscalls.h"
extern num_syscalls         ; Number of system call functions defined within syscalls table.

do_swapgs:
    cmp QWORD [rsp + 0x08], 0x08
    je skip_swap
    swapgs
skip_swap:
    ret

;;; System Call Handler
;;; Registers Used:
;;;   rax  --  System Call Code
;;;
;;; System Call Code: Index offset into syscalls table.
system_call_handler_asm:
;;; Do nothing if system call code is invalid:
;;; Syscall code invalid if greater than or equal to total number of syscalls.
    cmp rax, [rel num_syscalls]
    jae invalid_syscall
;;; Save CPU state to be restored after system call.
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
    push r9                     ; 6th argument, rest on stack in reverse order
    push r8                     ; 5th argument
    push rbp
    push rdi                    ; 1st argument
    push rsi                    ; 2nd argument
    push rdx                    ; 3rd argument
    push rcx                    ; 4th argument
    push rbx
    push rsp
;;; Execute the system call.
    mov r11, rdx                ; mul and friends clobber RDX, we need to save it.
    mov rbx, 8                  ; 8 = sizeof(pointer) in 64 bit.
    mul rbx                     ; Get byte offset within syscall table of syscall entry.
    mov r10, syscalls           ; Store address of syscalls function table.
    add r10, rax                ; Add offset to base address.
    mov rdx, r11                ; Restore clobbered RDX.
    mov r11, rsp
    call [rel r10]              ; Call function at syscalls table base address + syscall number offset.
;;; Restore CPU state, then return from interrupt.
    add rsp, 8                  ; Eat `rsp` off the stack.
    pop rbx
    pop rcx                     ; 1st return value
    pop rdx                     ; 2nd return value
    pop rsi                     ; 3rd return value
    pop rdi                     ; 4th return value
    pop rbp
    pop r8                      ; 5th return value
    pop r9                      ; 6th return value
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop fs
    pop gs
    add rsp, 8                  ; Eat `rax` off the stack.
    call do_swapgs
invalid_syscall:                ; If system call code is invalid, jump directly to exit.
    iretq                       ; iretq -> interrupt return quad word (64 bit)

GLOBAL system_call_handler_asm
