#include <kernel/base_types.h>
#include <lib/string.h>

static void swap(void *left, void *right, size_t n)
{
    char *l = left;
    char *r = right;
    char c;

    if (left == right) return;

    for (size_t i = 0; i < n; ++i)
    {
        c = l[i];
        l[i] = r[i];
        r[i] = c;
    }
}

static void insert_sort(void *base, size_t n, size_t width,
        int (*compare)(const void *, const void *))
{
    for (size_t i = 1; i < n; ++i)
    {
        size_t j = i;
        while (j > 0 && compare(base + j * width, base + (j - 1) * width) < 0)
        {
            swap(base + j * width, base + (j - 1) * width, width);
            --j;
        }
    }
}

void qsort(void *base, size_t n, size_t width,
        int (*compare)(const void *, const void *))
{
    char pivot[width];
    char *pbase = base;
    size_t i = 1, l = 0, g = 1;

    if (n <= 10) return insert_sort(base, n, width, compare);

    memcpy(pivot, base, width);

    /* Partition, [0, l) < pivot, [l, g) == pivot, [g, n) > pivot */
    while (i < n)
    {
        int result = compare(pbase + i * width, pivot);
        if (result < 0)
        {
            swap(pbase + l * width, pbase + i * width, width);
            ++l;
        }
        else if (result == 0)
        {
            swap(pbase + g * width, pbase + i * width, width);
            ++g;
            ++i;
        }
        else
        {
            ++i;
        }
    }

    qsort(pbase, l, width, compare);
    qsort(pbase + g * width, n - g, width, compare);
}
