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
    movq $0, %rbp
    pushq %rbp
    pushq %rbp
    movq %rsp, %rbp

    movl 16(%rbp), %edi
    movq 20(%rbp), %rsi
    movq $0, %rdx
    call __libc_run_main

    ;;# Call exit with return status from main as argument
    movq %rax, %rdi
    call _Exit
