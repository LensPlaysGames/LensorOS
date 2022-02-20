#ifndef LENSOR_OS_HIGH_PRECISION_EVENT_TIMER_H
#define LENSOR_OS_HIGH_PRECISION_EVENT_TIMER_H

#define HPET_REG_GENERAL_CAPABILITIES_AND_ID  0x000
#define HPET_REG_GENERAL_CONFIGURATION        0x010
#define HPET_REG_GENERAL_INTERRUPT_STATUS     0x020
#define HPET_REG_MAIN_COUNTER_VALUE           0x0f0
#define HPET_REG_TIMER_N_CONFIG_AND_CAPABILITIES (n) ((0x100 + 0x20 * n) - (0x107 + 0x20 * n))
#define HPET_REG_TIMER_N_COMPARATOR_VALUE        (n) ((0x108 + 0x20 * n) - (0x10f + 0x20 * n))
#define HPET_REG_TIMER_N_FSB_INTERRUPT_ROUTE     (n) ((0x110 + 0x20 * n) - (0x117 + 0x20 * n))

/* HPET Registers 
 * General Capabilities and ID:
 *   Bits 0-7: Revision ID (must not be zero).
 *        8-12: Number of Timers.
 *        13: If set, main counter capable of 64-bit.
 *        14: Reserved.
 *        15: If set, HPET is capable of using legacy replacement interrupt mapping.
 *        16-31: PCI Vendor ID.
 *        32-63: Main counter tick period in femtoseconds (must not be zero).
 *        
 * General Configuration Register:
 *   Bit 0: If set, main counter enabled.
 *       1: If set, legacy replacement mapping is enabled.
 *
 * General Interrupt Status Register
 *   Bit n: If level-triggered, set when corresponding timer interrupt is active.
 *            If set, software can clear it by writing a 1 to this bit. Writes of 0 have no effect.
 *          If edge-triggered, ignored (must be zero).
 *       32-63: Reserved
 * 
 * Main Counter Value
 *   Do not write to this register unless main counter is disabled.
 *   Bits 0-63: Value of main counter.
 *
 * Timer N Configuration and Capabilities
 *   Bit 0: Reserved.
 *       1: Interrupt Type (0=edge-triggered, 1=level-triggered).
 *       2: Interrupt Enable.
 *       3: If periodic interrupts supported, writing a 1 enables periodic timer, else ignore.
 *       4: If set, timer has periodic interrupt capability.
 *       5: If set, timer size is 64-bit, otherwise 32-bit.
 *       6: Value Set Configuration.
 *       7: Reserved.
 *       8: If set and 64-bit, force 32-bit mode.
 *       9-13: I/O APIC Routing.
 *       14: If set, enable FSB interrupt mapping.
 *       15: If set, timer is capable of FSB interrupt mapping.
 *       16-31: Reserved.
 *       32-63: If bit X is set in this field, this timer can be mapped to IRQX line of I/O APIC.
 *
 * Timer N Comparator Value Register
 *   Bits 0-63: When main counter value is equal to this value, an interrupt will be generated.
 */

#endif
