#ifndef PAGING_H
#define PAGING_H

#include <mm/pmm.h>
#include <kernel/base.h>

/*
 * Init paging, the first step.
 * Returns the next free physical address.
 */
physical_addr_t pg_init_paging(physical_addr_t page_aligned_free);

/*
 * Complete paging, the second step.
 * Returns the next free physical address.
 */
physical_addr_t pg_complete_paging(physical_addr_t page_aligned_free,
                                   struct mmap_entry *entries, uint32_t num);

#endif /* PAGING_H */
