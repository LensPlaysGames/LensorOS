    .section .text
    .global _start
_start:
    ;# Stack frame linked list null entry.
    movq $0, %rbp
    pushq %rbp
    pushq %rbp
    movq %rsp, %rbp

    ;# Store argc/argv
    pushq %rsi
    pushq %rdi

    ;# Run global constructors.
    call _init

    popq %rdi
    popq %rsi

    ;# Run the code!
    call main

    ;# Call exit with return status from main as argument
    movq %rax, %rdi
    call exit
