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
    // Also known as "atomic exchange"
    bool test_and_set(bool* pointer_to_lock_flag, bool new_value);
};

class SpinlockLocker {
public:
    SpinlockLocker(Spinlock&);
    ~SpinlockLocker();

    inline void unlock();

private:
    Spinlock Lock;

    inline void lock(Spinlock&);
};

#endif /* if !defined LENSOR_OS_SPIN_LOCK_H */
