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

bool Spinlock::test_and_set(bool* pointer_to_lock_flag, bool new_value) {
    bool old_value;
    asm volatile ("lock xchg %0, %1" : "=a" (old_value) : "m" (pointer_to_lock_flag), "r" (new_value));
    return old_value;
}

class SpinlockLocker {
public:
    SpinlockLocker(Spinlock&);
    ~SpinlockLocker();

    inline void unlock();

private:
    Spinlock Lock;

    inline void lock(Spinlock&);
};

SpinlockLocker::SpinlockLocker(Spinlock& lock) {
    Lock = lock;
    lock.locked = false;
}

SpinlockLocker::~SpinlockLocker() {
    unlock();
}

void SpinlockLocker::lock(Spinlock& lock) {
    // Spin CPU until atomic exchange returns false, meaning lock has been acquired.
    while (lock.test_and_set(&lock.locked, true));
}

void SpinlockLocker::unlock() {
    Lock.locked = false;
}

#endif /* if !defined LENSOR_OS_SPIN_LOCK_H */
