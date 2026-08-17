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

;; A pointer to a function that increments timer ticks by one.
extern timer_tick
;; The unified C++ scheduling logic
extern switch_process

; ==============================================================================
; VOLUNTARY YIELD WRAPPER
; ==============================================================================
global yield
yield:
    ; 1. Allocate the entire CPUState block (184 bytes minus the 8 bytes 'call' used)
    sub rsp, 184

    ; 2. Immediately save pristine RAX and RBX into their exact final struct slots
    ; before modifying them for layout setup.
    mov [rsp + 136], rax  ; Save pristine RAX at its exact struct offset
    mov [rsp + 8], rbx    ; Save pristine RBX at its exact struct offset

    ; 3. Now RAX and RBX are completely safe. We can use them as scratch registers.
    mov rax, [rsp + 176]        ; Fetch the return RIP (pushed by 'call yield')
    mov [rsp + 144], rax        ; Frame.ip = Return RIP

    lea rbx, [rsp + 184]        ; Calculate pristine RSP from before 'call yield'
    mov [rsp + 168], rbx        ; Frame.sp = Pristine original RSP

    mov [rsp + 176], qword 0x10 ; Frame.ss = 0x10
    mov [rsp + 152], qword 0x08 ; Frame.cs = 0x08

    pushfq
    pop rbx                     ; Use safe RBX to grab flags
    mov [rsp + 160], rbx        ; Frame.rflags

    ; Fill out the remaining general-purpose registers, excluding rax and rbx.
    mov [rsp + 128], gs
    mov [rsp + 120], fs
    mov [rsp + 112], r15
    mov [rsp + 104], r14
    mov [rsp + 96], r13
    mov [rsp + 88], r12
    mov [rsp + 80], r11
    mov [rsp + 72], r10
    mov [rsp + 64], r9
    mov [rsp + 56], r8
    mov [rsp + 48], rbp
    mov [rsp + 40], rdi
    mov [rsp + 32], rsi
    mov [rsp + 24], rdx
    mov [rsp + 16], rcx
    mov [rsp + 0], qword 0 ; Uniform placeholder for the struct's RSP slot

    ; --- HAND CONTROL TO THE SCHEDULER ---
    ; Pass the pointer to this completed CPUState struct (RSP) into C++
    mov rdi, rsp                ; Argument 1 (RDI) = CPUState* cpu
    call switch_process

    jmp switch_context_asm

; ==============================================================================
; THE HARDWARE REGULAR TIMER INTERRUPT WRAPPER
; ==============================================================================
global irq0_handler
irq0_handler:
    ; Hardware already pushed SS, RSP, RFLAGS, CS, and RIP.
    ; Push general purpose registers
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

    call [rel timer_tick]

    pop rsp
    push rsp

    ; --- HAND CONTROL TO THE SCHEDULER ---
    ; Pass the pointer to this completed CPUState struct (RSP) into C++
    mov rdi, rsp            ; Argument 1 (RDI) = CPUState* cpu
    call switch_process

    ; --- NOTIFY THE HARDWARE CONTROLLER ---
    ; Send End of Interrupt (EOI) command to the PIC/APIC controller
    ; This enables future hardware interrupts to fire safely.
    push rax
    mov al, 0x20            ; 0x20 = EOI command code
    out 0x20, al            ; Send to Master PIC command port
    pop rax

    ; --- LOAD NEW CONTEXT AND JUMP AWAY ---
    jmp switch_context_asm

; ==============================================================================
; THE UNIFIED CONTEXT SWITCH TRAMPOLINE
; ==============================================================================
global switch_context_asm
switch_context_asm:
    ; RAX = Process*

    ; RDI = process->kernel_stack
    mov rdi, qword [rax + 56]
    ; RSI = process->CR3
    mov rsi, qword [rax + 232]
    ; RDX = &process->CPUExtra
    lea rdx, qword [rax + 240]
    ; RCX = process->CPUExtraSet
    mov rcx, qword [rax + 752]

    ; Load target stack pointer
    mov rsp, rdi

    ; Change page map register
    mov rax, cr3
    cmp rax, rsi
    je .skip_cr3_flush
    mov cr3, rsi
.skip_cr3_flush:

    ; Reset segment registers to kernel data segment
    mov ax, 0x10
    mov es, ax
    mov ds, ax
    mov fs, ax
    mov gs, ax

    ; Restore FPU state if active
    test cl, cl
    jz .skip_fpu_restore
    fxrstor64 [rdx]
.skip_fpu_restore:

    ; Pop registers from target CPUState struct
    add rsp, 8 ; skip rsp (wouldn't be able to pop now would we)
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
    ;; skip fs, gs (avoid popping user selectors for kernel mode)
    add rsp, 16
    pop rax

    ; Return to target process frame
    iretq
