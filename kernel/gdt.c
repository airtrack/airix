#include <kernel/gdt.h>
#include <kernel/klib.h>

/* GDT */
static gdt_entry_t gdt[GDT_ENTRY_NUM];

/* GDTR */
static gdtr_t gdtr = { sizeof(gdt) - 1, gdt };

void init_gdt()
{
    int num_entries;
    gdtr_t old_gdtr;

    /* Change GDT and GDTR */
    get_gdtr(&old_gdtr);
    num_entries = (old_gdtr.limit + 1) / sizeof(gdt_entry_t);
    for (int i = 0; i < num_entries; ++i)
        gdtr.base[i] = old_gdtr.base[i];
    set_gdtr(&gdtr);
}
