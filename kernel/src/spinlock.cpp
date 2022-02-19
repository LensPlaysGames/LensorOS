#include "spinlock.h"

bool Spinlock::test_and_set(bool* pointer_to_lock_flag, bool new_value) {
    bool old_value;
    asm volatile ("lock xchg %0, %1" : "=a" (old_value) : "m" (pointer_to_lock_flag), "r" (new_value));
    return old_value;
}

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
