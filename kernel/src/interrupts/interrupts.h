#ifndef LENSOR_OS_INTERRUPTS_H
#define LENSOR_OS_INTERRUPTS_H

#include <integers.h>

/* x86: Interrupt Request Vector Offsets 
 *   Programmable Interrupt Chip (some say Peripheral interrupt chip, who knows)
 *   Vector Capacity = 256
 *   IRQ vector offsets are reserved to be triggered by hardware only.
 *   
 *   IRQ Descriptions:
 *     0  -- System Timer (PIT, most likely)
 *     1  -- PS/2 Keyboard
 *     2  -- Cascaded signal from IRQs 8-15
 *     3  -- Serial Port Controller for COM2 & COM4
 *     4  -- Serial Port Controller for COM1 & COM3
 *     5  -- Parallel Port 3 or Sound Card
 *     6  -- Floppy Disk Controller
 *     7  -- Parallel Port 1 and Parallel Port 2
 *     8  -- Real Time Clock
 *     9  -- Intel: Advanced Configuration and Power Interface
 *           Other: IRQ2 redirects here
 *     11 -- Open for Peripherals (SCSI or NIC)
 *     12 -- PS/2 Mouse
 *     13 -- Co-processor or integrated FPU or inter-processor interrupt
 *     14 -- Primary ATA Channel
 *     15 -- Secondary ATA Channel
 */
#define PIC_IRQ_VECTOR_OFFSET 0x20
#define PIC_IRQ0  PIC_IRQ_VECTOR_OFFSET + 0
#define PIC_IRQ1  PIC_IRQ_VECTOR_OFFSET + 1
#define PIC_IRQ2  PIC_IRQ_VECTOR_OFFSET + 2
#define PIC_IRQ3  PIC_IRQ_VECTOR_OFFSET + 3
#define PIC_IRQ4  PIC_IRQ_VECTOR_OFFSET + 4
#define PIC_IRQ5  PIC_IRQ_VECTOR_OFFSET + 5
#define PIC_IRQ6  PIC_IRQ_VECTOR_OFFSET + 6
#define PIC_IRQ7  PIC_IRQ_VECTOR_OFFSET + 7
#define PIC_IRQ8  PIC_IRQ_VECTOR_OFFSET + 8
#define PIC_IRQ9  PIC_IRQ_VECTOR_OFFSET + 9
#define PIC_IRQ10 PIC_IRQ_VECTOR_OFFSET + 10
#define PIC_IRQ11 PIC_IRQ_VECTOR_OFFSET + 11
#define PIC_IRQ12 PIC_IRQ_VECTOR_OFFSET + 12
#define PIC_IRQ13 PIC_IRQ_VECTOR_OFFSET + 13
#define PIC_IRQ14 PIC_IRQ_VECTOR_OFFSET + 14
#define PIC_IRQ15 PIC_IRQ_VECTOR_OFFSET + 15

#define IRQ_SYSTEM_TIMER   0
#define IRQ_PS2_KEYBOARD   1
#define IRQ_CASCADED_PIC   2
#define IRQ_UART_COM2      3
#define IRQ_UART_COM1      4
#define IRQ_LPT2           5
#define IRQ_FLOPPY_DISK    6
#define IRQ_LPT1           7
#define IRQ_REAL_TIMER     8
#define IRQ_PERIPHERAL_0   9
#define IRQ_PERIPHERAL_1   10
#define IRQ_PERIPHERAL_2   11
#define IRQ_PS2_MOUSE      12
#define IRQ_INTERPROCESSOR 13
#define IRQ_PRIMARY_ATA    14
#define IRQ_SECONDARY_ATA  15

#define IRQ_BIT(IRQx) (1 << IRQx)

// Define PIC chip ports
#define PIC_EOI       0x20
#define PIC1_COMMAND  0x20
#define PIC1_DATA     0x21
#define PIC2_COMMAND  0xa0
#define PIC2_DATA     0xa1

#define ICW1_INIT     0x10
#define ICW1_ICW4     0x01
#define ICW4_8086     0x01

struct InterruptFrame {
    // Instruction Pointer
    u64 ip;
    // Code Segment
    u64 cs;
    u64 flags;
    // Stack Pointer
    u64 sp;
    // Segment Selector
    u64 ss;
} __attribute__((packed));

struct InterruptFrameError : public InterruptFrame {
    u64 error;
} __attribute__((packed));

// HARDWARE INTERRUPT REQUESTS (IRQs)
__attribute__((interrupt)) void system_timer_handler (InterruptFrame*);
__attribute__((interrupt)) void keyboard_handler     (InterruptFrame*);
__attribute__((interrupt)) void uart_com1_handler    (InterruptFrame*);
__attribute__((interrupt)) void rtc_handler          (InterruptFrame*);
__attribute__((interrupt)) void mouse_handler        (InterruptFrame*);
// EXCEPTION HANDLING
__attribute__((interrupt)) void divide_by_zero_handler           (InterruptFrame*);
__attribute__((interrupt)) void double_fault_handler             (InterruptFrameError*);
__attribute__((interrupt)) void stack_segment_fault_handler      (InterruptFrameError*);
__attribute__((interrupt)) void general_protection_fault_handler (InterruptFrameError*);
__attribute__((interrupt)) void page_fault_handler               (InterruptFrameError*);
__attribute__((interrupt)) void simd_exception_handler           (InterruptFrame*);

// HELPER FUNCTIONS TO TRIGGER HANDLERS FOR TESTING
void cause_div_by_zero(u8 one = 1);
void cause_page_not_present();
void cause_general_protection();

void remap_pic();

// Enable IRQx within the PIC masks.
void enable_interrupt(u8 irq);
// Disable IRQx within the PIC masks.
void disable_interrupt(u8 irq);
// Disable all IRQs within the PIC masks.
void disable_all_interrupts();

#endif
