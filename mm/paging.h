#ifndef PAGING_H
#define PAGING_H

#include <mm/pmm.h>
#include <mm/vmm.h>
#include <kernel/base.h>

/*
 * Init paging, the first step.
 * Returns the next free physical address.
 * This function should run in physical address space, when it returns,
 * kernel is running in virtual address space.
 */
physical_addr_t pg_init_paging(physical_addr_t page_aligned_free);

/*
 * Complete paging, the second step.
 * Returns the next free physical address.
 */
physical_addr_t pg_complete_paging(physical_addr_t page_aligned_free,
                                   struct mmap_entry *entries, uint32_t num);

/*
 * Copy kernel space to 'vaddr_space'.
 */
void pg_copy_kernel_space(struct page_directory *vaddr_space);

#endif /* PAGING_H */
