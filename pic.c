#include "pic.h"
#include "klib.h"

void init_pic()
{
    /* Init */
    out_byte(PIC_MASTER_CMD_STATUS, PIC_ICW1_INIT | PIC_ICW1_EXPECT_ICW4);
    out_byte(PIC_SLAVE_CMD_STATUS, PIC_ICW1_INIT | PIC_ICW1_EXPECT_ICW4);

    /* Set base interrupt number */
    out_byte(PIC_MASTER_IMR_DATA, PIC_ICW2_IRQ0);
    out_byte(PIC_SLAVE_IMR_DATA, PIC_ICW2_IRQ8);

    /* Set line 2 as connection line */
    out_byte(PIC_MASTER_IMR_DATA, PIC_ICW3_MASTER_IRQ_LINE2);
    out_byte(PIC_SLAVE_IMR_DATA, PIC_ICW3_SLAVE_IRQ_LINE2);

    /* Set x86 mode */
    out_byte(PIC_MASTER_IMR_DATA, PIC_ICW4_X86_MODE);
    out_byte(PIC_SLAVE_IMR_DATA, PIC_ICW4_X86_MODE);

    /* Disable all interrupts */
    out_byte(PIC_MASTER_IMR_DATA, 0xFF);
    out_byte(PIC_SLAVE_IMR_DATA, 0xFF);
}

void pic_enable_irq(uint8_t irq_line)
{
    uint16_t port;
    uint8_t value;

    if (irq_line < 8)
    {
        port = PIC_MASTER_IMR_DATA;
    }
    else
    {
        port = PIC_SLAVE_IMR_DATA;
        irq_line -= 8;
    }

    value = in_byte(port) | (1 << irq_line);
    out_byte(port, value);
}

void pic_disable_irq(uint8_t irq_line)
{
    uint16_t port;
    uint8_t value;

    if (irq_line < 8)
    {
        port = PIC_MASTER_IMR_DATA;
    }
    else
    {
        port = PIC_SLAVE_IMR_DATA;
        irq_line -= 8;
    }

    value = in_byte(port) & ~(1 << irq_line);
    out_byte(port, value);
}
