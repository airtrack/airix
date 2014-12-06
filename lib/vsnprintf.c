#include <lib/stdio.h>
#include <lib/string.h>
#include <stdbool.h>

static void reverse(char *buf, int len)
{
    int l = 0, h = len - 1;
    while (l < h)
    {
        char c = buf[l];
        buf[l++] = buf[h];
        buf[h--] = c;
    }
}

static bool utoa_s(unsigned int v, unsigned int base, char *buf, int len)
{
    char data[32];
    int data_len = 0;

    if (base > 16 || base < 2)
        return false;

    /* Convert to string */
    do
    {
        unsigned int r = v % base;
        v /= base;
        if (r < 10)
            data[data_len++] = '0' + r;
        else
            data[data_len++] = 'A' + r - 10;
    } while (v != 0);

    if (len < data_len + 1)
        return false;

    /* Reverse it */
    reverse(data, data_len);

    memcpy(buf, data, data_len);
    buf[data_len] = '\0';
    return true;
}

static bool itoa_s(int i, unsigned int base, char *buf, int len)
{
    unsigned int v;

    if (base == 10 && i < 0)
    {
        v = -i;
        if (utoa_s(v, base, buf + 1, len - 1))
        {
            *buf = '-';
            return true;
        }
        return false;
    }
    else
    {
        v = i;
        return utoa_s(v, base, buf, len);
    }
}

/* Copy at most n characters from src into dst, returns the number of
 * characters has been copy. */
static int copy_str(char *dst, const char *src, size_t n)
{
    char *start = dst;
    while (n-- > 0 && *src)
        *dst++ = *src++;
    return dst - start;
}

static inline int snprint_int(char *buf, size_t n, int i,
        unsigned int base)
{
    char str[33];
    if (itoa_s(i, base, str, sizeof(str)))
        return copy_str(buf, str, n);
    else
        return 0;
}

static inline int snprint_uint(char *buf, size_t n, unsigned int i,
        unsigned int base)
{
    char str[33];
    if (utoa_s(i, base, str, sizeof(str)))
        return copy_str(buf, str, n);
    else
        return 0;
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
    char *cur = str;
    char *end = str + size;

    while (*fmt && cur < end)
    {
        if (*fmt == '%')
        {
            switch (*++fmt)
            {
                case 'c':
                    *cur++ = va_arg(ap, int);
                    ++fmt;
                    break;
                case 's':
                    cur += copy_str(cur, va_arg(ap, char *), end - cur);
                    ++fmt;
                    break;
                case 'd':
                    cur += snprint_int(cur, end - cur, va_arg(ap, int), 10);
                    ++fmt;
                    break;
                case 'x':
                    cur += snprint_int(cur, end - cur, va_arg(ap, int), 16);
                    ++fmt;
                    break;
                case 'u':
                    cur += snprint_uint(cur, end - cur,
                            va_arg(ap, unsigned int), 10);
                    ++fmt;
                    break;
                case '%':
                    *cur++ = *fmt++;
                    break;
            }
        }
        else
        {
            *cur++ = *fmt++;
        }
    }

    if (cur < end)
    {
        *cur = '\0';
        return cur - str;
    }
    else if (end > str)
    {
        *--end = '\0';
        return end - str;
    }
    else
    {
        return 0;
    }
}
