#ifndef BASE_TYPES_H
#define BASE_TYPES_H

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
#define VIRTUAL_TO_PHYSICAL(vaddr) (vaddr - KERNEL_BASE)

#endif // BASE_TYPES_H
