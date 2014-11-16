#include "idt.h"
#include "klib.h"

/* IDT */
static idt_entry_t idt[IDT_ENTRY_NUM];

/* IDTR */
static idtr_t idtr = { sizeof(idt) - 1, idt };

void init_idt()
{
    set_idtr(&idtr);
}

void idt_set_entry(uint8_t entry_num, uint16_t selector,
        void *offset, uint8_t type, uint8_t dpl)
{
    idt_entry_t *entry = &idt[entry_num];
    entry->selector = selector;
    entry->offset_0 = (uint32_t)offset & 0xFFFF;
    entry->offset_1 = ((uint32_t)offset >> 16) & 0xFFFF;
    entry->type_attr = (type & 0xF) | ((dpl & 0x3) << 5) | IDT_ATTR_PRESENT;
}
