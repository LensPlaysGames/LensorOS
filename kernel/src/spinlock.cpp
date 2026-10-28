/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#include <spinlock.h>
#include <stdint.h>

bool SpinlockLocker::compare_and_swap_lock() {
#if defined(x86_64)
    /* Inline Assembly:
     * |- Desc:
     * |  |- `lock`                  -- Ensures that the following instruction runs atomically.
     * |  |- `cmpxchg <op0>, <op1>`  -- Compare 'A' register with <op0>; if equal, swap <op1> and <op0>.
     * |  `- `sete <op0>`            -- Assign <op0> to the 'equal' flag of the CPU (ZF==1).
     * |- Outputs:
     * |  |- "=q"  -- Write-only low-byte-accessible register; return value
     * |  `- "=m"  -- Write-only memory address; lock flag
     * `- Inputs:
     *    |- "r"  -- Any register; new value to swap to if expected  -- "%0"
     *    |- "m"  -- Memory address; lock flag  -- "%1"
     *    `- "a"  -- The 'A' register; lock flag will be set only if it matches this value  -- "%2"
     */
    bool ret;
    asm volatile(
        "lock cmpxchg %2, %1\n"
        "sete %0\n"
        : "=q"(ret), "=m"(Lock.locked)
        : "r"(true), "m"(Lock.locked), "a"(false)
        : "memory");
    return ret;
#elif defined(aarch64)

    uint32_t actual;
    uint32_t store_failed;

    asm volatile(
        "1:\n\t"
        "ldaxrb  %w[actual], [%[ptr]]\n\t"  // Load current byte value with Acquire ordering
        "cmp     %w[actual], #0\n\t"        // Check if it matches expected (false/0)
        "bne     2f\n\t"                    // If not 0 (already locked), exit immediately

        "stlxrb  %w[failed], %w[desired], [%[ptr]]\n\t"  // Try storing desired (true/1) with Release ordering
        "cbnz    %w[failed], 1b\n\t"                     // If store failed (bus contention), retry loop

        "2:\n\t"
        : [actual] "=&r"(actual), [failed] "=&r"(store_failed), "+m"(Lock.locked)
        : [ptr] "r"(&Lock.locked), [desired] "r"(1)
        : "memory", "cc");

    return actual == 0;

#else
#error "kernel spinlock: unsupported arch"
#endif
}

bool SpinlockLocker::test_and_set_lock() {
#if defined(x86_64)
    /* In x86, when an instruction is prefixed with `lock`,
     *   it is guaranteed to be run atomically (thread-safe).
     * The `xchg` instruction sets a memory address or register
     *   to a new value while returning the old value in RAX.
     * It can be determined that the lock was free (unlocked) and is
     *   acquired (locked) by currently executing code by testing
     *   that the old value that is returned is equal to false.
     */

    /* Inline Assembly:
     * |- Desc: Old value is set to lock, then lock is set to new value.
     * |  |- `lock`               -- Ensures that the following instruction runs atomically.
     * |  |- `xchg <op0>, <op1>`  -- Exchanges the contents of the two operands <op1> into <op0>.
     * |- Output(s)
     * |  `- "=a"  -- Write-only; output in RAX
     * `- Input(s)
     *    |- "m"  -- Memory Address; lock flag
     *    `- "r"  -- Any register; new lock flag value
     */
    bool old_value;
    asm volatile("lock xchg %0, %1" : "=a"(old_value) : "m"(Lock.locked), "r"(true));
    return old_value;
#elif defined(aarch64)

    uint32_t old_value;
    uint32_t store_failed;

    asm volatile(
        "1:\n\t"
        "ldaxrb  %w[old], [%[ptr]]\n\t"                  // Load old byte value with Acquire ordering
        "stlxrb  %w[failed], %w[desired], [%[ptr]]\n\t"  // Attempt to store true (1) with Release ordering
        "cbnz    %w[failed], 1b\n\t"                     // If store failed (bus contention), retry loop
        : [old] "=&r"(old_value), [failed] "=&r"(store_failed), "+m"(Lock.locked)
        : [ptr] "r"(&Lock.locked), [desired] "r"(1)
        : "memory");

    return (bool)old_value;
#else
#error "kernel spinlock: unsupported arch"
#endif
}

SpinlockLocker::SpinlockLocker(Spinlock& l)
    : Lock(l) {
    lock();
}

SpinlockLocker::~SpinlockLocker() {
    unlock();
}

void SpinlockLocker::lock() {
    while (compare_and_swap_lock() == true);
}

void SpinlockLocker::unlock() {
    Lock.locked = false;
}
