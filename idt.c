#include "idt.h"
#include "klib.h"

/* IDT */
static idt_entry_t idt[256];

/* IDTR */
static idtr_t idtr = { sizeof(idt) - 1, idt };

void init_idt()
{
    set_idtr(&idtr);
}
