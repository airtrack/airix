#ifndef PIC_H
#define PIC_H

#include <kernel/base.h>

/* Initialization Control Words */
enum pic_icw1
{
    PIC_ICW1_EXPECT_ICW4 = 0x1,
    PIC_ICW1_SINGLE = 0x2,
    PIC_ICW1_INTERVAL4 = 0x4,
    PIC_ICW1_LEVEL_MODE = 0x8,
    PIC_ICW1_INIT = 0x10,
};

enum pic_icw2
{
    PIC_IDT_BASE_NUM = 0x20,                /* Base interrupt numbers */
    PIC_ICW2_IRQ0 = PIC_IDT_BASE_NUM,
    PIC_ICW2_IRQ8 = PIC_IDT_BASE_NUM + 8,
};

enum pic_icw3
{
    /* x86 architecture uses IRQ line 2 */
    PIC_ICW3_MASTER_IRQ_LINE2 = 0x4,        /* 00000100b line 2 */
    PIC_ICW3_SLAVE_IRQ_LINE2 = 0x2,         /* 00000010b line 2 */
};

enum pic_icw4
{
    PIC_ICW4_X86_MODE = 0x1,
    PIC_ICW4_AUTO_EOI = 0x2,
    PIC_ICW4_MASTER_BUF = 0x4,
    PIC_ICW4_BUFFER_MODE = 0x8,
    PIC_ICW4_SFNM = 0x10,
};

enum pic_ocw2
{
    PIC_OCW2_EOI = 0x20,    /* End of Interrupt(EOI) OCW2 */
};

/* PIC ports */
enum pic_port
{
    PIC_MASTER_CMD_STATUS = 0x20,
    PIC_MASTER_IMR_DATA = 0x21,
    PIC_SLAVE_CMD_STATUS = 0xA0,
    PIC_SLAVE_IMR_DATA = 0xA1,
};

/* IRQs */
enum irq
{
    IRQ0 = 0,
    IRQ1,
    IRQ2,
    IRQ3,
    IRQ4,
    IRQ5,
    IRQ6,
    IRQ7,
    IRQ8,
    IRQ9,
    IRQ10,
    IRQ11,
    IRQ12,
    IRQ13,
    IRQ14,
    IRQ15,
    IRQ_NUM,
};

/* PIC init function */
void pic_initialize();

/* Enable/Disable IRQ line */
void pic_enable_irq(uint8_t irq_line);
void pic_disable_irq(uint8_t irq_line);

/* Register ISR(Interrupt Service Routine) */
typedef void (*pic_isr_t)();
void pic_register_isr(uint8_t irq_line, pic_isr_t isr);

#endif /* PIC_H */
