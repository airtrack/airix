#include <kernel/pic.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/klib.h>

static pic_isr_t isr_table[IRQ_NUM];
static void *isr_entry_table[IRQ_NUM] =
{
    isr_entry0,
    isr_entry1,
    isr_entry2,
    isr_entry3,
    isr_entry4,
    isr_entry5,
    isr_entry6,
    isr_entry7,
    isr_entry8,
    isr_entry9,
    isr_entry10,
    isr_entry11,
    isr_entry12,
    isr_entry13,
    isr_entry14,
    isr_entry15
};

static inline void register_isr_entry(uint8_t irq_line)
{
    void *entry = isr_entry_table[irq_line];
    uint8_t idt_num = PIC_IDT_BASE_NUM + irq_line;
    idt_set_entry(idt_num, KERNEL_CODE_SELECTOR, entry, IDT_TYPE_INT, DPL_0);
}

static void default_handler()
{
    panic("PIC default handler: unhandled interrupt");
}

static void register_isr_entries()
{
    for (uint32_t i = 0; i < IRQ_NUM; ++i)
        isr_table[i] = default_handler;

    for (uint32_t i = 0; i < IRQ_NUM; ++i)
        register_isr_entry(i);
}

void pic_initialize()
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

    register_isr_entries();
}

void pic_enable_irq(uint8_t irq_line)
{
    uint16_t port;
    uint8_t value;

    if (irq_line < IRQ8)
    {
        port = PIC_MASTER_IMR_DATA;
    }
    else
    {
        port = PIC_SLAVE_IMR_DATA;
        irq_line -= IRQ8;
    }

    value = in_byte(port) & ~(1 << irq_line);
    out_byte(port, value);
}

void pic_disable_irq(uint8_t irq_line)
{
    uint16_t port;
    uint8_t value;

    if (irq_line < IRQ8)
    {
        port = PIC_MASTER_IMR_DATA;
    }
    else
    {
        port = PIC_SLAVE_IMR_DATA;
        irq_line -= IRQ8;
    }

    value = in_byte(port) | (1 << irq_line);
    out_byte(port, value);
}

/* Notify PIC interrupt is done */
static inline void send_eoi(uint8_t irq_line)
{
    if (irq_line >= IRQ8)
        out_byte(PIC_SLAVE_CMD_STATUS, PIC_OCW2_EOI);

    out_byte(PIC_MASTER_CMD_STATUS, PIC_OCW2_EOI);
}

/* All PIC entries call this function */
void pic_interrupt(uint8_t irq_line)
{
    isr_table[irq_line]();
    send_eoi(irq_line);
}

void pic_register_isr(uint8_t irq_line, pic_isr_t isr)
{
    isr_table[irq_line] = isr;
    pic_enable_irq(irq_line);
}
