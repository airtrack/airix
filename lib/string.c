#include <string.h>

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p)
        ++p;
    return p - s;
}

char * strcpy(char *dst, const char *src)
{
    char *p = dst;
    while ((*p++ = *src++));
    return dst;
}
