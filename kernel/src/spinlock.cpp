#include "spinlock.h"

bool Spinlock::test_and_set(bool* pointer_to_lock_flag, bool new_value) {
    /* In x86, when an instruction is prefixed with `lock`, it is guaranteed to be run atomically (thread-safe).
     * The `xchg` instruction sets a memory address or register to a new value while returning the old value in RAX.
     * It can be determined that the lock *was* free (unlocked) and *is* acquired (locked)
     *   by currently executing code by testing that the old value that is returned is equal to false.
     */
    
     /* Inline Assembly:
     * |- Input(s)
     * |  |- "m" -> Memory Address of lock flag
     * |  `- "r" -> Register of new lock flag value
     * `- Output(s)
     *    `- "=a" -> Write-only; output in RAX
     */
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
