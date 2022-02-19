#include "spinlock.h"

bool SpinlockLocker::compare_and_swap
(
 bool* pointer_to_lock_flag
 , bool expected_value_before_swap
 , bool new_value
 )
{
    // It can be determined the lock was free and has been
    //   acquired by passing an expected value before swap of `false`.
    /* Inline Assembly:
     * |- Desc:
     * |  |- `lock`                  -- Ensures that the following instruction runs atomically.
     * |  |- `cmpxchg <op0>, <op1>`  -- Compare 'A' register with <op0>; if equal, swap <op1> and <op0>.
     * |  |- `sete <op0>`            -- Assign <op0> to the 'equal' flag of the CPU (ZF==1).
     * |- Outputs:
     * |  |- "=q"  -- Write-only low-byte-accessible register; return value
     * |  `- "=m"  -- Write-only memory address; lock flag
     * `- Inputs:
     *    |- "r"  -- Any register; new value to swap to if expected  -- "%0"
     *    |- "m"  -- Memory address; lock flag  -- "%1"
     *    `- "a"  -- The 'A' register; lock flag will be set only if it matches this value  -- "%2"
     */
    bool ret;
    asm volatile("lock cmpxchg %2, %1\n"
                 "sete %0\n"
                 : "=q" (ret), "=m" (*pointer_to_lock_flag)
                 : "r" (new_value), "m" (*pointer_to_lock_flag), "a" (expected_value_before_swap)
                 : "memory");
    return ret;
}

bool SpinlockLocker::test_and_set(bool* pointer_to_lock_flag, bool new_value) {
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
    while (compare_and_swap(&lock.locked, false, true) == true);
}

void SpinlockLocker::unlock() {
    Lock.locked = false;
}
