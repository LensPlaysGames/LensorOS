#include "spinlock.h"

bool SpinlockLocker::compare_and_swap_lock() {
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
                 : "=q" (ret), "=m" (Lock.locked)
                 : "r" (true), "m" (Lock.locked), "a" (false)
                 : "memory");
    return ret;
}

bool SpinlockLocker::test_and_set_lock() {
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
    asm volatile ("lock xchg %0, %1" : "=a" (old_value) : "m" (&Lock.locked), "r" (true));
    return old_value;
}

SpinlockLocker::SpinlockLocker(Spinlock& l)
    : Lock(l)
{
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
