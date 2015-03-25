#include <mm/paging.h>
#include <kernel/klib.h>

/* Number of entries in page directory/table */
#define NUM_PDE 1024
#define NUM_PTE 1024

/* Page directory/table entry flags */
enum pde_flag
{
    PDE_PRESENT = 0x1,
    PDE_WRITABLE = 0x2,
};

enum pte_flag
{
    PTE_PRESENT = 0x1,
    PTE_WRITABLE = 0x2,
};

/* Page directory entry type */
typedef uint32_t pde_t;
/* Page table entry type */
typedef uint32_t pte_t;

/* Page directory, size is 4KB */
struct page_directory
{
    pde_t entries[NUM_PDE];
};

/* Page table, size is 4KB */
struct page_table
{
    pte_t entries[NUM_PTE];
};

static struct page_directory *pg_dir;

static inline uint32_t get_pde_index(uint32_t addr)
{
    return (addr >> 22) & 0x3FF;
}

physical_addr_t pg_init_paging(physical_addr_t page_aligned_free)
{
    struct page_directory *page_dir;
    struct page_table *page_tab;

    /* Alloc the page directory */
    page_dir = (void *)page_aligned_free;
    page_aligned_free += PAGE_SIZE;

    /* Alloc page table */
    page_tab = (void *)page_aligned_free;
    page_aligned_free += PAGE_SIZE;

    /* Map virtual address to physical address:
     * [KERNEL_BASE, KERNEL_BASE + 4MB) to [0, 4MB),
     * [0, 4MB) to [0, 4MB).
     * [0, 4MB) to [0, 4MB) is required when enable_paging returning */
    for (uint32_t i = 0; i < NUM_PDE; ++i)
        page_dir->entries[i] = PDE_WRITABLE;
    for (uint32_t i = 0; i < NUM_PTE; ++i)
        page_tab->entries[i] = (i * 0x1000) | PTE_WRITABLE | PTE_PRESENT;

    page_dir->entries[0] =
        (pde_t)page_tab | PDE_WRITABLE | PDE_PRESENT;
    page_dir->entries[get_pde_index(KERNEL_BASE)] =
        (pde_t)page_tab | PDE_WRITABLE | PDE_PRESENT;

    /* Enable paging */
    VIRTUAL_TO_PHYSICAL(enable_paging)(page_dir);

    pg_dir = CAST_PHYSICAL_TO_VIRTUAL((physical_addr_t)page_dir);
    return page_aligned_free;
}

physical_addr_t pg_complete_paging(physical_addr_t page_aligned_free,
                                   struct mmap_entry *entries, uint32_t num)
{
    uint64_t physical_addr = NUM_PTE * PAGE_SIZE;
    uint64_t max_physical_addr = pmm_max_physical_address(entries, num);

    /* Clear virtual address's [0, 4MB) to physical address's [0, 4MB) map */
    pg_dir->entries[0] = PDE_WRITABLE;

    /* Install paging tables for [4MB, max_physical_addr).
     * Map [4MB, max_physical_addr) to [KERNEL_BASE + 4MB, ...) */
    while (physical_addr < max_physical_addr)
    {
        uint32_t virtual_addr = (uint32_t)physical_addr + KERNEL_BASE;
        struct page_table *pg_tab = CAST_PHYSICAL_TO_VIRTUAL(page_aligned_free);

        /* Fill all entries of page table */
        for (uint32_t i = 0; i < NUM_PTE; ++i, physical_addr += PAGE_SIZE)
            pg_tab->entries[i] = (pte_t)physical_addr
                | PTE_WRITABLE | PTE_PRESENT;

        /* Fill page directory entry */
        pg_dir->entries[get_pde_index(virtual_addr)] =
            (pde_t)CAST_VIRTUAL_TO_PHYSICAL(pg_tab)
            | PDE_WRITABLE | PDE_PRESENT;

        page_aligned_free += PAGE_SIZE;
    }

    return page_aligned_free;
}
