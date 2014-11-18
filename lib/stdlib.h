#ifndef STDLIB_H
#define STDLIB_H

#include <kernal/base_types.h>

/* Sort an array of n objects, the initial member of which is
 * pointed to by base, the size of each object is specified by width.
 * The comparison function return an integer less than, equal to, or
 * greater than zero if the first argument is less than, equal to, or
 * greater than the second. */
void qsort(void *base, size_t n, size_t width,
        int (*compare)(const void *, const void *));

#endif // STDLIB_H
