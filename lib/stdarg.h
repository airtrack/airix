#ifndef STDARG_H
#define STDARG_H

typedef char * va_list;

#define _INT_SIZE_OF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))

#define va_start(ap, v) (ap = (va_list)(&(v)) + _INT_SIZE_OF(v))

#define va_arg(ap, t) (*(t *)((ap += _INT_SIZE_OF(t)) - _INT_SIZE_OF(t)))

#define va_end(ap) (ap = (va_list)0)

#define va_copy(dst, src) (dst = src)

#endif /* STDARG_H */
