extern kmain

;# Virtual to physical and Physical to virtual address conversion.
;# These are needed because the kernel is loaded physically lower than it is linked.
%define V2P(a) ((a)-0xffffff8000000000)
%define P2V(a) ((a)+0xffffff8000000000)

;# Allocate known good stack
SECTION .bss
align 4096
prekernel_stack_bottom:
    resb 4096
prekernel_stack_top:

boot_info:
    resb 48

SECTION .data
;# Setup a page table level four where higher half is mapped.
align 4096
GLOBAL prekernel_pml4
%define PAGE_SIZE           4096
%define PAGE_PRESENT        1
%define PAGE_READ_WRITE     1<<1
%define PAGE_USER_SUPER     1<<2
%define PAGE_WRITE_THROUGH  1<<3
%define PAGE_CACHE_DISABLED 1<<4
%define PAGE_ACCESSED       1<<5
%define PAGE_DIRTY          1<<6
%define PAGE_LARGER_PAGES   1<<7
%define PAGE_GLOBAL         1<<8
%define PAGE_NX             1<<63
%define ENTRIES_PER_PAGE_TABLE 512
prekernel_pml4:
dq V2P(prekernel_pml3) + (PAGE_PRESENT | PAGE_READ_WRITE)
%rep ENTRIES_PER_PAGE_TABLE - 2
    dq 0
%endrep
dq V2P(prekernel_pml3) + (PAGE_PRESENT | PAGE_READ_WRITE | PAGE_GLOBAL)
prekernel_pml3:
dq V2P(prekernel_pml2) + (PAGE_PRESENT | PAGE_READ_WRITE | PAGE_GLOBAL)
%rep ENTRIES_PER_PAGE_TABLE - 1
    dq 0
%endrep
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

    ;# Copy boot info structure...
    ;# This doesn't actually work as there is pointers within (see below).
    mov rcx, 48
    mov rsi, rdi
    mov rdi, V2P(boot_info)
    cld
    rep movsb
    
    ;# TODO FIXME: This is the issue, as of right now.
    ;# BootInfo
    ;# |-- Framebuffer* <- Just a pointer! Would need to copy data from this.
    ;# |-- PSF1_Font*   <- See above -^
    ;# |   `-- PSF1_Header <- Also would need copied.
    ;# |-- EFI_MEMORY_DESCRIPTOR* <- An entire array of memory descriptors to copy, or something.
    ;# `-- RSDP2 Header <- Pointer, data needs copied.
    ;#
    ;# Obviously, it would be great if copying wasn't necessary,
    ;# but I'm having a hard time figuring out exactly how to map
    ;# the higher half for the kernel while also preserving the
    ;# data the bootloader loaded in the lower memory...

    ;# Preliminary idea: allocate space for stuffs on stack, move pointer in BootInfo structure to there.

    ;# Copy Framebuffer structure to stack, update pointer in boot info
    ;# 28 bytes of data (although padding causes it to be 32 bytes total)
    sub rsp, 28
    mov rcx, 28
    mov rsi, QWORD [V2P(boot_info)]
    mov rdx, rsp
    mov rdi, rsp
    cld
    rep movsb
    mov rax, 0xffffff8000000000
    add rdx, rax
    mov QWORD [V2P(boot_info)], rdx

    ;# Ensure CR4.PAE is set, enabling Page Address Extension.
    mov rax, cr4
    or rax, 1 << 5
    mov cr4, rax

    ;# Prepare prekernel Page Map Level One
    mov rax, V2P(prekernel_pml1)
%assign i 0
%rep ENTRIES_PER_PAGE_TABLE
    mov QWORD [rax], (i << 12) + (PAGE_PRESENT | PAGE_READ_WRITE | PAGE_GLOBAL)
    add rax, 8
%assign i i+1
%endrep

    ;# Load prekernel Page Map Level Four (PML4)
    mov rax, V2P(prekernel_pml4)
    mov cr3, rax
    ;# At this point, higher half is mapped (no more manual address conversion)!
    ;# This may be confirmed using the `info mem` QEMU monitor command after booting.
    
    mov rax, higher_half_init
    jmp rax

higher_half_init:
    ;# Move stack pointer to higher half
    mov rax, 0xffffff8000000000
    add rsp, rax
    mov rbp, rsp

    ;# Remove identity mapping
    mov rax, 0
    mov [rel prekernel_pml4], rax

    ;# Force page tables to update
    mov rax, cr3
    mov cr3, rax

    ;# Jump to kmain()
    mov rdi, boot_info
    mov rax, kmain
    call rax

    mov rax, hlt_forever
hlt_forever:
    hlt
    jmp [rax]
