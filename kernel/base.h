#ifndef BASE_H
#define BASE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t physical_addr_t;

#define KERNEL_BASE 0xC0000000
#define VIRTUAL_TO_PHYSICAL(vaddr) ((vaddr) - KERNEL_BASE)
#define PHYSICAL_TO_VIRTUAL(paddr) ((paddr) + KERNEL_BASE)

#define CAST_VIRTUAL_TO_PHYSICAL(addr) \
    VIRTUAL_TO_PHYSICAL((physical_addr_t)(addr))
#define CAST_PHYSICAL_TO_VIRTUAL(addr) \
    (void *)PHYSICAL_TO_VIRTUAL((addr))
#define CAST_P2V_OR_NULL(addr) \
    ((addr) ? CAST_PHYSICAL_TO_VIRTUAL(addr) : NULL)

#define ALIGN_MASK(x, mask)  \
    (((x) + (mask)) & ~(mask))

#define ALIGN(x, align) \
    ALIGN_MASK(x, (align) - 1)

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define KMIN(a, b) ((a) < (b) ? (a) : (b))
#define KMAX(a, b) ((a) > (b) ? (a) : (b))

#endif /* BASE_H */
