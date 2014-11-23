#include <mm/pmm.h>
#include <kernel/klib.h>
#include <lib/stdlib.h>

static int memory_map_entry_compare(const void *left, const void *right)
{
    const memory_map_entry_t *l = (const memory_map_entry_t *)left;
    const memory_map_entry_t *r = (const memory_map_entry_t *)right;
    if (l->base < r->base) return -1;
    else if (l->base == r->base) return 0;
    else return 1;
}

void init_pmm(memory_map_entry_t *entries, uint32_t num)
{
    qsort(entries, num, sizeof(*entries), memory_map_entry_compare);

    for (uint32_t i = 0; i < num; ++i)
    {
        printk("%d: 0x%x - 0x%x %d %d 0x%x\n", i, entries[i].base,
                (char *)entries[i].base + entries[i].length,
                entries[i].length, entries[i].type, entries[i].acpi_attr);
    }
}
