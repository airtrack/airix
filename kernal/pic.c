#include <kernal/pic.h>
#include <kernal/gdt.h>
#include <kernal/idt.h>
#include <kernal/klib.h>

static pic_isr_t isr_table[IRQ_NUM];
static void *isr_entry_table[IRQ_NUM] =
{
    isr_entry0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    isr_entry7,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static inline void pic_register_isr_entry(uint8_t irq_line)
{
    void *entry = isr_entry_table[irq_line];
    uint8_t idt_num = PIC_IDT_BASE_NUM + irq_line;
    idt_set_entry(idt_num, GDT_FLAT_MEM_TEXT_SEL, entry, IDT_TYPE_INT, DPL_0);
}

static void pic_register_isr_entries()
{
    pic_register_isr_entry(IRQ0);
    pic_register_isr_entry(IRQ7);
}

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

    pic_register_isr_entries();
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

    value = in_byte(port) & ~(1 << irq_line);
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

    value = in_byte(port) | (1 << irq_line);
    out_byte(port, value);
}

static inline void pic_send_eoi()
{
    out_byte(PIC_MASTER_CMD_STATUS, PIC_OCW2_EOI);
}

void pic_interrupt(uint8_t irq_line)
{
    isr_table[irq_line]();
    pic_send_eoi();
}

void pic_register_isr(uint8_t irq_line, pic_isr_t isr)
{
    isr_table[irq_line] = isr;
    pic_enable_irq(irq_line);
}
