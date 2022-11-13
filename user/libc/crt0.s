;;;# Copyright 2022, Contributors To LensorOS.
;;;# All rights reserved.
;;;#
;;;# This file is part of LensorOS.
;;;#
;;;# LensorOS is free software: you can redistribute it and/or modify
;;;# it under the terms of the GNU General Public License as published by
;;;# the Free Software Foundation, either version 3 of the License, or
;;;# (at your option) any later version.
;;;#
;;;# LensorOS is distributed in the hope that it will be useful,
;;;# but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;;;# GNU General Public License for more details.
;;;#
;;;# You should have received a copy of the GNU General Public License
;;;# along with LensorOS. If not, see <https://www.gnu.org/licenses

    .section .text
    .global _start
_start:
    ;;# Stack frame linked list null entry.
    xor %rbp, %rbp
    pushq %rbp
    movq %rsp, %rbp

    movq 8(%rsp), %rdi
    lea 16(%rsp), %rsi
    ;;# TODO: Dynamic linker will pass its deinit function in %rdx. It should
    ;;#       be registered with atexit() and called before the program exits.

    ;;# Push garbage to align the stack to 16 bytes.
    pushq %rax

    call __libc_run_main

    ;;# Call exit with return status from main as argument
    movq %rax, %rdi
    call _Exit
