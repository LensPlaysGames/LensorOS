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
    bool locked { false };
};

class SpinlockLocker {
public:
    SpinlockLocker(Spinlock&);
    ~SpinlockLocker();

    inline void unlock();

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

#endif /* if !defined LENSOR_OS_SPIN_LOCK_H */
