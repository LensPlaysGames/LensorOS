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

extern kmain

;;# KERNEL_VIRTUAL -- linker provided symbol
extern KERNEL_VIRTUAL

;;# Paging
%define PAGE_SIZE               0x1000
%define PAGE_SIZE_LARGE         0x200000
%define ENTRIES_PER_PAGE_TABLE  512
%define PAGE_PRESENT            1
%define PAGE_READ_WRITE         1<<1
%define PAGE_USER_SUPER         1<<2
%define PAGE_LARGER_PAGES       1<<7

;;# Allocate known good stack
SECTION .bss
align PAGE_SIZE
prekernel_stack_bottom:
    ;;# 64 KiB stack
    resb 0x16000
prekernel_stack_top:

boot_info:
    resb 56

SECTION .data
align PAGE_SIZE
prekernel_pml4:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep
prekernel_pml3:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep
prekernel_pml2:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep
prekernel_pml1:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep

prekernel_pml3_ident:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep
prekernel_pml2_ident:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep

prekernel_pml3_high:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep
prekernel_pml2_high:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep
prekernel_pml1_high:
%rep ENTRIES_PER_PAGE_TABLE
    dq 0
%endrep


[BITS 64]
SECTION .text
GLOBAL _start
_start:
    ;# RDI -> void* :: kernel loaded at this address
    ;# RSI -> BootInfo* :: boot information
    ;#        Note: Could be (likely) located outside kernel image. Copy somewhere
    ;#        inside kernel image as soon as possible, and BEFORE switching page
    ;#        tables.

    cli

    mov r8, rdi

    ;# Move stack pointer to known good stack.
    lea rsp, [rel prekernel_stack_top]
    mov rbp, rsp

    ;# Copy boot info structure.
    mov rcx, 56
    ;# NOTE: RSI already contains BootInfo*
    lea rdi, [rel boot_info]
    cld
    rep movsb

    ;# Copy EFI Memory Map to prekernel stack, update pointer in boot info.
    ;# Map size in 8 bytes is at boot_info + 40
    mov rcx, QWORD [rel boot_info + 40] ; RCX = total map size in bytes
    sub rsp, rcx
    mov rdx, rsp
    mov rsi, QWORD [rel boot_info + 32]
    mov rdi, rsp
    cld
    rep movsb

    lea rdx, [rel KERNEL_VIRTUAL + rdx]
    sub rdx, r8 ;# RDX -> P2V(EFI_MEMORY_DESCRIPTOR*)
    mov QWORD [rel boot_info + 32], rdx

    ;# Since we don't know the physical location at compile time, we use R8
    ;# (the physical base) to dynamically map our static structures.

    ;# --- Map Kernel Image Identity Page Tables ---
    ;# We need to figure out which page table entries correspond to our
    ;# dynamic base (R8).
    ;# A 64-bit address is split into:
    ;#   PML4 Index = (address >> 39) & 0x1ff
    ;#   PML3 Index = (address >> 30) & 0x1ff
    ;#   PML2 Index = (address >> 21) & 0x1ff

    ;# --- Calculate PML4 Entry Offset ---
    mov rax, r8
    shr rax, 39
    and rax, 0x1ff                          ;# RAX = PML4 Index for dynamic base

    lea rbx, [rel prekernel_pml3]
    or  rbx, (PAGE_PRESENT | PAGE_READ_WRITE)
    lea rdi, [rel prekernel_pml4]
    mov [rdi + rax * 8], rbx                ;# PML4[Dynamic Index] -> prekernel_pml3

    ;# --- Calculate PML3 Entry Offset ---
    mov rax, r8
    shr rax, 30
    and rax, 0x1ff                          ;# RAX = PML3 Index for dynamic base

    lea rbx, [rel prekernel_pml2]
    or  rbx, (PAGE_PRESENT | PAGE_READ_WRITE)
    lea rdi, [rel prekernel_pml3]
    mov [rdi + rax * 8], rbx                ;# PML3[Dynamic Index] -> prekernel_pml2

    ;# --- Calculate PML2 Entry Offset ---
    mov rax, r8
    shr rax, 21
    and rax, 0x1ff                          ;# RAX = PML2 Index for dynamic base

    lea rbx, [rel prekernel_pml1]
    or  rbx, (PAGE_PRESENT | PAGE_READ_WRITE)
    lea rdi, [rel prekernel_pml2]
    mov [rdi + rax * 8], rbx                ;# PML2[Dynamic Index] -> prekernel_pml1

    ;# --- Populate Lower PML1 Frame Base ---
    ;# We align our starting physical mapping address to a 2 MiB boundary relative to R8
    mov rax, r8
    shr rax, 21
    shl rax, 21                             ;# Align RAX to the nearest 2MB boundary
    or  rax, (PAGE_PRESENT | PAGE_READ_WRITE)

    lea rdi, [rel prekernel_pml1]
    mov rcx, ENTRIES_PER_PAGE_TABLE
.loop_lower_pml1:
    mov [rdi], rax
    add rdi, 8
    add rax, PAGE_SIZE
    loop .loop_lower_pml1


    ;# --- Map 64MB Identity-Mapped Page Tables ---
    ;# This allows the physical memory manager to return allocated pages
    ;# within this mapped range, without worrying about the chicken-and-egg
    ;# problem of having to have the region mapped which requires allocating
    ;# page tables which requires writing to them which requires having them
    ;# mapped, etc.

    ;# PML4[256] -> PML3_IDENT
    lea rax, [rel prekernel_pml3_ident]
    or rax, (PAGE_PRESENT | PAGE_READ_WRITE)
    mov [rel prekernel_pml4 + 256 * 8], rax

    ;# PML3[0] -> PML2_IDENT
    lea rax, [rel prekernel_pml2_ident]
    or rax, (PAGE_PRESENT | PAGE_READ_WRITE)
    mov [rel prekernel_pml3_ident + 0 * 8], rax

    ;# Loop to fill the entire PML2 table with 512 Huge Pages (2 MiB each = 1 GiB total)
    mov rax, 0
    or  rax, (PAGE_PRESENT | PAGE_READ_WRITE | PAGE_LARGER_PAGES)
    lea rdi, [rel prekernel_pml2_ident]
    mov rcx, ENTRIES_PER_PAGE_TABLE
.loop_identity_pml2_huge:
    mov [rdi], rax
    add rdi, 8
    add rax, PAGE_SIZE_LARGE            ; Advance physical address by exactly 2 MiB
    loop .loop_identity_pml2_huge


    ;# --- Map Higher-Half Page Tables ---

    ;# PML4[511] -> PML3_HIGH
    lea rax, [rel prekernel_pml3_high]
    or  rax, (PAGE_PRESENT | PAGE_READ_WRITE)
    mov [rel prekernel_pml4 + 511 * 8], rax

    ;# PML3_HIGH[510] -> PML2_HIGH
    lea rax, [rel prekernel_pml2_high]
    or  rax, (PAGE_PRESENT | PAGE_READ_WRITE)
    mov [rel prekernel_pml3_high + 510 * 8], rax

    ;# PML2_HIGH[0] -> PML1_HIGH
    lea rax, [rel prekernel_pml1_high]
    or  rax, (PAGE_PRESENT | PAGE_READ_WRITE)
    mov [rel prekernel_pml2_high], rax

    ;# Populate High PML1 to map virtual higher-half to the dynamic physical base
    lea rdi, [rel prekernel_pml1_high]
    mov rcx, ENTRIES_PER_PAGE_TABLE
    ;# Map back to same dynamic physical base
    mov rax, r8
    or  rax, (PAGE_PRESENT | PAGE_READ_WRITE)
.loop_high:
    mov [rdi], rax
    add rdi, 8
    add rax, PAGE_SIZE
    loop .loop_high

    ;# Load prekernel Page Map Level Four (PML4).
    lea rax, [rel prekernel_pml4]
    mov cr3, rax

    ;# Move stack pointer to higher half
    lea rsp, [rel KERNEL_VIRTUAL + rsp]
    sub rsp, r8
    lea rbp, [rel KERNEL_VIRTUAL + rbp]
    sub rbp, r8

    mov rax, higher_half_init
    jmp rax

higher_half_init:
    ;# Jump to kmain()
    mov rdi, r8
    mov rsi, boot_info
    jmp kmain

hlt_forever:
    hlt
    jmp [rel hlt_forever]
