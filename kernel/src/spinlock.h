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
    Spinlock Lock;

    inline void lock(Spinlock&);

    /* Compare and Swap
     *   If value at pointer is equal to expected value, 
     *     update value at pointer to new value.
     *
     *   This can be done atomically using the x86
     *     `cmpxchg` instruction with a `lock` prefix.
     *
     *   Returns `true` when lock has been acquired given
     *     expected_value_before_swap = `false`, `new_value` = `true`,
     *     and pointer_to_lock_flag points to `Lock.locked`.
     */
    inline bool compare_and_swap(bool* pointer_to_lock_flag, bool expected_value_before_swap, bool new_value);

    /* Test and Set
     *   Cache original value of lock, then set the value to the new value.
     *   If the old value is equal to `false`, the lock has been acquired.
     *
     *   This can be done atomically using the x86
     *     `xchg` instruction with a `lock` prefix.
     *
     *   Returns `false` when lock has been acquired given
     *     new_value = `true` and pointer_to_lock_flag points to `Lock.locked`.
     */
    inline bool test_and_set(bool* pointer_to_lock_flag, bool new_value);
};

#endif /* if !defined LENSOR_OS_SPIN_LOCK_H */
