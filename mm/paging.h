#ifndef PAGING_H
#define PAGING_H

#include <kernel/base_types.h>

/* Init paging */
physical_addr_t init_paging(physical_addr_t page_aligned_free);

#endif // PAGING_H
