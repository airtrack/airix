#include <mm/pmm.h>
#include <kernel/klib.h>
#include <lib/stdlib.h>

static int mmap_entry_compare(const void *left, const void *right)
{
    const struct mmap_entry *l = (const struct mmap_entry *)left;
    const struct mmap_entry *r = (const struct mmap_entry *)right;
    if (l->base < r->base) return -1;
    else if (l->base == r->base) return 0;
    else return 1;
}

void init_pmm(struct mmap_entry *entries, uint32_t num)
{
    qsort(entries, num, sizeof(*entries), mmap_entry_compare);

    for (uint32_t i = 0; i < num; ++i)
    {
        printk("%d: 0x%x - 0x%x %d %d 0x%x\n", i, entries[i].base,
                (char *)entries[i].base + entries[i].length,
                entries[i].length, entries[i].type, entries[i].acpi_attr);
    }
}
