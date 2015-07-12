#ifndef STDIO_H
#define STDIO_H

#include <kernel/base.h>
#include <lib/stdarg.h>

/*
 * Writes at most size - 1 of the characters printed into the output string(
 * null-terminated).
 * Returns the number of characters printed(not including the final '\0').
 */
int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);

#endif /* STDIO_H */
