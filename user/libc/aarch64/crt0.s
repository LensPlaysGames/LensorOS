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
.align 2
_start:
    // Stack frame linked list null entry.
    mov x29, #0
    mov x30, #0
    stp x29, x30, [sp, #-16]!
    mov x29, sp

    ldr x0, [sp, #24]
    add x1, sp, #32
    ;;# TODO: Dynamic linker will pass its deinit function. It should be
    ;;# registered with atexit() and called before the program exits.

    bl __libc_run_main

    bl _Exit
