#include "gdt.h"
#include "klib.h"

/* C entry */
void cstart()
{
    int num_entries;
    gdtr_t old_gdtr;

    /* Change GDT and GDTR */
    get_gdtr(&old_gdtr);
    num_entries = (old_gdtr.limit + 1) / sizeof(gdt_entry);
    for (int i = 0; i < num_entries; ++i)
        gdtr.base[i] = old_gdtr.base[i];
    set_gdtr(&gdtr);

    for (unsigned char c = 0;; ++c)
        display_char(20, 10, c);
}
