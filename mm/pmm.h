#ifndef PMM_H
#define PMM_H

#include <kernel/base_types.h>

/* This file define Physical Memory Manager */

/* Memory map entry struct */
struct mmap_entry
{
    void *base;             /* Base address */
    void *base_high;        /* For 64 bits address */
    uint32_t length;        /* Region length */
    uint32_t length_high;   /* For 64 bits length */
    uint32_t type;          /* Region type */
    uint32_t acpi_attr;     /* ACPI 3.0 extended attributes bitfield */
};

/* Values of struct mmap_entry.type */
#define PMM_MM_ENTRY_TYPE_NORMAL 1
#define PMM_MM_ENTRY_TYPE_RESERVED 2
#define PMM_MM_ENTRY_TYPE_ACPI_REC 3
#define PMM_MM_ENTRY_TYPE_ACPI_NVS 4
#define PMM_MM_ENTRY_TYPE_BAD 5

#define BUDDY_MAX_ORDER 11

#define PAGE_SIZE 4096
#define PAGE_NUMBER(addr) ((uint32_t)(addr) / PAGE_SIZE)
#define PAGE_ADDRESS(page_num) (void *)((page_num) * PAGE_SIZE)

/* PMM init function */
void init_pmm(void *free_addr, struct mmap_entry *entries, uint32_t num);

/* Alloc 2 ^ order pages, returns start page number or 0 if failed */
uint32_t alloc_pages(uint32_t order);

/* Alloc one page, returns page number or 0 if failed */
inline uint32_t alloc_page() { return alloc_pages(0); }

/* Alloc 2 ^ order pages, returns start page address or NULL if failed */
inline void * alloc_pages_address(uint32_t order)
{
    return PAGE_ADDRESS(alloc_pages(order));
}

/* Alloc one page, returns page address or NULL if failed */
inline void * alloc_page_address() { return alloc_pages_address(0); }

/* Free page block, start from page_num, the block has 2 ^ order pages */
void free_pages(uint32_t page_num, uint32_t order);

/* Free one page */
inline void free_page(uint32_t page_num) { free_pages(page_num, 0); }

/* Free page block by address */
inline void free_pages_address(void *start, uint32_t order)
{
    free_pages(PAGE_NUMBER(start), order);
}

/* Free one page by address */
inline void free_page_address(void *start) { free_pages_address(start, 0); }

#endif // PMM_H
