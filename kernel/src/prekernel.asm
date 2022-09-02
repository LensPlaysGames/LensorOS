    ; Copyright 2022, Contributors To LensorOS.
; All rights reserved.

; This file is part of LensorOS.

; LensorOS is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

; LensorOS is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
; GNU General Public License for more details.

; You should have received a copy of the GNU General Public License
; along with LensorOS. If not, see <https://www.gnu.org/licenses 
    extern kmain

;# Virtual to physical and Physical to virtual address conversion.
;# These are needed because the kernel is loaded physically lower than it is linked.
%define V2P(a) ((a)-0xffffffff80000000)
%define P2V(a) ((a)+0xffffffff80000000)

;# Paging
%define PAGE_SIZE               0x1000
%define ENTRIES_PER_PAGE_TABLE  512
%define PAGE_PRESENT            1
%define PAGE_READ_WRITE         1<<1
%define PAGE_USER_SUPER         1<<2
%define PAGE_WRITE_THROUGH      1<<3
%define PAGE_CACHE_DISABLED     1<<4
%define PAGE_ACCESSED           1<<5
%define PAGE_DIRTY              1<<6
%define PAGE_LARGER_PAGES       1<<7
%define PAGE_GLOBAL             1<<8
%define PAGE_NX                 1<<63

;# Allocate known good stack
SECTION .bss
align 0x1000
prekernel_stack_bottom:
    resb 0x16000
prekernel_stack_top:

boot_info:
    resb 48

SECTION .data
align 0x1000
prekernel_pml4:
    dq V2P(prekernel_pml3) + (PAGE_PRESENT | PAGE_READ_WRITE)
%rep ENTRIES_PER_PAGE_TABLE - 2
    dq 0
%endrep
    dq V2P(prekernel_pml3_high) + (PAGE_PRESENT | PAGE_READ_WRITE | PAGE_GLOBAL)
prekernel_pml3:
    dq V2P(prekernel_pml2) + (PAGE_PRESENT | PAGE_READ_WRITE)
%rep ENTRIES_PER_PAGE_TABLE - 1
    dq 0
%endrep
prekernel_pml3_high:
%rep ENTRIES_PER_PAGE_TABLE - 2
    dq 0
%endrep
    dq V2P(prekernel_pml2) + (PAGE_PRESENT | PAGE_READ_WRITE | PAGE_GLOBAL)
    dq 0
prekernel_pml2:
    dq V2P(prekernel_pml1) + (PAGE_PRESENT | PAGE_READ_WRITE | PAGE_GLOBAL)
%rep ENTRIES_PER_PAGE_TABLE - 1
    dq 0
%endrep
prekernel_pml1:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep

[BITS 64]
SECTION .text
GLOBAL _start
_start:
    cli

    ;# Move stack pointer to known good stack.
    mov rsp, V2P(prekernel_stack_top)
    mov rbp, rsp

    ;# Copy boot info structure.
    mov rcx, 48
    mov rsi, rdi
    mov rdi, V2P(boot_info)
    cld
    rep movsb

    ;# Copy EFI Memory Map to prekernel stack, update pointer in boot info.
    ;# Map size in 8 bytes is at boot_info + 24
    mov rcx, QWORD [V2P(boot_info) + 24] ; RCX = total map size in bytes
    sub rsp, rcx
    mov rsi, QWORD [V2P(boot_info) + 16]
    mov rdx, rsp
    mov rdi, rsp
    cld
    rep movsb
    mov rax, 0xffffffff80000000
    add rdx, rax
    mov QWORD [V2P(boot_info) + 16], rdx

    ;# Prepare an identity mapped Page Map Level One (PML1)
    mov rax, V2P(prekernel_pml1)
%assign i 0
%rep ENTRIES_PER_PAGE_TABLE
    mov QWORD [rax], (i << 12) + (PAGE_PRESENT | PAGE_READ_WRITE | PAGE_GLOBAL)
    add rax, 8
%assign i i+1
%endrep

    ;# Load prekernel Page Map Level Four (PML4).
    mov rax, V2P(prekernel_pml4)
    mov cr3, rax

    mov rax, higher_half_init
    jmp rax

higher_half_init:
    ;# Move stack pointer to higher half
    mov rax, 0xffffffff80000000
    add rsp, rax
    mov rbp, rsp

    ;# Jump to kmain()
    mov rdi, boot_info
    mov rax, kmain
    call rax

    mov rax, hlt_forever
hlt_forever:
    hlt
    jmp [rax]
