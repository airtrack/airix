#include <kernel/gdt.h>
#include <kernel/klib.h>

/* GDT */
static gdt_entry_t gdt[GDT_ENTRY_NUM];

/* GDTR */
static struct gdtr gdtr = { sizeof(gdt) - 1, gdt };

void gdt_initialize()
{
    uint32_t num_entries;
    struct gdtr old_gdtr;

    /* Change GDT and GDTR */
    get_gdtr(&old_gdtr);

    num_entries = (old_gdtr.limit + 1) / sizeof(gdt_entry_t);
    old_gdtr.base = CAST_PHYSICAL_TO_VIRTUAL((physical_addr_t)old_gdtr.base);

    for (uint32_t i = 0; i < num_entries; ++i)
        gdtr.base[i] = old_gdtr.base[i];

    set_gdtr(&gdtr);
}
