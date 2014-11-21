#ifndef STRING_H
#define STRING_H

#include <kernel/base_types.h>

/* Copy n bytes from memory area src to memory area dst.
 * Defined in memory.s */
void * memcpy(void *restrict dst, const void *restrict src, size_t n);

#endif // STRING_H
