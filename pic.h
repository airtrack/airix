#ifndef PIC_H
#define PIC_H

#include "base_types.h"

/* Initialization Control Words */
#define PIC_ICW1_EXPECT_ICW4 0x1
#define PIC_ICW1_SINGLE 0x2
#define PIC_ICW1_INTERVAL4 0x4
#define PIC_ICW1_LEVEL_MODE 0x8
#define PIC_ICW1_INIT 0x10

/* Base interrupt numbers */
#define PIC_IDT_BASE_NUM 0x20
#define PIC_ICW2_IRQ0 PIC_IDT_BASE_NUM
#define PIC_ICW2_IRQ8 (PIC_IDT_BASE_NUM + 8)

/* x86 architecture uses IRQ line 2 */
#define PIC_ICW3_MASTER_IRQ_LINE2 0x4 /* 00000100b line 2 */
#define PIC_ICW3_SLAVE_IRQ_LINE2 0x2 /* 00000010b line 2 */

#define PIC_ICW4_X86_MODE 0x1
#define PIC_ICW4_AUTO_EOI 0x2
#define PIC_ICW4_MASTER_BUF 0x4
#define PIC_ICW4_BUFFER_MODE 0x8
#define PIC_ICW4_SFNM 0x10

/* End of Interrupt(EOI) OCW2 */
#define PIC_OCW2_EOI 0x20

/* PIC ports */
#define PIC_MASTER_CMD_STATUS 0x20
#define PIC_MASTER_IMR_DATA 0x21
#define PIC_SLAVE_CMD_STATUS 0xA0
#define PIC_SLAVE_IMR_DATA 0xA1

/* IRQs */
#define IRQ0 0
#define IRQ1 1
#define IRQ2 2
#define IRQ3 3
#define IRQ4 4
#define IRQ5 5
#define IRQ6 6
#define IRQ7 7
#define IRQ8 8
#define IRQ9 9
#define IRQ10 10
#define IRQ11 11
#define IRQ12 12
#define IRQ13 13
#define IRQ14 14
#define IRQ15 15
#define IRQ_NUM 16

/* PIC init function */
void init_pic();

/* Enable/Disable IRQ line */
void pic_enable_irq(uint8_t irq_line);
void pic_disable_irq(uint8_t irq_line);

/* Register ISR(Interrupt Service Routine) */
typedef void (*pic_isr_t)();
void pic_register_isr(uint8_t irq_line, pic_isr_t isr);

#endif // PIC_H
