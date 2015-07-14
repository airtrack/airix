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

static size_t utoa_s(unsigned int v, unsigned int base, char *buf, size_t len)
{
    char data[32];
    size_t data_len = 0;

    if (base > 16 || base < 2)
        return 0;

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
        return 0;

    /* Reverse it */
    reverse(data, data_len);

    memcpy(buf, data, data_len);
    buf[data_len] = '\0';
    return data_len;
}

static size_t itoa_s(int i, unsigned int base, char *buf, size_t len)
{
    unsigned int v;

    if (base == 10 && i < 0)
    {
        if (len < 1)
            return 0;

        v = -i;
        len = utoa_s(v, base, buf + 1, len - 1);
        if (len == 0)
            return 0;

        *buf = '-';
        return len + 1;
    }

    v = i;
    return utoa_s(v, base, buf, len);
}

/*
 * Copy at most n characters from src into dst, returns the number of
 * characters has been copy.
 */
static size_t copy_str(char *dst, const char *src, size_t n)
{
    char *start = dst;

    while (n-- > 0 && *src)
        *dst++ = *src++;

    return dst - start;
}

static inline size_t fill_padding(char *buf, size_t size, char padding,
                                  size_t padding_len)
{
    char *ptr = buf;
    char *end = buf + size;

    while (ptr < end && padding_len > 0)
    {
        *ptr++ = padding;
        --padding_len;
    }

    return ptr - buf;
}

static inline size_t copy_with_padding(char *buf, size_t size, const char *str,
                                       size_t len, bool left, char padding,
                                       size_t width)
{
    size_t copyed = 0;
    size_t filled = 0;
    size_t padding_len = 0;

    if (len == 0) len = strlen(str);

    padding_len = width > len ? width - len : 0;

    /* Fill padding if it is right adjustment */
    if (!left)
    {
        filled = fill_padding(buf, size, padding, padding_len);
        buf += filled;
        size -= filled;
    }

    copyed = copy_str(buf, str, size);
    buf += copyed;
    size -= copyed;

    /* Fill padding if it is left adjustment */
    if (left)
        filled = fill_padding(buf, size, padding, padding_len);

    return copyed + filled;
}

static inline size_t snprint_int(char *buf, size_t size, int i,
                                 unsigned int base, bool left,
                                 char padding, size_t width)
{
    char str[33];
    size_t len = itoa_s(i, base, str, sizeof(str));

    if (len == 0)
        return 0;

    return copy_with_padding(buf, size, str, len, left, padding, width);
}

static inline size_t snprint_uint(char *buf, size_t size, unsigned int i,
                                  unsigned int base, bool left,
                                  char padding, size_t width)
{
    char str[33];
    size_t len = utoa_s(i, base, str, sizeof(str));

    if (len == 0)
        return 0;

    return copy_with_padding(buf, size, str, len, left, padding, width);
}

static inline size_t snprint_pointer(char *buf, size_t size, void *ptr,
                                     bool left, char padding, size_t width)
{
    char str[35] = "0x";
    size_t len = utoa_s((unsigned int)ptr, 16, str + 2, sizeof(str) - 2);

    if (len == 0)
        return 0;

    len += 2;
    return copy_with_padding(buf, size, str, len, left, padding, width);
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
    char *cur = str;
    char *end = str + size;

    while (*fmt && cur < end)
    {
        if (*fmt == '%')
        {
            char c = *++fmt;
            char padding = ' ';
            bool left = false;
            size_t width = 0;

            /* Zero or more flags */
            while (c == '-' || c == ' ' || c == '0')
            {
                switch (c)
                {
                case '-': left = true; break;
                case ' ': padding = ' '; break;
                case '0': padding = '0'; break;
                }
                c = *++fmt;
            }

            /* A - override a 0 */
            if (left) padding = ' ';

            /* Get width if c is digit */
            while (c >= '0' && c <= '9')
            {
                width = width * 10 + c - '0';
                c = *++fmt;
            }

            switch (c)
            {
            case 'c':
                *cur++ = va_arg(ap, int);
                ++fmt;
                break;
            case 's':
                cur += copy_with_padding(cur, end - cur, va_arg(ap, char *),
                                         0, left, padding, width);
                ++fmt;
                break;
            case 'd':
                cur += snprint_int(cur, end - cur, va_arg(ap, int),
                                   10, left, padding, width);
                ++fmt;
                break;
            case 'p':
                cur += snprint_pointer(cur, end - cur, va_arg(ap, void *),
                                       left, padding, width);
                ++fmt;
                break;
            case 'x':
                cur += snprint_uint(cur, end - cur, va_arg(ap, unsigned int),
                                    16, left, padding, width);
                ++fmt;
                break;
            case 'u':
                cur += snprint_uint(cur, end - cur, va_arg(ap, unsigned int),
                                    10, left, padding, width);
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
