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

#ifndef LENSOR_OS_SPIN_LOCK_H
#define LENSOR_OS_SPIN_LOCK_H

// A simple thread-safe locking mechanism inspired by the following page:
// https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf

class Spinlock {
    friend class SpinlockLocker;

public:
    Spinlock() : locked(false) {}
    bool get() { return locked; }

private:
    volatile bool locked { false };
};

class SpinlockLocker {
public:
    explicit SpinlockLocker(Spinlock&);
    ~SpinlockLocker();

    void unlock();

private:
    Spinlock& Lock;

    inline void lock();

    /* Compare and Swap
     *   If value at pointer is equal to expected value,
     *     update value at pointer to new value.
     *
     *   This can be done atomically using the x86
     *     `cmpxchg` instruction with a `lock` prefix.
     *
     *   Returns `true` when lock has been acquired.
     */
    inline bool compare_and_swap_lock();

    /* Test and Set
     *   Cache original value of lock, then set the value to the new value.
     *   If the old value is equal to `false`, the lock has been acquired.
     *
     *   This can be done atomically using the x86
     *     `xchg` instruction with a `lock` prefix.
     *
     *   Returns `false` when lock has been acquired.
     */
    inline bool test_and_set_lock();
};

#endif /* if not defined LENSOR_OS_SPIN_LOCK_H */
