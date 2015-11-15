#ifndef STRING_H
#define STRING_H

#include <stddef.h>

/*
 * Copy n bytes from memory area src to memory area dst.
 * Defined in memory.s
 */
void * memcpy(void *restrict dst, const void *restrict src, size_t n);

/*
 * Write len bytes of value c to the string b.
 * Defined in memory.s
 */
void * memset(void *b, int c, size_t len);

/*
 * Returns the number of characters that precede the terminating NUL character.
 * Defined in string.c
 */
size_t strlen(const char *s);

/*
 * Copies the string src to dst(including the terminating '\0' character).
 * Returns dst.
 */
char * strcpy(char *dst, const char *src);

#endif /* STRING_H */
