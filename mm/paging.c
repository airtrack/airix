#include <mm/paging.h>
#include <mm/pmm.h>
#include <kernel/base_types.h>
#include <kernel/klib.h>

/* Number of entries in page directory/table */
#define NUM_PDE 1024
#define NUM_PTE 1024

/* Page directory/table entry flags */
#define PAGE_PRESENT 0x1
#define PAGE_READ_WRITE 0x2

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

static struct page_directory *page_root;

void init_paging()
{
    struct page_table *first_table;

    /* Alloc the page directory */
    page_root = alloc_page_address();

    for (uint32_t i = 0; i < NUM_PDE; ++i)
        page_root->entries[i] = PAGE_READ_WRITE;

    /* Alloc first page table */
    first_table = alloc_page_address();

    for (uint32_t i = 0; i < NUM_PTE; ++i)
        first_table->entries[i] = (i * 0x1000) | PAGE_READ_WRITE | PAGE_PRESENT;

    page_root->entries[0] = (pde_t)first_table | PAGE_READ_WRITE | PAGE_PRESENT;

    /* Enable paging */
    enable_paging(page_root);
}
