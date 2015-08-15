#ifndef BASE_H
#define BASE_H

#include <stdbool.h>

#define NULL (void *)0

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef uint32_t size_t;

typedef uint32_t physical_addr_t;

#define KERNEL_BASE 0xC0000000
#define VIRTUAL_TO_PHYSICAL(vaddr) ((vaddr) - KERNEL_BASE)
#define PHYSICAL_TO_VIRTUAL(paddr) ((paddr) + KERNEL_BASE)

#define CAST_VIRTUAL_TO_PHYSICAL(addr) \
    VIRTUAL_TO_PHYSICAL((physical_addr_t)(addr))
#define CAST_PHYSICAL_TO_VIRTUAL(addr) \
    (void *)PHYSICAL_TO_VIRTUAL((addr))

#define ALIGN_MASK(x, mask)  \
    (((x) + (mask)) & ~(mask))

#define ALIGN(x, align) \
    ALIGN_MASK(x, (align) - 1)

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define KMIN(a, b) ((a) < (b) ? (a) : (b))
#define KMAX(a, b) ((a) > (b) ? (a) : (b))

#endif /* BASE_H */
