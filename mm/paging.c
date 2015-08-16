#include <mm/paging.h>
#include <mm/vmm.h>
#include <kernel/klib.h>

static struct page_directory *pg_dir;

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

    /*
     * Map virtual address to physical address:
     * [KERNEL_BASE, KERNEL_BASE + 4MB) to [0, 4MB),
     * [0, 4MB) to [0, 4MB).
     * [0, 4MB) to [0, 4MB) is required when set_cr3 returning
     */
    for (uint32_t i = 0; i < NUM_PDE; ++i)
        page_dir->entries[i] = VMM_WRITABLE;
    for (uint32_t i = 0; i < NUM_PTE; ++i)
        page_tab->entries[i] = (i * 0x1000) | VMM_WRITABLE | VMM_PRESENT;

    page_dir->entries[0] =
        (pde_t)page_tab | VMM_WRITABLE | VMM_PRESENT;
    page_dir->entries[vmm_pde_index((void *)KERNEL_BASE)] =
        (pde_t)page_tab | VMM_WRITABLE | VMM_PRESENT;

    /*
     * Enable paging
     * This function is running in physical address, and all addresses of
     * global symbols are virtual address, so we should convert virtual
     * address to physical address before we call it.
     */
    VIRTUAL_TO_PHYSICAL(set_cr3)((physical_addr_t)page_dir);

    pg_dir = CAST_PHYSICAL_TO_VIRTUAL((physical_addr_t)page_dir);
    return page_aligned_free;
}

physical_addr_t pg_complete_paging(physical_addr_t page_aligned_free,
                                   struct mmap_entry *entries, uint32_t num)
{
    uint64_t physical_addr = NUM_PTE * PAGE_SIZE;
    uint64_t max_physical_addr = pmm_max_physical_address(entries, num);

    /* Clear virtual address's [0, 4MB) to physical address's [0, 4MB) map */
    pg_dir->entries[0] = VMM_WRITABLE;

    /*
     * Install paging tables for [4MB, max_physical_addr).
     * Map [4MB, max_physical_addr) to [KERNEL_BASE + 4MB, ...)
     */
    while (physical_addr < max_physical_addr)
    {
        uint32_t virtual_addr = (uint32_t)physical_addr + KERNEL_BASE;
        struct page_table *pg_tab = CAST_PHYSICAL_TO_VIRTUAL(page_aligned_free);

        /* Fill all entries of page table */
        for (uint32_t i = 0; i < NUM_PTE; ++i, physical_addr += PAGE_SIZE)
            pg_tab->entries[i] = (pte_t)physical_addr
                | VMM_WRITABLE | VMM_PRESENT;

        /* Fill page directory entry */
        pg_dir->entries[vmm_pde_index((void *)virtual_addr)] =
            (pde_t)CAST_VIRTUAL_TO_PHYSICAL(pg_tab)
            | VMM_WRITABLE | VMM_PRESENT;

        page_aligned_free += PAGE_SIZE;
    }

    /* Refresh paging directory */
    set_cr3(CAST_VIRTUAL_TO_PHYSICAL(pg_dir));

    return page_aligned_free;
}
