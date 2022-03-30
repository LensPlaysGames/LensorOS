    extern kmain

;# Virtual to physical and Physical to virtual address conversion.
;# These are needed because the kernel is loaded physically lower than it is linked.
%define V2P(a) ((a)-0xffffff8000000000)
%define P2V(a) ((a)+0xffffff8000000000)

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
GLOBAL prekernel_pml4
prekernel_pml4:
    dq V2P(prekernel_pml3) + (PAGE_PRESENT | PAGE_READ_WRITE)
%rep ENTRIES_PER_PAGE_TABLE - 2
    dq 0
%endrep
    dq V2P(prekernel_pml3) + (PAGE_PRESENT | PAGE_READ_WRITE | PAGE_GLOBAL)
prekernel_pml3:
    dq V2P(prekernel_pml2) + (PAGE_PRESENT | PAGE_READ_WRITE)
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

    ;# Copy boot info structure.
    mov rcx, 48
    mov rsi, rdi
    mov rdi, V2P(boot_info)
    cld
    rep movsb

    ;# Copy Framebuffer structure to stack, update pointer in boot info
    ;# 28 bytes of data (although padding causes it to be 32 bytes total).
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

    ;# Copy PSF1_Font structure to stack, update pointer in BootInfo.
    sub rsp, 16
    mov rcx, 16
    mov rsi, QWORD [V2P(boot_info) + 8]
    mov rdx, rsp
    mov rdi, rsp
    cld
    rep movsb
    mov rbx, rdx                ; RBX = bottom address of font structure
    mov rax, 0xffffff8000000000
    add rdx, rax
    mov QWORD [V2P(boot_info + 8)], rdx

    ;# Copy PSF1 Font Header to stack, update pointer in copied font structure on stack.
    sub rsp, 4
    mov rcx, 4
    mov rsi, QWORD [rbx]
    mov rdx, rsp
    mov rdi, rsp
    cld
    rep movsb
    mov rdi, rdx                ; RDI = bottom address of font header
    mov rax, 0xffffff8000000000
    add rdx, rax
    mov QWORD [rbx], rdx

    ;# Load PSF1 font glyph buffer.
    ;# Need to compare against font header mode field to know if 256 or 512 glyphs.
    ;# RBX + 2 = address of top of mode byte within font header
    mov rcx, 256
    xor rax, rax
    mov al, BYTE [rdi + 2]
    cmp al, 1
    jnz two_fifty_six_glyphs
    add rcx, 256
two_fifty_six_glyphs:
    ;# RCX = glyph count
    ;# glyph buffer size = glyph count * character size
    ;# RBX + 3 = address of top of character size byte within font header
    xor rax, rax
    mov al, BYTE [rdi + 3]      ; RAX = character size
    mul rcx                     ; RAX = total glyph buffer size
    sub rsp, rax
    mov rcx, rax
    mov rsi, QWORD [rbx + 8]
    mov rdx, rsp
    mov rdi, rsp
    cld
    rep movsb
    mov rax, 0xffffff8000000000
    add rdx, rax
    mov QWORD [rbx + 8], rdx

    ;# Copy EFI Memory Map to prekernel stack, update pointer in boot info.
    ;# Map size in 8 bytes is at boot_info + 24
    mov rcx, QWORD [V2P(boot_info) + 24] ; RCX = total map size in bytes
    sub rsp, rcx
    mov rsi, QWORD [V2P(boot_info) + 16]
    mov rdx, rsp
    mov rdi, rsp
    cld
    rep movsb
    mov rax, 0xffffff8000000000
    add rdx, rax
    mov QWORD [V2P(boot_info) + 16], rdx

    ;# Prepare prekernel Page Map Level One
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
    ;# At this point, higher half is mapped (no more manual address conversion)!
    mov rax, higher_half_init
    jmp rax

higher_half_init:
    ;# Move stack pointer to higher half
    mov rax, 0xffffff8000000000
    add rsp, rax
    mov rbp, rsp

;    ;# Remove identity mapping
;    mov rax, 0
;    mov [rel prekernel_pml4], rax
;
;    ;# Force page tables to update
;    mov rax, cr3
;    mov cr3, rax

    ;# Jump to kmain()
    mov rdi, boot_info
    mov rax, kmain
    call rax

    mov rax, hlt_forever
hlt_forever:
    hlt
    jmp [rax]
