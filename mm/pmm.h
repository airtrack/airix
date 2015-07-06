#ifndef PMM_H
#define PMM_H

#include <kernel/base.h>

/* This file define Physical Memory Manager */

/* Memory map entry struct */
struct mmap_entry
{
    physical_addr_t base;       /* Base address */
    physical_addr_t base_high;  /* For 64 bits address */
    uint32_t length;            /* Region length */
    uint32_t length_high;       /* For 64 bits length */
    uint32_t type;              /* Region type */
    uint32_t acpi_attr;         /* ACPI 3.0 extended attributes bitfield */
};

/* Values of struct mmap_entry.type */
enum pmm_mm_entry_type
{
    PMM_MM_ENTRY_TYPE_NORMAL = 1,
    PMM_MM_ENTRY_TYPE_RESERVED = 2,
    PMM_MM_ENTRY_TYPE_ACPI_REC = 3,
    PMM_MM_ENTRY_TYPE_ACPI_NVS = 4,
    PMM_MM_ENTRY_TYPE_BAD = 5,
};

#define BUDDY_MAX_ORDER 11
#define MEM_ALIGNMENT 4

#define PAGE_SIZE 4096
#define PAGE_NUMBER(addr) ((uint32_t)(addr) / PAGE_SIZE)
#define PAGE_ADDRESS(page_num) (physical_addr_t)((page_num) * PAGE_SIZE)

/* Alignment macros */
#define ALIGN_HELPER(addr, align) \
    ((((uint32_t)(addr) - 1) / (align) + 1) * (align))

#define ALIGN_SIZE(addr) ALIGN_HELPER(addr, MEM_ALIGNMENT)
#define ALIGN_ADDRESS(addr) (void *)ALIGN_SIZE(addr)
#define ALIGN_PAGE(addr) ALIGN_HELPER(addr, PAGE_SIZE)
#define ALIGN_PAGE_ADDRESS(addr) (void *)ALIGN_PAGE(addr)

/* PMM init function */
void pmm_initialize(physical_addr_t free_addr, struct mmap_entry *entries,
                    uint32_t num);

/* Get max address of physical memory */
uint64_t pmm_max_physical_address(struct mmap_entry *entries, uint32_t num);

/* Alloc 2 ^ order pages, returns start page number or 0 if failed */
uint32_t pmm_alloc_pages(uint32_t order);

/* Print memory statistics information */
void pmm_print_statistics(struct mmap_entry *entries, uint32_t num);

/* Alloc one page, returns page number or 0 if failed */
static inline uint32_t pmm_alloc_page()
{
    return pmm_alloc_pages(0);
}

/* Alloc 2 ^ order pages, returns start page physical address or 0 if failed */
static inline physical_addr_t pmm_alloc_pages_address(uint32_t order)
{
    return PAGE_ADDRESS(pmm_alloc_pages(order));
}

/* Alloc one page, returns page physical address or 0 if failed */
static inline physical_addr_t pmm_alloc_page_address()
{
    return pmm_alloc_pages_address(0);
}

/* Free page block, start from page_num, the block has 2 ^ order pages */
void pmm_free_pages(uint32_t page_num, uint32_t order);

/* Free one page */
static inline void pmm_free_page(uint32_t page_num)
{
    pmm_free_pages(page_num, 0);
}

/* Free page block by physical address */
static inline void pmm_free_pages_address(physical_addr_t start, uint32_t order)
{
    pmm_free_pages(PAGE_NUMBER(start), order);
}

/* Free one page by physical address */
static inline void pmm_free_page_address(physical_addr_t start)
{
    pmm_free_pages_address(start, 0);
}

#endif // PMM_H
