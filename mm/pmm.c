#include <mm/pmm.h>
#include <kernel/klib.h>
#include <lib/stdlib.h>

#define MEM_ALIGNMENT 4
#define ALIGN_ADDRESS(addr) \
    (void *)((((uint32_t)addr - 1) / MEM_ALIGNMENT + 1) * MEM_ALIGNMENT)
#define ALIGN_SIZE(size) \
    (((size - 1) / MEM_ALIGNMENT + 1) * MEM_ALIGNMENT)

/* Allocator provide memory allocation before MM initialized.
 * Only support memory alloc, do not support memory free. */
struct boot_allocator
{
    char *start_address;
    char *free_address;
};

static struct boot_allocator boot_allocator;

static void init_boot_allocator(void *start_address)
{
    start_address = ALIGN_ADDRESS(start_address);
    boot_allocator.start_address = start_address;
    boot_allocator.free_address = start_address;

    printk("Boot allocator start address: 0x%x\n", start_address);
}

static inline void * boot_alloc(uint32_t size)
{
    void *address = boot_allocator.free_address;
    boot_allocator.free_address += ALIGN_SIZE(size);
    return address;
}

static int mmap_entry_compare(const void *left, const void *right)
{
    const struct mmap_entry *l = (const struct mmap_entry *)left;
    const struct mmap_entry *r = (const struct mmap_entry *)right;
    if (l->base < r->base) return -1;
    else if (l->base == r->base) return 0;
    else return 1;
}

void init_pmm(void *free_mem, struct mmap_entry *entries, uint32_t num)
{
    init_boot_allocator(free_mem);

    qsort(entries, num, sizeof(*entries), mmap_entry_compare);
    for (uint32_t i = 0; i < num; ++i)
    {
        printk("%d: 0x%x - 0x%x %d %d 0x%x\n", i, entries[i].base,
                (char *)entries[i].base + entries[i].length,
                entries[i].length, entries[i].type, entries[i].acpi_attr);
    }
}
